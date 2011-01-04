/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PIECE_PROJECTILE_H
#define PIECE_PROJECTILE_H

#include "Projectile.h"
#include "Sim/Misc/DamageArray.h"

//! Piece Flags
const int PF_Shatter    = (1 << 0); // 1
const int PF_Explode    = (1 << 1); // 2
const int PF_Fall       = (1 << 2); // 4, if they dont fall they could live forever
const int PF_Smoke      = (1 << 3); // 8, smoke and fire is turned off when there are too many projectiles so make sure they are unsynced
const int PF_Fire       = (1 << 4); // 16
const int PF_NONE       = (1 << 5); // 32
const int PF_NoCEGTrail = (1 << 6); // 64
const int PF_NoHeatCloud= (1 << 7); // 128

class CSmokeTrailProjectile;
struct S3DModelPiece;
struct LocalModelPiece;
struct S3DOPiece;
struct SS3OPiece;

class CPieceProjectile: public CProjectile
{
	CR_DECLARE(CPieceProjectile);

	void creg_Serialize(creg::ISerializer& s);

public:
	CPieceProjectile(const float3& pos, const float3& speed, LocalModelPiece* piece, int flags, CUnit* owner, float radius);
	virtual ~CPieceProjectile();

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	void Collision();
	void Collision(CUnit* unit);

	void DrawCallback();

private:
	bool HasVertices() const;
	float3 RandomVertexPos();

public:
	unsigned int flags;
	unsigned int dispList;
	unsigned int cegID;

	const S3DModelPiece* omp;

	float3 spinVec;
	float spinSpeed;
	float spinAngle;

	float alphaThreshold;

	float3 oldSmoke, oldSmokeDir;
	bool drawTrail;
	CSmokeTrailProjectile* curCallback;
	int* numCallback;
	int age;

	struct OldInfo {
		float3 pos;
		float size;
	};
	OldInfo* oldInfos[8];

	int colorTeam;
};

#endif /* PIECE_PROJECTILE_H */
