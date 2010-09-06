/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_PATH_MANAGER_H
#define I_PATH_MANAGER_H

#include <boost/cstdint.hpp> /* Replace with <stdint.h> if appropriate */
#include "float3.h"

struct MoveData;
class CSolidObject;

class IPathManager {
public:
	static IPathManager* GetInstance();

	virtual ~IPathManager() {}

	virtual boost::uint32_t GetPathCheckSum() const { return 0; }
	virtual unsigned int GetPathResolution() const { return 0; }

	virtual void Update() {}
	virtual void UpdatePath(const CSolidObject* owner, unsigned int pathId) {}
	virtual void DeletePath(unsigned int pathId) {}

	virtual float3 NextWaypoint(
		unsigned int pathId, float3 callerPos, float minDistance = 0.0f,
		int numRetries = 0, int ownerId = 0, bool synced = true
	) const { return ZeroVector; }

	//! NOTE: should not be in the interface
	virtual void GetEstimatedPath(
		unsigned int pathId,
		std::vector<float3>& points,
		std::vector<int>& starts
	) const {}

	virtual unsigned int RequestPath(
		const MoveData* moveData,
		float3 startPos, float3 goalPos, float goalRadius = 8.0f,
		CSolidObject* caller = 0,
		bool synced = true
	) { return 0; }

	virtual void TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {}
};

extern IPathManager* pathManager;

#endif
