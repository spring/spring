#include "Connection.h"


namespace netcode {

const unsigned char CConnection::NETMSG_HELLO = 1;

CConnection::CConnection()
{
	dataSent = 0; sentOverhead = 0;
	dataRecv = 0; recvOverhead = 0;
	active=true;
}

CConnection::~CConnection()
{
}

} // namespace netcode
