#ifndef UNPACK_PACKET_H
#define UNPACK_PACKET_H

#include <string>

#include "RawPacket.h"

namespace netcode
{

class UnpackPacket : public RawPacket
{
public:
	UnpackPacket(const unsigned char* const data, const unsigned length);
	void Reset() {pos = 0;};
	
	template <typename T>
	void operator>>(T& t) {t = *(T*)(data+pos); pos += sizeof(T);};
	void operator>>(std::string& text) {text = std::string((char*)(data + pos)); pos += text.size()+1;};
	
private:
	unsigned pos;
};

};

#endif
