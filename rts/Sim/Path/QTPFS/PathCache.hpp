/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHCACHE_HDR
#define QTPFS_PATHCACHE_HDR

#include <list>
#include <map>
#include <vector>

#include "PathDefines.hpp"
#include "Path.hpp"

struct SRectangle;

namespace QTPFS {
	struct PathCache {
		PathCache() {
			numCacheHits = 0;
			numCacheMisses = 0;
		}

		typedef std::map<unsigned int, IPath*> PathMap;
		typedef std::map<unsigned int, IPath*>::iterator PathMapIt;

		bool MarkDeadPaths(const SRectangle& r);
		void KillDeadPaths();

		IPath* GetLivePath(unsigned int pathID);
		IPath* GetTempPath(unsigned int pathID);
		IPath* GetDeadPath(unsigned int pathID);

		void AddTempPath(IPath* path);
		void AddLivePath(IPath* path);

		void DelPath(unsigned int pathID);

		const PathMap& GetTempPaths() const { return tempPaths; }
		const PathMap& GetLivePaths() const { return livePaths; }
		const PathMap& GetDeadPaths() const { return deadPaths; }

	private:
		PathMap tempPaths;
		PathMap livePaths;
		PathMap deadPaths;

		unsigned int numCacheHits;
		unsigned int numCacheMisses;
	};
};

#endif

