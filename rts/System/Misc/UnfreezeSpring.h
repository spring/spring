#pragma once

#include "System/Platform/Watchdog.h"
#include "System/Input/InputHandler.h"

namespace spring {
	static void UnfreezeSpring(WatchdogThreadnum num = WDT_MAIN, int everyN = 1) {
		assert(everyN > 0);
		static int counter = 0;
		static int pEveryN = -1;
		if (everyN != pEveryN) {
			counter = 0;
			pEveryN = everyN;
		}

		if (counter % everyN == 0) {
			Watchdog::ClearTimer(num);
			input.PushEvents();
		}
		++counter;
	}
}