/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ADV_WATER_H
#define ADV_WATER_H

#include "IWater.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"

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

	GLuint reflectTexture = 0;
	GLuint bumpmapTexture = 0;

	GLuint rawBumpTextures[4] = {0, 0, 0, 0};

	Shader::IProgramObject* waterShader = nullptr;
};

#endif // ADV_WATER_H
