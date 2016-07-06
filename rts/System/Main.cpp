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

#ifdef WIN32
	#include "lib/SOP/SOP.hpp" // NvOptimus

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

	// run
	try {
		SpringApp app(argc, argv);
		ret = app.Run();
	} CATCH_SPRING_ERRORS

	// check if (a thread in) Spring crashed, if so display an error message
	Threading::Error* err = Threading::GetThreadError();

	if (err != NULL) {
		ErrorMessageBox(" error: " + err->message, err->caption, err->flags, true);
	}

	return ret;
}


/**
 * Always run on dedicated GPU
 * @return true when restart is required with new env vars
 */
static bool SetNvOptimusProfile(const std::string& processFileName)
{
#ifdef WIN32
	if (SOP_CheckProfile("Spring"))
		return false;

	const bool profileChanged = (SOP_SetProfile("Spring", processFileName) == SOP_RESULT_CHANGE);

	// on Windows execvp breaks lobbies (new process: new PID)
	return (false && profileChanged);
#endif
	return false;
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

