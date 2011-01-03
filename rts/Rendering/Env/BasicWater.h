/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASIC_WATER_H__
#define __BASIC_WATER_H__

#include "Rendering/GL/myGL.h"
#include "BaseWater.h"

class CBasicWater : public CBaseWater
{
public:
	CBasicWater();
	~CBasicWater();

	void Draw();
	void UpdateWater(CGame*) {}
	int GetID() const { return WATER_RENDERER_BASIC; }
	const char* GetName() const { return "basic"; }

	GLuint texture;
	unsigned int displist;
	int textureWidth;
	int textureHeight;
};

#endif // __BASIC_WATER_H__
