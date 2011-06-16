/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ArchiveBase.h"

#include "CRC.h"
#include "Util.h"

CArchiveBase::CArchiveBase(const std::string& archiveName)
	: archiveFile(archiveName)
{
}

CArchiveBase::~CArchiveBase()
{
}

std::string CArchiveBase::GetArchiveName() const
{
	return archiveFile;
}

bool CArchiveBase::FileExists(const std::string& normalizedFilePath) const
{
	return (lcNameIndex.find(normalizedFilePath) != lcNameIndex.end());
}

unsigned int CArchiveBase::FindFile(const std::string& filePath) const
{
	const std::string normalizedFilePath = StringToLower(filePath);
	const std::map<std::string, unsigned int>::const_iterator it = lcNameIndex.find(normalizedFilePath);
	if (it != lcNameIndex.end()) {
		return it->second;
	} else {
		return NumFiles();
	}
}

bool CArchiveBase::HasLowReadingCost(unsigned int fid) const
{
	return true;
}

unsigned int CArchiveBase::GetCrc32(unsigned int fid)
{
	CRC crc;
	std::vector<boost::uint8_t> buffer;
	if (GetFile(fid, buffer)) {
		crc.Update(&buffer[0], buffer.size());
	}

	return crc.GetDigest();
}

bool CArchiveBase::GetFile(const std::string& name, std::vector<boost::uint8_t>& buffer)
{
	const unsigned int fid = FindFile(name);
	const bool found = (fid < NumFiles());

	if (found) {
		GetFile(fid, buffer);
		return true;
	}

	return found;
}
