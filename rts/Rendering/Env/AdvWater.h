/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ADV_WATER_H__
#define __ADV_WATER_H__

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "BaseWater.h"

class CAdvWater : public CBaseWater
{
public:
	CAdvWater(bool loadShader = true);
	virtual ~CAdvWater();
	virtual int GetID() const { return WATER_RENDERER_REFLECTIVE; }
	virtual const char* GetName() const { return "reflective"; }

	virtual void Draw();
	void Draw(bool useBlending);
	void UpdateWater(CGame* game);

protected:
	FBO reflectFBO;
	FBO bumpFBO;

	GLuint reflectTexture;
	GLuint bumpTexture;
	GLuint rawBumpTexture[4];
	float3 waterSurfaceColor;

	unsigned int waterFP;
};

#endif // __ADV_WATER_H__
