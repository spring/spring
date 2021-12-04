/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/GlobalConfig.h"

#include <cstddef>

GlobalConfig* globalConfig = NULL;

GlobalConfig::GlobalConfig() {

	initialNetworkTimeout = 30;
	networkTimeout = 120;
	reconnectTimeout = 15;
	mtu = 1400;
	teamHighlight = 1;

	linkOutgoingBandwidth = 64 * 1024;
	linkIncomingSustainedBandwidth = 2;
	linkIncomingPeakBandwidth = 32;
	linkIncomingMaxPacketRate = 64;
	linkIncomingMaxWaitingPackets = 512;
	if ((linkIncomingSustainedBandwidth > 0) && (linkIncomingPeakBandwidth < linkIncomingSustainedBandwidth)) {
		linkIncomingPeakBandwidth = linkIncomingSustainedBandwidth;
	}
	if ((linkIncomingPeakBandwidth > 0) && (linkIncomingSustainedBandwidth <= 0)) {
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth;
	}
	if ((linkIncomingMaxPacketRate > 0) && (linkIncomingSustainedBandwidth <= 0)) {
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth = 1024 * 1024;
	}
	useNetMessageSmoothingBuffer = true;
	luaWritableConfigFile = false;
	networkLossFactor = 0;
}

void GlobalConfig::Instantiate() {
	Deallocate();
	globalConfig = new GlobalConfig();
}

void GlobalConfig::Deallocate() {
	delete globalConfig;
	globalConfig = NULL;
}
