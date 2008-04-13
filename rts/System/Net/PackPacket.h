#ifndef PACK_PACKET_H
#define PACK_PACKET_H

#include <string>
#include <vector>
#include <assert.h>
#include <cstring>

#include "RawPacket.h"

namespace netcode
{

class PackPacket : public RawPacket
{
public:
	PackPacket(const unsigned length);
	
	template <typename T>
	PackPacket& operator<<(const T& t) {
		unsigned size = sizeof(T);
		assert(size + pos <= length);
		*(T*)(data+pos) = t;
		pos += size;
		return *this;
	};
	PackPacket& operator<<(const std::string& text);
	template <typename element>
	PackPacket& operator<<(const std::vector<element>& vec) {
		unsigned size = vec.size()* sizeof(element);
		if (size > 0) {
			assert(size + pos <= length);
                        std::memcpy((data+pos), (void*)(&vec[0]), size);
			pos += size;
		}
		return *this;
	};
	
private:
	unsigned pos;
};

};

#endif
