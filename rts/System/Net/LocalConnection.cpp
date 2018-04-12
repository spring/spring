/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LocalConnection.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Exception.h"
#include "ProtocolDef.h"
#include "System/Log/ILog.h"
#include "System/SpringFormat.h"

namespace netcode {

// static stuff
unsigned CLocalConnection::instances = 0;

std::deque< std::shared_ptr<const RawPacket> > CLocalConnection::pktQueues[2];
spring::mutex CLocalConnection::mutexes[2];

CLocalConnection::CLocalConnection()
{
	if (instances > 1)
		throw network_error("Opening a third local connection is not allowed");

	instance = instances;
	instances++;

	// clear data that might have been left over (if we reloaded)
	pktQueues[instance].clear();

	// make sure protocoldef is initialized
	CBaseNetProtocol::Get();
}


void CLocalConnection::Close(bool flush)
{
	if (flush) {
		std::lock_guard<spring::mutex> scoped_lock(mutexes[instance]);
		pktQueues[instance].clear();
	}
}

void CLocalConnection::SendData(std::shared_ptr<const RawPacket> pkt)
{
	if (!ProtocolDef::GetInstance()->IsValidPacket(pkt->data, pkt->length)) {
		// having this check here makes it easier to find networking bugs
		// also when testing locally
		LOG_L(L_ERROR, "[LocalConn::%s] discarding invalid packet: ID %d, LEN %d",
			__func__, (pkt->length > 0) ? (int)pkt->data[0] : -1, pkt->length);
		return;
	}

	dataSent += pkt->length;
	numPings += (pkt->data[0] == NETMSG_PING);

	// when sending from A to B we must lock B's queue
	std::lock_guard<spring::mutex> scoped_lock(mutexes[OtherInstance()]);
	pktQueues[OtherInstance()].push_back(pkt);
}

std::shared_ptr<const RawPacket> CLocalConnection::GetData()
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instance]);

	if (pktQueues[instance].empty())
		return {};

	std::shared_ptr<const RawPacket> pkt = pktQueues[instance].front();
	pktQueues[instance].pop_front();

	dataRecv += pkt->length;
	numPings -= (pkt->data[0] == NETMSG_PING);
	return pkt;
}

std::shared_ptr<const RawPacket> CLocalConnection::Peek(unsigned ahead) const
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instance]);

	if (ahead >= pktQueues[instance].size())
		return {};

	return pktQueues[instance][ahead];
}

void CLocalConnection::DeleteBufferPacketAt(unsigned index)
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instance]);

	if (index >= pktQueues[instance].size())
		return;

	numPings -= (pktQueues[instance][0]->data[0] == NETMSG_PING);
	pktQueues[instance].erase(pktQueues[instance].begin() + index);
}


std::string CLocalConnection::Statistics() const
{
	std::string msg = "[LocalConnection::Statistics]\n";
	msg += spring::format("\t%u bytes sent  \n", dataRecv);
	msg += spring::format("\t%u bytes recv'd\n", dataSent);
	return msg;
}

std::string CLocalConnection::GetFullAddress() const
{
	return "Localhost";
}


bool CLocalConnection::HasIncomingData() const
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instance]);
	return (!pktQueues[instance].empty());
}

unsigned int CLocalConnection::GetPacketQueueSize() const
{
	std::lock_guard<spring::mutex> scoped_lock(mutexes[instance]);
	return (!pktQueues[instance].size());
}

} // namespace netcode

