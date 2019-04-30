/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Watchdog.h"

#ifdef USE_VALGRIND
	#include <valgrind/valgrind.h>
#endif

#include <algorithm>
#include <functional>

#include "Game/GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/MainDefines.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/Misc.h"
#include "System/Platform/Threading.h"
#include "System/StringHash.h"
#include "System/Threading/SpringThreading.h"
#include "System/UnorderedMap.hpp"

CONFIG(int, HangTimeout).defaultValue(10).minimumValue(-1).maximumValue(600)
		.description("Number of seconds that, if spent in the same code segment, indicate a hang; -1 to disable.");

namespace Watchdog
{
	static const char* threadNames[] = {"main", "load", "audio", "vfsi"};

	static spring::mutex wdmutex;

	static unsigned int curorder = 0;

	struct WatchDogThreadInfo {
		WatchDogThreadInfo() { ResetThreadInfo(); }

		void ResetThreadInfo() {
			timer = spring_notime;

			thread = {};
			threadid = {0};
			numreg = {0};
		}
		void ResetThreadControls() {
			#ifndef WIN32
			ctls.reset();
			#endif
		}

		void SetThreadControls()
		{
			#ifndef WIN32
			ctls = Threading::GetCurrentThreadControls();
			#endif
		}

		spring_time timer;

		std::atomic<Threading::NativeThreadHandle> thread;
		std::atomic<Threading::NativeThreadId> threadid;
		std::atomic<unsigned int> numreg;

		// not used on Windows
		std::shared_ptr<Threading::ThreadControls> ctls;
	};

	struct WatchDogThreadSlot {
		std::atomic<bool> primary = {false};
		std::atomic<bool> active = {false};
		std::atomic<unsigned int> regorder = {0};
	};

	// NOTE:
	//   these arrrays include one extra element so threads
	//   point somewhere non-NULL after being deregistered
	static WatchDogThreadInfo registeredThreadsData[WDT_COUNT + 1];
	static WatchDogThreadInfo* registeredThreads[WDT_COUNT + 1] = {nullptr};
	static WatchDogThreadSlot threadSlots[WDT_COUNT + 1];

	// maps hash(name) to WTD_*
	static spring::unsynced_map<unsigned int, unsigned int> threadNumTable;

	static spring::thread hangDetectorThread;
	static std::atomic<bool> hangDetectorThreadInterrupted = {false};

	static spring_time hangTimeout = spring_msecs(0);


	static inline void UpdateActiveThreads(Threading::NativeThreadId num) {
		unsigned int active = WDT_COUNT;

		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			WatchDogThreadInfo* threadInfo = registeredThreads[i];

			if (threadInfo->numreg == 0)
				continue;
			if (!Threading::NativeThreadIdsEqual(num, threadInfo->threadid))
				continue;

			threadSlots[i].active = false;

			// find the active thread with maximum regorder
			if (threadSlots[i].regorder > threadSlots[active].regorder)
				active = i;
		}

		if (active == WDT_COUNT)
			return;

		threadSlots[active].active = true;
	}


	__FORCE_ALIGN_STACK__
	static void HangDetectorLoop()
	{
		Threading::SetThreadName("watchdog");
		Threading::SetWatchDogThread();

		while (!hangDetectorThreadInterrupted) {
			const spring_time curtime = spring_gettime();

			bool hangDetected = false;
			bool hangThreads[WDT_COUNT] = {false};

			for (unsigned int i = 0; i < WDT_COUNT; ++i) {
				hangThreads[i] = false;

				if (!threadSlots[i].active)
					continue;

				WatchDogThreadInfo* threadInfo = registeredThreads[i];
				const spring_time curwdt = threadInfo->timer;

				if (spring_istime(curwdt) && (curtime - curwdt) > hangTimeout) {
					hangDetected = true;
					hangThreads[i] = true;
					threadInfo->timer = curtime;
				}
			}

			if (hangDetected) {
				LOG_L(L_WARNING, "[Watchdog] Hang detection triggered for Spring %s.", SpringVersion::GetFull().c_str());
				LOG_L(L_WARNING, "\t(in threads: {%s,%s,%s,%s}={%d,%d,%d,%d})",
					threadNames[WDT_MAIN], threadNames[WDT_LOAD], threadNames[WDT_AUDIO], threadNames[WDT_VFSI],
					hangThreads[WDT_MAIN], hangThreads[WDT_LOAD], hangThreads[WDT_AUDIO], hangThreads[WDT_VFSI]
				);

				CrashHandler::PrepareStacktrace(LOG_LEVEL_WARNING);

				// generate traces for all active (including non-hanged) threads
				for (unsigned int i = 0; i < WDT_COUNT; ++i) {
					if (!threadSlots[i].active)
						continue;

					#ifdef WIN32
					CrashHandler::Stacktrace(registeredThreads[i]->thread, threadNames[i], LOG_LEVEL_WARNING);
					#else
					CrashHandler::SuspendedStacktrace(registeredThreads[i]->ctls.get(), threadNames[i]);
					#endif
				}

				CrashHandler::CleanupStacktrace(LOG_LEVEL_WARNING);
			}

			spring::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}


	void RegisterThread(WatchdogThreadnum num, bool primary)
	{
		std::lock_guard<spring::mutex> lock(wdmutex);

		if (num >= WDT_COUNT || registeredThreads[num]->numreg != 0) {
			LOG_L(L_ERROR, "[Watchdog::%s] Invalid thread number %u", __func__, num);
			return;
		}

		Threading::NativeThreadHandle thread = Threading::GetCurrentThread();
		Threading::NativeThreadId threadId = Threading::GetCurrentThreadId();

		unsigned int inact = WDT_COUNT;
		unsigned int i;

		for (i = 0; i < WDT_COUNT; ++i) {
			const WatchDogThreadInfo* threadInfo = &registeredThreadsData[i];

			// determine first inactive thread; stop if thread
			// is already registered under a different wdt-num
			if (threadInfo->numreg == 0)
				inact = std::min(i, inact);
			else if (Threading::NativeThreadIdsEqual(threadInfo->threadid, threadId))
				break;
		}

		if (i >= WDT_COUNT)
			i = inact;
		if (i >= WDT_COUNT) {
			LOG_L(L_ERROR, "[Watchdog::%s] Internal error", __func__);
			return;
		}

		registeredThreads[num] = &registeredThreadsData[i];

		// caller optionally does this so it can choose arbitrary names
		// Threading::SetThreadName(threadNames[num]);

		WatchDogThreadInfo* threadInfo = registeredThreads[num];
		threadInfo->thread = thread;
		threadInfo->threadid = threadId;
		threadInfo->timer = spring_gettime();
		threadInfo->numreg += 1;

		// note: WDT_MAIN and WDT_LOAD share the same controls if LoadingMT=0
		LOG("[WatchDog::%s] registering controls for thread [%s]", __func__, threadNames[num]);
		threadInfo->SetThreadControls();

		threadSlots[num].primary = primary;
		threadSlots[num].regorder = ++curorder;

		UpdateActiveThreads(threadId);
	}


	bool DeregisterThread(WatchdogThreadnum num)
	{
		std::lock_guard<spring::mutex> lock(wdmutex);

		WatchDogThreadInfo* threadInfo = nullptr;

		if (num >= WDT_COUNT || registeredThreads[num] == nullptr || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s] invalid thread number %u", __func__, num);
			return false;
		}

		threadSlots[num].primary = false;
		threadSlots[num].regorder = 0;

		UpdateActiveThreads(threadInfo->threadid);

		// reset the main thread's controls only if actually called from it;
		// otherwise Load would act in place of Main in the LoadingMT=0 case
		if (num == WDT_MAIN || !Threading::IsMainThread()) {
			LOG("[WatchDog::%s] deregistering controls for thread [%s]", __func__, threadNames[num]);
			threadInfo->ResetThreadControls();
		}

		if (0 == --(threadInfo->numreg))
			threadInfo->ResetThreadInfo();

		registeredThreads[num] = &registeredThreadsData[WDT_COUNT];
		return true;
	}

	bool DeregisterCurrentThread()
	{
		if (Threading::IsMainThread())
			return (DeregisterThread(WDT_MAIN));
		if (Threading::IsGameLoadThread())
			return (DeregisterThread(WDT_LOAD));
		if (Threading::IsAudioThread())
			return (DeregisterThread(WDT_AUDIO));
		if (Threading::IsFileSysThread())
			return (DeregisterThread(WDT_VFSI));

		return false;
	}

	bool HasThread(WatchdogThreadnum num) {
		return ((registeredThreads[num] != nullptr) && (registeredThreads[num] != &registeredThreadsData[WDT_COUNT]));
	}


	void ClearTimer(Threading::NativeThreadId* _threadId, bool disable)
	{
		// bail if Watchdog isn't running
		if (!hangDetectorThread.joinable())
			return;
		// calling thread can be the watchdog thread
		// itself (see HangDetectorLoop), which does
		// not count here and is unregistered anyway
		if (Threading::IsWatchDogThread())
			return;

		Threading::NativeThreadId threadId;

		if (_threadId != nullptr) {
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
			LOG_L(L_ERROR, "[Watchdog::%s(id)] Invalid thread %d (_threadId=%p)", __func__, num, _threadId);
			return;
		}

		// notime always satisfies !spring_istime
		threadInfo->timer = disable ? spring_notime : spring_gettime();
	}


	void ClearTimer(WatchdogThreadnum num, bool disable)
	{
		if (!hangDetectorThread.joinable())
			return;
		if (Threading::IsWatchDogThread())
			return;

		WatchDogThreadInfo* threadInfo;

		if (num >= WDT_COUNT || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s(num)] Invalid thread %d", __func__, num);
			return;
		}

		threadInfo->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearTimer(const char* name, bool disable)
	{
		if (!hangDetectorThread.joinable())
			return;
		if (Threading::IsWatchDogThread())
			return;

		const auto i = threadNumTable.find(hashString(name));
		unsigned int num;
		WatchDogThreadInfo* threadInfo;

		if (i == threadNumTable.end() || (num = i->second) >= WDT_COUNT || (threadInfo = registeredThreads[num])->numreg == 0) {
			LOG_L(L_ERROR, "[Watchdog::%s(name)] Invalid thread name \"%s\"", __func__, name);
			return;
		}

		threadInfo->timer = disable ? spring_notime : spring_gettime();
	}

	void ClearTimers(bool disable, bool primary)
	{
		// bail if Watchdog is not running
		if (!hangDetectorThread.joinable())
			return;

		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			WatchDogThreadInfo* threadInfo = registeredThreads[i];

			if (!primary || threadSlots[i].primary)
				threadInfo->timer = disable ? spring_notime : spring_gettime();
		}
	}


	void Install()
	{
		std::lock_guard<spring::mutex> lock(wdmutex);

		memset(registeredThreadsData, 0, sizeof(registeredThreadsData));
		for (unsigned int i = 0; i < WDT_COUNT; ++i) {
			registeredThreads[i] = &registeredThreadsData[WDT_COUNT];
			threadNumTable[hashString(threadNames[i])] = i;
		}
		memset(threadSlots, 0, sizeof(threadSlots));

		// disable if gdb is running
		if (Platform::IsRunningInGDB()) {
			LOG("[WatchDog::%s] disabled (gdb detected)", __func__);
			return;
		}
	#ifdef USE_VALGRIND
		if (RUNNING_ON_VALGRIND) {
			LOG("[WatchDog::%s] disabled (Valgrind detected)", __func__);
			return;
		}
	#endif
		const int hangTimeoutSecs = configHandler->GetInt("HangTimeout");

		// HangTimeout = -1 to force disable hang detection
		if (hangTimeoutSecs <= 0) {
			LOG("[WatchDog::%s] disabled", __func__);
			return;
		}

		hangTimeout = spring_secs(hangTimeoutSecs);

		// start the watchdog thread
		hangDetectorThread = std::move(spring::thread(&HangDetectorLoop));

		LOG("[WatchDog::%s] installed (hang-timeout: %is)", __func__, hangTimeoutSecs);
	}


	void Uninstall()
	{
		LOG_L(L_INFO, "[WatchDog::%s][1] hangDetectorThread=%p (joinable=%d)", __func__, &hangDetectorThread, hangDetectorThread.joinable());

		if (!hangDetectorThread.joinable())
			return;

		std::lock_guard<spring::mutex> lock(wdmutex);

		hangDetectorThreadInterrupted = true;

		LOG_L(L_INFO, "[WatchDog::%s][2]", __func__);
		hangDetectorThread.join();
		LOG_L(L_INFO, "[WatchDog::%s][3]", __func__);

		memset(registeredThreadsData, 0, sizeof(registeredThreadsData));
		for (unsigned int i = 0; i < WDT_COUNT; ++i)
			registeredThreads[i] = &registeredThreadsData[WDT_COUNT];
		memset(threadSlots, 0, sizeof(threadSlots));
		threadNumTable.clear();
	}
}
