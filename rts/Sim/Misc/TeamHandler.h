/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#ifndef TEAMHANDLER_H
#define TEAMHANDLER_H

#include "creg/creg.h"
#include "Team.h"

class CGameSetup;


/** @brief Handles teams and allyteams */
class CTeamHandler
{
public:
	CR_DECLARE(CTeamHandler);

	CTeamHandler();
	~CTeamHandler();

	void LoadFromSetup(const CGameSetup* setup);

	// from CGlobalSynced:

	/**
	 * @brief Team
	 * @param i index to fetch
	 * @return CTeam pointer
	 *
	 * Accesses a CTeam instance at a given index
	 */
	CTeam* Team(int i);

	/**
	 * @brief ally
	 * @param a first allyteam
	 * @param b second allyteam
	 * @return allies[a][b]
	 *
	 * Returns ally at [a][b]
	 */
	bool Ally(int a, int b);

	/**
	 * @brief ally team
	 * @param team team to find
	 * @return the ally team
	 *
	 * returns the team2ally at given index
	 */
	int AllyTeam(int team);

	/**
	 * @brief allied teams
	 * @param a first team
	 * @param b second team
	 * @return whether teams are allied
	 *
	 * Tests whether teams are allied
	 */
	bool AlliedTeams(int a, int b);

	/**
	 * @brief set ally team
	 * @param team team to set
	 * @param allyteam allyteam to set
	 *
	 * Sets team's ally team
	 */
	void SetAllyTeam(int team, int allyteam);

	/**
	 * @brief set ally
	 * @param allyteamA first allyteam
	 * @param allyteamB second allyteam
	 * @param allied whether or not these two teams are allied
	 *
	 * Sets two allyteams to be allied or not
	 */
	void SetAlly(int allyteamA, int allyteamB, bool allied);

	// accessors

	int GaiaTeamID() const;
	int GaiaAllyTeamID() const;

	int ActiveTeams() const;
	int ActiveAllyTeams() const;

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
	 * @brief allies array
	 *
	 * Array indicates whether teams are allied,
	 * allies[a][b] means allyteam a is allied with
	 * allyteam b, NOT the other way around
	 */
	std::vector< std::vector<bool> > allies;

	/**
	 * @brief team to ally team
	 *
	 * Array stores what ally team a specific team is part of
	 */
	std::vector<int> team2allyteam;

	/**
	 * @brief teams
	 *
	 * Array of CTeam instances for teams in game
	 */
	std::vector<CTeam> teams;
};

extern CTeamHandler* teamHandler;

#endif // !TEAMHANDLER_H
