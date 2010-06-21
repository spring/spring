/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKYBOX_H
#define SKYBOX_H

#include "BaseSky.h"
#include <string>

class CSkyBox : public CBaseSky
{
public:
	CSkyBox(const std::string&);
	~CSkyBox(void);

	void Draw();
	void Update() {}
	void DrawSun(void) {}

private:
	unsigned int tex;
};

#endif
