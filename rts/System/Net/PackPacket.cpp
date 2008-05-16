#include "PackPacket.h"

namespace netcode
{

PackPacket::PackPacket(const unsigned length) : RawPacket(length), pos(0)
{
}

PackPacket::PackPacket(const unsigned length, unsigned char msgID) : RawPacket(length), pos(0)
{
	*this << msgID;
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

