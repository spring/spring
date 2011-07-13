/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "Rendering/GL/myGL.h"
#include "System/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "Sim/Misc/ModInfo.h"

GlobalConfig* globalConfig = NULL;

GlobalConfig::GlobalConfig() {
	// Recommended semantics for "expert" type config values:
	//  0 = use default value
	// <0 = disable (if applicable)
	// This enables new engine versions to provide modified default values without any changes to config file
#define READ_CONFIG(name, str, def, min) name = configHandler->Get(str, 0); if(name == 0) name = def; else if(name < min) name = min;
	READ_CONFIG(initialNetworkTimeout, "InitialNetworkTimeout", 30, 0)
	READ_CONFIG(networkTimeout, "NetworkTimeout", 120, 0)
	READ_CONFIG(reconnectTimeout, "ReconnectTimeout", 15, 0)
	READ_CONFIG(mtu, "MaximumTransmissionUnit", 1400, 300)
	teamHighlight = configHandler->Get("TeamHighlight", 1);

	READ_CONFIG(linkOutgoingBandwidth, "LinkOutgoingBandwidth", 64 * 1024, 0)
	READ_CONFIG(linkIncomingSustainedBandwidth, "LinkIncomingSustainedBandwidth", 2 * 1024, 0)
	READ_CONFIG(linkIncomingPeakBandwidth, "LinkIncomingPeakBandwidth", 32 * 1024, 0)
	READ_CONFIG(linkIncomingMaxPacketRate, "LinkIncomingMaxPacketRate", 64, 0) // maximum lag induced by command-
	READ_CONFIG(linkIncomingMaxWaitingPackets, "LinkIncomingMaxWaitingPackets", 512, 0) // -spam is 512/64=8 seconds
	if(linkIncomingSustainedBandwidth > 0 && linkIncomingPeakBandwidth < linkIncomingSustainedBandwidth)
		linkIncomingPeakBandwidth = linkIncomingSustainedBandwidth;
	if(linkIncomingPeakBandwidth > 0 && linkIncomingSustainedBandwidth <= 0)
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth;
	if(linkIncomingMaxPacketRate > 0 && linkIncomingSustainedBandwidth <= 0)
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth = 1024 * 1024;
#if defined(USE_GML) && GML_ENABLE_SIM
	enableDrawCallIns = !!configHandler->Get("EnableDrawCallIns", 1);
#endif
#if (defined(USE_GML) && GML_ENABLE_SIM) || defined(USE_LUA_MT)
	multiThreadLua = configHandler->Get("MultiThreadLua", 0);
#endif
}


int GlobalConfig::GetMultiThreadLua() {
#if (defined(USE_GML) && GML_ENABLE_SIM) || defined(USE_LUA_MT)
	return std::max(1, std::min((multiThreadLua == 0) ? modInfo.luaThreadingModel : multiThreadLua, 5));
#else
	return 0;
#endif
}


void GlobalConfig::Instantiate() {
	Deallocate();
	globalConfig = new GlobalConfig();
}

void GlobalConfig::Deallocate() {
	delete globalConfig;
	globalConfig = NULL;
}
