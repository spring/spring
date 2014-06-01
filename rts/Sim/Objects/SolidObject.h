/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOLID_OBJECT_H
#define SOLID_OBJECT_H

#include "WorldObject.h"
#include "System/bitops.h"
#include "System/Matrix44f.h"
#include "System/type2.h"
#include "System/Misc/BitwiseEnum.h"
#include "System/Sync/SyncedFloat3.h"
#include "System/Sync/SyncedPrimitive.h"

struct MoveDef;
struct CollisionVolume;
struct LocalModelPiece;
struct SolidObjectDef;
struct SolidObjectGroundDecal;

struct DamageArray;
class CUnit;

enum TerrainChangeTypes {
	TERRAINCHANGE_DAMAGE_RECALCULATION = 0, // update after regular explosion or terraform event
	TERRAINCHANGE_SQUARE_TYPEMAP_INDEX = 1, // update after typemap-index of a square changed (Lua)
	TERRAINCHANGE_TYPEMAP_SPEED_VALUES = 2, // update after speed-values of a terrain-type changed (Lua)
	TERRAINCHANGE_OBJECT_INSERTED      = 3,
	TERRAINCHANGE_OBJECT_INSERTED_YM   = 4,
	TERRAINCHANGE_OBJECT_DELETED       = 5,
};

enum YardmapStates {
	YARDMAP_OPEN        = 0,    // always free      (    walkable      buildable)
//  YARDMAP_WALKABLE    = 4,    // open for walk    (    walkable, not buildable)
	YARDMAP_YARD        = 1,    // walkable when yard is open
	YARDMAP_YARDINV     = 2,    // walkable when yard is closed
	YARDMAP_BLOCKED     = 0xFF & ~YARDMAP_YARDINV, // always block     (not walkable, not buildable)

	// helpers
	YARDMAP_YARDBLOCKED = YARDMAP_YARD,
	YARDMAP_YARDFREE    = ~YARDMAP_YARD,
	YARDMAP_GEO         = YARDMAP_BLOCKED,
};
typedef Bitwise::BitwiseEnum<YardmapStates> YardMapStatus;



class CSolidObject: public CWorldObject {
public:
	CR_DECLARE(CSolidObject)

	enum PhysicalState {
		// NOTE:
		//   {ONGROUND,*WATER} and INAIR are mutually exclusive
		//   {UNDERGROUND,UNDERWATER} are not (and are the only
		//   bits to take radius into account)
		PSTATE_BIT_ONGROUND    = (1 << 0),
		PSTATE_BIT_INWATER     = (1 << 1),
		PSTATE_BIT_UNDERWATER  = (1 << 2),
		PSTATE_BIT_UNDERGROUND = (1 << 3),
		PSTATE_BIT_INAIR       = (1 << 4),
		PSTATE_BIT_INVOID      = (1 << 5),

		// special bits for impulse-affected objects that do
		// not get set automatically by UpdatePhysicalState;
		// also used by aircraft to control block / unblock
		// behavior
		// NOTE: FLYING DOES NOT ALWAYS IMPLY INAIR!
		PSTATE_BIT_MOVING   = (1 <<  6),
		PSTATE_BIT_FLYING   = (1 <<  7),
		PSTATE_BIT_FALLING  = (1 <<  8),
		PSTATE_BIT_SKIDDING = (1 <<  9),
		PSTATE_BIT_CRASHING = (1 << 10),
		PSTATE_BIT_BLOCKING = (1 << 11),
	};
	enum CollidableState {
		CSTATE_BIT_SOLIDOBJECTS = (1 << 0), // can be set while (physicalState & PSTATE_BIT_BLOCKING) == 0!
		CSTATE_BIT_PROJECTILES  = (1 << 1),
		CSTATE_BIT_QUADMAPRAYS  = (1 << 2),
	};
	enum DamageType {
		DAMAGE_EXPLOSION_WEAPON  = 0, // weapon-projectile that triggered GameHelper::Explosion (weaponDefID >= 0)
		DAMAGE_EXPLOSION_DEBRIS  = 1, // piece-projectile that triggered GameHelper::Explosion (weaponDefID < 0)
		DAMAGE_COLLISION_GROUND  = 2, // ground collision
		DAMAGE_COLLISION_OBJECT  = 3, // object collision
		DAMAGE_EXTSOURCE_FIRE    = 4,
		DAMAGE_EXTSOURCE_WATER   = 5, // lava/acid/etc
		DAMAGE_EXTSOURCE_KILLED  = 6,
		DAMAGE_EXTSOURCE_CRUSHED = 7,
	};

	CSolidObject();
	virtual ~CSolidObject();

	virtual bool AddBuildPower(CUnit* builder, float amount) { return false; }
	virtual void DoDamage(const DamageArray& damages, const float3& impulse, CUnit* attacker, int weaponDefID, int projectileID) {}

	virtual void ApplyImpulse(const float3& impulse) { SetVelocity(speed + impulse); }

	virtual void Kill(CUnit* killer, const float3& impulse, bool crushed);
	virtual int GetBlockingMapID() const { return -1; }

	virtual void ForcedMove(const float3& newPos) {}
	virtual void ForcedSpin(const float3& newDir);

	virtual void UpdatePhysicalState(float eps);

	void Move(const float3& v, bool relative) {
		const float3& dv = relative? v: (v - pos);

		pos += dv;
		midPos += dv;
		aimPos += dv;
	}

	// this should be called whenever the direction
	// vectors are changed (ie. after a rotation) in
	// eg. movetype code
	void UpdateMidAndAimPos() {
		midPos = GetMidPos();
		aimPos = GetAimPos();
	}
	void SetMidAndAimPos(const float3& mp, const float3& ap, bool relative) {
		SetMidPos(mp, relative);
		SetAimPos(ap, relative);
	}


	void SetDirVectors(const CMatrix44f& matrix) {
		rightdir.x = -matrix[0]; updir.x = matrix[4]; frontdir.x = matrix[ 8];
		rightdir.y = -matrix[1]; updir.y = matrix[5]; frontdir.y = matrix[ 9];
		rightdir.z = -matrix[2]; updir.z = matrix[6]; frontdir.z = matrix[10];
	}
	// update object's <heading> from current frontdir
	// should always be called after a SetDirVectors()
	void SetHeadingFromDirection();
	// update object's local coor-sys from current <heading>
	// (unlike ForcedSpin which updates from given <updir>)
	// NOTE: movetypes call this directly
	void UpdateDirVectors(bool useGroundNormal);

	virtual CMatrix44f GetTransformMatrix(const bool synced = false, const bool error = false) const {
		// should never get called (should be pure virtual, but cause of CREG we cannot use it)
		assert(false);
		return CMatrix44f();
	}

	virtual const CollisionVolume* GetCollisionVolume(const LocalModelPiece* lmp) const { return collisionVolume; }


	/**
	 * adds this object to the GroundBlockingMap if and only
	 * if HasCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS), else
	 * does nothing
	 */
	void Block();
	/**
	 * Removes this object from the GroundBlockingMap if it
	 * is currently marked on it, does nothing otherwise.
	 */
	void UnBlock();

	// these transform a point or vector to object-space
	float3 GetObjectSpaceVec(const float3& v) const { return (      (frontdir * v.z) + (rightdir * v.x) + (updir * v.y)); }
	float3 GetObjectSpacePos(const float3& p) const { return (pos + (frontdir * p.z) + (rightdir * p.x) + (updir * p.y)); }
	float3 GetObjectSpacePosUnsynced(const float3& p) const { return (drawPos + GetObjectSpaceVec(p)); }

	int2 GetMapPos() const { return (GetMapPos(pos)); }
	int2 GetMapPos(const float3& position) const;

	float3 GetDragAccelerationVec(const float4& params) const;
	float3 GetWantedUpDir(bool useGroundNormal) const;

	YardMapStatus GetGroundBlockingMaskAtPos(float3 gpos) const;

	bool BlockMapPosChanged() const { return (groundBlockPos != pos); }

	bool IsOnGround   () const { return (HasPhysicalStateBit(PSTATE_BIT_ONGROUND   )); }
	bool IsInAir      () const { return (HasPhysicalStateBit(PSTATE_BIT_INAIR      )); }
	bool IsInWater    () const { return (HasPhysicalStateBit(PSTATE_BIT_INWATER    )); }
	bool IsUnderWater () const { return (HasPhysicalStateBit(PSTATE_BIT_UNDERWATER )); }
	bool IsUnderGround() const { return (HasPhysicalStateBit(PSTATE_BIT_UNDERGROUND)); }
	bool IsInVoid     () const { return (HasPhysicalStateBit(PSTATE_BIT_INVOID     )); }

	bool IsMoving  () const { return (HasPhysicalStateBit(PSTATE_BIT_MOVING  )); }
	bool IsFlying  () const { return (HasPhysicalStateBit(PSTATE_BIT_FLYING  )); }
	bool IsFalling () const { return (HasPhysicalStateBit(PSTATE_BIT_FALLING )); }
	bool IsSkidding() const { return (HasPhysicalStateBit(PSTATE_BIT_SKIDDING)); }
	bool IsCrashing() const { return (HasPhysicalStateBit(PSTATE_BIT_CRASHING)); }
	bool IsBlocking() const { return (HasPhysicalStateBit(PSTATE_BIT_BLOCKING)); }

	bool    HasPhysicalStateBit(unsigned int bit) const { return ((physicalState & bit) != 0); }
	void    SetPhysicalStateBit(unsigned int bit) { unsigned int ps = physicalState; ps |= ( bit); physicalState = static_cast<PhysicalState>(ps); }
	void  ClearPhysicalStateBit(unsigned int bit) { unsigned int ps = physicalState; ps &= (~bit); physicalState = static_cast<PhysicalState>(ps); }
	void   PushPhysicalStateBit(unsigned int bit) { UpdatePhysicalStateBit(1u << (32u - (bits_ffs(bit) - 1u)), HasPhysicalStateBit(bit)); }
	void    PopPhysicalStateBit(unsigned int bit) { UpdatePhysicalStateBit(bit, HasPhysicalStateBit(1u << (32u - (bits_ffs(bit) - 1u)))); }
	bool UpdatePhysicalStateBit(unsigned int bit, bool set) {
		if (set) {
			SetPhysicalStateBit(bit);
		} else {
			ClearPhysicalStateBit(bit);
		}
		return (HasPhysicalStateBit(bit));
	}

	bool    HasCollidableStateBit(unsigned int bit) const { return ((collidableState & bit) != 0); }
	void    SetCollidableStateBit(unsigned int bit) { unsigned int cs = collidableState; cs |= ( bit); collidableState = static_cast<CollidableState>(cs); }
	void  ClearCollidableStateBit(unsigned int bit) { unsigned int cs = collidableState; cs &= (~bit); collidableState = static_cast<CollidableState>(cs); } 
	void   PushCollidableStateBit(unsigned int bit) { UpdateCollidableStateBit(1u << (32u - (bits_ffs(bit) - 1u)), HasCollidableStateBit(bit)); }
	void    PopCollidableStateBit(unsigned int bit) { UpdateCollidableStateBit(bit, HasCollidableStateBit(1u << (32u - (bits_ffs(bit) - 1u)))); }
	bool UpdateCollidableStateBit(unsigned int bit, bool set) {
		if (set) {
			SetCollidableStateBit(bit);
		} else {
			ClearCollidableStateBit(bit);
		}
		return (HasCollidableStateBit(bit));
	}

	bool SetVoidState();
	bool ClearVoidState();
	void UpdateVoidState(bool set);

private:
	void SetMidPos(const float3& mp, bool relative) {
		if (relative) {
			relMidPos = mp; midPos = GetMidPos();
		} else {
			midPos = mp; relMidPos = midPos - pos;
		}
	}
	void SetAimPos(const float3& ap, bool relative) {
		if (relative) {
			relAimPos = ap; aimPos = GetAimPos();
		} else {
			aimPos = ap; relAimPos = aimPos - pos;
		}
	}

	float3 GetMidPos() const { return (GetObjectSpacePos(relMidPos)); }
	float3 GetAimPos() const { return (GetObjectSpacePos(relAimPos)); }

public:
	float health;
	float mass;                                 ///< the physical mass of this object (run-time constant)
	float crushResistance;                      ///< how much MoveDef::crushStrength is required to crush this object (run-time constant)

	bool crushable;                             ///< whether this object can potentially be crushed during a collision with another object
	bool immobile;                              ///< whether this object can be moved or not (except perhaps along y-axis, to make it stay on ground)
	bool blockEnemyPushing;                     ///< if false, object can be pushed during enemy collisions even when modrules forbid it
	bool blockHeightChanges;                    ///< if true, map height cannot change under this object (through explosions, etc.)

	bool luaDraw;                               ///< if true, LuaRules::Draw{Unit, Feature} will be called for this object (UNSYNCED)
	bool noSelect;                              ///< if true, unit/feature can not be selected/mouse-picked by a player (UNSYNCED)

	int xsize;                                  ///< The x-size of this object, according to its footprint. (Note: this is rotated depending on buildFacing!!!)
	int zsize;                                  ///< The z-size of this object, according to its footprint. (Note: this is rotated depending on buildFacing!!!)
	int2 footprint;                             ///< The unrotated x-/z-size of this object, according to its footprint.

	SyncedSshort heading;                       ///< Contains the same information as frontdir, but in a short signed integer.
	PhysicalState physicalState;                ///< bitmask indicating current state of this object within the game world
	CollidableState collidableState;            ///< bitmask indicating which types of objects this object can collide with

	int team;                                   ///< team that "owns" this object
	int allyteam;                               ///< allyteam that this->team is part of

	int tempNum;                                ///< used to check if object has already been processed (in QuadField queries, etc)

	const SolidObjectDef* objectDef;            ///< points to a UnitDef or to a FeatureDef instance

	MoveDef* moveDef;                           ///< mobility information about this object (if NULL, object is either static or aircraft)
	CollisionVolume* collisionVolume;
	SolidObjectGroundDecal* groundDecal;

	SyncedFloat3 frontdir;                      ///< object-local z-axis (in WS)
	SyncedFloat3 rightdir;                      ///< object-local x-axis (in WS)
	SyncedFloat3 updir;                         ///< object-local y-axis (in WS)

	SyncedFloat3 relMidPos;                     ///< local-space vector from pos to midPos (read from model, used to initialize midPos)
	SyncedFloat3 relAimPos;                     ///< local-space vector from pos to aimPos (read from model, used to initialize aimPos)
	SyncedFloat3 midPos;                        ///< mid-position of model in WS, used as center of mass (and many other things)
	SyncedFloat3 aimPos;                        ///< used as aiming position by weapons

	int2 mapPos;                                ///< current position on GroundBlockingObjectMap
	float3 groundBlockPos;

	float3 dragScales;

	float3 drawPos;                             ///< = pos + speed * timeOffset (unsynced)
	float3 drawMidPos;                          ///< = drawPos + relMidPos (unsynced)

	const YardMapStatus* blockMap;              ///< Current (unrotated!) blockmap/yardmap of this object. 0 means no active yardmap => all blocked.
	short int buildFacing;                      ///< Orientation of footprint, 4 different states

	static const float DEFAULT_MASS;
	static const float MINIMUM_MASS;
	static const float MAXIMUM_MASS;

	static int deletingRefID;
	static void SetDeletingRefID(int id) { deletingRefID = id; }
	// returns the object (command reference) id of the object currently being deleted,
	// for units this equals unit->id, and for features feature->id + unitHandler->MaxUnits()
	static int GetDeletingRefID() { return deletingRefID; }
};

#endif // SOLID_OBJECT_H
