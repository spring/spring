/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CRC.h"

extern "C" {
#include "lib/7z/7zCrc.h"
}


CRC::CRC(): crc(CRC_INIT_VAL)
{
	InitTable();
}


uint32_t CRC::InitTable()
{
	static bool crcTableInitialized = false;

	if (crcTableInitialized)
		return 1;

	CrcGenerateTable();

	crcTableInitialized = true;
	return 0;
}

uint32_t CRC::CalcDigest(const void* data, size_t size)
{
	return (InitTable(), CRC_GET_DIGEST(CrcUpdate(CRC_INIT_VAL, data, size)));
}

uint32_t CRC::GetDigest() const
{
	return CRC_GET_DIGEST(crc);
}


CRC& CRC::Update(const void* data, size_t size)
{
	crc = CrcUpdate(crc, data, size);
	return *this;
}

CRC& CRC::Update(uint32_t data)
{
	crc = CrcUpdate(crc, &data, sizeof(data));
	return *this;
}

