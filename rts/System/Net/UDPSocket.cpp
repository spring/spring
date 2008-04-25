#include "UDPSocket.h"

#include "Exception.h"

namespace netcode {
#ifndef _WIN32
const int SOCKET_ERROR = -1;
#else
typedef int socklen_t;
#endif

UDPSocket::UDPSocket(const int port) : Socket(DATAGRAM)
{
	Bind(port);
	SetBlocking(false);
}

UDPSocket::~UDPSocket()
{
}

unsigned UDPSocket::RecvFrom(unsigned char* const buf, const unsigned bufLength, sockaddr_in* const fromAddress) const 
{
	socklen_t fromsize = sizeof(*fromAddress);
	const int data =  recvfrom(mySocket,(char*)buf,bufLength,0,(sockaddr*)fromAddress,&fromsize);
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

void UDPSocket::SendTo(const unsigned char* const buf, const unsigned dataLength, const sockaddr_in* const destination) const
{
	const int error = sendto(mySocket,(const char*)buf,dataLength,0,(const struct sockaddr* const)destination,sizeof(*destination));
	if (error == SOCKET_ERROR)
	{
		if (IsFakeError()) {
			return;
		} else {
			throw network_error(std::string("Error sending data to socket: ") + GetErrorMsg());
    	}
	}
}



} // namespace netcode

 
