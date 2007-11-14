#include "LocalConnection.h"

#include <string.h>

#include "Exception.h"

namespace netcode {

// static stuff
unsigned CLocalConnection::Instances = 0;
std::queue<RawPacket*> CLocalConnection::Data[2];
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
		Data[instance].pop();
	}
}

void CLocalConnection::SendData(const unsigned char *data, const unsigned length)
{
	boost::mutex::scoped_lock scoped_lock(Mutex[OtherInstance()]);

	dataSent += length;
	Data[OtherInstance()].push(new RawPacket(data, length));
	dataSent += length;
}

RawPacket* CLocalConnection::GetData()
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	
	if (!Data[instance].empty())
	{
		RawPacket* next = Data[instance].front();
		Data[instance].pop();
		dataRecv += next->length;
		return next;
	}
	else
		return 0;
}

void CLocalConnection::Flush(const bool forced)
{
}

bool CLocalConnection::CheckTimeout() const
{
	return false;
}

unsigned CLocalConnection::OtherInstance() const
{
	if (instance == 0)
		return 1;
	else
		return 0;
}

} // namespace netcode

