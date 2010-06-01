/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Net/UDPConnection.h"

#ifndef _MSC_VER
#include "StdAfx.h"
#endif

#include <SDL_timer.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "mmgr.h"

#include "Net/LocalConnection.h"
#include "NetProtocol.h"

#include "Game/GameData.h"
#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "ConfigHandler.h"
#include "LoadSave/DemoRecorder.h"
#include "lib/gml/gml.h"
#include "Net/UnpackPacket.h"


CNetProtocol::CNetProtocol() : disableDemo(false)
{
}

CNetProtocol::~CNetProtocol()
{
	Send(CBaseNetProtocol::Get().SendQuit(""));
	logOutput.Print(serverConn->Statistics());
}

void CNetProtocol::InitClient(const char *server_addr, unsigned portnum, const std::string& myName, const std::string& myPasswd, const std::string& myVersion)
{
	GML_STDMUTEX_LOCK(net); // InitClient
	int sourceport = configHandler->Get("SourcePort", 0);
	
	netcode::UDPConnection* conn = new netcode::UDPConnection(sourceport, server_addr, portnum);
	serverConn.reset(conn);
	serverConn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myPasswd, myVersion));
	serverConn->Flush(true);
	
	logOutput.Print("Connecting to %s:%i using name %s", server_addr, portnum, myName.c_str());
}

void CNetProtocol::AttemptReconnect(const std::string& myName, const std::string& myPasswd, const std::string& myVersion) {
	GML_STDMUTEX_LOCK(net); // AttemptReconnect

	netcode::UDPConnection* conn = new netcode::UDPConnection(*serverConn);
	conn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myPasswd, myVersion, true));
	conn->Flush(true);

	logOutput.Print("Reconnecting to server... %ds", dynamic_cast<netcode::UDPConnection &>(*serverConn).GetReconnectSecs());

	delete conn;
}

bool CNetProtocol::NeedsReconnect() {
	return serverConn->NeedsReconnect();
}

void CNetProtocol::InitLocalClient()
{
	GML_STDMUTEX_LOCK(net); // InitLocalClient

	serverConn.reset(new netcode::CLocalConnection);
	serverConn->Flush();
	
	logOutput.Print("Connecting to local server");
}

bool CNetProtocol::CheckTimeout(int nsecs, bool initial) const {
	return serverConn->CheckTimeout(nsecs, initial);
}

bool CNetProtocol::Connected() const
{
	return (serverConn->GetDataReceived() > 0);
}

std::string CNetProtocol::ConnectionStr() const
{
	return serverConn->GetFullAddress();
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
		else if (ret->data[0] == NETMSG_GAMEDATA && !disableDemo) {
			try {
				GameData gd(ret);

				logOutput.Print("Starting demo recording");
				record.reset(new CDemoRecorder());
				record->WriteSetupText(gd.GetSetup());
				record->SaveToDemo(ret->data, ret->length, gu->modGameTime);
			} catch (netcode::UnpackPacketException &e) {
				logOutput.Print("Invalid GameData received");
			}
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

void CNetProtocol::DisableDemoRecording()
{
	disableDemo = true;
}

CNetProtocol* net=0;
