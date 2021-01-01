/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_PATH_MANAGER_H
#define I_PATH_MANAGER_H

#include <vector>
#include <cinttypes>

#include "PFSTypes.h"
#include "System/type2.h"
#include "System/float3.h"

struct MoveDef;
class CSolidObject;

class IPathManager {
public:
	static IPathManager* GetInstance(int type);
	static void FreeInstance(IPathManager*);

	virtual ~IPathManager() {}

	virtual std::int32_t GetPathFinderType() const { return NOPFS_TYPE; }
	virtual std::uint32_t GetPathCheckSum() const { return 0; }

	virtual std::int64_t Finalize() { return 0; }

	/**
	 * returns if a path was changed after RequestPath returned its pathID
	 * this can happen eg. if a PathManager reacts to TerrainChange events
	 * (by re-requesting affected paths without changing their ID's)
	 */
	virtual bool PathUpdated(unsigned int pathID) { return false; }

	virtual void RemoveCacheFiles() {}
	virtual void Update() {}
	virtual void UpdatePath(const CSolidObject* owner, unsigned int pathID) {}

	/**
	 * When a path is no longer used, call this function to release it from
	 * memory.
	 * @param pathID
	 *     The path-id returned by RequestPath.
	 */
	virtual void DeletePath(unsigned int pathID) {}

	/**
	 * Returns the next waypoint of the path.
	 *
	 * @param pathID
	 *     The path-id returned by RequestPath.
	 * @param callerPos
	 *     The current position of the user of the path.
	 *     This extra information is needed to keep the path connected to its
	 *     user.
	 * @param minDistance
	 *     Could be used to set a minimum required distance between callerPos
	 *     and the returned waypoint.
	 * @param numRetries
	 *     Dont set this, used internally
	 * @param owner
	 *     The unit the path is used for, or NULL.
	 * @param synced
	 *     Whether this evaluation has to run synced or unsynced.
	 *     If false, this call may not change any state of the path manager
	 *     that could alter paths requested in the future.
	 *     example: if (synced == false) turn of heat-mapping
	 * @return
	 *     the next waypoint of the path, or (-1,-1,-1) in case no new
	 *     waypoint could be found.
	 */
	virtual float3 NextWayPoint(
		const CSolidObject* owner,
		unsigned int pathID,
		unsigned int numRetries,
		float3 callerPos,
		float radius,
		bool synced
	) {
		return -OnesVector;
	}


	/**
	 * Returns all waypoints of a path. Different segments of a path might
	 * have different resolutions, or each segment might be represented at
	 * multiple different resolution levels. In the former case, a subset
	 * of waypoints (those belonging to i-th resolution path SEGMENTS) are
	 * stored between points[starts[i]] and points[starts[i + 1]], while in
	 * the latter case ALL waypoints (of the i-th resolution PATH) are stored
	 * between points[starts[i]] and points[starts[i + 1]]
	 *
	 * @param pathID
	 *     The path-id returned by RequestPath.
	 * @param points
	 *     The list of waypoints.
	 * @param starts
	 *     The list of starting indices for the different resolutions
	 */
	virtual void GetPathWayPoints(
		unsigned int pathID,
		std::vector<float3>& points,
		std::vector<int>& starts
	) const {
	}


	/**
	 * Generate a path from startPos to the target defined by
	 * (goalPos, goalRadius).
	 * If no complete path from startPos to goalPos could be found,
	 * then a path getting as "close" as possible to target is generated.
	 *
	 * @param moveDef
	 *     Defines the move details of the unit to use the path.
	 * @param startPos
	 *     The starting location of the requested path.
	 * @param goalPos
	 *     The center of the path goal area.
	 * @param goalRadius
	 *     Use goalRadius to define a goal area within any square that could
	 *     be accepted as path goal.
	 *     If a singular goal position is wanted, use goalRadius = 0.
	 * @param caller
	 *     The unit or feature the path will be used for.
	 * @param synced
	 *     Whether this evaluation has to run synced or unsynced.
	 *     If false, this call may not change any state of the path manager
	 *     that could alter paths requested in the future.
	 *     example: if (synced == false) turn off heat-mapping
	 * @return
	 *     a path-id >= 1 on success, 0 on failure
	 *     Failure means, no path getting "closer" to goalPos then startPos
	 *     could be found
	 */
	virtual unsigned int RequestPath(
		CSolidObject* caller,
		const MoveDef* moveDef,
		float3 startPos,
		float3 goalPos,
		float goalRadius,
		bool synced
	) {
		return 0;
	}

	/**
	 * Whenever there are any changes in the terrain
	 * (examples: explosions, new buildings, etc.)
	 * this function will be called.
	 * @param x1
	 *     First corners X-axis value, defining the rectangular area
	 *     affected by the changes.
	 * @param z1
	 *     First corners Z-axis value, defining the rectangular area
	 *     affected by the changes.
	 * @param x2
	 *     Second corners X-axis value, defining the rectangular area
	 *     affected by the changes.
	 * @param z2
	 *     Second corners Z-axis value, defining the rectangular area
	 *     affected by the changes.
	 * @param type see @TerrainChangeTypes
	 */
	virtual void TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2, unsigned int type) {}

	virtual bool SetNodeExtraCosts(const float* costs, unsigned int sizex, unsigned int sizez, bool synced) { return false; }
	virtual bool SetNodeExtraCost(unsigned int x, unsigned int z, float cost, bool synced) { return false; }
	virtual float GetNodeExtraCost(unsigned int x, unsigned int z, bool synced) const { return 0.0f; }
	virtual const float* GetNodeExtraCosts(bool synced) const { return nullptr; }

	virtual int2 GetNumQueuedUpdates() const { return (int2(0, 0)); }
};

extern IPathManager* pathManager;

#endif
