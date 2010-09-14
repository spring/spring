/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <map>
#include <boost/cstdint.hpp> /* Replace with <stdint.h> if appropriate */

#include "Sim/Path/IPathManager.h"
#include "IPath.h"

class CSolidObject;
class CPathFinder;
class CPathEstimator;
class CPathFinderDef;
struct MoveData;
class CMoveMath;

class CPathManager: public IPathManager {
public:
	CPathManager();
	~CPathManager();

	boost::uint32_t GetPathCheckSum() const;
	unsigned int GetPathResolution() const;

	void Update();
	void UpdatePath(const CSolidObject*, unsigned int);

	void DeletePath(unsigned int pathId);


	float3 NextWaypoint(
		unsigned int pathId, float3 callerPos, float minDistance = 0.0f,
		int numRetries = 0, int ownerId = 0, bool synced = true
	) const;

	unsigned int RequestPath(
		const MoveData* moveData,
		const float3& startPos,
		const float3& goalPos,
		float goalRadius = 8.0f,
		CSolidObject* caller = 0,
		bool synced = true
	);

	/**
	 * Returns current detail path waypoints. For the full path, @see GetEstimatedPath.
	 * @param pathId
	 *     The path-id returned by RequestPath.
	 * @param points
	 *     The list of detail waypoints.
	 */
	void GetDetailedPath(unsigned pathId, std::vector<float3>& points) const;

	/**
	 * Returns current detail path waypoints as a square list. For the full path, @see GetEstimatedPath.
	 *
	 * @param pathId
	 *     The path-id returned by RequestPath.
	 * @param points
	 *     The list of detail waypoints.
	 */
	void GetDetailedPathSquares(unsigned pathId, std::vector<int2>& points) const;

	void GetEstimatedPath(unsigned int pathId, std::vector<float3>& points, std::vector<int>& starts) const;

	void TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);


	/** Enable/disable heat mapping */
	void SetHeatMappingEnabled(bool enabled);
	bool GetHeatMappingEnabled();

	void SetHeatOnSquare(int x, int y, int value, int ownerId);
	const int GetHeatOnSquare(int x, int y);

private:
	unsigned int RequestPath(
		const MoveData* moveData,
		const float3& startPos,
		const float3& goalPos,
		CPathFinderDef* peDef,
		CSolidObject* caller,
		bool synced = true
	);

	struct MultiPath {
		MultiPath(const float3 start, const CPathFinderDef* peDef, const MoveData* moveData);
		~MultiPath();

		// Paths
		IPath::Path lowResPath;
		IPath::Path medResPath;
		IPath::Path maxResPath;
		IPath::SearchResult searchResult;

		// Request definition
		const float3 start;
		const CPathFinderDef* peDef;
		const MoveData* moveData;

		// Additional information.
		float3 finalGoal;
		CSolidObject* caller;
	};

	unsigned int Store(MultiPath* path);
	void LowRes2MedRes(MultiPath& path, const float3& startPos, int ownerId, bool synced) const;
	void MedRes2MaxRes(MultiPath& path, const float3& startPos, int ownerId) const;

	CPathFinder* maxResPF;
	CPathEstimator* medResPE;
	CPathEstimator* lowResPE;

	std::map<unsigned int, MultiPath*> pathMap;
	unsigned int nextPathId;
};

#endif
