/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <memory>

// NOTE: these _must_ be included before NetProtocol.h due to some ambiguity in
// Boost hash_float.hpp ("call of overloaded ‘ldexp(float&, int&)’ is ambiguous")
#include "System/Net/UDPConnection.h"
#include "System/Net/LocalConnection.h"

#include "NetProtocol.h"

#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Net/UnpackPacket.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Platform/Threading.h"
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"

CONFIG(int, SourcePort).defaultValue(0);


CNetProtocol* clientNet = nullptr;

CNetProtocol::CNetProtocol() : keepUpdating(false)
{
	demoRecorder.reset(nullptr);
}

CNetProtocol::~CNetProtocol()
{
	// when the client-server connection is deleted, make sure
	// the server cleans up its corresponding connection to the
	// client
	Send(CBaseNetProtocol::Get().SendQuit(__FUNCTION__));
	Close(true);

	LOG("%s", serverConn->Statistics().c_str());
}


void CNetProtocol::InitClient(const char* server_addr, unsigned portnum, const std::string& myName, const std::string& myPasswd, const std::string& myVersion)
{
	userName = myName;
	userPasswd = myPasswd;

	serverConn.reset(new netcode::UDPConnection(configHandler->GetInt("SourcePort"), server_addr, portnum));
	serverConn->Unmute();
	serverConn->SendData(CBaseNetProtocol::Get().SendAttemptConnect(userName, userPasswd, myVersion, globalConfig->networkLossFactor));
	serverConn->Flush(true);

	LOG("[NetProto::%s] connecting to IP %s on port %i using name %s", __func__, server_addr, portnum, myName.c_str());
}

void CNetProtocol::InitLocalClient()
{
	serverConn.reset(new netcode::CLocalConnection());
	serverConn->Flush();

	LOG("[NetProto::%s] connecting to local server", __func__);
}


void CNetProtocol::AttemptReconnect(const std::string& myVersion)
{
	netcode::UDPConnection conn(*serverConn);

	conn.Unmute();
	conn.SendData(CBaseNetProtocol::Get().SendAttemptConnect(userName, userPasswd, myVersion, globalConfig->networkLossFactor, true));
	conn.Flush(true);

	LOG("[NetProto::%s] reconnecting to server... %ds", __func__, dynamic_cast<netcode::UDPConnection&>(*serverConn).GetReconnectSecs());
}


bool CNetProtocol::NeedsReconnect() {
	return serverConn->NeedsReconnect();
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

std::shared_ptr<const netcode::RawPacket> CNetProtocol::Peek(unsigned ahead) const
{
	return serverConn->Peek(ahead);
}

void CNetProtocol::DeleteBufferPacketAt(unsigned index)
{
	return serverConn->DeleteBufferPacketAt(index);
}

float CNetProtocol::GetPacketTime(int frameNum) const
{
	// startTime is not yet defined pre-simframe
	if (frameNum < 0)
		return gu->gameTime;

	return (gu->startTime + frameNum / (1.0f * GAME_SPEED));
}

std::shared_ptr<const netcode::RawPacket> CNetProtocol::GetData(int frameNum)
{
	std::shared_ptr<const netcode::RawPacket> ret = serverConn->GetData();

	if (ret.get() == nullptr)
		return ret;
	if (ret->data[0] == NETMSG_GAMEDATA)
		return ret;

	if (demoRecorder.get() != nullptr)
		demoRecorder->SaveToDemo(ret->data, ret->length, GetPacketTime(frameNum));

	return ret;
}

void CNetProtocol::Send(std::shared_ptr<const netcode::RawPacket> pkt)
{
	serverConn->SendData(pkt);
}

void CNetProtocol::Send(const netcode::RawPacket* pkt)
{
	std::shared_ptr<const netcode::RawPacket> ptr(pkt);
	Send(ptr);
}

__FORCE_ALIGN_STACK__
void CNetProtocol::UpdateLoop()
{
	Threading::SetThreadName("heartbeat");

	while (keepUpdating) {
		Update();
		spring_msecs(100).sleep();
	}
}

void CNetProtocol::Update()
{
	serverConn->Update();
}

void CNetProtocol::Close(bool flush)
{
	serverConn->Close(flush);
}



void CNetProtocol::SetDemoRecorder(CDemoRecorder* r) { demoRecorder.reset(r); }
CDemoRecorder* CNetProtocol::GetDemoRecorder() const { return demoRecorder.get(); }

unsigned int CNetProtocol::GetNumWaitingServerPackets() const { return (serverConn.get())->GetPacketQueueSize(); }

