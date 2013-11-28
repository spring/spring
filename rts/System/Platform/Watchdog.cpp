/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Watchdog.h"

#include "lib/gml/gml_base.h"
#ifndef _WIN32
	#include <fstream>
	#include <iostream>
#endif

#ifdef USE_VALGRIND
	#include <valgrind/valgrind.h>
#endif

#include <algorithm>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "Game/GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/maindefines.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"

CONFIG(int, HangTimeout).defaultValue(10).minimumValue(-1).maximumValue(600)
		.description("Number of seconds that, if spent in the same code segment, indicate a hang; -1 to disable.");

namespace Watchdog
{
	const char* threadNames[] = {"main", "sim", "load", "audio", "self"};

	static boost::mutex wdmutex;

	static unsigned int curorder = 0;

	struct WatchDogThreadInfo {
		WatchDogThreadInfo()
			: threadid(0)
			, timer(spring_notime)
			, numreg(0)
		{}
		volatile Threading::NativeThreadHandle thread;
		volatile Threading::NativeThreadId threadid;
		spring_time timer;
		volatile unsigned int numreg;
	};

	struct WatchDogThreadSlot {
		WatchDogThreadSlot()
			: primary(false)
			, active(false)
			, regorder(0)
		{}
		volatile bool primary;
		volatile bool active;
		volatile unsigned int regorder;
	};

	// NOTE:
	//   these arrrays include one extra element so threads
	//   point somewhere non-NULL after being deregistered
	static WatchDogThreadInfo registeredThreadsData[WDT_COUNT + 1];
	static WatchDogThreadInfo* registeredThreads[WDT_COUNT + 1] = {NULL};
	static WatchDogThreadSlot threadSlots[WDT_COUNT + 1];

	static std::map<std::string, unsigned int> threadNameToNum;

	static boost::thread* hangDetectorThread = NULL;
	static spring_time hangTimeout = spring_msecs(0);
	static volatile bool hangDetectorThreadInterrupted = false;

	static inline void UpdateActiveThreads(Threading::NativeThreadId num) {
		unsigned int active = WDT_COUNT;

		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			WatchDogThreadInfo* th_info = registeredThreads[i];
			if (th_info->numreg && Threading::NativeThreadIdsEqual(num, th_info->threadid)) {
				threadSlots[i].active = false;
				if (threadSlots[i].regorder > threadSlots[active].regorder)
					active = i;
			}
		}

		if (active < WDT_COUNT) {
			threadSlots[active].active = true;
		}
	}


	__FORCE_ALIGN_STACK__
	static void HangDetectorLoop()
	{
		Threading::SetThreadName("watchdog");
		Threading::SetWatchDogThread();

		while (!hangDetectorThreadInterrupted) {
			spring_time curtime = spring_gettime();
			bool hangDetected = false;

			for (unsigned int i = 0; i < WDT_COUNT; ++i) {
				if (!threadSlots[i].active)
					continue;

				WatchDogThreadInfo* th_info = registeredThreads[i];
				spring_time curwdt = th_info->timer;

				if (spring_istime(curwdt) && (curtime - curwdt) > hangTimeout) {
					if (!hangDetected) {
						LOG_L(L_WARNING, "[Watchdog] Hang detection triggered for Spring %s.", SpringVersion::GetFull().c_str());
						if (GML::Enabled())
							LOG_L(L_WARNING, "MT with %d threads.", GML::ThreadCount());
					}
					LOG_L(L_WARNING, "  (in thread: %s)", threadNames[i]);

					hangDetected = true;
					th_info->timer = curtime;
				}
			}

			if (hangDetected) {
				CrashHandler::PrepareStacktrace(LOG_LEVEL_WARNING);

				for (unsigned int i = 0; i < WDT_COUNT; ++i) {
					if (!threadSlots[i].active)
						continue;

					CrashHandler::Stacktrace(registeredThreads[i]->thread, threadNames[i], LOG_LEVEL_WARNING);
				}

				CrashHandler::CleanupStacktrace(LOG_LEVEL_WARNING);
			}

			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}


	void RegisterThread(WatchdogThreadnum num, bool primary)
	{
		boost::mutex::scoped_lock lock(wdmutex);

		if (num >= WDT_COUNT || registeredThreads[num]->numreg) {
			LOG_L(L_ERROR, "[Watchdog::RegisterThread] Invalid thread number");
			return;
		}

		Threading::NativeThreadHandle thread = Threading::GetCurrentThread();
		Threading::NativeThreadId threadId = Threading::GetCurrentThreadId();

		unsigned int inact = WDT_COUNT;
		unsigned int i;

		for (i = 0; i < WDT_COUNT; ++i) {
			WatchDogThreadInfo* th_info = &registeredThreadsData[i];
			if (!th_info->numreg)
				inact = std::min(i, inact);
			else if (Threading::NativeThreadIdsEqual(th_info->threadid, threadId))
				break;
		}

		if (i >= WDT_COUNT)
			i = inact;
		if (i >= WDT_COUNT) {
			LOG_L(L_ERROR, "[Watchdog::RegisterThread] Internal error");
			return;
		}

		registeredThreads[num] = &registeredThreadsData[i];

		// set threadname
		//Threading::SetThreadName(threadNames[num]);

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

		if (num >= WDT_COUNT || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::DeregisterThread] Invalid thread number");
			return;
		}

		threadSlots[num].primary = false;
		threadSlots[num].regorder = 0;
		UpdateActiveThreads(th_info->threadid);

		if (!--(th_info->numreg))
			memset(th_info, 0, sizeof(WatchDogThreadInfo));

		registeredThreads[num] = &registeredThreadsData[WDT_COUNT];
	}


	void ClearTimer(bool disable, Threading::NativeThreadId* _threadId)
	{
		// bail if Watchdog isn't running
		if (hangDetectorThread == NULL)
			return;
		// calling thread can be the watchdog thread
		// itself (see HangDetectorLoop), which does
		// not count here and is unregistered anyway
		if (Threading::IsWatchDogThread())
			return;

		Threading::NativeThreadId threadId;

		if (_threadId) {
			threadId = *_threadId;
		} else {
			threadId = Threading::GetCurrentThreadId();
		}

		unsigned int num = 0;

		for (; num < WDT_COUNT; ++num) {
			if (Threading::NativeThreadIdsEqual(registeredThreads[num]->threadid, threadId))
				break;
		}

		WatchDogThreadInfo* th_info;

		if (num >= WDT_COUNT || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::%s(id)] Invalid thread %d (_threadId=%p)", __FUNCTION__, num, _threadId);
			return;
		}

		th_info->timer = disable ? spring_notime : spring_gettime();
	}


	void ClearTimer(WatchdogThreadnum num, bool disable)
	{
		if (hangDetectorThread == NULL)
			return;
		if (Threading::IsWatchDogThread())
			return;

		WatchDogThreadInfo* th_info;

		if (num >= WDT_COUNT || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::%s(num)] Invalid thread %d", __FUNCTION__, num);
			return;
		}

		th_info->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearTimer(const std::string& name, bool disable)
	{
		if (hangDetectorThread == NULL)
			return;
		if (Threading::IsWatchDogThread())
			return;

		const auto i = threadNameToNum.find(name);
		unsigned int num;
		WatchDogThreadInfo* th_info;

		if (i == threadNameToNum.end() || (num = i->second) >= WDT_COUNT || !(th_info = registeredThreads[num])->numreg) {
			LOG_L(L_ERROR, "[Watchdog::%s(name)] Invalid thread name \"%s\"", __FUNCTION__, name.c_str());
			return;
		}

		th_info->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearPrimaryTimers(bool disable)
	{
		if (hangDetectorThread == NULL)
			return; //! Watchdog isn't running

		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			WatchDogThreadInfo* th_info = registeredThreads[i];
			if (threadSlots[i].primary)
				th_info->timer = disable ? spring_notime : spring_gettime();
		}
	}


	void Install()
	{
		boost::mutex::scoped_lock lock(wdmutex);

		memset(registeredThreadsData, 0, sizeof(registeredThreadsData));
		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			registeredThreads[i] = &registeredThreadsData[WDT_COUNT];
			threadNameToNum[std::string(threadNames[i])] = i;
		}
		memset(threadSlots, 0, sizeof(threadSlots));

	#ifndef _WIN32
		// disable if gdb is running
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
	#ifdef USE_VALGRIND
		if (RUNNING_ON_VALGRIND) {
			LOG("[Watchdog] disabled (Valgrind detected)");
			return;
		}
	#endif
		const int hangTimeoutSecs = configHandler->GetInt("HangTimeout");

		// HangTimeout = -1 to force disable hang detection
		if (hangTimeoutSecs <= 0) {
			LOG("[Watchdog] disabled");
			return;
		}

		hangTimeout = spring_secs(hangTimeoutSecs);

		// start the watchdog thread
		hangDetectorThread = new boost::thread(&HangDetectorLoop);

		LOG("[Watchdog] Installed (HangTimeout: %isec)", hangTimeoutSecs);
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
		for (unsigned int i = 0; i < WDT_COUNT; ++i)
			registeredThreads[i] = &registeredThreadsData[WDT_COUNT];
		memset(threadSlots, 0, sizeof(threadSlots));
		threadNameToNum.clear();
	}
}
