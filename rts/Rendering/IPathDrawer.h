/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#ifndef IPATHDRAWER_HDR
#define IPATHDRAWER_HDR

#include "System/Color.h"

struct MoveDef;

struct IPathDrawer {
public:
	IPathDrawer(): enabled(false) {}

	virtual ~IPathDrawer() {}
	virtual void DrawAll() const {}

	virtual void UpdateExtraTexture(int, int, int, int, unsigned char*) const {}

	bool ToggleEnabled() { enabled = !enabled; return enabled; }
	bool IsEnabled() const { return enabled; }

	static IPathDrawer* GetInstance();
	static void FreeInstance(IPathDrawer*);

	static const MoveDef* GetSelectedMoveDef();
	static SColor GetSpeedModColor(const float sm);
	static float GetSpeedModNoObstacles(const MoveDef* md, int sqx, int sqz);

protected:
	bool enabled;
};

extern IPathDrawer* pathDrawer;

#endif

