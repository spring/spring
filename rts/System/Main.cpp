/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * Main application class that launches
 * everything else
 */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include <sstream>
#include <boost/system/system_error.hpp>

#include "Platform/errorhandler.h"
#ifndef _MSC_VER
#include "StdAfx.h"
#endif
#include "lib/gml/gml.h"
#include "LogOutput.h"
#include "Exceptions.h"

#include "SpringApp.h"

#ifdef WIN32
#include "Platform/Win/win32.h"
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


	try {
		SpringApp app;
		return app.Run(argc, argv);
	}
	catch (const content_error& e) {
		ErrorMessageBox(e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);
	}
#ifndef NO_CATCH_EXCEPTIONS
	catch (const boost::system::system_error& e) {
		std::stringstream ss;
		ss << e.code().value() << ": " << e.what();
		std::string tmp = ss.str();
		ErrorMessageBox(tmp, "Fatal Error", MBF_OK | MBF_EXCL);
	}
	catch (const std::exception& e) {
		ErrorMessageBox(e.what(), "Fatal Error", MBF_OK | MBF_EXCL);
	}
	catch (const char* e) {
		ErrorMessageBox(e, "Fatal Error", MBF_OK | MBF_EXCL);
	}
#endif

	return -1;
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
