#ifndef PIECEPROJECTILE_H
#define PIECEPROJECTILE_H

#include "Projectile.h"
#include "Sim/Misc/DamageArray.h"

const int PP_Fall       = (1 << 0); // 1, if they dont fall they could live forever
const int PP_Smoke      = (1 << 1); // 2, smoke and fire is turned off when there are too many projectiles so make sure they are unsynced
const int PP_Fire       = (1 << 2); // 4
const int PP_Explode    = (1 << 3); // 8
const int PP_NoCEGTrail = (1 << 6); // 64, TODO should be 16

class CSmokeTrailProjectile;
struct LocalS3DO;
struct S3DO;
struct SS3O;

class CPieceProjectile: public CProjectile
{
	CR_DECLARE(CPieceProjectile);

	void creg_Serialize(creg::ISerializer& s);

	int flags;
	int dispList;
	S3DO* piece3do;
	SS3O* pieces3o;
	float3 spinVec;
	float spinSpeed;
	float spinPos;
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
	CPieceProjectile(const float3& pos, const float3& speed, LocalS3DO* piece, int flags, CUnit* owner, float radius);
	virtual ~CPieceProjectile(void);
	void Update();
	void Draw();
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
