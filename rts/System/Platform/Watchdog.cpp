/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
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
#include "System/Log/ILog.h"
#include "System/LogOutput.h"
#include "System/maindefines.h"
#include "System/myTime.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"


namespace Watchdog
{
	const char *threadNames[] = {"main", "sim", "load", "audio"};

	static boost::mutex wdmutex;

	static unsigned int curorder = 0;

	struct WatchDogThreadInfo {
		WatchDogThreadInfo() : timer(spring_notime) {}
		volatile Threading::NativeThreadHandle thread;
		volatile Threading::NativeThreadId threadid;
		volatile spring_time timer;
		volatile unsigned int numreg;
	};
	static WatchDogThreadInfo registeredThreadsData[WDT_SIZE];
	struct WatchDogThreadSlot {
		volatile bool primary;
		volatile bool active;
		volatile unsigned int regorder;
	};
	static WatchDogThreadInfo *registeredThreads[WDT_SIZE] = {NULL};
	static WatchDogThreadSlot threadSlots[WDT_SIZE];

	static std::map<std::string, unsigned int> threadNameToNum;

	static boost::thread* hangDetectorThread = NULL;
	static spring_time hangTimeout = spring_msecs(0);
	static volatile bool hangDetectorThreadInterrupted = false;

	static inline void UpdateActiveThreads(Threading::NativeThreadId num) {
		unsigned int active = WDT_LAST;
		for (unsigned int i = 0; i < WDT_LAST; ++i) {
			WatchDogThreadInfo* th_info = registeredThreads[i];
			if (th_info->numreg && Threading::NativeThreadIdsEqual(num, th_info->threadid)) {
				threadSlots[i].active = false;
				if (threadSlots[i].regorder > threadSlots[active].regorder)
					active = i;
			}
		}
		if (active < WDT_LAST)
			threadSlots[active].active = true;
	}


	static void HangDetectorLoop()
	{
		while (!hangDetectorThreadInterrupted) {
			spring_time curtime = spring_gettime();
			bool hangDetected = false;

			for (unsigned int i = 0; i < WDT_LAST; ++i) {
				if (!threadSlots[i].active)
					continue;

				WatchDogThreadInfo* th_info = registeredThreads[i];
				spring_time curwdt = th_info->timer;

				if (spring_istime(curwdt) && curtime > curwdt && (curtime - curwdt) > hangTimeout) {
					if (!hangDetected) {
						LOG_L(L_WARNING, "[Watchdog] Hang detection triggered for Spring %s.", SpringVersion::GetFull().c_str());
#ifdef USE_GML
						LOG_L(L_WARNING, "MT with %d threads.", gmlThreadCount);
#endif
					}
					LOG_L(L_WARNING, "  (in thread: %s)", threadNames[i]);

					hangDetected = true;
					th_info->timer = curtime;
				}
			}

			if (hangDetected) {
				CrashHandler::PrepareStacktrace();

				for (unsigned int i = 0; i < WDT_LAST; ++i) {
					if (!threadSlots[i].active)
						continue;

					WatchDogThreadInfo* th_info = registeredThreads[i];
					CrashHandler::Stacktrace(th_info->thread, threadNames[i]);
					logOutput.Flush();
				}

				CrashHandler::CleanupStacktrace();
			}

			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}


	void RegisterThread(WatchdogThreadnum num, bool primary)
	{
		boost::mutex::scoped_lock lock(wdmutex);

		if (num >= WDT_LAST || registeredThreads[num]->numreg) {
			LOG_L(L_ERROR, "[Watchdog::RegisterThread] Invalid thread number");
			return;
		}

		Threading::NativeThreadHandle thread = Threading::GetCurrentThread();
		Threading::NativeThreadId threadId = Threading::GetCurrentThreadId();

		unsigned int inact = WDT_LAST;
		unsigned int i;
		for (i = 0; i < WDT_LAST; ++i) {
			WatchDogThreadInfo* th_info = &registeredThreadsData[i];
			if (!th_info->numreg)
				inact = std::min(i, inact);
			else if(Threading::NativeThreadIdsEqual(th_info->threadid, threadId))
				break;
		}
		if (i >= WDT_LAST)
			i = inact;
		if (i >= WDT_LAST) {
			LOG_L(L_ERROR, "[Watchdog::RegisterThread] Internal error");
			return;
		}
		registeredThreads[num] = &registeredThreadsData[i];

		WatchDogThreadInfo* th_info = registeredThreads[num];
		th_info->thread = thread;
		th_info->threadid = threadId;
		th_info->timer = spring_gettime();
		++th_info->numreg;

		threadSlots[num].primary = primary;
		threadSlots[num].regorder = ++curorder;
		UpdateActiveThreads(threadId);
	}


	void DeregisterThread(WatchdogThreadnum num)
	{
		boost::mutex::scoped_lock lock(wdmutex);

		WatchDogThreadInfo* th_info;
		if (num >= WDT_LAST || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::DeregisterThread] Invalid thread number");
			return;
		}
		threadSlots[num].primary = false;
		threadSlots[num].regorder = 0;
		UpdateActiveThreads(th_info->threadid);

		if(!--(th_info->numreg))
			memset(th_info, 0, sizeof(WatchDogThreadInfo));

		registeredThreads[num] = &registeredThreadsData[WDT_LAST];
	}


	void ClearTimer(bool disable, Threading::NativeThreadId* _threadId)
	{
		if (hangDetectorThread == NULL)
			return; //! Watchdog isn't running

		Threading::NativeThreadId threadId;
		if (_threadId)
			threadId = *_threadId;
		else
			threadId = Threading::GetCurrentThreadId();
		unsigned int num = 0;
		for (; num < WDT_LAST; ++num)
			if (Threading::NativeThreadIdsEqual(registeredThreads[num]->threadid, threadId))
				break;
		WatchDogThreadInfo* th_info;
		if (num >= WDT_LAST || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::ClearTimer] Invalid thread %d", num);
			return;
		}

		th_info->timer = disable ? spring_notime : spring_gettime();
	}


	void ClearTimer(WatchdogThreadnum num, bool disable)
	{
		if (hangDetectorThread == NULL)
			return; //! Watchdog isn't running

		WatchDogThreadInfo* th_info;
		if (num >= WDT_LAST || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::ClearTimer] Invalid thread number %d", num);
			return;
		}

		th_info->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearTimer(const std::string &name, bool disable)
	{
		if (hangDetectorThread == NULL)
			return; //! Watchdog isn't running

		std::map<std::string, unsigned int>::iterator i = threadNameToNum.find(name);
		unsigned int num;
		WatchDogThreadInfo* th_info;
		if (i == threadNameToNum.end() || (num = i->second) >= WDT_LAST || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::ClearTimer] Invalid thread name");
			return;
		}

		th_info->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearPrimaryTimers(bool disable)
	{
		if (hangDetectorThread == NULL)
			return; //! Watchdog isn't running

		for (unsigned int i = 0; i < WDT_LAST; ++i) {
			WatchDogThreadInfo* th_info = registeredThreads[i];
			if (threadSlots[i].primary)
				th_info->timer = disable ? spring_notime : spring_gettime();
		}
	}


	void Install()
	{
		boost::mutex::scoped_lock lock(wdmutex);

		memset(registeredThreadsData, 0, sizeof(registeredThreadsData));
		for (unsigned int i = 0; i < WDT_LAST; ++i) {
			registeredThreads[i] = &registeredThreadsData[WDT_LAST];
			threadNameToNum[std::string(threadNames[i])] = i;
		}
		memset(threadSlots, 0, sizeof(threadSlots));

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
				LOG("[Watchdog] disabled (gdb detected)");
				return;
			}
		}
	#endif
		int hangTimeoutSecs = configHandler->Get("HangTimeout", 0);

		//! HangTimeout = -1 to force disable hang detection
		if (hangTimeoutSecs < 0) {
			LOG("[Watchdog] disabled");
			return;
		}
		if (hangTimeoutSecs == 0)
			hangTimeoutSecs = 10;

		hangTimeout = spring_secs(hangTimeoutSecs);

		//! start the watchdog thread
		hangDetectorThread = new boost::thread(&HangDetectorLoop);

		LOG("[Watchdog] Installed (timeout: %isec)", hangTimeoutSecs);
	}


	void Uninstall()
	{
		if (hangDetectorThread == NULL) {
			return;
		}

		boost::mutex::scoped_lock lock(wdmutex);

		hangDetectorThreadInterrupted = true;
		hangDetectorThread->join();
		delete hangDetectorThread;
		hangDetectorThread = NULL;

		memset(registeredThreadsData, 0, sizeof(registeredThreadsData));
		for (unsigned int i = 0; i < WDT_LAST; ++i)
			registeredThreads[i] = &registeredThreadsData[WDT_LAST];
		memset(threadSlots, 0, sizeof(threadSlots));
		threadNameToNum.clear();
	}
}
