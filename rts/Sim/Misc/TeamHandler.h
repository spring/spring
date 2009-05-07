/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#ifndef TEAMHANDLER_H
#define TEAMHANDLER_H

#include "creg/creg_cond.h"
#include "Team.h"
#include "AllyTeam.h"

class CGameSetup;


/** @brief Handles teams and allyteams */
class CTeamHandler
{
	CR_DECLARE(CTeamHandler);

public:
	CTeamHandler();
	~CTeamHandler();

	void LoadFromSetup(const CGameSetup* setup);

	/**
	 * @brief Team
	 * @param i index to fetch
	 * @return CTeam pointer
	 *
	 * Accesses a CTeam instance at a given index
	 */
	CTeam* Team(int i) { return &teams[i]; }

	/**
	 * @brief ally
	 * @param a first allyteam
	 * @param b second allyteam
	 * @return allies[a][b]
	 *
	 * Returns ally at [a][b]
	 */
	bool Ally(int a, int b) { return allyTeams[a].allies[b]; }

	/**
	 * @brief ally team
	 * @param team team to find
	 * @return the ally team
	 *
	 * returns the team2ally at given index
	 */
	int AllyTeam(int team) { return teams[team].teamAllyteam; }
	::AllyTeam& GetAllyTeam(size_t id) { return allyTeams[id]; };

	bool ValidAllyTeam(size_t id)
	{
		return id >= 0 && id < allyTeams.size();
	};

	/**
	 * @brief allied teams
	 * @param a first team
	 * @param b second team
	 * @return whether teams are allied
	 *
	 * Tests whether teams are allied
	 */
	bool AlliedTeams(int a, int b) { return allyTeams[AllyTeam(a)].allies[AllyTeam(b)]; }

	/**
	 * @brief set ally team
	 * @param team team to set
	 * @param allyteam allyteam to set
	 *
	 * Sets team's ally team
	 */
	void SetAllyTeam(int team, int allyteam) { teams[team].teamAllyteam = allyteam; }

	/**
	 * @brief set ally
	 * @param allyteamA first allyteam
	 * @param allyteamB second allyteam
	 * @param allied whether or not these two teams are allied
	 *
	 * Sets two allyteams to be allied or not
	 */
	void SetAlly(int allyteamA, int allyteamB, bool allied) { allyTeams[allyteamA].allies[allyteamB] = allied; }

	// accessors

	int GaiaTeamID() const { return gaiaTeamID; }
	int GaiaAllyTeamID() const { return gaiaAllyTeamID; }

	int ActiveTeams() const { return teams.size(); }
	int ActiveAllyTeams() const { return allyTeams.size(); }

	void GameFrame(int frameNum);

private:

	/**
	 * @brief gaia team
	 *
	 * gaia's team id
	 */
	int gaiaTeamID;

	/**
	 * @brief gaia team
	 *
	 * gaia's team id
	 */
	int gaiaAllyTeamID;

	/**
	 * @brief teams
	 *
	 * Array of CTeam instances for teams in game
	 */
	std::vector<CTeam> teams;
	std::vector< ::AllyTeam > allyTeams;
};

extern CTeamHandler* teamHandler;

#endif // !TEAMHANDLER_H
