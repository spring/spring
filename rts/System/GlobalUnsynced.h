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


class CUnit;

/**
 * @brief Global unsynced stuff
 *
 * Class containing globally accessible
 * stuff that does not remain synced
 */
class CGlobalUnsyncedStuff
{
	CR_DECLARE(CGlobalUnsyncedStuff);

public:
	CGlobalUnsyncedStuff();  //!< Constructor
	~CGlobalUnsyncedStuff(); //!< Destructor

	int    usRandInt();    //!< Unsynced random int
	float  usRandFloat();  //!< Unsynced random float
	float3 usRandVector(); //!< Unsynced random vector

	void SetMyPlayer(const int mynumber);

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

	// the starting time in tick for last draw frame
	unsigned lastFrameStart;

	// 0.001f * GAME_SPEED * gs->speedFactor, used for rendering
	float weightedSpeedFactor;

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
	 * @brief if the GPU (drivers) support NonPowerOfTwoTextures
	 *
	 * Especially some ATI cards report that they support NPOTs, but they don't (or just very limited).
	 */
	bool supportNPOTs;

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

	/**
	 * @brief direct control
	 *
	 * Pointer to unit being directly controlled by
	 * this player
	 */
	CUnit* directControl;

private:
	/**
	* @brief rand seed
	*
	* Stores the unsynced random seed
	*/
	int usRandSeed;

};

extern CGlobalUnsyncedStuff* gu;
extern bool fullscreen;

#endif /* GLOBALSTUFF_H */
