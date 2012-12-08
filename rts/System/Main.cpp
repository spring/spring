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
	if (Threading::GetAvailableCores() >= 3) {
		if (!getenv("OMP_WAIT_POLICY")) {
			// omp threads will use a spinlock instead of yield'ing when waiting
			// cause 100% cpu usage in the omp threads
			setenv("OMP_WAIT_POLICY", "ACTIVE", 1);
			restart = true;
		}
		// another envvar is "GOMP_SPINCOUNT", but it seems less predictable
	}
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
	if (SetNvOptimusProfile(argv)) {
		LOG_L(L_ERROR, "Optimus profile not set, please set it to get better performance!");
	}
	if (SetOpenMpEnvVars(argv)) {
		LOG_L(L_ERROR, "Please set env var OMP_WAIT_POLICY=ACTIVE to get better performance!");
	}
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
