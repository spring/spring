/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOOTH_HEIGHTMESH_DRAWER_H
#define SMOOTH_HEIGHTMESH_DRAWER_H

struct SmoothHeightMeshDrawer {
private:
	SmoothHeightMeshDrawer();
	~SmoothHeightMeshDrawer();

public:
	static SmoothHeightMeshDrawer* GetInstance();

	void Draw(float yoffset);
	bool& DrawEnabled() { return drawEnabled; }

private:
	bool drawEnabled;
};

#define smoothHeightMeshDrawer (SmoothHeightMeshDrawer::GetInstance())

#endif // SMOOTH_HEIGHTMESH_DRAWER_H

