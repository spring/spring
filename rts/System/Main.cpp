/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
	\mainpage
	This is the documentation of the Spring RTS Engine.
	http://springrts.com/
*/


#include "System/SpringApp.h"

#include "lib/gml/gml_base.h"
#include "lib/gml/gmlmut.h"
#include "System/Exceptions.h"
#include "System/Platform/EngineTypeHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Misc.h"
#include "System/Log/ILog.h"


#if !defined(__APPLE__) || !defined(HEADLESS)
	// SDL_main.h contains a macro that replaces the main function on some OS, see SDL_main.h for details
	#include <SDL_main.h>
#endif

#ifdef WIN32
	#include "lib/SOP/SOP.hpp" // NvOptimus
	#include "System/FileSystem/FileSystem.h"
	#include <stdlib.h>
	#include <process.h>
	#define setenv(k,v,o) SetEnvironmentVariable(k,v)
#endif




int Run(int argc, char* argv[])
{
	int ret = -1;

#ifdef __MINGW32__
	// For the MinGW backtrace() implementation we need to know the stack end.
	{
		extern void* stack_end;
		char here;
		stack_end = (void*) &here;
	}
#endif

	Threading::DetectCores();
	Threading::SetMainThread();

#ifdef USE_GML
	GML::ThreadNumber(GML_DRAW_THREAD_NUM);
  #if GML_ENABLE_TLS_CHECK
	if (GML::ThreadNumber() != GML_DRAW_THREAD_NUM) { //XXX how does this check relate to TLS??? and how does it relate to the line above???
		ErrorMessageBox("Thread Local Storage test failed", "GML error:", MBF_OK | MBF_EXCL);
	}
  #endif
#endif

	// run
	try {
		SpringApp app;
		ret = app.Run(argc, argv);
	} CATCH_SPRING_ERRORS

	// check if Spring crashed, if so display an error message
	Threading::Error* err = Threading::GetThreadError();
	if (err)
		ErrorMessageBox("Error in main(): " + err->message, err->caption, err->flags);

	return ret;
}


/**
 * Set some performance relevant OpenMP EnvVars.
 * @return true when restart is required with new env vars
 */
static bool SetOpenMpEnvVars(char* argv[])
{
	bool restart = false;
	
//FIXME GML creates additional threads that need `free` cores too, the problem is to detect if
//      GML is used or not and to reduce the omp threads then (gomp doesn't give you an interface to kill threads once they are started!)
#if !defined(USE_GML)
	if (Threading::GetAvailableCores() >= 3) {
		if (!getenv("OMP_WAIT_POLICY")) {
			// omp threads will use a spinlock instead of yield'ing when waiting
			// cause 100% cpu usage in the omp threads
			setenv("OMP_WAIT_POLICY", "ACTIVE", 1);
			restart = true;
		}
		// another envvar is "GOMP_SPINCOUNT", but it seems to be less predictable
	}
#endif
	return restart;
}


/**
 * Always run on dedicated GPU
 * @return true when restart is required with new env vars
 */
static bool SetNvOptimusProfile(char* argv[])
{
#ifdef WIN32
	if (SOP_CheckProfile("Spring"))
		return false;

	const std::string exename = FileSystem::GetFilename(argv[0]);
	const int res = SOP_SetProfile("Spring", exename);
	return (res == SOP_RESULT_CHANGE);
#else
	return false;
#endif
}



/**
 * @brief main
 * @return exit code
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Main entry point function
 */
int main(int argc, char* argv[])
{
// PROFILE builds exit on execv ...
// HEADLESS run mostly in parallel for testing purposes, 100% omp threads wouldn't help then
#if !defined(PROFILE) && !defined(HEADLESS)
	bool restart = false;
	restart |= SetNvOptimusProfile(argv);
	restart |= SetOpenMpEnvVars(argv);

  #ifndef WIN32
	if (restart) {
		std::vector<std::string> args(argc-1);
		for (int i=1; i<argc; i++) {
			args[i-1] = argv[i];
		}
		const std::string err = Platform::ExecuteProcess(argv[0], args);
		ErrorMessageBox(err, "Execv error:", MBF_OK | MBF_EXCL);
	}
  #endif
#endif
	int ret = Run(argc, argv);
	std::string exe = EngineTypeHandler::GetRestartExecutable();
	if (exe != "") {
		std::vector<std::string> args;
		for (int i=1; i<argc; i++)
			args.push_back(argv[i]);
		if (!EngineTypeHandler::RestartEngine(exe, args)) {
			handleerror(NULL, EngineTypeHandler::GetRestartErrorMessage(), "Missing engine type", MBF_OK | MBF_EXCL);
		}
	}
	return ret;
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
