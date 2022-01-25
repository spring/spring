/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TKPFS_PATHMANAGER_H
#define TKPFS_PATHMANAGER_H

#include <cinttypes>

#include "Sim/Path/IPathManager.h"
#include "Sim/Path/Default/IPath.h"
#include "IPathFinder.h"
#include "Sim/Path/Default/PathFinderDef.h"
#include "System/UnorderedMap.hpp"

#include <mutex>

class CSolidObject;
class PathFlowMap;

class CPathFinderDef;
struct MoveDef;

namespace TKPFS {

class CPathEstimator;
class PathHeatMap;
class PathingState;
class CPathFinder;

class CPathManager: public IPathManager {
public:
	struct MultiPath {
		MultiPath(): moveDef(nullptr), caller(nullptr) {}
		MultiPath(const MoveDef* moveDef, const float3& startPos, const float3& goalPos, float goalRadius)
			: searchResult(IPath::Error)
			, start(startPos)
			, peDef(startPos, goalPos, goalRadius, 3.0f, 2000)
			, moveDef(moveDef)
			, caller(nullptr)
		{}

		//MultiPath(const MultiPath& mp) = delete;
		MultiPath(const MultiPath& mp) {
			*this = mp;
		}
		MultiPath(MultiPath&& mp) { *this = std::move(mp); }

		//MultiPath& operator = (const MultiPath& mp) = delete;
		MultiPath& operator = (const MultiPath& mp) {
			lowResPath = mp.lowResPath;
			medResPath = mp.medResPath;
			maxResPath = mp.maxResPath;

			searchResult = mp.searchResult;

			start = mp.start;
			finalGoal = mp.finalGoal;

			peDef   = mp.peDef;
			moveDef = mp.moveDef;
			caller  = mp.caller;

			return *this;
		}
		MultiPath& operator = (MultiPath&& mp) {
			lowResPath = std::move(mp.lowResPath);
			medResPath = std::move(mp.medResPath);
			maxResPath = std::move(mp.maxResPath);

			searchResult = mp.searchResult;

			start = mp.start;
			finalGoal = mp.finalGoal;

			peDef   = mp.peDef;
			moveDef = mp.moveDef;
			caller  = mp.caller;

			mp.moveDef = nullptr;
			mp.caller  = nullptr;
			return *this;
		}

		// paths
		IPath::Path lowResPath;
		IPath::Path medResPath;
		IPath::Path maxResPath;

		IPath::SearchResult searchResult;

		// request definition; start is const after ctor
		float3 start;
		float3 finalGoal;

		CCircularSearchConstraint peDef;

		const MoveDef* moveDef;

		// additional information
		CSolidObject* caller;
	};

public:
	CPathManager();
	~CPathManager();

	std::int32_t GetPathFinderType() const override { return HAPFS_TYPE; }
	std::uint32_t GetPathCheckSum() const override;

	std::int64_t Finalize() override;
	std::int64_t PostFinalizeRefresh() override;

	void RemoveCacheFiles() override;
	void Update() override;
	void UpdatePath(const CSolidObject*, unsigned int) override;
	void DeletePath(unsigned int pathID) override {
		if (pathID == 0)
			return;
		{
			const std::lock_guard<std::mutex> lock(pathMapUpdate);
			const auto pi = pathMap.find(pathID);

			if (pi == pathMap.end())
				return;

			pathMap.erase(pi);
		}
	}


	float3 NextWayPoint(
		const CSolidObject* owner,
		unsigned int pathID,
		unsigned int numRetries,
		float3 callerPos,
		float radius,
		bool synced
	) override;

	unsigned int RequestPath(
		CSolidObject* caller,
		const MoveDef* moveDef,
		float3 startPos,
		float3 goalPos,
		float goalRadius,
		bool synced
	) override;

	/**
	 * Returns waypoints of the max-resolution path segments.
	 * @param pathID
	 *     The path-id returned by RequestPath.
	 * @param points
	 *     The list of detail waypoints.
	 */
	void GetDetailedPath(unsigned pathID, std::vector<float3>& points) const;

	/**
	 * Returns waypoints of the max-resolution path segments as a square list.
	 *
	 * @param pathID
	 *     The path-id returned by RequestPath.
	 * @param points
	 *     The list of detail waypoints.
	 */
	void GetDetailedPathSquares(unsigned pathID, std::vector<int2>& points) const;

	void GetPathWayPoints(unsigned int pathID, std::vector<float3>& points, std::vector<int>& starts) const override;

	void TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2, unsigned int type) override;

	bool SetNodeExtraCost(unsigned int, unsigned int, float, bool) override;
	bool SetNodeExtraCosts(const float*, unsigned int, unsigned int, bool) override;
	float GetNodeExtraCost(unsigned int, unsigned int, bool) const override;
	const float* GetNodeExtraCosts(bool) const override;

	int2 GetNumQueuedUpdates() const override;

	const CPathFinder* GetMaxResPF() const;
	const CPathEstimator* GetMedResPE() const;
	const CPathEstimator* GetLowResPE() const;
	const PathingState* GetMedResPS() const;
	const PathingState* GetLowResPS() const;

	const PathFlowMap* GetPathFlowMap() const { return pathFlowMap; }
	const PathHeatMap* GetPathHeatMap() const { return pathHeatMap; }
	int GetPathFinderGroups() const { return pathFinderGroups; }

	const spring::unordered_map<unsigned int, MultiPath>& GetPathMap() const { return pathMap; }

private:

	void InitStatic();

	IPath::SearchResult ArrangePath(
		MultiPath* newPath,
		const MoveDef* moveDef,
		const float3& startPos,
		const float3& goalPos,
		CSolidObject* caller
	) const;

	MultiPath* GetMultiPath(int pathID) {return (const_cast<MultiPath*>(GetMultiPathConst(pathID))); }

	// Used by MT code - a copy must be taken
	MultiPath GetMultiPathMT(int pathID) const {
		const std::lock_guard<std::mutex> lock(pathMapUpdate);
		const auto pi = pathMap.find(pathID);
		if (pi == pathMap.end())
			return MultiPath();
		return pi->second;
	}

	// Used by MT code
	void UpdateMultiPathMT(int pathID, MultiPath& updatedPath) {
		const std::lock_guard<std::mutex> lock(pathMapUpdate);
		const auto pi = pathMap.find(pathID);
		if (pi != pathMap.end())
			pi->second = updatedPath;
	}

	const MultiPath* GetMultiPathConst(int pathID) const {
		const std::lock_guard<std::mutex> lock(pathMapUpdate);
		const auto pi = pathMap.find(pathID);
		if (pi == pathMap.end())
			return nullptr;
		return &(pi->second);
	}

	unsigned int Store(MultiPath& path) {
		unsigned int assignedId = 0;
		{
			const std::lock_guard<std::mutex> lock(pathMapUpdate);
			assignedId = ++nextPathID;
			pathMap.reserve(assignedId);
			pathMap[assignedId] = std::move(path);
		}
		return assignedId;
	}


	static void FinalizePath(MultiPath* path, const float3 startPos, const float3 goalPos, const bool cantGetCloser);

	void LowRes2MedRes(MultiPath& path, const float3& startPos, const CSolidObject* owner, bool synced) const;
	void MedRes2MaxRes(MultiPath& path, const float3& startPos, const CSolidObject* owner, bool synced) const;

	//bool IsFinalized() const { return (maxResPF != nullptr); }
	bool IsFinalized() const { return finalized; }

	bool SupportsMultiThreadedRequests() const { return true; }
	void SavePathCacheForPathId(int pathIdToSave) override;

private:
	mutable std::mutex pathMapUpdate;

	
	bool finalized = false;

	PathFlowMap* pathFlowMap;
	PathHeatMap* pathHeatMap;

	spring::unordered_map<unsigned int, MultiPath> pathMap;

	unsigned int nextPathID;

	int pathFinderGroups = 0;

	CPathEstimator* medResPEs;
	CPathEstimator* lowResPEs;
	CPathFinder* maxResPFs;

	std::vector<IPathFinder*> pathFinders;
};

}

#endif
