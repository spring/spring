/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASIC_WATER_H__
#define __BASIC_WATER_H__

#include "Rendering/GL/myGL.h"
#include "BaseWater.h"

class CBasicWater : public CBaseWater
{
public:
	void Draw();
	void UpdateWater(CGame* game);
	CBasicWater();
	virtual ~CBasicWater();
	int GetID() const { return 0; }

	GLuint texture;
	unsigned int displist;
	int textureWidth;
	int textureHeight;
};

#endif // __BASIC_WATER_H__
