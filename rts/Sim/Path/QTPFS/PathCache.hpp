/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHCACHE_HDR
#define QTPFS_PATHCACHE_HDR

#include <vector>

#include "PathEnums.hpp"
#include "Path.hpp"
#include "System/UnorderedMap.hpp"

#ifdef GetTempPath
#undef GetTempPath
#undef GetTempPathA
#endif

struct SRectangle;

namespace QTPFS {
	struct PathCache {
		PathCache() {
			numCacheHits.resize(PATH_TYPE_DEAD + 1, 0);
			numCacheMisses.resize(PATH_TYPE_DEAD + 1, 0);
		}

		typedef spring::unordered_map<unsigned int, IPath*> PathMap;
		typedef spring::unordered_map<unsigned int, IPath*>::iterator PathMapIt;

		bool MarkDeadPaths(const SRectangle& r);
		void KillDeadPaths();

		const IPath* GetTempPath(unsigned int pathID) const { return (GetConstPath(pathID, PATH_TYPE_TEMP)); }
		const IPath* GetLivePath(unsigned int pathID) const { return (GetConstPath(pathID, PATH_TYPE_LIVE)); }
		const IPath* GetDeadPath(unsigned int pathID) const { return (GetConstPath(pathID, PATH_TYPE_DEAD)); }
		      IPath* GetTempPath(unsigned int pathID)       { return (GetPath(pathID, PATH_TYPE_TEMP)); }
		      IPath* GetLivePath(unsigned int pathID)       { return (GetPath(pathID, PATH_TYPE_LIVE)); }
		      IPath* GetDeadPath(unsigned int pathID)       { return (GetPath(pathID, PATH_TYPE_DEAD)); }

		void AddTempPath(IPath* path);
		void AddLivePath(IPath* path);

		void DelPath(unsigned int pathID);

		const PathMap& GetTempPaths() const { return tempPaths; }
		const PathMap& GetLivePaths() const { return livePaths; }
		const PathMap& GetDeadPaths() const { return deadPaths; }

	private:
		const IPath* GetConstPath(unsigned int pathID, unsigned int pathType) const;
		      IPath* GetPath(unsigned int pathID, unsigned int pathType);

		PathMap tempPaths;
		PathMap livePaths;
		PathMap deadPaths;

		std::vector<unsigned int> numCacheHits;
		std::vector<unsigned int> numCacheMisses;
	};
}

#endif

