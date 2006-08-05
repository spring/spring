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

protected:
	void SetupWaterDepthTex();

	unsigned int target;
	unsigned int subSurfaceTex; // the screen is copied into this texture and used for water rendering
};

#endif
