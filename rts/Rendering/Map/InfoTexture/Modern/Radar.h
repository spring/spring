/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RADAR_TEXTURE_H
#define _RADAR_TEXTURE_H

#include "PboInfoTexture.h"


class CRadarTexture : public CPboInfoTexture
{
public:
	CRadarTexture();

public:
	void Update();
	bool IsUpdateNeeded() { return true; }
};

#endif // _RADAR_TEXTURE_H
