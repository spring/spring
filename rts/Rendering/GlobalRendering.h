/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RENDERING_H
#define _GLOBAL_RENDERING_H

#include "System/creg/creg_cond.h"
#include "System/Misc/SpringTime.h"
#include "System/type2.h"

struct SDL_Window;

/**
 * @brief Globally accessible unsynced, rendering related data
 *
 * Contains globally accessible rendering related data
 * that does not remain synced.
 */
class CGlobalRendering {
	CR_DECLARE_STRUCT(CGlobalRendering)

public:
	CGlobalRendering();
	~CGlobalRendering();
	void PostInit();
	void SetFullScreen(bool configFullScreen, bool cmdLineWindowed, bool cmdLineFullScreen);
	// Notify on Fullscreen/WindowBorderless change
	void ConfigNotify(const std::string& key, const std::string& value);
	void SetDualScreenParams();
	void UpdateViewPortGeometry();
	void UpdatePixelGeometry();
	int2 GetWantedViewSize(const bool fullscreen);

	bool SetGetDrawDebug(bool dbg) { const bool ret =       dbg; drawdebug = dbg; return ret; }
	bool GetSetDrawDebug(bool dbg) { const bool ret = drawdebug; drawdebug = dbg; return ret; }

	/**
	 * @brief time offset
	 *
	 * Time in number of frames since last update
	 * (for interpolation)
	 */
	float timeOffset;

	/**
	 * @brief last frame time
	 *
	 * How long the last draw cycle took in real time (MILLIseconds)
	 */
	float lastFrameTime;

	/// the starting time in tick for last draw frame
	spring_time lastFrameStart;

	/// 0.001f * gu->simFPS, used for rendering
	float weightedSpeedFactor;

	/// the draw frame number (never 0)
	unsigned int drawFrame;

	/// Frames Per Second
	float FPS;

	/// the window state (0=normal,1=maximized,2=minimized)
	enum {
		WINSTATE_DEFAULT   = 0,
		WINSTATE_MAXIMIZED = 1,
		WINSTATE_MINIMIZED = 2
	};
	int winState;

	/// the screen size in pixels
	int screenSizeX;
	int screenSizeY;

	/// the window position relative to the screen's bottom-left corner
	int winPosX;
	int winPosY;

	/// the window size in pixels
	int winSizeX;
	int winSizeY;

	/// the viewport position relative to the window's bottom-left corner
	int viewPosX;
	int viewPosY;

	/// the viewport size in pixels
	int viewSizeX;
	int viewSizeY;

	/// size of one pixel in viewport coordinates, i.e. 1/viewSizeX and 1/viewSizeY
	float pixelX;
	float pixelY;

	/**
	 * @brief aspect ratio
	 *
	 * (float)viewSizeX / (float)viewSizeY
	 */
	float aspectRatio;

	/**
	 * @brief view range
	 *
	 * Player's view range
	 */
	float zNear;
	float viewRange;


	/**
	 * @brief FSAA
	 *
	 * Level of full-screen anti-aliasing
	 */
	int FSAA;

	/**
	 * @brief maxTextureSize
	 *
	 * maximum 2D texture size
	 */
	int maxTextureSize;

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
	 * @brief draw debug
	 *
	 * Whether debugging info is drawn
	 */
	bool drawdebugtraceray;

	/**
	 * Does the user want team colored nanospray?
	 */
	bool teamNanospray;


	/**
	 * @brief active video
	 *
	 * Whether the graphics need to be drawn
	 */
	bool active;

	/**
	 * @brief compressTextures
	 *
	 * If set, many (not all) textures will compressed on run-time.
	*/
	bool compressTextures;

	/**
	 * @brief GPU driver's vendor
	 *
	 * These can be used to enable workarounds for bugs in their drivers.
	 * Note, you should always give the user the possiblity to override such workarounds via config-tags.
	 */
	bool haveATI;
	bool haveMesa;
	bool haveIntel;
	bool haveNvidia;

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
	 * @brief support24bitDepthBuffers
	 *
	 * if GL_DEPTH_COMPONENT24 is supported (many ATIs don't do so)
	 */
	bool support24bitDepthBuffers;

	bool supportRestartPrimitive;

	/**
	 * Shader capabilities
	 */
	bool haveARB;
	bool haveGLSL;

	/**
	 * @brief maxSmoothPointSize
	 *
	 * maximum smooth point size (driver might fallback in software rendering if larger)
	 */
	float maxSmoothPointSize;

	/**
	 * Shader capabilities
	 */
	int glslMaxVaryings;
	int glslMaxAttributes;
	int glslMaxDrawBuffers;
	int glslMaxRecommendedIndices;
	int glslMaxRecommendedVertices;
	int glslMaxUniformBufferBindings;
	int glslMaxUniformBufferSize; ///< in bytes

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
	 * @brief full-screen or windowed rendering
	 */
	bool fullScreen;

	SDL_Window* window;

	/**
	* @brief max view range in elmos
	*/
	static const float MAX_VIEW_RANGE;

	/**
	* @brief near z-plane distance in elmos
	*/
	static const float NEAR_PLANE;


	/// magic constant to reduce overblending on SMF maps
	/// (scales the MapInfo::light_t::ground*Color values)
	static const float SMF_INTENSITY_MULT;


	static const int minWinSizeX;
	static const int minWinSizeY;
};

extern CGlobalRendering* globalRendering;

#endif /* _GLOBAL_RENDERING_H */
