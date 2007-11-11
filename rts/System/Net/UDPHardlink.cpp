#include "UDPHardlink.h"

#include <string>

#ifdef _WIN32
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "Exception.h"

namespace netcode
{

#ifdef _WIN32
#else
	typedef struct hostent* LPHOSTENT;
	typedef struct in_addr* LPIN_ADDR;
	const int SOCKET_ERROR = -1;
#endif

UDPHardlink::UDPHardlink(const char* const server, const unsigned remoteport, const int port, const unsigned range)
: UDPSocket(port,range)
{
	LPHOSTENT lpHostEntry;
	sockaddr_in remoteAddr;

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(remoteport);

#ifdef _WIN32
	unsigned long ul;
	if((ul=inet_addr(server))!=INADDR_NONE){
		remoteAddr.sin_addr.S_un.S_addr = ul;
	} else
#else
		if(inet_aton(server,&(remoteAddr.sin_addr))==0)
#endif
		{
			lpHostEntry = gethostbyname(server);
			if (lpHostEntry == NULL)
			{
				throw network_error(std::string("Error looking up server from DNS: ")+std::string(server));
			}
			remoteAddr.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
		}
		connect(mySocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr));
}


void UDPHardlink::Send(const unsigned char* const buf, const unsigned dataLength) const
{
	send(mySocket, (char*)buf, dataLength, 0);
}

unsigned UDPHardlink::Recv(unsigned char* buf, const unsigned bufLength) const
{
	const int data = recv(mySocket,(char*)buf,bufLength,0);
	return data;
}


} // namespace netcode
 
