/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHCACHE_HDR
#define QTPFS_PATHCACHE_HDR

#include <list>
#include <map>
#include <vector>

#include "PathEnums.hpp"
#include "Path.hpp"

#ifdef GetTempPath
#undef GetTempPath
#undef GetTempPathA
#endif

struct SRectangle;

namespace QTPFS {
	struct PathCache {
		PathCache() {
			for (unsigned int n = 0; n <= PATH_TYPE_DEAD; n++) {
				numCacheHits[n] = 0;
				numCacheMisses[n] = 0;
			}
		}

		typedef std::map<unsigned int, IPath*> PathMap;
		typedef std::map<unsigned int, IPath*>::iterator PathMapIt;

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

		unsigned int numCacheHits[PATH_TYPE_DEAD + 1];
		unsigned int numCacheMisses[PATH_TYPE_DEAD + 1];
	};
};

#endif

