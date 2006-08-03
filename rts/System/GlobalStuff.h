/**
 * @file GlobalStuff.h
 * @brief Globally accessible stuff
 *
 * Contains the synced and unsynced global
 * data accessed by the program
 */
#ifndef GLOBALSTUFF_H
#define GLOBALSTUFF_H

/**
 * @brief pi
 * 
 * Defines PI as a float
 */
#define PI 3.141592654f

/**
 * @brief maximum world size
 * 
 * Defines the maximum world size as 1000000
 */
#define MAX_WORLD_SIZE 1000000;

/**
 * @brief square size
 * 
 * Defines the size of 1 square as 8
 */
#define SQUARE_SIZE 8

/**
 * @brief game speed
 * 
 * Defines the game speed as 30
 */
#define GAME_SPEED 30

/**
 * @brief randint max
 * 
 * Defines the maximum random integer as 0x7fff
 */
#define RANDINT_MAX 0x7fff

/**
 * @brief max view range
 * 
 * Defines the maximum view range as 8000
 */
#define MAX_VIEW_RANGE 8000

/**
 * @brief max teams
 * 
 * Defines the maximum number of teams
 * as 17 (team 16 is the GAIA team)
 */
#define MAX_TEAMS 17

/**
 * @brief max players
 * 
 * Defines the maximum number of players as 32
 */
#define MAX_PLAYERS 32

/**
 * @brief near plane
 * 
 * Defines the near plane as 2.8f
 */
#define NEAR_PLANE 2.8f

void SendChat(char* c);

#include <list>
#include "float3.h"

/**
 * @brief int2 struct
 * 
 * Struct containing two integers
 */
struct int2 {
	CR_DECLARE_STRUCT(int2);
	
	/**
	 * @brief Constructor
	 * 
	 * Does nothing
	 */
	int2(){};

	/**
	 * @brief Constructor
	 * @param x x value to use
	 * @param y y value to use
	 * 
	 * Initializes struct with a
	 * given x and y
	 */
	int2(int x,int y):x(x),y(y){};

	int x; //!< x component of int2
	int y; //!< y component of int2
};

/**
 * @brief float2 struct
 * 
 * Struct containing two floats
 */
struct float2 {
	CR_DECLARE_STRUCT(float2);
	float x; //!< x component of float2
	float y; //!< y component of float2
};

/**
 * @brief upwards vector
 * 
 * Defines constant upwards vector
 * (0,1,0)
 */
const float3 UpVector(0,1,0);

/**
 * @brief zero vector
 * 
 * Defines constant zero vector
 * (0,0,0)
 */
const float3 ZeroVector(0,0,0);

class CTeam;
class CUnit;
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
	CGlobalSyncedStuff(); 		//!< Constructor
	virtual ~CGlobalSyncedStuff(); 	//!< Destructor

	int randInt();			//!< synced random int
	float randFloat(); 		//!< synced random float
	float3 randVector(); 		//!< synced random vector

	/**
	 * @brief random seed
	 * 
	 * Holds the synced random seed
	 */
	int randSeed;

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
	 * @brief cheat enabled
	 * 
	 * Whether cheating is enabled
	 */
	bool cheatEnabled;

	/**
	 * @brief game mode
	 * 
	 * Determines the commander mode of this game
	 * (0 means game continues after commander dies,
	 * 1 means game ends when commander dies)
	 */
	int gameMode;

	/**
	 * @brief gravity
	 * 
	 * Stores the gravity as a negative number
	 * and in units/frame^2
	 * (NOT positive units/second^2 as in the mapfile)
	 */
	float gravity;

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
	 * @brief sun vector
	 * 
	 * Holds vector for the direction of the sun
	 */
	float3 sunVector;

	/**
	 * @brief sun vector4
	 * 
	 * Holds vector for the sun as 4 components
	 */
	float sunVector4[4];

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
	 * @param teamA first team
	 * @param teamB second team
	 * @param allied whether or not these two teams are allied
	 * 
	 * Sets two teams to be allied or not
	 */
	void SetAlly(int teamA,int teamB, bool allied) { allies[teamA][teamB]=allied; }

protected:
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

/**
 * @brief Global unsynced stuff
 * 
 * Class containing globally accessible
 * stuff that does not remain synced
 */
class CGlobalUnsyncedStuff  
{
public:
	CGlobalUnsyncedStuff(); 		//!< Constructor
	virtual ~CGlobalUnsyncedStuff(); 	//!< Destructor

	int usRandInt(); 			//!< Unsynced random int
	float usRandFloat(); 			//!< Unsynced random float
	float3 usRandVector(); 			//!< Unsynced random vector

	/**
	 * @brief rand seed
	 * 
	 * Stores the unsynced random seed
	 */
	int usRandSeed;

	/**
	 * @brief mod game time
	 * 
	 * How long the game has been going on
	 * (modified with game's speed factor)
	 */
	double modGameTime;

	/**
	 * @brief game time
	 * 
	 * How long the game has been going on
	 * in real time
	 */
	double gameTime;

	/**
	 * @brief last frame time
	 * 
	 * How long the last draw cycle took in real time
	 */
	float lastFrameTime;

	/**
	 * @brief screen x
	 * 
	 * X size of player's game window
	 */
	int screenx;

	/**
	 * @brief screen y
	 * 
	 * Y size of player's game window
	 */
	int screeny;

	/**
	 * @brief my player num
	 * 
	 * Local player's number
	 */
	int myPlayerNum;

	/**
	 * @brief my team
	 * 
	 * Local player's team
	 */
	int myTeam;

	/**
	 * @brief my ally team
	 * 
	 * Local player's ally team
	 */
	int myAllyTeam;

	/**
	 * @brief spectating
	 * 
	 * Whether this player is spectating
	 * (can see everything but can't give orders)
	 */
	bool spectating;

	/**
	 * @brief draw debug
	 * 
	 * Whether debugging info is drawn
	 */
	bool drawdebug;

	/**
	 * @brief active video
	 * 
	 * Whether the graphics need to be drawn
	 */
	bool active;

	/**
	 * @brief view range
	 * 
	 * Player's view range
	 */
	float viewRange;

	/**
	 * @brief time offset
	 * 
	 * Time in number of frames since last update
	 * (for interpolation)
	 */
	float timeOffset;

	/**
	 * @brief draw fog
	 * 
	 * Whether fog (of war) is drawn or not
	 */
	bool drawFog;

	/**
	 * @brief team coloured nanospray?
	 */
	bool team_nanospray;

	/**
	 * @brief quit automatically?
	 *
	 * If set, quit immediately on game over or if gameTime > quitTime,
	 * whichever comes first.
	 */
	bool autoQuit;

	/**
	 * @brief quit time
	 *
	 * If autoQuit is set, the host quits if gameTime > quitTime.
	 * (This automatically causes all other clients to quit too.)
	 */
	float quitTime;

#ifdef DIRECT_CONTROL_ALLOWED
	/**
	 * @brief direct control
	 * 
	 * Pointer to unit being directly controlled by
	 * this player
	 */
	CUnit* directControl;
#endif

};

extern CGlobalSyncedStuff* gs;
extern CGlobalUnsyncedStuff* gu;
extern bool fullscreen;

#endif /* GLOBALSTUFF_H */
