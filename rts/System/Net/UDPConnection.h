#ifndef _REMOTE_CONNECTION
#define _REMOTE_CONNECTION

#include "Connection.h"
#include "Net.h"
#include "UDPSocket.h"
#include "RawPacket.h"

#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>

namespace netcode {

/**
How Spring protocolheader looks like (size in bytes):
4 (int): number of packet (continuous)
4 (int):	last in order (tell the client we recieved all packages with packetNumber less or equal)
1 (unsigned char): nak (we missed x packets, starting with firstUnacked)

*/

/**
@brief Communication class over UDP
*/
class UDPConnection : public CConnection
{
public:
	UDPConnection(boost::shared_ptr<UDPSocket> NetSocket, const sockaddr_in& MyAddr);
	UDPConnection(boost::shared_ptr<UDPSocket> NetSocket, const std::string& address, const unsigned port);
	virtual ~UDPConnection();

	/// use this if you want data to be sent
	virtual void SendData(const unsigned char *data, const unsigned length);

	virtual const RawPacket* Peek(unsigned ahead) const;

	/**
	@brief use this to recieve ready data
	@return a network message encapsulated in a RawPacket,
	or NULL if there are no more messages available.
	*/
	virtual RawPacket* GetData();

	/**
	@brief update internals
	Check for unack'd packets, timeout etc.
	*/
	void Update();
	
	/**
	@brief strip and parse header data and add data to waitingPackets
	UDPConnection takes the ownership of the packet and will delete it in this func
	*/
	void ProcessRawPacket(RawPacket* packet);

	/// send all data waiting in char outgoingData[]
	virtual void Flush(const bool forced = false);
	
	virtual bool CheckTimeout() const;
	
	virtual std::string Statistics() const;

	/// do we have these address?
	bool CheckAddress(const sockaddr_in&) const;

	/// The size of the protocol-header (Packets smaller than this get rejected)
	static const unsigned hsize;

private:
	typedef std::deque<RawPacket*> MsgQueue;

	void Init();
	
	unsigned lastSendTime;
	unsigned lastReceiveTime;
	
	typedef boost::ptr_map<int,RawPacket> packetMap;
	/// all packets with number <= nextAck arrived at the other end
	void AckPackets(const int nextAck);
	/// add header to data and send it
	void SendRawPacket(const unsigned char* data, const unsigned length, const int packetNum);
	/// address of the other end
	sockaddr_in addr;

	///outgoing stuff (pure data without header) waiting to be sended
	unsigned char outgoingData[NETWORK_BUFFER_SIZE];
	unsigned outgoingLength;

	/// packets the other side didn't ack'ed until now
	boost::ptr_deque<RawPacket> unackedPackets;
	int firstUnacked;
	int currentNum;

	/// packets we have recieved but not yet read
	packetMap waitingPackets;
	int lastInOrder;
	int lastNak;
	unsigned lastNakTime;
	MsgQueue msgQueue;

	/** Our socket.
	*/
	boost::shared_ptr<UDPSocket> const mySocket;
	
	RawPacket* fragmentBuffer;

	// Traffic statistics and stuff //
	/// number of calls to Flush() that needed to sent multiple packets because of mtu
	unsigned fragmentedFlushes;
	
	/// packets that are resent
	unsigned resentPackets;
	
	unsigned droppedPackets;
	
	unsigned sentOverhead, recvOverhead;
	unsigned sentPackets, recvPackets;
};

} //namespace netcode


#endif
