/**
 * @file GlobalStuff.h
 * @brief Globally accessible stuff
 *
 * Contains the synced and unsynced global
 * data accessed by the program
 */
#ifndef GLOBALSTUFF_H
#define GLOBALSTUFF_H

#include "float3.h"

/**
 * @brief pi
 *
 * Defines PI as a float
 */
const float PI = 3.141592654f;

/**
 * @brief maximum world size
 *
 * Defines the maximum world size as 1000000
 */
const int MAX_WORLD_SIZE = 1000000;

/**
 * @brief square size
 *
 * Defines the size of 1 square as 8
 */
const int  SQUARE_SIZE = 8;

/**
 * @brief game speed
 *
 * Defines the game speed as 30
 */
const int GAME_SPEED = 30;

/**
 * @brief max view range
 *
 * Defines the maximum view range as 8000
 */
const int MAX_VIEW_RANGE = 8000;

/**
 * @brief max teams
 *
 * Defines the maximum number of teams
 * as 17 (16 real teams, and an extra slot for the GAIA team)
 */
const int MAX_TEAMS = 17;

/**
 * @brief max players
 *
 * Defines the maximum number of players as 32
 */
const int MAX_PLAYERS = 32;

/**
 * @brief near plane
 *
 * Defines the near plane as 2.8f
 */
const float NEAR_PLANE = 2.8f;

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
class LuaParser;
class CGameSetup;

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

/**
 * @brief Global unsynced stuff
 *
 * Class containing globally accessible
 * stuff that does not remain synced
 */
class CGlobalUnsyncedStuff
{
public:
	CR_DECLARE(CGlobalUnsyncedStuff);
	CGlobalUnsyncedStuff();  //!< Constructor
	~CGlobalUnsyncedStuff(); //!< Destructor

	int    usRandInt();    //!< Unsynced random int
	float  usRandFloat();  //!< Unsynced random float
	float3 usRandVector(); //!< Unsynced random vector

	void LoadFromSetup(const CGameSetup*);

	/**
	 * Does the user want team colored nanospray if the mod allows it?
	 */
	bool teamNanospray;

	/**
	 * @brief mod game time
	 *
	 * How long the game has been going on
	 * (modified with game's speed factor)
	 */
	float modGameTime;

	/**
	 * @brief game time
	 *
	 * How long the game has been going on
	 * in real time
	 */
	float gameTime;

	/**
	 * @brief last frame time
	 *
	 * How long the last draw cycle took in real time
	 */
	float lastFrameTime;

	// the draw frame number (never 0)
	unsigned int drawFrame;

	// the screen size in pixels
	int screenSizeX;
	int screenSizeY;

	// the window position relative to the screen's bottom-left corner
	int winPosX;
	int winPosY;

	// the window size in pixels
	int winSizeX;
	int winSizeY;

	// the viewport position relative to the window's bottom-left corner
	int viewPosX;
	int viewPosY;

	// the viewport size in pixels
	int viewSizeX;
	int viewSizeY;

	// size of one pixel in viewport coordinates, i.e. 1/viewSizeX and 1/viewSizeY
	float pixelX;
	float pixelY;

	/**
	 * @brief aspect ratio
	 *
	 * (float)viewSizeX / (float)viewSizeY
	 */
	float aspectRatio;

	/**
	 * @brief Depthbuffer bits
	 *
	 * depthbuffer precision
	 */
	int depthBufferBits;

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
	 * (can't give orders, and can see everything if spectateFullView is true)
	 */
	bool spectating;

	/**
	 * @brief spectatingFullView
	 *
	 * Whether this player is a spectator, and can see everything
	 * (if set to false, visibility is determined by the current team)
	 */
	bool spectatingFullView;

	/**
	 * @brief spectatingFullSelect
	 *
	 * Can all units be selected when spectating?
	 */
	bool spectatingFullSelect;

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
	 * @brief compressTextures
	 *
	 * If set, many (not all) textures will compressed on run-time.
	*/
	bool compressTextures;

	/**
	 * @brief collection of some ATI bugfixes
	 *
	 * enables some ATI bugfixes
	 */
	bool atiHacks;

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

	/**
	 * @brief dual screen mode
	 * In dual screen mode, the screen is split up between a game screen and a minimap screen.
	 * In this case viewSizeX is half of the actual GL viewport width,
	 */
	bool dualScreenMode;

	/**
	 * @brief dual screen minimap on left
	 * In dual screen mode, allow the minimap to either be shown on the left or the right display.
	 * Minimap on the right is the default.
	 */
	bool dualScreenMiniMapOnLeft;

#ifdef DIRECT_CONTROL_ALLOWED
	/**
	 * @brief direct control
	 *
	 * Pointer to unit being directly controlled by
	 * this player
	 */
	CUnit* directControl;
#endif

private:
	/**
	* @brief rand seed
	*
	* Stores the unsynced random seed
	*/
	int usRandSeed;

};

extern CGlobalSyncedStuff* gs;
extern CGlobalUnsyncedStuff* gu;
extern bool fullscreen;

#endif /* GLOBALSTUFF_H */
