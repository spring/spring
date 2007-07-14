#ifndef _REMOTE_CONNECTION
#define _REMOTE_CONNECTION

#include "Connection.h"

#include <deque>
#include <map>

namespace netcode {

#ifndef _WIN32
	typedef int SOCKET;
#endif

/**
How Spring protocolheader looks like (size in bytes):
4 (int): number of packet (continuous)
4 (int):	last in order (tell the client we recieved all packages with packetNumber less or equal)
1 (unsigned char): nak (we missed x packets, starting with firstUnacked)

*/

struct Packet
{
	Packet();
	Packet(const void* indata,const unsigned int length);
	~Packet();

	unsigned int length;
	unsigned char* data;
};

class CRemoteConnection : public CConnection
{
public:
	CRemoteConnection(const sockaddr_in MyAddr, const SOCKET* const NetSocket);
	virtual ~CRemoteConnection();

	/// use this if you want data to be sent
	virtual int SendData(const unsigned char *data, const unsigned length);
	/**
	@brief use this to recieve ready data
	Will read all waiting in-order packages from waitingPackets, copy their  data to buf and deleting them
	@return bytes of data read, or -1 on error
	@param buf buffer to hold the data
	@param length the free space in the buffer
	*/
	virtual int GetData(unsigned char *buf, const unsigned length);

	/**
	@brief update internals
	Check for unack'd packets, timeout etc.
	*/
	virtual void Update(const bool inInitialConnect);
	/// strip and parse header data and add data to waitingPackets
	virtual void ProcessRawPacket(const unsigned char* data, const unsigned length);

	/// send all data waiting in char outgoingData[]
	virtual void Flush();
	/// send a NETMSG_HELLO
	virtual void Ping();

	/// do we have these address?
	virtual bool CheckAddress(const sockaddr_in&) const;


private:
	/// all packets with number <= nextAck arrived at the other end
	void AckPackets(const int nextAck);
	/// add header to data and send it
	void SendRawPacket(const unsigned char* data, const unsigned length, const int packetNum);
	/// address of the other end
	sockaddr_in addr;

	int lastSendFrame;

	///outgoing stuff (pure data without header) waiting to be sended
	unsigned char outgoingData[NETWORK_BUFFER_SIZE];
	int outgoingLength;

	/// packets the other side didn't ack'ed until now
	std::deque<Packet*> unackedPackets;
	int firstUnacked;
	int currentNum;

	/// packets we have recieved but not yet read
	std::map<int,Packet*> waitingPackets;
	int lastInOrder;
	int lastNak;
	float lastNakTime;

	/** Our socket. Every CRemoteConnection uses the same socket, which is bad but
	can't be circumvented since we don't want more than 1 listening UDP-Sockets. */
	const SOCKET* const mySocket;

	/** Configurable Maximum Transmission Unit. This is enforced by Flush:
	it only gives this much bytes to each SendRawPacket() call.
	This is exclusive the Spring header of 9 bytes. */
	int mtu;

	/// number of calls to Flush() that needed to sent multiple packets because of mtu
	int fragmentedFlushes;

	/// packets that are resent
	int resentPackets;
};

} //namespace netcode


#endif
