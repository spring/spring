#ifndef PROJECTILE_H
#define PROJECTILE_H
// Projectile.h: interface for the CProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROJECTILE_H__C7B284A9_3578_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_PROJECTILE_H__C7B284A9_3578_11D4_AD55_0080ADA84DE3__INCLUDED_

#pragma warning(disable:4291)

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

class CProjectileHandler;
class CFace;
class CEdge;
class CBuilding;

#include "MemPool.h"
#include "WorldObject.h"
class CUnit;
class CFeature;
class CVertexArray;

class CProjectile : public CWorldObject
{
public:
#ifndef SYNCIFY
	inline void* operator new(size_t size){return mempool.Alloc(size);};
	inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
#endif
	static bool inArray;
	static CVertexArray* va;
	static unsigned int textures[10];
	static void DrawArray();

	virtual void Draw();
	CProjectile(const float3& pos,const float3& speed,CUnit* owner);
	virtual void Collision();
	virtual void Collision(CUnit* unit);
	virtual void Collision(CFeature* feature);
	virtual ~CProjectile();
	virtual void Update();
	void DependentDied(CObject* o);

	bool checkCol;
	bool isUnitPart;
	bool deleteMe;
	bool castShadow;

	CUnit* owner;
	float3 speed;
	virtual void DrawCallback(void);			//används om en projektil vill ritas efter(ovanpå) en annan
	virtual void DrawUnitPart(void);
};

#endif // !defined(AFX_PROJECTILE_H__C7B284A9_3578_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* PROJECTILE_H */
