/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Watchdog.h"

#include "lib/gml/gml.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "Game/GameVersion.h"
#include "System/ConfigHandler.h"
#include "System/LogOutput.h"
#include "System/myTime.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"


namespace Watchdog
{
	struct WatchDogThreadInfo {
		Threading::NativeThreadHandle thread;
		volatile spring_time timer;
		std::string name;
	};
	typedef std::map<Threading::NativeThreadId, WatchDogThreadInfo> ThreadsMap;
	typedef std::map<std::string, Threading::NativeThreadId> ThreadNameToIdMap;
	static ThreadsMap registeredThreads;
	static ThreadNameToIdMap threadNameToId;

	static boost::shared_mutex mutex;
	static boost::thread* hangdetectorthread = NULL;
	static spring_time hangTimeout = spring_msecs(0);


	/** Print stack traces for relevant threads. */
	static void HangHandler(const WatchDogThreadInfo& threadinfo)
	{
		logOutput.Print("Hang detection triggered (in thread: %s) for Spring %s.", threadinfo.name.c_str(), SpringVersion::GetFull().c_str());
	#ifdef USE_GML
		logOutput.Print("MT with %d threads.", gmlThreadCount);
	#endif

		CrashHandler::Stacktrace(threadinfo.thread);
		logOutput.Flush();
	}


	static void HangDetector()
	{
		while (!boost::this_thread::interruption_requested()) {
			{
				boost::shared_lock<boost::shared_mutex> lock(mutex);
				const spring_time curtime = spring_gettime();
				for (ThreadsMap::iterator it=registeredThreads.begin(); it != registeredThreads.end(); ++it) {
					WatchDogThreadInfo& th_info = it->second;
					if (spring_istime(th_info.timer) && (curtime - th_info.timer) > hangTimeout) {
						HangHandler(th_info);
						th_info.timer = curtime;
					}
				}
			}
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}


	void RegisterThread(std::string name)
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);
		Threading::NativeThreadHandle thread = Threading::_GetCurrentThread();
		Threading::NativeThreadId threadId = Threading::_GetCurrentThreadId();
		WatchDogThreadInfo& th_info = registeredThreads[threadId];
		th_info.thread = thread;
		th_info.timer = spring_gettime();
		th_info.name = name;
		threadNameToId[name] = threadId;
	}


	/*void DeregisterThread()
	{
		boost::unique_lock<boost::shared_mutex> lock(mutex);
		//FIXME ADD CODE
	}*/


	void ClearTimer(bool disable, Threading::NativeThreadId* _threadId)
	{
		boost::shared_lock<boost::shared_mutex> lock(mutex);
		if (!hangdetectorthread)
			return; //! Watchdog isn't running
		Threading::NativeThreadId threadId;
			if (_threadId) {
			threadId = *_threadId;
		} else {
			threadId = Threading::_GetCurrentThreadId();
		}
		ThreadsMap::iterator it = registeredThreads.find(threadId);
		if (it == registeredThreads.end()) {
			return;
		}
		assert(it != registeredThreads.end()); //! the thread isn't registered!

		WatchDogThreadInfo& th_info = it->second;
		th_info.timer = disable ? spring_notime : spring_gettime();
	}


	void ClearTimer(const std::string& name, bool disable)
	{
		boost::shared_lock<boost::shared_mutex> lock(mutex);
		if (!hangdetectorthread)
			return; //! Watchdog isn't running
		ThreadNameToIdMap::iterator it = threadNameToId.find(name);
		if (it == threadNameToId.end()) {
			logOutput.Print("[Watchdog::ClearTimer] No thread found named \"%s\".", name.c_str());
			return;
		}
		ClearTimer(disable, &(it->second));
	}


	void Install()
	{
		int hangTimeoutSecs = configHandler->Get("HangTimeout", 0);

		//! HangTimeout = -1 to force disable hang detection
		if (hangTimeoutSecs < 0)
			return;
		if (hangTimeoutSecs == 0)
			hangTimeoutSecs = 10;

		hangTimeout = spring_secs(hangTimeoutSecs);

		//! register (this) mainthread
		RegisterThread("main");

		//! start the watchdog thread
		hangdetectorthread = new boost::thread(&HangDetector);

		logOutput.Print("[Watchdog] Installed (timeout: %isec)", hangTimeoutSecs);
	}


	void Uninstall()
	{
		if (hangdetectorthread) {
			hangdetectorthread->interrupt();
			hangdetectorthread->join();
			delete hangdetectorthread;
			hangdetectorthread = NULL;

			boost::unique_lock<boost::shared_mutex> lock(mutex);
			registeredThreads.clear();
			threadNameToId.clear();
		}
	}
}
