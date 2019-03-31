/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ZipArchive.h"

#include <algorithm>
#include <stdexcept>
#include <assert.h>

#include "System/StringUtil.h"
#include "System/Log/ILog.h"


IArchive* CZipArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CZipArchive(filePath);
}


CZipArchive::CZipArchive(const std::string& archiveName): CBufferedArchive(archiveName)
{
	zip = unzOpen(archiveName.c_str());

	if (zip == nullptr) {
		LOG_L(L_ERROR, "Error opening \"%s\"", archiveName.c_str());
		return;
	}

	// We need to map file positions to speed up opening later
	for (int ret = unzGoToFirstFile(zip); ret == UNZ_OK; ret = unzGoToNextFile(zip)) {
		unz_file_info info;
		char fName[512];

		unzGetCurrentFileInfo(zip, &info, fName, sizeof(fName), nullptr, 0, nullptr, 0);

		const std::string fLowerName = StringToLower(fName);
		if (fLowerName.empty())
			continue;

		if ((fLowerName.back() == '/') || (fLowerName.back() == '\\'))
			continue; // exclude directory names

		FileData fd;
		unzGetFilePos(zip, &fd.fp);
		fd.size = info.uncompressed_size;
		fd.origName = fName;
		fd.crc = info.crc;
		fileData.push_back(fd);
		lcNameIndex[fLowerName] = fileData.size() - 1;
	}
}

CZipArchive::~CZipArchive()
{
	if (zip != nullptr) {
		unzClose(zip);
		zip = nullptr;
	}
}

bool CZipArchive::IsOpen()
{
	return (zip != nullptr);
}

unsigned int CZipArchive::NumFiles() const
{
	return fileData.size();
}

void CZipArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));

	name = fileData[fid].origName;
	size = fileData[fid].size;
}

#if 0
unsigned int CZipArchive::GetCrc32(unsigned int fid)
{
	assert(IsFileId(fid));

	return fileData[fid].crc;
}
#endif

// To simplify things, files are always read completely into memory from
// the zip-file, since zlib does not provide any way of reading more
// than one file at a time
int CZipArchive::GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	// Prevent opening files on missing/invalid archives
	if (zip == nullptr)
		return -4;

	assert(IsFileId(fid));

	unzGoToFilePos(zip, &fileData[fid].fp);

	unz_file_info fi;
	unzGetCurrentFileInfo(zip, &fi, nullptr, 0, nullptr, 0, nullptr, 0);

	if (unzOpenCurrentFile(zip) != UNZ_OK)
		return -3;

	buffer.clear();
	buffer.resize(fi.uncompressed_size);

	int ret = 1;

	if (!buffer.empty() && unzReadCurrentFile(zip, &buffer[0], fi.uncompressed_size) != fi.uncompressed_size)
		ret -= 2;
	if (unzCloseCurrentFile(zip) == UNZ_CRCERROR)
		ret -= 1;

	if (ret != 1)
		buffer.clear();

	return ret;
}
