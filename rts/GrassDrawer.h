#pragma once



class CAdvTreeDrawer;
class CVertexArray;

class CGrassDrawer
{
public:
	CGrassDrawer();
	~CGrassDrawer(void);

	void Draw(void);
	void CreateGrassBladeTex(unsigned char* buf);
	void CreateFarTex(void);
	void AddGrass(float3 pos);

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

	void ResetPos(const float3& pos);
	void CreateGrassDispList(int listNum);

	bool grassOff;

	int blocksX;
	int blocksY;

	unsigned int grassDL;
	unsigned int grassBladeTex;
	unsigned int farTex;


	unsigned int grassVP;
	unsigned int grassFarVP;
	unsigned int grassFarNSVP;

	float maxGrassDist;
	float maxDetailedDist;
	int detailedBlocks;
	int numTurfs;
	int strawPerTurf;

	unsigned char* grassMap;
	void RemoveGrass(int x, int z);
	void SetTexGen(float scalex,float scaley, float offsetx, float offsety);
};

