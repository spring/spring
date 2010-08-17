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
	READ_CONFIG(linkBandwidth, "LinkBandwidth", 64 * 1024, 0)
}

void GlobalConfig::Instantiate() {
	Deallocate();
	gc = new GlobalConfig();
}

void GlobalConfig::Deallocate() {
	delete gc;
	gc = NULL;
}
