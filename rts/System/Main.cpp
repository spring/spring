/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * Main application class that launches
 * everything else
 */

#include <sstream>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>

#include "System/SpringApp.h"

#include "lib/gml/gml.h"
#include "lib/gml/gmlmut.h"
#include "System/Exceptions.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"

#ifdef WIN32
	#include "System/Platform/Win/win32.h"
#endif

#ifndef __APPLE__
	// SDL_main.h contains a macro that replaces the main function on some OS, see SDL_main.h for details
	#include <SDL_main.h>
#endif


void MainFunc(int argc, char** argv, int* ret) {
#ifdef __MINGW32__
	// For the MinGW backtrace() implementation we need to know the stack end.
	{
		extern void* stack_end;
		char here;
		stack_end = (void*) &here;
	}
#endif

	while (!Threading::IsMainThread())
		;

#ifdef USE_GML
	set_threadnum(GML_DRAW_THREAD_NUM);
  #if GML_ENABLE_TLS_CHECK
	if (gmlThreadNumber != GML_DRAW_THREAD_NUM) {
		ErrorMessageBox("Thread Local Storage test failed", "GML error:", MBF_OK | MBF_EXCL);
	}
  #endif
#endif

	try {
		SpringApp app;
		*ret = app.Run(argc, argv);
	} CATCH_SPRING_ERRORS
}



int Run(int argc, char* argv[])
{
	int ret = -1;

	Threading::SetMainThread();
	MainFunc(argc, argv, &ret);

	//! check if Spring crashed, if so display an error message
	Threading::Error* err = Threading::GetThreadError();
	if (err)
		ErrorMessageBox("Error in main(): " + err->message, err->caption, err->flags);

	return ret;
}

/**
	\mainpage
	This is the documentation of the Spring RTS Engine.

	http://springrts.com/

*/



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
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
