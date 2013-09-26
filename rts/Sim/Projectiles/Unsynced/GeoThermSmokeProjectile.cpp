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
	if (geo != NULL && geo->solidOnTop != NULL) {
		const CSolidObject* o = geo->solidOnTop;

		float3 geoVector = pos - o->pos;
		const float sql = geoVector.SqLength();

		if ((sql > 0.0f) && (sql < (o->radius * o->radius)) && o->collidable) {
			geoVector *= (o->radius * fastmath::isqrt(sql));

			SetPosition(pos * 0.3f + (o->pos + geoVector) * 0.7f);

			if (geoVector.y < (o->radius * 0.4f)) {
				const float3 orthoDir = float3(geoVector.z, 0.0f, -geoVector.x);
				const float3 newDir = (geoVector.cross(orthoDir)).ANormalize();

				SetVelocityAndSpeed(newDir * speed.w);
			}
		}
	}

	CWorldObject::SetVelocity(speed + UpVector);
	CWorldObject::SetVelocity(speed + XZVector * (wind.GetCurrentWind() / GAME_SPEED));
	// now update dir and speed.w
	SetVelocityAndSpeed(speed);

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
