/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKY_BOX_H
#define SKY_BOX_H

#include <string>
#include "ISky.h"

class CSkyBox : public ISky
{
public:
	CSkyBox(const std::string& texture);
	~CSkyBox();

	void Draw();
	void Update() {}
	void UpdateSunDir() {}
	void UpdateSkyTexture() {}
	void DrawSun() {}

private:
	unsigned int tex;
};

#endif // SKY_BOX_H
