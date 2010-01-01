#ifndef PLAYERSTATISTICS_H
#define PLAYERSTATISTICS_H

#pragma pack(push, 1)

/**
 * @brief Contains statistical data about a player concerning a single game.
 */
struct PlayerStatistics
{
public:
	/// how many pixels the mouse has traversed in total
	int mousePixels;
	int mouseClicks;
	int keyPresses;

	int numCommands;
	/**
	 * The Total amount of units affected by commands.
	 * Divide by numCommands for average units/command.
	 */
	int unitCommands;

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};

#pragma pack(pop)

#endif