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

CONFIG(int, HardwareThreadCount).defaultValue(0);
CONFIG(bool, MultiThreadShareLists).defaultValue(true);
CONFIG(bool, MultiThreadSim).defaultValue(true);


extern gmlClientServer<void, int, CUnit*> *gmlProcessor;

static COffscreenGLContext* ogc = NULL;



#if GML_ENABLE_SIM

static void gmlSimLoop(void*)
{
	try {
		Watchdog::ClearTimer(WDT_SIM, true);

		while(gmlKeepRunning && !gmlStartSim)
			SDL_Delay(100);

		if (gmlKeepRunning) {
			if (gmlShareLists)
				ogc->WorkerThreadPost();

			//Threading::SetAffinity(3);

			Watchdog::ClearTimer(WDT_SIM);

			while(gmlKeepRunning) {
				if(!gmlMultiThreadSim) {
					Watchdog::ClearTimer(WDT_SIM, true);
					while(!gmlMultiThreadSim && gmlKeepRunning)
						SDL_Delay(500);
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
				ogc->WorkerThreadFree();
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
		gmlShareLists = configHandler->GetBool("MultiThreadShareLists");
		if (!gmlShareLists) {
			gmlMaxServerThreadNum = GML_LOAD_THREAD_NUM;
			gmlNoGLThreadNum = GML_SIM_THREAD_NUM;
		}
		gmlThreadCountOverride = configHandler->GetInt("HardwareThreadCount");
		gmlThreadCount = GML_CPU_COUNT;

		gmlProcessor = new gmlClientServer<void, int, CUnit*>;
	#if GML_ENABLE_SIM
		gmlMultiThreadSim = configHandler->GetBool("MultiThreadSim");

		gmlKeepRunning = true;
		gmlStartSim = false;

		// create offscreen OpenGL context
		if (gmlShareLists)
			ogc = new COffscreenGLContext();

		// start sim thread
		gmlProcessor->AuxWork(&gmlSimLoop, NULL);
	#endif
	}

	void Exit()
	{
		if(gmlProcessor) {
	#if GML_ENABLE_SIM
			gmlKeepRunning = false; // wait for sim to finish
			while(!gmlProcessor->PumpAux())
				boost::thread::yield();
			if(gmlShareLists) {
				delete ogc;
				ogc = NULL;
			}
	#endif
			delete gmlProcessor;
			gmlProcessor = NULL;
		}
	}

	void PumpAux()
	{
	#if GML_ENABLE_SIM
		gmlProcessor->PumpAux();
	#endif
	}

	bool SimThreadRunning()
	{
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
