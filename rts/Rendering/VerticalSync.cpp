/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "VerticalSync.h"
#include "GL/myGL.h"
#include "System/myMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#if defined HEADLESS
#elif defined WIN32
	#include <GL/wglew.h>
#elif !defined(__APPLE__)
	#include <GL/glxew.h>
#endif

#include <SDL_video.h>

static constexpr int MAX_ADAPTIVE_INTERVAL = -6;
static constexpr int MAX_STANDARD_INTERVAL = +6;

CONFIG(int, VSync).
	defaultValue(0).
	minimumValue(MAX_ADAPTIVE_INTERVAL).
	maximumValue(MAX_STANDARD_INTERVAL).
	description(
		"Synchronize buffer swaps with vertical blanking interval."
		" Modes are -N (adaptive), +N (standard), or 0 (disabled)."
	);


CVerticalSync VSync;


void CVerticalSync::Delay() const {}
void CVerticalSync::Toggle() {
	// no-arg switch, select smallest interval
	switch (Clamp(SDL_GL_GetSwapInterval(), -1, 1)) {
		case -1: { SetInterval( 0); } break;
		case  0: { SetInterval(+1); } break;
		case +1: { SetInterval(-1); } break;
		default: {} break;
	}
}

void CVerticalSync::SetInterval() { SetInterval(configHandler->GetInt("VSync")); }
void CVerticalSync::SetInterval(int i)
{
	configHandler->Set("VSync", interval = Clamp(i, MAX_ADAPTIVE_INTERVAL, MAX_STANDARD_INTERVAL));

	#if defined HEADLESS
	return;
	#endif

	// adaptive (delay swap iff frame-rate > vblank-rate)
	if (interval < 0 && SDL_GL_SetSwapInterval(interval) == 0) {
		LOG("[VSync::%s] interval=%d (adaptive)", __func__, interval);
		return;
	}
	// standard (<interval> vblanks per swap)
	if (interval > 0 && SDL_GL_SetSwapInterval(interval) == 0) {
		LOG("[VSync::%s] interval=%d (standard)", __func__, interval);
		return;
	}

	// disabled (never wait for vblank)
	SDL_GL_SetSwapInterval(0);
	LOG("[VSync::%s] interval=%d (disabled)", __func__, interval);
}
