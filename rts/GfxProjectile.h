// GfxProjectile.h: interface for the CGfxProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GFXPROJECTILE_H__666D5ED5_9AC0_418E_883E_8066B05A37D9__INCLUDED_)
#define AFX_GFXPROJECTILE_H__666D5ED5_9AC0_418E_883E_8066B05A37D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Projectile.h"

class CGfxProjectile : public CProjectile  
{
public:
	void Update();
	void Draw();
	CGfxProjectile(const float3& pos,const float3& speed,int lifeTime,const float3& color);
	virtual ~CGfxProjectile();

	int creationTime;
	int lifeTime;
	unsigned char color[4];
};

#endif // !defined(AFX_GFXPROJECTILE_H__666D5ED5_9AC0_418E_883E_8066B05A37D9__INCLUDED_)

