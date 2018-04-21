/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LocalConnection.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Exception.h"
#include "ProtocolDef.h"
#include "System/Log/ILog.h"
#include "System/SpringFormat.h"

namespace netcode {

// static stuff
unsigned int CLocalConnection::numInstances = 0;

std::deque< std::shared_ptr<const RawPacket> > CLocalConnection::pktQueues[CLocalConnection::MAX_INSTANCES];
spring::mutex CLocalConnection::mutexes[CLocalConnection::MAX_INSTANCES];
CLocalConnection* CLocalConnection::instancePtrs[MAX_INSTANCES] = {nullptr, nullptr};

CLocalConnection::CLocalConnection()
{
	if (numInstances >= MAX_INSTANCES)
		throw network_error("Opening a third local connection is not allowed");

	// clear data that might have been left over (if we reloaded)
	pktQueues[instanceIdx = numInstances++].clear();
	instancePtrs[instanceIdx] = this;

	// make sure protocoldef is initialized
	CBaseNetProtocol::Get();
}

CLocalConnection::~CLocalConnection()
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);

	instancePtrs[instanceIdx] = nullptr;
	numInstances--;
}


void CLocalConnection::Close(bool flush)
{
	if (!flush)
		return;

	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);
	pktQueues[instanceIdx].clear();
}

void CLocalConnection::SendData(std::shared_ptr<const RawPacket> pkt)
{
	if (!ProtocolDef::GetInstance()->IsValidPacket(pkt->data, pkt->length)) {
		// having this check here makes it easier to find networking bugs, also when testing locally
		LOG_L(L_ERROR, "[LocalConn::%s] discarding invalid packet: ID %d, LEN %d", __func__, (pkt->length > 0) ? (int)pkt->data[0] : -1, pkt->length);
		return;
	}

	dataSent += pkt->length;

	{
		// when sending from A to B we must lock B's queue
		std::lock_guard<spring::mutex> scoped_lock(mutexes[RemoteInstanceIdx()]);

		// outgoing for A, incoming for B
		if (instancePtrs[RemoteInstanceIdx()] != nullptr)
			instancePtrs[RemoteInstanceIdx()]->numPings += (pkt->data[0] == NETMSG_PING);

		pktQueues[RemoteInstanceIdx()].push_back(pkt);
	}
}

std::shared_ptr<const RawPacket> CLocalConnection::GetData()
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);
	std::deque<std::shared_ptr<const RawPacket>>& pktQueue = pktQueues[instanceIdx];

	if (pktQueue.empty())
		return {};

	std::shared_ptr<const RawPacket> pkt = pktQueue.front();
	pktQueue.pop_front();

	dataRecv += pkt->length;
	numPings -= (pkt->data[0] == NETMSG_PING);
	return pkt;
}

std::shared_ptr<const RawPacket> CLocalConnection::Peek(unsigned ahead) const
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);
	std::deque<std::shared_ptr<const RawPacket>>& pktQueue = pktQueues[instanceIdx];

	if (ahead >= pktQueue.size())
		return {};

	return pktQueue[ahead];
}

void CLocalConnection::DeleteBufferPacketAt(unsigned index)
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);
	std::deque<std::shared_ptr<const RawPacket>>& pktQueue = pktQueues[instanceIdx];

	if (index >= pktQueue.size())
		return;

	numPings -= (pktQueue[0]->data[0] == NETMSG_PING);
	pktQueue.erase(pktQueue.begin() + index);
}


std::string CLocalConnection::Statistics() const
{
	std::string msg = "[LocalConnection::Statistics]\n";
	msg += spring::format("\t%u bytes sent  \n", dataSent);
	msg += spring::format("\t%u bytes recv'd\n", dataRecv);
	return msg;
}


bool CLocalConnection::HasIncomingData() const
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);
	return (!pktQueues[instanceIdx].empty());
}

unsigned int CLocalConnection::GetPacketQueueSize() const
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instanceIdx]);
	return (!pktQueues[instanceIdx].size());
}

} // namespace netcode

