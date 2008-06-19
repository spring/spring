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
class CBFGroundDrawer :
	public CBaseGroundDrawer
{
public:
	CBFGroundDrawer(CSmfReadMap* rm);
	~CBFGroundDrawer(void);
	void Draw(bool drawWaterReflection = false, bool drawUnitReflection = false, unsigned int overrideVP = 0);

	void IncreaseDetail();
	void DecreaseDetail();

protected:
	int viewRadius;
	CSmfReadMap* map;
	CBFGroundTextures* textures;

	const int bigSquareSize;
	const int numBigTexX;
	const int numBigTexY;

	float* heightData;
	const int heightDataX;

	CVertexArray* va;

	struct fline {
		float base;
		float dir;
	};
	std::vector<fline> right,left;

	friend class CSmfReadMap;

	unsigned int groundVP;
	unsigned int groundShadowVP;
	unsigned int groundFPShadow;
	bool waterDrawn;

	volatile unsigned int mt_overrideVP;

  bool DrawMT(int bty);
	static void DrawMTcb(void *c,int bty) {((CBFGroundDrawer *)c)->DrawMT(bty);}
	void DrawVertexAMT(CVertexArray *ma, int x, int y);
	void DrawVertexAMT(CVertexArray *ma, int x, int y, float height);
  void EndStripMT(CVertexArray *ma);
  void DrawGroundVertexArrayMT(CVertexArray * &ma);
	bool DrawShadowPassMT(int nlod);
	static void DrawShadowPassMTcb(void *c,int nlod) {((CBFGroundDrawer *)c)->DrawShadowPassMT(nlod);}

	inline void DrawVertexA(int x, int y);
	inline void DrawVertexA(int x, int y, float height);
	inline void EndStrip();
	inline void DrawWaterPlane(bool);
	inline bool BigTexSquareRowVisible(int);
	inline void DrawGroundVertexArray();
	void SetupTextureUnits(bool drawReflection, unsigned int overrideVP);
	void ResetTextureUnits(bool drawReflection, unsigned int overrideVP);

	void AddFrustumRestraint(const float3& side);
	void UpdateCamRestraints();
	void Update() {}
public:
	void DrawShadowPass();
};

#endif // __BF_GROUND_DRAWER_H__
