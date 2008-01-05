#include "UDPConnection.h"

#include <SDL_timer.h>
#include <boost/version.hpp>
#include <boost/format.hpp>

#ifdef _WIN32
 #include "Platform/Win/win32.h"
#else
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netdb.h>
#endif

#include "ProtocolDef.h"
#include "Exception.h"

namespace netcode {


#ifdef _WIN32
#else
	typedef struct hostent* LPHOSTENT;
	typedef struct in_addr* LPIN_ADDR;
	const int SOCKET_ERROR = -1;
#endif

const unsigned UDPConnection::hsize = 9;

UDPConnection::UDPConnection(boost::shared_ptr<UDPSocket> NetSocket, const sockaddr_in& MyAddr) : mySocket(NetSocket)
{
	addr=MyAddr;
	Init();
}

UDPConnection::UDPConnection(boost::shared_ptr<UDPSocket> NetSocket, const std::string& address, const unsigned port) : mySocket(NetSocket)
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

	while (!msgQueue.empty())
	{
		delete msgQueue.front();
		msgQueue.pop_front();
	}
}

void UDPConnection::SendData(const unsigned char *data, const unsigned length)
{
	if(outgoingLength+length>=NETWORK_BUFFER_SIZE){
		throw network_error("Buffer overflow in UDPConnection (SendData)");
	}
	memcpy(&outgoingData[outgoingLength],data,length);
	outgoingLength+=length;
}

const RawPacket* UDPConnection::Peek(unsigned ahead) const
{
	if (ahead < msgQueue.size())
		return msgQueue[ahead];

	return NULL;
}

RawPacket* UDPConnection::GetData()
{
	if (!msgQueue.empty())
	{
		RawPacket* msg = msgQueue.front();
		msgQueue.pop_front();
		return msg;
	}
	else
		return NULL;
}

void UDPConnection::Update()
{
	const unsigned curTime = SDL_GetTicks();
	bool force = false;	// should we force to send a packet?

	if((dataRecv == 0) && lastSendTime < curTime-1000 && !unackedPackets.empty()){		//server hasnt responded so try to send the connection attempt again
		SendRawPacket(unackedPackets[0].data,unackedPackets[0].length,0);
		lastSendTime = curTime;
		force = true;
	}

	if (lastSendTime<curTime-5000 && !(dataRecv == 0)) { //we havent sent anything for a while so send something to prevent timeout
		force = true;
	}
	else if(lastSendTime<curTime-200 && !waitingPackets.empty()){	//we have at least one missing incomming packet lying around so send a packet to ensure the other side get a nak
		force = true;
	}

	Flush(force);
}

void UDPConnection::ProcessRawPacket(RawPacket* packet)
{
	lastReceiveTime=SDL_GetTicks();
	dataRecv += packet->length;
	recvOverhead += hsize;
	++recvPackets;

	int packetNum = *(int*)packet->data;
	int ack = *(int*)(packet->data+4);
	unsigned char nak = packet->data[8];

	AckPackets(ack);

	if (nak > 0)	// we have lost $nak packets
	{
		int nak_abs = nak + firstUnacked - 1;
		if (nak_abs >= currentNum)
		{
			// we got a nak for packets which never got sent
			//TODO give error message
		}
		else if (nak_abs != lastNak || lastNakTime < lastReceiveTime-100)
		{
			// resend all packets from firstUnacked till nak_abs
			lastNak=nak_abs;
			lastNakTime=lastReceiveTime;
			for (int b = firstUnacked; b <= nak_abs; ++b)
			{
				SendRawPacket(unackedPackets[b-firstUnacked].data,unackedPackets[b-firstUnacked].length,b);
				++resentPackets;
			}
		}
	}

	if (lastInOrder >= packetNum || waitingPackets.find(packetNum) != waitingPackets.end())
	{
		++droppedPackets;
		delete packet;
		return;
	}

	waitingPackets.insert(packetNum, new RawPacket(packet->data + hsize, packet->length - hsize));
	delete packet;
	packet = NULL;

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
			fragmentBuffer = NULL;
		}

		lastInOrder++;
#if (BOOST_VERSION >= 103400)
		memcpy(buf + bufLength, wpi->second->data, wpi->second->length);
		bufLength += (wpi->second)->length;
#else
		memcpy(buf + bufLength, (*wpi).data, (*wpi).length);
		bufLength += (*wpi).length;
#endif
		waitingPackets.erase(wpi);

		for (unsigned pos = 0; pos < bufLength;)
		{
			char msgid = buf[pos];
			ProtocolDef* proto = ProtocolDef::instance();
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

					// got enough data in the buffer to read the length of the message?
					if (bufLength >= pos + -length_t)
					{
						// yes => read the length (as byte or word)
						if (length_t == -1)
						{
							msglength = buf[pos+1];
						}
						else if (length_t == -2)
						{
							msglength = *(short*)(buf+pos+1);
						}
					}
					else
					{
						// no => store the fragment and break
						fragmentBuffer = new RawPacket(buf + pos, bufLength - pos);
						break;
					}
				}

				// if this isn't true we'll loop infinitely while filling up memory
				assert(msglength != 0);

				// got the complete message in the buffer?
				if (bufLength >= pos + msglength)
				{
					// yes => add to msgQueue and keep going
					msgQueue.push_back(new RawPacket(buf + pos, msglength));
					pos += msglength;
				}
				else
				{
					// no => store the fragment and break
					fragmentBuffer = new RawPacket(buf + pos, bufLength - pos);
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
	const float curTime = SDL_GetTicks();
	if (forced || (outgoingLength>0 && (lastSendTime < (curTime - 200 + outgoingLength * 10))))
	{
		ProtocolDef* proto = ProtocolDef::instance();
		lastSendTime=SDL_GetTicks();

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
}

bool UDPConnection::CheckTimeout() const
{
	const unsigned curTime = SDL_GetTicks();
	const unsigned timeout = ((dataRecv == 0) ? 45000 : 30000);
	if((lastReceiveTime+timeout) < curTime)
	{
		return true;
	}
	else
		return false;
}

std::string UDPConnection::Statistics() const
{
	std::string msg = "Statistics for UDP connection:\n";
	msg += str( boost::format("Recieved: %1% bytes in %2% packets (%3% bytes/package)\n") %dataRecv %recvPackets %((float)dataRecv / (float)recvPackets));
	msg += str( boost::format("Sent: %1% bytes in %2% packets (%3% bytes/package)\n") %dataSent %sentPackets %((float)dataSent / (float)sentPackets));
	msg += str( boost::format("Relative protocol overhead: %1% up, %2% down\n") %((float)sentOverhead / (float)dataSent) %((float)recvOverhead / (float)dataRecv) );
	msg += str( boost::format("%1% incoming packets had been dropped, %2% outgoing packets had to be resent\n") %droppedPackets %resentPackets);
	msg += str( boost::format("%1% packets were splitted due to MTU\n") %fragmentedFlushes);
	return msg;
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
	lastReceiveTime = SDL_GetTicks();
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
	sentPackets = recvPackets = 0;
	droppedPackets = 0;
}

void UDPConnection::AckPackets(const int nextAck)
{
	while (nextAck >= firstUnacked && !unackedPackets.empty()){
		unackedPackets.pop_front();
		firstUnacked++;
	}
}

void UDPConnection::SendRawPacket(const unsigned char* data, const unsigned length, const int packetNum)
{
	unsigned char* tempbuf = new unsigned char[hsize+length];
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
	delete[] tempbuf;

	dataSent += length;
	sentOverhead += hsize;
	++sentPackets;
}


} // namespace netcode
