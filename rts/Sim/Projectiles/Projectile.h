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

#include "Sim/Objects/WorldObject.h"
class CUnit;
class CFeature;
class CVertexArray;
struct S3DOModel;

class CProjectile : public CWorldObject
{
public:
	CR_DECLARE(CProjectile);

	static bool inArray;
	static CVertexArray* va;
	static void DrawArray();

	virtual void Draw();
	CProjectile(); // default constructor is needed for creg
	CProjectile(const float3& pos,const float3& speed,CUnit* owner);
	virtual void Collision();
	virtual void Collision(CUnit* unit);
	virtual void Collision(CFeature* feature);
	virtual ~CProjectile();
	virtual void Update();
	void DependentDied(CObject* o);
	virtual void Init(const float3& pos, CUnit *owner);

	bool checkCol;
	bool deleteMe;
	bool castShadow;

	CUnit* owner;
	float3 speed;
	virtual void DrawCallback(void);
	virtual void DrawUnitPart(void);
	virtual void DrawS3O(){DrawUnitPart();};

	S3DOModel* s3domodel;
};

#endif /* PROJECTILE_H */
