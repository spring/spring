#include "StdAfx.h"
#include "ArchivePool.h"
#include "FileSystem.h"
#include <algorithm>
#include <stdexcept>
#include "Util.h"
#include "mmgr.h"

CArchivePool::CArchivePool(const std::string& name):
	CArchiveBuffered(name),
	isOpen(false)

{
	gzFile in = gzopen (name.c_str(), "r");
	if (in == NULL) return;

	while (true) {
		int n = 0;
		FileData f;
		std::string size_string;

		while (true) {
			char c = gzgetc(in);
			if (c == -1) return;
			if (c == ',') break;
			f.name.push_back(tolower(c));
		}

		while (true) {
			char c = gzgetc(in);
			if (c == -1) return;
			if (c == ',') break;
			f.hex.push_back(c);
		}


		while (true) {
			char c = gzgetc(in);
			if (c == -1) return;
			if (c == ',') break;
			size_string.push_back(c);
		}
		f.size = atoi(size_string.c_str());

		// Skip compressed size
		while (true) {
			char c = gzgetc(in);
			if (c == -1) break;
			if (c == '\n') break;
		}

		files.push_back(f);
		fileMap[f.name] = f;
		if (gzeof(in)) break;
	}
	isOpen = true;
	gzclose(in);
}

CArchivePool::~CArchivePool(void)
{
//	Memory management goes here
}

//unsigned int CArchivePool::GetCrc32 (const std::string& fileName)
//{
//	std::string lower = StringToLower(fileName);
//	FileData fd = fileData[lower];
//	return fd.crc;
//	return 0;
//}

bool CArchivePool::IsOpen()
{
	return isOpen;
}



// To simplify things, files are always read completely into memory from the zipfile, since zlib does not
// provide any way of reading more than one file at a time
ABOpenFile_t* CArchivePool::GetEntireFile(const std::string& fName)
{

	if (!isOpen)
		return NULL;

	std::string fileName = StringToLower(fName);
	if (fileMap.find(fileName) == fileMap.end()) {
		return NULL;
	}

	FileData f = fileMap[fileName];
	
	std::string path = filesystem.LocateFile("pool/" + f.hex + ".gz");
	gzFile in = gzopen (path.c_str(), "r");
	if (in == NULL) return NULL;

	ABOpenFile_t* of = new ABOpenFile_t;
	of->size = f.size;
	of->pos = 0;
	of->data = (char*) malloc(of->size);

	int j, i = 0;
	while (true) {
		j = gzread(in, of->data + i, of->size - i);
		if (j == -1) {
			delete of;
			return NULL;
		}

		if (j == 0) break;
		i += j;
	}
	
	gzclose(in);
	return of;
}

int CArchivePool::FindFiles(int cur, std::string* name, int* size)
{
	if (cur >= files.size()) {
		return 0;
	} else {
		*name = files[cur].name;
		*size = files[cur].size;
		return cur + 1;
	}
}

