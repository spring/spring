/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOLID_OBJECT_H
#define SOLID_OBJECT_H

#include "WorldObject.h"
#include "System/Vec2.h"
#include "System/Misc/BitwiseEnum.h"
#include "System/Sync/SyncedFloat3.h"
#include "System/Sync/SyncedPrimitive.h"

class CUnit;
struct DamageArray;
struct CollisionVolume;
struct MoveDef;


enum YardmapStati {
	YARDMAP_OPEN        = 0,    // always free      (    walkable      buildable)
	//YARDMAP_WALKABLE    = 4,    // open for walk    (    walkable, not buildable)
	YARDMAP_YARD        = 1,    // walkable when yard is open
	YARDMAP_YARDINV     = 2,    // walkable when yard is closed
	YARDMAP_BLOCKED     = 0xFF & ~YARDMAP_YARDINV, // always block     (not walkable, not buildable)

	// helpers
	YARDMAP_YARDBLOCKED = YARDMAP_YARD,
	YARDMAP_YARDFREE    = ~YARDMAP_YARD,
	YARDMAP_GEO         = YARDMAP_BLOCKED,
};
typedef BitwiseEnum<YardmapStati> YardmapStatus;



class CSolidObject: public CWorldObject {
public:
	CR_DECLARE(CSolidObject)

	enum PhysicalState {
		OnGround,
		Floating,
		Hovering,
		Flying,
	};
	enum DamageType {
		DAMAGE_EXPLOSION_WEAPON = 0, // weapon-projectile that triggered GameHelper::Explosion (weaponDefID >= 0)
		DAMAGE_EXPLOSION_DEBRIS = 1, // piece-projectile that triggered GameHelper::Explosion (weaponDefID < 0)
		DAMAGE_COLLISION_GROUND = 2, // ground collision
		DAMAGE_COLLISION_OBJECT = 3, // object collision
		DAMAGE_EXTSOURCE_FIRE   = 4,
		DAMAGE_EXTSOURCE_WATER  = 5, // lava/acid/etc
		DAMAGE_EXTSOURCE_KILLED = 6,
	};

	CSolidObject();
	virtual ~CSolidObject();

	virtual bool AddBuildPower(float amount, CUnit* builder) { return false; }
	virtual void DoDamage(const DamageArray& damages, const float3& impulse, CUnit* attacker, int weaponDefID) {}
	virtual void Kill(const float3& impulse, bool crushKill);
	virtual int GetBlockingMapID() const { return -1; }

	void Move3D(const float3& v, bool relative) {
		const float3& dv = relative? v: (v - pos);

		pos += dv;
		midPos += dv;
		aimPos += dv;
	}

	void Move1D(const float v, int d, bool relative) {
		const float dv = relative? v: (v - pos[d]);

		pos[d] += dv;
		midPos[d] += dv;
		aimPos[d] += dv;
	}

	// this should be called whenever the direction
	// vectors are changed (ie. after a rotation) in
	// eg. movetype code
	void UpdateMidAndAimPos() {
		UpdateMidPos();
		UpdateAimPos();
	}
	void SetMidAndAimPos(const float3& mp, const float3& ap) {
		SetMidPos(mp);
		SetAimPos(ap);
	}

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

	YardmapStatus GetGroundBlockingAtPos(float3 gpos) const;

private:
	void SetMidPos(const float3& p) { midPos = p; relMidPos = pos - midPos; }
	void SetAimPos(const float3& p) { aimPos = p; relAimPos = pos - aimPos; }

	void UpdateMidPos() {
		const float3 dz = (frontdir * relMidPos.z);
		const float3 dy = (updir    * relMidPos.y);
		const float3 dx = (rightdir * relMidPos.x);

		midPos = pos + dz + dy + dx;
	}
	void UpdateAimPos() {
		const float3 dz = (frontdir * relAimPos.z);
		const float3 dy = (updir    * relAimPos.y);
		const float3 dx = (rightdir * relAimPos.x);

		aimPos = pos + dz + dy + dx;
	}

public:
	float health;
	float mass;                                 ///< the physical mass of this object (run-time constant)
	float crushResistance;                      ///< how much MoveDef::crushStrength is required to crush this object (run-time constant)

	bool blocking;                              ///< if this object can be collided with at all (NOTE: Some objects could be flat => not collidable.)
	bool crushable;                             ///< whether this object can potentially be crushed during a collision with another object
	bool immobile;                              ///< whether this object can be moved or not (except perhaps along y-axis, to make it stay on ground)
	bool crushKilled;                           ///< true if this object died by being crushed during a collision
	bool blockEnemyPushing;                     ///< if false, object can be pushed during enemy collisions even when modrules forbid it
	bool blockHeightChanges;                    ///< if true, map height cannot change under this object (through explosions, etc.)

	bool luaDraw;                               ///< if true, LuaRules::Draw{Unit, Feature} will be called for this object (UNSYNCED)
	bool noSelect;                              ///< if true, unit/feature can not be selected/mouse-picked by a player (UNSYNCED)

	int xsize;                                  ///< The x-size of this object, according to its footprint. (Note: this is rotated depending on buildFacing!!!)
	int zsize;                                  ///< The z-size of this object, according to its footprint. (Note: this is rotated depending on buildFacing!!!)
	int2 footprint;                             ///< The unrotated x-/z-size of this object, according to its footprint.

	SyncedSshort heading;                       ///< Contains the same information as frontdir, but in a short signed integer.
	PhysicalState physicalState;                ///< The current state of the object within the gameworld. I.e Flying or OnGround.

	bool isMoving;                              ///< = velocity.length() > 0.0
	bool isUnderWater;                          ///< true if this object is completely submerged (pos + height < 0)
	bool isMarkedOnBlockingMap;                 ///< true if this object is currently marked on the GroundBlockingMap

	float3 speed;                               ///< current velocity vector (length in elmos/frame)
	float3 residualImpulse;                     ///< Used to sum up external impulses.

	int team;                                   ///< team that "owns" this object
	int allyteam;                               ///< allyteam that this->team is part of

	MoveDef* moveDef;                           ///< holds information about the mobility of this object (if NULL, object is either static or aircraft)
	CollisionVolume* collisionVolume;

	SyncedFloat3 frontdir;                      ///< object-local z-axis (in WS)
	SyncedFloat3 rightdir;                      ///< object-local x-axis (in WS)
	SyncedFloat3 updir;                         ///< object-local y-axis (in WS)

	SyncedFloat3 relMidPos;                     ///< local-space vector from pos to midPos (read from model, used to initialize midPos)
	SyncedFloat3 relAimPos;                     ///< local-space vector from pos to aimPos (read from model, used to initialize aimPos)
	SyncedFloat3 midPos;                        ///< mid-position of model in WS, used as center of mass (and many other things)
	SyncedFloat3 aimPos;                        ///< used as aiming position by weapons
	int2 mapPos;                                ///< current position on GroundBlockingObjectMap

	float3 drawPos;                             ///< = pos + speed * timeOffset (unsynced)
	float3 drawMidPos;                          ///< = drawPos + relMidPos (unsynced)

	const YardmapStatus* blockMap;              ///< Current (unrotated!) blockmap/yardmap of this object. 0 means no active yardmap => all blocked.
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
