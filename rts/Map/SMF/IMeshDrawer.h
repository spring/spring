/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MESH_DRAWER_H_
#define _MESH_DRAWER_H_

#include "Map/BaseGroundDrawer.h" // DrawPass::e

class CSMFReadMap;
class CSMFGroundDrawer;

/**
 * Map mesh drawer implementation
 */
class IMeshDrawer
{
public:
	IMeshDrawer() {}
	IMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd) {}
	virtual ~IMeshDrawer() {}

	virtual void Update() = 0;
	virtual void DrawMesh(const DrawPass::e& drawPass) = 0;
	virtual void DrawBorderMesh(const DrawPass::e& drawPass) = 0;
};

#endif // _MESH_DRAWER_H_
