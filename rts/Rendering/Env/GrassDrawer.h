/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GRASSDRAWER_H
#define GRASSDRAWER_H

#include <array>
#include <vector>

#include "Rendering/GL/RenderDataBuffer.hpp"
#include "System/float3.h"
#include "System/type2.h"
#include "System/EventClient.h"

namespace Shader {
	struct IProgramObject;
}

class CCamera;
class CGrassDrawer : public CEventClient
{
public:
	CGrassDrawer();
	~CGrassDrawer();

	void Draw();
	void DrawShadow();
	void AddGrass(const float3& pos);
	void RemoveGrass(const float3& pos);
	uint8_t GetGrass(const float3& pos);

	float IncrDrawDistance();
	float DecrDrawDistance();
	void ChangeDetail(int detail);

	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	void HandleAction(int arg) {
		switch (arg) {
			case -1: { defDrawGrass = !defDrawGrass; luaDrawGrass = !luaDrawGrass; } break;
			case  0: { defDrawGrass =         false; luaDrawGrass =         false; } break;
			case  1: { defDrawGrass =          true; luaDrawGrass =         false; } break;
			case  2: { defDrawGrass =         false; luaDrawGrass =          true; } break;
			default: {                                                             } break;
		}
	}

	bool DefDrawGrass() const { return defDrawGrass; }
	bool LuaDrawGrass() const { return luaDrawGrass; }

public:
	// EventClient
	void UnsyncedHeightMapUpdate(const SRectangle& rect) override {}
	void Update() override;

protected:
	enum GrassShaderProgram {
		GRASS_PROGRAM_OPAQUE = 0,
		GRASS_PROGRAM_SHADOW = 1,
		GRASS_PROGRAM_CURR   = 2,
		GRASS_PROGRAM_LAST   = 3,
	};

	bool LoadGrassShaders();
	void CreateGrassBladeTex(uint8_t* buf);
	void CreateGrassBuffer();

	void EnableShader(const GrassShaderProgram type);
	void SetupStateOpaque();
	void ResetStateOpaque();
	void SetupStateShadow();
	void ResetStateShadow();

	unsigned int DrawBlock(const float3& camPos, const int2& blockPos, unsigned int turfMatIndex);
	void DrawBlocks(const CCamera* cam);

protected:
	int2 blockCount;
	int2 turfDetail;

	unsigned int grassBladeTex = 0;

	std::vector<int2> grassBlocks;
	std::vector<uint8_t> grassMap;

	std::array<Shader::IProgramObject*, GRASS_PROGRAM_LAST> grassShaders;

	GL::RenderDataBuffer grassBuffer;

	float grassDrawDist;
	float maxDetailedDist;

	float3 prvUpdateCamPos;
	float3 prvUpdateCamDir;

	bool luaDrawGrass = false;
	bool defDrawGrass = false;
	bool updateVisibility = false;
};

extern CGrassDrawer* grassDrawer;


#endif /* GRASSDRAWER_H */
