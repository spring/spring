#ifndef PACK_PACKET_H
#define PACK_PACKET_H

#include <string>

#include "RawPacket.h"

namespace netcode
{

class PackPacket : public RawPacket
{
public:
	PackPacket(const unsigned length);
	
	template <typename T>
	PackPacket& operator<<(const T& t) {
		*(T*)(data+pos) = t;
		pos += sizeof(T);
		return *this;
	};
	PackPacket& operator<<(const std::string& text);
	
private:
	unsigned pos;
};

};

#endif
