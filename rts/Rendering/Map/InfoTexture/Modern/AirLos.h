/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AIRLOS_TEXTURE_H
#define _AIRLOS_TEXTURE_H

#include "PboInfoTexture.h"
#include "Rendering/GL/FBO.h"


namespace Shader {
	struct IProgramObject;
}


class CAirLosTexture : public CPboInfoTexture
{
public:
	CAirLosTexture();
	~CAirLosTexture();

public:
	void Update() override;
	bool IsUpdateNeeded() override { return true; }

private:
	void UpdateCPU();

private:
	FBO fbo;
	GLuint uploadTex;
	Shader::IProgramObject* shader;
};

#endif // _AIRLOS_TEXTURE_H
