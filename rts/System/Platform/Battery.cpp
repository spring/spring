/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Battery.h"
#include "System/EventHandler.h"
#include "System/Threading/ThreadPool.h"
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
	const spring_time now = spring_gettime();
	if (next_check > now)
		return;
	next_check = now + spring_secs(5);

	// SDL_GetPowerInfo() seems to cause a lag of (milli-)secs
	// run it threaded, so that it doesn't block the mainthread
	ThreadPool::Enqueue([&]{
		int secs, pct;
		onBattery = (SDL_GetPowerInfo(&secs, &pct) == SDL_POWERSTATE_ON_BATTERY);
	});
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

