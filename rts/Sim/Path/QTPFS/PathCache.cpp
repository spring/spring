/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "PathCache.hpp"
#include "PathDefines.hpp"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Rectangle.h"

static void GetRectangleCollisionVolume(const SRectangle& r, CollisionVolume& v, float3& rm) {
	float3 vScales;

	// rectangle dimensions (WS)
	vScales.x = ((r.x2 - r.x1) * SQUARE_SIZE);
	vScales.z = ((r.z2 - r.z1) * SQUARE_SIZE);
	vScales.y = 1.0f;

	// rectangle mid-point (WS)
	rm.x = ((r.x1 + r.x2) * SQUARE_SIZE) >> 1;
	rm.z = ((r.z1 + r.z2) * SQUARE_SIZE) >> 1;
	rm.y = 0.0f;

	#define CV CollisionVolume
	v.InitShape(vScales, ZeroVector, CV::COLVOL_TYPE_BOX, CV::COLVOL_HITTEST_CONT, CV::COLVOL_AXIS_Y);
	#undef CV
}



const QTPFS::IPath* QTPFS::PathCache::GetConstPath(unsigned int pathID, unsigned int pathType) const {
	static IPath path; // dummy
	const PathMap* map;

	switch (pathType) {
		case PATH_TYPE_TEMP: { map = &tempPaths; } break;
		case PATH_TYPE_LIVE: { map = &livePaths; } break;
		case PATH_TYPE_DEAD: { map = &deadPaths; } break;
		default:             { map =       NULL; } break;
	}

	if (map == NULL)
		return &path;

	const PathMap::const_iterator it = map->find(pathID);

	if (it != map->end()) {
		return it->second;
	}

	return &path;
}

QTPFS::IPath* QTPFS::PathCache::GetPath(unsigned int pathID, unsigned int pathType) {
	IPath* path = const_cast<IPath*>(GetConstPath(pathID, pathType));

	if (path->GetID() != 0) {
		numCacheHits[pathType] += 1;
	} else {
		numCacheMisses[pathType] += 1;
	}

	return path;
}



void QTPFS::PathCache::AddTempPath(IPath* path) {
	assert(path->GetID() != 0);
	assert(path->NumPoints() == 2);
	assert(tempPaths.find(path->GetID()) == tempPaths.end());
	assert(livePaths.find(path->GetID()) == livePaths.end());

	tempPaths.insert(std::pair<unsigned int, IPath*>(path->GetID(), path));
}

void QTPFS::PathCache::AddLivePath(IPath* path) {
	assert(path->GetID() != 0);
	assert(path->NumPoints() >= 2);

	assert(tempPaths.find(path->GetID()) != tempPaths.end());
	assert(livePaths.find(path->GetID()) == livePaths.end());
	assert(deadPaths.find(path->GetID()) == deadPaths.end());

	// promote a path from temporary- to live-status (no deletion)
	tempPaths.erase(path->GetID());
	livePaths.insert(std::pair<unsigned int, IPath*>(path->GetID(), path));
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

	// NOTE: not static, we run in multiple threads
	CollisionVolume rv;
	float3 rm;

	GetRectangleCollisionVolume(r, rv, rm);

	// "mark" any live path crossing the area of a terrain
	// deformation, for which some or all of its waypoints
	// might now be invalid and need to be recomputed
	std::vector<PathMapIt> livePathIts;
	livePathIts.reserve(livePaths.size());

	for (PathMapIt it = livePaths.begin(); it != livePaths.end(); ++it) {
		IPath* path = it->second;

		const float3& pathMins = path->GetBoundingBoxMins();
		const float3& pathMaxs = path->GetBoundingBoxMaxs();

		// if rectangle does not overlap bounding-box, skip this path
		if ((r.x2 * SQUARE_SIZE) < pathMins.x) { continue; }
		if ((r.z2 * SQUARE_SIZE) < pathMins.z) { continue; }
		if ((r.x1 * SQUARE_SIZE) > pathMaxs.x) { continue; }
		if ((r.z1 * SQUARE_SIZE) > pathMaxs.z) { continue; }

		// figure out if <path> has at least one edge crossing <r>
		// we only care about the segments we have not yet visited
		const unsigned int minIdx = std::max(path->GetNextPointIndex(), 2U) - 2;
		const unsigned int maxIdx = std::max(path->NumPoints(), 1u) - 1;

		for (unsigned int i = minIdx; i < maxIdx; i++) {
			const float3& p0 = path->GetPoint(i    );
			const float3& p1 = path->GetPoint(i + 1);

			const bool p0InRect =
				((p0.x >= (r.x1 * SQUARE_SIZE) && p0.x < (r.x2 * SQUARE_SIZE)) &&
				 (p0.z >= (r.z1 * SQUARE_SIZE) && p0.z < (r.z2 * SQUARE_SIZE)));
			const bool p1InRect =
				((p1.x >= (r.x1 * SQUARE_SIZE) && p1.x < (r.x2 * SQUARE_SIZE)) &&
				 (p1.z >= (r.z1 * SQUARE_SIZE) && p1.z < (r.z2 * SQUARE_SIZE)));
			const bool havePointInRect = (p0InRect || p1InRect);

			// NOTE:
			//     box-volume tests in its own space, but points are
			//     in world-space so we must inv-transform them first
			//     (p0 --> p0 - rm, p1 --> p1 - rm)
			const bool
				xRangeInRect = (p0.x >= (r.x1 * SQUARE_SIZE) && p1.x <  (r.x2 * SQUARE_SIZE)),
				xRangeExRect = (p0.x <  (r.x1 * SQUARE_SIZE) && p1.x >= (r.x2 * SQUARE_SIZE)),
				zRangeInRect = (p0.z >= (r.z1 * SQUARE_SIZE) && p1.z <  (r.z2 * SQUARE_SIZE)),
				zRangeExRect = (p0.z <  (r.z1 * SQUARE_SIZE) && p1.z >= (r.z2 * SQUARE_SIZE));
			const bool edgeCrossesRect =
				(xRangeExRect && zRangeInRect) ||
				(xRangeInRect && zRangeExRect) ||
				CCollisionHandler::IntersectBox(&rv, p0 - rm, p1 - rm, NULL);

			// remember the ID of each path affected by the deformation
			if (havePointInRect || edgeCrossesRect) {
				assert(tempPaths.find(path->GetID()) == tempPaths.end());
				deadPaths.insert(std::pair<unsigned int, IPath*>(path->GetID(), path));
				livePathIts.push_back(it);
				break;
			}
		}
	}

	for (auto it = livePathIts.begin(); it != livePathIts.end(); ++it) {
		livePaths.erase(*it);
	}

	return true;
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

