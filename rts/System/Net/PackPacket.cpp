//#include <algorithm>
#include "PackPacket.h"
#include "LogOutput.h"

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
	unsigned size = std::min(text.size()+1, length-pos);
	if (size + pos > length) {
		logOutput.Print("netcode warning: string data truncated in packet\n");
		#ifdef DEBUG
		abort();
		#endif
	}
	memcpy((char*)(data+pos), text.data(), size);
	pos += size;
	return *this;
}

}

