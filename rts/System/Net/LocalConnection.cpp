#include "LocalConnection.h"

#include <string.h>
#include <boost/format.hpp>

#include "Exception.h"

namespace netcode {

// static stuff
unsigned CLocalConnection::Instances = 0;
CLocalConnection::MsgQueue CLocalConnection::Data[2];
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
	while (!Data[instance].empty())
	{
		delete Data[instance].front();
		Data[instance].pop_front();
	}
}

void CLocalConnection::SendData(const unsigned char *data, const unsigned length)
{
	SendData(new RawPacket(data, length));
}

void CLocalConnection::SendData(const RawPacket* data)
{
	dataSent += data->length;
	boost::mutex::scoped_lock scoped_lock(Mutex[OtherInstance()]);
	Data[OtherInstance()].push_back(data);
}

const RawPacket* CLocalConnection::Peek(unsigned ahead) const
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);

	if (ahead < Data[instance].size())
		return Data[instance][ahead];

	return NULL;
}

RawPacket* CLocalConnection::GetData()
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	
	if (!Data[instance].empty())
	{
		const RawPacket* next = Data[instance].front();
		Data[instance].pop_front();
		dataRecv += next->length;
		return const_cast<RawPacket*>(next);
	}
	else
		return NULL;
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
	msg += str( boost::format("Recieved: %1% bytes\n") %dataRecv );
	msg += str( boost::format("Sent: %1% bytes\n") %dataSent );
	return msg;
}

NetAddress CLocalConnection::GetPeerName() const
{
	NetAddress addr;
	addr.port = 0;
	addr.host = 0;
	return addr;
}

unsigned CLocalConnection::OtherInstance() const
{
	if (instance == 0)
		return 1;
	else
		return 0;
}

} // namespace netcode

