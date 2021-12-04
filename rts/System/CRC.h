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
	unsigned int GetDigest() const;

	static unsigned int GetCRC(const void* data, unsigned int size);

	/** @brief Update CRC over the data. */
	CRC& Update(const void* data, unsigned int size);
	/** @brief Update CRC over the 4 bytes of data. */
	CRC& Update(unsigned int data);

	CRC& operator<<(int data)      { return Up(data); }
	CRC& operator<<(unsigned data) { return Up(data); }
	CRC& operator<<(float data)    { return Up(data); }

private:
	unsigned int crc;
};

#endif // !CRC_H
