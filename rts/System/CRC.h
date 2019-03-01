/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CRC_H
#define CRC_H

#include <string>

/** @brief An updateable CRC-32 checksum. */
class CRC
{
private:
	template<class T>
	CRC& Up(T data) {
		Update((void*) &data, sizeof(data));
		return *this;
	}

public:
	/** @brief Construct a new CRC object. */
	CRC();

	/** @brief Get the final CRC digest. */
	uint32_t GetDigest() const;

	static uint32_t InitTable();
	static uint32_t CalcDigest(const void* data, size_t size);

	/** @brief Update CRC over the data. */
	CRC& Update(const void* data, size_t size);
	/** @brief Update CRC over the 4 bytes of data. */
	CRC& Update(uint32_t data);

	CRC& operator << ( int32_t data) { return Up(data); }
	CRC& operator << (uint32_t data) { return Up(data); }
	CRC& operator << (   float data) { return Up(data); }

private:
	uint32_t crc;
};

#endif // !CRC_H

