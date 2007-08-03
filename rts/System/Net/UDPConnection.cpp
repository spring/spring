#include "UDPConnection.h"

#include <SDL_timer.h>

#ifdef _WIN32
#include "Platform/Win/win32.h"
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#endif

#include "LogOutput.h"
#include "GlobalStuff.h"
#include "Sync/Syncify.h"
#include "Platform/ConfigHandler.h"

namespace netcode {


#ifdef _WIN32
#else
	typedef struct hostent* LPHOSTENT;
	typedef struct in_addr* LPIN_ADDR;
	const int SOCKET_ERROR = -1;
#endif

const unsigned UDPConnection::hsize = 9;

UDPConnection::UDPConnection(UDPSocket* const NetSocket, const sockaddr_in& MyAddr) : mySocket(NetSocket)
{
	addr=MyAddr;
	lastInOrder=-1;
	waitingPackets.clear();
	firstUnacked=0;
	currentNum=0;
	outgoingLength=0;
	lastSendFrame=0;
	lastNak=-1;
	lastNakTime=0;
	mtu = configHandler.GetInt("MaximumTransmissionUnit", 512) - 9; // subtract spring header size
	if (mtu < 50)
	{
		logOutput.Print("Your MaximumTransmissionUnit is to low (%i, minimum 50)", mtu);
		mtu = 50;
	}
	fragmentedFlushes = 0;
	resentPackets = 0;
}

UDPConnection::UDPConnection(UDPSocket* const NetSocket, const std::string& address, const unsigned port) : mySocket(NetSocket)
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
	
	//TODO same code as in other constructor, merge together
	lastInOrder=-1;
	waitingPackets.clear();
	firstUnacked=0;
	currentNum=0;
	outgoingLength=0;
	lastSendFrame=0;
	lastNak=-1;
	lastNakTime=0;
	mtu = configHandler.GetInt("MaximumTransmissionUnit", 512) - 9; // subtract spring header size
	if (mtu < 50)
	{
		logOutput.Print("Your MaximumTransmissionUnit is to low (%i, minimum 50)", mtu);
		mtu = 50;
	}
	fragmentedFlushes = 0;
	resentPackets = 0;
}

UDPConnection::~UDPConnection()
{
	logOutput.Print("Network statistics for %s",inet_ntoa(addr.sin_addr));
	logOutput.Print("Bytes send/received: %i/%i (Overhead: %i/%i)", dataSent, dataRecv, sentOverhead, recvOverhead);
	logOutput.Print("Packets send/received: %i/%i (Lost: %i%%)", currentNum, lastInOrder, resentPackets * 100 / currentNum);
	logOutput.Print("Flushes requiring fragmentation: %i", fragmentedFlushes);
}

int UDPConnection::SendData(const unsigned char *data, const unsigned length)
{
	if(active){
		if(outgoingLength+length>=NETWORK_BUFFER_SIZE){
			logOutput.Print("Overflow when sending to remote connection %i %i %i",outgoingLength,length,NETWORK_BUFFER_SIZE);
			return 0;
		}
		memcpy(&outgoingData[outgoingLength],data,length);
		outgoingLength+=length;
	}
	return 1;
}

int UDPConnection::GetData(unsigned char *buf, const unsigned length)
{
	if(active){
		unsigned readyLength = 0;

		packetMap::iterator wpi;
		//process all in order packets that we have waiting
		while ((wpi = waitingPackets.find(lastInOrder+1)) != waitingPackets.end()) {
		//	if (readyLength + wpi->second->length >= length) {	// does only work with boost >= 1.34
			if (readyLength + (*wpi).length >= length) {	// does only work with boost < 1.34
				logOutput.Print("Overflow in incoming network buffer");
				break;
			}

			lastInOrder++;

		//	memcpy(buf+readyLength,wpi->second->data,wpi->second->length);
		//	readyLength += (wpi->second)->length;
			memcpy(buf + readyLength, (*wpi).data, (*wpi).length);
			readyLength += (*wpi).length;

			waitingPackets.erase(wpi);
		}

		return readyLength;
	} else {
		return -1;
	}
}

void UDPConnection::Update()
{
	if (!active)
		return;

	const float curTime = static_cast<float>(SDL_GetTicks())/1000.0f;

	if((dataRecv == 0) && lastSendTime<curTime-1){		//server hasnt responded so try to send the connection attempt again
		SendRawPacket(unackedPackets[0].data,unackedPackets[0].length,0);
		lastSendTime=curTime;
	}
	else if(lastSendTime<curTime-5 && !(dataRecv == 0)){		//we havent sent anything for a while so send something to prevent timeout
		Ping();
	}
	else if(lastSendTime<curTime-0.2f && !waitingPackets.empty()){	//we have at least one missing incomming packet lying around so send a packet to ensure the other side get a nak
		Ping();
	}
	else if(lastReceiveTime < curTime-((dataRecv == 0) ? 40 : 30))
	{
		active=false;
	}

	if(outgoingLength>0 && (lastSendTime < (curTime-0.2f+outgoingLength*0.01f) || lastSendFrame < gs->frameNum-1)){
		Flush();
	}
}

void UDPConnection::ProcessRawPacket(RawPacket* packet)
{
	lastReceiveTime=static_cast<float>(SDL_GetTicks())/1000.0f;

	int packetNum = *(int*)packet->data;
	int ack = *(int*)(packet->data+4);
	unsigned char nak = *(unsigned char*)(packet->data+8);

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

	if(!active || lastInOrder>=packetNum || waitingPackets.find(packetNum)!=waitingPackets.end())
	{
		delete packet;
		return;
	}

	waitingPackets.insert(packetNum, new RawPacket(packet->data + hsize, packet->length - hsize));

	dataRecv += packet->length;
	recvOverhead += hsize;
	
	delete packet;
}

void UDPConnection::Flush()
{
	if (outgoingLength <= 0)
		return;

	lastSendFrame=gs->frameNum;
	lastSendTime=static_cast<float>(SDL_GetTicks())/1000.0f;

	// Manually fragment packets to respect configured MTU.
	// This is an attempt to fix the bug where players drop out of the game if
	// someone in the game gives a large order.

	if (outgoingLength > mtu)
		++fragmentedFlushes;

	for (int pos = 0; outgoingLength != 0; pos += mtu) {
		int length = std::min(mtu, outgoingLength);
		SendRawPacket(outgoingData + pos, length, currentNum++);
		unackedPackets.push_back(new RawPacket(outgoingData + pos, length));
		outgoingLength -= length;
	}
}

void UDPConnection::Ping()
{
	SendData(&NETMSG_HELLO, sizeof(NETMSG_HELLO));
}

bool UDPConnection::CheckAddress(const sockaddr_in& from) const
{
	if(active){
		if(
#ifdef _WIN32
			addr.sin_addr.S_un.S_addr==from.sin_addr.S_un.S_addr
#else
			addr.sin_addr.s_addr==from.sin_addr.s_addr
#endif
			&& addr.sin_port==from.sin_port){
			return true;
		}
	}
	return false;
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
	if (!active)
		return;

	const unsigned hsize = 9;
	unsigned char tempbuf[NETWORK_BUFFER_SIZE];
	*(int*)tempbuf = packetNum;
	*(int*)(tempbuf+4) = lastInOrder;
	if(!waitingPackets.empty() && waitingPackets.find(lastInOrder+1)==waitingPackets.end()){
		int nak = (waitingPackets.begin().key() - 1) - lastInOrder;
		assert(nak >= 0);
		if (nak <= 255)
			*(unsigned char*)(tempbuf+8) = (unsigned char)nak;
		else
			*(unsigned char*)(tempbuf+8) = 255;
	}
	else {
		*(unsigned char*)(tempbuf+8) = 0;
	}

	memcpy(&tempbuf[hsize],data,length);

	mySocket->SendTo(tempbuf, length+hsize, &addr);
	
	dataSent += length;
	sentOverhead += hsize;
}




} // namespace netcode
