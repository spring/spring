/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_APP
#define SPRING_APP

#include <string>
#include <memory>

class ClientSetup;
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
	void Reload(const std::string& script);

	static void ShutDown();                         //!< Shuts down application

private:
	bool Initialize();                              //!< Initialize app
	void ParseCmdLine(int argc, char* argv[]);          //!< Parse command line
	void Startup();                                 //!< Parses startup data (script etc.) and starts SelectMenu or PreGame
	void StartScript(const std::string& script); //!< Starts game from specified script.txt
	void LoadSpringMenu();                          //!< Load menu (old or luaified depending on start parameters)
	bool InitWindow(const char* title);             //!< Initializes window

	int Update();                                   //!< Run simulation and draw

	static void InitOpenGL();                       //!< Initializes OpenGL
	static void LoadFonts();                        //!< Initialize glFonts (font & smallFont)

	static void GetDisplayGeometry();
	static void SetupViewportGeometry();
	static void SaveWindowPosition();

	std::string inputFile;

	// this gets passed along to PreGame (or SelectMenu then PreGame),
	// and from thereon to GameServer if this client is also the host
	std::shared_ptr<ClientSetup> clientSetup;

private:
	bool MainEventHandler(const SDL_Event& ev);

	CGameController* RunScript(const std::string& buf);
	CGameController* LoadSaveFile(const std::string& saveName); //!< Starts game from a specified save
	CGameController* LoadDemoFile(const std::string& demoName); //!< Starts game from a specified save
};

/**
 * @brief current active controller
 *
 * Pointer to the currently active controller
 * (could be a PreGame, could be a Game, etc)
 */
extern CGameController* activeController;

#endif
