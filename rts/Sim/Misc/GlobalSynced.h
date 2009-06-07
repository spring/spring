#ifndef GLOBAL_UNSYNCED_H
#define GLOBAL_UNSYNCED_H

#include <string>

#include "float3.h"
#include "creg/creg_cond.h"


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
	* Whether helper AIs are allowed, including LuaUI control widgets
	*/
	bool noHelperAIs;

	/**
	* @brief definition editing enabled
	*
	* Whether definition editing is enabled
	*/
	bool editDefsEnabled;

	/**
	* @brief LuaGaia control
	*
	* Whether or not LuaGaia is enabled
	*/
	bool useLuaGaia;

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
};

extern CGlobalSyncedStuff* gs;

#endif //GLOBAL_UNSYNCED_H
