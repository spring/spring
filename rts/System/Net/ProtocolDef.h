/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PROTO_DEF
#define _PROTO_DEF

namespace netcode {
	
class ProtocolDef
{
public:
	static ProtocolDef* instance();
	
	void AddType(const unsigned char id, const int MsgLength);

	int PacketLength(const unsigned char* const buf, const unsigned bufLength) const;
	bool IsValidLength(const int pktLength, const unsigned bufLength) const;
	bool IsValidPacket(const unsigned char* const buf, const unsigned bufLength) const;

private:
	ProtocolDef();
	ProtocolDef( const ProtocolDef& );
	
	struct MsgType
	{
		int Length;
	};
	
	MsgType msg[256];
	static ProtocolDef* instance_ptr;
};

}

#endif
