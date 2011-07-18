/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF3_GROUND_DRAWER_H_
#define _SMF3_GROUND_DRAWER_H_

#include "Frustum.h"
#include "terrain/Terrain.h" // for Camera
#include "Map/BaseGroundDrawer.h"

class CSm3ReadMap;
namespace terrain {
	class Terrain;
	class RenderContext;
}

class CSm3GroundDrawer : public CBaseGroundDrawer
{
public:
	CSm3GroundDrawer(CSm3ReadMap* map);
	~CSm3GroundDrawer();

	void Draw(bool drawWaterReflection, bool drawUnitReflection);
	void DrawShadowPass();
	void Update();
	void UpdateSunDir() {}

	void IncreaseDetail();
	void DecreaseDetail();

protected:
	void DrawObjects(bool drawWaterReflection, bool drawUnitReflection);

	CSm3ReadMap* map;

	terrain::Terrain* tr;
	terrain::RenderContext* rc;
	terrain::RenderContext* shadowrc;
	terrain::RenderContext* reflectrc;
	terrain::Camera cam;
	terrain::Camera shadowCam;
	terrain::Camera reflectCam;
	Frustum frustum;

	friend class CSm3ReadMap;
};

#endif // _SMF3_GROUND_DRAWER_H_

