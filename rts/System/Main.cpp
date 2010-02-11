/**
 * @file Main.cpp
 * @brief Main class
 *
 * Main application class that launches
 * everything else
 */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include <sstream>
#include <boost/system/system_error.hpp>
#include <boost/asio.hpp>
#include <boost/version.hpp>

#include "Platform/errorhandler.h"
#ifndef _MSC_VER
#include "StdAfx.h"
#endif
#include "lib/gml/gml.h"
#include "LogOutput.h"
#include "Exceptions.h"

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
		logOutput.SetSubscribersEnabled(false);
		logOutput.Print("Content error: %s\n", e.what());
		handleerror(NULL, e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);
		return -1;
	}
	catch (const boost::system::system_error& e) {
		logOutput.Print("Fatal system error: %d: %s", e.code().value(), e.what());
	#ifdef _MSC_VER
		throw;
	#else
		std::stringstream ss;
		ss << e.code().value() << ": " << e.what();
		std::string tmp = ss.str();
		handleerror(NULL, tmp.c_str(), "Fatal Error", MBF_OK | MBF_EXCL);
		return -1;
	#endif
	}
	catch (const std::exception& e) {
		SDL_Quit();
	#ifdef _MSC_VER
		logOutput.Print("Fatal error: %s\n", e.what());
		logOutput.SetSubscribersEnabled(false);
		throw; // let the error handler catch it
	#else
		logOutput.SetSubscribersEnabled(false);
		handleerror(NULL, e.what(), "Fatal Error", MBF_OK | MBF_EXCL);
		return -1;
	#endif
	}
	catch (const char* e) {
		SDL_Quit();
	#ifdef _MSC_VER
		logOutput.Print("Fatal error: %s\n", e);
		logOutput.SetSubscribersEnabled(false);
		throw; // let the error handler catch it
	#else
		logOutput.SetSubscribersEnabled(false);
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
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return Run(__argc, __argv);
}
#endif
