/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOOTH_HEIGHTMESH_DRAWER_H
#define SMOOTH_HEIGHTMESH_DRAWER_H

#include "System/EventClient.h"

struct SmoothHeightMeshDrawer: public CEventClient {
public:
	static SmoothHeightMeshDrawer* GetInstance();
	static void FreeInstance();

	SmoothHeightMeshDrawer();
	~SmoothHeightMeshDrawer();

	// CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "DrawInMiniMap");
	}
	void DrawInMiniMap() override;


	void Draw(float yoffset);
	bool& DrawEnabled() { return drawEnabled; }

private:
	bool drawEnabled;
};

#define smoothHeightMeshDrawer (SmoothHeightMeshDrawer::GetInstance())

#endif // SMOOTH_HEIGHTMESH_DRAWER_H

