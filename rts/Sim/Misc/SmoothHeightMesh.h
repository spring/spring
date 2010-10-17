/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOOTH_HEIGHT_MESH_H
#define SMOOTH_HEIGHT_MESH_H

class CGround;

/**
 * This class requires that BaseMesh objects support GetHeight(float x, float y) method.
 * Provides a GetHeight(x, y) of its own that smooths the mesh.
 */
class SmoothHeightMesh
{
protected:
	int maxx, maxy;
	float fmaxx, fmaxy;
	float resolution;
	float smoothRadius;

	float* mesh;
	float* origMesh;

public:
	bool drawEnabled;

	SmoothHeightMesh(const CGround* ground, float mx, float my, float res, float smoothRad);
	~SmoothHeightMesh();

	void MakeSmoothMesh(const CGround* ground);

	float GetHeight(float x, float y);
	float GetHeight2(float x, float y);
	float SetHeight(int index, float h);
	float AddHeight(int index, float h);
	float SetMaxHeight(int index, float h);

	int GetMaxX() const { return maxx; }
	int GetMaxY() const { return maxy; }
	float GetResolution() const { return resolution; }

	float* GetMeshData() { return mesh; }
	float* GetOriginalMeshData() { return origMesh; }

	void DrawWireframe(float yoffset);
};

extern SmoothHeightMesh* smoothGround;

#endif // SMOOTH_HEIGHT_MESH_H
