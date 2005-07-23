#include "StdAfx.h"
// Net.cpp: implementation of the CNet class.
//
//////////////////////////////////////////////////////////////////////

#include "Net.h"
#include "stdio.h"
#include "FileHandler.h"
#include "InfoConsole.h"
#include "Game.h"
#include "ScriptHandler.h"
#include "GameSetup.h"
//#include "mmgr.h"
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define NETWORK_VERSION 1

CNet* net=0;
CNet* serverNet=0;
extern std::string stupidGlobalMapname;
extern int stupidGlobalMapId;

unsigned char netbuf[NETWORK_BUFFER_SIZE];	//buffer space for outgoing data

CNet::CNet()
{
	LARGE_INTEGER t,f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
#ifdef _WIN32
	curTime=double(t.QuadPart)/double(f.QuadPart);
#else
	curTime=double(t)/double(f);
#endif
	WORD wVersionRequested;
#ifdef _WIN32
	WSADATA wsaData;
	int err; 

	wVersionRequested = MAKEWORD( 2, 2 ); 
	err = WSAStartup( wVersionRequested, &wsaData );if ( err != 0 ) {
		MessageBox(NULL,"Couldnt initialize winsock.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		return;
	} 
	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */ 
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {
		MessageBox(NULL,"Wrong WSA version.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		WSACleanup( );
		return; 
	}
#endif
	connected=false;

	for(int a=0;a<MAX_PLAYERS;a++){
		connections[a].active=false;
		connections[a].localConnection=0;
	}

	imServer=false;
	onlyLocal=false;
	inInitialConnect=false;

	recordDemo=0;
	playbackDemo=0;
	mySocket=0;

	netMutex=CreateMutex(0,false,"SpringNetLocalBufferMutex");
}

CNet::~CNet()
{
#ifndef NO_NET
	if(connected && (imServer || !connections[0].localConnection)){
		unsigned char t=NETMSG_QUIT;
		SendData(&t,1);
		FlushNet();
	}
	if(mySocket)
#ifdef _WIN32
		closesocket(mySocket);
	WSACleanup( );
#else
		close(mySocket);
#endif


	delete recordDemo;
	delete playbackDemo;

	for (int a=0;a<MAX_PLAYERS;a++){
		Connection* c=&connections[a];
		if(!c->active)
			continue;
		std::deque<Packet*>::iterator pi;
		for(pi=c->unackedPackets.begin();pi!=c->unackedPackets.end();++pi)
			delete (*pi);
		std::map<int,Packet*>::iterator pi2;
		for(pi2=c->waitingPackets.begin();pi2!=c->waitingPackets.end();++pi2)
			delete (pi2->second);
	}
#endif //NO_NET
	CloseHandle(netMutex);
}

int CNet::InitServer(int portnum)
{
#ifndef NO_NET
	connected=true;
	waitOnCon=true;
	imServer=true;

	if ((mySocket= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET ){ /* create socket */
		MessageBox(NULL,"Error initializing socket.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		connected=false;
		exit(0);
		return -1;
	}

	sockaddr_in saMe;

	saMe.sin_family = AF_INET;
	saMe.sin_addr.s_addr = INADDR_ANY; // Let WinSock assign address
	saMe.sin_port = htons(portnum);	   // Use port passed from user

	if (bind(mySocket,(struct sockaddr *)&saMe,sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
#ifdef _WIN32
		closesocket(mySocket);
#else
		close(mySocket);
#endif
		MessageBox(NULL,"Error binding socket.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		connected=false;
		exit(0);
		return -1;
	}

#ifdef _WIN32
	u_long u=1;
	ioctlsocket(mySocket,FIONBIO,&u);
#else
	fcntl(mySocket, F_SETFL, O_NONBLOCK);
#endif
#endif //NO_NET
	return 0;
}

void CNet::StopListening()
{
	waitOnCon=false;
}

int CNet::InitClient(const char *server, int portnum,bool localConnect)
{
#ifndef NO_NET
  LPHOSTENT lpHostEntry;

	LARGE_INTEGER t,f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
#ifndef NO_WINSTUFF
	curTime=double(t.QuadPart)/f.QuadPart;
#else
	curTime = double(t)/double(f);
#endif

	if(FindDemoFile(server)){
		onlyLocal=true;
		connections[0].active=true;
		connections[0].readyLength=0;
		connected=true;
		gu->spectating=true;
		return 1;
	} else {
		CreateDemoFile("test.sdf");
	}

	sockaddr_in saOther;

	saOther.sin_family = AF_INET;
	saOther.sin_port = htons(portnum);

#ifdef _WIN32
	unsigned long ul;
	if((ul=inet_addr(server))!=INADDR_NONE){
		saOther.sin_addr.S_un.S_addr = 	ul;
	} else {
#else /* } */
	if(inet_aton(server,&(saOther.sin_addr))==0) {
#endif
		lpHostEntry = gethostbyname(server);
		if (lpHostEntry == NULL)
		{
			MessageBox(NULL,"Error looking up server from dns.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
			exit(0);
			return -1;
		}
		saOther.sin_addr = 	*((LPIN_ADDR)*lpHostEntry->h_addr_list);	
	}

	if ((mySocket= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET ){ /* create socket */
		MessageBox(NULL,"Error initializing socket.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		exit(0);
		return -1;
	}


	u_long u=1;
#ifdef _WIN32
	ioctlsocket(mySocket,FIONBIO,&u);
#else
	fcntl(mySocket, F_SETFL, O_NONBLOCK);
#endif

	waitOnCon=false;
	imServer=false;

	InitNewConn(&saOther,false,0);
	if(!localConnect){
		netbuf[0]=NETMSG_ATTEMPTCONNECT;
		if(gameSetup)
			netbuf[1]=gameSetup->myPlayer;
		else
			netbuf[1]=0;
		netbuf[2]=NETWORK_VERSION;
		SendData(netbuf,3);
		FlushNet();
		inInitialConnect=true;
	}
	return 1;
#endif
}

int CNet::SendData(unsigned char *data, int length)
{
	int ret=1;			//becomes 0 if any connection return 0
	for (int a=0;a<MAX_PLAYERS;a++){
		Connection* c=&connections[a];
		if(c->active){
			ret&=SendData(data,length,a);
		}
	}
	return ret;
}

int CNet::SendData(unsigned char *data, int length,int connection)
{
	if(playbackDemo)
		return 1;
	if(length<=0)
		MessageBox(0,"errenous length in send","network",0);

	Connection* c=&connections[connection];
	if(c->active){
		if(c->localConnection){
			WaitForSingleObject(netMutex,INFINITE);
			Connection* lc=c->localConnection;
			if(lc->readyLength+length>=NETWORK_BUFFER_SIZE){
				info->AddLine("Overflow when sending to local connection %i %i %i %i %i",imServer,connection,lc->readyLength,length,NETWORK_BUFFER_SIZE);
				ReleaseMutex(netMutex);
				return 0;
			}
			memcpy(&lc->readyData[lc->readyLength],data,length);
			lc->readyLength+=length;
			ReleaseMutex(netMutex);
		} else {
			if(c->outgoingLength+length>=NETWORK_BUFFER_SIZE){
				info->AddLine("Overflow when sending to remote connection %i %i %i %i %i",imServer,connection,c->outgoingLength,length,NETWORK_BUFFER_SIZE);
				return 0;
			}
			memcpy(&c->outgoingData[c->outgoingLength],data,length);
			c->outgoingLength+=length;
		}
	}
	return 1;
}

int CNet::GetData(unsigned char *buf, int length,int conNum)
{
	if(connections[conNum].active){
		if(connections[conNum].localConnection)
			WaitForSingleObject(netMutex,INFINITE);

		int ret=connections[conNum].readyLength;
		if(length<=ret){
			if(connections[conNum].localConnection)
				ReleaseMutex(netMutex);
			return 0;
		}
		memcpy(buf,connections[conNum].readyData,connections[conNum].readyLength);
		connections[conNum].readyLength=0;

		if(recordDemo && ret>0)
			SaveToDemo(buf,ret);

		if(connections[conNum].localConnection)
			ReleaseMutex(netMutex);
		return ret;
	} else {
		return -1;
	}
}

void CNet::Update(void)
{
	LARGE_INTEGER t,f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
#ifdef _WIN32
	curTime=double(t.QuadPart)/double(f.QuadPart);
#else
	curTime=double(t)/double(f);
#endif
	if(onlyLocal){
		if(playbackDemo)
			ReadDemoFile();
		return;
	}
	sockaddr_in from;
#ifdef _WIN32
	int fromsize;
#else
	socklen_t fromsize;
#endif
	fromsize=sizeof(from);
	int r;
	unsigned char inbuf[16000];
	if(connected)
	while(true){
#ifndef NO_NET
		if((r=recvfrom(mySocket,(char*)inbuf,16000,0,(sockaddr*)&from,&fromsize))==SOCKET_ERROR){
#ifdef _WIN32
			if(WSAGetLastError()==WSAEWOULDBLOCK || WSAGetLastError()==WSAECONNRESET) 
#else
			if(errno==EWOULDBLOCK||errno==ECONNRESET)
#endif
				break;
			char test[500];
#ifdef _WIN32
			sprintf(test,"Error receiving data. %i %d",(int)imServer,WSAGetLastError());
#else
			sprintf(test,"Error receiving data. %i %d",(int)imServer,errno);
#endif
			MessageBox(NULL,test,"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
			exit(0);
		}
#endif
		int conn=ResolveConnection(&from);
		if(conn==-1){
			if(waitOnCon && r>=12 && (*(int*)inbuf)==0 && (*(int*)&inbuf[4])==-1 && inbuf[8]==0 && inbuf[9]==NETMSG_ATTEMPTCONNECT && inbuf[11]==NETWORK_VERSION){
				conn=InitNewConn(&from,false,inbuf[10]);
			} else {
				continue;
			}
		}
		inInitialConnect=false;
		int packetNum=(*(int*)inbuf);
		ProcessRawPacket(inbuf,r,conn);
	}

	for(int a=0;a<MAX_PLAYERS;++a){
		Connection* c=&connections[a];
		if(c->localConnection || !c->active)
			continue;
		std::map<int,Packet*>::iterator wpi;
		while((wpi=c->waitingPackets.find(c->lastInOrder+1))!=c->waitingPackets.end()){		//process all in order packets that we have waiting
			memcpy(&c->readyData[c->readyLength],wpi->second->data,wpi->second->length);
			c->readyLength+=wpi->second->length;
			delete wpi->second;
			c->waitingPackets.erase(wpi);
			c->lastInOrder++;
		}
		if(inInitialConnect && c->lastSendTime<curTime-1){		//server hasnt responded so try to send the connection attempt again
			SendRawPacket(a,c->unackedPackets[0]->data,c->unackedPackets[0]->length,0);
		}

		if(c->lastSendTime<curTime-5 && !c->localConnection && !inInitialConnect){		//we havent sent anything for a while so send something to prevent timeout
			netbuf[0]=NETMSG_HELLO;
			SendData(netbuf,1);
		}
		if(c->lastSendTime<curTime-0.2 && !c->waitingPackets.empty()){	//we have at least one missing incomming packet lying around so send a packet to ensure the other side get a nak
			netbuf[0]=NETMSG_HELLO;
			SendData(netbuf,1);
		}
		if(c->lastReceiveTime < curTime-(inInitialConnect ? 40 : 30) && !c->localConnection){		//other side has timed out
			c->active=false;
		}

		if(c->outgoingLength>0 && (c->lastSendTime < (curTime-0.2+c->outgoingLength*0.01) || c->lastSendFrame < gs->frameNum-1)){
			FlushConnection(a);
		}
	}
}

void CNet::ProcessRawPacket(unsigned char* data, int length, int conn)
{
	Connection* c=&connections[conn];
	c->lastReceiveTime=curTime;

	int packetNum=*(int*)data;
	int ack=*(int*)&data[4];
	int hsize=9;

	while(ack>=c->firstUnacked){
		delete c->unackedPackets.front();
		c->unackedPackets.pop_front();
		c->firstUnacked++;
	}
//	if(!imServer)
//		info->AddLine("Got packet %i %i %i %i %i %i",(int)imServer,packetNum,length,ack,c->lastInOrder,c->waitingPackets.size());
	if(data[8]==1){
		hsize=13;
		int nak=*(int*)&data[9];
//		info->AddLine("Got nak %i %i %i",nak,c->lastNak,c->firstUnacked);
		if(nak!=c->lastNak || c->lastNakTime < curTime-0.1){
			c->lastNak=nak;
			c->lastNakTime=curTime;
			for(int b=c->firstUnacked;b<=nak;++b){
				SendRawPacket(conn,c->unackedPackets[b-c->firstUnacked]->data,c->unackedPackets[b-c->firstUnacked]->length,b);
			}
		}
	}

	if(!c->active || c->lastInOrder>=packetNum || c->waitingPackets.find(packetNum)!=c->waitingPackets.end())
		return;

	Packet* p=new Packet(data+hsize,length-hsize);
	c->waitingPackets[packetNum]=p;
}

int CNet::ResolveConnection(sockaddr_in* from)
{
#ifndef NO_NET
	unsigned int addr;
#ifdef _WIN32
	addr = from->sin_addr.S_un.S_addr;
#else
	addr = from->sin_addr.s_addr;
#endif
	for(int a=0;a<MAX_PLAYERS;++a){
		if(connections[a].active){
			if(
#ifdef _WIN32
			    connections[a].addr.sin_addr.S_un.S_addr==addr 
#else
			    connections[a].addr.sin_addr.s_addr==addr 
#endif
			    && connections[a].addr.sin_port==from->sin_port){
				return a;
			}
		}
	}
	return -1;
#endif
}

int CNet::InitNewConn(sockaddr_in* other,bool localConnect,int wantedNumber)
{
	int freeConn=0;
	if(wantedNumber){
		if(wantedNumber<0 || wantedNumber>=MAX_PLAYERS){
			info->AddLine("Warning attempt to connect to errenous connection number");
			wantedNumber=0;		
		}
		if(connections[wantedNumber].active){
			info->AddLine("Warning attempt to connect to already active connection number");
			wantedNumber=0;
		}
		freeConn=wantedNumber;
	}
	if(wantedNumber==0){
		for(freeConn=0;freeConn<MAX_PLAYERS;++freeConn){
			if(!connections[freeConn].active)
				break;
		}
	}

	gs->players[freeConn]->active=true;
	connections[freeConn].active=true;
	connections[freeConn].addr=*other;
	connections[freeConn].lastInOrder=-1;
	connections[freeConn].readyLength=0;
	connections[freeConn].waitingPackets.clear();
	connections[freeConn].firstUnacked=0;
	connections[freeConn].currentNum=0;
	connections[freeConn].outgoingLength=0;
	connections[freeConn].lastReceiveTime=curTime;
	connections[freeConn].lastSendFrame=0;
	connections[freeConn].lastSendTime=0;
	connections[freeConn].lastNak=-1;
	connections[freeConn].lastNakTime=0;

	connected=true;

	if(imServer && !localConnect){
		netbuf[0]=NETMSG_SETPLAYERNUM;
		netbuf[1]=freeConn;
		SendData(netbuf,2,freeConn);

		netbuf[0]=NETMSG_SCRIPT;
		netbuf[1]=CScriptHandler::Instance()->chosenName.size()+3;
		for(unsigned int b=0;b<CScriptHandler::Instance()->chosenName.size();b++)
			netbuf[b+2]=CScriptHandler::Instance()->chosenName.at(b);
		netbuf[CScriptHandler::Instance()->chosenName.size()+2]=0;
		SendData(netbuf,CScriptHandler::Instance()->chosenName.size()+3);

		netbuf[0]=NETMSG_MAPNAME;
		netbuf[1]=stupidGlobalMapname.size()+7;
		*(int*)&netbuf[2]=stupidGlobalMapId;
		for(unsigned int a=0;a<stupidGlobalMapname.size();a++)
			netbuf[a+6]=stupidGlobalMapname.at(a);
		netbuf[stupidGlobalMapname.size()+6]=0;
		SendData(netbuf,stupidGlobalMapname.size()+7);

		for(int a=0;a<MAX_PLAYERS;a++){
			if(!gs->players[a]->readyToStart)
				continue;
			netbuf[0]=NETMSG_PLAYERNAME;
			netbuf[1]=gs->players[a]->playerName.size()+4;
			netbuf[2]=a;
			for(unsigned int b=0;b<gs->players[a]->playerName.size();b++)
				netbuf[b+3]=gs->players[a]->playerName.at(b);
			netbuf[gs->players[a]->playerName.size()+3]=0;
			SendData(netbuf,gs->players[a]->playerName.size()+4);
		}
		FlushNet();
	}
	return freeConn;
}

void CNet::FlushNet(void)
{
	for (int a=0;a<MAX_PLAYERS;a++){
		Connection* c=&connections[a];
		if(c->active && c->outgoingLength>0){
			FlushConnection(a);
		}
	}
}

void CNet::FlushConnection(int conn)
{
	Connection* c=&connections[conn];
	c->lastSendFrame=gs->frameNum;
	c->lastSendTime=curTime;

	SendRawPacket(conn,c->outgoingData,c->outgoingLength,c->currentNum++);
	Packet* p=new Packet(c->outgoingData,c->outgoingLength);
	c->outgoingLength=0;
	c->unackedPackets.push_back(p);
}

void CNet::SendRawPacket(int conn, unsigned char* data, int length, int packetNum)
{
	Connection* c=&connections[conn];

	*(int*)&netbuf[0]=packetNum;
	*(int*)&netbuf[4]=c->lastInOrder;
	int hsize=9;
	if(!c->waitingPackets.empty() && c->waitingPackets.find(c->lastInOrder+1)==c->waitingPackets.end()){
		netbuf[8]=1;
		*(int*)&netbuf[9]=c->waitingPackets.begin()->first-1;
		hsize=13;
	} else {
		netbuf[8]=0;
	}

	memcpy(&netbuf[hsize],data,length);
//	if(rand()&7)
#ifndef NO_NET
	if(sendto(mySocket,(char*)netbuf,length+hsize,0,(sockaddr*)&c->addr,sizeof(c->addr))==SOCKET_ERROR){
#ifdef _WIN32
		if(WSAGetLastError()==WSAEWOULDBLOCK)
#else
		if(errno==EWOULDBLOCK)
#endif
			return;
		char test[100];
#ifdef _WIN32
		sprintf(test,"Error sending data. %d",WSAGetLastError());
#else
		sprintf(test,"Error sending data. %d",errno);
#endif
		MessageBox(NULL,test,"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		exit(0);
	}
#endif
}

void CNet::CreateDemoFile(char* name)
{
	recordDemo=new ofstream(name, ios::out|ios::binary);
	if(gameSetup){
		char c=1;
		recordDemo->write(&c,1);
		recordDemo->write((char*)&gameSetup->gameSetupTextLength,sizeof(int));
		recordDemo->write(gameSetup->gameSetupText,gameSetup->gameSetupTextLength);
	} else {
		char c=0;
		recordDemo->write(&c,1);
	}
}

void CNet::SaveToDemo(unsigned char* buf,int length)
{
	recordDemo->write((char*)&gu->modGameTime,sizeof(double));
	recordDemo->write((char*)&length,sizeof(int));
	recordDemo->write((char*)buf,length);
	recordDemo->flush();
}

bool CNet::FindDemoFile(const char* name)
{
	playbackDemo=new CFileHandler(name);
	if(playbackDemo->FileExists()){
		info->AddLine("Playing demo from %s",name);
		char c;
		playbackDemo->Read(&c,1);
		if(c){
			int length;
			playbackDemo->Read(&length,sizeof(int));
			char* buf=new char[length];
			playbackDemo->Read(buf,length);

			gameSetup=new CGameSetup();
			gameSetup->Init(buf,length);
			delete[] buf;
		}
		playbackDemo->Read(&demoTimeOffset,sizeof(double));
		demoTimeOffset=gu->modGameTime-demoTimeOffset;
		nextDemoRead=gu->modGameTime-0.01;
		gs->cheatEnabled=true;	//allow switching between teams etc
		return true;
	} else {
		delete playbackDemo;
		playbackDemo=0;
		return false;
	}
}

void CNet::ReadDemoFile(void)
{
	while(connections[0].readyLength<500 && nextDemoRead<gu->modGameTime/**/){
		if(playbackDemo->Eof()){
			if(connections[0].readyLength<30 && game->inbuflength-game->inbufpos<30)
				connections[0].active=false;
			return;
		}
		int l;
		playbackDemo->Read(&l,sizeof(int));
		playbackDemo->Read(&connections[0].readyData[connections[0].readyLength],l);
		connections[0].readyLength+=l;
		playbackDemo->Read(&nextDemoRead,sizeof(double));
		nextDemoRead+=demoTimeOffset;
		if(playbackDemo->Eof()){
			info->AddLine("End of demo");
			nextDemoRead=gu->modGameTime+4*gs->speedFactor;
		}
//		info->AddLine("Read packet length %i ready %i time %.0f",l,connections[0].readyLength,nextDemoRead);
	}
}

