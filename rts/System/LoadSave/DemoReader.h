/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEMO_READER
#define DEMO_READER

#include <fstream>
#include <vector>

#include "Demo.h"

#include "Game/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"

namespace netcode { class RawPacket; }

/**
 * @brief Utility class for reading demofiles
 */
class CDemoReader : public CDemo
{
public:
	/**
	@brief Open a demofile for reading
	@throw std::runtime_error Demofile not found / header corrupt / outdated
	*/
	CDemoReader(const std::string& filename, float curTime);
	
	/**
	@brief read from demo file
	@return The data read (or 0 if no data), don't forget to delete it
	*/
	netcode::RawPacket* GetData(float curTime);

	/**
	@brief Wether the demo has reached the end
	@return true when end reached, false when there is still stuff to read
	*/
	bool ReachedEnd() const;

	float GetModGameTime() const { return chunkHeader.modGameTime; }
	float GetDemoTimeOffset() const { return demoTimeOffset; }
	float GetNextDemoReadTime() const { return nextDemoReadTime; }

	const std::string& GetSetupScript() const
	{
		return setupScript;
	};
	
	const std::vector<PlayerStatistics>& GetPlayerStats() const;
	const std::vector< std::vector<TeamStatistics> >& GetTeamStats() const;

	/// Not needed for normal demo watching
	void LoadStats();

private:
	std::ifstream playbackDemo;
	float demoTimeOffset;
	float nextDemoReadTime;
	int bytesRemaining;
	DemoStreamChunkHeader chunkHeader;
	std::string setupScript;	// the original, unaltered version from script

	std::vector<PlayerStatistics> playerStats;
	std::vector< std::vector<TeamStatistics> > teamStats;
};

#endif

