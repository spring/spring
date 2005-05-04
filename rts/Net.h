// Net.h: interface for the CNet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NET_H__A0712941_2020_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_NET_H__A0712941_2020_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <deque>
#include <map>

class CFileHandler;
using namespace std;

#define NETMSG_HELLO					1
#define NETMSG_QUIT						2
#define NETMSG_NEWFRAME				3
#define NETMSG_STARTPLAYING		4
#define NETMSG_SETPLAYERNUM		5
#define NETMSG_PLAYERNAME			6
#define NETMSG_CHAT						7
#define NETMSG_RANDBASE				8
//#define NETMSG_COMPARE				9
//#define NETMSG_PROJCOMPARE		10
#define NETMSG_COMMAND				11
#define NETMSG_SELECT					12
#define NETMSG_PAUSE					13
#define NETMSG_AICOMMAND			14
//#define NETMSG_SPENDING				15
#define NETMSG_SCRIPT					16
#define NETMSG_MEMDUMP				17
#define NETMSG_MAPNAME				18
#define NETMSG_USER_SPEED			19
#define NETMSG_INTERNAL_SPEED	20
#define NETMSG_CPU_USAGE			21
#define NETMSG_DIRECT_CONTROL 22
#define NETMSG_DC_UPDATE			23
//#define NETMSG_SETACTIVEPLAYERS 24
#define NETMSG_ATTEMPTCONNECT 25
#define NETMSG_SHARE					26
#define NETMSG_SETSHARE				27
#define NETMSG_SENDPLAYERSTAT	28
#define NETMSG_PLAYERSTAT			29
#define NETMSG_GAMEOVER				30
#define NETMSG_MAPDRAW				31
#define NETMSG_SYNCREQUEST		32
#define NETMSG_SYNCRESPONSE		33
#define NETMSG_SYNCERROR			34
#define NETMSG_SYSTEMMSG			35
#define NETMSG_STARTPOS				36
#define NETMSG_EXECHECKSUM		37


#define NETWORK_BUFFER_SIZE 40000

extern unsigned char netbuf[NETWORK_BUFFER_SIZE];	//buffer space for outgoing data, should only be used by main thread

class CNet  
{
public:
	CNet();
	void StopListening();
	int GetData(unsigned char* buf,int length,int conNum);
	int SendData(unsigned char* data,int length);
	int SendData(unsigned char* data,int length,int connection);
	int InitClient(const char* server,int portnum,bool localConnect=false);
	int InitServer(int portnum);
	void Update(void);
	virtual ~CNet();

	bool connected;
	bool inInitialConnect;
	bool waitOnCon;

	bool imServer;
	bool onlyLocal;

	double curTime;

	struct Packet {
		int length;
		unsigned char* data;

		Packet(): data(0),length(0){};
		Packet(const void* indata,int length): length(length){data=new unsigned char[length];memcpy(data,indata,length);};

		~Packet(){delete[] data;};
	};
	struct RawPacket : public Packet{};

	struct Connection {
		sockaddr_in addr;
		bool active;
		double lastReceiveTime;
		double lastSendTime;
		int lastSendFrame;
		Connection* localConnection;

		//outgoing stuff
		unsigned char outgoingData[NETWORK_BUFFER_SIZE];
		int outgoingLength;

		std::deque<Packet*> unackedPackets;
		int firstUnacked;
		int currentNum;

		//incomming stuff
		unsigned char readyData[NETWORK_BUFFER_SIZE];
		int readyLength;

		std::map<int,Packet*> waitingPackets;
		int lastInOrder;
		int lastNak;
		double lastNakTime;
	};
	Connection connections[MAX_PLAYERS];

	SOCKET mySocket;
	HANDLE netMutex;

	void ProcessRawPacket(unsigned char* data, int length, int conn);
	int InitNewConn(sockaddr_in* other,bool localConnect,int wantedNumber);
	int ResolveConnection(sockaddr_in* from);
	void FlushNet(void);
	void FlushConnection(int conn);
	void SendRawPacket(int conn, unsigned char* data, int length, int packetNum);

	void CreateDemoFile(char* name);
	void SaveToDemo(unsigned char* buf,int length);
	bool FindDemoFile(const char* name);

	ofstream* recordDemo;
	CFileHandler* playbackDemo;

	double demoTimeOffset;
	double nextDemoRead;
	void ReadDemoFile(void);
};

extern CNet* serverNet;
extern CNet* net;

#endif // !defined(AFX_NET_H__A0712941_2020_11D4_AD55_0080ADA84DE3__INCLUDED_)

