/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Net/UDPConnection.h"
#include "Rendering/TeamHighlight.h"
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "Sim/Misc/ModInfo.h"
#include "Lua/LuaConfig.h"

CONFIG(int, NetworkLossFactor)
.defaultValue(netcode::UDPConnection::MIN_LOSS_FACTOR)
	.minimumValue(netcode::UDPConnection::MIN_LOSS_FACTOR)
	.maximumValue(netcode::UDPConnection::MAX_LOSS_FACTOR);

CONFIG(int, InitialNetworkTimeout)
	.defaultValue(30)
	.minimumValue(10);

CONFIG(int, NetworkTimeout)
	.defaultValue(120)
	.minimumValue(0);

CONFIG(int, ReconnectTimeout)
	.defaultValue(15)
	.minimumValue(0);

CONFIG(int, MaximumTransmissionUnit)
	.defaultValue(1400)
	.minimumValue(400);

CONFIG(int, LinkOutgoingBandwidth)
	.defaultValue(64 * 1024)
	.minimumValue(0);

CONFIG(int, LinkIncomingSustainedBandwidth)
	.defaultValue(2 * 1024)
	.minimumValue(0);

CONFIG(int, LinkIncomingPeakBandwidth)
	.defaultValue(32 * 1024)
	.minimumValue(0);

CONFIG(int, LinkIncomingMaxPacketRate)
	.defaultValue(64)
	.minimumValue(0);

// maximum lag induced by command spam is:
// LinkIncomingMaxWaitingPackets / LinkIncomingMaxPacketRate = 8 seconds
CONFIG(int, LinkIncomingMaxWaitingPackets)
	.defaultValue(512)
	.minimumValue(0);

CONFIG(int, TeamHighlight)
	.defaultValue(CTeamHighlight::HIGHLIGHT_PLAYERS)
	.minimumValue(CTeamHighlight::HIGHLIGHT_FIRST)
	.maximumValue(CTeamHighlight::HIGHLIGHT_LAST);

CONFIG(bool, UseNetMessageSmoothingBuffer).defaultValue(true);

CONFIG(bool, LuaWritableConfigFile).defaultValue(true);

CONFIG(bool, EnableDrawCallIns).defaultValue(true);

GlobalConfig* globalConfig = NULL;

GlobalConfig::GlobalConfig()
{
	// Recommended semantics for "expert" type config values:
	// <0 = disable (if applicable)
	networkLossFactor = configHandler->GetInt("NetworkLossFactor");
	initialNetworkTimeout = configHandler->GetInt("InitialNetworkTimeout");
	networkTimeout = configHandler->GetInt("NetworkTimeout");
	reconnectTimeout = configHandler->GetInt("ReconnectTimeout");
	mtu = configHandler->GetInt("MaximumTransmissionUnit");

	linkOutgoingBandwidth = configHandler->GetInt("LinkOutgoingBandwidth");
	linkIncomingSustainedBandwidth = configHandler->GetInt("LinkIncomingSustainedBandwidth");
	linkIncomingPeakBandwidth = configHandler->GetInt("LinkIncomingPeakBandwidth");
	linkIncomingMaxPacketRate = configHandler->GetInt("LinkIncomingMaxPacketRate");
	linkIncomingMaxWaitingPackets = configHandler->GetInt("LinkIncomingMaxWaitingPackets");

	if (linkIncomingSustainedBandwidth > 0 && linkIncomingPeakBandwidth < linkIncomingSustainedBandwidth)
		linkIncomingPeakBandwidth = linkIncomingSustainedBandwidth;
	if (linkIncomingPeakBandwidth > 0 && linkIncomingSustainedBandwidth <= 0)
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth;
	if (linkIncomingMaxPacketRate > 0 && linkIncomingSustainedBandwidth <= 0)
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth = 1024 * 1024;

	useNetMessageSmoothingBuffer = configHandler->GetBool("UseNetMessageSmoothingBuffer");
	luaWritableConfigFile = configHandler->GetBool("LuaWritableConfigFile");

	teamHighlight = configHandler->GetInt("TeamHighlight");
}

void GlobalConfig::Instantiate()
{
	Deallocate();
	globalConfig = new GlobalConfig();
}

void GlobalConfig::Deallocate()
{
	delete globalConfig;
	globalConfig = NULL;
}
