/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKYBOX_H
#define SKYBOX_H

#include <string>
#include "BaseSky.h"

class CSkyBox : public IBaseSky
{
public:
	CSkyBox(const std::string&);
	~CSkyBox();

	void Draw();
	void Update() {}
	void UpdateSunDir() {}
	void UpdateSkyTexture() {}
	void DrawSun() {}

private:
	unsigned int tex;
};

#endif
