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

	//workaround #1: _execv() fails horribly when the binary path contains spaces, in that case the 2nd process will got a splitted arg0
	//workaround #2: _execv() makes the parent process return directly after the new one started, POSIX behaviour is that to wait for the new one and pass the return value
	static void execv(char*, char*)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory( &si, sizeof(STARTUPINFO) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );

		char* cmdLine = GetCommandLine();
		int result = CreateProcess(NULL, cmdLine, NULL, NULL, false, NULL, NULL, NULL, &si, &pi);

		if (result) {
			int ret = WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			exit(ret);
		}

		exit(GetLastError());
	}
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
 */
static void SetOpenMpEnvVars(char* argv[])
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

	// restart with new envvars
	if (restart) {
		execv(argv[0], argv);
	}
}


/**
 * Always run on dedicated GPU.
 */
static void SetNvOptimusProfile(char* argv[])
{
#ifdef WIN32
	if (SOP_CheckProfile("Spring"))
		return;

	const std::string exename = FileSystem::GetFilename(argv[0]);
	const int res = SOP_SetProfile("Spring", exename);
	if (res == SOP_RESULT_CHANGE)
		execv(argv[0], argv);
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
	SetNvOptimusProfile(argv);
	SetOpenMpEnvVars(argv);
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
