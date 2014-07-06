/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GRASSDRAWER_H
#define GRASSDRAWER_H

#include <vector>
#include "System/float3.h"
#include "System/EventClient.h"

namespace Shader {
	struct IProgramObject;
}

class CVertexArray;
struct VA_TYPE_TN;


class CGrassDrawer : public CEventClient
{
public:
	CGrassDrawer();
	~CGrassDrawer();

	void Draw();
	void DrawShadow();
	void AddGrass(const float3& pos);
	void ResetPos(const float3& pos);
	void RemoveGrass(const float3& pos);

	void ChangeDetail(int detail);

	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

public:
	// EventClient
	void UnsyncedHeightMapUpdate(const SRectangle& rect);
	void Update();

public:
	struct InviewNearGrass {
		int x;
		int y;
		float dist;
	};
	struct GrassStruct {
		GrassStruct()
			: posX(0)
			, posZ(0)
			, va(nullptr)
			, lastSeen(0)
			, lastDist(0.f)
		{}
		~GrassStruct();

		int posX, posZ;
		CVertexArray* va;
		int lastSeen;
		float lastDist;
	};

	enum GrassShaderProgram {
		GRASS_PROGRAM_NEAR        = 0,
		GRASS_PROGRAM_DIST        = 1,
		GRASS_PROGRAM_SHADOW_GEN  = 2,
		GRASS_PROGRAM_LAST        = 3
	};

protected:
	void LoadGrassShaders();
	void CreateGrassBladeTex(unsigned char* buf);
	void CreateFarTex();
	void CreateGrassDispList(int listNum);

	void EnableShader(const GrassShaderProgram type);
	void SetupGlStateNear();
	void ResetGlStateNear();
	void SetupGlStateFar();
	void ResetGlStateFar();
	void DrawNear(const std::vector<InviewNearGrass>& inviewGrass);
	void DrawFarBillboards(const std::vector<GrassStruct*>& inviewGrass);
	void DrawNearBillboards(const std::vector<InviewNearGrass>& inviewNearGrass);
	void DrawBillboard(const int x, const int y, const float dist, VA_TYPE_TN* va_tn);

	void ResetPos(const int grassBlockX, const int grassBlockZ);

protected:
	friend class CGrassBlockDrawer;

	bool grassOff;

	int blocksX;
	int blocksY;

	unsigned int grassDL;
	unsigned int grassBladeTex;
	unsigned int farTex;
	CVertexArray* farnearVA;

	std::vector<Shader::IProgramObject*> grassShaders;
	Shader::IProgramObject* grassShader;

	float maxGrassDist;
	float maxDetailedDist;
	int detailedBlocks;
	int numTurfs;
	int strawPerTurf;

	float3 oldCamPos;
	float3 oldCamDir;
	int lastVisibilityUpdate;
	bool updateBillboards;

	std::vector<GrassStruct> grass;
	unsigned char* grassMap;
};

extern CGrassDrawer* grassDrawer; //FIXME can be nullptr


#endif /* GRASSDRAWER_H */
