/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PIECE_PROJECTILE_H
#define PIECE_PROJECTILE_H

#include "Projectile.h"
#include "Sim/Misc/DamageArray.h"

// Piece Explosion Flags
const int PF_Shatter    = (1 << 0); // 1
const int PF_Explode    = (1 << 1); // 2
const int PF_Fall       = (1 << 2); // 4, if they dont fall they could live forever
const int PF_Smoke      = (1 << 3); // 8, smoke and fire is turned off when there are too many projectiles so make sure they are unsynced
const int PF_Fire       = (1 << 4); // 16
const int PF_NONE       = (1 << 5); // 32
const int PF_NoCEGTrail = (1 << 6); // 64
const int PF_NoHeatCloud= (1 << 7); // 128
const int PF_Recursive  = (1 << 8); // 256

class CSmokeTrailProjectile;
struct S3DModelPiece;
struct LocalModelPiece;

class CPieceProjectile: public CProjectile
{
	CR_DECLARE(CPieceProjectile)

public:
	CPieceProjectile(
		CUnit* owner,
		LocalModelPiece* piece,
		const float3& pos,
		const float3& speed,
		int flags,
		float radius
	);
	virtual ~CPieceProjectile();
	virtual void Detach();

	void Update();
	void Draw();
	void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	void Collision();
	void Collision(CUnit* unit);

	void DrawCallback();

private:
	bool HasVertices() const;
	float3 RandomVertexPos();

public:
	struct FireTrailPoint {
		float3 pos;
		float size;
	};

	int age;

	unsigned int explFlags;
	unsigned int dispList;

	const S3DModelPiece* omp;

	CSmokeTrailProjectile* curCallback;
	FireTrailPoint* fireTrailPoints[8];

	float3 spinVec;
	float spinSpeed;
	float spinAngle;

	float3 oldSmokePos;
	float3 oldSmokeDir;

	bool drawTrail;
};

#endif /* PIECE_PROJECTILE_H */
