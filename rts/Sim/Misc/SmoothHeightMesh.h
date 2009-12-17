#ifndef SMOOTHHEIGHTMESH_H
#define SMOOTHHEIGHTMESH_H

#include "Map/Ground.h"

/** This class requires that BaseMesh objects support GetHeight(float x, float y) method.

Provides a GetHeight(x, y) of its own that smooths the mesh.
*/

class SmoothHeightMesh
{
protected:
	int maxx, maxy;
	float fmaxx, fmaxy;
	float resolution;
	float smoothRadius;

	float *mesh;

public:
	bool drawEnabled;

	SmoothHeightMesh(const CGround* ground, float maxx_, float maxy_, float resolution_,
			 float smoothRadius_):
			fmaxx(maxx_), fmaxy(maxy_), resolution(resolution_),
			smoothRadius(smoothRadius_),
			mesh(0),
			drawEnabled(false)
	{
		maxx = fmaxx/resolution + 1;
		maxy = fmaxy/resolution + 1;
		MakeSmoothMesh(ground);
	};
	~SmoothHeightMesh() { delete[] mesh; mesh = 0; }

	void MakeSmoothMesh(const CGround* ground);

	float GetHeight(float x, float y);

	void DrawWireframe(float yoffset);
};


extern SmoothHeightMesh *smoothGround;

#endif // SMOOTHHEIGHTMESH_H
