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
};

extern GlobalConfig* gc;

#endif // GLOBALCONFIG_H
