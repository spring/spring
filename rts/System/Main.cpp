/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * Main application class that launches
 * everything else
 */

#ifdef WIN32
#define BOOST_MAIN_THREAD 1 // main thread as separate boost::thread
#else
#define BOOST_MAIN_THREAD 0 // linux has some SDL problems with this
#endif

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include <sstream>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>

#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"

#ifndef _MSC_VER
#include "StdAfx.h"
#endif
#include "lib/gml/gml.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"

#include "SpringApp.h"

#ifdef WIN32
#include "Platform/Win/win32.h"
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

#if BOOST_MAIN_THREAD
	boost::thread* mainThread = new boost::thread(boost::bind(&MainFunc, argc, argv, &ret));
	Threading::SetMainThread(mainThread);
	while(mainThread->joinable())
		if(mainThread->timed_join(boost::posix_time::seconds(1)))
			break;
	delete mainThread;
#else
	Threading::SetMainThread();
	MainFunc(argc, argv, &ret);
#endif

	//! check if Spring crashed, if so display an error message
	Threading::Error* err = Threading::GetThreadError();
	if (err)
		ErrorMessageBox(err->message, err->caption, err->flags | MBF_MAIN);

	return ret;
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
