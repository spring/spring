/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEMO_RECORDER
#define DEMO_RECORDER

#include <vector>
#include <sstream>
#include <list>

#include "Demo.h"
#include "Game/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"

/**
 * @brief Used to record demos
 */
class CDemoRecorder : public CDemo
{
public:
	CDemoRecorder(const std::string& mapName, const std::string& modName);
	~CDemoRecorder();

	void WriteSetupText(const std::string& text);
	void SaveToDemo(const unsigned char* buf, const unsigned length, const float modGameTime);
	
	/**
	@brief assign a map name for the demo file
	*/
	void SetName(const std::string& mapname, const std::string& modname);
	const std::string& GetName() const { return demoName; }

	void SetGameID(const unsigned char* buf);
	void SetTime(int gameTime, int wallclockTime);

	void InitializeStats(int numPlayers, int numTeams );
	void SetPlayerStats(int playerNum, const PlayerStatistics& stats);
	void SetTeamStats(int teamNum, const std::list< TeamStatistics >& stats);
	void SetWinningAllyTeams(const std::vector<unsigned char>& winningAllyTeams);

private:
	#ifndef _MSC_VER
	#define CAST_SSB(b) static_cast<const spring_stringstreambuffer_t*>(b)
	typedef std::stringstream::__stringbuf_type stream_buffer_t;
	typedef stream_buffer_t::__string_type buffer_string_t;
	struct spring_stringstreambuffer_t: public stream_buffer_t {
		const buffer_string_t& str_ref() const { return _M_string; }
	};

	const buffer_string_t& GetStringFromStreamBuffer(const stream_buffer_t* buffer) const { return (CAST_SSB(buffer)->str_ref()); }
	#undef CAST_SSB
	#else
	// MS's STL probably does not use the same naming conventions
	const buffer_string_t& GetStringFromStreamBuffer(const stream_buffer_t* buffer) const { return (buffer->str()); }
	#endif		


	unsigned int WriteFileHeader(bool updateStreamLength);
	void SetFileHeader();
	void WritePlayerStats();
	void WriteTeamStats();
	void WriteWinnerList();
	void WriteDemoFile(const std::string& name, const std::string& data);

	std::stringstream demoStream;
	std::vector<PlayerStatistics> playerStats;
	std::vector< std::vector<TeamStatistics> > teamStats;
	std::vector<unsigned char> winningAllyTeams;
};


#endif

