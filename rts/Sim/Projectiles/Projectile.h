#ifndef PROJECTILE_H
#define PROJECTILE_H
#include "Rendering/GL/myGL.h"
// Projectile.h: interface for the CProjectile class.
//
//////////////////////////////////////////////////////////////////////


#ifdef _MSC_VER
#pragma warning(disable:4291)
#endif

class CProjectileHandler;
class CBuilding;

#include "ExplosionGenerator.h"
#include "Sim/Units/UnitHandler.h"

class CUnit;
class CFeature;
class CVertexArray;
struct S3DModel;

#define COLLISION_NOFRIENDLY	1
#define COLLISION_NOFEATURE		2
#define COLLISION_NONEUTRAL		4

class CProjectile: public CExpGenSpawnable
{
public:
	CR_DECLARE(CProjectile);

	static bool inArray;
	static CVertexArray* va;
	static int DrawArray();

	virtual void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	CProjectile(); // default constructor is needed for creg
	CProjectile(const float3& pos, const float3& speed, CUnit* owner, bool synced, bool weapon GML_PARG_H);
	virtual void Collision();
	virtual void Collision(CUnit* unit);
	virtual void Collision(CFeature* feature);
	virtual ~CProjectile();
	virtual void Update();
	virtual void Init(const float3& pos, CUnit* owner GML_PARG_H);

	bool synced;
	bool weapon;
	bool checkCol;
	bool deleteMe;
	bool castShadow;
	unsigned int collisionFlags;

	void UpdateDrawPos();
	float3 drawPos;
#if defined(USE_GML) && GML_ENABLE_SIM
	unsigned lastProjUpdate;
#endif

	inline CUnit* owner() const
	{
		return uh->units[ownerId];
	}

	float3 speed;
	virtual void DrawCallback(void);
	virtual void DrawUnitPart(void);
	virtual void DrawS3O() { DrawUnitPart(); }

	S3DModel* s3domodel;

	float tempdist; // temp distance used for sorting when rendering
	
private:
	int ownerId;
};

#endif /* PROJECTILE_H */
