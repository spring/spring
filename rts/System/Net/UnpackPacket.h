#ifndef UNPACK_PACKET_H
#define UNPACK_PACKET_H

#include <string>
#include <boost/shared_ptr.hpp>

#include "RawPacket.h"

namespace netcode
{

class UnpackPacket
{
public:
	UnpackPacket(boost::shared_ptr<const RawPacket>);
	
	template <typename T>
	void operator>>(T& t) {t = *(T*)(pckt->data+pos); pos += sizeof(T);};
	void operator>>(std::string& text) {text = std::string((char*)(pckt->data + pos)); pos += text.size()+1;};
	
private:
	boost::shared_ptr<const RawPacket> pckt;
	unsigned pos;
};

};

#endif
