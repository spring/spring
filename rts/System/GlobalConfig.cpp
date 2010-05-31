/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ConfigHandler.h"

GlobalConfig* gc = NULL;

GlobalConfig::GlobalConfig() {
	initialNetworkTimeout =  std::max(configHandler->Get("InitialNetworkTimeout", 30), 0);
	networkTimeout = std::max(configHandler->Get("NetworkTimeout", 120), 0);
	reconnectTimeout = configHandler->Get("ReconnectTimeout", 15);
	mtu = std::max(configHandler->Get("MaximumTransmissionUnit", 1400), 300);
}

void GlobalConfig::Instantiate() {
	Deallocate();
	gc = new GlobalConfig();
}

void GlobalConfig::Deallocate() {
	if(gc != NULL) {
		delete gc;
		gc = NULL;
	}
}
