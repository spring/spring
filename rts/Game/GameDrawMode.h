/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_DRAW_MODE_H
#define _GAME_DRAW_MODE_H

namespace Game {
	enum DrawMode {
		NotDrawing     = 0,
		NormalDraw     = 1,
		ShadowDraw     = 2,
		ReflectionDraw = 3,
		RefractionDraw = 4,
		DeferredDraw   = 5,
	};
};

#endif

