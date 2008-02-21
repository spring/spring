#include "UDPConnectedSocket.h"

#include <string>

#ifdef _WIN32
#else
#include <sys/socket.h>
#endif

#include "Exception.h"

namespace netcode
{

#ifdef _WIN32
#else
	const int SOCKET_ERROR = -1;
#endif

UDPConnectedSocket::UDPConnectedSocket(const char* const server, const unsigned remoteport, const int port)
: UDPSocket(port)
{
	sockaddr_in remoteAddr = ResolveHost(server, remoteport);
	connect(mySocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr));
}


void UDPConnectedSocket::Send(const unsigned char* const buf, const unsigned dataLength) const
{
	send(mySocket, (char*)buf, dataLength, 0);
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
	return data;
}


} // namespace netcode
 
 
