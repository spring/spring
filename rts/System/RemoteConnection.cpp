#include "RemoteConnection.h"

#include <SDL_timer.h>

#ifdef _WIN32
#include "Platform/Win/win32.h"
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "LogOutput.h"
#include "GlobalStuff.h"
#include "Sync/Syncify.h"
#include "Platform/ConfigHandler.h"

namespace netcode {


CRemoteConnection::CRemoteConnection(const sockaddr_in MyAddr, UDPSocket* const NetSocket) : mySocket(NetSocket)
{
	addr=MyAddr;
	lastInOrder=-1;
	waitingPackets.clear();
	firstUnacked=0;
	currentNum=0;
	outgoingLength=0;
	lastReceiveTime= static_cast<float>(SDL_GetTicks())/1000.0f;
	lastSendFrame=0;
	lastSendTime=0;
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

CRemoteConnection::~CRemoteConnection()
{
	logOutput.Print("Network statistics for %s",inet_ntoa(addr.sin_addr));
	logOutput.Print("Bytes send/received: %i/%i (Overhead: %i/%i)", dataSent, dataRecv, sentOverhead, recvOverhead);
	logOutput.Print("Packets send/received: %i/%i (Lost: %i%%)", currentNum, lastInOrder, resentPackets * 100 / currentNum);
	logOutput.Print("Flushes requiring fragmentation: %i", fragmentedFlushes);
}

int CRemoteConnection::SendData(const unsigned char *data, const unsigned length)
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

int CRemoteConnection::GetData(unsigned char *buf, const unsigned length)
{
	if(active){
		unsigned readyLength = 0;

		packetMap::iterator wpi;
		//process all in order packets that we have waiting
		while ((wpi = waitingPackets.find(lastInOrder+1)) != waitingPackets.end()) {
		//	if (readyLength + wpi->second->length >= length) {
			if (readyLength + (*wpi).length >= length) {
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

void CRemoteConnection::Update(const bool inInitialConnect)
{
	if (!active)
		return;

	const float curTime = static_cast<float>(SDL_GetTicks())/1000.0f;

	if(inInitialConnect && lastSendTime<curTime-1){		//server hasnt responded so try to send the connection attempt again
		SendRawPacket(unackedPackets[0].data,unackedPackets[0].length,0);
		lastSendTime=curTime;
	}
	if(lastSendTime<curTime-5 && !inInitialConnect){		//we havent sent anything for a while so send something to prevent timeout
		Ping();
	}
	if(lastSendTime<curTime-0.2f && !waitingPackets.empty()){	//we have at least one missing incomming packet lying around so send a packet to ensure the other side get a nak
		Ping();
	}

	if(lastReceiveTime < curTime-(inInitialConnect ? 40 : 30))
	{
		active=false;
	}

	if(outgoingLength>0 && (lastSendTime < (curTime-0.2f+outgoingLength*0.01f) || lastSendFrame < gs->frameNum-1)){
		Flush();
	}
}

void CRemoteConnection::ProcessRawPacket(const unsigned char* data, const unsigned length)
{
	lastReceiveTime=static_cast<float>(SDL_GetTicks())/1000.0f;

	const unsigned hsize = 9;
	int packetNum=*(int*)data;
	int ack=*(int*)(data+4);
	unsigned char nak = *(unsigned char*)(data+8);

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
		return;

	waitingPackets.insert(packetNum, new RawPacket(data+hsize,length-hsize));

	dataRecv += length;
	recvOverhead += hsize;
}

void CRemoteConnection::Flush()
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

void CRemoteConnection::Ping()
{
	SendData(&NETMSG_HELLO, sizeof(NETMSG_HELLO));
}

bool CRemoteConnection::CheckAddress(const sockaddr_in& from) const
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

void CRemoteConnection::AckPackets(const int nextAck)
{
	while(nextAck>=firstUnacked){
		unackedPackets.pop_front();
		firstUnacked++;
	}
}

void CRemoteConnection::SendRawPacket(const unsigned char* data, const unsigned length, const int packetNum)
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
