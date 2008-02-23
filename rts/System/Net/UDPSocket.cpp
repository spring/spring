#include "UDPSocket.h"

#include "Exception.h"

namespace netcode {


#ifdef _WIN32
	typedef int socklen_t;
#else
	const int SOCKET_ERROR = -1;
#endif


UDPSocket::UDPSocket(const int port) : Socket(UDP)
{
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Let the OS assign a address	
	myAddr.sin_port = htons(port);	   // Use port passed from user

	if (bind(mySocket,(struct sockaddr *)&myAddr,sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		throw network_error(std::string("Error binding socket: ") + GetErrorMsg());
	}
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

 
