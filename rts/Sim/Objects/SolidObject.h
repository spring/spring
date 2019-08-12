/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOLID_OBJECT_H
#define SOLID_OBJECT_H

#include "WorldObject.h"
#include "Lua/LuaRulesParams.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/bitops.h"
#include "System/Matrix44f.h"
#include "System/type2.h"
#include "System/Misc/BitwiseEnum.h"
#include "System/Sync/SyncedFloat3.h"
#include "System/Sync/SyncedPrimitive.h"

struct MoveDef;
struct LocalModelPiece;
struct SolidObjectDef;
struct SolidObjectGroundDecal;

class DamageArray;
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
//	YARDMAP_WALKABLE    = 4,    // open for walk    (    walkable, not buildable)
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
	CR_DECLARE_DERIVED(CSolidObject)


	virtual const SolidObjectDef* GetDef() const = 0;

	enum PhysicalState {
		// NOTE:
		//   {ONGROUND,*WATER} and INAIR are mutually exclusive
		//   {UNDERGROUND,UNDERWATER} are not (and are the only
		//   bits to take radius into account)
		// TODO:
		//   should isDead be on this list for spatial queries?
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

	virtual ~CSolidObject() {}

	void PostLoad();

	virtual bool AddBuildPower(CUnit* builder, float amount) { return false; }
	virtual void DoDamage(const DamageArray& damages, const float3& impulse, CUnit* attacker, int weaponDefID, int projectileID) {}

	virtual void ApplyImpulse(const float3& impulse) { SetVelocity(speed + impulse); }

	virtual void Kill(CUnit* killer, const float3& impulse, bool crushed);
	virtual int GetBlockingMapID() const { return -1; }

	virtual const YardMapStatus* GetBlockMap() const { return nullptr; }

	virtual void ForcedMove(const float3& newPos) {}
	virtual void ForcedSpin(const float3& newDir);

	virtual void UpdatePhysicalState(float eps);

	void SlowUpdateLocalModel() { localModel.UpdateBoundingVolume(); }
	void     UpdateLocalModel() { localModel.UpdatePieceMatrices(); }

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


	void SetDirVectorsEuler(const float3 angles);
	void SetDirVectors(const CMatrix44f& matrix) {
		rightdir.x = -matrix[0]; updir.x = matrix[4]; frontdir.x = matrix[ 8];
		rightdir.y = -matrix[1]; updir.y = matrix[5]; frontdir.y = matrix[ 9];
		rightdir.z = -matrix[2]; updir.z = matrix[6]; frontdir.z = matrix[10];
	}

	void AddHeading(short deltaHeading, bool useGroundNormal, bool useObjectNormal) { SetHeading(heading + deltaHeading, useGroundNormal, useObjectNormal); }
	void SetHeading(short worldHeading, bool useGroundNormal, bool useObjectNormal) {
		heading = worldHeading;

		UpdateDirVectors(useGroundNormal, useObjectNormal);
		UpdateMidAndAimPos();
	}

	// update object's <heading> from current frontdir
	// should always be called after a SetDirVectors()
	void SetHeadingFromDirection();
	// update object's <buildFacing> from current heading
	void SetFacingFromHeading();
	// update object's local coor-sys from current <heading>
	// (unlike ForcedSpin which updates from given <updir>)
	// NOTE: movetypes call this directly
	void UpdateDirVectors(bool useGroundNormal, bool useObjectNormal);

	CMatrix44f ComposeMatrix(const float3& p) const { return (CMatrix44f(p, -rightdir, updir, frontdir)); }
	virtual CMatrix44f GetTransformMatrix(bool synced = false, bool fullread = false) const = 0;

	const CollisionVolume* GetCollisionVolume(const LocalModelPiece* lmp) const {
		if (lmp == nullptr)
			return &collisionVolume;
		if (!collisionVolume.DefaultToPieceTree())
			return &collisionVolume;

		return (lmp->GetCollisionVolume());
	}

	const LuaObjectMaterialData* GetLuaMaterialData() const { return (localModel.GetLuaMaterialData()); }
	      LuaObjectMaterialData* GetLuaMaterialData()       { return (localModel.GetLuaMaterialData()); }

	const LocalModelPiece* GetLastHitPiece(int frame, int synced = true) const {
		if (frame == pieceHitFrames[synced])
			return hitModelPieces[synced];

		return nullptr;
	}

	void SetLastHitPiece(const LocalModelPiece* piece, int frame, int synced = true) {
		hitModelPieces[synced] = piece;
		pieceHitFrames[synced] = frame;
	}


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

	void SetMapPos(const int2 mp) {
		mapPos = mp;
		groundBlockPos = pos;
	}


	// these transform a point or vector to object-space
	float3 GetObjectSpaceVec(const float3& v) const { return (      (frontdir * v.z) + (rightdir * v.x) + (updir * v.y)); }
	float3 GetObjectSpacePos(const float3& p) const { return (pos + (frontdir * p.z) + (rightdir * p.x) + (updir * p.y)); }

	// note: requires drawPos to have been set first
	float3 GetObjectSpaceDrawPos(const float3& p) const { return (drawPos + GetObjectSpaceVec(p)); }

	// unsynced mid-{position,vector}s
	float3 GetMdlDrawMidPos() const { return (GetObjectSpaceDrawPos(localModel.GetRelMidPos())); }
	float3 GetObjDrawMidPos() const { return (GetObjectSpaceDrawPos(              relMidPos  )); }
	float3 GetMdlDrawRelMidPos() const { return (GetObjectSpaceVec(localModel.GetRelMidPos())); }
	float3 GetObjDrawRelMidPos() const { return (GetObjectSpaceVec(              relMidPos  )); }


	int2 GetMapPos() const { return (GetMapPos(pos)); }
	int2 GetMapPos(const float3& position) const;

	float2 GetFootPrint(float scale) const { return {xsize * scale, zsize * scale}; }

	float3 GetDragAccelerationVec(const float4& params) const;
	float3 GetWantedUpDir(bool useGroundNormal, bool useObjectNormal) const;

	float GetDrawRadius() const override { return (localModel.GetDrawRadius()); }
	float CalcFootPrintMinExteriorRadius(float scale = 1.0f) const;
	float CalcFootPrintMaxInteriorRadius(float scale = 1.0f) const;
	float CalcFootPrintAxisStretchFactor() const;

	YardMapStatus GetGroundBlockingMaskAtPos(float3 gpos) const;

	bool FootPrintOnGround() const;
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
	void   PushPhysicalStateBit(unsigned int bit) { UpdatePhysicalStateBit(1u << (32u - bits_ffs(bit)), HasPhysicalStateBit(bit)); }
	void    PopPhysicalStateBit(unsigned int bit) { UpdatePhysicalStateBit(bit, HasPhysicalStateBit(1u << (32u - bits_ffs(bit)))); }
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
	void   PushCollidableStateBit(unsigned int bit) { UpdateCollidableStateBit(1u << (32u - bits_ffs(bit)), HasCollidableStateBit(bit)); }
	void    PopCollidableStateBit(unsigned int bit) { UpdateCollidableStateBit(bit, HasCollidableStateBit(1u << (32u - bits_ffs(bit)))); }
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

	virtual void SetMass(float newMass);

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
	float health = 0.0f;
	float maxHealth = 1.0f;

	///< the physical mass of this object (can be changed by SetMass)
	float mass = DEFAULT_MASS;
	///< how much MoveDef::crushStrength is required to crush this object (run-time constant)
	float crushResistance = 0.0f;

	///< whether this object can potentially be crushed during a collision with another object
	bool crushable = false;
	///< whether this object can be moved or not (except perhaps along y-axis, to make it stay on ground)
	bool immobile = false;
	bool yardOpen = false;

	///< if false, object can be pushed during enemy collisions even when modrules forbid it
	bool blockEnemyPushing = true;
	///< if true, map height cannot change under this object (through explosions, etc.)
	bool blockHeightChanges = false;

	///< if true, object will not be drawn at all (neither as model nor as icon/fartex)
	bool noDraw = false;
	///< if true, LuaRules::Draw{Unit, Feature} will be called for this object (UNSYNCED)
	bool luaDraw = false;
	///< if true, unit/feature can not be selected/mouse-picked by a player (UNSYNCED)
	bool noSelect = false;

	///< x-size of this object, according to its footprint (note: rotated depending on buildFacing)
	int xsize = 1;
	///< z-size of this object, according to its footprint (note: rotated depending on buildFacing)
	int zsize = 1;

	///< unrotated x-/z-size of this object, according to its footprint
	int2 footprint = {1, 1};

	///< contains the same information as frontdir, but in a short signed integer
	SyncedSshort heading = 0;
	///< orientation of footprint, 4 different states
	SyncedSshort buildFacing = 0;


	///< objects start out non-blocking but fully collidable
	///< SolidObjectDef::collidable controls only the SO-bit
	///<
	///< bitmask indicating current state of this object within the game world
	PhysicalState physicalState = PhysicalState(PSTATE_BIT_ONGROUND);
	///< bitmask indicating which types of objects this object can collide with
	CollidableState collidableState = CollidableState(CSTATE_BIT_SOLIDOBJECTS | CSTATE_BIT_PROJECTILES | CSTATE_BIT_QUADMAPRAYS);


	///< team that "owns" this object
	int team = 0;
	///< allyteam that this->team is part of
	int allyteam = 0;

	///< [i] := frame on which hitModelPieces[i] was last hit
	int pieceHitFrames[2] = {-1, -1};

	///< mobility information about this object (if NULL, object is either static or aircraft)
	MoveDef* moveDef = nullptr;

	LocalModel localModel;
	CollisionVolume collisionVolume;
	CollisionVolume selectionVolume;

	///< pieces that were last hit by a {[0] := unsynced, [1] := synced} projectile
	const LocalModelPiece* hitModelPieces[2];

	SolidObjectGroundDecal* groundDecal = nullptr;

	///< object-local {z,x,y}-axes (in WS)
	SyncedFloat3 frontdir =  FwdVector;
	SyncedFloat3 rightdir = -RgtVector;
	SyncedFloat3    updir =   UpVector;

	///< local-space vector from pos to midPos (read from model, used to initialize midPos)
	SyncedFloat3 relMidPos;
	///< local-space vector from pos to aimPos (read from model, used to initialize aimPos)
	SyncedFloat3 relAimPos;
	///< mid-position of model in WS, used as center of mass (etc)
	SyncedFloat3 midPos;
	///< aim-position of model in WS, used by weapons
	SyncedFloat3 aimPos;

	///< current position on GroundBlockingObjectMap
	int2 mapPos;
	float3 groundBlockPos;

	float3 dragScales = OnesVector;

	///< pos + speed * timeOffset (unsynced)
	float3 drawPos;
	///< drawPos + relMidPos (unsynced)
	float3 drawMidPos;

	/**
	 * @brief mod controlled parameters
	 * This is a set of parameters that is initialized
	 * in CreateUnitRulesParams() and may change during the game.
	 * Each parameter is uniquely identified only by its id
	 * (which is the index in the vector).
	 * Parameters may or may not have a name.
	 */
	LuaRulesParams::Params  modParams;

public:
	static constexpr float DEFAULT_MASS = 1e5f;
	static constexpr float MINIMUM_MASS = 1e0f; // 1.0f
	static constexpr float MAXIMUM_MASS = 1e6f;

	static int deletingRefID;

	static void SetDeletingRefID(int id) { deletingRefID = id; }
	// returns the object (command reference) id of the object currently being deleted,
	// for units this equals unit->id, and for features feature->id + unitHandler.MaxUnits()
	static int GetDeletingRefID() { return deletingRefID; }
};

#endif // SOLID_OBJECT_H
