//
// Net.cpp: implementation of the CNet class.
//
//////////////////////////////////////////////////////////////////////

// #include "StdAfx.h"

#include "Net.h"
#include "Connection.h"
#include "UDPConnection.h"
#include "UDPListener.h"
#include "LocalConnection.h"
#include "ProtocolDef.h"

#include <boost/format.hpp>

namespace netcode {

CNet::CNet()
{
	proto = new ProtocolDef();
	proto->UDP_MTU = 500;
}

CNet::~CNet()
{	
	delete proto;
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
	connections[connNumber]->Flush(true);
	connections[connNumber].reset();
}

bool CNet::Connected() const
{
	for (unsigned i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (connections[i] && (connections[i]->GetDataRecieved() > 0))
		{
			return true;
		}
	}
	
	return false;
}

int CNet::GetData(unsigned char *buf, const unsigned conNum)
{
	if (connections[conNum])
	{
		return connections[conNum]->GetData(buf);
	}
	else
	{
		return -1;
	}
}

void CNet::RegisterMessage(unsigned char id, int length)
{
	proto->AddType(id, length);
}

unsigned CNet::SetMTU(unsigned mtu)
{
	proto->UDP_MTU = mtu;
	return mtu;
}

int CNet::InitServer(unsigned portnum)
{
	udplistener.reset(new UDPListener(portnum, proto));

	return 0;
}

int CNet::InitClient(const char *server, unsigned portnum,unsigned sourceport, unsigned playerNum)
{
	udplistener.reset(new UDPListener(sourceport, proto));
	boost::shared_ptr<UDPConnection> incoming(udplistener->SpawnConnection(std::string(server), portnum));

	return InitNewConn(incoming, playerNum);
}

int CNet::InitLocalClient(const unsigned wantedNumber)
{
	boost::shared_ptr<CLocalConnection> conn(new CLocalConnection());
	
	return InitNewConn(conn, wantedNumber);
}

unsigned CNet::InitNewConn(boost::shared_ptr<CConnection> newClient, const unsigned wantedNumber)
{
	unsigned freeConn = 0;
	
	if(wantedNumber>=MAX_CONNECTIONS){
	}
	else if(connections[wantedNumber]){
	}
	else
	{
		freeConn = wantedNumber;
	}	

	if (freeConn == 0)
		for(freeConn=0;freeConn<MAX_CONNECTIONS;++freeConn){
			if(!connections[freeConn])
				break;
		}

	connections[freeConn]= newClient;

	return freeConn;
}

void CNet::SendData(const unsigned char *data, const unsigned length)
{
#ifdef DEBUG
	{
		unsigned msglength = 0;
		unsigned char msgid = data[0];
		if (GetProto()->HasFixedLength(msgid))
		{
			msglength = GetProto()->GetLength(msgid);
		}
		else
		{
			int length_t = GetProto()->GetLength(msgid);
			if (length_t == -1)
			{
				msglength = (unsigned)data[1];
			}
			else if (length_t == -2)
			{
				msglength = *((short*)(data+1));
			}
		}
		
		if (length != msglength || length == 0)
		{
			throw network_error( str( boost::format("Message length error (ID %1% with length %2% should be %3%) while sending (CNet::SendData(char*, unsigned))") % (unsigned)data[0] % length % msglength ) );
		}
	}
#endif

	for (unsigned a=0;a<MAX_CONNECTIONS;a++){	//improve: dont check every connection
		if(connections[a]){
			SendData(data,length, a);
		}
	}
}

void CNet::SendData(const unsigned char* data,const unsigned length, const unsigned playerNum)
{
	if (connections[playerNum])
	{
		connections[playerNum]->SendData(data,length);
	}
	else
	{
		throw network_error("Cant send data (wrong connection number)");
	}
}

void CNet::Update(void)
{
	if (udplistener)
	{
		udplistener->Update();
		while (udplistener->HasWaitingConnection())
		{
			waitingQueue.push(udplistener->GetWaitingConnection());
		}
	}
	
	for (unsigned a=0;a<MAX_CONNECTIONS;a++)
	{
		if(connections[a] && connections[a]->CheckTimeout()){
			//delete connections[a];
			connections[a].reset();
		}
	}
}

void CNet::FlushNet(void)
{
	for (unsigned a=0;a<MAX_CONNECTIONS;a++){
		if(connections[a]){
			connections[a]->Flush();
		}
	}
}

bool CNet::HasIncomingConnection() const
{
	return !waitingQueue.empty();
}

unsigned CNet::GetData(unsigned char* buf)
{
	if (HasIncomingConnection())
	{
		return waitingQueue.front()->GetData(buf);
	}
	else
	{
		throw std::runtime_error("No Connection waiting (no data recieved)");
	}
}

unsigned CNet::AcceptIncomingConnection(const unsigned wantedNumber)
{
	if (HasIncomingConnection())
	{
		unsigned buffer = InitNewConn(waitingQueue.front(), wantedNumber);
		waitingQueue.pop();
		return buffer;
	}
	else
	{
		throw network_error("No Connection waiting (no connection accepted)");
	}
}

void CNet::RejectIncomingConnection()
{
	if (HasIncomingConnection())
	{
		waitingQueue.pop();
	}
	else
	{
		throw network_error("No connection found to reject");
	}
}

const ProtocolDef* CNet::GetProto() const
{
	return proto;
}

} // namespace netcode


