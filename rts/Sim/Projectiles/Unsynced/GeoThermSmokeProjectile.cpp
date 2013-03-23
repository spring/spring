/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GeoThermSmokeProjectile.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/Wind.h"

CR_BIND_DERIVED(CGeoThermSmokeProjectile, CSmokeProjectile, (ZeroVector, ZeroVector, 1, NULL));

CR_REG_METADATA(CGeoThermSmokeProjectile, (
	CR_MEMBER(geo),
	CR_RESERVED(8)
));

CGeoThermSmokeProjectile::CGeoThermSmokeProjectile(const float3& pos, const float3& speed, int ttl, CFeature* geo)
	: CSmokeProjectile(pos, speed, ttl, 6, 0.35f, NULL, 0.8f)
	, geo(geo)
{}

void CGeoThermSmokeProjectile::Update()
{
	if (geo && geo->solidOnTop) {
		CSolidObject* o = geo->solidOnTop;

		float3 d = pos - o->pos;
		float sql = d.SqLength();

		if ((sql > 0.0f) && (sql < (o->radius * o->radius)) && o->blocking) {
			d *= o->radius * fastmath::isqrt(sql);
			pos = pos * 0.3f + (o->pos + d) * 0.7f;

			if (d.y < (o->radius * 0.4f)) {
				const float speedlen = fastmath::apxsqrt(speed.SqLength());
				float3 right(d.z, 0.0f, -d.x);
				speed = d.cross(right);
				speed.ANormalize();
				speed *= speedlen;
			}
		}
	}

	const float l = fastmath::apxsqrt(speed.SqLength());
	speed.y += 1.0f;
	speed.x += (wind.GetCurrentWind().x / GAME_SPEED);
	speed.z += (wind.GetCurrentWind().z / GAME_SPEED);
	speed *= (l * fastmath::isqrt(speed.SqLength()));

	CSmokeProjectile::Update();
}


void CGeoThermSmokeProjectile::GeoThermDestroyed(const CFeature* geo)
{
	for (ProjectileContainer::iterator it = projectileHandler->unsyncedProjectiles.begin(); it != projectileHandler->unsyncedProjectiles.end(); ++it) {
		CGeoThermSmokeProjectile* geoPuff = dynamic_cast<CGeoThermSmokeProjectile*>(*it);
		if (geoPuff) {
			if (geoPuff->geo == geo) {
				geoPuff->geo = NULL;
			}
		}
	}
}
