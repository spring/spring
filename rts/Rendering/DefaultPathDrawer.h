/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEFAULT_PATHDRAWER_HDR
#define DEFAULT_PATHDRAWER_HDR

#include "IPathDrawer.h"
#include "System/EventClient.h"

class CPathManager;
class CPathFinderDef;
class CPathFinder;
class CPathEstimator;
struct UnitDef;

struct DefaultPathDrawer: public IPathDrawer, public CEventClient {
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "DrawInMiniMap");
	}
	void DrawInMiniMap();

public:
	DefaultPathDrawer();

	void DrawAll() const;
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
	void Draw(const CPathFinder*) const;
	void Draw(const CPathEstimator*) const;

private:
	CPathManager* pm;
};

#endif

