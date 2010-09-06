/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEFAULTPATHDRAWER_HDR
#define DEFAULTPATHDRAWER_HDR

#include "IPathDrawer.h"

class CPathManager;
class CPathFinderDef;
class CPathFinder;
class CPathEstimator;

struct DefaultPathDrawer: public IPathDrawer {
public:
	DefaultPathDrawer();

	void Draw() const;
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const;

private:
	void Draw(const CPathManager*) const;
	void Draw(const CPathFinderDef*) const;
	void Draw(const CPathFinder*) const;
	void Draw(const CPathEstimator*) const;

	CPathManager* pm;
};

#endif
