// BFGroundDrawer.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_GROUND_DRAWER_H__
#define __BF_GROUND_DRAWER_H__

#include "Map/BaseGroundDrawer.h"

class CVertexArray;
class CSmfReadMap;
class CBFGroundTextures;


/**
Map drawer implementation for the CSmfReadMap map system.
*/
class CBFGroundDrawer : public CBaseGroundDrawer
{
public:
	CBFGroundDrawer(CSmfReadMap* rm);
	~CBFGroundDrawer(void);

	void Draw(bool drawWaterReflection = false, bool drawUnitReflection = false, unsigned int VP = 0);
	void DrawShadowPass();
	void Update();

	void IncreaseDetail();
	void DecreaseDetail();

protected:
	int viewRadius;
	CSmfReadMap* map;
	CBFGroundTextures* textures;

	const int bigSquareSize;
	const int numBigTexX;
	const int numBigTexY;
	const int maxIdx;

	int mapWidth;
	int bigTexH;

	int neededLod;

	float* heightData;
	const int heightDataX;

	struct fline {
		float base;
		float dir;
	};
	std::vector<fline> right,left;

	friend class CSmfReadMap;

	unsigned int groundVP;
	unsigned int groundShadowVP;
	unsigned int groundFPShadow;
	unsigned int overrideVP;
	bool waterDrawn;

#ifdef USE_GML
	static void DoDrawGroundRowMT(void *c,int bty) {((CBFGroundDrawer *)c)->DoDrawGroundRow(bty);}
	static void DoDrawGroundShadowLODMT(void *c,int nlod) {((CBFGroundDrawer *)c)->DoDrawGroundShadowLOD(nlod);}
#endif

	GLuint waterPlaneCamOutDispList;
	GLuint waterPlaneCamInDispList;

protected:
	void CreateWaterPlanes(const bool &camOufOfMap);
	inline void DrawWaterPlane(bool drawWaterReflection);

	void FindRange(int &xs, int &xe, const std::vector<fline> &left, const std::vector<fline> &right, int y, int lod);
	void DoDrawGroundRow(int bty);
	void DrawVertexAQ(CVertexArray *ma, int x, int y);
	void DrawVertexAQ(CVertexArray *ma, int x, int y, float height);
	void EndStripQ(CVertexArray *ma);
	void DrawGroundVertexArrayQ(CVertexArray * &ma);
	void DoDrawGroundShadowLOD(int nlod);

	inline bool BigTexSquareRowVisible(int);
	inline void SetupBigSquare(const int bigSquareX, const int bigSquareY);
	void SetupTextureUnits(bool drawReflection);
	void ResetTextureUnits(bool drawReflection);

	void AddFrustumRestraint(const float3& side);
	void UpdateCamRestraints();
};

#endif // __BF_GROUND_DRAWER_H__

