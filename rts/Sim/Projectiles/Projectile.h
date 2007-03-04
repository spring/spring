#ifndef PROJECTILE_H
#define PROJECTILE_H
// Projectile.h: interface for the CProjectile class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4291)

class CProjectileHandler;
class CFace;
class CEdge;
class CBuilding;

#include "ExplosionGenerator.h"
class CUnit;
class CFeature;
class CVertexArray;
struct S3DOModel;

#define COLLISION_NOFRIENDLY		1
#define COLLISION_NOFEATURE			2

class CProjectile : public CExpGenSpawnable
{
public:
	CR_DECLARE(CProjectile);

	static bool inArray;
	static CVertexArray* va;
	static void DrawArray();

	virtual void Draw();
	CProjectile(); // default constructor is needed for creg
	CProjectile(const float3& pos,const float3& speed,CUnit* owner, bool synced);
	virtual void Collision();
	virtual void Collision(CUnit* unit);
	virtual void Collision(CFeature* feature);
	virtual ~CProjectile();
	virtual void Update();
	void DependentDied(CObject* o);
	virtual void Init(const float3& pos, CUnit *owner);
	
	bool synced;
	bool checkCol;
	bool deleteMe;
	bool castShadow;
	unsigned int collisionFlags;

	CUnit* owner;
	float3 speed;
	virtual void DrawCallback(void);
	virtual void DrawUnitPart(void);
	virtual void DrawS3O(){DrawUnitPart();};

	S3DOModel* s3domodel;
};

#endif /* PROJECTILE_H */
