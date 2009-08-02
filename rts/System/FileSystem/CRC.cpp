#include "CRC.h"

extern "C" {
#include "lib/7z/7zCrc.h"
};


static bool crcTableInitialized;


/** @brief Construct a new CRC object. */
CRC::CRC()
{
	if (!crcTableInitialized) {
		crcTableInitialized = true;
		crc = CRC_INIT_VAL;
		CrcGenerateTable();
	}
}


/** @brief Get the final CRC digest. */
unsigned int CRC::GetDigest() const
{
	// make a temporary copy to get away with the const
	unsigned int temp = crc;
	return CRC_GET_DIGEST(temp);
}


/** @brief Update CRC over the data. */
CRC& CRC::Update(const void* data, unsigned int size)
{
	CrcUpdate(crc, data, size);
	return *this;
}


/** @brief Update CRC over the 4 bytes of data. */
CRC& CRC::Update(unsigned int data)
{
	CrcUpdate(crc, &data, sizeof(unsigned));
	return *this;
}
