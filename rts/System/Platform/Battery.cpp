/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Battery.h"
#include "System/EventHandler.h"
#include "System/ThreadPool.h"
#include "System/Config/ConfigHandler.h"
#include <SDL2/SDL_power.h>


CBattery* battery = nullptr;


CBattery::CBattery()
	: CEventClient("[CBattery]", 314160, false)
	, onBattery(false)
{
	if (HasBattery()) {
		eventHandler.AddClient(this);
	}
}


CBattery::~CBattery()
{
	eventHandler.RemoveClient(this);
}


void CBattery::Update()
{
	// switch ThreadPool's busy-wait to sleep-wait to save more power

	const spring_time now = spring_gettime();
	if (next_check > now)
		return;
	next_check = now + spring_secs(5);

	int secs, pct;
	bool newOnBattery = (SDL_GetPowerInfo(&secs, &pct) == SDL_POWERSTATE_ON_BATTERY);
	if (newOnBattery == onBattery)
		return;

	if (onBattery = newOnBattery) {
		ThreadPool::SetThreadSpinTime(0);
	} else {
		ThreadPool::SetThreadSpinTime(configHandler->GetUnsigned("WorkerThreadSpinTime"));
	}
}


bool CBattery::HasBattery()
{
	int secs, pct;
	switch (SDL_GetPowerInfo(&secs, &pct)) {
		case SDL_POWERSTATE_ON_BATTERY:
		case SDL_POWERSTATE_CHARGING:
		case SDL_POWERSTATE_CHARGED:
			return true;
		//case SDL_POWERSTATE_UNKNOWN:
		//case SDL_POWERSTATE_NO_BATTERY:
		default:
			return false;
	}
}

