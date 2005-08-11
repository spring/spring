/*
  HPIUtil - HPI file utilities
*/

#define NO_COMPRESSION 0
#define LZ77_COMPRESSION 1
#define ZLIB_COMPRESSION 2

struct _HPIFILE {
	FILE *f;   // handle to open file
	char* d;    // pointer to decrypted directory
	unsigned int Key;	  // Key
	unsigned int Start;  // Start of directory
};
typedef int (*HPICALLBACK)(char* FileName, char* HPIName, int FileCount, int FileCountTotal, int FileBytes, int FileBytesTotal, int TotalBytes, int TotalBytesTotal);
void* HPIOpen(const char* FileName);
char* HPIOpenFile(struct _HPIFILE *hpi, const char *FileName);
void HPIGet(char* Dest, char* FileHandle, const int offset, const int bytecount);
bool HPIGetFiles(const struct _HPIFILE *hpi, const int Next, char *Name, int* Type, int* Size);
bool HPIClose(struct _HPIFILE *hpi);
bool HPICloseFile(char* FileHandle);
bool HPIDir(const struct _HPIFILE *hpi, const int Next, const char* DirName, char* Name, int* Type, int* Size);
