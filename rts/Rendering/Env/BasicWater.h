/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASIC_WATER_H
#define BASIC_WATER_H

#include "IWater.h"

class CBasicWater : public IWater
{
public:
	CBasicWater();
	~CBasicWater();

	void Draw();
	void UpdateWater(CGame* game) {}
	int GetID() const { return WATER_RENDERER_BASIC; }
	const char* GetName() const { return "basic"; }

	bool CanDrawReflectionPass() const override { return false; }
	bool CanDrawRefractionPass() const override { return false; }
private:
	unsigned int GenWaterQuadsList(unsigned int textureWidth, unsigned int textureHeight) const;

	unsigned int textureID;
	unsigned int displistID;
};

#endif // BASIC_WATER_H
