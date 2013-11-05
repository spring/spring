/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "lib/gml/gml_base.h"

#ifdef _MSC_VER
#pragma warning(disable:4291)
#endif

#include "ExplosionGenerator.h"
#include "System/float3.h"
#include "System/type2.h"

class CUnit;
class CFeature;
class CVertexArray;
class CMatrix44f;


class CProjectile: public CExpGenSpawnable
{
	CR_DECLARE(CProjectile);

	/// used only by creg
	CProjectile();

public:
	CProjectile(const float3& pos, const float3& speed, CUnit* owner, bool isSynced, bool isWeapon, bool isPiece);
	virtual ~CProjectile();
	virtual void Detach();

	virtual void Collision();
	virtual void Collision(CUnit* unit);
	virtual void Collision(CFeature* feature);
	virtual void Update();
	virtual void Init(const float3& pos, CUnit* owner);

	virtual void Draw() {}
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	virtual void DrawCallback() {}

	// override WorldObject::SetVelocityAndSpeed so
	// we can keep <dir> in sync with speed-vector
	// (unlike other world objects, projectiles must
	// always point directly along their speed-vector)
	//
	// should be called when speed-vector is changed
	// s.t. both speed.w and dir need to be updated
	void SetVelocityAndSpeed(const float3& vel) {
		CWorldObject::SetVelocityAndSpeed(vel);

		if (speed.w > 0.0f) {
			dir = speed / speed.w;
		}
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

	void SetQuadFieldCellCoors(const int2& cell) { quadFieldCellCoors = cell; }
	const int2& GetQuadFieldCellCoors() const { return quadFieldCellCoors; }

	void SetQuadFieldCellIter(const std::list<CProjectile*>::iterator& it) { quadFieldCellIter = it; }
	const std::list<CProjectile*>::iterator& GetQuadFieldCellIter() { return quadFieldCellIter; }

	unsigned int GetProjectileType() const { return projectileType; }
	unsigned int GetCollisionFlags() const { return collisionFlags; }

	void SetCustomExplosionGeneratorID(unsigned int id) { cegID = id; }

	// UNSYNCED ONLY
	CMatrix44f GetTransformMatrix(bool offsetPos) const;

public:
	static bool inArray;
	static CVertexArray* va;
	static int DrawArray();

	bool synced; ///< is this projectile part of the simulation?
	bool weapon; ///< is this a weapon projectile? (true implies synced true)
	bool piece;  ///< is this a piece projectile? (true implies synced true)

	bool luaMoveCtrl;
	bool checkCol;
	bool ignoreWater;
	bool deleteMe;
	bool castShadow;

	float3 dir;
	float3 drawPos;

	unsigned lastProjUpdate;

	float mygravity;
	float tempdist; ///< temp distance used for sorting when rendering

protected:
	unsigned int ownerID;
	unsigned int teamID;
	unsigned int cegID;

	unsigned int projectileType;
	unsigned int collisionFlags;

	int2 quadFieldCellCoors;
	std::list<CProjectile*>::iterator quadFieldCellIter;
};

#endif /* PROJECTILE_H */

