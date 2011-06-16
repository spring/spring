/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CRC.h"

extern "C" {
#include "lib/7z/7zCrc.h"
};


static bool crcTableInitialized;


CRC::CRC()
{
	crc = CRC_INIT_VAL;
	if (!crcTableInitialized) {
		crcTableInitialized = true;
		CrcGenerateTable();
	}
}


unsigned int CRC::GetDigest() const
{
	// make a temporary copy to get away with the const
	unsigned int temp = crc;
	return CRC_GET_DIGEST(temp);
}


CRC& CRC::Update(const void* data, unsigned int size)
{
	crc = CrcUpdate(crc, data, size);
	return *this;
}


CRC& CRC::Update(unsigned int data)
{
	crc = CrcUpdate(crc, &data, sizeof(unsigned));
	return *this;
}
