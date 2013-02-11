/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Net/UDPConnection.h"

#include <SDL_timer.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>


// NOTE: these _must_ be included before NetProtocol.h due to some ambiguity in
// Boost hash_float.hpp ("call of overloaded ‘ldexp(float&, int&)’ is ambiguous")
#include "System/Net/LocalConnection.h"
#include "System/NetProtocol.h"

#include "Game/GameData.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Net/UnpackPacket.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "lib/gml/gmlmut.h"

CONFIG(int, SourcePort).defaultValue(0);

CNetProtocol::CNetProtocol() : loading(false)
{
	demoRecorder.reset(NULL);
}

CNetProtocol::~CNetProtocol()
{
	Send(CBaseNetProtocol::Get().SendQuit(""));
	Close();
	LOG("%s", serverConn->Statistics().c_str());
}

void CNetProtocol::InitClient(const char* server_addr, unsigned portnum, const std::string& myName, const std::string& myPasswd, const std::string& myVersion)
{
	GML_STDMUTEX_LOCK(net); // InitClient

	netcode::UDPConnection* conn = new netcode::UDPConnection(configHandler->GetInt("SourcePort"), server_addr, portnum);
	conn->Unmute();
	serverConn.reset(conn);
	serverConn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myPasswd, myVersion, globalConfig->networkLossFactor));
	serverConn->Flush(true);

	LOG("Connecting to %s:%i using name %s", server_addr, portnum, myName.c_str());
}

void CNetProtocol::AttemptReconnect(const std::string& myName, const std::string& myPasswd, const std::string& myVersion) {
	GML_STDMUTEX_LOCK(net); // AttemptReconnect

	netcode::UDPConnection* conn = new netcode::UDPConnection(*serverConn);
	conn->Unmute();
	conn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(myName, myPasswd, myVersion, globalConfig->networkLossFactor, true));
	conn->Flush(true);

	LOG("Reconnecting to server... %ds", dynamic_cast<netcode::UDPConnection&>(*serverConn).GetReconnectSecs());

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

	LOG("Connecting to local server");
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

void CNetProtocol::DeleteBufferPacketAt(unsigned index)
{
	GML_STDMUTEX_LOCK(net); // DeleteBufferPacketAt

	return serverConn->DeleteBufferPacketAt(index);
}

float CNetProtocol::GetPacketTime(int frameNum) const
{
	if (frameNum == 0)
		return gu->gameTime;

	return (gu->startTime + (float)frameNum / (float)GAME_SPEED);
}

boost::shared_ptr<const netcode::RawPacket> CNetProtocol::GetData(int frameNum)
{
	GML_STDMUTEX_LOCK(net); // GetData

	boost::shared_ptr<const netcode::RawPacket> ret = serverConn->GetData();

	if (ret.get() == NULL) { return ret; }
	if (ret->data[0] == NETMSG_GAMEDATA) { return ret; }

	if (demoRecorder.get() != NULL) {
		demoRecorder->SaveToDemo(ret->data, ret->length, GetPacketTime(frameNum));
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
	Threading::SetThreadName("heartbeat");
	loading = true;
	while (loading) {
		Update();
		SDL_Delay(400);
	}
}

void CNetProtocol::Update()
{
	GML_STDMUTEX_LOCK(net); // Update

	serverConn->Update();
}

void CNetProtocol::Close(bool flush) {
	GML_STDMUTEX_LOCK(net); // Close

	serverConn->Close(flush);
}



void CNetProtocol::SetDemoRecorder(CDemoRecorder* r) { demoRecorder.reset(r); }
CDemoRecorder* CNetProtocol::GetDemoRecorder() const { return demoRecorder.get(); }

CNetProtocol* net = NULL;

