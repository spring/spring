/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOLID_OBJECT_H
#define SOLID_OBJECT_H

#include "WorldObject.h"
#include "System/Vec2.h"
#include "System/Sync/SyncedFloat3.h"
#include "System/Sync/SyncedPrimitive.h"

class CUnit;
struct DamageArray;
struct CollisionVolume;
struct MoveData;

class CSolidObject: public CWorldObject {
public:
	CR_DECLARE(CSolidObject)

	enum PhysicalState {
		OnGround,
		Floating,
		Hovering,
		Flying,
	};

	CSolidObject();
	virtual ~CSolidObject();

	virtual bool AddBuildPower(float amount, CUnit* builder) { return false; }
	virtual void DoDamage(const DamageArray& damages, CUnit* attacker, const float3& impulse) {}
	virtual void Kill(const float3& impulse, bool crushKill) {}
	virtual int GetBlockingMapID() const { return -1; }

	void Move3D(const float3& v, bool relative) {
		const float3& dv = relative? v: (v - pos);

		pos += dv;
		midPos += dv;
	}

	void Move1D(const float v, int d, bool relative) {
		const float dv = relative? v: (v - pos[d]);

		pos[d] += dv;
		midPos[d] += dv;
	}
	// this should be called whenever the direction
	// vectors are changed (ie. after a rotation) in
	// eg. movetype code
	void UpdateMidPos();

	/**
	 * Adds this object to the GroundBlockingMap if and only if its collidable
	 * property is set (blocking), else does nothing (except call UnBlock()).
	 */
	void Block();
	/**
	 * Removes this object from the GroundBlockingMap if it is currently marked
	 * on it, does nothing otherwise.
	 */
	void UnBlock();

	int2 GetMapPos() const { return (GetMapPos(pos)); }
	int2 GetMapPos(const float3& position) const;

public:
	// Static properties
	float mass;                                 ///< the physical mass of this object
	bool blocking;                              ///< if this object can be collided with at all (NOTE: Some objects could be flat => not collidable.)
	bool immobile;                              ///< Immobile objects can not be moved. (Except perhaps along y-axis, to make them stay on ground.)
	bool blockHeightChanges;                    ///< if true, map height cannot change under this object (through explosions, etc.)
	bool crushKilled;                           ///< true if this object died by being crushed (currently applies only to features)

	int xsize;                                  ///< The x-size of this object, according to its footprint.
	int zsize;                                  ///< The z-size of this object, according to its footprint.

	SyncedSshort heading;                       ///< Contains the same information as frontdir, but in a short signed integer.
	PhysicalState physicalState;                ///< The current state of the object within the gameworld. I.e Flying or OnGround.

	bool isMoving;                              ///< = velocity.length() > 0.0
	bool isUnderWater;                          ///< true if this object is completely submerged (pos + height < 0)
	bool isMarkedOnBlockingMap;                 ///< true if this object is currently marked on the GroundBlockingMap

	float3 speed;                               ///< current velocity vector (length in elmos/frame)
	float3 residualImpulse;                     ///< Used to sum up external impulses.

	int team;                                   ///< team that "owns" this object
	int allyteam;                               ///< allyteam that this->team is part of

	MoveData* mobility;                         ///< holds information about the mobility and movedata of this object (if NULL, object is either static or aircraft)
	CollisionVolume* collisionVolume;

	SyncedFloat3 frontdir;                      ///< object-local z-axis (in WS)
	SyncedFloat3 rightdir;                      ///< object-local x-axis (in WS)
	SyncedFloat3 updir;                         ///< object-local y-axis (in WS)

	SyncedFloat3 relMidPos;                     ///< local-space vector from pos to midPos read from model, used to initialize midPos
	SyncedFloat3 midPos;                        ///< mid-position of model (pos is at the very bottom of the model) in WS, used as center of mass
	int2 mapPos;                                ///< current position on GroundBlockingObjectMap

	float3 drawPos;                             ///< = pos + speed * timeOffset (unsynced)
	float3 drawMidPos;                          ///< = drawPos + relMidPos (unsynced)

	const unsigned char* curYardMap;            ///< Current active yardmap of this object. 0 means no active yardmap => all blocked.
	int buildFacing;                            ///< Orientation of footprint, 4 different states

	static const float DEFAULT_MASS;
	static const float MINIMUM_MASS;
	static const float MAXIMUM_MASS;

	static int deletingRefID;
	static void SetDeletingRefID(int id) { deletingRefID = id; }
	// returns the object (command reference) id of the object currently being deleted,
	// for units this equals unit->id, and for features feature->id + uh->MaxUnits()
	static int GetDeletingRefID() { return deletingRefID; }
};

#endif // SOLID_OBJECT_H
