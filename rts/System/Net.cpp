//
// Net.cpp: implementation of the CNet class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef _WIN32
#include "Platform/Win/win32.h"
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "Net.h"
#include "LogOutput.h"
#include "Game/GameVersion.h"
#include "RemoteConnection.h"
#include "LocalConnection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace netcode {

std::string GetErrorMsg()
{
#ifdef _WIN32
	return strerror(WSAGetLastError());	// does windows have such a function?
#else
	return strerror(errno);
#endif
}

#ifdef _WIN32
typedef int socklen_t;
#else
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
typedef struct hostent* LPHOSTENT;
typedef struct in_addr* LPIN_ADDR;
#define closesocket(x) close(x)
#endif

bool IsFakeError()
{
#ifdef _WIN32
	int err=WSAGetLastError();
	return err==WSAEWOULDBLOCK || err==WSAECONNRESET || err==WSAEINTR;
#else
	return errno==EWOULDBLOCK || errno==ECONNRESET || errno==EINTR;
#endif
}

CNet::CNet()
{
#ifdef _WIN32
	unsigned short wVersionRequested;
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
	connected=false;

	for(int a=0;a<MAX_PLAYERS;a++){
		connections[a]=0;
	}

	imServer=false;
	inInitialConnect=false;
	onlyLocal = false;

	mySocket=0;
}

CNet::~CNet()
{
	if(mySocket)
		closesocket(mySocket);
#ifdef _WIN32
	WSACleanup();
#endif

	for (int a=0;a<MAX_PLAYERS;a++){
		if(connections[a])
			delete connections[a];
	}
}

void CNet::StopListening()
{
	waitOnCon=false;
}

void CNet::Kill(const unsigned connNumber)
{
	connections[connNumber]->Flush();
	connections[connNumber]->active = false;
}

void CNet::PingAll()
{
	for(unsigned a=0;a<MAX_PLAYERS;++a){
		if (connections[a] && connections[a]->active)
			connections[a]->Ping();
	}
}

int CNet::GetData(unsigned char *buf, const unsigned length, const unsigned conNum)
{
	if (!connections[conNum])
	{
		logOutput.Print("Someone tried to read data from empty connection %i", conNum);
		return -1;
	}
	return connections[conNum]->GetData(buf,length);
}

int CNet::InitServer(unsigned portnum)
{
	waitOnCon=true;
	imServer=true;

	if ((mySocket= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET ){ /* create socket */
		throw network_error("Error initializing socket as server: "+GetErrorMsg());
		exit(0);
	}

	connected = true;

	sockaddr_in saMe;

	saMe.sin_family = AF_INET;
	saMe.sin_addr.s_addr = INADDR_ANY; // Let WinSock assign address
	saMe.sin_port = htons(portnum);	   // Use port passed from user

	if (bind(mySocket,(struct sockaddr *)&saMe,sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		closesocket(mySocket);
		throw network_error("Error binding socket as server: "+GetErrorMsg());
		exit(0);
	}

#ifdef _WIN32
		u_long u=1;
		ioctlsocket(mySocket,FIONBIO,&u);
#else
		fcntl(mySocket, F_SETFL, O_NONBLOCK);
#endif

	return 0;
}

int CNet::InitClient(const char *server, unsigned portnum,unsigned sourceport, unsigned playerNum)
{
	LPHOSTENT lpHostEntry;

	Pending WannaBeClient;
	WannaBeClient.wantedNumber = playerNum;
	WannaBeClient.other.sin_family = AF_INET;
	WannaBeClient.other.sin_port = htons(portnum);

#ifdef _WIN32
	unsigned long ul;
	if((ul=inet_addr(server))!=INADDR_NONE){
		WannaBeClient.other.sin_addr.S_un.S_addr = 	ul;
	} else
#else
	if(inet_aton(server,&(WannaBeClient.other.sin_addr))==0)
#endif
	{
		lpHostEntry = gethostbyname(server);
		if (lpHostEntry == NULL)
		{
			throw network_error("Error looking up server from DNS: "+std::string(server));
		}
		WannaBeClient.other.sin_addr = 	*((LPIN_ADDR)*lpHostEntry->h_addr_list);
	}

	sockaddr_in saMe;
	saMe.sin_family = AF_INET;
	saMe.sin_addr.s_addr = INADDR_ANY; // Let WinSock assign address
	saMe.sin_port = htons(sourceport);	   // Use port passed from user

	if ((mySocket= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET ){ /* create socket */
		throw network_error("Error initializing socket "+GetErrorMsg());
	}

	unsigned numTries=0;
	while (bind(mySocket,(struct sockaddr *)&saMe,sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		numTries++;
		if(numTries>10){
			closesocket(mySocket);
			throw network_error("Error binding socket as client: "+GetErrorMsg());
		}
		saMe.sin_port = htons(sourceport+numTries);
	}

#ifdef _WIN32
	u_long u=1;
	ioctlsocket(mySocket,FIONBIO,&u);
#else
	fcntl(mySocket, F_SETFL, O_NONBLOCK);
#endif

	waitOnCon=false;
	imServer=false;
	connected = true;
	onlyLocal = false;

	InitNewConn(WannaBeClient);

	return 1;
}

int CNet::InitLocalClient(const unsigned wantedNumber)
{
	waitOnCon=false;
	imServer=false;
	onlyLocal = true;

	Pending temp;
	temp.wantedNumber = wantedNumber;
	InitNewConn(temp, true);

	return 1;
}

int CNet::InitNewConn(const Pending& NewClient, bool local)
{
	unsigned freeConn = 0;
	if(NewClient.wantedNumber){
		if(NewClient.wantedNumber>=MAX_PLAYERS){
			logOutput.Print("Warning attempt to connect to errenous connection number");
		}
		else if(connections[NewClient.wantedNumber] && connections[NewClient.wantedNumber]->active){
			logOutput.Print("Warning attempt to connect to already active connection number");
		}
		else
			freeConn = NewClient.wantedNumber;
	}

	if (freeConn == 0)
		for(freeConn=0;freeConn<MAX_PLAYERS;++freeConn){
		if(!connections[freeConn])
			break;
		}

	if (!local)
		connections[freeConn]= new CRemoteConnection(NewClient.other, &mySocket);
	else
		connections[freeConn]= new CLocalConnection();

	connected=true;

	return freeConn;
}

int CNet::SendData(const unsigned char *data, const unsigned length)
{
	if(length<=0)
		logOutput.Print("Errenous send length in SendData %i",length);

	unsigned ret=1;			//becomes 0 if any connection return 0
	for (unsigned a=0;a<MAX_PLAYERS;a++){	//improve: dont check every connection
		CConnection* c=connections[a];
		if(c){
			ret&=c->SendData(data,length);
		}
	}
	return ret;
}

void CNet::Update(void)
{
	if(!onlyLocal && connected) {
		sockaddr_in from;
		socklen_t fromsize;
		fromsize=sizeof(from);
		int r;
		unsigned char inbuf[16000];

		while(true)
		{
			if((r=recvfrom(mySocket,(char*)inbuf,16000,0,(sockaddr*)&from,&fromsize))==SOCKET_ERROR)
			{
				if (IsFakeError())
					break;
				throw network_error("Error receiving data: "+GetErrorMsg());
			}
			int conn=ResolveConnection(&from);
			if(conn>= 0)
			{
				inInitialConnect=false;
				connections[conn]->ProcessRawPacket(inbuf,r);
			}
			else
			{
				int packetNum=*(unsigned int*)inbuf;
				// int ack=*(int*)(inbuf+4);
				// unsigned char nak = *(int*)(inbuf+8);

				if(waitOnCon && r>=12 && packetNum ==0)
				{
					Pending newConn;
					newConn.other = from;
					newConn.netmsg = inbuf[9];
					newConn.networkVersion = inbuf[11];
					newConn.wantedNumber = inbuf[10];
					justConnected.push_back(newConn);
				}
			}
		}
	}

	for(unsigned a=0;a<MAX_PLAYERS;++a){
		if (connections[a] != 0)
			connections[a]->Update(inInitialConnect);
	}
}

void CNet::FlushNet(void)
{
	for (int a=0;a<MAX_PLAYERS;a++){
		CConnection* c=connections[a];
		if(c){
			c->Flush();
		}
	}
}

int CNet::ResolveConnection(const sockaddr_in* from) const
{
	for(int a=0;a<MAX_PLAYERS;++a){
		if(connections[a] && connections[a]->active){
			if(connections[a]->CheckAddress(*from)){
				return a;
			}
		}
	}
	return -1;
}


} // namespace netcode


