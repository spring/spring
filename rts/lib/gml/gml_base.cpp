/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_GML

#include "gml_base.h"
#include "gmlsrv.h"

#include "Game/GameController.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/OffscreenGLContext.h"
#include "System/SpringApp.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"

CONFIG(bool, MultiThreadShareLists).defaultValue(true);
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

					if(!GML::UpdateSim(activeController))
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

	bool UpdateSim(CGameController *ac) {
		GML_MSTMUTEX_LOCK(sim); // UpdateSim

		Threading::SetSimThread(true);
		bool ret = ac->Update();
		Threading::SetSimThread(false);
		return ret;
	}

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
};

#endif // USE_GML
