#include "PackPacket.h"

#include <algorithm>
#include <cstdlib>

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
	size_t size = std::min(text.size()+1, static_cast<size_t>(length-pos));
	if (std::string::npos != text.find_first_of('\0'))
	{
		logOutput.Print("A text must not contain a '\\0' inside, trunkating");
		size = text.find_first_of('\0')+1;
	}
	if (size + pos > length) {
		logOutput.Print("netcode warning: string data truncated in packet\n");
		#ifdef DEBUG
		std::abort();
		#endif
	}
	memcpy((char*)(data+pos), text.c_str(), size);
	pos += size;
	return *this;
}

}

