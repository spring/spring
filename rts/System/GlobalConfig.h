/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H


class GlobalConfig {
public:
	GlobalConfig();

	static void Instantiate();

	static void Deallocate();

	/**
	 * @brief initial network timeout
	 *
	 * Network timeout in seconds, effective before the game has started
	 */
	int initialNetworkTimeout;

	/**
	 * @brief network timeout
	 *
	 * Network timeout in seconds, effective after the game has started
	 */
	int networkTimeout;

	/**
	 * @brief reconnect timeout
	 *
	 * Network timeout in seconds after which a player is allowed to reconnect with a different IP
	 */
	int reconnectTimeout;

	/**
	 * @brief MTU
	 *
	 * Maximum size of network packets to send
	 */
	unsigned mtu;

	/**
	 * @brief teamHighlight
	 *
	 * Team highlighting for teams that are uncontrolled or have connection problems
	 */
	int teamHighlight;

	/**
	 * @brief linkBandwidth
	 *
	 * Maximum outgoing bandwidth from server in bytes, per user
	 */
	int linkOutgoingBandwidth;

	/**
	 * @brief linkIncomingSustainedBandwidth
	 *
	 * Maximum incoming sustained bandwidth to server in bytes, per user
	 */
	int linkIncomingSustainedBandwidth;

	/**
	 * @brief linkIncomingPeakBandwidth
	 *
	 * Maximum peak incoming bandwidth to server in bytes, per user
	 */
	int linkIncomingPeakBandwidth;

	/**
	 * @brief linkIncomingMaxPacketRate
	 *
	 * Maximum number of incoming packets to server, per user and second
	 */
	int linkIncomingMaxPacketRate;

	/**
	 * @brief linkIncomingMaxWaitingPackets
	 *
	 * Maximum number of queued incoming packets to server, per user
	 */
	int linkIncomingMaxWaitingPackets;

#ifdef USE_GML
	bool enableDrawCallIns;
#endif
};

extern GlobalConfig* gc;

#endif // GLOBALCONFIG_H
