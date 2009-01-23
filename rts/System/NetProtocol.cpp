#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <SDL_timer.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>

#include "mmgr.h"

#include "NetProtocol.h"

#include "Game/GameSetup.h"
#include "Game/GameData.h"
#include "Game/GameServer.h"
#include "LogOutput.h"
#include "DemoRecorder.h"
#include "ConfigHandler.h"
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
	boost::shared_ptr<netcode::UDPSocket> sock(new netcode::UDPSocket(sourceport));
	sock->SetBlocking(false);
	netcode::UDPConnection* conn = new netcode::UDPConnection(sock, server_addr, portnum);
	conn->SetMTU(configHandler.Get("MaximumTransmissionUnit", 0));
	serverConn.reset(conn);
	serverConn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myVersion));
	serverConn->Flush(true);
	
	logOutput.Print("Connecting to %s:%i using name %s", server_addr, portnum, myName.c_str());
}

void CNetProtocol::InitLocalClient()
{
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
	return serverConn->Peek(ahead);
}

boost::shared_ptr<const netcode::RawPacket> CNetProtocol::GetData()
{
	boost::shared_ptr<const netcode::RawPacket> ret = serverConn->GetData();

	if (ret) {
		if (record) {
			record->SaveToDemo(ret->data, ret->length);
		}
		else if (ret->data[0] == NETMSG_GAMEDATA && !gameServer->HasDemo()) {
			logOutput.Print("Starting demo recording");
			GameData gd(ret);
			record.reset(new CDemoRecorder());
			record->WriteSetupText(gd.GetSetup());
			record->SaveToDemo(ret->data, ret->length);
		}
	}

	return ret;
}

void CNetProtocol::Send(boost::shared_ptr<const netcode::RawPacket> pkt)
{
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
	serverConn->Update();
}

CNetProtocol* net=0;
