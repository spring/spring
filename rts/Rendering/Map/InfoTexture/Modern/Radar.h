/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RADAR_TEXTURE_H
#define _RADAR_TEXTURE_H


#include "PboInfoTexture.h"
#include "Rendering/GL/FBO.h"


namespace Shader {
	struct IProgramObject;
}


class CRadarTexture : public CPboInfoTexture
{
public:
	CRadarTexture();
	~CRadarTexture();

public:
	void Update() override;
	bool IsUpdateNeeded() override { return true; }

private:
	void UpdateCPU();

private:
	FBO fbo;
	GLuint uploadTexRadar;
	GLuint uploadTexJammer;
	Shader::IProgramObject* shader;
};

#endif // _RADAR_TEXTURE_H
