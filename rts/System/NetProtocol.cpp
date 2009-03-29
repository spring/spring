#include "StdAfx.h"
#include <SDL_timer.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "mmgr.h"

#include "NetProtocol.h"

#include "Game/GameData.h"
#include "LogOutput.h"
#include "DemoRecorder.h"
#include "ConfigHandler.h"
#include "GlobalUnsynced.h"
#include "Net/UDPConnection.h"
#include "Net/LocalConnection.h"
#include "Net/UDPSocket.h"


CNetProtocol::CNetProtocol()
{
}

CNetProtocol::~CNetProtocol()
{
	Send(CBaseNetProtocol::Get().SendQuit());
	logOutput.Print(serverConn->Statistics());
}

void CNetProtocol::InitClient(const char *server_addr, unsigned portnum,unsigned sourceport, const std::string& myName, const std::string& myVersion)
{
	GML_STDMUTEX_LOCK(net); // InitClient

	boost::shared_ptr<netcode::UDPSocket> sock(new netcode::UDPSocket(sourceport));
	sock->SetBlocking(false);
	netcode::UDPConnection* conn = new netcode::UDPConnection(sock, server_addr, portnum);
	conn->SetMTU(configHandler->Get("MaximumTransmissionUnit", 0));
	serverConn.reset(conn);
	serverConn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myVersion));
	serverConn->Flush(true);
	
	logOutput.Print("Connecting to %s:%i using name %s", server_addr, portnum, myName.c_str());
}

void CNetProtocol::InitLocalClient()
{
	GML_STDMUTEX_LOCK(net); // InitLocalClient

	serverConn.reset(new netcode::CLocalConnection);
	serverConn->Flush();
	
	logOutput.Print("Connecting to local server");
}

bool CNetProtocol::Active() const
{
	return !serverConn->CheckTimeout();
}

bool CNetProtocol::Connected() const
{
	return (serverConn->GetDataReceived() > 0);
}

boost::shared_ptr<const netcode::RawPacket> CNetProtocol::Peek(unsigned ahead) const
{
	GML_STDMUTEX_LOCK(net); // Peek

	return serverConn->Peek(ahead);
}

boost::shared_ptr<const netcode::RawPacket> CNetProtocol::GetData()
{
	GML_STDMUTEX_LOCK(net); // GetData

	boost::shared_ptr<const netcode::RawPacket> ret = serverConn->GetData();

	if (ret) {
		if (record) {
			record->SaveToDemo(ret->data, ret->length, gu->modGameTime);
		}
		else if (ret->data[0] == NETMSG_GAMEDATA) {
			logOutput.Print("Starting demo recording");
			GameData gd(ret);
			record.reset(new CDemoRecorder());
			record->WriteSetupText(gd.GetSetup());
			record->SaveToDemo(ret->data, ret->length, gu->modGameTime);
		}
	}

	return ret;
}

void CNetProtocol::Send(boost::shared_ptr<const netcode::RawPacket> pkt)
{
	GML_STDMUTEX_LOCK(net); // Send

	serverConn->SendData(pkt);
}

void CNetProtocol::Send(const netcode::RawPacket* pkt)
{
	boost::shared_ptr<const netcode::RawPacket> ptr(pkt);
	Send(ptr);
}

void CNetProtocol::UpdateLoop()
{
	loading = true;
	while (loading)
	{
		Update();
		SDL_Delay(400);
	}
}

void CNetProtocol::Update()
{
	GML_STDMUTEX_LOCK(net); // Update

	serverConn->Update();
}

CNetProtocol* net=0;
