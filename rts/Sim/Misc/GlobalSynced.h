#ifndef GLOBAL_UNSYNCED_H
#define GLOBAL_UNSYNCED_H

#include <string>

#include "float3.h"
#include "GlobalConstants.h"
#include "creg/creg.h"


class CGameSetup;
class CTeam;
class CPlayer;

/**
 * @brief Global synced stuff
 *
 * Class contains globally accessible
 * stuff that remains synced.
 */
class CGlobalSyncedStuff
{
public:
	CR_DECLARE(CGlobalSyncedStuff);
	CGlobalSyncedStuff();  //!< Constructor
	~CGlobalSyncedStuff(); //!< Destructor
	void LoadFromSetup(const CGameSetup*);

	int    randInt();    //!< synced random int
	float  randFloat();  //!< synced random float
	float3 randVector(); //!< synced random vector

	void SetRandSeed(unsigned int seed, bool init = false) {
		randSeed = seed;
		if (init) { initRandSeed = randSeed; }
	}
	unsigned int GetRandSeed()     const { return randSeed; }
	unsigned int GetInitRandSeed() const { return initRandSeed; }

	/**
	* @brief frame number
	*
	* Stores the current frame number
	*/
	int frameNum;

	/**
	* @brief speed factor
	*
	* Contains the actual gamespeed used by the game
	*/
	float speedFactor;

	/**
	* @brief user speed factor
	*
	* Contains the user's speed factor.
	* The real gamespeed can be up to this
	* but is lowered if a computer can't keep up
	*/
	float userSpeedFactor;

	/**
	* @brief paused
	*
	* Holds whether the game is paused
	*/
	bool paused;

	/**
	* @brief map x
	*
	* The map's number of squares in the x direction
	* (note that the number of vertices is one more)
	*/
	int mapx;

	/**
	* @brief map y
	*
	* The map's number of squares in the y direction
	*/
	int mapy;

	/**
	* @brief map squares
	*
	* Total number of squares on the map
	*/
	int mapSquares;

	/**
	* @brief half map x
	*
	* Contains half of the number of squares in the x direction
	*/
	int hmapx;

	/**
	* @brief half map y
	*
	* Contains half of the number of squares in the y direction
	*/
	int hmapy;

	/**
	* @brief map x power of 2
	*
	* Map's size in the x direction rounded
	* up to the next power of 2
	*/
	int pwr2mapx;

	/**
	* @brief map y power of 2
	*
	* Map's size in the y direction rounded
	* up to the next power of 2
	*/
	int pwr2mapy;

	/**
	* @brief temp num
	*
	* Used for getting temporary but unique numbers
	* (increase after each use)
	*/
	int tempNum;

	/**
	* @brief god mode
	*
	* Whether god mode is enabled, allows all players to control all units (even specs)
	*/
	bool godMode;

	/**
	* @brief global line-of-sight
	*
	* Whether everything on the map is visible at all times
	*/
	bool globalLOS;

	/**
	* @brief cheat enabled
	*
	* Whether cheating is enabled
	*/
	bool cheatEnabled;

	/**
	* @brief disable helper AIs
	*
	* Whether helper AIs are allowed, including GroupAI and LuaUI control widgets
	*/
	bool noHelperAIs;

	/**
	* @brief definition editing enabled
	*
	* Whether definition editing is enabled
	*/
	bool editDefsEnabled;

	/**
	* @brief LuaRules control
	*
	* Whether or not LuaRules is enabled
	*/
	bool useLuaRules;

	/**
	* @brief LuaGaia control
	*
	* Whether or not LuaGaia is enabled
	*/
	bool useLuaGaia;

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
	* @brief game mode
	*
	* Determines the commander mode of this game
	*   0: game continues after commander dies
	*   1: game ends when commander dies
	*   2: lineage mode (all descendants die when a commander dies)
	*/
	int gameMode;

	/**
	* @brief players
	*
	* Array of CPlayer pointers, for all the
	* players in the game
	* (size MAX_PLAYERS)
	*/
	CPlayer* players[MAX_PLAYERS];

	/**
	* @brief active teams
	*
	* The number of active teams
	* (don't change during play)
	*/
	int activeTeams;

	/**
	* @brief active ally teams
	*
	* The number of active ally teams
	*/
	int activeAllyTeams;

	/**
	* @brief active players
	*
	* The number of active players
	*/
	int activePlayers;

	/**
	* @brief Player
	* @param name name of the player
	* @return his playernumber of -1 if not found
	*
	* Search a player by name.
	*/
	int Player(const std::string& name);

	/**
	* @brief Team
	* @param i index to fetch
	* @return CTeam pointer
	*
	* Accesses a CTeam pointer at a given index
	*/
	CTeam *Team(int i) { return teams[i]; }

	/**
	* @brief ally
	* @param a first index
	* @param b second index
	* @return allies[a][b]
	*
	* Returns ally at [a][b]
	*/
	bool Ally(int a,int b) { return allies[a][b]; }

	/**
	* @brief ally team
	* @param team team to find
	* @return the ally team
	*
	* returns the team2ally at given index
	*/
	int AllyTeam(int team) { return team2allyteam[team]; }

	/**
	* @brief allied teams
	* @param a first team
	* @param b second team
	* @return whether teams are allied
	*
	* Tests whether teams are allied
	*/
	bool AlliedTeams(int a,int b) { return allies[team2allyteam[a]][team2allyteam[b]]; }

	/**
	* @brief set ally team
	* @param team team to set
	* @param allyteam allyteam to set
	*
	* Sets team's ally team
	*/
	void SetAllyTeam(int team, int allyteam) { team2allyteam[team]=allyteam; }

	/**
	* @brief set ally
	* @param allyteamA first allyteam
	* @param allyteamB second allyteam
	* @param allied whether or not these two teams are allied
	*
	* Sets two allyteams to be allied or not
	*/
	void SetAlly(int allyteamA,int allyteamB, bool allied) { allies[allyteamA][allyteamB]=allied; }

private:
	/**
	* @brief random seed
	*
	* Holds the synced random seed
	*/
	int randSeed;

	/**
	* @brief initial random seed
	*
	* Holds the synced initial random seed
	*/
	int initRandSeed;

	/**
	* @brief allies array
	*
	* Array indicates whether teams are allied,
	* allies[a][b] means allyteam a is allied with
	* allyteam b, NOT the other way around
	*/
	bool allies[MAX_TEAMS][MAX_TEAMS];

	/**
	* @brief team to ally team
	*
	* Array stores what ally team a specific team is part of
	*/
	int team2allyteam[MAX_TEAMS];

	/**
	* @brief teams
	*
	* Array of CTeam pointers for teams in game
	*/
	CTeam* teams[MAX_TEAMS];
};

extern CGlobalSyncedStuff* gs;

#endif //GLOBAL_UNSYNCED_H
