#include "ArchiveBase.h"

#include "CRC.h"
#include "Util.h"

CArchiveBase::CArchiveBase(const std::string& archiveName) : archiveFile(archiveName)
{
}

CArchiveBase::~CArchiveBase()
{
}

std::string CArchiveBase::GetArchiveName()
{
	return archiveFile;
}

unsigned CArchiveBase::FindFile(const std::string& name) const
{
	const std::string fileName = StringToLower(name);
	std::map<std::string, unsigned>::const_iterator it = lcNameIndex.find(fileName);
	if (it != lcNameIndex.end())
		return it->second;
	else
		return NumFiles();
}

unsigned CArchiveBase::GetCrc32(unsigned fid)
{
	CRC crc;
	std::vector<uint8_t> buffer;
	if (GetFile(fid, buffer))
		crc.Update(&buffer[0], buffer.size());

	return crc.GetDigest();
}

bool CArchiveBase::GetFile(const std::string& name, std::vector<uint8_t>& buffer)
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
