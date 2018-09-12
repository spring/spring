/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UDPConnection.h"

#include <cinttypes>


#include "Socket.h"
#include "ProtocolDef.h"
#include "Exception.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Config/ConfigHandler.h"
#include "System/CRC.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/SpringFormat.h"
#include "System/SafeUtil.h"

#ifndef UNIT_TEST
CONFIG(bool, UDPConnectionLogDebugMessages).defaultValue(false);
#endif


namespace netcode {
using namespace asio;

static constexpr unsigned udpMaxPacketSize = 4096;
static constexpr int maxChunkSize = 254;
static constexpr int chunksPerSec = 30;



#if NETWORK_TEST
static CGlobalUnsyncedRNG rng;

float RANDOM_NUMBER() { return (rng.NextFloat()); }

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
		crc = (uint8_t)rng.NextInt();
}

#define LOSS_COUNTER lossCounter

#else
static int dummyLossCounter = 0;
inline bool EMULATE_PACKET_LOSS(int& lossCtr) { return false; }
inline void EMULATE_PACKET_CORRUPTION(std::uint8_t& crc) {}
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

	void Unpack(std::vector<std::uint8_t>& t, unsigned unpackLength) {
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
	Packer(std::vector<std::uint8_t>& data): data(data)
	{
		assert(data.empty());
	}

	template<typename T>
	void Pack(T& t) {
		const size_t pos = data.size();
		data.resize(pos + sizeof(T));
		*reinterpret_cast<T*>(&data[pos]) = t;
	}

	void Pack(std::vector<std::uint8_t>& _data) {
		std::copy(_data.begin(), _data.end(), std::back_inserter(data));
	}

private:
	std::vector<std::uint8_t>& data;
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
			if (buf.Remaining() < sizeof(naks[i]))
				break;

			if (naks.size() <= i)
				naks.push_back(0);

			buf.Unpack(naks[i]);
		}
	}

	chunks.reserve(buf.Remaining() / Chunk::headerSize);

	while (buf.Remaining() > Chunk::headerSize) {
		ChunkPtr temp(new Chunk);
		buf.Unpack(temp->chunkNumber);
		buf.Unpack(temp->chunkSize);

		// defective, ignore
		if (buf.Remaining() < temp->chunkSize)
			break;

		buf.Unpack(temp->data, temp->chunkSize);
		chunks.push_back(temp);
	}
}


unsigned Packet::GetSize() const
{
	unsigned size = headerSize + naks.size();

	for (auto chk = chunks.begin(); chk != chunks.end(); ++chk)
		size += (*chk)->GetSize();

	return size;
}

std::uint8_t Packet::GetChecksum() const
{
	CRC crc;
	crc << lastContinuous;
	crc << (unsigned int)nakType;

	if (!naks.empty())
		crc.Update(&naks[0], naks.size());

	for (auto chk = chunks.begin(); chk != chunks.end(); ++chk)
		(*chk)->UpdateChecksum(crc);

	return (std::uint8_t)crc.GetDigest();
}

void Packet::Serialize(std::vector<std::uint8_t>& data)
{
	data.clear();
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




UDPConnection::UDPConnection(std::shared_ptr<ip::udp::socket> netSocket, const ip::udp::endpoint& myAddr)
	: addr(myAddr)
	, sharedSocket(true)
	, mySocket(netSocket)
{
	Init();
}

UDPConnection::UDPConnection(int sourcePort, const std::string& address, const unsigned port)
	: sharedSocket(false)
{
	asio::error_code err;
	addr = ResolveAddr(address, port, &err);

	ip::address sourceAddr = GetAnyAddress(addr.address().is_v6());
	std::shared_ptr<ip::udp::socket> tempSocket(new ip::udp::socket(
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
	waitingPackets.reserve(256);
	incomingChunkNums.clear();
	incomingChunkNums.reserve(256);

	resendRequested.clear();
	resendRequested.reserve(256);
	erasedResendChunks.clear();
	erasedResendChunks.reserve(256);

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

	resentChunks = 0;
	sentPackets = 0;
	recvPackets = 0;
	droppedChunks = 0;
	mtu = globalConfig.mtu;
	reconnectTime = globalConfig.reconnectTimeout;

	muted = true;
	closed = false;
	resend = false;

	#ifndef UNIT_TEST
	logMessages = configHandler->GetBool("UDPConnectionLogDebugMessages");
	#endif

	netLossFactor = globalConfig.networkLossFactor;
	lastMidChunk = -1;
#if	NETWORK_TEST
	lossCounter = 0;
#endif
}

void UDPConnection::ReconnectTo(CConnection& conn) {
	dynamic_cast<UDPConnection&>(conn).CopyConnection(*this);
}

void UDPConnection::CopyConnection(UDPConnection &conn) {
	conn.InitConnection(addr, mySocket);
}

void UDPConnection::InitConnection(ip::udp::endpoint address, std::shared_ptr<ip::udp::socket> socket) {
	addr = address;
	mySocket = socket;
}

UDPConnection::~UDPConnection()
{
	fragmentBuffer.Delete();
	waitingPackets.clear();

	Flush(true);
}

void UDPConnection::SendData(std::shared_ptr<const RawPacket> pkt)
{
	assert(pkt->length > 0);
	outgoingData.push_back(pkt);
}

std::shared_ptr<const RawPacket> UDPConnection::Peek(unsigned ahead) const
{
	if (ahead >= msgQueue.size())
		return {};

	return msgQueue[ahead];
}

#ifdef ENABLE_DEBUG_STATS
std::shared_ptr<const RawPacket> UDPConnection::GetData()
{
	numTotalGetDataCalls++;

	if (!msgQueue.empty()) {
		std::shared_ptr<const RawPacket> msg = msgQueue.front();
		msgQueue.pop_front();

		numPings                -= (msg->data[0] == NETMSG_PING    );
		numEnqueuedFramePackets -= (msg->data[0] == NETMSG_NEWFRAME);
		numEnqueuedFramePackets -= (msg->data[0] == NETMSG_KEYFRAME);

		return msg;
	}

	numEmptyGetDataCalls++;
	return {};
}
#else
std::shared_ptr<const RawPacket> UDPConnection::GetData()
{
	if (msgQueue.empty())
		return {};

	numPings -= (msgQueue[0]->data[0] == NETMSG_PING);

	std::shared_ptr<const RawPacket> msg = msgQueue.front();
	msgQueue.pop_front();
	return msg;
}
#endif


void UDPConnection::DeleteBufferPacketAt(unsigned index)
{
	if (index >= msgQueue.size())
		return;

	numPings -= (msgQueue[index]->data[0] == NETMSG_PING);
	msgQueue.erase(msgQueue.begin() + index);
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
					__func__, numReceivedFramePackets, debugMssgDeltaTime, avgFramePacketRate, numEmptyGetDataCalls, numTotalGetDataCalls
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

		size_t bytesAvailable = 0;

		while ((bytesAvailable = mySocket->available()) > 0) {
			recvBuffer.clear();
			recvBuffer.resize(bytesAvailable, 0);

			ip::udp::endpoint udpEndPoint;
			ip::udp::socket::message_flags msgFlags = 0;
			asio::error_code err;

			const size_t bytesReceived = mySocket->receive_from(asio::buffer(recvBuffer), udpEndPoint, msgFlags, err);

			if (CheckErrorCode(err))
				break;

			if (bytesReceived < Packet::headerSize)
				continue;

			Packet data(&recvBuffer[0], bytesReceived);

			if (IsUsingAddress(udpEndPoint))
				ProcessRawPacket(data);

			// not likely, but make sure we do not get stuck here
			if ((spring_gettime() - curTime) > spring_msecs(10)) {
				break;
			}
		}
	}


	Flush(false);
}

void UDPConnection::UpdateWaitingPackets()
{
	const auto beg = waitingPackets.begin();
	const auto end = waitingPackets.end();
	const auto pos = std::remove_if(beg, end, [](const std::pair<int, RawPacket>& p) { return ((p.second).length == 0); });

	// erase processed packets
	waitingPackets.erase(pos, end);
}

void UDPConnection::UpdateResendRequests()
{
	using P = decltype(resendRequested)::value_type;

	const auto cmpPred = [](const P& a, const P& b) { return (a.first <  b.first); };
	const auto dupPred = [](const P& a, const P& b) { return (a.first == b.first); };

	// sort by chunk-number
	std::sort(resendRequested.begin(), resendRequested.end(), cmpPred);

	{
		const auto beg = resendRequested.begin();
		const auto end = resendRequested.end();
		const auto iter = std::unique(beg, end, dupPred);

		// filter duplicates
		resendRequested.erase(iter, end);
	}

	if (erasedResendChunks.empty())
		return;

	{
		const auto pred = [&](const std::pair<std::int32_t, ChunkPtr>& p) { return (erasedResendChunks.find(p.first) != erasedResendChunks.end()); };

		const auto beg = resendRequested.begin();
		const auto end = resendRequested.end();
		const auto pos = std::remove_if(beg, end, pred);

		// remove chunks that no longer need resending
		resendRequested.erase(pos, end);
		erasedResendChunks.clear();
	}
}


void UDPConnection::ProcessRawPacket(Packet& incoming)
{
	#ifdef ENABLE_DEBUG_STATS
	if (logMessages)
		LOG_L(L_INFO, "\t[%s] checksum=(%u : %u) mtu=%u", __func__, incoming.GetChecksum(), incoming.checksum, mtu);
	#endif

	lastPacketRecvTime = spring_gettime();
	dataRecv += incoming.GetSize();
	recvOverhead += Packet::headerSize;
	recvPackets += 1;

//	if (EMULATE_PACKET_LOSS(lossCounter))
//		return;

	if (incoming.GetChecksum() != incoming.checksum) {
		LOG_L(L_ERROR, "\t[%s] discarding incoming corrupted packet: CRC %d, LEN %d", __func__, incoming.checksum, incoming.GetSize());
		return;
	}

	if (incoming.lastContinuous < 0 && lastInOrder >= 0 &&
		(unackedChunks.empty() || unackedChunks[0]->chunkNumber > 0)) {
		LOG_L(L_WARNING, "\t[%s] discarding superfluous reconnection attempt", __func__);
		return;
	}


	AckChunks(incoming.lastContinuous);
	UpdateResendRequests();

	if (!unackedChunks.empty()) {
		const int nextCont = incoming.lastContinuous + 1;
		const int unAckDiff = unackedChunks[0]->chunkNumber - nextCont;

		if (-256 <= unAckDiff && unAckDiff <= 256) {
			if (incoming.nakType < 0) {
				for (int i = 0; i != -incoming.nakType; ++i) {
					const int unAckPos = i + unAckDiff;

					if (unAckPos >= 0 && unAckPos < unackedChunks.size()) {
						assert(unackedChunks[unAckPos]->chunkNumber == nextCont + i);
						RequestResend(unackedChunks[unAckPos], true);
					}
				}
			} else if (incoming.nakType > 0) {
				int unAckPos = 0;

				for (int i = 0; i != incoming.naks.size(); ++i) {
					if (unAckDiff + incoming.naks[i] < 0)
						continue;

					while (unAckPos < (unAckDiff + incoming.naks[i])) {
						// if there are gaps in the array, assume that further resends are not needed
						if (unAckPos < unackedChunks.size())
							erasedResendChunks.insert(unackedChunks[unAckPos]->chunkNumber);

						++unAckPos;
					}

					if (unAckPos < unackedChunks.size()) {
						assert(unackedChunks[unAckPos]->chunkNumber == (nextCont + incoming.naks[i]));
						RequestResend(unackedChunks[unAckPos], true);
					}

					++unAckPos;
				}
			}

			UpdateResendRequests();
		}
	}


	for (const std::shared_ptr<netcode::Chunk>& c: incoming.chunks) {
		if ((lastInOrder >= c->chunkNumber) || incomingChunkNums.find(c->chunkNumber) != incomingChunkNums.end()) {
			++droppedChunks;
			continue;
		}

		waitingPackets.emplace_back(c->chunkNumber, std::move(RawPacket(&c->data[0], c->data.size())));
		incomingChunkNums.insert(c->chunkNumber);
	}


	using P = decltype(waitingPackets)::value_type;

	const auto cmpPred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto binFind = [&](int cn) { return std::lower_bound(waitingPackets.begin(), waitingPackets.end(), P{cn, RawPacket{}}, cmpPred); };

	std::sort(waitingPackets.begin(), waitingPackets.end(), cmpPred);

	// process all in-order packets that we have waiting
	for (auto wpi = binFind(lastInOrder + 1); wpi != waitingPackets.end() && wpi->first == (lastInOrder + 1); ++wpi) {
		waitBuffer.clear();

		if (fragmentBuffer.data != nullptr) {
			// combine with fragment buffer (packet reassembly)
			waitBuffer.resize(fragmentBuffer.length);
			waitBuffer.assign(fragmentBuffer.data, fragmentBuffer.data + fragmentBuffer.length);

			fragmentBuffer.Delete();
		}

		std::copy(wpi->second.data, wpi->second.data + wpi->second.length, std::back_inserter(waitBuffer));

		incomingChunkNums.erase(wpi->first);
		// waitingPackets.erase(wpi);

		// mark as processed
		(wpi->second).Delete();

		// next expected chunk-number
		lastInOrder++;


		for (unsigned pos = 0; pos < waitBuffer.size(); ) {
			const unsigned char* bufp = &waitBuffer[pos];
			const unsigned int msgLength = waitBuffer.size() - pos;

			const int pktLength = ProtocolDef::GetInstance()->PacketLength(bufp, msgLength);

			// this returns false for zero/invalid pktLength
			if (ProtocolDef::GetInstance()->IsValidLength(pktLength, msgLength)) {
				msgQueue.emplace_back(new RawPacket(bufp, pktLength));
				std::shared_ptr<const RawPacket>& msgPacket = msgQueue.back();

				#ifdef ENABLE_DEBUG_STATS
				// server sends both of these, clients send only keyframe messages
				// TODO: would be easy to feed this data into a Q3A-style lagometer
				//
				if (msgPacket->data[0] == NETMSG_NEWFRAME || msgPacket->data[0] == NETMSG_KEYFRAME) {
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
							__func__, numReceivedFramePackets, numEnqueuedFramePackets, dt.toMilliSecsf(),
							minDeltaFramePacketRecvTime, maxDeltaFramePacketRecvTime, sumDeltaFramePacketRecvTime
						);
					}
				}
				#endif

				pos += pktLength;
				numPings += (msgPacket->data[0] == NETMSG_PING); // incoming
			} else {
				if (pktLength >= 0) {
					// partial packet in buffer
					fragmentBuffer = std::move(RawPacket(bufp, msgLength));
					break;
				}

				LOG_L(L_ERROR, "\t[%s] discarding incoming invalid packet: ID %d, LEN %d", __func__, (int)*bufp, pktLength);

				// if the packet is invalid, skip a single byte
				// until we encounter a good packet
				++pos;
			}
		}
	}

	UpdateWaitingPackets();
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
		std::uint8_t buffer[udpMaxPacketSize];
		unsigned pos = 0;

		// Manually fragment packets to respect configured UDP_MTU.
		// This is an attempt to fix the bug where players drop out
		// of the game if someone in the game gives a large order.
		bool partialPacket = false;
		bool sendMore = true;

		do {
			sendMore  = (outgoing.GetAverage(true) <= globalConfig.linkOutgoingBandwidth);
			sendMore |= ((globalConfig.linkOutgoingBandwidth <= 0) || partialPacket || forced);

			if (!outgoingData.empty() && sendMore) {
				std::shared_ptr<const RawPacket>& packet = *(outgoingData.begin());

				if (!partialPacket && !ProtocolDef::GetInstance()->IsValidPacket(packet->data, packet->length)) {
					LOG_L(L_ERROR,
						"[UDPConnection::%s] discarding outgoing invalid packet: ID %d, LEN %d",
						__func__, ((packet->length > 0) ? (int)packet->data[0] : -1), packet->length
					);
					outgoingData.pop_front();
				} else {
					const unsigned numBytes = std::min((unsigned)maxChunkSize - pos, packet->length);

					assert(packet->length > 0);
					memcpy(buffer + pos, packet->data, numBytes);

					pos += numBytes;
					sentOverhead += Packet::headerSize;

					outgoing.DataSent(numBytes, true);

					if ((partialPacket = (numBytes != packet->length))) {
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
				? globalConfig.networkTimeout
				: globalConfig.initialNetworkTimeout;
	} else if (seconds > 0) {
		timeout = seconds;
	} else {
		timeout = globalConfig.reconnectTimeout;
	}

	return (timeout > 0 && (spring_gettime() - lastPacketRecvTime) > spring_secs(timeout));
}

bool UDPConnection::NeedsReconnect() {

	if (CanReconnect()) {
		if (!CheckTimeout(-1)) {
			reconnectTime = globalConfig.reconnectTimeout;
		} else if (CheckTimeout(reconnectTime)) {
			++reconnectTime;
			return true;
		}
	}

	return false;
}

bool UDPConnection::CanReconnect() const {
	return (globalConfig.reconnectTimeout > 0);
}

std::string UDPConnection::Statistics() const
{
	const char* fmts[] = {
		"\t%u bytes sent   in %u packets (%.3f bytes/packet)\n",
		"\t%u bytes recv'd in %u packets (%.3f bytes/packet)\n",
		"\t{%.3fx, %.3fx} relative protocol overhead {up, down}\n",
		"\t%u incoming chunks dropped, %u outgoing chunks resent\n",
		"\t%u incoming chunks processed\n",
	};

	std::string msg = "[UDPConnection::Statistics]\n";
	msg += spring::format(fmts[0], dataSent, sentPackets, spring::SafeDivide(dataSent * 1.0f, sentPackets * 1.0f));
	msg += spring::format(fmts[1], dataRecv, recvPackets, spring::SafeDivide(dataRecv * 1.0f, recvPackets * 1.0f));
	msg += spring::format(fmts[2], spring::SafeDivide(sentOverhead * 1.0f, dataSent * 1.0f), spring::SafeDivide(recvOverhead * 1.0f, dataRecv * 1.0f));
	msg += spring::format(fmts[3], droppedChunks, resentChunks);
	msg += spring::format(fmts[4], lastInOrder + 1);
	return msg;
}

std::string UDPConnection::GetFullAddress() const
{
	return spring::format("[%s]:%u", addr.address().to_string().c_str(), addr.port());
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
	std::copy(data, data + length, std::back_inserter(buf->data));
	newChunks.push_back(buf);
	lastChunkCreatedTime = spring_gettime();
}

void UDPConnection::SendIfNecessary(bool flushed)
{
	const spring_time curTime = spring_gettime();
	const spring_time difTime = curTime - lastPacketSendTime;
	const spring_time unackTime = spring_msecs(400 >> netLossFactor);

	int nak = 0;
	int rev = 0;

	droppedPackets.clear();

	{
		int packetNum = lastInOrder + 1;

		for (const auto& pair: waitingPackets) {
			const int diff = pair.first - packetNum;

			for (int i = 0; i < diff; ++i) {
				droppedPackets.push_back(packetNum++);
			}

			packetNum++;
		}

		while (!droppedPackets.empty() && (droppedPackets.back() - (lastInOrder + 1)) > 255) {
			droppedPackets.pop_back();
		}


		unsigned int numContinuous = 0;

		for (unsigned int i = 0; i != droppedPackets.size(); ++i) {
			if (droppedPackets[i] != (lastInOrder + i + 1))
				break;

			numContinuous++;
		}

		if ((numContinuous < 8) && (curTime - lastNakTime) > (unackTime * 0.5f)) {
			nak = std::min(droppedPackets.size(), (size_t)127);
			// needs 1 byte per requested packet, so do not spam to often
			lastNakTime = curTime;
		} else {
			nak = -(int)std::min(127u, numContinuous);
		}
	}

	if (!unackedChunks.empty() &&
		(curTime - lastChunkCreatedTime) > unackTime &&
		(curTime - lastUnackResentTime) > unackTime) {

		// resend last packet if we didn't get an ack within reasonable time
		// and don't plan sending out a new chunk either
		if (newChunks.empty())
			RequestResend(*unackedChunks.rbegin(), false);

		lastUnackResentTime = curTime;
	}


	const bool flushSend = (flushed || !newChunks.empty());
	const bool otherSend = (UseMinLossFactor() && !resendRequested.empty());
	const bool unackSend = (nak > 0) || (difTime > (unackTime * 0.5f));

	if (!flushSend && !otherSend && !unackSend)
		return;

	int maxResend = resendRequested.size();
	int unackPrevSize = unackedChunks.size();

	decltype(resendRequested)::iterator resFwdIter = resendRequested.begin();
	decltype(resendRequested)::iterator resMidIter;
	decltype(resendRequested)::iterator resMidIterStart;
	decltype(resendRequested)::iterator resMidIterEnd;
	decltype(resendRequested)::reverse_iterator resRevIter;

	// resend chunk size
	const auto CalcResendSize = [&]() {
		return ((UseMinLossFactor() || (rev == 0)) ? resFwdIter->second->GetSize() : ((rev == 1) ? resRevIter->second->GetSize() : resMidIter->second->GetSize()));
	};

	if (!UseMinLossFactor()) {
		// keep resend reasonable, or it could cause a tremendous flood of packets
		maxResend = std::min(maxResend, 20 * netLossFactor);

		resMidIter = resendRequested.begin();
		resMidIterStart = resendRequested.begin();
		resMidIterEnd = resendRequested.end();
		resRevIter = resendRequested.rbegin();

		const int resMidStart = (maxResend + 3) / 4;
		const int resMidEnd   = (maxResend + 2) / 4;

		std::advance(resMidIterStart, resMidStart);

		if (resMidIterStart != resendRequested.end() && lastMidChunk < resMidIterStart->first)
			lastMidChunk = resMidIterStart->first - 1;

		std::advance(resMidIterEnd, -resMidEnd);

		while (resMidIter != resendRequested.end() && resMidIter->first <= lastMidChunk) {
			++resMidIter;
		}

		if (resMidIter == resendRequested.end() || resMidIterEnd == resendRequested.end() || resMidIter->first >= resMidIterEnd->first)
			resMidIter = resMidIterStart;
	}


	while (((outgoing.GetAverage() <= globalConfig.linkOutgoingBandwidth) || (globalConfig.linkOutgoingBandwidth <= 0))) {
		Packet buf(lastInOrder, nak);

		if (nak > 0) {
			buf.naks.resize(nak);

			for (unsigned i = 0; i != buf.naks.size(); ++i) {
				buf.naks[i] = droppedPackets[i] - (lastInOrder + 1); // zero means request resend of lastInOrder + 1
			}

			// 1 request is enough, unless high loss
			nak *= (1 - UseMinLossFactor());
		}


		bool sent = false;

		while (true) {
			// NB: if maxResend equals 0, then resendRequested is empty and iterators will be invalid
			const bool canResend = (maxResend > 0) && ((buf.GetSize() + CalcResendSize()) <= mtu);
			const bool canSendNew = !newChunks.empty() && ((buf.GetSize() + newChunks[0]->GetSize()) <= mtu);

			if (!canResend && !canSendNew)
				break;

			// alternate between send and resend to make sure neither is starved
			resend = !resend;

			if (resend && canResend) {
				if (UseMinLossFactor()) {
					if (erasedResendChunks.find(resFwdIter->first) == erasedResendChunks.end())
						buf.chunks.push_back(resFwdIter->second);

					erasedResendChunks.insert((resFwdIter++)->first);
				} else {
					// on a lossy connection, just keep resending until it is acked
					// alternate between sending from front, middle and back of requested
					// chunks, since this improves performance on high latency connections
					switch (rev) {
						case 0: {
							buf.chunks.push_back((resFwdIter++)->second);
						} break;
						case 1: {
							buf.chunks.push_back((resRevIter++)->second);
						} break;
						case 2:
						case 3: {
							buf.chunks.push_back(resMidIter->second);

							lastMidChunk = resMidIter->first;

							if ((++resMidIter) == resMidIterEnd)
								resMidIter = resMidIterStart;
						} break;
					}

					rev = (rev + 1) % 4;
				}

				resentChunks += 1;
				maxResend -= 1;

				sent = true;
			} else if (!resend && canSendNew) {
				buf.chunks.push_back(newChunks[0]);
				unackedChunks.push_back(newChunks[0]);
				newChunks.pop_front();
				sent = true;
			}
		}

		buf.checksum = buf.GetChecksum();
		EMULATE_PACKET_CORRUPTION(buf.checksum);

		SendPacket(buf);

		if (!sent || (maxResend == 0 && newChunks.empty()))
			break;
	}


	if (UseMinLossFactor()) {
		UpdateResendRequests();
		return;
	}

	// on a lossy connection chunks can be sent multiple times, see switch above
	for (int i = unackPrevSize; i < unackedChunks.size(); ++i) {
		RequestResend(unackedChunks[i], true);
	}

	UpdateResendRequests();
}

void UDPConnection::SendPacket(Packet& pkt)
{
	pkt.Serialize(sendBuffer);

	outgoing.DataSent(sendBuffer.size());
	lastPacketSendTime = spring_gettime();

	ip::udp::socket::message_flags flags = 0;
	asio::error_code err;

	EMULATE_LATENCY( !EMULATE_PACKET_LOSS( LOSS_COUNTER ) ) {
		mySocket->send_to(buffer(sendBuffer), addr, flags, err);
	}

	if (CheckErrorCode(err))
		return;

	dataSent += sendBuffer.size();
	sentPackets += 1;
}

void UDPConnection::AckChunks(int lastAck)
{
	while (!unackedChunks.empty() && (lastAck >= (*unackedChunks.begin())->chunkNumber)) {
		unackedChunks.pop_front();
	}

	// resend requested and later acked, happens every now and then
	for (size_t i = 0, n = resendRequested.size(); i < n; i++) {
		if (lastAck < resendRequested[i].first)
			break;

		erasedResendChunks.insert(resendRequested[i].first);
	}
}

void UDPConnection::RequestResend(ChunkPtr ptr, bool noSort)
{
	resendRequested.emplace_back(ptr->chunkNumber, ptr);

	if (noSort)
		return;

	// swap into position; duplicates are filtered out later
	for (size_t i = resendRequested.size() - 1; i > 0; i--) {
		if (resendRequested[i - 1].first < resendRequested[i].first)
			break;

		std::swap(resendRequested[i - 1], resendRequested[i]);
	}
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

	if (closed)
		return;

	Flush(flush);
	muted = true;
	if (!sharedSocket) {
		try {
			mySocket->close();
		} catch (const asio::system_error& ex) {
			LOG_L(L_ERROR, "[UDPConnection::%s] error \"%s\" closing socket", __func__, ex.what());
		}
	}
	closed = true;
}

void UDPConnection::SetLossFactor(int factor) {
	netLossFactor = factor;
	netLossFactor = std::max(netLossFactor, int(MIN_LOSS_FACTOR));
	netLossFactor = std::min(netLossFactor, int(MAX_LOSS_FACTOR));
}

} // namespace netcode
