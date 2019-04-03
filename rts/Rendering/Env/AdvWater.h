/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ADV_WATER_H
#define ADV_WATER_H

#include "IWater.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"

namespace Shader {
	struct IProgramObject;
};
namespace {
	constexpr static const char* ADV_WATER_NAMES[2] = {"reflective", "refractive"};
	constexpr static int ADV_WATER_MODES[2] = {IWater::WATER_RENDERER_REFLECTIVE, IWater::WATER_RENDERER_REFRACTIVE};
};


class CAdvWater : public IWater
{
public:
	CAdvWater(bool refractive = false);
	virtual ~CAdvWater();

	int GetID() const override { return ADV_WATER_MODES[subsurfTexture != 0]; }
	const char* GetName() const override { return ADV_WATER_NAMES[subsurfTexture != 0]; }

	void Draw() override { Draw(subsurfTexture == 0); }

	void Draw(bool useBlending);
	void UpdateWater(CGame* game);

protected:
	FBO reflectFBO;
	FBO bumpFBO;

	GLuint reflectTexture = 0;
	GLuint bumpmapTexture = 0;
	GLuint subsurfTexture = 0; // screen is copied into this texture

	GLuint rawBumpTextures[4] = {0, 0, 0, 0};

	Shader::IProgramObject* waterShader = nullptr;
};

#endif // ADV_WATER_H
