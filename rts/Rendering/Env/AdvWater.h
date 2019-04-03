/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ADV_WATER_H
#define ADV_WATER_H

#include "IWater.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"

namespace Shader {
	struct IProgramObject;
};

class CAdvWater : public IWater
{
public:
	CAdvWater(bool refractive = false);
	virtual ~CAdvWater();

	int GetID() const override { return modes[subsurfTexture != 0]; }
	const char* GetName() const override { return names[subsurfTexture != 0]; }

	void Draw() override { Draw(subsurfTexture == 0); }

	void Draw(bool useBlending);
	void UpdateWater(CGame* game);

protected:
	constexpr inline static const char* names[] = {"reflective", "refractive"};
	constexpr inline static int modes[] = {WATER_RENDERER_REFLECTIVE, WATER_RENDERER_REFRACTIVE};

	FBO reflectFBO;
	FBO bumpFBO;

	GLuint reflectTexture = 0;
	GLuint bumpmapTexture = 0;
	GLuint subsurfTexture = 0; // screen is copied into this texture

	GLuint rawBumpTextures[4] = {0, 0, 0, 0};

	Shader::IProgramObject* waterShader = nullptr;
};

#endif // ADV_WATER_H
