/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOOTH_HEIGHT_MESH_H
#define SMOOTH_HEIGHT_MESH_H

#include <vector>

class CGround;

/**
 * Provides a GetHeight(x, y) of its own that smooths the mesh.
 */
class SmoothHeightMesh
{
public:
	void Init(float mx, float my, float res, float smoothRad);
	void Kill();

	float GetHeight(float x, float y);
	float GetHeightAboveWater(float x, float y);
	float SetHeight(int index, float h);
	float AddHeight(int index, float h);
	float SetMaxHeight(int index, float h);

	int GetMaxX() const { return maxx; }
	int GetMaxY() const { return maxy; }
	float GetFMaxX() const { return fmaxx; }
	float GetFMaxY() const { return fmaxy; }
	float GetResolution() const { return resolution; }

	const float* GetMeshData() const { return &mesh[0]; }
	const float* GetOriginalMeshData() const { return &origMesh[0]; }

private:
	void MakeSmoothMesh();

	int maxx = 0;
	int maxy = 0;
	float fmaxx = 0.0f;
	float fmaxy = 0.0f;
	float resolution = 0.0f;
	float smoothRadius = 0.0f;

	std::vector<float> mesh;
	std::vector<float> origMesh;

	std::vector<float> colsMaxima;
	std::vector<int> maximaRows;
};

extern SmoothHeightMesh smoothGround;

#endif
