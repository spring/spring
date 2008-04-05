#include "PackPacket.h"

namespace netcode
{

PackPacket::PackPacket(const unsigned length) : RawPacket(length), pos(0)
{
}

PackPacket& PackPacket::operator<<(const std::string& text)
{
	strcpy((char*)(data+pos), text.c_str());
	pos += text.size()+1;
	return *this;
}

}

