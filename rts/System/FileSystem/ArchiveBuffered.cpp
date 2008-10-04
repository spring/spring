#include "StdAfx.h"
#include "mmgr.h"

#include "ArchiveBuffered.h"

#include <string.h>
#include <cstdlib>

CArchiveBuffered::CArchiveBuffered(const std::string& name):
	CArchiveBase(name),
	curFileHandle(1)
{
}

CArchiveBuffered::~CArchiveBuffered(void)
{
	for (std::map<int, ABOpenFile_t*>::iterator i = fileHandles.begin(); i != fileHandles.end(); ++i) {
		free((i->second)->data);
		delete i->second;
	}
}

int CArchiveBuffered::OpenFile(const std::string& fileName)
{
	ABOpenFile_t* fh = GetEntireFile(fileName);
	if (!fh)
		return 0;

	curFileHandle++;
	fileHandles[curFileHandle] = fh;

	return curFileHandle;
}

int CArchiveBuffered::ReadFile(int handle, void* buffer, int numBytes)
{
	ABOpenFile_t* of = fileHandles[handle];

	int maxRead = std::min(numBytes, of->size - of->pos);
	memcpy(buffer, of->data + of->pos, maxRead);
	of->pos += maxRead;

	return maxRead;
}

void CArchiveBuffered::CloseFile(int handle)
{
	free(fileHandles[handle]->data);
	delete fileHandles[handle];
	fileHandles.erase(handle);
}

void CArchiveBuffered::Seek(int handle, int pos)
{
	ABOpenFile_t* of = fileHandles[handle];
	of->pos = std::min(pos, of->size);
}

int CArchiveBuffered::Peek(int handle)
{
	ABOpenFile_t* of = fileHandles[handle];
	if (of->pos >= of->size)
		return -1;
	return of->data[of->pos];
}

bool CArchiveBuffered::Eof(int handle)
{
	ABOpenFile_t* of = fileHandles[handle];
	return (of->pos >= of->size);
}

int CArchiveBuffered::FileSize(int handle)
{
	return fileHandles[handle]->size;
}
