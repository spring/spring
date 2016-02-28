/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Watchdog.h"

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
#include "System/Platform/Misc.h"
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
			, numreg(0)
			, timer(spring_notime)
		{}

		void ResetThreadControls() {
			#ifndef WIN32
			// this is not auto-destructed (!)
			ctls.reset();
			#endif
		}

		void SetThreadControls(const std::shared_ptr<Threading::ThreadControls>& c)
		{
			#ifndef WIN32
			assert(c.get() != nullptr);
			// copy shared_ptr object, not shared_ptr*
			ctls = c;
			#endif
		}

		volatile Threading::NativeThreadHandle thread;
		volatile Threading::NativeThreadId threadid;
		volatile unsigned int numreg;

		spring_time timer;

		// not used on Windows
		std::shared_ptr<Threading::ThreadControls> ctls;
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
			WatchDogThreadInfo* threadInfo = registeredThreads[i];

			if (threadInfo->numreg != 0 && Threading::NativeThreadIdsEqual(num, threadInfo->threadid)) {
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

				WatchDogThreadInfo* threadInfo = registeredThreads[i];
				spring_time curwdt = threadInfo->timer;

				if (spring_istime(curwdt) && (curtime - curwdt) > hangTimeout) {
					if (!hangDetected) {
						LOG_L(L_WARNING, "[Watchdog] Hang detection triggered for Spring %s.", SpringVersion::GetFull().c_str());
					}
					LOG_L(L_WARNING, "  (in thread: %s)", threadNames[i]);

					hangDetected = true;
					threadInfo->timer = curtime;
				}
			}

			if (hangDetected) {
				CrashHandler::PrepareStacktrace(LOG_LEVEL_WARNING);

				for (unsigned int i = 0; i < WDT_COUNT; ++i) {
					if (!threadSlots[i].active)
						continue;

#ifdef WIN32
					CrashHandler::Stacktrace(registeredThreads[i]->thread, threadNames[i], LOG_LEVEL_WARNING);
#else
					CrashHandler::SuspendedStacktrace(registeredThreads[i]->ctls.get(), std::string(threadNames[i]));
#endif
				}

				CrashHandler::CleanupStacktrace(LOG_LEVEL_WARNING);
			}

			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}


	void RegisterThread(WatchdogThreadnum num, bool primary)
	{
		boost::mutex::scoped_lock lock(wdmutex);

		if (num >= WDT_COUNT || registeredThreads[num]->numreg != 0) {
			LOG_L(L_ERROR, "[Watchdog::%s] Invalid thread number %u", __FUNCTION__, num);
			return;
		}

		Threading::NativeThreadHandle thread = Threading::GetCurrentThread();
		Threading::NativeThreadId threadId = Threading::GetCurrentThreadId();

		unsigned int inact = WDT_COUNT;
		unsigned int i;

		for (i = 0; i < WDT_COUNT; ++i) {
			const WatchDogThreadInfo* threadInfo = &registeredThreadsData[i];

			if (threadInfo->numreg == 0)
				inact = std::min(i, inact);
			else if (Threading::NativeThreadIdsEqual(threadInfo->threadid, threadId))
				break;
		}

		if (i >= WDT_COUNT)
			i = inact;
		if (i >= WDT_COUNT) {
			LOG_L(L_ERROR, "[Watchdog::%s] Internal error", __FUNCTION__);
			return;
		}

		registeredThreads[num] = &registeredThreadsData[i];

		// set threadname
		//Threading::SetThreadName(threadNames[num]);

		WatchDogThreadInfo* threadInfo = registeredThreads[num];
		threadInfo->thread = thread;
		threadInfo->threadid = threadId;
		threadInfo->timer = spring_gettime();

		// note: WDT_MAIN and WDT_LOAD share the same controls if LoadingMT=0
		LOG("[WatchDog] registering controls for thread [%s]", threadNames[num]);
		threadInfo->SetThreadControls(Threading::GetCurrentThreadControls());

		++threadInfo->numreg;

		threadSlots[num].primary = primary;
		threadSlots[num].regorder = ++curorder;
		UpdateActiveThreads(threadId);
	}


	void DeregisterThread(WatchdogThreadnum num)
	{
		boost::mutex::scoped_lock lock(wdmutex);

		WatchDogThreadInfo* threadInfo = nullptr;

		if (num >= WDT_COUNT || registeredThreads[num] == nullptr || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s] Invalid thread number %u", __FUNCTION__, num);
			return;
		}

		threadSlots[num].primary = false;
		threadSlots[num].regorder = 0;
		UpdateActiveThreads(threadInfo->threadid);

		// reset the main thread's controls only if actually called from it;
		// otherwise Load would act in place of Main in the LoadingMT=0 case
		if (num == WDT_MAIN || !Threading::IsMainThread()) {
			LOG("[WatchDog] deregistering controls for thread [%s]", threadNames[num]);
			threadInfo->ResetThreadControls();
		}

		if (0 == --(threadInfo->numreg))
			memset(threadInfo, 0, sizeof(WatchDogThreadInfo));

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

		WatchDogThreadInfo* threadInfo;

		if (num >= WDT_COUNT || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s(id)] Invalid thread %d (_threadId=%p)", __FUNCTION__, num, _threadId);
			return;
		}

		threadInfo->timer = disable ? spring_notime : spring_gettime();
	}


	void ClearTimer(WatchdogThreadnum num, bool disable)
	{
		if (hangDetectorThread == NULL)
			return;
		if (Threading::IsWatchDogThread())
			return;

		WatchDogThreadInfo* threadInfo;

		if (num >= WDT_COUNT || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s(num)] Invalid thread %d", __FUNCTION__, num);
			return;
		}

		threadInfo->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearTimer(const std::string& name, bool disable)
	{
		if (hangDetectorThread == NULL)
			return;
		if (Threading::IsWatchDogThread())
			return;

		const auto i = threadNameToNum.find(name);
		unsigned int num;
		WatchDogThreadInfo* threadInfo;

		if (i == threadNameToNum.end() || (num = i->second) >= WDT_COUNT || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s(name)] Invalid thread name \"%s\"", __FUNCTION__, name.c_str());
			return;
		}

		threadInfo->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearPrimaryTimers(bool disable)
	{
		if (hangDetectorThread == NULL)
			return; //! Watchdog isn't running

		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			WatchDogThreadInfo* threadInfo = registeredThreads[i];
			if (threadSlots[i].primary)
				threadInfo->timer = disable ? spring_notime : spring_gettime();
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

		// disable if gdb is running
		if (Platform::IsRunningInGDB()) {
			LOG("[WatchDog::%s] disabled (gdb detected)", __FUNCTION__);
			return;
		}
	#ifdef USE_VALGRIND
		if (RUNNING_ON_VALGRIND) {
			LOG("[WatchDog::%s] disabled (Valgrind detected)", __FUNCTION__);
			return;
		}
	#endif
		const int hangTimeoutSecs = configHandler->GetInt("HangTimeout");

		// HangTimeout = -1 to force disable hang detection
		if (hangTimeoutSecs <= 0) {
			LOG("[WatchDog::%s] disabled", __FUNCTION__);
			return;
		}

		hangTimeout = spring_secs(hangTimeoutSecs);

		// start the watchdog thread
		hangDetectorThread = new boost::thread(&HangDetectorLoop);

		LOG("[WatchDog%s] Installed (HangTimeout: %isec)", __FUNCTION__, hangTimeoutSecs);
	}


	void Uninstall()
	{
		LOG_L(L_INFO, "[WatchDog::%s][1] hangDetectorThread=%p", __FUNCTION__, hangDetectorThread);

		if (hangDetectorThread == NULL)
			return;

		boost::mutex::scoped_lock lock(wdmutex);

		hangDetectorThreadInterrupted = true;

		LOG_L(L_INFO, "[WatchDog::%s][2]", __FUNCTION__);
		hangDetectorThread->join();
		delete hangDetectorThread;
		hangDetectorThread = NULL;
		LOG_L(L_INFO, "[WatchDog::%s][3]", __FUNCTION__);

		memset(registeredThreadsData, 0, sizeof(registeredThreadsData));
		for (unsigned int i = 0; i < WDT_COUNT; ++i)
			registeredThreads[i] = &registeredThreadsData[WDT_COUNT];
		memset(threadSlots, 0, sizeof(threadSlots));
		threadNameToNum.clear();
	}
}
