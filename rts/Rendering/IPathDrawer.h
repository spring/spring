/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#ifndef IPATHDRAWER_HDR
#define IPATHDRAWER_HDR

struct IPathDrawer {
	virtual ~IPathDrawer() {}
	virtual void Draw() const {}

	virtual void UpdateExtraTexture(int, int, int, int, unsigned char*) const {}

	static IPathDrawer* GetInstance();
	static void FreeInstance(IPathDrawer*);
};

extern IPathDrawer* pathDrawer;

#endif
