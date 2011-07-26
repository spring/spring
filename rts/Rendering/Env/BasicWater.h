/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASIC_WATER_H
#define BASIC_WATER_H

#include "Rendering/GL/myGL.h"
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

private:
	GLuint texture;
	unsigned int displist;
	int textureWidth;
	int textureHeight;
};

#endif // BASIC_WATER_H
