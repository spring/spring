/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
	\mainpage
	This is the documentation of the Spring RTS Engine.
	https://springrts.com/
*/

#include "System/ExportDefines.h"
#include "System/SpringApp.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Misc.h"
#include "System/Log/ILog.h"

#include <clocale>
#include <cstdlib>
#include <cstdint>

// https://stackoverflow.com/a/27881472/9819318
EXTERNALIZER_B EXPORT_CLAUSE uint32_t NvOptimusEnablement = 0x00000001;         EXTERNALIZER_E //Optimus/NV use discrete GPU hint
EXTERNALIZER_B EXPORT_CLAUSE uint32_t AmdPowerXpressRequestHighPerformance = 1; EXTERNALIZER_E // AMD use discrete GPU hint

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

	// already the default, but be explicit for locale-dependent functions (atof,strtof,...)
	setlocale(LC_ALL, "C");

	Threading::DetectCores();
	Threading::SetMainThread();

	SpringApp app(argc, argv);
	return (app.Run());
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
	return (Run(argc, argv));
}



#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

