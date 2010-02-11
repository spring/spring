
#include "Map/BaseGroundDrawer.h"
#include "terrain/TerrainBase.h"
#include "terrain/Terrain.h"
#include "Frustum.h"

class CSm3ReadMap;

class CSm3GroundDrawer : public CBaseGroundDrawer
{
public:
	CSm3GroundDrawer(CSm3ReadMap *map);
	~CSm3GroundDrawer();

	void Draw(bool drawWaterReflection,bool drawUnitReflection);
	void DrawShadowPass(void);
	void Update();

	void IncreaseDetail();
	void DecreaseDetail();

protected:
	void DrawObjects(bool drawWaterReflection,bool drawUnitReflection);

	CSm3ReadMap *map;

	terrain::Terrain *tr;
	terrain::RenderContext *rc, *shadowrc, *reflectrc;
	terrain::Camera cam, shadowCam, reflectCam;
	Frustum frustum;

	friend class CSm3ReadMap;
};
