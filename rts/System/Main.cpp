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
		SpringApp app(argc, argv);
		ret = app.Run();
	} CATCH_SPRING_ERRORS

	// check if Spring crashed, if so display an error message
	Threading::Error* err = Threading::GetThreadError();
	if (err)
		ErrorMessageBox("Error in main(): " + err->message, err->caption, err->flags);

	return ret;
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
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
