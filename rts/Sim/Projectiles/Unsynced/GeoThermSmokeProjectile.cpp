/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GeoThermSmokeProjectile.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/Wind.h"

CR_BIND_DERIVED(CGeoThermSmokeProjectile, CSmokeProjectile, (ZeroVector, ZeroVector, 1, NULL))

CR_REG_METADATA(CGeoThermSmokeProjectile, (
	CR_MEMBER(geo),
	CR_RESERVED(8)
))

CGeoThermSmokeProjectile::CGeoThermSmokeProjectile(
	const float3& pos,
	const float3& spd,
	int ttl,
	const CFeature* geo
)
	: CSmokeProjectile(NULL, pos, spd, ttl, 6, 0.35f, 0.8f)
	, geo(geo)
{}

void CGeoThermSmokeProjectile::Update()
{
	UpdateDir();

	// NOTE:
	//   speed.w is never changed from its initial value (!), otherwise the
	//   particles would just keep accelerating even when wind == ZeroVector
	//   due to UpVector being added each frame --> if |speed| grows LARGER
	//   than speed.w then newSpeed will be adjusted downward and vice versa
	CWorldObject::SetVelocity(speed + UpVector);
	CWorldObject::SetVelocity(speed + XZVector * (wind.GetCurrentWind() / GAME_SPEED));

	const float curSpeed = fastmath::sqrt(speed.SqLength());
	const float newSpeed = speed.w * (speed.w / curSpeed);

	CWorldObject::SetVelocity((dir = (speed / curSpeed)) * newSpeed);
	CSmokeProjectile::Update();
}

void CGeoThermSmokeProjectile::UpdateDir()
{
	if (geo == NULL)
		return;

	const CSolidObject* obj = geo->solidOnTop;

	if (obj == NULL)
		return;
	if (!obj->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return;

	float3 geoVector = pos - obj->pos;

	if (geoVector.SqLength() == 0.0f)
		return;
	if (geoVector.SqLength() >= (obj->radius * obj->radius))
		return;

	geoVector *= (obj->radius * fastmath::isqrt(geoVector.SqLength()));

	SetPosition(pos * 0.3f + (obj->pos + geoVector) * 0.7f);

	if (geoVector.y >= (obj->radius * 0.4f))
		return;

	// escape sideways if covered by geothermal object (unreliable)
	const float3 orthoDir = float3(geoVector.z, 0.0f, -geoVector.x);
	const float3 newDir = (geoVector.cross(orthoDir)).ANormalize();

	SetVelocityAndSpeed(newDir * speed.w);
}

void CGeoThermSmokeProjectile::GeoThermDestroyed(const CFeature* geo)
{
	for (ProjectileContainer::iterator it = projectileHandler->unsyncedProjectiles.begin(); it != projectileHandler->unsyncedProjectiles.end(); ++it) {
		CGeoThermSmokeProjectile* geoPuff = dynamic_cast<CGeoThermSmokeProjectile*>(*it);

		if (geoPuff == NULL)
			continue;
		if (geoPuff->geo != geo)
			continue;

		geoPuff->geo = NULL;
	}
}

