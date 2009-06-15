#include "LocalConnection.h"

#include <boost/format.hpp>

#include "Exception.h"

namespace netcode {

// static stuff
unsigned CLocalConnection::Instances = 0;
std::deque< boost::shared_ptr<const RawPacket> > CLocalConnection::Data[2];
boost::mutex CLocalConnection::Mutex[2];

CLocalConnection::CLocalConnection()
{
	if (Instances > 1)
	{
		throw network_error("Opening a third local connection is not allowed");
	}
	instance = Instances;
	Instances++;
}

CLocalConnection::~CLocalConnection()
{
	Instances--;
}

void CLocalConnection::SendData(boost::shared_ptr<const RawPacket> data)
{
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

boost::shared_ptr<const RawPacket> CLocalConnection::GetData()
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	
	if (!Data[instance].empty())
	{
		boost::shared_ptr<const RawPacket> next = Data[instance].front();
		Data[instance].pop_front();
		dataRecv += next->length;
		return next;
	}
	else
	{
		boost::shared_ptr<const RawPacket> empty;
		return empty;
	}
}

void CLocalConnection::Flush(const bool forced)
{
}

bool CLocalConnection::CheckTimeout() const
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

bool CLocalConnection::HasIncomingData() const
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	return (!Data[instance].empty());
}

unsigned CLocalConnection::OtherInstance() const
{
	if (instance == 0)
		return 1;
	else
		return 0;
}

} // namespace netcode

