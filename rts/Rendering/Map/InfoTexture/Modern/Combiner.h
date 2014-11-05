/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INFO_TEXTURE_COMBINER_H
#define _INFO_TEXTURE_COMBINER_H

#include "PboInfoTexture.h"
#include "Rendering/GL/FBO.h"
#include "System/Color.h"
#include <string>


namespace Shader {
	struct IProgramObject;
}


class CInfoTextureCombiner : public CPboInfoTexture
{
public:
	CInfoTextureCombiner();

	bool IsEnabled() { return !disabled; }
	const std::string& GetMode() const { return curMode; }
	void SwitchMode(const std::string& name);

public:
	void Update();
	bool IsUpdateNeeded() { return !disabled; }

private:
	bool CreateShader(const std::string& filename, const SColor clearColor = SColor(0.0f, 0.0f, 0.0f, 1.0f));

private:
	bool disabled;
	FBO fbo;
	Shader::IProgramObject* shader;
	std::string curMode;
};

#endif // _INFO_TEXTURE_COMBINER_H
