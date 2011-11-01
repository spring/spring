/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "PathCache.hpp"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Rectangle.h"

QTPFS::IPath* QTPFS::PathCache::GetLivePath(unsigned int pathID) {
	static IPath livePath; // dummy

	PathMapIt it = livePaths.find(pathID);

	if (it != livePaths.end()) {
		numCacheHits += 1;
		return (it->second);
	}

	numCacheMisses += 1;
	return &livePath;
}

QTPFS::IPath* QTPFS::PathCache::GetTempPath(unsigned int pathID) {
	static IPath tempPath; // dummy

	PathMapIt it = tempPaths.find(pathID);

	if (it != tempPaths.end()) {
		return (it->second);
	}

	return &tempPath;
}

QTPFS::IPath* QTPFS::PathCache::GetDeadPath(unsigned int pathID) {
	static IPath deadPath; // dummy

	PathMapIt it = deadPaths.find(pathID);

	if (it != deadPaths.end()) {
		return (it->second);
	}

	return &deadPath;
}



void QTPFS::PathCache::AddTempPath(IPath* path) {
	assert(path->GetID() != 0);
	assert(path->NumPoints() == 2);
	assert(tempPaths.find(path->GetID()) == tempPaths.end());
	assert(livePaths.find(path->GetID()) == livePaths.end());

	tempPaths.insert(std::make_pair<unsigned int, IPath*>(path->GetID(), path));
}

void QTPFS::PathCache::AddLivePath(IPath* path) {
	assert(path->GetID() != 0);
	assert(path->NumPoints() >= 2);

	assert(tempPaths.find(path->GetID()) != tempPaths.end());
	assert(livePaths.find(path->GetID()) == livePaths.end());
	assert(deadPaths.find(path->GetID()) == deadPaths.end());

	// promote a path from temporary- to live-status (no deletion)
	tempPaths.erase(path->GetID());
	livePaths.insert(std::make_pair<unsigned int, IPath*>(path->GetID(), path));
}

void QTPFS::PathCache::DelPath(unsigned int pathID) {
	// if pathID is in xPaths, then yPaths and zPaths are guaranteed not
	// to contain it (*only* exception is that deadPaths briefly overlaps
	// tempPaths between QueueDeadPathSearches and KillDeadPaths)
	PathMapIt it;

	if ((it = tempPaths.find(pathID)) != tempPaths.end()) {
		assert(livePaths.find(pathID) == livePaths.end());
		assert(deadPaths.find(pathID) == deadPaths.end());
		delete (it->second);
		tempPaths.erase(it);
		return;
	}
	if ((it = livePaths.find(pathID)) != livePaths.end()) {
		assert(deadPaths.find(pathID) == deadPaths.end());
		delete (it->second);
		livePaths.erase(it);
		return;
	}
	if ((it = deadPaths.find(pathID)) != deadPaths.end()) {
		delete (it->second);
		deadPaths.erase(it);
	}
}




bool QTPFS::PathCache::MarkDeadPaths(const SRectangle& r) {
	#ifdef QTPFS_IGNORE_DEAD_PATHS
	return false;
	#endif

	if (livePaths.empty())
		return false;

	const unsigned int numDeadPaths = deadPaths.size();

	// "mark" any live path crossing the area of a terrain
	// deformation, for which some or all of its waypoints
	// might now be invalid and need to be recomputed
	//
	std::list<PathMapIt> livePathIts;

	for (PathMap::const_iterator it = livePaths.begin(); it != livePaths.end(); ++it) {
		IPath* path = it->second;

		const float3& pathMins = path->GetBoundingBoxMins();
		const float3& pathMaxs = path->GetBoundingBoxMaxs();

		// if rectangle does not overlap bounding-box, skip this path
		if ((r.x2 * SQUARE_SIZE) < pathMins.x) { continue; }
		if ((r.z2 * SQUARE_SIZE) < pathMins.z) { continue; }
		if ((r.x1 * SQUARE_SIZE) > pathMaxs.x) { continue; }
		if ((r.z1 * SQUARE_SIZE) > pathMaxs.z) { continue; }

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
			const bool havePointInRect = (pointInRect0 || pointInRect1);

			// remember the ID of each path affected by the deformation
			if (havePointInRect || edgeCrossesRect) {
				assert(tempPaths.find(path->GetID()) == tempPaths.end());
				deadPaths.insert(std::make_pair<unsigned int, IPath*>(path->GetID(), path));
				livePathIts.push_back(it);
				break;
			}
		}
	}

	for (std::list<PathMapIt>::const_iterator it = livePathIts.begin(); it != livePathIts.end(); ++it) {
		livePaths.erase(*it);
	}

	return (deadPaths.size() > numDeadPaths);
}

void QTPFS::PathCache::KillDeadPaths() {
	for (PathMap::const_iterator deadPathsIt = deadPaths.begin(); deadPathsIt != deadPaths.end(); ++deadPathsIt) {
		// NOTE: "!=" because re-requested dead paths go onto the temp-pile
		assert(tempPaths.find(deadPathsIt->first) != tempPaths.end());
		assert(livePaths.find(deadPathsIt->first) == livePaths.end());
		delete (deadPathsIt->second);
	}

	deadPaths.clear();
}

