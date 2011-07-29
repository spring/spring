/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "UDPConnection.h"


#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

#include "System/mmgr.h"

#include "Socket.h"
#include "ProtocolDef.h"
#include "Exception.h"
#include "System/Config/ConfigHandler.h"
#include "System/CRC.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"

namespace netcode {
using namespace boost::asio;

static const unsigned udpMaxPacketSize = 4096;
static const int maxChunkSize = 254;
static const int chunksPerSec = 30;

// for reliability testing, introduce fake packet loss with a percentage probability
#define PACKET_LOSS_FACTOR 0                       // in [0, 100)
#define SEVERE_PACKET_LOSS_FACTOR 0                // in [0, 100)
#define PACKET_CORRUPTION_FACTOR 0                 // in [0, 100)
#define SEVERE_PACKET_LOSS_MAX_COUNT 60            // max continuous number of packets to be lost
#define RANDOM_NUMBER (rand() / float(RAND_MAX))   // in [0, 1)

#if PACKET_LOSS_FACTOR > 0 || SEVERE_PACKET_LOSS_FACTOR > 0 || PACKET_CORRUPTION_FACTOR > 0
static int lossCounter = 0;
bool EMULATE_PACKET_LOSS(int data) {
	if (data < 1000)
		return false;
	if (RANDOM_NUMBER < (PACKET_LOSS_FACTOR / 100.0f))
		return true;

	const bool loss = RANDOM_NUMBER < (SEVERE_PACKET_LOSS_FACTOR / 100.0f);

	if (loss && lossCounter == 0)
		lossCounter = SEVERE_PACKET_LOSS_MAX_COUNT * RANDOM_NUMBER;

	return lossCounter > 0 && lossCounter--;
}
void EMULATE_PACKET_CORRUPTION(int data, uint8_t& crc) {
	if ((data > 1000) && (RANDOM_NUMBER < (PACKET_CORRUPTION_FACTOR / 100.0f)))
		crc = (uint8_t)rand();
}
#else
inline bool EMULATE_PACKET_LOSS(int data) { return false; }
inline void EMULATE_PACKET_CORRUPTION(int data, uint8_t& crc) {}
#endif


void Chunk::UpdateChecksum(CRC& crc) const {

	crc << chunkNumber;
	crc << (unsigned int)chunkSize;
	if (data.size() > 0) {
		crc.Update(&data[0], data.size());
	}
}

unsigned Packet::GetSize() const {

	unsigned size = headerSize + naks.size();
	std::list<ChunkPtr>::const_iterator chk;
	for (chk = chunks.begin(); chk != chunks.end(); ++chk) {
		size += (*chk)->GetSize();
	}
	return size;
}

uint8_t Packet::GetChecksum() const {

	CRC crc;
	crc << lastContinuous;
	crc << (unsigned int)nakType;
	if (naks.size() > 0) {
		crc.Update(&naks[0], naks.size());
	}
	std::list<ChunkPtr>::const_iterator chk;
	for (chk = chunks.begin(); chk != chunks.end(); ++chk) {
		(*chk)->UpdateChecksum(crc);
	}
	return (uint8_t)crc.GetDigest();
}

class Unpacker
{
public:
	Unpacker(const unsigned char* data, unsigned length)
		: data(data)
		, length(length)
		, pos(0)
	{
	}

	template<typename T>
	void Unpack(T& t) {
		assert(length >= pos + sizeof(t));
		t = *reinterpret_cast<const T*>(data + pos);
		pos += sizeof(t);
	}

	void Unpack(std::vector<uint8_t>& t, unsigned unpackLength) {
		std::copy(data + pos, data + pos + unpackLength, std::back_inserter(t));
		pos += unpackLength;
	}

	unsigned Remaining() const {
		return length - std::min(pos, length);
	}
private:
	const unsigned char* data;
	unsigned length;
	unsigned pos;
};

class Packer
{
public:
	Packer(std::vector<uint8_t>& data)
		: data(data)
	{
		assert(data.empty());
	}

	template<typename T>
	void Pack(T& t) {
		const size_t pos = data.size();
		data.resize(pos + sizeof(T));
		*reinterpret_cast<T*>(&data[pos]) = t;
	}

	void Pack(std::vector<uint8_t>& _data) {
		std::copy(_data.begin(), _data.end(), std::back_inserter(data));
	}

private:
	std::vector<uint8_t>& data;
};

Packet::Packet(const unsigned char* data, unsigned length)
{
	Unpacker buf(data, length);
	buf.Unpack(lastContinuous);
	buf.Unpack(nakType);
	buf.Unpack(checksum);

	if (nakType > 0) {
		naks.reserve(nakType);
		for (int i = 0; i != nakType; ++i) {
			if (buf.Remaining() >= sizeof(naks[i])) {
				if (naks.size() <= i) {
					naks.push_back(0);
				}
				buf.Unpack(naks[i]);
			} else {
				break;
			}
		}
	}

	while (buf.Remaining() > Chunk::headerSize) {
		ChunkPtr temp(new Chunk);
		buf.Unpack(temp->chunkNumber);
		buf.Unpack(temp->chunkSize);
		if (buf.Remaining() >= temp->chunkSize) {
			buf.Unpack(temp->data, temp->chunkSize);
			chunks.push_back(temp);
		} else {
			// defective, ignore
			break;
		}
	}
}

Packet::Packet(int _lastContinuous, int _nak)
	: lastContinuous(_lastContinuous)
	, nakType(_nak)
{
}

void Packet::Serialize(std::vector<uint8_t>& data)
{
	data.reserve(GetSize());
	Packer buf(data);
	buf.Pack(lastContinuous);
	buf.Pack(nakType);
	buf.Pack(checksum);
	buf.Pack(naks);
	std::list<ChunkPtr>::const_iterator ci;
	for (ci = chunks.begin(); ci != chunks.end(); ++ci) {
		buf.Pack((*ci)->chunkNumber);
		buf.Pack((*ci)->chunkSize);
		buf.Pack((*ci)->data);
	}
}

UDPConnection::UDPConnection(boost::shared_ptr<ip::udp::socket> netSocket, const ip::udp::endpoint& myAddr)
	: addr(myAddr)
	, sharedSocket(true)
	, mySocket(netSocket)
{
	Init();
}

UDPConnection::UDPConnection(int sourcePort, const std::string& address, const unsigned port)
	: sharedSocket(false)
{
	addr = ResolveAddr(address, port);

	ip::address sourceAddr;
	if (addr.address().is_v6()) {
		sourceAddr = ip::address_v6::any();
	} else {
		sourceAddr = ip::address_v4::any();
	}
	boost::shared_ptr<ip::udp::socket> tempSocket(new ip::udp::socket(
			netcode::netservice, ip::udp::endpoint(sourceAddr, sourcePort)));
	mySocket = tempSocket;

	Init();
}

UDPConnection::UDPConnection(CConnection& conn)
	: sharedSocket(true)
{
	ReconnectTo(conn);
	Init();
}

void UDPConnection::ReconnectTo(CConnection& conn) {
	dynamic_cast<UDPConnection &>(conn).CopyConnection(*this);
}

void UDPConnection::CopyConnection(UDPConnection &conn) {
	conn.InitConnection(addr, mySocket);
}

void UDPConnection::InitConnection(ip::udp::endpoint address, boost::shared_ptr<ip::udp::socket> socket) {
	addr = address;
	mySocket = socket;
}

UDPConnection::~UDPConnection()
{
	delete fragmentBuffer;
	fragmentBuffer = NULL;
	Flush(true);
}

void UDPConnection::SendData(boost::shared_ptr<const RawPacket> data)
{
	assert(data->length > 0);
	outgoingData.push_back(data);
}

bool UDPConnection::HasIncomingData() const
{
	return !msgQueue.empty();
}

boost::shared_ptr<const RawPacket> UDPConnection::Peek(unsigned ahead) const
{
	if (ahead < msgQueue.size()) {
		return msgQueue[ahead];
	} else {
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

void UDPConnection::DeleteBufferPacketAt(unsigned index)
{
	if (index < msgQueue.size()) {
		msgQueue.erase(msgQueue.begin() + index);
	}
}

boost::shared_ptr<const RawPacket> UDPConnection::GetData()
{
	if (!msgQueue.empty()) {
		boost::shared_ptr<const RawPacket> msg = msgQueue.front();
		msgQueue.pop_front();
		return msg;
	} else {
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

void UDPConnection::Update()
{
	spring_time curTime = spring_gettime();
	outgoing.UpdateTime(curTime);

	if (!sharedSocket) {
		// duplicated code with UDPListener
		netservice.poll();
		size_t bytes_avail = 0;
		while ((bytes_avail = mySocket->available()) > 0) {
			std::vector<uint8_t> buffer(bytes_avail);
			ip::udp::endpoint sender_endpoint;
			size_t bytesReceived;
			ip::udp::socket::message_flags flags = 0;
			boost::system::error_code err;
			bytesReceived = mySocket->receive_from(boost::asio::buffer(buffer), sender_endpoint, flags, err);

			if (EMULATE_PACKET_LOSS(dataRecv))
				continue;

			if (CheckErrorCode(err)) {
				break;
			}

			if (bytesReceived < Packet::headerSize) {
				continue;
			}
			Packet data(&buffer[0], bytesReceived);
			if (IsUsingAddress(sender_endpoint)) {
				ProcessRawPacket(data);
			}
			// not likely, but make sure we do not get stuck here
			if ((spring_gettime() - curTime) > 10) {
				break;
			}
		}
	}

	Flush(false);
}

void UDPConnection::ProcessRawPacket(Packet& incoming)
{
	lastReceiveTime = spring_gettime();
	dataRecv += incoming.GetSize();
	recvOverhead += Packet::headerSize;
	++recvPackets;

	if (incoming.GetChecksum() != incoming.checksum) {
		LOG_L(L_ERROR,
			"Discarding incoming corrupted packet: CRC %d, LEN %d",
			incoming.checksum, incoming.GetSize());
		return;
	}

	if (incoming.lastContinuous < 0 && lastInOrder >= 0) {
		LOG_L(L_WARNING, "Discarding superfluous reconnection attempt");
		return;
	}

	AckChunks(incoming.lastContinuous);

	if (!unackedChunks.empty()) {
		int nextCont = incoming.lastContinuous + 1;
		int unAckDiff = unackedChunks[0]->chunkNumber - nextCont;
		if (unAckDiff >= 0) {
			if (incoming.nakType < 0) {
				for (int i = 0; i != -incoming.nakType; ++i) {
					int unAckPos = i + unAckDiff;
					if (unAckPos < unackedChunks.size()) {
						assert(unackedChunks[unAckPos]->chunkNumber == nextCont + i);
						RequestResend(unackedChunks[unAckPos]);
					}
				}
			} else if (incoming.nakType > 0) {
				for (int i = 0; i != incoming.naks.size(); ++i) {
					int unAckPos = incoming.naks[i] + unAckDiff;
					if (unAckPos < unackedChunks.size()) {
						assert(unackedChunks[unAckPos]->chunkNumber == nextCont + incoming.naks[i]);
						RequestResend(unackedChunks[unAckPos]);
					}
				}
			}
		}
	}
	std::list<ChunkPtr>::const_iterator ci;
	for (ci = incoming.chunks.begin(); ci != incoming.chunks.end(); ++ci) {
		if ((lastInOrder >= (*ci)->chunkNumber)
				|| (waitingPackets.find((*ci)->chunkNumber) != waitingPackets.end()))
		{
			++droppedChunks;
			continue;
		}
		waitingPackets.insert((*ci)->chunkNumber, new RawPacket(&(*ci)->data[0], (*ci)->data.size()));
	}

	packetMap::iterator wpi;
	//process all in order packets that we have waiting
	while ((wpi = waitingPackets.find(lastInOrder+1)) != waitingPackets.end()) {
		std::vector<boost::uint8_t> buf;
		if (fragmentBuffer) {
			buf.resize(fragmentBuffer->length);
			// combine with fragment buffer
			std::copy(fragmentBuffer->data, fragmentBuffer->data+fragmentBuffer->length, buf.begin());
			delete fragmentBuffer;
			fragmentBuffer = NULL;
		}

		lastInOrder++;
		std::copy(wpi->second->data, wpi->second->data+wpi->second->length, std::back_inserter(buf));
		waitingPackets.erase(wpi);

		for (unsigned pos = 0; pos < buf.size(); ) {
			unsigned char* bufp = &buf[pos];
			unsigned msglength = buf.size() - pos;

			int pktlength = ProtocolDef::GetInstance()->PacketLength(bufp, msglength);
			if (ProtocolDef::GetInstance()->IsValidLength(pktlength, msglength)) { // this returns false for zero/invalid pktlength
				msgQueue.push_back(boost::shared_ptr<const RawPacket>(new RawPacket(bufp, pktlength)));
				pos += pktlength;
			} else {
				if (pktlength >= 0) {
					// partial packet in buffer
					fragmentBuffer = new RawPacket(bufp, msglength);
					break;
				}
				LOG_L(L_ERROR,
						"Discarding incoming invalid packet: ID %d, LEN %d",
						(int)*bufp, pktlength);
				// if the packet is invalid, skip a single byte
				// until we encounter a good packet
				++pos;
			}
		}
	}
}

void UDPConnection::Flush(const bool forced)
{
	if (muted)
		return;
	const spring_time curTime = spring_gettime();

	// do not create chunks more than chunksPerSec times per second
	const bool waitMore = lastChunkCreated >= curTime - spring_msecs(1000 / chunksPerSec);
	// if the packet is tiny, reduce the send frequency further
	const int requiredLength = (spring_msecs(200) - (int)(curTime - lastChunkCreated)) / 10;

	int outgoingLength = 0;
	if (!waitMore) {
		packetList::const_iterator pi;
		for (pi = outgoingData.begin(); (pi != outgoingData.end()) && (outgoingLength <= requiredLength); ++pi) {
			outgoingLength += (*pi)->length;
		}
	}

	if (forced || (!waitMore && outgoingLength > requiredLength)) {
		boost::uint8_t buffer[udpMaxPacketSize];
		unsigned pos = 0;
		// Manually fragment packets to respect configured UDP_MTU.
		// This is an attempt to fix the bug where players drop out of the game if
		// someone in the game gives a large order.
		bool partialPacket = false;
		bool sendMore = true;

		do {
			sendMore = (outgoing.GetAverage(true) <= globalConfig->linkOutgoingBandwidth)
					|| (globalConfig->linkOutgoingBandwidth <= 0)
					|| partialPacket
					|| forced;
			if (!outgoingData.empty() && sendMore) {
				boost::shared_ptr<const RawPacket>& packet = *(outgoingData.begin());
				if (!partialPacket && !ProtocolDef::GetInstance()->IsValidPacket(packet->data, packet->length)) {
					LOG_L(L_ERROR,
							"Discarding outgoing invalid packet: ID %d, LEN %d",
							((packet->length > 0) ? (int)packet->data[0] : -1),
							packet->length);
					outgoingData.pop_front();
				} else {
					unsigned numBytes = std::min((unsigned)maxChunkSize - pos, packet->length);
					assert(packet->length > 0);
					memcpy(buffer+pos, packet->data, numBytes);
					pos+= numBytes;
					outgoing.DataSent(numBytes, true);
					partialPacket = (numBytes != packet->length);
					if (partialPacket) {
						// partially transfered
						packet.reset(new RawPacket(packet->data + numBytes, packet->length - numBytes));
					} else { // full packet copied
						outgoingData.pop_front();
					}
				}
			}
			if ((pos > 0) && (outgoingData.empty() || (pos == maxChunkSize) || !sendMore)) {
				CreateChunk(buffer, pos, currentNum++);
				pos = 0;
			}
		} while (!outgoingData.empty() && sendMore);
	}
	SendIfNecessary(forced);
}

bool UDPConnection::CheckTimeout(int seconds, bool initial) const {

	spring_duration timeout;
	if (seconds == 0) {
		timeout = spring_secs((dataRecv && !initial)
				? globalConfig->networkTimeout
				: globalConfig->initialNetworkTimeout);
	} else if (seconds > 0) {
		timeout = spring_secs(seconds);
	} else {
		timeout = spring_secs(globalConfig->reconnectTimeout);
	}

	return (timeout > 0 && (lastReceiveTime + timeout) < spring_gettime());
}

bool UDPConnection::NeedsReconnect() {

	if (CanReconnect()) {
		if (!CheckTimeout(-1)) {
			reconnectTime = globalConfig->reconnectTimeout;
		} else if (CheckTimeout(reconnectTime)) {
			++reconnectTime;
			return true;
		}
	}

	return false;
}

bool UDPConnection::CanReconnect() const {
	return (globalConfig->reconnectTimeout > 0);
}

int UDPConnection::GetReconnectSecs() const {
	return reconnectTime;
}

std::string UDPConnection::Statistics() const
{
	std::string msg = "Statistics for UDP connection:\n";
	msg += str( boost::format("Received: %1% bytes in %2% packets (%3% bytes/package)\n")
			%dataRecv %recvPackets %((float)dataRecv / (float)recvPackets));
	msg += str( boost::format("Sent: %1% bytes in %2% packets (%3% bytes/package)\n")
			%dataSent %sentPackets %((float)dataSent / (float)sentPackets));
	msg += str( boost::format("Relative protocol overhead: %1% up, %2% down\n")
			%((float)sentOverhead / (float)dataSent) %((float)recvOverhead / (float)dataRecv) );
	msg += str( boost::format("%1% incoming chunks had been dropped, %2% outgoing chunks had to be resent\n")
			%droppedChunks %resentChunks);
	return msg;
}

bool UDPConnection::IsUsingAddress(const ip::udp::endpoint& from) const
{
	return (addr == from);
}

std::string UDPConnection::GetFullAddress() const
{
	return str( boost::format("[%s]:%u") %addr.address().to_string() %addr.port() );
}

void UDPConnection::SetMTU(unsigned mtu2)
{
	if ((mtu2 > 300) && (mtu2 < udpMaxPacketSize)) {
		mtu = mtu2;
	}
}

void UDPConnection::Init()
{
	lastNakTime = spring_notime;
	lastSendTime = spring_notime;
	lastUnackResent = spring_notime;
	lastReceiveTime = spring_gettime();
	lastInOrder = -1;
	waitingPackets.clear();
	currentNum = 0;
	lastNak = -1;
	sentOverhead = 0;
	recvOverhead = 0;
	fragmentBuffer = 0;
	resentChunks = 0;
	sentPackets = recvPackets = 0;
	droppedChunks = 0;
	mtu = globalConfig->mtu;
	reconnectTime = globalConfig->reconnectTimeout;
	lastChunkCreated = spring_gettime();
	muted = true;
}

void UDPConnection::CreateChunk(const unsigned char* data, const unsigned length, const int packetNum)
{
	assert((length > 0) && (length < 255));
	ChunkPtr buf(new Chunk);
	buf->chunkNumber = packetNum;
	buf->chunkSize = length;
	std::copy(data, data+length, std::back_inserter(buf->data));
	newChunks.push_back(buf);
	lastChunkCreated = spring_gettime();
}

void UDPConnection::SendIfNecessary(bool flushed)
{
	const spring_time curTime = spring_gettime();

	int nak = 0;
	std::vector<int> dropped;

	{
		int packetNum = lastInOrder+1;
		for (packetMap::iterator pi = waitingPackets.begin(); pi != waitingPackets.end(); ++pi)
		{
			const int diff = pi->first - packetNum;
			if (diff > 0) {
				for (int i = 0; i < diff; ++i) {
					dropped.push_back(packetNum);
					packetNum++;
				}
			}
			packetNum++;
		}
		unsigned numContinuous = 0;
		for (unsigned i = 0; i != dropped.size(); ++i) {
			if (dropped[i] == (lastInOrder + i + 1)) {
				numContinuous++;
			} else {
				break;
			}
		}

		if ((numContinuous < 8) && !((lastNakTime + spring_msecs(200)) < spring_gettime())) {
			nak = std::min(dropped.size(), (size_t)127);
			// needs 1 byte per requested packet, so do not spam to often
			lastNakTime = curTime;
		} else {
			nak = -(int)std::min((unsigned)127, numContinuous);
		}
	}

	lastUnackResent = std::max(lastUnackResent, lastChunkCreated);
	if (!unackedChunks.empty() && ((lastUnackResent + spring_msecs(400)) < curTime))
	{
		if (newChunks.empty()) {
			// resent last packet if we didn't get an ack after 0,4 seconds
			// and don't plan sending out a new chunk either
			RequestResend(*(unackedChunks.end()-1));
		}
		lastUnackResent = curTime;
	}

	if (flushed || !newChunks.empty() || !resendRequested.empty() || (nak > 0) || ((lastSendTime + spring_msecs(200)) < curTime))
	{
		bool todo = true;
		while (todo && ((outgoing.GetAverage() <= globalConfig->linkOutgoingBandwidth) || (globalConfig->linkOutgoingBandwidth <= 0)))
		{
			Packet buf(lastInOrder, nak);
			if (nak > 0) {
				buf.naks.resize(nak);
				for (unsigned i = 0; i != buf.naks.size(); ++i) {
					buf.naks[i] = dropped[i] - (lastInOrder + 1); // zero means request resend of lastInOrder + 1
				}
				nak = 0; // 1 request is enought
			}

			while (true) {
				if (!resendRequested.empty() && ((buf.GetSize() + resendRequested[0]->GetSize()) <= mtu)) {
					buf.chunks.push_back(resendRequested[0]);
					resendRequested.pop_front();
					++resentChunks;
				} else if (!newChunks.empty() && ((buf.GetSize() + newChunks[0]->GetSize()) <= mtu)) {
					buf.chunks.push_back(newChunks[0]);
					unackedChunks.push_back(newChunks[0]);
					newChunks.pop_front();
				} else {
					break;
				}
			}

			buf.checksum = buf.GetChecksum();
			EMULATE_PACKET_CORRUPTION(dataSent, buf.checksum);

			SendPacket(buf);
			if (resendRequested.empty() && newChunks.empty()) {
				todo = false;
			}
		}
	}
}

void UDPConnection::SendPacket(Packet& pkt)
{
	std::vector<uint8_t> data;
	pkt.Serialize(data);

	outgoing.DataSent(data.size());
	lastSendTime = spring_gettime();
	ip::udp::socket::message_flags flags = 0;
	boost::system::error_code err;

	if (!EMULATE_PACKET_LOSS(dataSent))
		mySocket->send_to(buffer(data), addr, flags, err);
	if (CheckErrorCode(err)) {
		return;
	}

	dataSent += data.size();
	++sentPackets;
}

void UDPConnection::AckChunks(int lastAck)
{
	while (!unackedChunks.empty() && (lastAck >= (*unackedChunks.begin())->chunkNumber))
	{
		unackedChunks.pop_front();
	}

	bool done;
	do {
		// resend requested and later acked, happens every now and then
		done = true;
		std::deque<ChunkPtr>::iterator ci;
		for (ci = resendRequested.begin(); ci != resendRequested.end(); ++ci)
		{
			if ((*ci)->chunkNumber <= lastAck) {
				resendRequested.erase(ci);
				done = false;
				break;
			}
		}
	} while (!done);
}

void UDPConnection::RequestResend(ChunkPtr ptr)
{
	std::deque<ChunkPtr>::const_iterator ci;
	for (ci = resendRequested.begin(); ci != resendRequested.end(); ++ci) {
		// filter out duplicates
		if (*ci == ptr) {
			return;
		}
	}
	resendRequested.push_back(ptr);
}

UDPConnection::BandwidthUsage::BandwidthUsage()
	: lastTime(0)
	, trafficSinceLastTime(1)
	, prelTrafficSinceLastTime(0)
	, average(0.0)
{
}

void UDPConnection::BandwidthUsage::UpdateTime(unsigned newTime)
{
	if (newTime > (lastTime + 100)) {
		average = (average*9 + float(trafficSinceLastTime) / float(newTime-lastTime) * 1000.0f) / 10.0f;
		trafficSinceLastTime = 0;
		prelTrafficSinceLastTime = 0;
		lastTime = newTime;
	}
}

void UDPConnection::BandwidthUsage::DataSent(unsigned amount, bool prel)
{
	if (prel) {
		prelTrafficSinceLastTime += amount;
	} else {
		trafficSinceLastTime += amount;
	}
}

float UDPConnection::BandwidthUsage::GetAverage(bool prel) const
{
	// not exactly accurate, but does job
	return average + (prel ? std::max(trafficSinceLastTime, prelTrafficSinceLastTime) : trafficSinceLastTime);
}

} // namespace netcode

