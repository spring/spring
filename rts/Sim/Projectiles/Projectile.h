/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4291)
#endif

#include "ExpGenSpawnable.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "System/float3.h"
#include "System/type2.h"

class CUnit;
class CFeature;
class CMatrix44f;

class CProjectile: public CExpGenSpawnable
{
	CR_DECLARE_DERIVED(CProjectile)

public:
	CProjectile();
	CProjectile(
		const float3& pos,
		const float3& spd,
		const CUnit* owner,
		bool isSynced,
		bool isWeapon,
		bool isPiece,
		bool isHitScan = false
	);
	virtual ~CProjectile();

	virtual void Collision() { Delete(); }
	virtual void Collision(CUnit* unit) { Collision(); }
	virtual void Collision(CFeature* feature) { Collision(); }
	//Not inheritable - used for removing a projectile from Lua.
	void Delete();
	virtual void Update();
	virtual void Init(const CUnit* owner, const float3& offset) override;

	virtual void Draw(GL::RenderDataBufferTC* va) const {}
	virtual void DrawOnMinimap(GL::RenderDataBufferC* va);

	virtual int GetProjectilesCount() const = 0;

	// override WorldObject::SetVelocityAndSpeed so
	// we can keep <dir> in sync with speed-vector
	// (unlike other world objects, projectiles must
	// always point directly along their vel-vector)
	//
	// should be called when speed-vector is changed
	// s.t. both speed.w and dir need to be updated
	void SetVelocityAndSpeed(const float3& vel) override {
		CWorldObject::SetVelocityAndSpeed(vel);

		if (speed.w <= 0.0f)
			return;

		dir = speed / speed.w;
	}

	void SetDirectionAndSpeed(const float3& _dir, float _spd) {
		dir = _dir;
		speed.w = _spd;

		// keep speed-vector in sync with <dir>
		CWorldObject::SetVelocity(dir * _spd);
	}

	CUnit* owner() const;

	unsigned int GetOwnerID() const { return ownerID; }
	unsigned int GetTeamID() const { return teamID; }
	int GetAllyteamID() const { return allyteamID; }

	unsigned int GetProjectileType() const { return projectileType; }
	unsigned int GetCollisionFlags() const { return collisionFlags; }
	unsigned int GetRenderIndex() const { return renderIndex; }

	void SetCustomExpGenID(unsigned int id) { cegID = id; }
	void SetRenderIndex(unsigned int idx) { renderIndex = idx; }

	// UNSYNCED ONLY
	CMatrix44f GetTransformMatrix(float posOffsetMult = 0.0f) const;

	float GetSortDist() const { return sortDist; }
	void SetSortDist(float d) { sortDist = d + sortDistOffset; }


public:
	bool synced = false;           // is this projectile part of the simulation?
	bool weapon = false;           // is this a weapon projectile? (true implies synced true)
	bool piece = false;            // is this a piece projectile? (true implies synced true)
	bool hitscan = false;          // is this a hit-scan projectile?

	bool luaDraw = false;          // whether the DrawProjectile callin is enabled for us
	bool luaMoveCtrl = false;
	bool checkCol = true;
	bool ignoreWater = false;
	bool deleteMe = false;
	bool callEvent = true;         // do we need to call the ProjectileCreated event

	bool castShadow = false;
	bool drawSorted = true;

	float3 dir;                    // set via Init()
	float3 drawPos;

	float myrange = 0.0f;          // used by WeaponProjectile::TraveledRange
	float mygravity = 0.0f;

	float sortDist = 0.0f;         // distance used for z-sorting when rendering
	float sortDistOffset = 0.0f;   // an offset used for z-sorting

protected:
	unsigned int ownerID = -1u;
	unsigned int teamID = -1u;
	int allyteamID = -1;
	unsigned int cegID = -1u;

	unsigned int projectileType = -1u;
	unsigned int collisionFlags = 0;
	unsigned int renderIndex = -1u;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

public:
	std::vector<int> quads;
};

#endif /* PROJECTILE_H */

