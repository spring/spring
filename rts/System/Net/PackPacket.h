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
	void operator<<(const T& t) {*(T*)(data+pos) = t; pos += sizeof(T);};
	void operator<<(const std::string& text) {strcpy((char*)(data+pos), text.c_str()); pos += text.size()+1;};
	
private:
	unsigned pos;
};

};

#endif
