/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Watchdog.h"

#include "lib/gml/gml.h"
#ifndef _WIN32
	#include <fstream>
	#include <iostream>
#endif
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "Game/GameVersion.h"
#include "System/ConfigHandler.h"
#include "System/LogOutput.h"
#include "System/maindefines.h"
#include "System/myTime.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"


namespace Watchdog
{
	struct WatchDogThreadInfo {
		WatchDogThreadInfo() : timer(spring_notime) {}
		Threading::NativeThreadHandle thread;
		volatile spring_time timer;
		std::vector<std::string> names;
		bool primary;
	};
	typedef std::map<Threading::NativeThreadId, WatchDogThreadInfo> ThreadsMap;
	typedef std::map<std::string, Threading::NativeThreadId> ThreadNameToIdMap;
	static ThreadsMap registeredThreads;
	static ThreadNameToIdMap threadNameToId;

	static boost::recursive_mutex mutex;
	static boost::thread* hangdetectorthread = NULL;
	static spring_time hangTimeout = spring_msecs(0);

	static void HangDetector()
	{
		while (!boost::this_thread::interruption_requested()) {
			{
				boost::recursive_mutex::scoped_lock lock(mutex);
				const spring_time curtime = spring_gettime();
				bool hangDetected = false;
				for (ThreadsMap::iterator it=registeredThreads.begin(); it != registeredThreads.end(); ++it) {
					WatchDogThreadInfo& th_info = it->second;
					if (spring_istime(th_info.timer) && (curtime - th_info.timer) > hangTimeout) {
						if(!hangDetected) {
							logOutput.Print("[Watchdog] Hang detection triggered for Spring %s.", SpringVersion::GetFull().c_str());
						#ifdef USE_GML
							logOutput.Print("MT with %d threads.", gmlThreadCount);
						#endif
						}
						logOutput.Print("  (in thread: %s)", th_info.names.front().c_str());
						
						hangDetected = true;
						th_info.timer = curtime;
					}
				}
				if(hangDetected) {
					CrashHandler::PrepareStacktrace();
					for (ThreadsMap::iterator it=registeredThreads.begin(); it != registeredThreads.end(); ++it) {
						WatchDogThreadInfo& th_info = it->second;
						CrashHandler::Stacktrace(th_info.thread, th_info.names.front());
						logOutput.Flush();
					}
					CrashHandler::CleanupStacktrace();
				}
			}
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}


	void RegisterThread(std::string name, bool primary)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		Threading::NativeThreadHandle thread = Threading::GetCurrentThread();
		Threading::NativeThreadId threadId = Threading::GetCurrentThreadId();
		WatchDogThreadInfo& th_info = registeredThreads[threadId];
		th_info.thread = thread;
		th_info.timer = spring_gettime();
		th_info.names.push_back(name);
		th_info.primary = primary;
		threadNameToId[name] = threadId;
	}


	void DeregisterThread(std::string name)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		ThreadNameToIdMap::iterator it = threadNameToId.find(name);
		if (it == threadNameToId.end()) {
			logOutput.Print("[Watchdog::DeregisterThread] No thread found named \"%s\".", name.c_str());
			return;
		}
		WatchDogThreadInfo& th_info = registeredThreads[it->second];
		std::vector<std::string>::iterator it2 = find(th_info.names.begin(), th_info.names.end(), name);
		assert(it2 != th_info.names.end());
		th_info.names.erase(it2);
		if (th_info.names.empty()) {
			registeredThreads.erase(it->second);
		}
		threadNameToId.erase(name);
		//logOutput.Print("[Watchdog::DeregisterThread] thread removed \"%s\".", name.c_str());
	}

	inline void DoClearTimer(bool disable, Threading::NativeThreadId* _threadId)
	{
		if (!hangdetectorthread)
			return; //! Watchdog isn't running
		Threading::NativeThreadId threadId;
		if (_threadId) {
			threadId = *_threadId;
		} else {
			threadId = Threading::GetCurrentThreadId();
		}
		ThreadsMap::iterator it = registeredThreads.find(threadId);
		if (it == registeredThreads.end()) {
			return;
		}
		assert(it != registeredThreads.end()); //! the thread isn't registered!

		WatchDogThreadInfo& th_info = it->second;
		th_info.timer = disable ? spring_notime : spring_gettime();
	}

	void ClearTimer(bool disable, Threading::NativeThreadId* _threadId)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		DoClearTimer(disable, _threadId);
	}


	void ClearTimer(const std::string& name, bool disable)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		if (!hangdetectorthread)
			return; //! Watchdog isn't running
		ThreadNameToIdMap::iterator it = threadNameToId.find(name);
		if (it == threadNameToId.end()) {
			logOutput.Print("[Watchdog::ClearTimer] No thread found named \"%s\".", name.c_str());
			return;
		}
		DoClearTimer(disable, &(it->second));
	}


	void ClearPrimaryTimers(bool disable)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		if (!hangdetectorthread)
			return; //! Watchdog isn't running

		for (ThreadsMap::iterator it=registeredThreads.begin(); it != registeredThreads.end(); ++it) {
			WatchDogThreadInfo& th_info = it->second;
			if (th_info.primary)
				th_info.timer = disable ? spring_notime : spring_gettime();
		}
	}


	void Install()
	{
	#ifndef _WIN32
		//! disable if gdb is running
		char buf[1024];
		SNPRINTF(buf, sizeof(buf), "/proc/%d/cmdline", getppid());
		std::ifstream f(buf);
		if (f.good()) {
			f.read(buf,sizeof(buf));
			f.close();
			std::string str(buf);
			if (str.find("gdb") != std::string::npos) {
				logOutput.Print("[Watchdog] disabled (gdb detected)");
				return;
			}
		}
	#endif
		int hangTimeoutSecs = configHandler->Get("HangTimeout", 0);

		//! HangTimeout = -1 to force disable hang detection
		if (hangTimeoutSecs < 0) {
			logOutput.Print("[Watchdog] disabled");
			return;
		}
		if (hangTimeoutSecs == 0)
			hangTimeoutSecs = 10;

		hangTimeout = spring_secs(hangTimeoutSecs);

		//! start the watchdog thread
		hangdetectorthread = new boost::thread(&HangDetector);

		//! register (this) mainthread
		RegisterThread("main", true);

		logOutput.Print("[Watchdog] Installed (timeout: %isec)", hangTimeoutSecs);
	}


	void Uninstall()
	{
		if (hangdetectorthread) {
			hangdetectorthread->interrupt();
			hangdetectorthread->join();
			delete hangdetectorthread;
			hangdetectorthread = NULL;

			boost::recursive_mutex::scoped_lock lock(mutex);
			registeredThreads.clear();
			threadNameToId.clear();
		}
	}
}
