/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GRASSDRAWER_H
#define GRASSDRAWER_H

#include "Rendering/GL/myGL.h"

class CVertexArray;

class CGrassDrawer
{
public:
	CGrassDrawer();
	~CGrassDrawer(void);

	void Draw(void);
	void AddGrass(float3 pos);
	void ResetPos(const float3& pos);
	void RemoveGrass(int x, int z);

protected:
	void CreateGrassBladeTex(unsigned char* buf);
	void CreateFarTex(void);

	struct GrassStruct {
		CVertexArray* va;
		int lastSeen;
		int square;
		float3 pos;
	};
	GrassStruct grass[32*32];
	int lastListClean;

	struct NearGrassStruct{
		float rotation;
		int square;
	};
	NearGrassStruct nearGrass[32*32];

	void CreateGrassDispList(int listNum);

	friend class CGrassBlockDrawer;

	bool grassOff;

	int blocksX;
	int blocksY;

	unsigned int grassDL;
	GLuint grassBladeTex;
	GLuint farTex;

	unsigned int grassVP;
	unsigned int grassFarVP;
	unsigned int grassFarNSVP;

	float maxGrassDist;
	float maxDetailedDist;
	int detailedBlocks;
	int numTurfs;
	int strawPerTurf;

	unsigned char* grassMap;
	void SetTexGen(float scalex,float scaley, float offsetx, float offsety);
};


#endif /* GRASSDRAWER_H */
