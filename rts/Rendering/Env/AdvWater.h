/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ADV_WATER_H
#define ADV_WATER_H

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "IWater.h"

class CAdvWater : public IWater
{
public:
	CAdvWater();
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

#endif // ADV_WATER_H
