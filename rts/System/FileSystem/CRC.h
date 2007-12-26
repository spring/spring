#ifndef CRC_H
#define CRC_H

#include <string>

/** @brief Object representing an updateable CRC-32 checksum. */
class CRC
{
public:
	CRC();

	void UpdateData(const unsigned char* buf, unsigned bytes);
	void UpdateData(const std::string& buf);
	bool UpdateFile(const std::string& filename);

	unsigned int GetCRC() const { return crc ^ 0xFFFFFFFF; }

private:
	static unsigned int crcTable[256];
	static void GenerateCRCTable();

	unsigned int crc;
};

#endif // !CRC_H
