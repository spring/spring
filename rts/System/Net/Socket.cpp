#include "Socket.h"

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "Exception.h"

namespace netcode
{

#ifdef _WIN32
	unsigned Socket::numSockets = 0;
#else
	typedef struct hostent* LPHOSTENT;
	typedef struct in_addr* LPIN_ADDR;
	const int INVALID_SOCKET = -1;
	const int SOCKET_ERROR = -1;
#endif

Socket::Socket(const SocketType type)
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
	
	if (type == DATAGRAM)
		mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // create UDP socket
	else if (type == STREAM)
		mySocket = socket(AF_INET, SOCK_STREAM, 0); // create TCP socket

	if (mySocket == INVALID_SOCKET)
		throw network_error("Error in creating socket: " + GetErrorMsg());
}

Socket::~Socket()
{
#ifndef _WIN32
	close(mySocket);
#else
	closesocket(mySocket);
	--numSockets;
	if (numSockets == 0)
		WSACleanup();
#endif
}

void Socket::SetBlocking(const bool block) const
{
#ifdef _WIN32
	u_long u = block ? 0 : 1;
	if (ioctlsocket(mySocket,FIONBIO,&u) == SOCKET_ERROR)
#else
	if (fcntl(mySocket, F_SETFL, block ? 0 : O_NONBLOCK) == -1)
#endif
	{
		throw network_error(std::string("Error setting socket I/O mode: ") + GetErrorMsg());
	}
}

bool Socket::HasIncomingData(int timeout) const
{
#ifndef _WIN32
	// linux has poll() which is faster and easier to use than select()
	pollfd pd;
	pd.fd = mySocket;
	pd.events = POLLIN | POLLPRI;
	const int ret = poll(&pd, 1, timeout);
#else
	// Windows only provides poll() in Vista, so use select() here
	fd_set pd;
	FD_ZERO(&pd);
	FD_SET(mySocket, &pd);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = timeout;
	const int ret = select(mySocket+1, &pd, 0, 0, &tv);
#endif
	
	if (ret > 0)
		return true;
	else if (ret == 0)
		return false;
	else
		throw network_error(std::string("Poll for data failed: ") + GetErrorMsg());
}

void Socket::Bind(unsigned short port) const
{
	sockaddr_in myAddr;
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Let the OS assign a address	
	myAddr.sin_port = htons(port);	   // Use port passed from user

	if (bind(mySocket,(struct sockaddr *)&myAddr,sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		throw network_error(std::string("Error binding socket: ") + GetErrorMsg());
	}
}

sockaddr_in Socket::ResolveHost(const std::string& address, const unsigned port) const
{
	sockaddr_in remoteAddr;

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(port);

#ifdef _WIN32
	unsigned long ul;
	if((ul=inet_addr(address.c_str())) != INADDR_NONE)
	{
		remoteAddr.sin_addr.S_un.S_addr = ul;
	}
	else
#else
	if (inet_aton(address.c_str(),&(remoteAddr.sin_addr)) == 0)
#endif
	{
		LPHOSTENT lpHostEntry;
		lpHostEntry = gethostbyname(address.c_str());
		if (lpHostEntry == NULL)
		{
			throw network_error(std::string("Error looking up server from DNS: ")+address);
		}
		remoteAddr.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
	}
	return remoteAddr;
}

std::string Socket::GetErrorMsg() const
{
#ifdef _WIN32
	return strerror(WSAGetLastError());
#else
	return strerror(errno);
#endif
}

bool Socket::IsFakeError() const
{
#ifdef _WIN32
	int err=WSAGetLastError();
	return err==WSAEWOULDBLOCK || err==WSAECONNRESET || err==WSAEINTR;
#else
	return errno==EWOULDBLOCK || errno==ECONNRESET || errno==EINTR;
#endif
}

} // namespace netcode

