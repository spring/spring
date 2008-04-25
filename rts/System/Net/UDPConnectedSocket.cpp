#include "UDPConnectedSocket.h"

#include "Exception.h"

namespace netcode
{

#ifndef _WIN32
const int SOCKET_ERROR = -1;
#endif

UDPConnectedSocket::UDPConnectedSocket(const std::string& server, const unsigned remoteport)
: Socket(DATAGRAM)
{
	sockaddr_in remoteAddr = ResolveHost(server, remoteport);
	if (connect(mySocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR)
	{
		throw network_error(std::string("Error while connecting: ") + GetErrorMsg());
	}
	SetBlocking(false);
}


void UDPConnectedSocket::Send(const unsigned char* const buf, const unsigned dataLength) const
{
	int error = send(mySocket, (char*)buf, dataLength, 0);
	if (error == SOCKET_ERROR && !IsFakeError())
	{
		throw network_error(std::string("Error sending data to socket: ") + GetErrorMsg());
	}
}

unsigned UDPConnectedSocket::Recv(unsigned char* buf, const unsigned bufLength) const
{
	const int data = recv(mySocket,(char*)buf,bufLength,0);
	if (data == SOCKET_ERROR)
	{
		if (IsFakeError())
		{
			return 0;
		}
		else
		{
			throw network_error(std::string("Error receiving data from socket: ") + GetErrorMsg());
		}
	}
	return (unsigned)data;
}


} // namespace netcode
 
 
