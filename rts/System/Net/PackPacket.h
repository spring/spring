/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
	PackPacket(const unsigned length, unsigned char msgID);
	
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
		const size_t size = vec.size()* sizeof(element);
		assert(size + pos <= length);
		if (size > 0) {
			std::memcpy((data+pos), (void*)(&vec[0]), size);
			pos += size;
		}
		return *this;
	};
	
	unsigned char* GetWritingPos() {return data+pos;};
	
private:
	unsigned pos;
};

};

#endif
