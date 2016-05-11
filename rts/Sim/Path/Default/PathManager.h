/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <map>
#include <boost/cstdint.hpp> /* Replace with <stdint.h> if appropriate */

#include "Sim/Path/IPathManager.h"
#include "IPath.h"
#include "PathFinderDef.h"

class CSolidObject;
class CPathFinder;
class CPathEstimator;
class PathFlowMap;
class PathHeatMap;
class CPathFinderDef;
struct MoveDef;

class CPathManager: public IPathManager {
public:
	CPathManager();
	~CPathManager();

	unsigned int GetPathFinderType() const override { return PFS_TYPE_DEFAULT; }
	boost::uint32_t GetPathCheckSum() const override;

	boost::int64_t Finalize() override;

	void Update() override;
	void UpdatePath(const CSolidObject*, unsigned int) override;
	void DeletePath(unsigned int pathID) override;


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

private:
	struct MultiPath {
		MultiPath(const float3& pos, const CPathFinderDef* def, const MoveDef* moveDef)
			: searchResult(IPath::Error)
			, start(pos)
			, peDef(def)
			, moveDef(moveDef)
			, finalGoal(ZeroVector)
			, caller(NULL)
		{}

		~MultiPath() { delete peDef; }

		// Paths
		IPath::Path lowResPath;
		IPath::Path medResPath;
		IPath::Path maxResPath;
		IPath::SearchResult searchResult;

		// Request definition
		const float3 start;
		const CPathFinderDef* peDef;
		const MoveDef* moveDef;

		// Additional information.
		float3 finalGoal;
		CSolidObject* caller;
	};

private:
	IPath::SearchResult ArrangePath(
		MultiPath* newPath,
		const MoveDef* moveDef,
		const float3& startPos,
		const float3& goalPos,
		CPathFinderDef* peDef,
		CSolidObject* caller
	) const;

	inline MultiPath* GetMultiPath(int pathID) const;
	unsigned int Store(MultiPath* path);
	static void FinalizePath(MultiPath* path, const float3 startPos, const float3 goalPos, const bool cantGetCloser);
	void LowRes2MedRes(MultiPath& path, const float3& startPos, const CSolidObject* owner, bool synced) const;
	void MedRes2MaxRes(MultiPath& path, const float3& startPos, const CSolidObject* owner, bool synced) const;

	bool IsFinalized() const { return (maxResPF != NULL); }

private:
	CPathFinder* maxResPF;
	CPathEstimator* medResPE;
	CPathEstimator* lowResPE;

	PathFlowMap* pathFlowMap;
	PathHeatMap* pathHeatMap;

	std::map<unsigned int, MultiPath*> pathMap;
	unsigned int nextPathID;
};

inline CPathManager::MultiPath* CPathManager::GetMultiPath(int pathID) const {
	const std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathID);
	if (pi == pathMap.end())
		return NULL;
	return pi->second;
}

#endif
