/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RENDERING_H
#define _GLOBAL_RENDERING_H

#include "System/creg/creg_cond.h"
#include "System/Misc/SpringTime.h"
#include "System/type2.h"

struct SDL_version;
struct SDL_Window;
typedef void* SDL_GLContext;

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
	/**
	 * @return whether setting the video mode was successful
	 *
	 * Sets SDL video mode options/settings
	 */
	bool CreateWindowAndContext(const char* title, bool minimized);
	bool CreateSDLWindow(const int2& winRes, const int2& minRes, const char* title);
	bool CreateGLContext(const int2& minCtx);
	void DestroyWindowAndContext();
	void PostInit();
	void SwapBuffers(bool allowSwapBuffers, bool clearErrors);

	void CheckGLExtensions() const;
	void SetGLSupportFlags();
	void QueryVersionInfo(char (&sdlVersionStr)[64], char (&glVidMemStr)[64]);
	void QueryGLMaxVals();
	void LogVersionInfo(const char* sdlVersionStr, const char* glVidMemStr) const;
	void LogDisplayMode() const;

	void SetFullScreen(bool configFullScreen, bool cmdLineWindowed, bool cmdLineFullScreen);
	// Notify on Fullscreen/WindowBorderless change
	void ConfigNotify(const std::string& key, const std::string& value);
	void SetDualScreenParams();
	void UpdateViewPortGeometry();
	void UpdatePixelGeometry();
	void UpdateWindowState();
	void ReadWindowPosAndSize();
	void SaveWindowPosAndSize();
	void UpdateGLConfigs();

	int2 GetScreenCenter() const { return {viewPosX + (viewSizeX >> 1), viewPosY + (viewSizeY >> 1)}; }
	int2 GetWantedViewSize(const bool fullscreen);

	bool CheckGLMultiSampling() const;
	bool CheckGLContextVersion(const int2& minCtx) const;
	bool ToggleGLDebugOutput();

public:
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


	int forceDisableShaders;
	int forceCoreContext;
	int forceSwapBuffers;

	/**
	 * @brief FSAA
	 *
	 * Level of full-screen anti-aliasing
	 */
	int fsaaLevel;

	/**
	 * @brief maxTextureSize
	 *
	 * maximum 2D texture size
	 */
	int maxTextureSize;

	int gpuMemorySize;

	float maxTexAnisoLvl;


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
	bool drawdebugtraceray;

	bool glDebug;
	bool glDebugErrors;

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

	/// whether we're capturing video - relevant for frame timing
	bool isVideoCapturing;

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
	bool supportClipControl;

	/**
	 * Shader capabilities
	 */
	bool haveARB;
	bool haveGLSL;

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
	bool borderless;

public:
	SDL_Window* window;
	SDL_GLContext glContext;

public:
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

