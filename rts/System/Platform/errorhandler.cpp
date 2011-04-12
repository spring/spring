/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */

#include <StdAfx.h>
#include <sstream>
#include "errorhandler.h"

#include "Game/GameServer.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Util.h"

#ifndef DEDICATED
	#include "SpringApp.h"
	#include "System/Platform/Threading.h"

    #ifndef HEADLESS
	#ifdef WIN32
		#include <windows.h>
	#else
		// from X_MessageBox.cpp:
		void X_MessageBox(const char* msg, const char* caption, unsigned int flags);
	#endif
    #endif // ifndef HEADLESS
#endif // ifndef DEDICATED


static void ExitMessage(const std::string& msg, const std::string& caption, unsigned int flags, bool forced)
{
	logOutput.SetSubscribersEnabled(false);
	if (forced)
		LogObject() << "WARNING: failed to shutdown normally, exit forced";
	LogObject() << caption << " " << msg;

#if !defined(DEDICATED) && !defined(HEADLESS)
  #ifdef WIN32
	//! Windows implementation, using MessageBox.

	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = MB_TOPMOST;		

	// MB_OK is default (0)
	if (flags & MBF_EXCL)
		winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)
		winFlags |= MB_ICONINFORMATION;
	if (flags & MBF_CRASH)
		winFlags |= MB_ICONERROR;

	MessageBox(GetActiveWindow(), msg.c_str(), caption.c_str(), winFlags);

  #else  // ifdef WIN32
	//! X implementation
	// TODO: write Mac OS X specific message box
	X_MessageBox(msg.c_str(), caption.c_str(), flags);

  #endif // ifdef WIN32
#endif // if !defined(DEDICATED) && !defined(HEADLESS)

#ifdef _MSC_VER
	TerminateProcess(GetCurrentProcess(), -1);
#else
	exit(-1); // continuing execution when SDL_Quit has already been run will result in a crash
#endif
}

volatile bool shutdownSucceeded = false;

void ForcedExit(const std::string& msg, const std::string& caption, unsigned int flags) {
	for (unsigned int n = 0; !shutdownSucceeded && n < 10; ++n)
		boost::this_thread::sleep(boost::posix_time::seconds(1));

	if (!shutdownSucceeded)
		ExitMessage(msg, caption, flags, true);
}

void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags)
{
#ifndef DEDICATED
	//! Non-MainThread. Inform main one and then interrupt it.
	if (!Threading::IsMainThread()) {
		gu->globalQuit = true;
		Threading::Error err(caption, msg, flags);
		Threading::SetThreadError(err);

		//! terminate thread
		throw boost::thread_interrupted();
	}

//#ifdef USE_GML
	//! SpringApp::Shutdown is extremely likely to deadlock or end up waiting indefinitely if any 
	//! MT thread has crashed or deviated from its normal execution path by throwing an exception
	boost::thread* forcedExitThread = new boost::thread(boost::bind(&ForcedExit, msg, caption, flags));
//#endif
#endif

#ifdef DEDICATED
	SafeDelete(gameServer);
#else
	//! exiting any possibly threads
	//! (else they would still run while the error messagebox is shown)
	SpringApp::Shutdown();

//#ifdef USE_GML
	shutdownSucceeded = true;
	forcedExitThread->join();
	delete forcedExitThread;
//#endif

#endif

	ExitMessage(msg, caption, flags, false);
}
