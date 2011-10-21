/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include "Landscape.h"
#include "Map/SMF/IMeshDrawer.h"

class CSMFReadMap;
class CSMFGroundDrawer;

/**
 * Map mesh drawer implementation
 */
class CRoamMeshDrawer : public IMeshDrawer //FIXME merge with landscape class
{
public:
	CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update();

	void DrawMesh(const DrawPass::e& drawPass);

private:
	CSMFReadMap* smfReadMap;
	CSMFGroundDrawer* smfGroundDrawer;

	Landscape landscape;
	bool* visibilitygrid;
	
	float3 lastCamPos;
	int lastViewRadius;

	int numBigTexX;
	int numBigTexY;
};

#endif // _ROAM_MESH_DRAWER_H_
