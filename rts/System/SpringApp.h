#ifndef SPRING_APP
#define SPRING_APP

#include <string>
#include <SDL_types.h>

class BaseCmd;
class CGameController;

/**
 * @brief Spring App
 *
 * Main Spring application class launched by main()
 */
class SpringApp
{
public:
	SpringApp (); 					//!< Constructor
	~SpringApp (); 					//!< Destructor

	int Run(int argc, char *argv[]); 		//!< Run game loop

protected:
	bool Initialize (); 	//!< Initialize app
	void CheckCmdLineFile (int argc,char *argv[]); 	//!< Check command line for files
	void ParseCmdLine(); 				//!< Parse command line
	void CreateGameSetup (); 			//!< Creates GameSetup
	bool InitWindow (const char* title); 		//!< Initializes window
	void InitOpenGL (); 				//!< Initializes OpenGL
	bool SetSDLVideoMode(); 			//!< Sets SDL video mode
	void Shutdown (); 				//!< Shuts down application
	int Update (); 					//!< Run simulation and draw
	void UpdateSDLKeys (); 				//!< Update SDL key array
	bool GetDisplayGeometry();
	void SetupViewportGeometry();

	/**
	 * @brief command line
	 *
	 * Pointer to instance of commandline parser
	 */
	BaseCmd *cmdline;

	/**
	 * @brief demofile
	 *
	 * Name of a demofile
	 */
	std::string demofile;

	/**
	 * @brief savefile
	 *
	 * Name of a savefile
	 */
	std::string savefile;

	/**
	 * @brief startscript
	 *
	 * Name of a start script
	 */
	std::string startscript;

	/**
	 * @brief screen width
	 *
	 * Game screen width
	 */
	int screenWidth;

	/**
	 * @brief screen height
	 *
	 * Game screen height
	 */
	int screenHeight;

	/**
	 * @brief FSAA
	 *
	 * Level of fullscreen anti-aliasing
	 */
	bool FSAA;

private:
	static void SigAbrtHandler(int unused);
};


/**
 * @brief current active controller
 *
 * Pointer to the currently active controller
 * (could be a PreGame, could be a Game, etc)
 */
extern CGameController* activeController;

/**
 * @brief global quit
 *
 * Global boolean indicating whether the user
 * wants to quit
 */
extern bool globalQuit;

/**
 * @brief keys
 *
 * Array of possible keys, and which are being pressed
 */
extern Uint8 *keys;

/**
 * @brief fullscreen
 *
 * Whether or not the game is running in fullscreen
 */
extern bool fullscreen;

#ifdef WIN32
/**
 * Win32 only: command line passed to WinMain() (not including exe filename)
 */
extern char *win_lpCmdLine;
#endif

#endif
