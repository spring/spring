#ifndef UNPACK_PACKET_H
#define UNPACK_PACKET_H

#include <string>
#include <vector>
#include <cstring>
#include <boost/shared_ptr.hpp>

#include "RawPacket.h"

namespace netcode
{

class UnpackPacket
{
public:
	UnpackPacket(boost::shared_ptr<const RawPacket>, size_t skipBytes = 0);
	
	template <typename T>
	void operator>>(T& t) {t = *(T*)(pckt->data+pos); pos += sizeof(T);};
	template <typename element>
	void operator>>(std::vector<element>& vec)
	{
		assert(pckt->length - pos >= vec.size() * sizeof(element));
		const size_t toCopy = vec.size() * sizeof(element);
		std::memcpy((void*)(&vec[0]), (pckt->data+pos), toCopy);
		pos += toCopy;
	};
	void operator>>(std::string& text) {text = std::string((char*)(pckt->data + pos)); pos += text.size()+1;};
	
private:
	boost::shared_ptr<const RawPacket> pckt;
	size_t pos;
};

};

#endif
