/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Defines the Spring demofile format
 *
 * This file defines the Spring demofile format and which parts of it are
 * supposed to be stable (can be used safely by 3rd party applications) or
 * unstable (format may change every Spring release without notice).
 */

#ifndef DEMO_FILE_H
#define DEMO_FILE_H

#include "System/Platform/byteorder.h"
#include <cinttypes>

/** The first 16 bytes of each demofile. */
#define DEMOFILE_MAGIC "spring demofile"

/**
 * The current demofile version. Only change on major modifications for which
 * appending stuff to DemoFileHeader is not sufficient.
 */
#define DEMOFILE_VERSION 5

#pragma pack(push, 1)

/**
 * @brief Spring demo file main header
 *
 * Demo file layout is like this:
 *
 * - DemoFileHeader
 *   - Data chunks:
 *     - Startscript (scriptSize)
 *     - Demo stream (demoStreamSize)
 *     - Player statistics, one PlayerStatistic for each player
 *     - Team statistics, consisting of:
 *       - Array of numTeams dwords indicating the number of
 *         CTeam::Statistics for each team.
 *       - Array of all CTeam::Statistics (total number of items is the
 *         sum of the elements in the array of dwords).
 *
 * The header is designed to be extensible: it contains a version field and a
 * headerSize field to support this. The version field is a major version number
 * of the file format, if this changes, anything may have changed in the file.
 * It is not supposed to change often (if at all). The headerSize field is a
 * minor version number, which happens to be equal to sizeof(DemoFileHeader).
 *
 * If Spring did not cleanup properly (crashed), the demoStreamSize is 0 and it
 * can be assumed the demo stream continues until the end of the file.
 */
struct DemoFileHeader
{
	char magic[16];               ///< DEMOFILE_MAGIC
	int version;                  ///< DEMOFILE_VERSION
	int headerSize;               ///< Size of the DemoFileHeader, minor version number.
	char versionString[256];      ///< Spring version string, e.g. "0.75b2", "0.75b2+svn4123"
	std::uint8_t gameID[16];    ///< Unique game identifier. Identical for each player of the game.
	std::uint64_t unixTime;     ///< Unix time when game was started.
	int scriptSize;               ///< Size of startscript.
	int demoStreamSize;           ///< Size of the demo stream.
	int gameTime;                 ///< Total number of seconds game time.
	int wallclockTime;            ///< Total number of seconds wallclock time.
	int numPlayers;               ///< Number of players for which stats are saved. (this contains also later joined spectators!)
	int playerStatSize;           ///< Size of the entire player statistics chunk.
	int playerStatElemSize;       ///< sizeof(CPlayer::Statistics)
	int numTeams;                 ///< Number of teams for which stats are saved.
	int teamStatSize;             ///< Size of the entire team statistics chunk.
	int teamStatElemSize;         ///< sizeof(CTeam::Statistics)
	int teamStatPeriod;           ///< Interval (in seconds) between team stats.
	int winningAllyTeamsSize;     ///< The size of the vector of the winning ally teams


	/// Change structure from host endian to little endian or vice versa.
	void swab() {
		swabDWordInPlace(version);
		swabDWordInPlace(headerSize);
		// FIXME endian: unixTime = swabQWordInPlace(unixTime);
		swabDWordInPlace(scriptSize);
		swabDWordInPlace(demoStreamSize);
		swabDWordInPlace(gameTime);
		swabDWordInPlace(wallclockTime);
		swabDWordInPlace(numPlayers);
		swabDWordInPlace(playerStatSize);
		swabDWordInPlace(playerStatElemSize);
		swabDWordInPlace(numTeams);
		swabDWordInPlace(teamStatSize);
		swabDWordInPlace(teamStatElemSize);
		swabDWordInPlace(teamStatPeriod);
		swabDWordInPlace(winningAllyTeamsSize);
	}
};

/**
 * @brief Spring demo stream chunk header
 *
 * The demo stream layout is as follows:
 *
 * - DemoStreamChunkHeader
 * - length bytes raw data from network stream
 * - DemoStreamChunkHeader
 * - length bytes raw data from network stream
 * - ...
 */
struct DemoStreamChunkHeader
{
	float modGameTime;      ///< Gametime at which this chunk was written/should be read.
	std::uint32_t length; ///< Length of the chunk of data following this header.

	/// Change structure from host endian to little endian or vice versa.
	void swab() {
		swabFloatInPlace(modGameTime);
		swabDWordInPlace(length);
	}
};

#pragma pack(pop)

#endif // DEMO_FILE_H
