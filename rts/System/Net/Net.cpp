//
// Net.cpp: implementation of the CNet class.
//
//////////////////////////////////////////////////////////////////////

// #include "StdAfx.h"

#include "Net.h"
#include "LogOutput.h"
#include "UDPConnection.h"
#include "UDPListener.h"
#include "LocalConnection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace netcode {

CNet::CNet()
{
	for(int a=0;a<MAX_PLAYERS;a++){
		connections[a]=0;
	}
	
	local = 0;

	imServer=false;

	udplistener=0;
}

CNet::~CNet()
{
	if(udplistener)
		delete udplistener;
	
	if (local)
		delete local;
}

void CNet::Listening(const bool state)
{
	if (udplistener)
	{
		udplistener->SetWaitingForConnections(false);
	}
}

bool CNet::Listening()
{
	if (udplistener)
	{
		return udplistener->GetWaitingForConnections();
	}
	else
	{
		return false;
	}
}

void CNet::Kill(const unsigned connNumber)
{
	// logOutput.Print("Killing connection %i", connNumber);
	connections[connNumber]->Flush(true);
	connections[connNumber]->active = false;
}

bool CNet::Connected() const
{
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (connections[i] && (connections[i]->dataRecv > 0))
		{
			return true;
		}
	}
	if (local)
	{
		return true;
	}
	else
	{
		return false;
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
	imServer=true;

	udplistener = new UDPListener(portnum);

	return 0;
}

int CNet::InitClient(const char *server, unsigned portnum,unsigned sourceport, unsigned playerNum)
{
	udplistener = new UDPListener(sourceport);
	UDPConnection* incoming = udplistener->SpawnConnection(std::string(server), portnum);

	imServer=false;

	return InitNewConn(incoming, playerNum);
}

int CNet::InitLocalClient(const unsigned wantedNumber)
{
	CLocalConnection* conn = new CLocalConnection();
	local = conn;
	
	return InitNewConn(conn, wantedNumber);
}

unsigned CNet::InitNewConn(CConnection* newClient, const unsigned wantedNumber)
{
	unsigned freeConn = 0;
	
	if(wantedNumber>=MAX_PLAYERS){
		logOutput.Print("Warning attempt to connect to errenous connection number");
	}
	else if(connections[wantedNumber] && connections[wantedNumber]->active && wantedNumber > 0){
		logOutput.Print("Warning attempt to connect to already active connection number");
	}
	else
	{
		freeConn = wantedNumber;
	}
	

	if (freeConn == 0)
		for(freeConn=0;freeConn<MAX_PLAYERS;++freeConn){
		if(!connections[freeConn])
			break;
		}

	connections[freeConn]= newClient;

	return freeConn;
}

int CNet::SendData(const unsigned char *data, const unsigned length)
{
	if(length==0)
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
	if (udplistener)
	{
		udplistener->Update();
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

CConnection* CNet::GetIncomingConnection() const
{
	if (udplistener)
	{
		return udplistener->GetWaitingConenction();
	}
	else
	{
		return 0;
	}
}

unsigned CNet::AcceptIncomingConnection(const unsigned wantedNumber)
{
	if (udplistener)
	{
		CConnection* conn = udplistener->GetWaitingConenction();
		
		if (conn == 0)
		{
			throw network_error("No waiting connections to accept");
		}
		
		udplistener->AcceptWaitingConnection();
		return InitNewConn(conn, wantedNumber);
	}
	else
	{
		throw network_error("No listensocket was set up so no connections can be accepted");
	}
}

void CNet::RejectIncomingConnection()
{
	if (udplistener && !udplistener->GetWaitingConenction())
	{
		udplistener->RejectWaitingConnection();
	}
	else
	{
		throw network_error("No connection found to reject");
	}
}


} // namespace netcode


