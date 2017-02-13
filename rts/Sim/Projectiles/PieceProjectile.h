/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PIECE_PROJECTILE_H
#define PIECE_PROJECTILE_H

#include "Projectile.h"

// Piece Explosion Flags
const int PF_Shatter    = (1 <<  0); // 1
const int PF_Explode    = (1 <<  1); // 2
const int PF_Fall       = (1 <<  2); // 4 (unused)
const int PF_Smoke      = (1 <<  3); // 8 (unsynced, smoke and fire are turned off when particle saturation >= 95%)
const int PF_Fire       = (1 <<  4); // 16
const int PF_NONE       = (1 <<  5); // 32
const int PF_NoCEGTrail = (1 <<  6); // 64
const int PF_NoHeatCloud= (1 <<  7); // 128
const int PF_Recursive  = (1 << 14); // 16384 (OTA-inherited COB scripts map [1<<8, 1<<13] to BITMAP* explosions)

class CSmokeTrailProjectile;
struct S3DModelPiece;
struct LocalModelPiece;

class CPieceProjectile: public CProjectile
{
	CR_DECLARE_DERIVED(CPieceProjectile)

public:
	CPieceProjectile(
		CUnit* owner,
		LocalModelPiece* piece,
		const float3& pos,
		const float3& speed,
		int flags,
		float radius
	);

	void Update() override;
	void Draw() override;
	void DrawOnMinimap(CVertexArray& lines, CVertexArray& points) override;
	void Collision() override;
	void Collision(CUnit* unit) override;
	void Collision(CFeature* f) override;

	int GetProjectilesCount() const override;

	float GetDrawAngle() const;

private:
	CPieceProjectile() { }
	float3 RandomVertexPos() const;
	void Collision(CUnit* unit, CFeature* feature);

public:
	struct FireTrailPoint {
		float3 pos;
		float size;
	};

	int age;

	unsigned int explFlags;
	unsigned int dispList;

	const S3DModelPiece* omp;

	static const unsigned NUM_TRAIL_PARTS = 8;
	FireTrailPoint fireTrailPoints[NUM_TRAIL_PARTS];

	float3 spinVec;
	float spinSpeed;
	float spinAngle;

	float3 oldSmokePos;
	float3 oldSmokeDir;
	CSmokeTrailProjectile* smokeTrail;
};

#endif /* PIECE_PROJECTILE_H */
