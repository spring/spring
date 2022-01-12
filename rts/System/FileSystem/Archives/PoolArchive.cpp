/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "PoolArchive.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cassert>
#include <cstring>
#include <iostream>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


CPoolArchiveFactory::CPoolArchiveFactory(): IArchiveFactory("sdp")
{
}

IArchive* CPoolArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CPoolArchive(filePath);
}



static uint32_t parse_uint32(uint8_t c[4])
{
	uint32_t i = 0;
	i = c[0] << 24 | i;
	i = c[1] << 16 | i;
	i = c[2] <<  8 | i;
	i = c[3] <<  0 | i;
	return i;
}

static bool gz_really_read(gzFile file, voidp buf, unsigned int len)
{
	return (gzread(file, reinterpret_cast<char*>(buf), len) == len);
}



CPoolArchive::CPoolArchive(const std::string& name): CBufferedArchive(name)
{
	memset(&dummyFileHash, 0, sizeof(dummyFileHash));

	char c_name[255];
	uint8_t c_md5sum[16];
	uint8_t c_crc32[4];
	uint8_t c_size[4];
	uint8_t length;

	gzFile in = gzopen(name.c_str(), "rb");

	if (in == nullptr)
		throw content_error("[" + std::string(__func__) + "] could not open " + name);

	files.reserve(1024);
	stats.reserve(1024);

	// get pool dir from .sdp absolute path
	assert(FileSystem::IsAbsolutePath(name));
	poolRootDir = FileSystem::GetParent(FileSystem::GetDirectory(name));
	assert(!poolRootDir.empty());

	while (gz_really_read(in, &length, 1)) {
		if (!gz_really_read(in, &c_name, length)) break;
		if (!gz_really_read(in, &c_md5sum, 16)) break;
		if (!gz_really_read(in, &c_crc32, 4)) break;
		if (!gz_really_read(in, &c_size, 4)) break;

		files.emplace_back();
		stats.emplace_back();
		FileData& f = files.back();
		FileStat& s = stats.back();

		f.name = std::move(std::string(c_name, length));

		std::memcpy(&f.md5sum, &c_md5sum, sizeof(f.md5sum));
		std::memset(&f.shasum, 0, sizeof(f.shasum));

		f.crc32 = parse_uint32(c_crc32);
		f.size = parse_uint32(c_size);

		s.fileIndx = files.size() - 1;
		s.readTime = 0;

		lcNameIndex[f.name] = s.fileIndx;
	}

	isOpen = gzeof(in);
	gzclose(in);
}

CPoolArchive::~CPoolArchive()
{
	const std::string& archiveFile = GetArchiveFile();
	const std::pair<uint64_t, uint64_t>& sums = GetSums();

	const unsigned long numZipFiles = files.size();
	const unsigned long sumInflSize = sums.first / 1024;
	const unsigned long sumReadTime = sums.second / (1000 * 1000);

	LOG_L(L_INFO, "[%s] archiveFile=\"%s\" numZipFiles=%lu sumInflSize=%lukb sumReadTime=%lums", __func__, archiveFile.c_str(), numZipFiles, sumInflSize, sumReadTime);

	std::partial_sort(stats.begin(), stats.begin() + std::min(stats.size(), size_t(10)), stats.end());

	// show top-10 worst access times
	for (size_t n = 0; n < std::min(stats.size(), size_t(10)); n++) {
		const FileStat& s = stats[n];
		const FileData& f = files[s.fileIndx];

		const unsigned long indx = s.fileIndx;
		const unsigned long time = s.readTime / (1000 * 1000);

		LOG_L(L_INFO, "\tfile=\"%s\" indx=%lu inflSize=%ukb readTime=%lums", f.name.c_str(), indx, f.size / 1024, time);
	}
}

int CPoolArchive::GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	FileData* f = &files[fid];
	FileStat* s = &stats[fid];

	constexpr const char table[] = "0123456789abcdef";
	char c_hex[32];

	for (int i = 0; i < 16; ++i) {
		c_hex[2 * i    ] = table[(f->md5sum[i] >> 4) & 0xf];
		c_hex[2 * i + 1] = table[ f->md5sum[i]       & 0xf];
	}

	const std::string prefix(c_hex,      2);
	const std::string pstfix(c_hex + 2, 30);

	      std::string rpath = poolRootDir + "/pool/" + prefix + "/" + pstfix + ".gz";
	const std::string  path = FileSystem::FixSlashes(rpath);

	const spring_time startTime = spring_now();


	gzFile in = gzopen(path.c_str(), "rb");

	if (in == nullptr)
		return -1;

	buffer.clear();
	buffer.resize(f->size);

	const int bytesRead = (buffer.empty()) ? 0 : gzread(in, reinterpret_cast<char*>(buffer.data()), buffer.size());
	gzclose(in);

	s->readTime = (spring_now() - startTime).toNanoSecsi();


	if (bytesRead != buffer.size()) {
		LOG_L(L_ERROR, "[PoolArchive::%s] could not read file \"%s\" (bytesRead=%d fileSize=%u)", __func__, path.c_str(), bytesRead, f->size);
		buffer.clear();
		return 0;
	}

	sha512::calc_digest(buffer.data(), buffer.size(), f->shasum.data());
	return 1;
}
