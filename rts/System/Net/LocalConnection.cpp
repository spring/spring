#include "LocalConnection.h"

#include <string.h>
#include <stdexcept>
//#include <iostream>

#include "ProtocolDef.h"
#include "UDPSocket.h"

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
	//std::cout << "Sending " << length << " bytes to " << OtherInstance() << std::endl;
	boost::mutex::scoped_lock scoped_lock(Mutex[OtherInstance()]);
	
	Data[OtherInstance()].push(new RawPacket(data, length));
	dataSent += length;
}

unsigned CLocalConnection::GetData(unsigned char *buf)
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	
	RawPacket* next = Data[instance].empty() ? 0 : Data[instance].front();
	
	if (next)
	{
		unsigned ret = next->length;
		//std::cout << "Recieving " << ret << " bytes from " << instance << std::endl;
		dataRecv += ret;
		memcpy(buf,next->data,ret);
		Data[instance].pop();
		delete next;
		return ret;
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

