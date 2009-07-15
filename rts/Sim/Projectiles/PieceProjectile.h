#ifndef PIECEPROJECTILE_H
#define PIECEPROJECTILE_H

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

	int flags;
	int dispList;
	S3DOPiece* piece3do;
	SS3OPiece* pieces3o;
	S3DModelPiece* omp;
	float3 spinVec;
	float spinSpeed;
	float spinAngle;
	float alphaThreshold;

	float3 oldSmoke, oldSmokeDir;
	bool drawTrail;
	CSmokeTrailProjectile* curCallback;
	int* numCallback;
	int age;

	CCustomExplosionGenerator ceg;

	struct OldInfo {
		float3 pos;
		float size;
	};
	OldInfo* oldInfos[8];
	int colorTeam;

public:
	CPieceProjectile(const float3& pos, const float3& speed, LocalModelPiece* piece, int flags, CUnit* owner, float radius GML_PARG_H);
	virtual ~CPieceProjectile(void);
	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	void Collision();
	void Collision(CUnit* unit);

	void DrawUnitPart(void);
	void DrawCallback(void);

	// should not be here
	void DrawS3O(void);
private:
	bool HasVertices();
	float3 RandomVertexPos();
};


#endif /* PIECEPROJECTILE_H */
