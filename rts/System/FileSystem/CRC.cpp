#include "CRC.h"

extern "C" {
#include "lib/7zip/7zCrc.h"
};


static bool crcTableInitialized;


/** @brief Construct a new CRC object. */
CRC::CRC()
{
	if (!crcTableInitialized) {
		crcTableInitialized = true;
		InitCrcTable();
	}
	CrcInit(&crc);
}


/** @brief Get the final CRC digest. */
unsigned int CRC::GetDigest() const
{
	// make a temporary copy to get away with the const
	unsigned int temp = crc;
	return CrcGetDigest(&temp);
}


/** @brief Update CRC over the data. */
CRC& CRC::Update(const void* data, unsigned int size)
{
	CrcUpdate(&crc, data, size);
	return *this;
}


/** @brief Update CRC over the 4 bytes of data. */
CRC& CRC::Update(unsigned int data)
{
	CrcUpdateUInt32(&crc, data);
	return *this;
}
