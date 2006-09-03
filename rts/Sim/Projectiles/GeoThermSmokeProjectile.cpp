#include "StdAfx.h"
#include "GeoThermSmokeProjectile.h"
#include "Sim/Misc/Feature.h"

CGeoThermSmokeProjectile::CGeoThermSmokeProjectile(const float3& pos,const float3& speed,int ttl, CFeature* geo)
: CSmokeProjectile(pos,speed,ttl,6,0.35f,0,0.8f), geo(geo)
{}

void CGeoThermSmokeProjectile::Update()
{
	if (geo->solidOnTop)
	{
		CSolidObject *o = geo->solidOnTop;

		float3 d = pos - o->pos;
		float sql = d.SqLength();
		if (sql > 0.0f && sql < o->radius*o->radius)
		{
			d *= o->radius / sqrtf(sql);
			pos = pos * 0.3f + (o->pos + d) * 0.7f;

			if(d.y < o->radius*0.4f)
			{
				float speedlen = speed.Length();
				float3 right(d.z, 0.0f, -d.x);
				speed = d.cross(right);
				speed.Normalize();
				speed *= speedlen;
			}
		}
	}
	float l = speed.Length();
	speed.y += 1.0f;
	speed *= l/speed.Length();

	CSmokeProjectile::Update();
}
