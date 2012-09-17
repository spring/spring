/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "lib/gml/gml_base.h"

#ifdef _MSC_VER
#pragma warning(disable:4291)
#endif

#include "ExplosionGenerator.h"
#include "Sim/Units/UnitHandler.h"
#include "System/float3.h"
#include "System/Vec2.h"

class CUnit;
class CFeature;
class CVertexArray;


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

	inline CUnit* owner() const {
		// Note: this death dependency optimization using "ownerId" is logically flawed,
		//  since ids are being reused it could return a unit that is not the original owner
		CUnit* unit = uh->GetUnit(ownerId);
		return GML::SimEnabled() ? *(CUnit* volatile*)&unit : unit; // make volatile
	}

	int GetOwnerID() const {
		return ownerId;
	}

	void SetQuadFieldCellCoors(const int2& cell) { quadFieldCellCoors = cell; }
	int2 GetQuadFieldCellCoors() const { return quadFieldCellCoors; }

	void SetQuadFieldCellIter(const std::list<CProjectile*>::iterator& it) { quadFieldCellIter = it; }
	const std::list<CProjectile*>::iterator& GetQuadFieldCellIter() { return quadFieldCellIter; }

	unsigned int GetProjectileType() const { return projectileType; }
	unsigned int GetCollisionFlags() const { return collisionFlags; }


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

	unsigned lastProjUpdate;

	float3 dir;
	float3 speed;
	float3 drawPos;

	float mygravity;
	float tempdist; ///< temp distance used for sorting when rendering

protected:
	int ownerId;
	unsigned int projectileType;
	unsigned int collisionFlags;

	int2 quadFieldCellCoors;
	std::list<CProjectile*>::iterator quadFieldCellIter;
};

#endif /* PROJECTILE_H */
