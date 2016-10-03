/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/format.hpp>

#include "LocalConnection.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Exception.h"
#include "ProtocolDef.h"
#include "System/Log/ILog.h"

namespace netcode {

// static stuff
unsigned CLocalConnection::instances = 0;

std::deque< boost::shared_ptr<const RawPacket> > CLocalConnection::pqueues[2];
std::mutex CLocalConnection::mutexes[2];

CLocalConnection::CLocalConnection()
{
	if (instances > 1) {
		throw network_error("Opening a third local connection is not allowed");
	}

	instance = instances;
	instances++;

	// clear data that might have been left over (if we reloaded)
	pqueues[instance].clear();

	// make sure protocoldef is initialized
	CBaseNetProtocol::Get();
}

CLocalConnection::~CLocalConnection()
{
	instances--;
}

void CLocalConnection::Close(bool flush)
{
	if (flush) {
		std::lock_guard<std::mutex> scoped_lock(mutexes[instance]);
		pqueues[instance].clear();
	}
}

void CLocalConnection::SendData(boost::shared_ptr<const RawPacket> packet)
{
	if (!ProtocolDef::GetInstance()->IsValidPacket(packet->data, packet->length)) {
		// having this check here makes it easier to find networking bugs
		// also when testing locally
		LOG_L(L_ERROR, "[%s] discarding invalid packet: ID %d, LEN %d",
			__FUNCTION__, (packet->length > 0) ? (int)packet->data[0] : -1, packet->length);
		return;
	}

	dataSent += packet->length;

	// when sending from A to B we must lock B's queue
	std::lock_guard<std::mutex> scoped_lock(mutexes[OtherInstance()]);
	pqueues[OtherInstance()].push_back(packet);
}

boost::shared_ptr<const RawPacket> CLocalConnection::GetData()
{
	std::lock_guard<std::mutex> scoped_lock(mutexes[instance]);

	if (!pqueues[instance].empty()) {
		boost::shared_ptr<const RawPacket> next = pqueues[instance].front();
		pqueues[instance].pop_front();
		dataRecv += next->length;
		return next;
	}

	boost::shared_ptr<const RawPacket> empty;
	return empty;
}

boost::shared_ptr<const RawPacket> CLocalConnection::Peek(unsigned ahead) const
{
	std::lock_guard<std::mutex> scoped_lock(mutexes[instance]);

	if (ahead < pqueues[instance].size())
		return pqueues[instance][ahead];

	boost::shared_ptr<const RawPacket> empty;
	return empty;
}

void CLocalConnection::DeleteBufferPacketAt(unsigned index)
{
	std::lock_guard<std::mutex> scoped_lock(mutexes[instance]);

	if (index >= pqueues[instance].size())
		return;

	pqueues[instance].erase(pqueues[instance].begin() + index);
}


std::string CLocalConnection::Statistics() const
{
	std::string msg = "Statistics for local connection:\n";
	msg += str( boost::format("Received: %1% bytes\n") %dataRecv );
	msg += str( boost::format("Sent: %1% bytes\n") %dataSent );
	return msg;
}

std::string CLocalConnection::GetFullAddress() const
{
	return "Localhost";
}


bool CLocalConnection::HasIncomingData() const
{
	std::lock_guard<std::mutex> scoped_lock(mutexes[instance]);
	return (!pqueues[instance].empty());
}

unsigned int CLocalConnection::GetPacketQueueSize() const
{
	std::lock_guard<std::mutex> scoped_lock(mutexes[instance]);
	return (!pqueues[instance].size());
}

} // namespace netcode

