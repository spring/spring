#include "ArchiveBase.h"

extern "C" {
#include "lib/7zip/7zCrc.h"
};

unsigned int CArchiveBase::GetCrc32(const std::string& fileName)
{
	UInt32 crc;
	unsigned char buffer [65536];
	int handle;

	CrcInit(&crc);
	handle = this->OpenFile(fileName);
	if (handle == 0) return CrcGetDigest(&crc);

	while (!this->Eof(handle)) {
		int maxRead = this->ReadFile(handle, &buffer, sizeof(buffer));
		CrcUpdate(&crc, buffer, maxRead);
	}

	this->CloseFile(handle);
	return CrcGetDigest(&crc);
};
