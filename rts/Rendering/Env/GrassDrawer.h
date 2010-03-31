/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GRASSDRAWER_H
#define GRASSDRAWER_H

#include <vector>
#include "System/float3.h"

namespace Shader {
	struct IProgramObject;
}

class CVertexArray;
class CGrassDrawer
{
public:
	CGrassDrawer();
	~CGrassDrawer(void);

	void Draw(void);
	void AddGrass(const float3& pos);
	void ResetPos(const float3& pos);
	void RemoveGrass(int x, int z);

	struct InviewGrass {
		int num;
		float dist;
	};
	struct InviewNearGrass {
		int x;
		int y;
		float dist;
	};
	struct GrassStruct {
		CVertexArray* va;
		int lastSeen;
		int square;
		float3 pos;
	};
	struct NearGrassStruct {
		float rotation;
		int square;
	};

	enum GrassShaderProgram {
		GRASS_PROGRAM_NEAR_SHADOW = 0,  // near-grass shader (V+F) with self-shadowing
		GRASS_PROGRAM_DIST_SHADOW = 1,  // far-grass shader (V+F) with self-shadowing
		GRASS_PROGRAM_DIST_BASIC  = 2,  // far-grass shader (V) without self-shadowing
		GRASS_PROGRAM_LAST        = 3
	};

protected:
	void LoadGrassShaders();
	void CreateGrassBladeTex(unsigned char* buf);
	void CreateFarTex(void);

	GrassStruct grass[32 * 32];
	NearGrassStruct nearGrass[32 * 32];

	int lastListClean;
	void CreateGrassDispList(int listNum);

	friend class CGrassBlockDrawer;

	bool grassOff;

	int blocksX;
	int blocksY;

	unsigned int grassDL;
	unsigned int grassBladeTex;
	unsigned int farTex;

	std::vector<Shader::IProgramObject*> grassShaders;

	float maxGrassDist;
	float maxDetailedDist;
	int detailedBlocks;
	int numTurfs;
	int strawPerTurf;

	unsigned char* grassMap;
};


#endif /* GRASSDRAWER_H */
