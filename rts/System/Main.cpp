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
	#include <stdlib.h>
	#include <fenv.h>
	#include <process.h>
	#define execv _execv
	#define setenv(k,v,o) _putenv_s(k,v)
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

static void SetOpenMpEnvVars(char* argv[])
{
	// set some performance relevant openmp envvars
	bool restart = false;
	/*if (!getenv("OMP_WAIT_POLICY")) {
		// omp threads will use a spinlock instead of yield'ing when waiting
		// cause 100% cpu usage in the omp threads
		setenv("OMP_WAIT_POLICY", "ACTIVE", 1);
		restart = true;
	}*/
	if (!getenv("GOMP_SPINCOUNT")) {
		// default spinlock time when waiting for new omp tasks is 3ms, increase it (better than active wait policy above)
		#define MILLISEC "00000"
		setenv("GOMP_SPINCOUNT", "20" MILLISEC, 1);
		restart = true;
	}

	// restart with new envvars
	if (restart) {
		execv(argv[0], argv);
	}
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
	SetOpenMpEnvVars(argv);
	return Run(argc, argv);
}



#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif
