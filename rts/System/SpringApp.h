/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_APP
#define SPRING_APP

#include <string>
#include <memory>

class ClientSetup;
class CGameController;

union SDL_Event;

namespace Threading {
	struct Error;
};

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

	static int PostKill(Threading::Error&&);
	static void Kill(bool fromRun);                 //!< Shuts down application

private:
	static void UpdateInterfaceGeometry();
	static void SaveWindowPosAndSize();

public:
	int Run();                                      //!< Run game loop

private:
	bool Init();                                    //!< Initializes engine
	bool InitWindow(const char* title);             //!< Initializes window
	bool InitPlatformLibs();
	bool InitFileSystem();
	bool MainEventHandler(const SDL_Event& ev);     //!< Handles SDL input events
	bool Update();                                  //!< Run simulation and rendering

	void ParseCmdLine(int argc, char* argv[]);      //!< Parse command line
	void Startup();                                 //!< Parses startup data (script etc.) and starts SelectMenu or PreGame
	void StartScript(const std::string& script);    //!< Starts game from specified script.txt
	void Reload(const std::string script);          //!< Returns from game back to menu, or directly starts a new game
	void LoadSpringMenu();                          //!< Load menu (old or luaified depending on start parameters)

	CGameController* RunScript(const std::string& buf);
	CGameController* LoadSaveFile(const std::string& saveName); //!< Starts game from a specified save
	CGameController* LoadDemoFile(const std::string& demoName); //!< Starts game from a specified demo

private:
	std::string inputFile;

	// this gets passed along to PreGame (or SelectMenu then PreGame),
	// and from thereon to GameServer if this client is also the host
	std::shared_ptr<ClientSetup> clientSetup;
};

/**
 * @brief current active controller
 *
 * Pointer to the currently active controller
 * (could be a PreGame, could be a Game, etc)
 */
extern CGameController* activeController;

#endif
