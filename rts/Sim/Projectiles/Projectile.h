/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <list>

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
	virtual void Detach();

	virtual void Collision();
	virtual void Collision(CUnit* unit);
	virtual void Collision(CFeature* feature);
	virtual void Update();
	virtual void Init(const CUnit* owner, const float3& offset);

	virtual void Draw() {}
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	virtual void DrawCallback() {}

	struct QuadFieldCellData {
		CR_DECLARE_STRUCT(QuadFieldCellData)

		// must match typeof(QuadField::Quad::projectiles)
		typedef std::list<CProjectile*>::iterator iter;

		const int2& GetCoor(unsigned int idx) const { return coors[idx]; }
		const iter& GetIter(unsigned int idx) const { return iters[idx]; }

		void SetCoor(unsigned int idx, const int2& co) { coors[idx] = co; }
		void SetIter(unsigned int idx, const iter& it) { iters[idx] = it; }

	private:
		// coordinates and iterators for pos, (pos+spd)*0.5, pos+spd
		// non-hitscan projectiles *only* use coors[0] and iters[0]!
		int2 coors[3];
		iter iters[3];
	};

	// override WorldObject::SetVelocityAndSpeed so
	// we can keep <dir> in sync with speed-vector
	// (unlike other world objects, projectiles must
	// always point directly along their vel-vector)
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

	void SetQuadFieldCellData(const QuadFieldCellData& qfcd) { qfCellData = qfcd; }
	const QuadFieldCellData& GetQuadFieldCellData() const { return qfCellData; }
	      QuadFieldCellData& GetQuadFieldCellData()       { return qfCellData; }

	unsigned int GetProjectileType() const { return projectileType; }
	unsigned int GetCollisionFlags() const { return collisionFlags; }

	void SetCustomExplosionGeneratorID(unsigned int id) { cegID = id; }

	// UNSYNCED ONLY
	CMatrix44f GetTransformMatrix(bool offsetPos) const;

public:
	static bool inArray;
	static CVertexArray* va;
	static int DrawArray();

	bool synced;  ///< is this projectile part of the simulation?
	bool weapon;  ///< is this a weapon projectile? (true implies synced true)
	bool piece;   ///< is this a piece projectile? (true implies synced true)
	bool hitscan; ///< is this a hit-scan projectile?

	bool luaMoveCtrl;
	bool checkCol;
	bool ignoreWater;
	bool deleteMe;
	bool castShadow;

	float3 dir;
	float3 drawPos;


	float mygravity;
	float tempdist; ///< temp distance used for sorting when rendering

protected:
	unsigned int ownerID;
	unsigned int teamID;
	unsigned int cegID;

	unsigned int projectileType;
	unsigned int collisionFlags;

	QuadFieldCellData qfCellData;
};

#endif /* PROJECTILE_H */

