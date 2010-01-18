#include "StdAfx.h"
#include "ArchiveBuffered.h"
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include "mmgr.h"


FileBuffer::FileBuffer() : pos(0), data(NULL)
{
}

FileBuffer::~FileBuffer()
{
}

ABOpenFile_t::~ABOpenFile_t()
{
	free(data);
}

inline FileBuffer* CArchiveBuffered::GetOpenFile(int handle)
{
	std::map<int, FileBuffer*>::iterator it = fileHandles.find(handle);

	if (it == fileHandles.end())
		throw std::runtime_error("Unregistered handle. Pass a handle returned by CArchiveBuffered::OpenFile.");

	return it->second;
}

CArchiveBuffered::CArchiveBuffered(const std::string& name):
	CArchiveBase(name),
	curFileHandle(1)
{
}

CArchiveBuffered::~CArchiveBuffered(void)
{
	for (std::map<int, FileBuffer*>::iterator i = fileHandles.begin(); i != fileHandles.end(); ++i)
	{
		delete i->second;
	}
}

int CArchiveBuffered::OpenFile(const std::string& fileName)
{
	FileBuffer* fh = GetEntireFile(fileName);
	if (!fh)
		return 0;

	curFileHandle++;
	fileHandles[curFileHandle] = fh;

	return curFileHandle;
}

int CArchiveBuffered::ReadFile(int handle, void* buffer, int numBytes)
{
	FileBuffer* of = GetOpenFile(handle);

	int maxRead = std::min(numBytes, of->size - of->pos);
	memcpy(buffer, of->data + of->pos, maxRead);
	of->pos += maxRead;

	return maxRead;
}

void CArchiveBuffered::CloseFile(int handle)
{
	FileBuffer* of = GetOpenFile(handle);

	delete of;
	fileHandles.erase(handle);
}

void CArchiveBuffered::Seek(int handle, int pos)
{
	FileBuffer* of = GetOpenFile(handle);
	of->pos = std::min(pos, of->size);
}

int CArchiveBuffered::Peek(int handle)
{
	FileBuffer* of = GetOpenFile(handle);
	if (of->pos >= of->size)
		return -1;
	return of->data[of->pos];
}

bool CArchiveBuffered::Eof(int handle)
{
	FileBuffer* of = GetOpenFile(handle);
	return (of->pos >= of->size);
}

int CArchiveBuffered::FileSize(int handle)
{
	FileBuffer* of = GetOpenFile(handle);
	return of->size;
}
