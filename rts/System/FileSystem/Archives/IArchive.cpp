/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IArchive.h"

#include "System/CRC.h"
#include "System/Util.h"

IArchive::IArchive(const std::string& archiveName)
	: archiveFile(archiveName)
{
}

unsigned int IArchive::FindFile(const std::string& filePath) const
{
	const std::string& normalizedFilePath = StringToLower(filePath);
	const auto it = lcNameIndex.find(normalizedFilePath);

	if (it != lcNameIndex.end())
		return it->second;

	return NumFiles();
}

unsigned int IArchive::GetCrc32(unsigned int fid)
{
	CRC crc;
	std::vector<std::uint8_t> buffer;
	if (GetFile(fid, buffer) && !buffer.empty())
		crc.Update(&buffer[0], buffer.size());

	return crc.GetDigest();
}

bool IArchive::GetFile(const std::string& name, std::vector<std::uint8_t>& buffer)
{
	const unsigned int fid = FindFile(name);

	if (!IsFileId(fid))
		return false;

	GetFile(fid, buffer);
	return true;
}

