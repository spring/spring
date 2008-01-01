//
// Net.cpp: implementation of the CNet class.
//
//////////////////////////////////////////////////////////////////////


#include "Net.h"

#include <boost/format.hpp>

#include "Connection.h"
#include "UDPConnection.h"
#include "UDPListener.h"
#include "LocalConnection.h"
#include "ProtocolDef.h"
#include "RawPacket.h"
#include "Exception.h"


namespace netcode {

CNet::CNet()
{
	SetMTU();
}

CNet::~CNet()
{
	FlushNet();
}

void CNet::InitServer(unsigned portnum)
{
	udplistener.reset(new UDPListener(portnum));
}

unsigned CNet::InitClient(const char *server, unsigned portnum,unsigned sourceport, unsigned playerNum)
{
	udplistener.reset(new UDPListener(sourceport));
	boost::shared_ptr<UDPConnection> incoming(udplistener->SpawnConnection(std::string(server), portnum));

	return InitNewConn(incoming, playerNum);
}

unsigned CNet::InitLocalClient(const unsigned wantedNumber)
{
	boost::shared_ptr<CLocalConnection> conn(new CLocalConnection());	
	return InitNewConn(conn, wantedNumber);
}

void CNet::ServerInitLocalClient()
{
	boost::shared_ptr<CLocalConnection> conn(new CLocalConnection());
	waitingQueue.push(conn);
}

void CNet::RegisterMessage(unsigned char id, int length)
{
	ProtocolDef::instance()->AddType(id, length);
}

void CNet::SetMTU(unsigned mtu)
{
	ProtocolDef::instance()->UDP_MTU = mtu;
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

void CNet::Listening(const bool state)
{
	if (udplistener)
	{
		udplistener->SetWaitingForConnections(state);
	}
}

void CNet::Kill(const unsigned connNumber)
{
	if (int(connNumber) > MaxConnectionID() || !connections[connNumber])
		throw network_error("Wrong connection ID in CNet::Kill()");
	
	connections[connNumber]->Flush(true);
	connections[connNumber].reset();
	
	if (int(connNumber) == MaxConnectionID())
	{
		//TODO remove other connections when neccesary
		connections.pop_back();
	}
}

bool CNet::Connected() const
{
	for (connVec::const_iterator  i = connections.begin(); i < connections.end(); ++i)
	{
		if ((*i) && (*i)->GetDataRecieved() > 0)
		{
			return true;
		}
	}
	
	return false;
}

int CNet::MaxConnectionID() const
{
	return connections.size()-1;
}

bool CNet::IsActiveConnection(const unsigned number) const
{
	if (int(number) > MaxConnectionID())
		throw network_error("Wrong connection ID in CNet::IsActiveConnection()");
	
	return connections[number];
}

const RawPacket* CNet::Peek(const unsigned conNum, unsigned ahead) const
{
	if (int(conNum) <= MaxConnectionID() && (bool)connections[conNum])
	{
		return connections[conNum]->Peek(ahead);
	}
	else
	{
		throw network_error(str( boost::format("Wrong connection ID in CNet::Peek(): %1%") %conNum ));
	}
}

RawPacket* CNet::GetData(const unsigned conNum)
{
	if (int(conNum) <= MaxConnectionID() && (bool)connections[conNum])
	{
		return connections[conNum]->GetData();
	}
	else
	{
		throw network_error(str( boost::format("Wrong connection ID in CNet::GetData(): %1%") %conNum ));
	}
}

void CNet::SendData(const unsigned char *data, const unsigned length)
{
#ifdef DEBUG
	{
		unsigned msglength = 0;
		unsigned char msgid = data[0];
		ProtocolDef* proto = ProtocolDef::instance();
		if (proto->HasFixedLength(msgid))
		{
			msglength = proto->GetLength(msgid);
		}
		else
		{
			int length_t = proto->GetLength(msgid);
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

	for (connVec::iterator it = connections.begin(); it != connections.end(); ++it)
	{
		if(*it){
			(*it)->SendData(data,length);
		}
	}
}

void CNet::SendData(const unsigned char* data,const unsigned length, const unsigned playerNum)
{
	if (int(playerNum) <= MaxConnectionID() && connections[playerNum])
	{
		connections[playerNum]->SendData(data,length);
	}
	else
	{
		throw network_error("Cant send data (wrong connection number)");
	}
}

void CNet::FlushNet()
{
	for (connVec::const_iterator  i = connections.begin(); i < connections.end(); ++i)
	{
		if((*i)){
			(*i)->Flush(true);
		}
	}
}

void CNet::Update()
{
	if (udplistener)
	{
		udplistener->Update(waitingQueue);
	}
	
	for (connVec::iterator  i = connections.begin(); i < connections.end(); ++i)
	{
		if((*i) && (*i)->CheckTimeout())
		{
			i->reset();
		}
	}
}

unsigned CNet::InitNewConn(const connPtr& newClient, const unsigned wantedNumber)
{
	unsigned freeConn = wantedNumber;
	
	if(int(wantedNumber) <= MaxConnectionID() && connections[wantedNumber])
	{
		// ID is already in use
		freeConn = 0;
	}

	if (freeConn == 0) // look for first free ID
	{
		for(freeConn = 0; int(freeConn) <= MaxConnectionID(); ++freeConn)
		{
			if(!connections[freeConn])
			{
				break;
			}
		}
	}
	
	if (int(freeConn) > MaxConnectionID())
	{
		// expand the vector
		connections.resize(freeConn+1);
	}
	connections[freeConn] = newClient;

	return freeConn;
}

bool CNet::HasIncomingConnection() const
{
	return !waitingQueue.empty();
}

RawPacket* CNet::GetData()
{
	if (HasIncomingConnection())
	{
		RawPacket* data = waitingQueue.front()->GetData();
		return data;
	}
	else
	{
		throw network_error("No Connection waiting (no data recieved)");
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

} // namespace netcode


