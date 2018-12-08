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

	// sic; on Windows execvp spawns a new process which breaks lobby state-tracking by PID
	return (SOP_SetProfile("Spring", processFileName) == SOP_RESULT_CHANGE, false);
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
// PROFILE builds exit on execv, HEADLESS does not use the GPU
#if !defined(PROFILE) && !defined(HEADLESS)
#define MAX_ARGS 32

	if (SetNvOptimusProfile(FileSystem::GetFilename(argv[0]))) {
		// prepare for restart
		std::array<std::string, MAX_ARGS> args;

		for (int i = 0, n = std::min(argc, MAX_ARGS); i < n; i++)
			args[i] = argv[i];

		// ExecProc normally does not return; if it does the retval is an error-string
		ErrorMessageBox(Platform::ExecuteProcess(args), "Execv error:", MBF_OK | MBF_EXCL);
	}
#undef MAX_ARGS
#endif

	return (Run(argc, argv));
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

