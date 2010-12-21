/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ConfigHandler.h"

GlobalConfig* gc = NULL;

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
#ifdef USE_GML
	enableDrawCallIns = !!configHandler->Get("EnableDrawCallIns", 1);
#endif
}

void GlobalConfig::Instantiate() {
	Deallocate();
	gc = new GlobalConfig();
}

void GlobalConfig::Deallocate() {
	delete gc;
	gc = NULL;
}
