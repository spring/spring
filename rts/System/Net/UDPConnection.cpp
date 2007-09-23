#include "UDPConnection.h"

#include <SDL/SDL_timer.h>

#ifdef _WIN32
#include "Platform/Win/win32.h"
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "ProtocolDef.h"
#include <boost/version.hpp>

namespace netcode {


#ifdef _WIN32
#else
	typedef struct hostent* LPHOSTENT;
	typedef struct in_addr* LPIN_ADDR;
	const int SOCKET_ERROR = -1;
#endif

const unsigned UDPConnection::hsize = 9;

UDPConnection::UDPConnection(boost::shared_ptr<UDPSocket> NetSocket, const sockaddr_in& MyAddr, const ProtocolDef* const myproto) : mySocket(NetSocket), proto(myproto)
{
	addr=MyAddr;
	Init();
}

UDPConnection::UDPConnection(boost::shared_ptr<UDPSocket> NetSocket, const std::string& address, const unsigned port, const ProtocolDef* const myproto) : mySocket(NetSocket), proto(myproto)
{
	LPHOSTENT lpHostEntry;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

#ifdef _WIN32
	unsigned long ul;
	if((ul=inet_addr(address.c_str()))!=INADDR_NONE){
		addr.sin_addr.S_un.S_addr = 	ul;
	} else
#else
		if(inet_aton(address.c_str(),&(addr.sin_addr))==0)
#endif
		{
			lpHostEntry = gethostbyname(address.c_str());
			if (lpHostEntry == NULL)
			{
				throw network_error("Error looking up server from DNS: "+address);
			}
			addr.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
		}
	Init();
}

UDPConnection::~UDPConnection()
{
	if (fragmentBuffer)
		delete fragmentBuffer;
}

void UDPConnection::SendData(const unsigned char *data, const unsigned length)
{
	if(outgoingLength+length>=NETWORK_BUFFER_SIZE){
		throw network_error("Buffer overflow in UDPConnection (SendData)");
	}
	memcpy(&outgoingData[outgoingLength],data,length);
	outgoingLength+=length;
}

unsigned UDPConnection::GetData(unsigned char *buf)
{
	if (!msgQueue.empty())
	{
		RawPacket* msg = msgQueue.front();
		unsigned length = msg->length;
		memcpy(buf, msg->data, length);
		delete msg;
		msgQueue.pop();
		return length;
	}
	else
	{
		return 0;
	}
}

void UDPConnection::Update()
{
	const float curTime = static_cast<float>(SDL_GetTicks())/1000.0f;
	bool force = false;	// should we force to send a packet?
	
	if((dataRecv == 0) && lastSendTime<curTime-1 && !unackedPackets.empty()){		//server hasnt responded so try to send the connection attempt again
		SendRawPacket(unackedPackets[0].data,unackedPackets[0].length,0);
		lastSendTime = curTime;
		force = true;
	}
	
	if (lastSendTime<curTime-5 && !(dataRecv == 0)) { //we havent sent anything for a while so send something to prevent timeout
		force = true;
	}
	else if(lastSendTime<curTime-0.2f && !waitingPackets.empty()){	//we have at least one missing incomming packet lying around so send a packet to ensure the other side get a nak
		force = true;
	}
	
	if (outgoingLength>0 && (lastSendTime < (curTime-0.2f+outgoingLength*0.01f)))
	{
		force = true;
	}
	
	Flush(force);
}

void UDPConnection::ProcessRawPacket(RawPacket* packet)
{
	lastReceiveTime=static_cast<float>(SDL_GetTicks())/1000.0f;
	dataRecv += packet->length;
	recvOverhead += hsize;

	int packetNum = *(int*)packet->data;
	int ack = *(int*)(packet->data+4);
	unsigned char nak = packet->data[8];

	AckPackets(ack);

	if (nak > 0)	// we have lost $nak packets
	{
		int nak_abs = nak + firstUnacked - 1;
		if (nak_abs!=lastNak || lastNakTime < lastReceiveTime-0.1f)
		{
			// resend all packets from firstUnacked till nak_abs
			lastNak=nak_abs;
			lastNakTime=lastReceiveTime;
			for(int b=firstUnacked;b<=nak_abs;++b){
				SendRawPacket(unackedPackets[b-firstUnacked].data,unackedPackets[b-firstUnacked].length,b);
				++resentPackets;
			}
		}
	}

	if(lastInOrder>=packetNum || waitingPackets.find(packetNum)!=waitingPackets.end())
	{
		delete packet;
		return;
	}

	waitingPackets.insert(packetNum, new RawPacket(packet->data + hsize, packet->length - hsize));	
	delete packet;
	
	packetMap::iterator wpi;
	//process all in order packets that we have waiting
	while ((wpi = waitingPackets.find(lastInOrder+1)) != waitingPackets.end())
	{
		unsigned char buf[8000];
		unsigned bufLength = 0;
		
		if (fragmentBuffer)
		{
			// combine with fragment buffer
			bufLength += fragmentBuffer->length;
			memcpy(buf, fragmentBuffer->data, bufLength);
			delete fragmentBuffer;
			fragmentBuffer = 0;
		}
		
		lastInOrder++;
#if (BOOST_VERSION >= 103400)
		memcpy(buf+bufLength,wpi->second->data,wpi->second->length);
		bufLength += (wpi->second)->length;
#else
		memcpy(buf + bufLength, (*wpi).data, (*wpi).length);
		bufLength += (*wpi).length;
#endif		
		waitingPackets.erase(wpi);
		
		for (unsigned pos = 0; pos < bufLength;)
		{
			char msgid = buf[pos];
			if (proto->IsAllowed(msgid))
			{
				unsigned msglength = 0;
				if (proto->HasFixedLength(msgid))
				{
					msglength = proto->GetLength(msgid);
				}
				else
				{
					int length_t = proto->GetLength(msgid);
					if (length_t == -1)
					{
						msglength = buf[pos+1];
					}
					else if (length_t == -2)
					{
						msglength = *(short*)(buf+pos+1);
					}
				}
				
				if (bufLength >= pos + msglength)
				{
					msgQueue.push(new RawPacket(buf + pos, msglength));
					pos += msglength;
				}
				else
				{
					fragmentBuffer = new RawPacket(buf + pos, bufLength-pos);
					break;
				}
			}
			else
			{
				// error
				pos++;
			}
		}
	}

}

void UDPConnection::Flush(const bool forced)
{
	if (outgoingLength <= 0 && !forced)
		return;

	lastSendTime=static_cast<float>(SDL_GetTicks())/1000.0f;

	// Manually fragment packets to respect configured UDP_MTU.
	// This is an attempt to fix the bug where players drop out of the game if
	// someone in the game gives a large order.

	if (outgoingLength > proto->UDP_MTU)
		++fragmentedFlushes;

	unsigned pos = 0;
	do
	{
		unsigned length = std::min(proto->UDP_MTU, outgoingLength);
		SendRawPacket(outgoingData + pos, length, currentNum++);
		unackedPackets.push_back(new RawPacket(outgoingData + pos, length));
		outgoingLength -= length;
		pos += proto->UDP_MTU;
	} while (outgoingLength != 0);
}

bool UDPConnection::CheckTimeout() const
{
	const float curTime = static_cast<float>(SDL_GetTicks())/1000.0f;
	if(lastReceiveTime < curTime-((dataRecv == 0) ? 45 : 30))
	{
		return true;
	}
	else
		return false;
}

bool UDPConnection::CheckAddress(const sockaddr_in& from) const
{
	if(
#ifdef _WIN32
		addr.sin_addr.S_un.S_addr==from.sin_addr.S_un.S_addr
#else
		addr.sin_addr.s_addr==from.sin_addr.s_addr
#endif
		&& addr.sin_port==from.sin_port){
		return true;
	}
	else
		return false;
}

void UDPConnection::Init()
{
	lastReceiveTime = static_cast<float>(SDL_GetTicks())/1000.0f;
	lastInOrder=-1;
	waitingPackets.clear();
	firstUnacked=0;
	currentNum=0;
	outgoingLength=0;
	lastNak=-1;
	lastNakTime=0;
	lastSendTime=0;
	sentOverhead = 0;
	recvOverhead = 0;
	fragmentedFlushes = 0;
	fragmentBuffer = 0;
	resentPackets = 0;
}

void UDPConnection::AckPackets(const int nextAck)
{
	while(nextAck>=firstUnacked){
		unackedPackets.pop_front();
		firstUnacked++;
	}
}

void UDPConnection::SendRawPacket(const unsigned char* data, const unsigned length, const int packetNum)
{
	const unsigned hsize = 9;
	unsigned char tempbuf[NETWORK_BUFFER_SIZE];
	*(int*)tempbuf = packetNum;
	*(int*)(tempbuf+4) = lastInOrder;
	if(!waitingPackets.empty() && waitingPackets.find(lastInOrder+1)==waitingPackets.end()){
#if (BOOST_VERSION >= 103400)
		int nak = (waitingPackets.begin()->first - 1) - lastInOrder;
#else
		int nak = (waitingPackets.begin().key() - 1) - lastInOrder;
#endif
		assert(nak >= 0);
		if (nak <= 255)
			*(unsigned char*)(tempbuf+8) = (unsigned char)nak;
		else
			*(unsigned char*)(tempbuf+8) = 255;
	}
	else {
		*(unsigned char*)(tempbuf+8) = 0;
	}

	memcpy(tempbuf+hsize, data, length);

	mySocket->SendTo(tempbuf, length+hsize, &addr);
	
	dataSent += length;
	sentOverhead += hsize;
}




} // namespace netcode
