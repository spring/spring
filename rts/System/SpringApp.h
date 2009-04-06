#ifndef SPRING_APP
#define SPRING_APP

#include <string>


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
	void Startup (); 		//!< Parses startup data (script etc.) and starts SelectMenu or PreGame
	bool InitWindow (const char* title); 		//!< Initializes window
	void InitOpenGL (); 				//!< Initializes OpenGL
	void LoadFonts();
	bool SetSDLVideoMode(); 			//!< Sets SDL video mode
	void Shutdown (); 				//!< Shuts down application
	int Update (); 					//!< Run simulation and draw
#if defined(USE_GML) && GML_ENABLE_SIM
	int Sim (); 					//!< Simulation  loop
	static void Simcb(void *c) {((SpringApp *)c)->Sim();}
	volatile int gmlKeepRunning;
#endif
	void UpdateSDLKeys (); 				//!< Update SDL key array
	bool GetDisplayGeometry();
	void SetupViewportGeometry();
	void SaveWindowGeometry();

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
	 * @brief window position - X
	 *
	 * Game window position from the left of the screen (if not fullscreen)
	 */
	int windowPosX;

	/**
	 * @brief window position - Y
	 *
	 * Game window position from the top of the screen (if not fullscreen)
	 */
	int windowPosY;

	/**
	 * @brief FSAA
	 *
	 * Level of fullscreen anti-aliasing
	 */
	bool FSAA;

	/**
	 * @brief depthBufferBits
	 *
	 * number of Depthbuffer bits
	 */
	bool depthBufferBits;

	/**
	 * @brief last required draw
	 *
	 * sim frame after which the last required draw was conducted
	 */
	int lastRequiredDraw;

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
extern boost::uint8_t *keys;

/**
 * @brief currentUnicode
 *
 * Unicode character for the current KeyPressed or KeyReleased
 */
extern boost::uint16_t currentUnicode;

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
