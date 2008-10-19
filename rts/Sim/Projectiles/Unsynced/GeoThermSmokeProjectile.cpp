#include "StdAfx.h"
#include "mmgr.h"

#include "GeoThermSmokeProjectile.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CGeoThermSmokeProjectile,CSmokeProjectile,(float3(),float3(),1,0));

CR_REG_METADATA(CGeoThermSmokeProjectile, (
	CR_MEMBER(geo),
	CR_RESERVED(8)
	));

CGeoThermSmokeProjectile::CGeoThermSmokeProjectile(const float3& pos,const float3& speed,int ttl, CFeature* geo)
: CSmokeProjectile(pos,speed,ttl,6,0.35f,0,0.8f), geo(geo)
{}

void CGeoThermSmokeProjectile::Update()
{
	if (geo && geo->solidOnTop)
	{
		CSolidObject *o = geo->solidOnTop;

		float3 d = pos - o->pos;
		float sql = d.SqLength();
		if (sql > 0.0f && sql < o->radius*o->radius && o->blocking)
		{
			d *= o->radius / sqrt(sql);
			pos = pos * 0.3f + (o->pos + d) * 0.7f;

			if(d.y < o->radius*0.4f)
			{
				float speedlen = speed.Length();
				float3 right(d.z, 0.0f, -d.x);
				speed = d.cross(right);
				speed.ANormalize();
				speed *= speedlen;
			}
		}
	}
	float l = speed.Length();
	speed.y += 1.0f;
	speed *= l/speed.Length();

	CSmokeProjectile::Update();
}


void CGeoThermSmokeProjectile::GeoThermDestroyed(const CFeature* geo)
{
	Projectile_List& pList = ph->ps;
	Projectile_List::iterator it;
	for (it = pList.begin(); it != pList.end(); ++it) {
		CGeoThermSmokeProjectile* geoPuff =
			dynamic_cast<CGeoThermSmokeProjectile*>(*it);
		if (geoPuff) {
			geoPuff->geo = NULL;
		}
	}
}


