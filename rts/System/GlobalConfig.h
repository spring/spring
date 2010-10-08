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
	 * Network link maximum bandwidth, per user, in bytes
	 */
	int linkBandwidth;

#ifdef USE_GML
	/**
	 * @brief multiThreadLua
	 *
	 * LuaHandle threading mode for Spring MT:
	 * 0: Use 'luaThreadingModel' setting from modInfo (default)
	 * 1: Single Lua state (fully backwards compatible but slow)
	 * 2: Dual Lua states for synced, synced/unsynced gadget communication via EXPORT table and SendToUnsynced, 
	 * 3: Dual Lua states for synced, synced/unsynced gadget communication via SendToUnsynced only
	 * 4: Dual Lua states for all, all synced/unsynced communication (widgets included) via SendToUnsynced only
	 */
	int multiThreadLua;
	bool enableDrawCallIns;
#endif
	int GetMultiThreadLua();
};

extern GlobalConfig* gc;

#endif // GLOBALCONFIG_H
