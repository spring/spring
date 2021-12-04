/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKY_BOX_H
#define SKY_BOX_H

#include <string>
#include "ISky.h"
#include "Map/MapTexture.h"

class CSkyBox : public ISky
{
public:
	CSkyBox(const std::string& texture);
	~CSkyBox();

	void Update() {}
	void UpdateSunDir() {}
	void UpdateSkyTexture() {}

	void Draw();
	void DrawSun() {}

	void SetLuaTexture(const MapTextureData& td)
	{
		skyTex.SetLuaTexture(td);
	}

private:
	MapTexture skyTex;
};

#endif // SKY_BOX_H
