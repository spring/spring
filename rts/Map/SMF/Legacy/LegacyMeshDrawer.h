/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LEGACY_MESH_DRAWER_H_
#define _LEGACY_MESH_DRAWER_H_

#include "Map/SMF/IMeshDrawer.h"
#include "System/float3.h"


class CVertexArray;
class CCamera;
class CSMFReadMap;
class CSMFGroundDrawer;


/**
 * Map drawer implementation
 */
class CLegacyMeshDrawer : public IMeshDrawer
{
public:
	CLegacyMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd);

	void Update() {}

	void DrawMesh(const DrawPass::e& drawPass);
	void DrawBorderMesh(const DrawPass::e& drawPass) {}
	void DrawShadowMesh();

private:
	void UpdateLODParams(const DrawPass::e& drawPass);

	void FindRange(const CCamera* cam, int& xs, int& xe, int y, int lod);
	inline bool BigTexSquareRowVisible(const CCamera* cam, int) const;

	inline void DrawVertexAQ(CVertexArray* ma, int x, int y);
	inline void DrawVertexAQ(CVertexArray* ma, int x, int y, float height);
	inline void DrawGroundVertexArrayQ(CVertexArray*& ma);

	void DoDrawGroundRow(const CCamera* cam, int bty);
	void DoDrawGroundShadowLOD(int nlod);

private:
	CSMFReadMap* smfReadMap;
	CSMFGroundDrawer* smfGroundDrawer;

	int viewRadius = 4;
	int neededLod = 4;
};

#endif // _LEGACY_MESH_DRAWER_H_
