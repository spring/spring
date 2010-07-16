/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_DRAWER_HDR
#define PATH_DRAWER_HDR

class CPathManager;
class CPathFinderDef;
class CPathFinder;
class CPathEstimator;

struct PathDrawer {
public:
	void Draw() const;

	static PathDrawer* GetInstance();

private:
	void Draw(const CPathManager*) const;
	void Draw(const CPathFinderDef*) const;
	void Draw(const CPathFinder*) const;
	void Draw(const CPathEstimator*) const;
};

#define pathDrawer (PathDrawer::GetInstance())

#endif
