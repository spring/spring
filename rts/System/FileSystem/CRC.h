#ifndef CRC_H
#define CRC_H

#include <string>

/** @brief Object representing an updateable CRC-32 checksum. */
class CRC
{
public:
	CRC();

	unsigned int GetDigest() const;

	CRC& Update(const void* data, unsigned int size);
	CRC& Update(unsigned int data);

private:
	unsigned int crc;
};

#endif // !CRC_H
