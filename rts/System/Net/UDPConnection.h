#ifndef _REMOTE_CONNECTION
#define _REMOTE_CONNECTION

#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/ip/udp.hpp>
#include <deque>
#include <list>

#include "Connection.h"
#include "System/myTime.h"

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
	UDPConnection(boost::shared_ptr<boost::asio::ip::udp::socket> NetSocket, const boost::asio::ip::udp::endpoint& MyAddr);
	UDPConnection(int sourceport, const std::string& address, const unsigned port);
	virtual ~UDPConnection();

	/**
	@brief Send packet to other instance
	*/
	virtual void SendData(boost::shared_ptr<const RawPacket> data);

	virtual bool HasIncomingData() const;

	virtual boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;

	/**
	@brief use this to recieve ready data
	@return a network message encapsulated in a RawPacket,
	or NULL if there are no more messages available.
	*/
	virtual boost::shared_ptr<const RawPacket> GetData();

	/**
	@brief update internals
	Check for unack'd packets, timeout etc.
	*/
	virtual void Update();
	
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
	bool CheckAddress(const boost::asio::ip::udp::endpoint&) const;
	std::string GetFullAddress() const;
	
	void SetMTU(unsigned mtu);

	/// The size of the protocol-header (Packets smaller than this get rejected)
	static const unsigned hsize;

private:
	void Init();
	
	spring_time lastSendTime;
	spring_time lastReceiveTime;
	
	typedef boost::ptr_map<int,RawPacket> packetMap;
	typedef std::list< boost::shared_ptr<const RawPacket> > packetList;
	/// all packets with number <= nextAck arrived at the other end
	void AckPackets(const int nextAck);
	/// add header to data and send it
	void SendRawPacket(const unsigned char* data, const unsigned length, const int packetNum);
	/// address of the other end
	boost::asio::ip::udp::endpoint addr;

	/// maximum size of packets to send
	unsigned mtu;
	
	bool sharedSocket;

	///outgoing stuff (pure data without header) waiting to be sended
	packetList outgoingData;

	/// packets the other side didn't ack'ed until now
	boost::ptr_deque<RawPacket> unackedPackets;
	int firstUnacked;
	int currentNum;

	/// packets we have recieved but not yet read
	packetMap waitingPackets;
	int lastInOrder;
	int lastNak;
	spring_time lastNakTime;
	std::deque< boost::shared_ptr<const RawPacket> > msgQueue;

	/** Our socket.
	*/
	boost::shared_ptr<boost::asio::ip::udp::socket> mySocket;
	
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
