#include "UDPSocket.h" 

#ifdef _WIN32
#else
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace netcode {


#ifdef _WIN32
	typedef int socklen_t;
#define close(x) closesocket(x)
#else
	const int INVALID_SOCKET = -1;
	const int SOCKET_ERROR = -1;
#endif


UDPSocket::UDPSocket(int port, unsigned range)
{
#ifdef _WIN32
	Uint16 wVersionRequested;
	WSADATA wsaData;
	int err;
		
	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &wsaData );if ( err != 0 ) {
		throw network_error("Couldn't initialize winsock");
		return;
	}
	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) {
		throw network_error("Wrong WSA version");
		WSACleanup( );
		return;
	}
#endif
	
	if ((mySocket= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET ){ /* create socket */
		throw network_error("Error initializing socket: "+GetErrorMsg());
		exit(0);
	}
	
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = INADDR_ANY; // Let the OS assign a address
	
	do
	{
		myAddr.sin_port = htons(port);	   // Use port passed from user		
		if (bind(mySocket,(struct sockaddr *)&myAddr,sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			if (range == 0)
			{
				throw network_error("Error binding socket: "+GetErrorMsg());	// last try, error will break through to higher levels
			}
			--range;
			++port;
		}
		else 
			break;
	} while (range > 0);
	
	// dont set PATH_MTU yet
	// int option = IP_PMTUDISC_DO;
	// setsockopt(mySocket, IPPROTO_IP, IP_MTU_DISCOVER, &option, sizeof(option));
#ifdef _WIN32
	u_long u=1;
	ioctlsocket(mySocket,FIONBIO,&u);
#else
	fcntl(mySocket, F_SETFL, O_NONBLOCK);
#endif
}

UDPSocket::~UDPSocket()
{
	close(mySocket);
#ifdef _WIN32
	WSACleanup();
#endif
}

unsigned UDPSocket::RecvFrom(unsigned char* buf, const unsigned bufLength, sockaddr_in* fromAddress)
{
	socklen_t fromsize = sizeof(*fromAddress);
	const int data =  recvfrom(mySocket,buf,bufLength,0,(sockaddr*)fromAddress,&fromsize);
	if (data == SOCKET_ERROR)
	{
		if (IsFakeError())
			return 0;
		else
			throw network_error("Error receiving data from socket: "+GetErrorMsg());
	}

	return data;
}

void UDPSocket::SendTo(const unsigned char* const buf, const unsigned dataLength, const sockaddr_in* const destination)
{
	const int error = sendto(mySocket,(const void* const)buf,dataLength,0,(const struct sockaddr* const)destination,sizeof(*destination));
	if(error==SOCKET_ERROR){
		if (IsFakeError())
			return;
		else
			throw network_error("Error sending data to socket: "+GetErrorMsg());
	}
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

 
