/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LocalConnection.h"

#include <boost/format.hpp>

#include "Exception.h"
#include "ProtocolDef.h"
#include "System/BaseNetProtocol.h"
#include "System/Log/ILog.h"

namespace netcode {

// static stuff
unsigned CLocalConnection::instances = 0;
std::deque< boost::shared_ptr<const RawPacket> > CLocalConnection::Data[2];
boost::mutex CLocalConnection::Mutex[2];

CLocalConnection::CLocalConnection()
{
	if (instances > 1) {
		throw network_error("Opening a third local connection is not allowed");
	}
	instance = instances;
	instances++;
	CBaseNetProtocol::Get(); // make sure protocoldef is initialized
}

CLocalConnection::~CLocalConnection()
{
	instances--;
}

void CLocalConnection::SendData(boost::shared_ptr<const RawPacket> data)
{
	if (!ProtocolDef::GetInstance()->IsValidPacket(data->data, data->length)) {
		// having this check here makes it easier to find networking bugs
		// also when testing locally
		LOG_L(L_ERROR, "Discarding invalid packet: ID %d, LEN %d",
				(data->length > 0) ? (int)data->data[0] : -1, data->length);
		return;
	}

	dataSent += data->length;
	boost::mutex::scoped_lock scoped_lock(Mutex[OtherInstance()]);
	Data[OtherInstance()].push_back(data);
}

boost::shared_ptr<const RawPacket> CLocalConnection::Peek(unsigned ahead) const
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);

	if (ahead < Data[instance].size())
		return Data[instance][ahead];
	else
	{
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

void CLocalConnection::DeleteBufferPacketAt(unsigned index)
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);

	if (index < Data[instance].size())
		Data[instance].erase(Data[instance].begin() + index);
}

boost::shared_ptr<const RawPacket> CLocalConnection::GetData()
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);

	if (!Data[instance].empty()) {
		boost::shared_ptr<const RawPacket> next = Data[instance].front();
		Data[instance].pop_front();
		dataRecv += next->length;
		return next;
	} else {
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

void CLocalConnection::Flush(const bool forced)
{
}

bool CLocalConnection::CheckTimeout(int seconds, bool initial) const
{
	return false;
}

bool CLocalConnection::CanReconnect() const
{
	return false;
}

bool CLocalConnection::NeedsReconnect()
{
	return false;
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
	return "shared memory";
}

bool CLocalConnection::HasIncomingData() const
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	return (!Data[instance].empty());
}

unsigned CLocalConnection::OtherInstance() const
{
	if (instance == 0) {
		return 1;
	} else {
		return 0;
	}
}

} // namespace netcode

