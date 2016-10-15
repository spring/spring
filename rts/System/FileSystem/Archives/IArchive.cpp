/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IArchive.h"

#include "System/CRC.h"
#include "System/Util.h"

IArchive::IArchive(const std::string& archiveName)
	: archiveFile(archiveName)
{
}

IArchive::~IArchive()
{
}

const std::string& IArchive::GetArchiveName() const
{
	return archiveFile;
}

bool IArchive::FileExists(const std::string& normalizedFilePath) const
{
	return (lcNameIndex.find(normalizedFilePath) != lcNameIndex.end());
}

unsigned int IArchive::FindFile(const std::string& filePath) const
{
	const std::string normalizedFilePath = StringToLower(filePath);
	const std::map<std::string, unsigned int>::const_iterator it = lcNameIndex.find(normalizedFilePath);
	if (it != lcNameIndex.end()) {
		return it->second;
	} else {
		return NumFiles();
	}
}

bool IArchive::HasLowReadingCost(unsigned int fid) const
{
	return true;
}

unsigned int IArchive::GetCrc32(unsigned int fid)
{
	CRC crc;
	std::vector<std::uint8_t> buffer;
	if (GetFile(fid, buffer) && !buffer.empty()) {
		crc.Update(&buffer[0], buffer.size());
	}

	return crc.GetDigest();
}

bool IArchive::GetFile(const std::string& name, std::vector<std::uint8_t>& buffer)
{
	const unsigned int fid = FindFile(name);
	const bool found = (fid < NumFiles());

	if (found) {
		GetFile(fid, buffer);
		return true;
	}

	return found;
}
