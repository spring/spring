#include "UDPSocket.h"

#ifdef _WIN32
	#include <direct.h>
	#include <io.h>
#else
	#include <fcntl.h>
	#include <errno.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif

#include "Exception.h"

namespace netcode {


#ifdef _WIN32
	typedef int socklen_t;
	inline int close(SOCKET mySocket) { return closesocket(mySocket); };
	unsigned UDPSocket::numSockets = 0;
#else
	typedef struct hostent* LPHOSTENT;
	typedef struct in_addr* LPIN_ADDR;
	const int INVALID_SOCKET = -1;
	const int SOCKET_ERROR = -1;
#endif


UDPSocket::UDPSocket(const int port)
{
#ifdef _WIN32
	if (numSockets == 0)
	{
		unsigned short wVersionRequested;
		WSADATA wsaData;
		int err;
			
		wVersionRequested = MAKEWORD( 2, 2 );
		err = WSAStartup( wVersionRequested, &wsaData );if ( err != 0 ) {
			throw network_error("Error initializing winsock: failed to start");
			return;
		}
		/* Confirm that the WinSock DLL supports 2.2.*/
		/* Note that if the DLL supports versions greater    */
		/* than 2.2 in addition to 2.2, it will still return */
		/* 2.2 in wVersion since that is the version we      */
		/* requested.                                        */
		if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) {
			throw network_error("Error initializing winsock: wrong version");
			WSACleanup( );
			return;
		}
	}
	++numSockets;
#endif
	
	mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // create socket
	if (mySocket == INVALID_SOCKET )
	{
		throw network_error(std::string("Error initializing socket: ") + GetErrorMsg());
	}
	
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Let the OS assign a address	
	myAddr.sin_port = htons(port);	   // Use port passed from user

	if (bind(mySocket,(struct sockaddr *)&myAddr,sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		throw network_error(std::string("Error binding socket: ") + GetErrorMsg());
	}
	
	// dont set PATH_MTU yet
	// int option = IP_PMTUDISC_DO;
	// setsockopt(mySocket, IPPROTO_IP, IP_MTU_DISCOVER, &option, sizeof(option));
#ifdef _WIN32
	u_long u=1;
	if (ioctlsocket(mySocket,FIONBIO,&u) == SOCKET_ERROR)
#else
	if (fcntl(mySocket, F_SETFL, O_NONBLOCK) == -1)
#endif
	{
		throw network_error(std::string("Error configuring socket: ") + GetErrorMsg());
	}
}

UDPSocket::~UDPSocket()
{
	close(mySocket);
#ifdef _WIN32
	--numSockets;
	if (numSockets == 0)
		WSACleanup();
#endif
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

sockaddr_in UDPSocket::ResolveHost(const char* const address, const unsigned port) const
{
	sockaddr_in remoteAddr;

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(port);

#ifdef _WIN32
	unsigned long ul;
	if((ul=inet_addr(address)) != INADDR_NONE)
	{
		remoteAddr.sin_addr.S_un.S_addr = ul;
	}
	else
#else
		if (inet_aton(address,&(remoteAddr.sin_addr)) == 0)
#endif
	{
		LPHOSTENT lpHostEntry;
		lpHostEntry = gethostbyname(address);
		if (lpHostEntry == NULL)
		{
			throw network_error(std::string("Error looking up server from DNS: ")+std::string(address));
		}
		remoteAddr.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
	}
	return remoteAddr;
}

std::string UDPSocket::GetErrorMsg() const
{
#ifdef _WIN32
	return strerror(WSAGetLastError());
#else
	return strerror(errno);
#endif
}

bool UDPSocket::IsFakeError() const
{
#ifdef _WIN32
	int err=WSAGetLastError();
	return err==WSAEWOULDBLOCK || err==WSAECONNRESET || err==WSAEINTR;
#else
	return errno==EWOULDBLOCK || errno==ECONNRESET || errno==EINTR;
#endif
}

} // namespace netcode

 
