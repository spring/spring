/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "PathCache.hpp"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Rectangle.h"

QTPFS::IPath* QTPFS::PathCache::GetLivePath(unsigned int pathID) {
	static IPath livePath; // dummy

	PathMap::iterator it = livePaths.find(pathID);

	if (it != livePaths.end()) {
		numCacheHits += 1;
		return (it->second);
	}

	numCacheMisses += 1;
	return &livePath;
}

QTPFS::IPath* QTPFS::PathCache::GetTempPath(unsigned int pathID) {
	static IPath tempPath; // dummy

	PathMap::iterator it = tempPaths.find(pathID);

	if (it != tempPaths.end()) {
		return (it->second);
	}

	return &tempPath;
}



void QTPFS::PathCache::AddTempPath(IPath* path) {
	assert(path->GetID() != 0);
	assert(path->NumPoints() == 2);
	assert(tempPaths.find(path->GetID()) == tempPaths.end());
	assert(livePaths.find(path->GetID()) == livePaths.end());

	tempPaths.insert(std::make_pair<unsigned int, IPath*>(path->GetID(), path));
}

void QTPFS::PathCache::AddLivePath(IPath* path, bool replace) {
	assert(path->GetID() != 0);
	assert(path->NumPoints() >= 2);

	if (!replace) {
		assert(tempPaths.find(path->GetID()) != tempPaths.end());
		assert(livePaths.find(path->GetID()) == livePaths.end());

		// promote a path from temporary- to live-status (no deletion)
		tempPaths.erase(path->GetID());
		livePaths.insert(std::make_pair<unsigned int, IPath*>(path->GetID(), path));
	} else {
		assert(tempPaths.find(path->GetID()) == tempPaths.end());
		assert(livePaths.find(path->GetID()) != livePaths.end());

		ReplacePath(path);
	}
}

void QTPFS::PathCache::DelPath(unsigned int pathID) {
	// if pathID is in tempPaths, livePaths and deadPaths
	// are guaranteed not to contain it (and vice versa)
	PathMap::iterator it;

	if ((it = tempPaths.find(pathID)) != tempPaths.end()) {
		delete it->second;
		tempPaths.erase(it);
	}
	if ((it = livePaths.find(pathID)) != livePaths.end()) {
		delete it->second;
		livePaths.erase(it);
	}

	deadPaths.erase(pathID);
}




bool QTPFS::PathCache::MarkDeadPaths(const SRectangle& r) {
	#if (IGNORE_DEAD_PATHS)
	return;
	#endif

	const unsigned int numDeadPaths = deadPaths.size();

	// NOTE:
	//     terrain might have been retesselated since
	//     path was originally cached (!) --> some or
	//     all of its waypoints might now be invalid
	//     --> need to adjust any live paths crossing
	//     the area of a terrain deformation
	//
	for (PathMap::const_iterator mit = livePaths.begin(); mit != livePaths.end(); ++mit) {
		const IPath* path = mit->second;

		// figure out if <path> has at least one edge crossing <r>
		for (unsigned int i = 0; i < (path->NumPoints() - 1); i++) {
			const float3& p0 = path->GetPoint(i    );
			const float3& p1 = path->GetPoint(i + 1);

			const bool pointInRect0 =
				((p0.x >= (r.x1 * SQUARE_SIZE) && p0.x < (r.x2 * SQUARE_SIZE)) &&
				 (p0.z >= (r.z1 * SQUARE_SIZE) && p0.z < (r.z2 * SQUARE_SIZE)));
			const bool pointInRect1 =
				((p1.x >= (r.x1 * SQUARE_SIZE) && p1.x < (r.x2 * SQUARE_SIZE)) &&
				 (p1.z >= (r.z1 * SQUARE_SIZE) && p1.z < (r.z2 * SQUARE_SIZE)));
			// TODO: also detect if edge from p0 to p1 crosses <r> at an angle
			const bool edgeCrossesRect =
				((p0.x < r.x1 && p1.x >= r.x2) && (p0.z >= r.z1 && p1.z < r.z2)) || // H
				((p0.z < r.z1 && p1.z >= r.z2) && (p0.x >= r.x1 && p1.x < r.x2)) || // V
				false;

			// remember the ID of each path affected by the deformation
			if ((int(pointInRect0) + int(pointInRect1) >= 1) || edgeCrossesRect) {
				deadPaths.insert(path->GetID());
				break;
			}
		}
	}

	return (deadPaths.size() > numDeadPaths);
}

void QTPFS::PathCache::KillDeadPaths(std::list<unsigned int>& replacedPathIDs) {
	// the (IDs of) paths that were succesfully
	// re-requested must no longer count as dead
	for (std::list<unsigned int>::const_iterator it = replacedPathIDs.begin(); it != replacedPathIDs.end(); ++it) {
		assert(tempPaths.find(*it) == tempPaths.end());
		deadPaths.erase(*it);
	}

	// get rid of paths that were marked as dead
	// (and were *not* replaced) up to this point
	for (PathIDSet::const_iterator it = deadPaths.begin(); it != deadPaths.end(); ++it) {
		assert(tempPaths.find(*it) == tempPaths.end());
		assert(livePaths.find(*it) != livePaths.end());

		PathMap::iterator pathIt = livePaths.find(*it);
		IPath* path = pathIt->second;

		delete path;
		livePaths.erase(pathIt);
	}

	deadPaths.clear();
	replacedPathIDs.clear();
}



void QTPFS::PathCache::ReplacePath(IPath* newPath) {
	PathMap::iterator oldPathIt = livePaths.find(newPath->GetID());
	IPath* oldPath = oldPathIt->second;

	assert(oldPathIt != livePaths.end());
	assert(oldPath->GetID() == newPath->GetID());
	assert(oldPath != newPath);

	delete oldPath;

	livePaths.erase(oldPathIt);
	livePaths.insert(std::make_pair<unsigned int, IPath*>(newPath->GetID(), newPath));
}

