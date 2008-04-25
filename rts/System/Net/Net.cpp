//
// Net.cpp: implementation of the CNet class.
//
//////////////////////////////////////////////////////////////////////


#include "Net.h"

#include <boost/format.hpp>
#include <boost/weak_ptr.hpp>

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
	localConnBuf = conn;
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
		return udplistener->Listen();
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
		udplistener->Listen(state);
	}
}

void CNet::Kill(const unsigned connNumber)
{
	if (int(connNumber) > MaxConnectionID() || !connections[connNumber])
		throw network_error("Wrong connection ID in CNet::Kill()");
	
	connections[connNumber]->Flush(true);
	connections[connNumber].reset();
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
		return false;
	else
		return connections[number];
}

std::string CNet::GetConnectionStatistics(const unsigned number) const
{
	return connections[number]->Statistics();
}

NetAddress CNet::GetConnectedAddress(const unsigned number)
{
	return connections[number]->GetPeerName();
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
		unsigned int msglength = 0;
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
				msglength = (unsigned int)data[1];
			}
			else if (length_t == -2)
			{
				msglength = *((short*)(data+1));
			}
		}
		
		if (length != msglength || length == 0)
		{
			throw network_error( str( boost::format("Message length error (ID %1% with length %2% should be %3%) while sending (CNet::SendData(char*, unsigned))") % (unsigned int)data[0] % length % msglength ) );
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

void CNet::SendData(const RawPacket* data)
{
	SendData(data->data, data->length);
	delete data;
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

void CNet::SendData(const RawPacket* data, const unsigned playerNum)
{
	if (int(playerNum) <= MaxConnectionID() && connections[playerNum])
	{
		connections[playerNum]->SendData(data);
	}
	else
	{
		delete data;
		throw network_error("Cant send data (wrong connection number)");
	}
}

void CNet::FlushNet()
{
	for (connVec::const_iterator i = connections.begin(); i != connections.end(); ++i)
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
		udplistener->Update();
	}
	
	for (connVec::iterator i = connections.begin(); i != connections.end(); ++i)
	{
		if((*i) && (*i)->CheckTimeout())
		{
			(*i)->Flush(true);
			i->reset();
		}
	}
	
	// remove the last pointer from the vector if it doesn't store a connection
	if (!connections.empty() && !(connections.back()))
		connections.pop_back();
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
	return (localConnBuf || (udplistener && udplistener->HasIncomingConnections()));
}

RawPacket* CNet::GetData()
{
	if (localConnBuf)
	{
		RawPacket* data = localConnBuf->GetData();
		return data;
	}
	else if (udplistener && udplistener->HasIncomingConnections())
	{
		boost::shared_ptr<UDPConnection> locked(udplistener->PreviewConnection());
		RawPacket* data = locked->GetData();
		return data;
	}
	else
	{
		throw network_error("No Connection waiting (no data recieved)");
	}
}

unsigned CNet::AcceptIncomingConnection(const unsigned wantedNumber)
{
	if (localConnBuf)
	{
		unsigned buffer = InitNewConn(localConnBuf, wantedNumber);
		localConnBuf.reset();
		return buffer;
	}
	else if (udplistener && udplistener->HasIncomingConnections())
	{
		unsigned buffer = InitNewConn(udplistener->AcceptConnection(), wantedNumber);
		return buffer;
	}
	else
	{
		throw network_error("No Connection waiting (no connection accepted)");
	}
}

void CNet::RejectIncomingConnection()
{
	if (localConnBuf)
	{
		localConnBuf.reset();
	}
	else if (udplistener && udplistener->HasIncomingConnections())
	{
		udplistener->RejectConnection();
	}
	else
	{
		throw network_error("No connection found to reject");
	}
}

} // namespace netcode


