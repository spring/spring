/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GLOBALSTUFF_H
#define GLOBALSTUFF_H

#include "float3.h"


class CUnit;

/**
 * @brief Globally accessible unsynced data
 *
 * Contains globally accessible data that does not remain synced
 */
class CGlobalUnsynced
{
	CR_DECLARE(CGlobalUnsynced);

public:
	CGlobalUnsynced();
	void PostInit();

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


	bool drawSky;
	bool drawWater;
	bool drawGround;
	bool drawMapMarks;
	/**
	 * @brief draw fog
	 *
	 * Whether fog (of war) is drawn or not
	 */
	bool drawFog;

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
	bool haveATI;
	bool atiHacks;

	/**
	 * @brief if the GPU (drivers) support NonPowerOfTwoTextures
	 *
	 * Especially some ATI cards report that they support NPOTs, but they don't (or just very limited).
	 */
	bool supportNPOTs;
	bool haveARB;
	bool haveGLSL;

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

	bool moveWarnings;
	bool buildWarnings;

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

extern CGlobalUnsynced* gu;
extern bool fullscreen;

#endif /* GLOBALSTUFF_H */
