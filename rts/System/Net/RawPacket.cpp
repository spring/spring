#include "RawPacket.h"

#include <string.h>

namespace netcode
{

RawPacket::RawPacket(const unsigned char* const tdata, const unsigned newLength) : length(newLength)
{
	if (length > 0)
	{
		data = new unsigned char[length];
		memcpy(data, tdata, length);
	}
}

RawPacket::~RawPacket()
{
	if (length > 0)
		delete[] data;
}

} // namespace netcode
