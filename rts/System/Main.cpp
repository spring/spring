/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
	\mainpage
	This is the documentation of the Spring RTS Engine.
	https://springrts.com/
*/


#include "System/SpringApp.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Misc.h"
#include "System/Log/ILog.h"

#include <clocale>
#include <cstdlib>

#ifdef WIN32
	#include "lib/SOP/SOP.hpp" // NvOptimus
#endif

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
	setlocale(LC_NUMERIC, "C");

	Threading::DetectCores();
	Threading::SetMainThread();

	SpringApp app(argc, argv);
	return (app.Run());
}


/**
 * Always run on dedicated GPU
 * @return true when restart is required with new env vars
 */
#if !defined(PROFILE) && !defined(HEADLESS)
static bool SetNvOptimusProfile(const std::string& processFileName)
{
#ifdef WIN32
	if (SOP_CheckProfile("Spring"))
		return false;

	// on Windows execvp breaks lobbies (new process: new PID)
	return (false && (SOP_SetProfile("Spring", processFileName) == SOP_RESULT_CHANGE));
#endif
	return false;
}
#endif



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
	if (SetNvOptimusProfile(FileSystem::GetFilename(argv[0]))) {
		// prepare for restart
		std::vector<std::string> args(argc - 1);

		for (int i = 1; i < argc; i++)
			args[i - 1] = argv[i];

		// ExecProc normally does not return; if it does the retval is an error-string
		ErrorMessageBox(Platform::ExecuteProcess(argv[0], args), "Execv error:", MBF_OK | MBF_EXCL);
	}
#endif

	return (Run(argc, argv));
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

