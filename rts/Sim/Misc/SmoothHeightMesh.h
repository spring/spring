/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOOTHHEIGHTMESH_H
#define SMOOTHHEIGHTMESH_H

#include "Map/Ground.h"

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

	float *mesh;
	float *origMesh;

public:
	bool drawEnabled;

	SmoothHeightMesh(const CGround* ground, float maxx_, float maxy_, float resolution_,
			 float smoothRadius_):
			fmaxx(maxx_), fmaxy(maxy_), resolution(resolution_),
			smoothRadius(smoothRadius_),
			mesh(0), origMesh(0),
			drawEnabled(false)
	{
		maxx = fmaxx/resolution + 1;
		maxy = fmaxy/resolution + 1;
		MakeSmoothMesh(ground);
	};
	~SmoothHeightMesh() { delete[] mesh; mesh = 0; delete[] origMesh; origMesh = 0; }

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


extern SmoothHeightMesh *smoothGround;

#endif // SMOOTHHEIGHTMESH_H
