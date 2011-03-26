/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ArchiveBase.h"

#include "CRC.h"
#include "Util.h"

CArchiveBase::CArchiveBase(const std::string& archiveName) : archiveFile(archiveName)
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

unsigned CArchiveBase::FindFile(const std::string& filePath) const
{
	const std::string normalizedFilePath = StringToLower(filePath);
	const std::map<std::string, unsigned>::const_iterator it = lcNameIndex.find(normalizedFilePath);
	if (it != lcNameIndex.end()) {
		return it->second;
	} else {
		return NumFiles();
	}
}

bool CArchiveBase::HasLowReadingCost(unsigned fid) const
{
	return true;
}

unsigned CArchiveBase::GetCrc32(unsigned fid)
{
	CRC crc;
	std::vector<boost::uint8_t> buffer;
	if (GetFile(fid, buffer))
		crc.Update(&buffer[0], buffer.size());

	return crc.GetDigest();
}

bool CArchiveBase::GetFile(const std::string& name, std::vector<boost::uint8_t>& buffer)
{
	unsigned fid = FindFile(name);
	if (fid < NumFiles())
	{
		GetFile(fid, buffer);
		return true;
	}
	else
		return false;
}
