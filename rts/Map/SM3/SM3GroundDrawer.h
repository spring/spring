/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF3_GROUND_DRAWER_H_
#define _SMF3_GROUND_DRAWER_H_

#include "Frustum.h"
#include "terrain/Terrain.h" // for Camera
#include "Map/BaseGroundDrawer.h"

class CSM3ReadMap;
namespace terrain {
	class Terrain;
	class RenderContext;
}

class CSM3GroundDrawer : public CBaseGroundDrawer
{
public:
	CSM3GroundDrawer(CSM3ReadMap* map);
	~CSM3GroundDrawer();

	void Draw(const DrawPass::e& drawPass);
	void DrawShadowPass();
	void Update();

	void IncreaseDetail();
	void DecreaseDetail();
	void SetDetail(int newGroundDetail);
	int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const;

protected:
	void DrawObjects(const DrawPass::e& drawPass);

	CSM3ReadMap* map;

	terrain::Terrain* tr;
	terrain::RenderContext* rc;
	terrain::RenderContext* shadowrc;
	terrain::RenderContext* reflectrc;
	terrain::Camera cam;
	terrain::Camera shadowCam;
	terrain::Camera reflectCam;
	Frustum frustum;

	friend class CSM3ReadMap;
};

#endif // _SMF3_GROUND_DRAWER_H_

