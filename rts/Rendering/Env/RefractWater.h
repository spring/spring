/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REFRACTEDWATER_H
#define REFRACTEDWATER_H

#include "AdvWater.h"

class CRefractWater : public CAdvWater 
{
public:
	CRefractWater();
	~CRefractWater();

	void LoadGfx();

	void Draw();
	int GetID() const { return WATER_RENDERER_REFL_REFR; }
	const char* GetName() const { return "reflective&refractive"; }

protected:
	void SetupWaterDepthTex();

	unsigned int target;
	GLuint subSurfaceTex; // the screen is copied into this texture and used for water rendering
};

#endif
