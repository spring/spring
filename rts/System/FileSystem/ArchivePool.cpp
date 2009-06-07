#include "StdAfx.h"
#include "ArchivePool.h"
#include "FileSystem.h"
#include <algorithm>
#include <stdexcept>
#include "Util.h"
#include "mmgr.h"

#include <sstream>
#include <string>
#include <iostream>


static unsigned int parse_int32(unsigned char c[4])
{
	unsigned int i = 0;
	i = c[0] << 24 | i;
	i = c[1] << 16 | i;
	i = c[2] << 8  | i;
	i = c[3] << 0  | i;
	return i;
}

static bool gz_really_read(gzFile file, voidp buf, unsigned len)
{
	unsigned offset = 0;

	while (true) {
		int i = gzread(file, ((char *)buf)+offset, len-offset);
		if (i == -1) return false;
		offset += i;
		if (offset == len) return true;
	}
}

CArchivePool::CArchivePool(const std::string& name):
	CArchiveBuffered(name),
	isOpen(false)

{
	char c_name[255];
	unsigned char c_md5[16];
	unsigned char c_crc32[4];
	unsigned char c_size[4];

	gzFile in = gzopen (name.c_str(), "rb");
	if (in == NULL) return;

	while (true) {
		if (gzeof(in)) {
			isOpen = true;
			break;
		}

		int length = gzgetc(in);
		if (length == -1) break;
		
		if (!gz_really_read(in, &c_name, length)) break;
		if (!gz_really_read(in, &c_md5, 16)) break;
		if (!gz_really_read(in, &c_crc32, 4)) break;
		if (!gz_really_read(in, &c_size, 4)) break;

		FileData *f = new FileData;
		f->name = std::string(c_name, length);
		memcpy(&f->md5, &c_md5, 16);
		f->crc32 = parse_int32(c_crc32);
		f->size = parse_int32(c_size);

		files.push_back(f);
		fileMap[f->name] = f;
	}
	gzclose(in);
}

CArchivePool::~CArchivePool(void)
{
	std::vector<FileData *>::iterator i = files.begin();
	for(; i < files.end(); i++) delete *i;
}

unsigned int CArchivePool::GetCrc32 (const std::string& fileName)
{
	std::string lower = StringToLower(fileName);
	FileData *f = fileMap[lower];
	return f->crc32;
}

bool CArchivePool::IsOpen()
{
	return isOpen;
}

ABOpenFile_t* CArchivePool::GetEntireFile(const std::string& fName)
{

	if (!isOpen) return NULL;

	std::string fileName = StringToLower(fName);
	if (fileMap.find(fileName) == fileMap.end()) {
		return NULL;
	}

	FileData *f = fileMap[fileName];

	char table[] = "0123456789abcdef";
	char c_hex[32];
	for (int i = 0; i < 16; ++i) {
		c_hex[2 * i] = table[(f->md5[i] >> 4) & 0xf];
		c_hex[2 * i + 1] = table[f->md5[i] & 0xf];
	}
	std::string prefix(c_hex, 2);
	std::string postfix(c_hex+2, 30);

	std::ostringstream accu;
	accu << "pool/" << prefix << "/" << postfix << ".gz";
	std::string rpath = accu.str();

	filesystem.FixSlashes(rpath);
	std::string path = filesystem.LocateFile(rpath);
	gzFile in = gzopen (path.c_str(), "rb");
	if (in == NULL) return NULL;

	ABOpenFile_t* of = new ABOpenFile_t;
	of->size = f->size;
	of->pos = 0;
	of->data = (char*) malloc(of->size);

	int j, i = 0;
	while (true) {
		j = gzread(in, of->data + i, of->size - i);
		if (j == 0) break;
		if (j == -1) {
			gzclose(in);
			delete of;
			return NULL;
		}
		i += j;
	}
	
	gzclose(in);
	return of;
}

int CArchivePool::FindFiles(int cur, std::string* name, int* size)
{
	if (cur < 0 || static_cast<size_t>(cur) >= files.size()) {
		return 0;
	} else {
		*name = files[cur]->name;
		*size = files[cur]->size;
		return cur + 1;
	}
}
