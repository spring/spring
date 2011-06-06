/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PROTOCOL_DEF_H
#define _PROTOCOL_DEF_H

namespace netcode {

class ProtocolDef
{
public:
	static ProtocolDef* GetInstance();

	void AddType(const unsigned char id, const int msgLength);

	/**
	 * @return <  -1: invalid id
	 *         == -1: invalid length
	 *         ==  0: unknown length, buffer too short
	 *         >   0: actual length
	 */
	int PacketLength(const unsigned char* const buf, const unsigned bufLength) const;
	bool IsValidLength(const int pktLength, const unsigned bufLength) const;
	bool IsValidPacket(const unsigned char* const buf, const unsigned bufLength) const;

private:
	ProtocolDef();

	struct MsgType {
		int length;
	};

	MsgType msg[256];
};

} // namespace netcode

#endif // _PROTOCOL_DEF_H
