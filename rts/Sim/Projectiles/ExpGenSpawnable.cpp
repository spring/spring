#include "ExpGenSpawnable.h"

#include "ExpGenSpawnableMemberInfo.h"
#include "ExpGenSpawner.h"
#include "ProjectileMemPool.h"
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
#include "System/Sync/HsiehHash.h"


CR_BIND_DERIVED_INTERFACE_POOL(CExpGenSpawnable, CWorldObject, projMemPool.allocMem, projMemPool.freeMem)
CR_REG_METADATA(CExpGenSpawnable, )

CExpGenSpawnable::CExpGenSpawnable(const float3& pos, const float3& spd)
 : CWorldObject(pos, spd)
{
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::CExpGenSpawnable()
 : CWorldObject()
{
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::~CExpGenSpawnable()
{
	assert(projMemPool.mapped(this));
}



bool CExpGenSpawnable::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	static const unsigned int memberHashes[] = {
		HsiehHash(          "pos",  sizeof(          "pos") - 1, 0),
		HsiehHash(        "speed",  sizeof(        "speed") - 1, 0),
		HsiehHash(    "useairlos",  sizeof(    "useairlos") - 1, 0),
		HsiehHash("alwaysvisible",  sizeof("alwaysvisible") - 1, 0),
	};

	CHECK_MEMBER_INFO_FLOAT3_HASH(CExpGenSpawnable, pos          , memberHashes[0])
	CHECK_MEMBER_INFO_FLOAT4_HASH(CExpGenSpawnable, speed        , memberHashes[1])
	CHECK_MEMBER_INFO_BOOL_HASH  (CExpGenSpawnable, useAirLos    , memberHashes[2])
	CHECK_MEMBER_INFO_BOOL_HASH  (CExpGenSpawnable, alwaysVisible, memberHashes[3])

	return false;
}

#define CHECK_ALL_SPAWNABLES() \
	CHECK_SPAWNABLE(CExpGenSpawner)         \
	CHECK_SPAWNABLE(CStandardGroundFlash)   \
	CHECK_SPAWNABLE(CSimpleGroundFlash)     \
	CHECK_SPAWNABLE(CBitmapMuzzleFlame)     \
	CHECK_SPAWNABLE(CDirtProjectile)        \
	CHECK_SPAWNABLE(CExploSpikeProjectile)  \
	CHECK_SPAWNABLE(CHeatCloudProjectile)   \
	CHECK_SPAWNABLE(CNanoProjectile)        \
	CHECK_SPAWNABLE(CSimpleParticleSystem)  \
	CHECK_SPAWNABLE(CSphereParticleSpawner) \
	CHECK_SPAWNABLE(CSmokeProjectile)       \
	CHECK_SPAWNABLE(CSmokeProjectile2)      \
	CHECK_SPAWNABLE(CSpherePartSpawner)     \
	CHECK_SPAWNABLE(CTracerProjectile)      \


bool CExpGenSpawnable::GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo)
{
#define CHECK_SPAWNABLE(spawnable) \
	if (spawnableName == #spawnable) \
		return spawnable::GetMemberInfo(memberInfo);

	CHECK_ALL_SPAWNABLES()

#undef CHECK_SPAWNABLE

	return false;
}


int CExpGenSpawnable::GetSpawnableID(const std::string& spawnableName)
{
	int i = 0;
#define CHECK_SPAWNABLE(spawnable)   \
	if (spawnableName == #spawnable) \
		return i;                    \
	++i;

	CHECK_ALL_SPAWNABLES()

#undef CHECK_SPAWNABLE

	return -1;
}


CExpGenSpawnable* CExpGenSpawnable::CreateSpawnable(int spawnableID)
{
	int i = 0;
#define CHECK_SPAWNABLE(spawnable)               \
	if (spawnableID == i)                        \
		return (projMemPool.alloc<spawnable>()); \
	++i;

	CHECK_ALL_SPAWNABLES()

#undef CHECK_SPAWNABLE

	return nullptr;
}
