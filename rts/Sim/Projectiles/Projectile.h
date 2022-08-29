/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <vector>
#include <array>

#ifdef _MSC_VER
#pragma warning(disable:4291)
#endif

#include "ExpGenSpawnable.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Threading/SpringThreading.h"
#include "Rendering/GL/RenderBuffers.h"

class CUnit;
class CFeature;
class CMatrix44f;
struct AtlasedTexture;
class CProjectileDrawer;

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

	virtual void Draw() {}
	virtual void DrawOnMinimap();

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

	uint32_t GetOwnerID() const { return ownerID; }
	uint32_t GetTeamID() const { return teamID; }
	int GetAllyteamID() const { return allyteamID; }

	uint32_t GetProjectileType() const { return projectileType; }
	uint32_t GetCollisionFlags() const { return collisionFlags; }
	uint32_t GetRenderIndex() const { return renderIndex; }

	void SetCustomExpGenID(uint32_t id) { cegID = id; }
	void SetRenderIndex(uint32_t idx) { renderIndex = idx; }

	// UNSYNCED ONLY
	CMatrix44f GetTransformMatrix(bool offsetPos) const;

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

	bool createMe =  true;
	bool deleteMe = false;

	bool castShadow = false;
	bool drawSorted = true;

	float3 dir;                    // set via Init()
	float3 drawPos;

	float myrange = 0.0f;          // used by WeaponProjectile::TraveledRange
	float mygravity = 0.0f;

	float sortDist = 0.0f;         // distance used for z-sorting when rendering
	float sortDistOffset = 0.0f;   // an offset used for z-sorting

	int drawOrder = 0;

	inline static spring::mutex mut = {};
protected:
	std::array<bool, 5> validTextures = {false, false, false, false, false}; //overall state and 4 textures
	uint32_t ownerID = -1u;
	uint32_t teamID = -1u;
	int allyteamID = -1;
	uint32_t cegID = -1u;

	uint32_t projectileType = -1u;
	uint32_t collisionFlags = 0;
	uint32_t renderIndex = -1u;

	static inline TypedRenderBuffer<VA_TYPE_C> mmLnsRB{ 1 << 12, 0 };
	static inline TypedRenderBuffer<VA_TYPE_C> mmPtsRB{ 1 << 14, 0 };

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);
	static bool IsValidTexture(const AtlasedTexture* tex);
public:
	static void AddMiniMapVertices(VA_TYPE_C&& v1, VA_TYPE_C&& v2);

	static TypedRenderBuffer<VA_TYPE_C>& GetMiniMapLinesRB() { return mmLnsRB; }
	static TypedRenderBuffer<VA_TYPE_C>& GetMiniMapPointsRB() { return mmPtsRB; }

	//static TypedRenderBuffer<VA_TYPE_C >& GetAnimationRenderBuffer();
	std::vector<int> quads;
};

#endif /* PROJECTILE_H */

