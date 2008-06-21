#include "StdAfx.h"
#include "Archive7Zip.h"
#include <algorithm>
#include <string.h>
#include "mmgr.h"

// Most of this code is taken from 7zMain.c

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
	CFileInStream *s = (CFileInStream *)object;
	size_t processedSizeLoc = fread(buffer, 1, size, s->File);
	if (processedSize != 0)
		*processedSize = processedSizeLoc;
	return SZ_OK;
}

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
	CFileInStream *s = (CFileInStream *)object;
	int res = fseek(s->File, (long)pos, SEEK_SET);
	if (res == 0)
		return SZ_OK;
	return SZE_FAIL;
}

CArchive7Zip::CArchive7Zip(const std::string& name):
	CArchiveBuffered(name),
	curSearchHandle(1),
	isOpen(false)
{
	SZ_RESULT res;

	archiveStream.File = fopen(name.c_str(), "rb");
	if (archiveStream.File == 0)
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

	isOpen = true;

	// Get contents of archive and store name->int mapping
	for (unsigned i = 0; i < db.Database.NumFiles; ++i) {
		CFileItem* fi = db.Database.Files + i;
		if ((fi->Size >= 0) && !fi->IsDirectory) {
			std::string name = fi->Name;
			//SetSlashesForwardToBack(name);

			FileData fd;
			fd.origName = name;
			fd.fp = i;
			fd.size = fi->Size;
			fd.crc = (fi->Size > 0) ? fi->FileCRC : 0;

			StringToLowerInPlace(name);
			fileData[name] = fd;
		}
	}
}

CArchive7Zip::~CArchive7Zip(void)
{
	if (archiveStream.File) {
		SzArDbExFree(&db, allocImp.Free);
		fclose(archiveStream.File);
	}
}

unsigned int CArchive7Zip::GetCrc32 (const std::string& fileName)
{
	std::string lower = StringToLower(fileName);
	FileData fd = fileData[lower];
	return fd.crc;
}

ABOpenFile_t* CArchive7Zip::GetEntireFile(const std::string& fName)
{
	if (!isOpen)
		return NULL;

	// Figure out the file index
	std::string fileName = StringToLower(fName);
	
	if (fileData.find(fileName) == fileData.end())
		return NULL;

	FileData fd = fileData[fileName];

	// Get 7zip to decompress it
	size_t offset;
	size_t outSizeProcessed;
	
	SZ_RESULT res;

	// We don't really support solid archives anyway, so these can be reset for each file
	UInt32 blockIndex = 0xFFFFFFFF; // it can have any value before first call (if outBuffer = 0) 
	Byte *outBuffer = 0; // it must be 0 before first call for each new archive. 
	size_t outBufferSize = 0;  // it can have any value before first call (if outBuffer = 0) 

	res = SzExtract(&archiveStream.InStream, &db, fd.fp, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);

	ABOpenFile_t* of = NULL;
	if (res == SZ_OK) {
		of = SAFE_NEW ABOpenFile_t;
		of->pos = 0;
		of->size = outSizeProcessed;
		of->data = (char*)malloc(of->size); 

		memcpy(of->data, outBuffer + offset, outSizeProcessed);
	}

	allocImp.Free(outBuffer);

	if (res != SZ_OK)
		return NULL;

	return of;
}

void CArchive7Zip::SetSlashesForwardToBack(std::string& name)
{
	for (unsigned int i = 0; i < name.length(); ++i) {
		if (name[i] == '/')
			name[i] = '\\';
	}
}

int CArchive7Zip::FindFiles(int cur, std::string* name, int* size)
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

bool CArchive7Zip::IsOpen()
{
	return isOpen;
}
