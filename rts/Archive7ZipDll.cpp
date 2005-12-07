#include "StdAfx.h"

extern "C" {
#include "7zip/7zCrc.h"
#include "7zip/7zIn.h"
}

#include "Archive7ZipDll.h"
#include <algorithm>
#include "SDL_types.h"
#include <ctype.h>
//#include "mmgr.h"

/*
 * Most of this code comes from the Client7z-example in the 7zip source distribution
 *
 * 7zip also includes a ansi c library for accessing 7zip files which seems to be much 
 * nicer to work with, but unfortunately it doesn't support folders inside archives
 * yet. But it should apparently be added soon. When it is, this class should probably
 * be rewritten to use that other library instead. I had written most of it before
 * I realised it didn't support directories anyway, so it should be quick. :)
 */

// {23170F69-40C1-278A-1000-000110050000}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

typedef struct _CFileInStream CFileInStream;

static SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
	CFileInStream *s = (CFileInStream *)object;
	size_t processedSizeLoc = fread(buffer, 1, size, s->File);
	if (processedSize != 0)
		*processedSize = processedSizeLoc;
	return SZ_OK;
}

static SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
	CFileInStream *s = (CFileInStream*)object;
	int res = fseek(s->File, (long)pos, SEEK_SET);
	if (!res)
		return SZ_OK;
	return SZE_FAIL;
}

CArchive7ZipDll::CArchive7ZipDll(const string& name) :
	CArchiveBuffered(name),
	curSearchHandle(1)
{
	SZ_RESULT res;

	archiveStream.File = fopen(name.c_str(),"rb");
	if (!archiveStream.File)
		return;

	archiveStream.InStream.Read = SzFileReadImp;
	archiveStream.InStream.Seek = SzFileSeekImp;

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	InitCrcTable();
	SzArDbExInit(&db);
	res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
	if (res != SZ_OK)
		return;
	for (UInt32 i = 0; i < db.Database.NumFiles; i++)
	{
		CFileItem *f = db.Database.Files + i;
		int size = (int)f->Size;
		
		if (size > 0) {
			string fileName = f->Name;
			transform(fileName.begin(), fileName.end(), fileName.begin(), (int (*)(int))tolower);

			FileData fd;
			fd.index = i;
			fd.size = size;
			fd.origName = f->Name;

			fileData[fileName] = fd;
		}
	}
}

CArchive7ZipDll::~CArchive7ZipDll(void)
{
	SzArDbExFree(&db, allocImp.Free);
	fclose(archiveStream.File);
}


ABOpenFile_t* CArchive7ZipDll::GetEntireFile(const string& fName)
{
	string fileName = fName;
	transform(fileName.begin(), fileName.end(), fileName.begin(), (int (*)(int))tolower);	

	if (fileData.find(fileName) == fileData.end())
		return NULL;

	FileData fd = fileData[fileName];

	ABOpenFile_t* of = new ABOpenFile_t;
	of->pos = 0;
	of->size = fd.size;
	of->data = (char*)malloc(of->size); 

	Uint32 blockIndex = 0xFFFFFFFF;
	Uint8 *outBuffer = 0;
	size_t outBufferSize = 0;

	SZ_RESULT res;
	size_t offset;
	size_t outSizeProcessed;
	CFileItem *f = db.Database.Files + fd.index;
	res = SzExtract(&archiveStream.InStream, &db, fd.index, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
	if (res != SZ_OK)
		return NULL;
	memcpy(of->data,outBuffer+offset,outSizeProcessed);
	allocImp.Free(outBuffer);
	return of;
}

bool CArchive7ZipDll::IsOpen()
{
	return (archiveStream.File);
}

int CArchive7ZipDll::FindFiles(int cur, string* name, int* size)
{
	if (cur == 0) {
		curSearchHandle++;
		cur = curSearchHandle;
		searchHandles[cur] = fileData.begin();
	}

	if (searchHandles[cur] == fileData.end()) {
		searchHandles.erase(cur);
		return 0;
	}

	*name = searchHandles[cur]->second.origName;
	*size = searchHandles[cur]->second.size;

	searchHandles[cur]++;
	return cur;
}
