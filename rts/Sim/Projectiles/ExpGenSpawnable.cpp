#include "ExpGenSpawnable.h"

#include "ExpGenSpawner.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/Env/Particles/Classes/BitmapMuzzleFlame.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/DirtProjectile.h"
#include "Rendering/Env/Particles/Classes/ExploSpikeProjectile.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/NanoProjectile.h"
#include "Rendering/Env/Particles/Classes/SimpleParticleSystem.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile2.h"
#include "Rendering/Env/Particles/Classes/SpherePartProjectile.h"
#include "Rendering/Env/Particles/Classes/TracerProjectile.h"


CR_BIND_DERIVED_INTERFACE(CExpGenSpawnable, CWorldObject)
CR_REG_METADATA(CExpGenSpawnable, )


bool CExpGenSpawnable::GetMemberInfo(const std::string& memberName, SExpGenSpawnableMemberInfo& memberInfo)
{
	CHECK_MEMBER_INFO_FLOAT3(CExpGenSpawnable, pos          )
	CHECK_MEMBER_INFO_FLOAT4(CExpGenSpawnable, speed        )
	CHECK_MEMBER_INFO_BOOL  (CExpGenSpawnable, useAirLos    )
	CHECK_MEMBER_INFO_BOOL  (CExpGenSpawnable, alwaysVisible)

	return false;
}


bool CExpGenSpawnable::GetSpawnableMemberInfo(const std::string& spawnableName, const std::string& memberName, SExpGenSpawnableMemberInfo& memberInfo)
{
#define CHECK_SPAWNABLE(spawnable) \
	if (spawnableName == #spawnable) \
		return spawnable::GetMemberInfo(memberName, memberInfo);

	CHECK_SPAWNABLE(CExpGenSpawner)

	CHECK_SPAWNABLE(CStandardGroundFlash)
	CHECK_SPAWNABLE(CSimpleGroundFlash)

	CHECK_SPAWNABLE(CBitmapMuzzleFlame)
	CHECK_SPAWNABLE(CBubbleProjectile)
	CHECK_SPAWNABLE(CDirtProjectile)
	CHECK_SPAWNABLE(CExploSpikeProjectile)
	CHECK_SPAWNABLE(CHeatCloudProjectile)
	CHECK_SPAWNABLE(CNanoProjectile)
	CHECK_SPAWNABLE(CSimpleParticleSystem)
	CHECK_SPAWNABLE(CSphereParticleSpawner)
	CHECK_SPAWNABLE(CSmokeProjectile)
	CHECK_SPAWNABLE(CSmokeProjectile2)
	CHECK_SPAWNABLE(CSpherePartSpawner)
	CHECK_SPAWNABLE(CTracerProjectile)

#undef CHECK_SPAWNABLE

	return false;
}

bool CExpGenSpawnable::CheckClass(const std::string& spawnableName)
{
#define CHECK_SPAWNABLE(spawnable) \
	if (spawnableName == #spawnable) \
		return true;

	CHECK_SPAWNABLE(CExpGenSpawner)

	CHECK_SPAWNABLE(CStandardGroundFlash)
	CHECK_SPAWNABLE(CSimpleGroundFlash)

	CHECK_SPAWNABLE(CBitmapMuzzleFlame)
	CHECK_SPAWNABLE(CBubbleProjectile)
	CHECK_SPAWNABLE(CDirtProjectile)
	CHECK_SPAWNABLE(CExploSpikeProjectile)
	CHECK_SPAWNABLE(CHeatCloudProjectile)
	CHECK_SPAWNABLE(CNanoProjectile)
	CHECK_SPAWNABLE(CSimpleParticleSystem)
	CHECK_SPAWNABLE(CSphereParticleSpawner)
	CHECK_SPAWNABLE(CSmokeProjectile)
	CHECK_SPAWNABLE(CSmokeProjectile2)
	CHECK_SPAWNABLE(CSpherePartSpawner)
	CHECK_SPAWNABLE(CTracerProjectile)

#undef CHECK_SPAWNABLE

	return false;
}