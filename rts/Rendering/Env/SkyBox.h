/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKY_BOX_H
#define SKY_BOX_H

#include <string>

#include "ISky.h"
#include "Map/MapTexture.h"
#include "Rendering/GL/RenderDataBuffer.hpp"


class CSkyBox : public ISky
{
public:
	CSkyBox(const std::string& texture);
	~CSkyBox();

	void Update() override {}
	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}

	void Draw(Game::DrawMode mode) override;
	void DrawSun(Game::DrawMode mode) override {}

	void SetLuaTexture(const MapTextureData& td) override
	{
		skyTex.SetLuaTexture(td);
	}

private:
	void LoadTexture(const std::string& texture);
	void LoadBuffer();

private:
	struct SkyBoxVertType {
		float2 xy;
		float3 tc;
	};

	typedef Shader::ShaderInput SkyBoxAttrType;
	typedef GL::TRenderDataBuffer<SkyBoxVertType> SkyBoxBuffer;


	MapTexture skyTex;
	SkyBoxBuffer skyBox;


	SkyBoxVertType* vtxPtr = nullptr;
	SkyBoxVertType* vtxPos = nullptr;
};

#endif // SKY_BOX_H
