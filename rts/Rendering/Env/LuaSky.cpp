/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSky.h"

#include "Rendering/GlobalRendering.h"
#include "System/EventHandler.h"

void CLuaSky::Draw()
{
	if (!globalRendering->drawSky)
		return;

	eventHandler.DrawSky();
}

void CLuaSky::DrawSun()
{
	if (!globalRendering->drawSky)
		return;

	eventHandler.DrawSun();
}

