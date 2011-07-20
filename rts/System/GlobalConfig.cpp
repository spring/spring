/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "Sim/Misc/ModInfo.h"

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
	.minimumValue(300);

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
	.defaultValue(1) // FIXME: magic constant
	.minimumValue(0)
	.maximumValue(2); // FIXME: magic constant

CONFIG(bool, EnableDrawCallIns)
	.defaultValue(true);

CONFIG(int, MultiThreadLua)
	.defaultValue(0)
	.minimumValue(0)
	.maximumValue(5); // FIXME: magic constant

GlobalConfig* globalConfig = NULL;

GlobalConfig::GlobalConfig()
{
	// Recommended semantics for "expert" type config values:
	// <0 = disable (if applicable)
	initialNetworkTimeout = configHandler->GetInt("InitialNetworkTimeout");
	networkTimeout = configHandler->GetInt("NetworkTimeout");
	reconnectTimeout = configHandler->GetInt("ReconnectTimeout");
	mtu = configHandler->GetInt("MaximumTransmissionUnit");
	teamHighlight = configHandler->GetInt("TeamHighlight");

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

#if defined(USE_GML) && GML_ENABLE_SIM
	enableDrawCallIns = configHandler->GetBool("EnableDrawCallIns");
#endif
#if (defined(USE_GML) && GML_ENABLE_SIM) || defined(USE_LUA_MT)
	multiThreadLua = configHandler->GetInt("MultiThreadLua");
#endif
}


int GlobalConfig::GetMultiThreadLua()
{
#if (defined(USE_GML) && GML_ENABLE_SIM) || defined(USE_LUA_MT)
	// FIXME: magic constant
	return std::max(1, std::min((multiThreadLua == 0) ? modInfo.luaThreadingModel : multiThreadLua, 5));
#else
	return 0;
#endif
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
