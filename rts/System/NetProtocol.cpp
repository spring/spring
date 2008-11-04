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
#include "LogOutput.h"
#include "DemoRecorder.h"
#include "Platform/ConfigHandler.h"
#include "Net/UDPConnection.h"
#include "Net/LocalConnection.h"
#include "Net/UDPSocket.h"


CNetProtocol::CNetProtocol()
{
}

CNetProtocol::~CNetProtocol()
{
	Send(CBaseNetProtocol::Get().SendQuit());
	logOutput.Print(server->Statistics());
}

void CNetProtocol::InitClient(const char *server_addr, unsigned portnum,unsigned sourceport, const std::string& myName, const std::string& myVersion)
{
	boost::shared_ptr<netcode::UDPSocket> sock(new netcode::UDPSocket(sourceport));
	sock->SetBlocking(false);
	netcode::UDPConnection* conn = new netcode::UDPConnection(sock, server_addr, portnum);
	conn->SetMTU(configHandler.GetInt("MaximumTransmissionUnit", 0));
	server.reset(conn);
	server->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myVersion));
	server->Flush(true);

	if (!gameSetup || !gameSetup->hostDemo)	//TODO do we really want this?
	{
		record.reset(new CDemoRecorder());
	}
	
	logOutput.Print("Connecting to %s:%i using name %s", server_addr, portnum, myName.c_str());
}

void CNetProtocol::InitLocalClient()
{
	server.reset(new netcode::CLocalConnection);
	server->Flush();
	record.reset(new CDemoRecorder()); //TODO don't record a new demo when already watching one
	
	logOutput.Print("Connecting to local server");
}

bool CNetProtocol::Active() const
{
	return !server->CheckTimeout();
}

bool CNetProtocol::Connected() const
{
	return (server->GetDataReceived() > 0);
}

boost::shared_ptr<const netcode::RawPacket> CNetProtocol::Peek(unsigned ahead) const
{
	return server->Peek(ahead);
}

boost::shared_ptr<const netcode::RawPacket> CNetProtocol::GetData()
{
	boost::shared_ptr<const netcode::RawPacket> ret = server->GetData();
	
	if (record && ret)
		record->SaveToDemo(ret->data, ret->length);
	
	return ret;
}

void CNetProtocol::Send(boost::shared_ptr<const netcode::RawPacket> pkt)
{
	server->SendData(pkt);
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
	server->Update();
}

CNetProtocol* net=0;
