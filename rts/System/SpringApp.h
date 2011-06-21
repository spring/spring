/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_APP
#define SPRING_APP

#include <string>
#include <boost/cstdint.hpp>

class CmdLineParams;
class CGameController;
class COffscreenGLContext;
union SDL_Event;

/**
 * @brief Spring App
 *
 * Main Spring application class launched by main()
 */
class SpringApp
{
public:
	SpringApp();
	~SpringApp();

	int Run(int argc, char *argv[]);                //!< Run game loop
	static void Shutdown();                         //!< Shuts down application

protected:
	bool Initialize();                              //!< Initialize app
	void ParseCmdLine();                            //!< Parse command line
	void Startup();                                 //!< Parses startup data (script etc.) and starts SelectMenu or PreGame
	bool InitWindow(const char* title);             //!< Initializes window
	static void InitOpenGL();                       //!< Initializes OpenGL
	static void UpdateOldConfigs();                 //!< Forces an update to new config defaults
	static void LoadFonts();                        //!< Initialize glFonts (font & smallFont)
	static bool SetSDLVideoMode();                  //!< Sets SDL video mode
	static void SetProcessAffinity(int);
	int Update();                                   //!< Run simulation and draw
	bool UpdateSim(CGameController *ac);

	static bool GetDisplayGeometry();
	static void SetupViewportGeometry();
	static void RestoreWindowPosition();
	static void SaveWindowPosition();

#if defined(USE_GML) && GML_ENABLE_SIM
	int Sim();                                      //!< Simulation  loop
	static void Simcb(void *c) {((SpringApp *)c)->Sim();}
	static volatile int gmlKeepRunning;
#endif

	/**
	 * @brief command line
	 *
	 * Pointer to instance of commandline parser
	 */
	CmdLineParams* cmdline;

	/**
	 * @brief last required draw
	 *
	 * sim frame after which the last required draw was conducted
	 */
	int lastRequiredDraw;

	static COffscreenGLContext* ogc;

private:
	bool MainEventHandler(const SDL_Event& ev);
};

/**
 * @brief current active controller
 *
 * Pointer to the currently active controller
 * (could be a PreGame, could be a Game, etc)
 */
extern CGameController* activeController;

#endif
