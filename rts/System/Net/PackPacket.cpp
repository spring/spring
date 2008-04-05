#include "PackPacket.h"

namespace netcode
{

PackPacket::PackPacket(const unsigned length) : RawPacket(length), pos(0)
{
}

PackPacket& PackPacket::operator<<(const std::string& text)
{
	unsigned size = text.size()+1;
	assert(size + pos <= length);
	strcpy((char*)(data+pos), text.c_str());
	pos += size;
	return *this;
}

}

