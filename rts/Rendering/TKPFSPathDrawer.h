/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TKPFS_PATHDRAWER_HDR
#define TKPFS_PATHDRAWER_HDR

#include "IPathDrawer.h"

class CPathFinderDef;
struct UnitDef;

namespace TKPFS {

class CPathManager;
class CPathFinder;
class CPathEstimator;

}

struct TKPFSPathDrawer: public IPathDrawer {
public:
	TKPFSPathDrawer();

	void DrawAll() const;
	void DrawInMiniMap();
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const;

	enum BuildSquareStatus {
		NOLOS          = 0,
		FREE           = 1,
		OBJECTBLOCKED  = 2,
		TERRAINBLOCKED = 3,
	};

private:
	void Draw() const;
	void Draw(const CPathFinderDef*) const;
	void Draw(const TKPFS::CPathFinder*) const;
	void Draw(const TKPFS::CPathEstimator*) const;

private:
	TKPFS::CPathManager* pm;
};

#endif

