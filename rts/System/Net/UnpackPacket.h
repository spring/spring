/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNPACK_PACKET_H
#define UNPACK_PACKET_H

#include <string>
#include <vector>
#include <cstring>
#include <boost/shared_ptr.hpp>

#include "RawPacket.h"

namespace netcode
{

class UnpackPacketException {
public:
	UnpackPacketException(const char *msg) : err(msg) {}
	std::string err;
};

class UnpackPacket
{
public:
	UnpackPacket(boost::shared_ptr<const RawPacket>, size_t skipBytes = 0);
	
	template <typename T>
	void operator>>(T& t) {
		if(pos + sizeof(T) > pckt->length)
			throw UnpackPacketException("Unpack failure (type)");
		t = *(T*)(pckt->data+pos); pos += sizeof(T);
	};
	template <typename element>
	void operator>>(std::vector<element>& vec)
	{
		if(pckt->length - pos < vec.size() * sizeof(element))
			throw UnpackPacketException("Unpack failure (vector)");
		const size_t toCopy = vec.size() * sizeof(element);
		std::memcpy((void*)(&vec[0]), (pckt->data+pos), toCopy);
		pos += toCopy;
	};
	void operator>>(std::string& text) {
		int i = pos;
		for(; i < pckt->length; ++i)
			if(pckt->data[i] == '\0')
				break;
		if(i >= pckt->length)
			throw UnpackPacketException("Unpack failure (string)");
		text = std::string((char*)(pckt->data + pos)); pos += text.size()+1;
	};
	
private:
	boost::shared_ptr<const RawPacket> pckt;
	size_t pos;
};

};

#endif
