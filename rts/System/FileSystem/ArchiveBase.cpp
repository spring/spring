#include "ArchiveBase.h"
#include "CRC.h"

CArchiveBase::CArchiveBase(const std::string& archiveName) : archiveFile(archiveName)
{
}

unsigned int CArchiveBase::GetCrc32(const std::string& fileName)
{
	CRC crc;
	unsigned char buffer [65536];
	int handle;
	int maxRead;
	int total = 0;

	handle = this->OpenFile(fileName);
	if (handle == 0) return crc.GetDigest();

	do {
		maxRead = this->ReadFile(handle, &buffer, sizeof(buffer));
		crc.Update(buffer, maxRead);
		total += maxRead;
	} while (maxRead == sizeof(buffer));

	this->CloseFile(handle);
	return crc.GetDigest();
};

std::string CArchiveBase::GetArchivName()
{
	return archiveFile;
}
