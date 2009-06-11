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

CGeoThermSmokeProjectile::CGeoThermSmokeProjectile(const float3& pos,const float3& speed,int ttl, CFeature* geo GML_PARG_C)
: CSmokeProjectile(pos,speed,ttl,6,0.35f,0,0.8f GML_PARG_P), geo(geo)
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
			d *= o->radius * fastmath::isqrt(sql);
			pos = pos * 0.3f + (o->pos + d) * 0.7f;

			if(d.y < o->radius*0.4f)
			{
				float speedlen = fastmath::sqrt(speed.SqLength());
				float3 right(d.z, 0.0f, -d.x);
				speed = d.cross(right);
				speed.ANormalize();
				speed *= speedlen;
			}
		}
	}
	float l = fastmath::sqrt(speed.SqLength());
	speed.y += 1.0f;
	speed *= l * fastmath::isqrt(speed.SqLength());

	CSmokeProjectile::Update();
}


void CGeoThermSmokeProjectile::GeoThermDestroyed(const CFeature* geo)
{
	for (ProjectileContainer::iterator it = ph->projectiles.begin(); it != ph->projectiles.end(); ++it) {
		CGeoThermSmokeProjectile* geoPuff = dynamic_cast<CGeoThermSmokeProjectile*>(*it);
		if (geoPuff) {
			geoPuff->geo = NULL;
		}
	}
}


