/**
 * @file Main.cpp
 * @brief Main class
 *
 * Main application class that launches
 * everything else
 */

#include "Platform/errorhandler.h"
#include "StdAfx.h"
#include "lib/gml/gml.h"
#include "FPUCheck.h"
#include "LogOutput.h"
#include "Exceptions.h"
#include "myMath.h"

#include "SpringApp.h"

#include <SDL.h>     // Must come after Game/UI/MouseHandler.h for ButtonPressEvt

#ifdef WIN32
#include "Platform/Win/win32.h"
#include <winreg.h>
#include <direct.h>
#include "Platform/Win/seh.h"
#endif



// On msvc main() is declared as a non-throwing function.
// Moving the catch clause to a seperate function makes it possible to re-throw the exception for the installed crash reporter
int Run(int argc, char* argv[])
{
#ifdef __MINGW32__
	// For the MinGW backtrace() implementation we need to know the stack end.
	{
		extern void* stack_end;
		char here;
		stack_end = (void*) &here;
	}
#endif

	good_fpu_control_registers("::Run");

#ifdef USE_GML
	set_threadnum(0);
#	if GML_ENABLE_TLS_CHECK
	if (gmlThreadNumber != 0) {
		handleerror(NULL, "Thread Local Storage test failed", "GML error:", MBF_OK | MBF_EXCL);
	}
#	endif
#endif

// It's nice to be able to disable catching when you're debugging
#ifndef NO_CATCH_EXCEPTIONS
	try {
		SpringApp app;
		return app.Run(argc, argv);
	}
	catch (const content_error& e) {
		SDL_Quit();
		logOutput.RemoveAllSubscribers();
		logOutput.Print("Content error: %s\n", e.what());
		handleerror(NULL, e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);
		return -1;
	}
	catch (const std::exception& e) {
		SDL_Quit();
	#ifdef _MSC_VER
		logOutput.Print("Fatal error: %s\n", e.what());
		logOutput.RemoveAllSubscribers();
		throw; // let the error handler catch it
	#else
		logOutput.RemoveAllSubscribers();
		handleerror(NULL, e.what(), "Fatal Error", MBF_OK | MBF_EXCL);
		return -1;
	#endif
	}
	catch (const char* e) {
		SDL_Quit();
	#ifdef _MSC_VER
		logOutput.Print("Fatal error: %s\n", e);
		logOutput.RemoveAllSubscribers();
		throw; // let the error handler catch it
	#else
		logOutput.RemoveAllSubscribers();
		handleerror(NULL, e, "Fatal Error", MBF_OK | MBF_EXCL);
		return -1;
	#endif
	}
#else
	SpringApp app;
	return app.Run(argc, argv);
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
	CMyMath::Init();
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CMyMath::Init();
	return Run(__argc, __argv);
}
#endif
