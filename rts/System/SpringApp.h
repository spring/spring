/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_APP
#define SPRING_APP

#include <string>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>


class ClientSetup;
class CmdLineParams;
class CGameController;
union SDL_Event;

/**
 * @brief Spring App
 *
 * Main Spring application class launched by main()
 */
class SpringApp
{
public:
	SpringApp(int argc, char** argv);
	~SpringApp();

	int Run();                                      //!< Run game loop
	static void ShutDown();                         //!< Shuts down application

protected:
	bool Initialize();                              //!< Initialize app
	void ParseCmdLine(const std::string&);          //!< Parse command line
	void Startup();                                 //!< Parses startup data (script etc.) and starts SelectMenu or PreGame
	bool InitWindow(const char* title);             //!< Initializes window
	static void InitOpenGL();                       //!< Initializes OpenGL
	static void LoadFonts();                        //!< Initialize glFonts (font & smallFont)
	static bool CreateSDLWindow(const char* title);    //!< Creates a SDL window
	int Update();                                   //!< Run simulation and draw
	bool UpdateSim(CGameController *ac);

	static void GetDisplayGeometry();
	static void SetupViewportGeometry();
	static void SaveWindowPosition();

	/**
	 * @brief command line
	 *
	 * Pointer to instance of commandline parser
	 */
	CmdLineParams* cmdline;

private:
	bool MainEventHandler(const SDL_Event& ev);
	void RunScript(boost::shared_ptr<ClientSetup> clientSetup, const std::string& buf);
};

/**
 * @brief current active controller
 *
 * Pointer to the currently active controller
 * (could be a PreGame, could be a Game, etc)
 */
extern CGameController* activeController;

#endif
