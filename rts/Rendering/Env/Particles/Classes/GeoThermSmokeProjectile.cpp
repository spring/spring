/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GeoThermSmokeProjectile.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/Wind.h"


CR_BIND_DERIVED(CGeoThermSmokeProjectile, CSmokeProjectile, )

CR_REG_METADATA(CGeoThermSmokeProjectile, (
	CR_MEMBER(geo)
))


CGeoThermSmokeProjectile::CGeoThermSmokeProjectile(
	const float3& pos,
	const float3& spd,
	int ttl,
	const CFeature* geo
)
	: CSmokeProjectile(nullptr, pos, spd, ttl, 6, 0.35f, 0.8f)
	, geo(geo)
{ }

void CGeoThermSmokeProjectile::Update()
{
	UpdateDir();

	// NOTE:
	//   speed.w is never changed from its initial value (!), otherwise the
	//   particles would just keep accelerating even when wind == ZeroVector
	//   due to UpVector being added each frame --> if |speed| grows LARGER
	//   than speed.w then newSpeed will be adjusted downward and vice versa
	CWorldObject::SetVelocity(speed + UpVector);
	CWorldObject::SetVelocity(speed + XZVector * (envResHandler.GetCurrentWindVec() / GAME_SPEED));

	const float curSpeed = fastmath::sqrt_builtin(speed.SqLength());
	const float newSpeed = speed.w * (speed.w / curSpeed);

	CWorldObject::SetVelocity((dir = (speed / curSpeed)) * newSpeed);
	CSmokeProjectile::Update();
}

void CGeoThermSmokeProjectile::UpdateDir()
{
	if (geo == nullptr)
		return;

	const CSolidObject* obj = geo->solidOnTop;

	if (obj == nullptr)
		return;
	if (!obj->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return;

	float3 geoVector = pos - obj->pos;

	if (geoVector.SqLength() == 0.0f)
		return;
	if (geoVector.SqLength() >= (obj->radius * obj->radius))
		return;

	geoVector *= (obj->radius * fastmath::isqrt_sse(geoVector.SqLength()));

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
	for (CProjectile* p: projectileHandler.projectileContainers[false]) {
		CGeoThermSmokeProjectile* geoPuff = dynamic_cast<CGeoThermSmokeProjectile*>(p);

		if (geoPuff == nullptr)
			continue;
		if (geoPuff->geo != geo)
			continue;

		geoPuff->geo = nullptr;
	}
}

