/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "PoolArchive.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>
#include <assert.h>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include <boost/format.hpp>


CPoolArchiveFactory::CPoolArchiveFactory()
	: IArchiveFactory("sdp")
{
}

IArchive* CPoolArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CPoolArchive(filePath);
}


static unsigned int parse_int32(unsigned char c[4])
{
	unsigned int i = 0;
	i = c[0] << 24 | i;
	i = c[1] << 16 | i;
	i = c[2] << 8  | i;
	i = c[3] << 0  | i;
	return i;
}

static bool gz_really_read(gzFile file, voidp buf, unsigned int len)
{
	return gzread(file, (char*)buf, len) == len;
}

CPoolArchive::CPoolArchive(const std::string& name)
	: CBufferedArchive(name)
	, isOpen(false)
{
	char c_name[255];
	unsigned char c_md5[16];
	unsigned char c_crc32[4];
	unsigned char c_size[4];
	unsigned char length;

	gzFile in = gzopen(name.c_str(), "rb");
	if (in == NULL) {
		throw content_error(boost::str(boost::format("couldn't open %s") % name));
	}

	while (true) {

		if (!gz_really_read(in, &length, 1)) {
			if (gzeof(in)) {
				isOpen = true;
			}
			break;
		}
		if (!gz_really_read(in, &c_name, length)) break;
		if (!gz_really_read(in, &c_md5, 16)) break;
		if (!gz_really_read(in, &c_crc32, 4)) break;
		if (!gz_really_read(in, &c_size, 4)) break;

		FileData* f = new FileData;
		f->name = std::string(c_name, length);
		std::memcpy(&f->md5, &c_md5, 16);
		f->crc32 = parse_int32(c_crc32);
		f->size = parse_int32(c_size);

		files.push_back(f);
		lcNameIndex[f->name] = files.size() - 1;
	}
	gzclose(in);
}

CPoolArchive::~CPoolArchive()
{
	std::vector<FileData*>::iterator fi;
	for (fi = files.begin(); fi < files.end(); ++fi) {
		delete *fi;
	}
}

bool CPoolArchive::IsOpen()
{
	return isOpen;
}

unsigned int CPoolArchive::NumFiles() const
{
	return files.size();
}

void CPoolArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));
	name = files[fid]->name;
	size = files[fid]->size;
}

unsigned int CPoolArchive::GetCrc32(unsigned int fid)
{
	assert(IsFileId(fid));
	return files[fid]->crc32;
}


bool CPoolArchive::GetFileImpl(unsigned int fid, std::vector<boost::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	FileData* f = files[fid];

	char table[] = "0123456789abcdef";
	char c_hex[32];
	for (int i = 0; i < 16; ++i) {
		c_hex[2 * i]     = table[(f->md5[i] >> 4) & 0xf];
		c_hex[2 * i + 1] = table[ f->md5[i]       & 0xf];
	}
	std::string prefix(c_hex,      2);
	std::string postfix(c_hex + 2, 30);

	std::ostringstream accu;
	accu << "pool/" << prefix << "/" << postfix << ".gz";
	std::string rpath = accu.str();

	FileSystem::FixSlashes(rpath);
	std::string path = dataDirsAccess.LocateFile(rpath);
	gzFile in = gzopen(path.c_str(), "rb");
	if (in == NULL){
		return false;
	}

	unsigned int len = f->size;
	buffer.resize(len);

	int bytesread = (len == 0) ? 0 : gzread(in, (char *)&buffer[0], len);
	gzclose(in);

	if (bytesread != len) {
		LOG_L(L_ERROR, "couldn't read %s", path.c_str());
		buffer.clear();
		return false;
	}

	return true;
}
