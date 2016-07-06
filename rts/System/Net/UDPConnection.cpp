/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UDPConnection.h"

#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>


#include "Socket.h"
#include "ProtocolDef.h"
#include "Exception.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Config/ConfigHandler.h"
#include "System/CRC.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#ifndef UNIT_TEST
CONFIG(bool, UDPConnectionLogDebugMessages).defaultValue(false);
#endif


namespace netcode {
using namespace boost::asio;

static const unsigned udpMaxPacketSize = 4096;
static const int maxChunkSize = 254;
static const int chunksPerSec = 30;



#if NETWORK_TEST
float RANDOM_NUMBER() {
	// spring has some srand calls that interfere with the random seed
	static int lastRand = 0;

	srand(lastRand);

	// [0.0f, 1.0f)
	return (lastRand = rand()) / float(RAND_MAX);
}

bool EMULATE_PACKET_LOSS(int& lossCtr) {
	if (RANDOM_NUMBER() < (PACKET_LOSS_FACTOR / 100.0f))
		return true;

	const bool loss = RANDOM_NUMBER() < (SEVERE_PACKET_LOSS_FACTOR / 100.0f);

	if (loss && lossCtr == 0)
		lossCtr = SEVERE_PACKET_LOSS_MAX_COUNT * RANDOM_NUMBER();

	return (lossCtr > 0 && lossCtr--);
}

void EMULATE_PACKET_CORRUPTION(uint8_t& crc) {
	if ((RANDOM_NUMBER() < (PACKET_CORRUPTION_FACTOR / 100.0f)))
		crc = (uint8_t)rand();
}

#define LOSS_COUNTER lossCounter

#else
static int dummyLossCounter = 0;
inline bool EMULATE_PACKET_LOSS(int &lossCtr) { return false; }
inline void EMULATE_PACKET_CORRUPTION(boost::uint8_t& crc) {}
#define LOSS_COUNTER dummyLossCounter
#endif


#if NETWORK_TEST && PACKET_MAX_LATENCY > 0 && PACKET_MAX_LATENCY >= PACKET_MIN_LATENCY
#define EMULATE_LATENCY(cond) \
	for (auto di = delayed.begin(); di != delayed.end(); ) { \
		spring_time curtime = spring_gettime(); \
		if (curtime > di->first && (curtime - di->first) > spring_msecs(0)) { \
			mySocket->send_to(buffer(di->second), addr, flags, err); \
			di = delayed.erase(di); \
		} else { ++di; } \
	} \
	if (cond) \
		delayed[spring_gettime() + spring_msecs(PACKET_MIN_LATENCY + (PACKET_MAX_LATENCY - PACKET_MIN_LATENCY) * RANDOM_NUMBER())] = data; \
	if (false)
#else
#define EMULATE_LATENCY(cond) if(cond)
#endif


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

	void Unpack(std::vector<boost::uint8_t>& t, unsigned unpackLength) {
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
	Packer(std::vector<boost::uint8_t>& data): data(data)
	{
		assert(data.empty());
	}

	template<typename T>
	void Pack(T& t) {
		const size_t pos = data.size();
		data.resize(pos + sizeof(T));
		*reinterpret_cast<T*>(&data[pos]) = t;
	}

	void Pack(std::vector<boost::uint8_t>& _data) {
		std::copy(_data.begin(), _data.end(), std::back_inserter(data));
	}

private:
	std::vector<boost::uint8_t>& data;
};



void Chunk::UpdateChecksum(CRC& crc) const {

	crc << chunkNumber;
	crc << (unsigned int)chunkSize;

	if (!data.empty()) {
		crc.Update(&data[0], data.size());
	}
}



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

unsigned Packet::GetSize() const {

	unsigned size = headerSize + naks.size();

	for (auto chk = chunks.begin(); chk != chunks.end(); ++chk)
		size += (*chk)->GetSize();

	return size;
}

boost::uint8_t Packet::GetChecksum() const {

	CRC crc;
	crc << lastContinuous;
	crc << (unsigned int)nakType;

	if (!naks.empty())
		crc.Update(&naks[0], naks.size());

	for (auto chk = chunks.begin(); chk != chunks.end(); ++chk)
		(*chk)->UpdateChecksum(crc);

	return (boost::uint8_t)crc.GetDigest();
}

void Packet::Serialize(std::vector<boost::uint8_t>& data)
{
	data.reserve(GetSize());
	Packer buf(data);
	buf.Pack(lastContinuous);
	buf.Pack(nakType);
	buf.Pack(checksum);
	buf.Pack(naks);

	for (auto ci = chunks.begin(); ci != chunks.end(); ++ci) {
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
	boost::system::error_code err;
	addr = ResolveAddr(address, port, &err);

	ip::address sourceAddr = GetAnyAddress(addr.address().is_v6());
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

void UDPConnection::Init()
{
	// make sure protocoldef is initialized
	CBaseNetProtocol::Get();

	lastNakTime = spring_gettime();
	lastUnackResentTime = spring_gettime();
	lastPacketSendTime = spring_gettime();
	lastPacketRecvTime = spring_gettime();
	lastChunkCreatedTime = spring_gettime();

	#ifdef ENABLE_DEBUG_STATS
	lastDebugMessageTime = spring_gettime();
	lastFramePacketRecvTime = spring_gettime();
	#endif

	lastInOrder = -1;
	waitingPackets.clear();

	#ifdef ENABLE_DEBUG_STATS
	sumDeltaFramePacketRecvTime = 0.0f;
	minDeltaFramePacketRecvTime = 0.0f;
	maxDeltaFramePacketRecvTime = 0.0f;

	numReceivedFramePackets = 0;
	numEnqueuedFramePackets = 0;
	numEmptyGetDataCalls = 0;
	numTotalGetDataCalls = 0;
	#endif
	currentPacketChunkNum = 0;

	lastNak = -1;
	sentOverhead = 0;
	recvOverhead = 0;
	fragmentBuffer = 0;
	resentChunks = 0;
	sentPackets = recvPackets = 0;
	droppedChunks = 0;
	mtu = globalConfig->mtu;
	reconnectTime = globalConfig->reconnectTimeout;

	muted = true;
	closed = false;
	resend = false;

	#ifndef UNIT_TEST
	logMessages = configHandler->GetBool("UDPConnectionLogDebugMessages");
	#endif

	netLossFactor = globalConfig->networkLossFactor;
	lastMidChunk = -1;
#if	NETWORK_TEST
	lossCounter = 0;
#endif
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

boost::shared_ptr<const RawPacket> UDPConnection::Peek(unsigned ahead) const
{
	if (ahead < msgQueue.size())
		return msgQueue[ahead];

	boost::shared_ptr<const RawPacket> empty;
	return empty;
}

#ifdef ENABLE_DEBUG_STATS
boost::shared_ptr<const RawPacket> UDPConnection::GetData()
{
	numTotalGetDataCalls++;

	if (!msgQueue.empty()) {
		boost::shared_ptr<const RawPacket> msg = msgQueue.front();
		msgQueue.pop_front();

		numEnqueuedFramePackets -= (msg->data[0] == NETMSG_NEWFRAME);
		numEnqueuedFramePackets -= (msg->data[0] == NETMSG_KEYFRAME);

		return msg;
	}

	numEmptyGetDataCalls++;

	boost::shared_ptr<const RawPacket> empty;
	return empty;
}
#else
boost::shared_ptr<const RawPacket> UDPConnection::GetData()
{
	if (!msgQueue.empty()) {
		boost::shared_ptr<const RawPacket> msg = msgQueue.front();
		msgQueue.pop_front();
		return msg;
	}

	boost::shared_ptr<const RawPacket> empty;
	return empty;
}
#endif


void UDPConnection::DeleteBufferPacketAt(unsigned index)
{
	if (index < msgQueue.size()) {
		msgQueue.erase(msgQueue.begin() + index);
	}
}

void UDPConnection::Update()
{
	spring_time curTime = spring_gettime();
	outgoing.UpdateTime(spring_tomsecs(curTime));

	#ifdef ENABLE_DEBUG_STATS
	{
		const float debugMssgDeltaTime = (curTime - lastDebugMessageTime).toMilliSecsf();
		const float avgFramePacketRate = numReceivedFramePackets / debugMssgDeltaTime;

		if (debugMssgDeltaTime >= 1000.0f) {
			if (logMessages) {
				LOG_L(L_INFO,
					"[UDPConnection::%s] %u NETMSG_*FRAME packets received (%fms : %fp/ms) during (empty=%u total=%u) GetData calls",
					__FUNCTION__, numReceivedFramePackets, debugMssgDeltaTime, avgFramePacketRate, numEmptyGetDataCalls, numTotalGetDataCalls
				);
			}

			lastDebugMessageTime = curTime;

			sumDeltaFramePacketRecvTime = 0.0f;
			minDeltaFramePacketRecvTime = 1e6f;
			maxDeltaFramePacketRecvTime = 0.0f;

			numReceivedFramePackets = 0;
			// numEnqueuedFramePackets = 0;

			numEmptyGetDataCalls = 0;
			numTotalGetDataCalls = 0;
		}
	}
	#endif

	if (!sharedSocket && !closed) {
		// duplicated code with UDPListener
		netservice.poll();
		size_t bytesAvail = 0;

		while ((bytesAvail = mySocket->available()) > 0) {
			std::vector<boost::uint8_t> buffer(bytesAvail, 0);
			ip::udp::endpoint sender_endpoint;
			ip::udp::socket::message_flags flags = 0;
			boost::system::error_code err;

			const size_t bytesReceived = mySocket->receive_from(boost::asio::buffer(buffer), sender_endpoint, flags, err);

			if (CheckErrorCode(err))
				break;

			if (bytesReceived < Packet::headerSize)
				continue;

			Packet data(&buffer[0], bytesReceived);

			if (IsUsingAddress(sender_endpoint))
				ProcessRawPacket(data);

			// not likely, but make sure we do not get stuck here
			if ((spring_gettime() - curTime) > spring_msecs(10)) {
				break;
			}
		}
	}

	Flush(false);
}

void UDPConnection::ProcessRawPacket(Packet& incoming)
{
	#ifdef ENABLE_DEBUG_STATS
	if (logMessages) {
		LOG_L(L_INFO, "\t[%s] checksum=(%u : %u) mtu=%u", __FUNCTION__, incoming.GetChecksum(), incoming.checksum, mtu);
	}
	#endif

	lastPacketRecvTime = spring_gettime();
	dataRecv += incoming.GetSize();
	recvOverhead += Packet::headerSize;
	++recvPackets;

//	if (EMULATE_PACKET_LOSS(lossCounter))
//		return;

	if (incoming.GetChecksum() != incoming.checksum) {
		LOG_L(L_ERROR, "Discarding incoming corrupted packet: CRC %d, LEN %d", incoming.checksum, incoming.GetSize());
		return;
	}

	if (incoming.lastContinuous < 0 && lastInOrder >= 0 &&
		(unackedChunks.empty() || unackedChunks[0]->chunkNumber > 0)) {
		LOG_L(L_WARNING, "Discarding superfluous reconnection attempt");
		return;
	}

	AckChunks(incoming.lastContinuous);

	if (!unackedChunks.empty()) {
		const int nextCont = incoming.lastContinuous + 1;
		const int unAckDiff = unackedChunks[0]->chunkNumber - nextCont;

		if (-256 <= unAckDiff && unAckDiff <= 256) {
			if (incoming.nakType < 0) {
				for (int i = 0; i != -incoming.nakType; ++i) {
					const int unAckPos = i + unAckDiff;

					if (unAckPos >= 0 && unAckPos < unackedChunks.size()) {
						assert(unackedChunks[unAckPos]->chunkNumber == nextCont + i);
						RequestResend(unackedChunks[unAckPos]);
					}
				}
			} else if (incoming.nakType > 0) {
				int unAckPos = 0;

				for (int i = 0; i != incoming.naks.size(); ++i) {
					if (unAckDiff + incoming.naks[i] < 0)
						continue;

					while (unAckPos < unAckDiff + incoming.naks[i]) {
						// if there are gaps in the array, assume that further resends are not needed
						if (unAckPos < unackedChunks.size())
							resendRequested.erase(unackedChunks[unAckPos]->chunkNumber);

						++unAckPos;
					}

					if (unAckPos < unackedChunks.size()) {
						assert(unackedChunks[unAckPos]->chunkNumber == nextCont + incoming.naks[i]);
						RequestResend(unackedChunks[unAckPos]);
					}

					++unAckPos;
				}
			}
		}
	}

	for (auto ci = incoming.chunks.begin(); ci != incoming.chunks.end(); ++ci) {
		const boost::shared_ptr<netcode::Chunk>& c = *ci;

		if ((lastInOrder >= c->chunkNumber) || (waitingPackets.find(c->chunkNumber) != waitingPackets.end())) {
			++droppedChunks;
			continue;
		}

		waitingPackets.insert(c->chunkNumber, new RawPacket(&c->data[0], c->data.size()));
	}

	packetMap::iterator wpi;

	// process all in order packets that we have waiting
	while ((wpi = waitingPackets.find(lastInOrder + 1)) != waitingPackets.end()) {
		std::vector<boost::uint8_t> buf;

		if (fragmentBuffer != NULL) {
			buf.resize(fragmentBuffer->length);
			// combine with fragment buffer (packet reassembly)
			std::copy(fragmentBuffer->data, fragmentBuffer->data + fragmentBuffer->length, buf.begin());
			delete fragmentBuffer;
			fragmentBuffer = NULL;
		}

		lastInOrder++;
		std::copy(wpi->second->data, wpi->second->data + wpi->second->length, std::back_inserter(buf));
		waitingPackets.erase(wpi);

		for (unsigned pos = 0; pos < buf.size(); ) {
			const unsigned char* bufp = &buf[pos];
			const unsigned msglength = buf.size() - pos;

			const int pktlength = ProtocolDef::GetInstance()->PacketLength(bufp, msglength);

			// this returns false for zero/invalid pktlength
			if (ProtocolDef::GetInstance()->IsValidLength(pktlength, msglength)) {
				msgQueue.push_back(boost::shared_ptr<const RawPacket>(new RawPacket(bufp, pktlength)));

				#ifdef ENABLE_DEBUG_STATS
				// server sends both of these, clients send only keyframe messages
				// TODO: would be easy to feed this data into a Q3A-style lagometer
				//
				if ((msgQueue.back())->data[0] == NETMSG_NEWFRAME || (msgQueue.back())->data[0] == NETMSG_KEYFRAME) {
					const spring_time dt = spring_gettime() - lastFramePacketRecvTime;

					sumDeltaFramePacketRecvTime += dt.toMilliSecsf();
					minDeltaFramePacketRecvTime = std::min(dt.toMilliSecsf(), minDeltaFramePacketRecvTime);
					maxDeltaFramePacketRecvTime = std::max(dt.toMilliSecsf(), maxDeltaFramePacketRecvTime);

					numReceivedFramePackets += 1;
					numEnqueuedFramePackets += 1;
					lastFramePacketRecvTime = spring_gettime();

					if (logMessages) {
						LOG_L(L_INFO,
							"\t[%s] (received=%u enqueued=%u) packets (dt=%fms mindt=%fms maxdt=%fms sumdt=%fms)",
							__FUNCTION__, numReceivedFramePackets, numEnqueuedFramePackets, dt.toMilliSecsf(),
							minDeltaFramePacketRecvTime, maxDeltaFramePacketRecvTime, sumDeltaFramePacketRecvTime
						);
					}
				}
				#endif

				pos += pktlength;
			} else {
				if (pktlength >= 0) {
					// partial packet in buffer
					fragmentBuffer = new RawPacket(bufp, msglength);
					break;
				}

				LOG_L(L_ERROR, "Discarding incoming invalid packet: ID %d, LEN %d", (int)*bufp, pktlength);

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
	const bool waitMore = (lastChunkCreatedTime >= (curTime - spring_msecs(1000 / chunksPerSec)));
	// if the packet is tiny, reduce the send frequency further
	const int requiredLength = ((200 >> netLossFactor) - spring_tomsecs(curTime - lastChunkCreatedTime)) / 10;

	int outgoingLength = 0;

	if (!waitMore) {
		for (auto pi = outgoingData.begin(); (pi != outgoingData.end()) && (outgoingLength <= requiredLength); ++pi) {
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
			sendMore  = (outgoing.GetAverage(true) <= globalConfig->linkOutgoingBandwidth);
			sendMore |= ((globalConfig->linkOutgoingBandwidth <= 0) || partialPacket || forced);

			if (!outgoingData.empty() && sendMore) {
				boost::shared_ptr<const RawPacket>& packet = *(outgoingData.begin());

				if (!partialPacket && !ProtocolDef::GetInstance()->IsValidPacket(packet->data, packet->length)) {
					LOG_L(L_ERROR,
						"Discarding outgoing invalid packet: ID %d, LEN %d",
						((packet->length > 0) ? (int)packet->data[0] : -1),
						packet->length);
					outgoingData.pop_front();
				} else {
					const unsigned numBytes = std::min((unsigned)maxChunkSize - pos, packet->length);

					assert(packet->length > 0);
					memcpy(buffer + pos, packet->data, numBytes);
					pos += numBytes;
					outgoing.DataSent(numBytes, true);
					partialPacket = (numBytes != packet->length);

					if (partialPacket) {
						// partially transfered
						packet.reset(new RawPacket(packet->data + numBytes, packet->length - numBytes));
					} else {
						// full packet copied
						outgoingData.pop_front();
					}
				}
			}
			if ((pos > 0) && (outgoingData.empty() || (pos == maxChunkSize) || !sendMore)) {
				CreateChunk(buffer, pos, currentPacketChunkNum++);
				pos = 0;
			}
		} while (!outgoingData.empty() && sendMore);
	}

	SendIfNecessary(forced);
}

bool UDPConnection::CheckTimeout(int seconds, bool initial) const {

	int timeout;

	if (seconds == 0) {
		timeout = (dataRecv && !initial)
				? globalConfig->networkTimeout
				: globalConfig->initialNetworkTimeout;
	} else if (seconds > 0) {
		timeout = seconds;
	} else {
		timeout = globalConfig->reconnectTimeout;
	}

	return (timeout > 0 && (spring_gettime() - lastPacketRecvTime) > spring_secs(timeout));
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

std::string UDPConnection::Statistics() const
{
	std::string msg = "Statistics for UDP connection:\n";
	msg += str( boost::format("Received: %1% bytes in %2% packets (%3% bytes/package)\n")
			%dataRecv %recvPackets %(SafeDivide(dataRecv, recvPackets)));
	msg += str( boost::format("Sent: %1% bytes in %2% packets (%3% bytes/package)\n")
			%dataSent %sentPackets %(SafeDivide(dataSent, sentPackets)));
	msg += str( boost::format("Relative protocol overhead: %1% up, %2% down\n")
			%SafeDivide(sentOverhead, dataSent) %SafeDivide(recvOverhead, dataRecv) );
	msg += str( boost::format("%1% incoming chunks dropped, %2% outgoing chunks resent\n")
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

void UDPConnection::CreateChunk(const unsigned char* data, const unsigned length, const int packetNum)
{
	assert((length > 0) && (length < 255));
	ChunkPtr buf(new Chunk);
	buf->chunkNumber = packetNum;
	buf->chunkSize = length;
	std::copy(data, data+length, std::back_inserter(buf->data));
	newChunks.push_back(buf);
	lastChunkCreatedTime = spring_gettime();
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
		while (!dropped.empty() && (dropped.back() - (lastInOrder + 1)) > 255)
			dropped.pop_back();
		unsigned numContinuous = 0;
		for (unsigned i = 0; i != dropped.size(); ++i) {
			if (dropped[i] == (lastInOrder + i + 1)) {
				numContinuous++;
			} else {
				break;
			}
		}

		if ((numContinuous < 8) && (curTime - lastNakTime) > spring_msecs(200 >> netLossFactor)) {
			nak = std::min(dropped.size(), (size_t)127);
			// needs 1 byte per requested packet, so do not spam to often
			lastNakTime = curTime;
		} else {
			nak = -(int)std::min((unsigned)127, numContinuous);
		}
	}

	if (!unackedChunks.empty() &&
		(curTime - lastChunkCreatedTime) > spring_msecs(400 >> netLossFactor) &&
		(curTime - lastUnackResentTime) > spring_msecs(400 >> netLossFactor)) {
		// resend last packet if we didn't get an ack within reasonable time
		// and don't plan sending out a new chunk either
		if (newChunks.empty())
			RequestResend(*unackedChunks.rbegin());
		lastUnackResentTime = curTime;
	}

	if (flushed || !newChunks.empty() || (netLossFactor == MIN_LOSS_FACTOR && !resendRequested.empty()) || (nak > 0) || (curTime - lastPacketSendTime) > spring_msecs(200 >> netLossFactor))
	{
		bool todo = true;

		int maxResend = resendRequested.size();
		int unackPrevSize = unackedChunks.size();

		std::map<boost::int32_t, ChunkPtr>::iterator resIter = resendRequested.begin();
		std::map<boost::int32_t, ChunkPtr>::iterator resMidIter, resMidIterStart, resMidIterEnd;
		std::map<boost::int32_t, ChunkPtr>::reverse_iterator resRevIter;

		if (netLossFactor != MIN_LOSS_FACTOR) {
			maxResend = std::min(maxResend, 20 * netLossFactor); // keep it reasonable, or it could cause a tremendous flood of packets

			resMidIter = resendRequested.begin();
			resMidIterStart = resendRequested.begin();
			resMidIterEnd = resendRequested.end();
			resRevIter = resendRequested.rbegin();

			const int resMidStart = (maxResend + 3) / 4;
			const int resMidEnd = (maxResend + 2) / 4;

			for (int i = 0; i < resMidStart; ++i)
				++resMidIterStart;
			if (resMidIterStart != resendRequested.end() && lastMidChunk < resMidIterStart->first)
				lastMidChunk = resMidIterStart->first - 1;

			for (int i = 0; i < resMidEnd; ++i)
				--resMidIterEnd;

			while (resMidIter != resendRequested.end() && resMidIter->first <= lastMidChunk)
				++resMidIter;

			if (resMidIter == resendRequested.end() || resMidIterEnd == resendRequested.end() ||
				resMidIter->first >= resMidIterEnd->first)
				resMidIter = resMidIterStart;
		}

		int rev = 0;

		while (todo && ((outgoing.GetAverage() <= globalConfig->linkOutgoingBandwidth) || (globalConfig->linkOutgoingBandwidth <= 0))) {
			Packet buf(lastInOrder, nak);

			if (nak > 0) {
				buf.naks.resize(nak);
				for (unsigned i = 0; i != buf.naks.size(); ++i) {
					buf.naks[i] = dropped[i] - (lastInOrder + 1); // zero means request resend of lastInOrder + 1
				}
				if (netLossFactor == MIN_LOSS_FACTOR)
					nak = 0; // 1 request is enough, unless high loss
			}

			bool sent = false;
			while (true) {
				bool canResend = maxResend > 0 &&
					((buf.GetSize() +
					(((netLossFactor == MIN_LOSS_FACTOR) || (rev == 0)) ? resIter->second->GetSize() : ((rev == 1) ? resRevIter->second->GetSize() : resMidIter->second->GetSize())) // resend chunk size
					) <= mtu);
				bool canSendNew = !newChunks.empty() && ((buf.GetSize() + newChunks[0]->GetSize()) <= mtu);

				if (!canResend && !canSendNew)
					break;

				// alternate between send and resend to make sure none is starved
				resend = !resend;

				if (resend && canResend) {
					if (netLossFactor == MIN_LOSS_FACTOR) {
						buf.chunks.push_back(resIter->second);
						resIter = resendRequested.erase(resIter);
					} else {
						// on a lossy connection, just keep resending until it is acked
						switch(rev) {
							case 0:
								buf.chunks.push_back(resIter->second);
								++resIter;
								break;
								// alternate between sending from front, middle and back of list of requested chunks,
							case 1:
								buf.chunks.push_back(resRevIter->second);
								++resRevIter;
								break;
								// since this improves performance on high latency connections
							case 2:
							case 3:
								buf.chunks.push_back(resMidIter->second);
								lastMidChunk = resMidIter->first;
								++resMidIter;
								if (resMidIter == resMidIterEnd)
									resMidIter = resMidIterStart;
								break;
						}
						rev = (rev + 1) % 4;
					}
					++resentChunks;
					--maxResend;
					sent = true;
				} else if (!resend && canSendNew) {
					buf.chunks.push_back(newChunks[0]);
					unackedChunks.push_back(newChunks[0]);
					newChunks.pop_front();
					sent = true;
				}
			}
			if (!sent || (maxResend == 0 && newChunks.empty()))
				todo = false;
			buf.checksum = buf.GetChecksum();
			EMULATE_PACKET_CORRUPTION(buf.checksum);

			SendPacket(buf);
		}

		if (netLossFactor != MIN_LOSS_FACTOR) {
			// on a lossy connection the packet will be sent multiple times
			for (int i = unackPrevSize; i < unackedChunks.size(); ++i)
				RequestResend(unackedChunks[i]);
		}
	}
}

void UDPConnection::SendPacket(Packet& pkt)
{
	std::vector<boost::uint8_t> data;
	pkt.Serialize(data);

	outgoing.DataSent(data.size());
	lastPacketSendTime = spring_gettime();
	ip::udp::socket::message_flags flags = 0;
	boost::system::error_code err;

	EMULATE_LATENCY( !EMULATE_PACKET_LOSS( LOSS_COUNTER ) ) {
		mySocket->send_to(buffer(data), addr, flags, err);
	}

	if (CheckErrorCode(err))
		return;

	dataSent += data.size();
	++sentPackets;
}

void UDPConnection::AckChunks(int lastAck)
{
	while (!unackedChunks.empty() && (lastAck >= (*unackedChunks.begin())->chunkNumber))
		unackedChunks.pop_front();

	// resend requested and later acked, happens every now and then
	while (!resendRequested.empty() && lastAck >= resendRequested.begin()->first)
		resendRequested.erase(resendRequested.begin());
}

void UDPConnection::RequestResend(ChunkPtr ptr)
{
	// filter out duplicates
	if (resendRequested.find(ptr->chunkNumber) == resendRequested.end())
		resendRequested[ptr->chunkNumber] = ptr;
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

void UDPConnection::Close(bool flush) {

	if (closed) {
		return;
	}

	Flush(flush);
	muted = true;
	if (!sharedSocket) {
		try {
			mySocket->close();
		} catch (const boost::system::system_error& ex) {
			LOG_L(L_ERROR, "Failed closing UDP connection: %s", ex.what());
		}
	}
	closed = true;
}

void UDPConnection::SetLossFactor(int factor) {
	netLossFactor = std::max((int)MIN_LOSS_FACTOR, std::min(factor, (int)MAX_LOSS_FACTOR));
}

} // namespace netcode
