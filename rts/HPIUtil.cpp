//
//  HPIUtil.DLL
//  Routines for accessing HPI files

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "HPIUtil.h"

#include "zlib.h"


/*
 * My minor performance mods, disable if they cause issues
 * - Chris Han <xiphux@gmail.com>
 */
#define MODS

#ifdef MODS
#define div2(n,d)	((n)>>(d))
#define mod2(n,d)	((n)&((d)-1))
#ifdef __GNUC__
#define likely(x)	__builtin_expect(!!(x),1)
#define unlikely(x)	__builtin_expect(!!(x),0)
#else
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif
#else
#define div2(n,d)	((n)/(1<<(d)))
#define mod2(n,d)	((n)%(d))
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif /* MODS */
/*
 * End mods
 */


#define HEX_HAPI 0x49504148
// 'BANK'
#define HEX_BANK 0x4B4E4142
// 'SQSH'
#define HEX_SQSH 0x48535153
#define HEX_MARKER 0x00010000

#define OUTBLOCKSIZE 16384

#define INBLOCKSIZE (65536+17)

typedef struct _HPIHEADER {
	unsigned int HPIMarker;              /* 'HAPI' */
	unsigned int SaveMarker;             /* 'BANK' if savegame */
	unsigned int DirectorySize;          /* Directory size */
	unsigned int Key;                    /* decode key */
	unsigned int Start;                  /* offset of directory */
} HPIHEADER;

typedef struct _HPIENTRY {
	unsigned int NameOffset;
	unsigned int CountOffset;
	unsigned char Flag;
} HPIENTRY;

typedef struct _HPICHUNK {
	unsigned int Marker;            /* always 0x48535153 (SQSH) */
	unsigned char Unknown1;          /* I have no idea what these mean */
	unsigned char CompMethod;				/* 1=lz77 2=zlib */
	unsigned char Encrypt;           /* Is the chunk encrypted? */
	unsigned int CompressedSize;    /* the length of the compressed data */
	unsigned int DecompressedSize;  /* the length of the decompressed data */
	unsigned int Checksum;          /* check sum */
} HPICHUNK;

typedef struct _HPIFILE HPIFILE;

typedef struct _DIRENTRY {
	struct _DIRENTRY *Next;
	struct _DIRENTRY *Prev;
	struct _DIRENTRY *FirstSub;
	struct _DIRENTRY *LastSub;
	int dirflag;
	int count;
	int *DirOffset;
	char* Name;
	char* FileName;
	int memflag;
} DIRENTRY;

typedef struct _TREENODE {
	struct _TREENODE *tree_l;
	struct _TREENODE *tree_r;
	int tree_b;
	void *tree_p;
} TREENODE;

typedef struct _PACKFILE {
	char* FileName;
	DIRENTRY *Root;
	int DirSize;
	int Key;
	FILE *HPIFile;
	char* Window;
	int WinLen;
	int WinOfs;
	int WIn;
	int WOut;
	int ChunkPtr;
	int BlockSize;
	TREENODE *SearchTree;
	TREENODE *TreeArray;
	int NextNode;
	HPICALLBACK CallBack;
	int FileCount;
	int FileCountTotal;
	int FileBytes;
	int FileBytesTotal;
	int TotalBytes;
	int TotalBytesTotal;
	int Stop;
} PACKFILE;

static const char szTrailer[] = "HPIPack by Joe D (joed@cws.org) FNORD Total Annihilation Copyright 1997 Cavedog Entertainment";
static char DirPath[256];
static HPIENTRY *DirEntry;

static inline void* GetMem(const int size, const int zero)
{
	if (zero)
		return calloc(1,size);
	else
		return malloc(size);
}

static inline void FreeMem(void* x)
{
	free(x);
}

static inline char* DupString(const char* x)
{
	char* s = (char*)GetMem(strlen(x)+1, false);
	strcpy(s, x);
	return s;
}

/*  tree stuff  */

/* as_tree - tree library for as
 * vix 14dec85 [written]
 * vix 02feb86 [added tree balancing from wirth "a+ds=p" p. 220-221]
 * vix 06feb86 [added TreeDestroy()]
 * vix 20jun86 [added TreeDelete per wirth a+ds (mod2 v.) p. 224]
 * vix 23jun86 [added Delete2 uar to add for replaced nodes]
 * vix 22jan93 [revisited; uses RCS, ANSI, POSIX; has bug fixes]
 */


/* This program text was created by Paul Vixie using examples from the book:
 * "Algorithms & Data Structures," Niklaus Wirth, Prentice-Hall, 1986, ISBN
 * 0-13-022005-1. This code and associated documentation is hereby placed
 * in the public domain.
 */

typedef int (*TREECOMPFUNC)(PACKFILE *Pack, const void *k1, const void *k2);

static inline TREENODE *NewNode(PACKFILE *Pack)
{
	return Pack->TreeArray+(Pack->NextNode++);
}

static void Sprout(PACKFILE *Pack, TREENODE **ppr, const void *p_data, int *pi_balance, TREECOMPFUNC TreeComp)
{
	/* are we grounded?  if so, add the node "here" and set the rebalance
	 * flag, then exit.
	 */
	if (!*ppr) {
		*ppr = (TREENODE *) NewNode(Pack);
		(*ppr)->tree_l = NULL;
		(*ppr)->tree_r = NULL;
		(*ppr)->tree_b = 0;
		(*ppr)->tree_p = (void*)p_data;
		*pi_balance = true;
		return;
	}

	/* compare the data using routine passed by caller.
	 */
	int cmp = TreeComp(Pack, p_data, (*ppr)->tree_p);

	/* if LESS, prepare to move to the left.
	 */
	if (cmp < 0) {
		Sprout(Pack, &(*ppr)->tree_l, p_data, pi_balance, TreeComp);
		if (*pi_balance) {  /* left branch has grown longer */
			switch ((*ppr)->tree_b) {
				case 1: /* right branch WAS longer; balance is ok now */
					(*ppr)->tree_b = 0;
					*pi_balance = false;
					break;
				case 0: /* balance WAS okay; now left branch longer */
					(*ppr)->tree_b = -1;
					break;
				case -1:
					/* left branch was already too long. rebalnce */
					TREENODE *p1 = (*ppr)->tree_l;
					if (p1->tree_b == -1) { /* LL */
						(*ppr)->tree_l = p1->tree_r;
						p1->tree_r = *ppr;
						(*ppr)->tree_b = 0;
						*ppr = p1;
					} else {      /* double LR */
						TREENODE *p2 = p1->tree_r;
						p1->tree_r = p2->tree_l;
						p2->tree_l = p1;
          
						(*ppr)->tree_l = p2->tree_r;
						p2->tree_r = *ppr;
          
						if (p2->tree_b == -1)
							(*ppr)->tree_b = 1;
						else
							(*ppr)->tree_b = 0;
          
						if (p2->tree_b == 1)
							p1->tree_b = -1;
						else
							p1->tree_b = 0;
						*ppr = p2;
					}
					(*ppr)->tree_b = 0;
					*pi_balance = false;
					break;
			} 
		}
		return;
	}

	/* if MORE, prepare to move to the right.
	 */
	if (cmp > 0) {
		Sprout(Pack, &(*ppr)->tree_r, p_data, pi_balance, TreeComp);
		if (*pi_balance) {  /* right branch has grown longer */
			switch ((*ppr)->tree_b) {
				case -1:
					(*ppr)->tree_b = 0;
					*pi_balance = false;
					break;
				case 0:
					(*ppr)->tree_b = 1;
					break;
				case 1:
					TREENODE *p1 = (*ppr)->tree_r;
					if (p1->tree_b == 1) {  /* RR */
						(*ppr)->tree_r = p1->tree_l;
						p1->tree_l = *ppr;
						(*ppr)->tree_b = 0;
						*ppr = p1;
					} else {      /* double RL */
						TREENODE *p2 = p1->tree_l;
						p1->tree_l = p2->tree_r;
						p2->tree_r = p1;

						(*ppr)->tree_r = p2->tree_l;
						p2->tree_l = *ppr;

						if (p2->tree_b == 1)
							(*ppr)->tree_b = -1;
						else
							(*ppr)->tree_b = 0;

						if (p2->tree_b == -1)
							p1->tree_b = 1;
						else
							p1->tree_b = 0;

						*ppr = p2;
					}
					(*ppr)->tree_b = 0;
					*pi_balance = false;
					break;
			} 
		}
		return;
	}

	/* not less, not more: this is the same key!  replace...
	 */
	*pi_balance = false;
	(*ppr)->tree_p = (void*)p_data;
}

static void BalanceLeft(const PACKFILE *Pack, TREENODE **ppr_p, int *pi_balance)
{
	int b1, b2;

	switch ((*ppr_p)->tree_b) {
		case -1:
			(*ppr_p)->tree_b = 0;
			break;
		case 0:
			(*ppr_p)->tree_b = 1;
			*pi_balance = false;
			break;
		case 1:
			TREENODE *p1 = (*ppr_p)->tree_r;
			int b1 = p1->tree_b;
			if (b1 >= 0) {
				(*ppr_p)->tree_r = p1->tree_l;
				p1->tree_l = *ppr_p;
				if (b1 == 0) {
					(*ppr_p)->tree_b = 1;
					p1->tree_b = -1;
					*pi_balance = false;
				} else {
					(*ppr_p)->tree_b = 0;
					p1->tree_b = 0;
				}
				*ppr_p = p1;
			} else {
				TREENODE *p2 = p1->tree_l;
				int b2 = p2->tree_b;
				p1->tree_l = p2->tree_r;
				p2->tree_r = p1;
				(*ppr_p)->tree_r = p2->tree_l;
				p2->tree_l = *ppr_p;
				if (b2 == 1)
					(*ppr_p)->tree_b = -1;
				else
					(*ppr_p)->tree_b = 0;
				if (b2 == -1)
					p1->tree_b = 1;
				else
					p1->tree_b = 0;
				*ppr_p = p2;
				p2->tree_b = 0;
			}
	}
}

static void BalanceRight(const PACKFILE *Pack, TREENODE **ppr_p, int *pi_balance)
{
	switch ((*ppr_p)->tree_b) {
		case 1: 
			(*ppr_p)->tree_b = 0;
			break;
		case 0:
			(*ppr_p)->tree_b = -1;
			*pi_balance = false;
			break;
		case -1:
			TREENODE *p1 = (*ppr_p)->tree_l;
			int b1 = p1->tree_b;
			if (b1 <= 0) {
				(*ppr_p)->tree_l = p1->tree_r;
				p1->tree_r = *ppr_p;
				if (b1 == 0) {
					(*ppr_p)->tree_b = -1;
					p1->tree_b = 1;
					*pi_balance = false;
				} else {
					(*ppr_p)->tree_b = 0;
					p1->tree_b = 0;
				}
				*ppr_p = p1;
			} else {
				TREENODE *p2 = p1->tree_r;
				int b2 = p2->tree_b;
				p1->tree_r = p2->tree_l;
				p2->tree_l = p1;
				(*ppr_p)->tree_l = p2->tree_r;
				p2->tree_r = *ppr_p;
				if (b2 == -1)
					(*ppr_p)->tree_b = 1;
				else
					(*ppr_p)->tree_b = 0;
				if (b2 == 1)
					p1->tree_b = -1;
				else
					p1->tree_b = 0;
				*ppr_p = p2;
				p2->tree_b = 0;
			}
	}
}

static void Delete3(const PACKFILE *Pack, TREENODE  **ppr_r, int  *pi_balance, TREENODE **ppr_q)
{
	if ((*ppr_r)->tree_r != NULL) {
		Delete3(Pack, &(*ppr_r)->tree_r, pi_balance, ppr_q);
		if (*pi_balance)
			BalanceRight(Pack, ppr_r, pi_balance);
	} else {
		(*ppr_q)->tree_p = (*ppr_r)->tree_p;
		*ppr_q = *ppr_r;
		*ppr_r = (*ppr_r)->tree_l;
		*pi_balance = true;
	}
}

static int Delete2(PACKFILE *Pack, TREENODE **ppr_p, TREECOMPFUNC TreeComp, const void *p_user, int *pi_balance)
{
	if (unlikely(*ppr_p == NULL))
		return false;

	int i_ret, i_comp = TreeComp(Pack, (*ppr_p)->tree_p, p_user);
	if (i_comp > 0) {
		i_ret = Delete2(Pack, &(*ppr_p)->tree_l, TreeComp, p_user, pi_balance);
		if (*pi_balance)
			BalanceLeft(Pack, ppr_p, pi_balance);
	} else if (i_comp < 0) {
		i_ret = Delete2(Pack, &(*ppr_p)->tree_r, TreeComp, p_user, pi_balance);
		if (*pi_balance)
			BalanceRight(Pack, ppr_p, pi_balance);
	} else {
		TREENODE *pr_q = *ppr_p;
		if (pr_q->tree_r == NULL) {
			*ppr_p = pr_q->tree_l;
			*pi_balance = true;
		} else if (pr_q->tree_l == NULL) {
			*ppr_p = pr_q->tree_r;
			*pi_balance = true;
		} else {
			Delete3(Pack, &pr_q->tree_l, pi_balance, &pr_q);
			if (*pi_balance)
				BalanceLeft(Pack, ppr_p, pi_balance);
		}
		//TreeFreeMem(Pack, pr_q); /* thanks to wuth@castrov.cuc.ab.ca */
		i_ret = true;
	}
	return i_ret;
}


static inline void TreeInit(PACKFILE *Pack)
{
	Pack->SearchTree = NULL;
	if (!Pack->TreeArray)
		Pack->TreeArray = (TREENODE*)malloc(65536 * sizeof(TREENODE));
	Pack->NextNode = 0;
}

static void *TreeSearch(PACKFILE *Pack, TREENODE **ppr_tree, TREECOMPFUNC TreeComp, const void *p_user)
{
	if (likely(*ppr_tree)) {
		int i_comp = TreeComp(Pack, p_user, (**ppr_tree).tree_p);
		if (i_comp > 0)
			return TreeSearch(Pack, &(**ppr_tree).tree_r, TreeComp, p_user);
		if (i_comp < 0)
			return TreeSearch(Pack, &(**ppr_tree).tree_l, TreeComp, p_user);

		/* not higher, not lower... this must be the one.
		 */
		return (**ppr_tree).tree_p;
	}

	/* grounded. NOT found.
	 */
	return NULL;
}

static inline void TreeAdd(PACKFILE *Pack, TREENODE **ppr_tree, TREECOMPFUNC TreeComp, const void *p_user)
{
	int i_balance = false;

	Sprout(Pack, ppr_tree, p_user, &i_balance, TreeComp);
}

static inline int TreeDelete(PACKFILE *Pack, TREENODE  **ppr_p, TREECOMPFUNC TreeComp, const void *p_user)
{
	int i_balance = false;

	return Delete2(Pack, ppr_p, TreeComp, p_user, &i_balance);
}

static inline void TreeDestroy(PACKFILE *Pack)
{
	Pack->NextNode = 0;
}


/* End tree stuff  */

static int ReadAndDecrypt(HPIFILE *hpi, const int fpos, char* buff, const int buffsize)
{
	int count;
	int tkey;
	int result;
	
	fseek(hpi->f, fpos, SEEK_SET); 
	if (unlikely(!(result=fread(buff,1,buffsize, hpi->f))))
		return 0;
	if (likely(hpi->Key)) {
		for (count = 0; count < result; count++) {
			tkey = (fpos + count) ^ hpi->Key;
			buff[count] = tkey ^ ~buff[count];
		}
	}
	return result;
}

static int ZLibDecompress(char *out, char *in, const HPICHUNK *Chunk)
{
	z_stream zs;

	zs.next_in = (Bytef*)in;
	zs.avail_in = Chunk->CompressedSize;
	zs.total_in = 0;

	zs.next_out = (Bytef*)out;
	zs.avail_out = 65536;
	zs.total_out = 0;

	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;

	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	int result;
	if (unlikely((result = inflateInit(&zs)) != Z_OK))
		return 0;

	if ((result = inflate(&zs, Z_FINISH)) != Z_STREAM_END)
		zs.total_out = 0;

	if ((result = inflateEnd(&zs)) != Z_OK)
		return 0;

	return zs.total_out;
}

static int LZ77Decompress(char *out, const char *in, const HPICHUNK *Chunk)
{
	char DBuff[4096];
	int inptr = 0;
	int outptr = 0;
	int work1 = 1;
	int work2 = 1;
	int work3 = in[inptr++];
	
	int done = false;
	while (!done) {
		if (!(work2 & work3)) {
			out[outptr++] = in[inptr];
			DBuff[work1] = in[inptr];
			work1 = (work1 + 1) & 0xFFF;
			inptr++;
		} else {
			int count = *((unsigned short *) (in+inptr));
			inptr += 2;
			int DPtr = count >> 4;
			if (DPtr == 0)
				return outptr;
			else {
				count = (count & 0x0f) + 2;
				if (count >= 0) {
					for (int x = 0; x < count; x++) {
						out[outptr++] = DBuff[DPtr];
						DBuff[work1] = DBuff[DPtr];
						DPtr = (DPtr + 1) & 0xFFF;
						work1 = (work1 + 1) & 0xFFF;
					}
				}
			}
		}
		work2 <<= 1;
		if (work2 & 0x0100) {
			work2 = 1;
			work3 = in[inptr++];
		}
	}
	return outptr;
}

static int Decompress(char *out, char *in, const HPICHUNK *Chunk)
{
	int Checksum = 0;
	for (int x = 0; x < Chunk->CompressedSize; x++) {
		Checksum += (unsigned char) in[x];
		if (Chunk->Encrypt)
			in[x] = (in[x] - x) ^ x;
	}

	if (unlikely(Chunk->Checksum != Checksum))
		return 0;

	switch (Chunk->CompMethod) {
		case 1 : return LZ77Decompress(out, in, Chunk);
		case 2 : return ZLibDecompress(out, in, Chunk);
		default : return 0;
	}
}

static char* DecodeFileToMem(HPIFILE *hpi, const HPIENTRY *Entry)
{
	if (unlikely(!Entry))
		return NULL;

	int Offset = *((int *) (hpi->d + Entry->CountOffset));
	int Length = *((int *) (hpi->d + Entry->CountOffset + 4));
	char FileFlag = *(hpi->d + Entry->CountOffset + 8);

	//if (FileFlag != 1)
	//	return NULL;

	char *WriteBuff = (char*)malloc(Length+1);
	if (unlikely(!WriteBuff))
		return NULL;
	WriteBuff[Length] = 0;

	if (FileFlag) {
		int DeCount = div2(Length,16);	/* Length/65536 */
		if (mod2(Length,65536))		/* Length%65536 */
			DeCount++;
		int DeLen = DeCount * sizeof(int);

		long *DeSize = (long*)GetMem(DeLen, true);

		ReadAndDecrypt(hpi, Offset, (char *) DeSize, DeLen);

		Offset += DeLen;
	
		int WritePtr = 0;

		for (int x = 0; x < DeCount; x++) {
			HPICHUNK *Chunk = (HPICHUNK*)GetMem(DeSize[x], true);
			ReadAndDecrypt(hpi, Offset, (char *) Chunk, DeSize[x]);
			Offset += DeSize[x];

			char *DeBuff = (char *) (Chunk+1);

			int WriteSize = Decompress(WriteBuff+WritePtr, DeBuff, Chunk);
			WritePtr += WriteSize;

			FreeMem(Chunk);
		}
		FreeMem(DeSize);
	} else
		ReadAndDecrypt(hpi, Offset, WriteBuff, Length);

	return WriteBuff;
}

void* HPIOpen(const char* FileName)
{
	return NULL;
	HPIHEADER Header;

	FILE *f = fopen(FileName, "r+");

	if (!f)
		return NULL;

	int BytesRead;
	if (unlikely(!(BytesRead = fread(&Header,1,sizeof(Header),f)))) {
		fclose(f);
		return NULL;
	}

	if (unlikely(BytesRead != sizeof(Header))) {
		fclose(f);
		return NULL;
	}

	if (Header.HPIMarker != HEX_HAPI) {  /* 'HAPI' */
		fclose(f);
		return NULL;
	}

	if (Header.SaveMarker != HEX_MARKER) {
		fclose(f);
		return NULL;
	}

	HPIFILE *hpi = (HPIFILE*)GetMem(sizeof(HPIFILE), true);

	hpi->f = f;
	if (Header.Key)
		hpi->Key = ~((Header.Key * 4)	| (Header.Key >> 6));
	else
		hpi->Key = 0;

	hpi->Start = Header.Start;
	hpi->d = (char*)GetMem(Header.DirectorySize, true);
	BytesRead = ReadAndDecrypt(hpi, Header.Start, hpi->d+Header.Start, Header.DirectorySize-Header.Start);
	if (unlikely(BytesRead != (Header.DirectorySize-Header.Start))) {
		FreeMem(hpi->d);
		FreeMem(hpi);
		fclose(f);
		return NULL;
	}
	return hpi;
}

static int ListDirectory(const HPIFILE *hpi, const int offset, const char* Path, const int Next, int This)
{
	int *Entries = (int *) (hpi->d + offset);
	int *EntryOffset = Entries + 1;
	HPIENTRY *Entry = (HPIENTRY *) (hpi->d + *EntryOffset);

	for (int count = 0; count < *Entries; count++) {
		char MyPath[1024];
		char *Name = hpi->d + Entry->NameOffset;
		strcpy(MyPath, Path);
		if (*Path)
			strcat(MyPath, "\\");
		strcat(MyPath, Name);

		if (This == Next) {
			if (!DirEntry) {
				DirEntry = Entry;
				strcpy(DirPath, MyPath);
			}
			return This;
		}
		This++;
		if (Entry->Flag == 1)
			This = ListDirectory(hpi, Entry->CountOffset, MyPath, Next, This);
		Entry++;
	}
	return This;
}

bool HPIGetFiles(const HPIFILE *hpi, const int Next, char* Name, int* Type, int* Size)
{
	*Name = 0;
	*Type = 0;
	*Size = 0;

	DirEntry = NULL;
	DirPath[0] = 0;

	int result = ListDirectory(hpi, hpi->Start, "", Next, 0);
	if (unlikely(!DirEntry))
		return 0;

	strcpy(Name, DirPath);
	int *Count = (int *) (hpi->d + DirEntry->CountOffset);
	*Type = DirEntry->Flag;
	if (!*Type)
		Count++;
	*Size = *Count;

	return result+1;
}

static int EnumDirectory(const HPIFILE *hpi, const int offset, const char* Path, const int Next, const char* DirName, int This)
{
	int *Entries = (int *) (hpi->d + offset);
	int *EntryOffset = Entries + 1;
	HPIENTRY *Entry = (HPIENTRY *) (hpi->d + *EntryOffset);

	//MessageBox(NULL, Path, "Searching", MB_OK);

	for (int count = 0; count < *Entries; count++) {
		if (!strcasecmp(Path, DirName)) {
  			if (This >= Next) {
	  			if (!DirEntry) {
					DirEntry = Entry;
					//MessageBox(NULL, hpi->d + Entry->NameOffset, "Found", MB_OK);
					//strcpy(DirPath, MyPath);
				}
				return This;
			}
			This++;
		}
		if (Entry->Flag == 1)	{
			char MyPath[1024];
			strcpy(MyPath, Path);
			if (*Path)
				strncat(MyPath, "\\",1);
			strncat(MyPath, (hpi->d + Entry->NameOffset),strlen(hpi->d+Entry->NameOffset));
			//MessageBox(NULL, MyPath, "recursing", MB_OK);
			This = EnumDirectory(hpi, Entry->CountOffset, MyPath, Next, DirName, This);
			if (DirEntry)
				return This;
		}
		Entry++;
	}
	return This;
}

bool HPIDir(const HPIFILE *hpi, const int Next, const char* DirName, char* Name, int* Type, int* Size)
{
	*Name = 0;
	*Type = 0;
	*Size = 0;

	DirEntry = NULL;

	int result = EnumDirectory(hpi, hpi->Start, "", Next, DirName, 0);
	if (!DirEntry)
		return 0;

	strcpy(Name, (hpi->d + DirEntry->NameOffset));
	int *Count = (int *) (hpi->d + DirEntry->CountOffset);
	*Type = DirEntry->Flag;
	if (!*Type)
		Count++;
	*Size = *Count;

	return result+1;
}

bool HPIClose(HPIFILE *hpi)
{
	if (unlikely(!hpi))
		return 0;

	if (likely(hpi->f))
		fclose(hpi->f);
	if (hpi->d)
		FreeMem(hpi->d);
	FreeMem(hpi);
	return 0;
}

static char* SearchForFile(HPIFILE *hpi, const int offset, const char* Path, const char* FName)
{
	int *Entries = (int *) (hpi->d + offset);
	int *EntryOffset = Entries + 1;
	HPIENTRY *Entry = (HPIENTRY *) (hpi->d + *EntryOffset);

	for (int count = 0; count < *Entries; count++) {
		char MyPath[1024];
		char *Name = hpi->d + Entry->NameOffset;
		strcpy(MyPath, Path);
		if (*Path)
			strcat(MyPath, "\\");
		strcat(MyPath, Name);

		if (Entry->Flag == 1)	{
			char *result = SearchForFile(hpi, Entry->CountOffset, MyPath, FName);
			if (result)
				return result;
		} else {
			if (!strcasecmp(MyPath, FName)) {
				DirEntry = Entry;
				return DecodeFileToMem(hpi, Entry);
			}
		}
		Entry++;
	}
	return NULL;
}

char* HPIOpenFile(HPIFILE *hpi, const char* FileName)
{
	DirEntry = NULL;
	return SearchForFile(hpi, hpi->Start, "", FileName);
}

void HPIGet(char* Dest, char* FileHandle, const int offset, const int bytecount)
{
	memmove(Dest, FileHandle+offset, bytecount);
}

bool HPICloseFile(char* FileHandle)
{
	if (likely(FileHandle))
		free(FileHandle);
	return 0;
}

static bool HPIExtractFile(HPIFILE *hpi, const char* FileName, const char* ExtractName)
{
	DirEntry = NULL;
	char *data = SearchForFile(hpi, hpi->Start, "", FileName);
	if (unlikely(!data))
		return 0;

	int *Size = (int *) (hpi->d + DirEntry->CountOffset);
	if (!DirEntry->Flag)
		Size++;
	else {
		HPICloseFile(data);
		return 0;
	}

	FILE *f = fopen(ExtractName, "w");

	if (unlikely(!f)) {
		HPICloseFile(data);
		return 0;
	}

	int Written = fwrite(data,1, *Size, f);
	fclose(f);

	HPICloseFile(data);

	return 1;
}

static inline void* HPICreate(const char* FileName, HPICALLBACK CallBack)
{
	PACKFILE *Pack = (PACKFILE*)GetMem(sizeof(PACKFILE), true);
	Pack->FileName = DupString(FileName);
	Pack->Root = (DIRENTRY*)GetMem(sizeof(DIRENTRY), true);
	Pack->CallBack = CallBack;
	return Pack;
}

static DIRENTRY *CreatePath(DIRENTRY *dir, char* FName)
{
	char Path[PATH_MAX];
	char *p = FName;
	DIRENTRY *d = dir->FirstSub;
	while (*p) {
		char *n = Path;
		while (*p && (likely(*p != '\\')))
			*n++ = *p++;
		*n = 0;
		if (*p)
			p++;

		while (d) {
			if (strcasecmp(d->Name, Path) == 0) {
				if (d->dirflag)
					break;
				else
					return NULL; // found, but not a directory
			}
			d = d->Next;
		}
		if (d)
			dir = d;
		else {
			DIRENTRY *e = (DIRENTRY*)GetMem(sizeof(DIRENTRY), true);
			e->dirflag = true;
			e->Name = DupString(Path);
			if (dir->LastSub) {
				dir->LastSub->Next = e;
				e->Prev = dir->LastSub;
				dir->LastSub = e;
			} else {
				dir->FirstSub = e;
				dir->LastSub = e;
			}
			dir = e;
		}
		d = dir->FirstSub;
	}
	return dir;
}

static inline bool HPICreateDirectory(PACKFILE *Pack, char* DirName)
{
	return (CreatePath(Pack->Root, DirName) != NULL);
}

static bool HPIAddFile(PACKFILE *Pack, const char* HPIName, const char* FileName)
{
	char DirName[PATH_MAX];
	DIRENTRY *dir;
	char* TName;

	FILE *f = fopen(FileName, "rb");
	if (unlikely(!f))
		return false;
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	fclose(f);

	strcpy(DirName, HPIName);
	char *c = strrchr(DirName, '\\');
	if (c) {
		TName = c+1;
		*c = 0;
		dir = CreatePath(Pack->Root, DirName);
	} else {
		dir = Pack->Root;
		TName = DirName;
	}

	if (unlikely(!dir))
		return false;
 
	DIRENTRY *d = dir->FirstSub;
	while (d) {
		if (!strcasecmp(d->Name, TName))
			return false;
		d = d->Next;
	}

	Pack->FileCountTotal++;
	Pack->TotalBytesTotal += fsize;
	d = (DIRENTRY*)GetMem(sizeof(DIRENTRY), true);
	d->dirflag = false;
	d->count = fsize;
	d->Name = DupString(TName);
	d->FileName = DupString(FileName);
	d->memflag = false;
	if (dir->LastSub) {
		dir->LastSub->Next = d;
		d->Prev = dir->LastSub;
		dir->LastSub = d;
	} else {
		dir->FirstSub = d;
		dir->LastSub = d;
	}
	return true;
}

static bool HPIAddFileFromMemory(PACKFILE *Pack, const char* HPIName, char* FileBlock, const int fsize)
{
	char DirName[PATH_MAX];
	DIRENTRY *dir;
	char* TName;

	strcpy(DirName, HPIName);
	char *c = strrchr(DirName, '\\');
	if (c) {
		TName = c+1;
		*c = 0;
		dir = CreatePath(Pack->Root, DirName);
	} else {
		dir = Pack->Root;
		TName = DirName;
	}

	if (unlikely(!dir))
		return false;
 
	DIRENTRY *d = dir->FirstSub;
	while (d) {
		if (unlikely(!strcasecmp(d->Name, TName)))
			return false;
		d = d->Next;
	}

	Pack->FileCountTotal++;
	Pack->TotalBytesTotal += fsize;
	d = (DIRENTRY*)GetMem(sizeof(DIRENTRY), true);
	d->dirflag = false;
	d->count = fsize;
	d->Name = DupString(TName);
	d->FileName = FileBlock;
	d->memflag = true;
	if (dir->LastSub) {
		dir->LastSub->Next = d;
		d->Prev = dir->LastSub;
		dir->LastSub = d;
	} else {
		dir->FirstSub = d;
		dir->LastSub = d;
	}
	return true;
}

static int BuildDirectoryBlock(char* Directory, const DIRENTRY *dir, const int DStart, const int CMethod)
{
	int c = DStart;
	*((int *)(Directory+c)) = dir->count;
	c += sizeof(int);
	*((int *)(Directory+c)) = c+sizeof(int);
	c += sizeof(int);
	int nofs = c + (dir->count * sizeof(HPIENTRY));
	DIRENTRY *d = dir->FirstSub;
	while (d) {
		*((int *)(Directory+c)) = nofs;
		c += sizeof(int);
		char *n = d->Name;
		while (*n)
   			Directory[nofs++] = *n++;
		Directory[nofs++] = 0;
		*((int *)(Directory+c)) = nofs;
		c += sizeof(int);
		if (d->dirflag) {
			Directory[c++] = 1;
			nofs = BuildDirectoryBlock(Directory, d, nofs, CMethod);
		} else {
			Directory[c++] = 0;
			d->DirOffset = (int *) (Directory+nofs);
			nofs += sizeof(int);
			*((int *)(Directory+nofs)) = d->count;
			nofs += sizeof(int);
			Directory[nofs++] = CMethod;
		}
		d = d->Next;
	}
	return nofs;
}

static int EncryptAndWrite(PACKFILE *Pack, const int fpos, const void *b, const int buffsize)
{
	char *buff;
	if (Pack->Key) {
		buff = (char*)GetMem(buffsize, false);
		for (int count = 0; count < buffsize; count++) {
			int tkey = (fpos + count) ^ Pack->Key;
			buff[count] = tkey ^ ~((char *) b)[count];
		}
	} else
		buff = (char*)b;
	fseek(Pack->HPIFile, fpos, SEEK_SET);
	int result = fwrite(buff, 1, buffsize, Pack->HPIFile);
	if (unlikely(buff != b))
		FreeMem(buff);
	return result;
}

static int WriteChar(PACKFILE *Pack, int fpos, char *OutBlock, int *OutSize, const void *data, const int dsize, HPICHUNK *BlockHeader)
{
	char *d = (char*)data;
	int o = *OutSize;

	BlockHeader->CompressedSize += dsize;
	for (int x = 0; x < dsize; x++) {
		unsigned char n = (unsigned char) *d++;
		n = (n ^ o) + o;
		BlockHeader->Checksum += n;
		OutBlock[o++] = (char) n;
		if (o == OUTBLOCKSIZE) {
			EncryptAndWrite(Pack, fpos, OutBlock, o);
			fpos += o;
			o = 0;
		}
	}
	*OutSize = o;

	return fpos;
}

static int CompFunc(PACKFILE *Pack, const void *d1, const void *d2)
{
	int i;
	//int l;

	if (unlikely(d1 == d2))
		return 0;

	//  if (!d1)
	//    return -1;
	//  if (!d2)
	//    return 1;

	unsigned int p1 = (unsigned int) d1;
	unsigned int p2 = (unsigned int) d2;
	char *k1 = Pack->Window+p1;
	char *k2 = Pack->Window+p2;
	int result = 0;
	//l = 0;

	for (i = 0; i < 17; i++) {
		result = k1[i] - k2[i];
		if (result)
			break;
	}
	if ((Pack->WinLen <= i) && ((p2 & 0xFFF) != 0xFFF)) {
		Pack->WinLen = i;
		Pack->WinOfs = p2;
	}

	if (!result)
		result = p1-p2;

	return result;
}

static int SearchWindow(PACKFILE *Pack)
{
	Pack->WinLen = 1;
	Pack->WinOfs = 0;

	TreeSearch(Pack, &(Pack->SearchTree), CompFunc, (void *) Pack->ChunkPtr);

	if (unlikely(Pack->WinLen > Pack->BlockSize))
		Pack->WinLen = Pack->BlockSize;

	return (Pack->WinLen >= 2);
}

static int LZ77CompressChunk(PACKFILE *Pack, char* Chunk, HPICHUNK *BlockHeader, int fpos)
{
	char OutBlock[OUTBLOCKSIZE];
	int OutSize = 0;
	char data[17];
	
	Pack->BlockSize = BlockHeader->DecompressedSize;

	int mask = 0x01;
	char flags = 0;
	int dptr = 1;
	Pack->WIn = 0;
	Pack->WOut = 0;
	TreeInit(Pack);
	Pack->ChunkPtr = 0;
	Pack->Window = Chunk;

	do {
		//DumpMessage("\nBlockSize: 0x%X   WIn: 0x%X   WOut: 0x%X   ChunkPtr: 0x%X\n", BlockSize, WIn, WOut, ChunkPtr);
		if (SearchWindow(Pack)) {
			//DumpMessage("Matched offset: 0x%X   Length: 0x%X\n", WinOfs, WinLen);

			Pack->WinOfs &= 0xFFF;
			flags |= mask;
			int olpair = ((Pack->WinOfs+1) << 4) | (Pack->WinLen-2);
			data[dptr++] = LOBYTE(LOWORD(olpair));
			data[dptr++] = HIBYTE(LOWORD(olpair));
		} else {
			data[dptr++] = Chunk[Pack->ChunkPtr];
			Pack->WinLen = 1;
		}
		int Len = Pack->WinLen;
		Pack->BlockSize -= Len;

		for (int i = 0; i < Len; i++) {
			TreeAdd(Pack, &(Pack->SearchTree), CompFunc, (void *) (Pack->ChunkPtr));
			Pack->ChunkPtr++;
			Pack->WOut++;
			if (Pack->WOut > 4095) {
				TreeDelete(Pack, &(Pack->SearchTree), CompFunc, (void *) Pack->WIn);
				Pack->WIn++;
			}
		}

		if (mask == 0x80) {
			data[0] = flags;
			fpos = WriteChar(Pack, fpos, OutBlock, &OutSize, data, dptr, BlockHeader);
			mask = 0x01;
			flags = 0;
			dptr = 1;
		} else
			mask <<= 1;
	} while (Pack->BlockSize > 0);
  
	flags |= mask;
	data[dptr++] = 0;
	data[dptr++] = 0;
	data[0] = flags;

	fpos = WriteChar(Pack, fpos, OutBlock, &OutSize, data, dptr, BlockHeader);

	if (OutSize) {
		EncryptAndWrite(Pack, fpos, OutBlock, OutSize);
		fpos += OutSize;
	}

	TreeDestroy(Pack);
	return fpos;
}

static int ZLibCompressChunk(PACKFILE *Pack, char *Chunk, HPICHUNK *BlockHeader, int fpos)
{
	z_stream zs;

	char *out = (char*)GetMem(131072, false);

	zs.next_in = (Bytef*)Chunk;
	zs.avail_in = BlockHeader->DecompressedSize;
	zs.total_in = 0;

	zs.next_out = (Bytef*)out;
	zs.avail_out = 131072;
	zs.total_out = 0;

	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;

	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	int result;
	if (unlikely((result = deflateInit(&zs,Z_DEFAULT_COMPRESSION)) != Z_OK))
		return fpos;

	if ((result = deflate(&zs,Z_FINISH)) != Z_STREAM_END)
		return fpos;

	if (zs.total_out) {
		//d = (unsigned char *) out;
		for (int x = 0; (unsigned int) x < zs.total_out; x++) {
			unsigned char n = (unsigned char) out[x];
			if (BlockHeader->Encrypt)	{
				n = (n ^ x) + x;
				out[x] = n;
			}
			BlockHeader->Checksum += n;
		}

		EncryptAndWrite(Pack, fpos, out, zs.total_out);

		fpos += zs.total_out;
		BlockHeader->CompressedSize += zs.total_out;
	}

	deflateEnd(&zs);

	FreeMem(out);
	
	return fpos;
}

static int CompressFile(PACKFILE *Pack, int fpos, DIRENTRY *d, const int CMethod)
{
	FILE *InFile;

	Pack->FileCount++;
	Pack->FileBytes = 0;
	Pack->FileBytesTotal = d->count;
	if (Pack->CallBack)
		Pack->Stop = Pack->CallBack(d->FileName, d->Name, Pack->FileCount, Pack->FileCountTotal, Pack->FileBytes, Pack->FileBytesTotal, Pack->TotalBytes, Pack->TotalBytesTotal);
	if (Pack->Stop)
		return fpos;
	if (!d->memflag) {
		InFile = fopen(d->FileName, "rb");
		if (unlikely(!InFile))
  			return fpos;
	}

	*d->DirOffset = fpos;

	if (CMethod) {
		int BlockCount = div2(d->count,16);	/* d->count/65536 */
		if (mod2(d->count,65536))		/* d->count%65536 */
			BlockCount++;

		int bpos = fpos;

		int PtrBlockSize = BlockCount * sizeof(int);
		int *BlockPtr = (int*)GetMem(PtrBlockSize, true);

		EncryptAndWrite(Pack, bpos, BlockPtr, PtrBlockSize);

		fpos += PtrBlockSize;

		char *InBlock = (char*)GetMem(INBLOCKSIZE, false);

		int remain = d->count;
		int BlockNo = 0;

		HPICHUNK BlockHeader;
		while (remain) {
			BlockHeader.Marker = HEX_SQSH;
			BlockHeader.Unknown1 = 0x02;
			BlockHeader.CompMethod = CMethod;
			BlockHeader.Encrypt = 0x01;
			BlockHeader.CompressedSize = 0;
			BlockHeader.DecompressedSize = (remain > 65536 ? 65536 : remain);
			BlockHeader.Checksum = 0;

			EncryptAndWrite(Pack, fpos, &BlockHeader, sizeof(BlockHeader));
			int hpos = fpos;

			fpos += sizeof(BlockHeader);

			memset(InBlock,0, INBLOCKSIZE);
			if (d->memflag)
				memmove(InBlock, d->FileName+Pack->FileBytes, BlockHeader.DecompressedSize);
			else
				fread(InBlock, 1, BlockHeader.DecompressedSize, InFile);

			switch (CMethod) {
				case LZ77_COMPRESSION :
					LZ77CompressChunk(Pack, InBlock, &BlockHeader, fpos);
					break;
				case ZLIB_COMPRESSION :
					ZLibCompressChunk(Pack, InBlock, &BlockHeader, fpos);
					break;
				default :
					break;
			}

			BlockPtr[BlockNo] = BlockHeader.CompressedSize+sizeof(BlockHeader);

			EncryptAndWrite(Pack, hpos, &BlockHeader, sizeof(BlockHeader));

			fpos += BlockHeader.CompressedSize;

			remain -= BlockHeader.DecompressedSize;

			Pack->FileBytes += BlockHeader.DecompressedSize;
			Pack->TotalBytes += BlockHeader.DecompressedSize;
			if (Pack->CallBack) {
				Pack->Stop = Pack->CallBack(d->FileName, d->Name, Pack->FileCount, Pack->FileCountTotal, Pack->FileBytes, Pack->FileBytesTotal, Pack->TotalBytes, Pack->TotalBytesTotal);
				if (Pack->Stop)
					remain = 0;
			}
			BlockNo++;
		}

		EncryptAndWrite(Pack, bpos, BlockPtr, PtrBlockSize);
		FreeMem(BlockPtr);
		FreeMem(InBlock);
	} else {
		if (d->memflag)
			EncryptAndWrite(Pack, fpos, d->FileName+Pack->FileBytes, d->count);
		else {
			char *InBlock = (char*)GetMem(d->count, false);
			fread(InBlock, 1, d->count, InFile);
			EncryptAndWrite(Pack, fpos, InBlock, d->count);
			FreeMem(InBlock);
		}
		fpos += d->count;
		if (Pack->CallBack) {
			Pack->FileBytes += d->count;
			Pack->TotalBytes += d->count;
			Pack->Stop = Pack->CallBack(d->FileName, d->Name, Pack->FileCount, Pack->FileCountTotal, Pack->FileBytes, Pack->FileBytesTotal, Pack->TotalBytes, Pack->TotalBytesTotal);
		}
	}

	if (!d->memflag)
		fclose(InFile);
	return fpos;
}

static int CompressAndEncrypt(PACKFILE *Pack, int fpos, const DIRENTRY *dir, const int CMethod)
{
	DIRENTRY *d;

	d = dir->FirstSub;

	while (d) {
		if (d->dirflag)
			fpos = CompressAndEncrypt(Pack, fpos, d, CMethod);
		else
			fpos = CompressFile(Pack, fpos, d, CMethod);
		d = d->Next;
	}

	return fpos;
}

static inline void FreeDir(DIRENTRY *dir)
{
	DIRENTRY *n;

	while (dir) {
		FreeDir(dir->FirstSub);
		if (dir->Name)
			FreeMem(dir->Name);
		if (dir->FileName && !dir->memflag)
			FreeMem(dir->FileName);
		n = dir->Next;
		FreeMem(dir);
		dir = n;
	}
}

static inline void FreePack(PACKFILE *Pack)
{
	if (Pack->FileName)
		FreeMem(Pack->FileName);
	if (Pack->HPIFile)
		FreeMem(Pack->HPIFile);
	FreeDir(Pack->Root);
	FreeMem(Pack);
}

static int ScanFileNames(DIRENTRY *Start)
{
	int dsize = 0;
	int count = 0;
	DIRENTRY *This = Start->FirstSub;
	while (This) {
		count++;
		dsize += strlen(This->Name)+1;
		if (This->dirflag)
			dsize += ScanFileNames(This);
		else
			dsize += sizeof(HPIENTRY);
		This = This->Next;
	}
	Start->count = count;

	dsize += (2 * sizeof(int)) + (count * sizeof(HPIENTRY));

	return dsize;
}

static bool HPIPackArchive(PACKFILE *Pack, const int CMethod)
{
	Pack->HPIFile = fopen(Pack->FileName, "w+b");
	if (unlikely(!Pack->HPIFile)) {
		FreePack(Pack);
		return false;
	}

	Pack->DirSize = ScanFileNames(Pack->Root);

	Pack->DirSize += sizeof(HPIHEADER);
	char *Directory = (char*)GetMem(Pack->DirSize, true);

	HPIHEADER Header;
	Header.HPIMarker = HEX_HAPI;
	Header.SaveMarker = 0x00010000;
	Header.DirectorySize = Pack->DirSize;
	if (CMethod == LZ77_COMPRESSION)
		Header.Key = 0x7D;
	else
		Header.Key = 0;
	Header.Start = sizeof(HPIHEADER);
	memmove(Directory, &Header, sizeof(Header));

	if (Header.Key)
		Pack->Key = ~((Header.Key * 4)	| (Header.Key >> 6));
	else
		Pack->Key = 0;

	BuildDirectoryBlock(Directory, Pack->Root, sizeof(HPIHEADER), CMethod);

	fwrite(Directory, Pack->DirSize, 1, Pack->HPIFile);

	int endpos = CompressAndEncrypt(Pack, Pack->DirSize, Pack->Root, CMethod);

	fseek(Pack->HPIFile, endpos, SEEK_SET);

	fwrite(szTrailer, 1, strlen(szTrailer), Pack->HPIFile);

	EncryptAndWrite(Pack, Header.Start, Directory+Header.Start, Pack->DirSize-Header.Start);

	fclose(Pack->HPIFile);
	Pack->HPIFile = NULL;
	FreePack(Pack);
	return true;
}

static inline bool HPIPackFile(PACKFILE *Pack)
{
	return HPIPackArchive(Pack, LZ77_COMPRESSION);
}
