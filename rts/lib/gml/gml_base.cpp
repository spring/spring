/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_GML

#include "gml_base.h"
#include "gmlsrv.h"

#include "Sim/Units/Unit.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/OffscreenGLContext.h"
#include "System/SpringApp.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"

//GPU debug tools report calls to gl functions from different threads as errors -> disable in safemode
CONFIG(bool, MultiThreadShareLists).defaultValue(true).safemodeValue(false).description("render-helper threads");
CONFIG(bool, MultiThreadSim).defaultValue(true);


extern gmlClientServer<void, int, CUnit*> *gmlProcessor;

COffscreenGLContext* ogc[GML_MAX_NUM_THREADS] = { NULL };



#if GML_ENABLE_SIM

static void gmlSimLoop(void*)
{
	try {
		while(gmlKeepRunning && !gmlStartSim) {
			// the other thread may ClearPrimaryTimers(), so it is not enough to disable the watchdog timer once
			Watchdog::ClearTimer(WDT_SIM, true);
			SDL_Delay((gs->frameNum > 1) ? 500 : 100);
		}

		if (gmlKeepRunning) {
			if (gmlShareLists)
				ogc[0]->WorkerThreadPost();

			Threading::SetAffinityHelper("Sim", configHandler->GetUnsigned("SetCoreAffinitySim"));

			Watchdog::ClearTimer(WDT_SIM);

			while(gmlKeepRunning) {
				if(!gmlMultiThreadSim) {
					while(!gmlMultiThreadSim && gmlKeepRunning) {
						Watchdog::ClearTimer(WDT_SIM, true);
						SDL_Delay(500);
					}
				}

				//FIXME activeController could change while processing this branch. Better make it safe with a mutex?
				if (activeController) {
					Watchdog::ClearTimer(WDT_SIM);
					gmlProcessor->ExpandAuxQueue();

					if (!Threading::UpdateGameController(activeController))
						gmlKeepRunning = false;

					gmlProcessor->GetQueue();
				}

				boost::thread::yield();
			}

			if(gmlShareLists)
				ogc[0]->WorkerThreadFree();
		}
	} CATCH_SPRING_ERRORS
}

#endif

namespace GML {

	void Init()
	{
		if (!Enabled())
			return;
		gmlShareLists = configHandler->GetBool("MultiThreadShareLists");
		if (!gmlShareLists) {
			gmlMaxServerThreadNum = GML_LOAD_THREAD_NUM;
			gmlMaxShareThreadNum = GML_LOAD_THREAD_NUM;
			gmlNoGLThreadNum = GML_SIM_THREAD_NUM;
		}
		gmlThreadCountOverride = configHandler->GetInt("MultiThreadCount");
		gmlThreadCount = GML_CPU_COUNT;

		if (gmlShareLists) { // create offscreen OpenGL contexts
			for (int i = 0; i < gmlThreadCount; ++i)
				ogc[i] = new COffscreenGLContext();
		}

		gmlProcessor = new gmlClientServer<void, int, CUnit*>;
	#if GML_ENABLE_SIM
		gmlMultiThreadSim = configHandler->GetBool("MultiThreadSim");

		gmlKeepRunning = true;
		gmlStartSim = false;

		// start sim thread
		gmlProcessor->AuxWork(&gmlSimLoop, NULL);
	#endif
	}

	void Exit()
	{
		if (!Enabled())
			return;
		if (gmlProcessor) {
	#if GML_ENABLE_SIM
			gmlKeepRunning = false; // wait for sim to finish
			while (!gmlProcessor->PumpAux())
				boost::thread::yield();
	#endif
			delete gmlProcessor;
			gmlProcessor = NULL;

			if (gmlShareLists) {
				for (int i = 0; i < gmlThreadCount; ++i) {
					delete ogc[i];
					ogc[i] = NULL;
				}
			}
		}
	}

	void PumpAux()
	{
	#if GML_ENABLE_SIM
		if (gmlProcessor)
			gmlProcessor->PumpAux();
	#endif
	}

	bool SimThreadRunning()
	{
		if (!Enabled())
			return false;
	#if GML_ENABLE_SIM
		if (!gmlStartSim && gmlMultiThreadSim && gs->frameNum > 0) {
			gmlStartSim = true;
		}
		return (gmlStartSim && gmlMultiThreadSim);
	#else
		return false;
	#endif
	}

	void PrintStartupMessage(int showMTInfo) {
		if (showMTInfo == -1)
			return;
		if (showMTInfo != MT_LUA_NONE) {
			if (showMTInfo == MT_LUA_SINGLE || showMTInfo == MT_LUA_SINGLE_BATCH || showMTInfo == MT_LUA_DUAL_EXPORT) {
				LOG("[Threading] Multithreading is enabled but currently running in compatibility mode %d", showMTInfo);
			} else {
				LOG("[Threading] Multithreading is enabled and currently running in mode %d", showMTInfo);
			}
			if (showMTInfo == MT_LUA_SINGLE) {
				CKeyBindings::HotkeyList lslist = keyBindings->GetHotkeys("luaui selector");
				std::string lskey = lslist.empty() ? "" : " (press " + lslist.front() + ")";
				LOG("[Threading] Games that use lua based rendering may run very slow in this mode, "
					"indicated by a high LUA-SYNC-CPU(MT) value displayed in the upper right corner");
				LOG("[Threading] Consider MultiThreadLua = %d in the settings to improve performance, "
					"or try to disable LuaShaders and all rendering widgets%s", (int)MT_LUA_SINGLE_BATCH, lskey.c_str());
			} else if (showMTInfo == MT_LUA_SINGLE_BATCH) {
				LOG("[Threading] Games that use lua gadget based rendering may run very slow in this mode, "
					"indicated by a high LUA-SYNC-CPU(MT) value displayed in the upper right corner");
			} else if (showMTInfo == MT_LUA_DUAL_EXPORT) {
				LOG("[Threading] Games that use lua gadgets which export data may run very slow in this mode, "
					"indicated by a high LUA-EXP-SIZE(MT) value displayed in the upper right corner");
			}
		} else {
			LOG("[Threading] Multithreading is disabled because the game or system appears incompatible");
			LOG("[Threading] MultiThreadCount > 1 in the settings will forcefully enable multithreading");
		}
	}
};

#endif // USE_GML
