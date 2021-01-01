/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on code from GlobalSynced.{cpp,h} */

#ifndef TEAMHANDLER_H
#define TEAMHANDLER_H

#include "System/creg/creg_cond.h"
#include "Team.h"
#include "AllyTeam.h"

class CGameSetup;


/** @brief Handles teams and allyteams */
class CTeamHandler
{
	CR_DECLARE_STRUCT(CTeamHandler)

public:
	void LoadFromSetup(const CGameSetup* setup);
	void SetDefaultStartPositions(const CGameSetup* setup);
	void ResetState() {
		teams.clear();
		allyTeams.clear();

		gaiaTeamID = -1;
		gaiaAllyTeamID = -1;
	}

	/**
	 * @brief Team
	 * @param i index to fetch
	 * @return CTeam pointer
	 *
	 * Accesses a CTeam instance at a given index
	 */
	CTeam* Team(int i)
	{
		assert(i >=            0);
		assert(i <  teams.size());
		return &teams[i];
	}

	/**
	 * @brief ally
	 * @param a first allyteam
	 * @param b second allyteam
	 * @return allies[a][b]
	 *
	 * Returns ally at [a][b]
	 */
	bool Ally(int a, int b) const { return allyTeams[a].allies[b]; }

	/**
	 * @brief ally team
	 * @param team team to find
	 * @return the ally team
	 *
	 * returns the team2ally at given index
	 */
	int AllyTeam(int team) const { return teams[team].teamAllyteam; }
	::AllyTeam& GetAllyTeam(size_t id) { return allyTeams[id]; };

	bool ValidAllyTeam(size_t id) const
	{
		return (id < allyTeams.size());
	}

	/**
	 * @brief allied teams
	 * @param a first team
	 * @param b second team
	 * @return whether teams are allied
	 *
	 * Tests whether teams are allied
	 */
	bool AlliedTeams(int teamA, int teamB) const { return allyTeams[AllyTeam(teamA)].allies[AllyTeam(teamB)]; }

	/**
	 * @brief allied allyteams
	 * @param a first allyteam
	 * @param b second allyteam
	 * @return whether allyteams are allied
	 *
	 * Tests whether allyteams are allied
	 */
	bool AlliedAllyTeams(int allyA, int allyB) const { return allyTeams[allyA].allies[allyB]; }


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

	// number of teams and allyteams that were *INITIALLY* part
	// of a game (teams.size() and allyTeams.size() are runtime
	// constants), ie. including teams that died during it and
	// are actually NO LONGER "active"
	//
	// NOTE: TEAM INSTANCES ARE NEVER DELETED UNTIL SHUTDOWN, THEY ONLY GET MARKED AS DEAD!
	int ActiveTeams() const { return teams.size(); }
	int ActiveAllyTeams() const { return allyTeams.size(); }

	bool IsValidTeam(int id) const {
		return ((id >= 0) && (id < ActiveTeams()));
	}
	bool IsActiveTeam(int id) const {
		return (IsValidTeam(id) && !teams[id].isDead);
	}

	bool IsValidAllyTeam(int id) const {
		return ((id >= 0) && (id < ActiveAllyTeams()));
	}
	bool IsActiveAllyTeam(int id) const {
		return (IsValidAllyTeam(id) /*&& !allyTeams[id].isDead*/);
	}

	unsigned int GetNumTeamsInAllyTeam(unsigned int allyTeam, bool countDeadTeams) const;

	void GameFrame(int frameNum);

	void UpdateTeamUnitLimitsPreSpawn(int liveTeamNum);
	void UpdateTeamUnitLimitsPreDeath(int deadTeamNum);

private:

	/**
	 * @brief gaia team
	 *
	 * gaia's team id
	 */
	int gaiaTeamID = -1;

	/**
	 * @brief gaia team
	 *
	 * gaia's team id
	 */
	int gaiaAllyTeamID = -1;

	/**
	 * @brief teams
	 *
	 * Array of CTeam instances for teams in game
	 */
	std::vector<CTeam> teams;
	std::vector< ::AllyTeam > allyTeams;
};

extern CTeamHandler teamHandler;

#endif // !TEAMHANDLER_H
