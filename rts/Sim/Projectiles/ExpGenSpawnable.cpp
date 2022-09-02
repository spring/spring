#include <limits>

#include "ExpGenSpawnableMemberInfo.h"
#include "ExpGenSpawner.h"
#include "ProjectileMemPool.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/GlobalRendering.h"
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
#include "Rendering/GL/RenderBuffers.h"
#include "System/Sync/HsiehHash.h"
#include "Sim/Misc/GlobalSynced.h"


CR_BIND_DERIVED_INTERFACE_POOL(CExpGenSpawnable, CWorldObject, projMemPool.allocMem, projMemPool.freeMem)
CR_REG_METADATA(CExpGenSpawnable, (
	CR_MEMBER(rotVal),
	CR_MEMBER(rotVel),
	CR_MEMBER(createFrame),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(rotParams),
		CR_MEMBER(animParams),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_IGNORED(animProgress)
))

CExpGenSpawnable::CExpGenSpawnable(const float3& pos, const float3& spd)
	: CWorldObject(pos, spd)
	, createFrame{0}
	, rotVal{0}
	, rotVel{0}
{
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::CExpGenSpawnable()
	: CWorldObject()
	, createFrame{ 0 }
	, rotVal{ 0 }
	, rotVel{ 0 }
{
	assert(projMemPool.alloced(this));
}

CExpGenSpawnable::~CExpGenSpawnable()
{
	assert(projMemPool.mapped(this));
}

void CExpGenSpawnable::Init(const CUnit* owner, const float3& offset)
{
	createFrame = gs->frameNum;
	rotParams *= float3(math::DEG_TO_RAD / GAME_SPEED, math::DEG_TO_RAD / (GAME_SPEED * GAME_SPEED), math::DEG_TO_RAD);

	UpdateRotation();
}

void CExpGenSpawnable::UpdateRotation()
{
	const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
	// rotParams.y is acceleration in angle per frame^2
	rotVel = rotParams.x + rotParams.y * t;
	rotVal = rotParams.z + rotVel * t;
}

void CExpGenSpawnable::UpdateAnimParams()
{
	if (static_cast<int>(animParams.x) <= 1 && static_cast<int>(animParams.y) <= 1) {
		animProgress = 0.0f;
		return;
	}

	const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
	animProgress = math::fmod(t, animParams.z) / animParams.z;
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

	CHECK_MEMBER_INFO_FLOAT3(CExpGenSpawnable, rotParams)
	CHECK_MEMBER_INFO_FLOAT3(CExpGenSpawnable, animParams)

	return false;
}

TypedRenderBuffer<VA_TYPE_PROJ>& CExpGenSpawnable::GetPrimaryRenderBuffer()
{
	return RenderBuffer::GetTypedRenderBuffer<VA_TYPE_PROJ>();
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

void CExpGenSpawnable::AddEffectsQuad(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl)
{
	float minS = std::numeric_limits<float>::max()   ; float minT = std::numeric_limits<float>::max()   ;
	float maxS = std::numeric_limits<float>::lowest(); float maxT = std::numeric_limits<float>::lowest();

	std::invoke([&minS, &minT, &maxS, &maxT](auto&&... args) {
		((minS = std::min(minS, args.s)), ...);
		((minT = std::min(minT, args.t)), ...);
		((maxS = std::max(maxS, args.s)), ...);
		((maxT = std::max(maxT, args.t)), ...);
	}, std::forward<VA_TYPE_TC>(tl), std::forward<VA_TYPE_TC>(tr), std::forward<VA_TYPE_TC>(br), std::forward<VA_TYPE_TC>(bl));

	auto& rb = GetPrimaryRenderBuffer();

	const float4 mm = float4{ minS, minT, maxS, maxT };
	const float3 animInfo = float3{ animParams.x, animParams.y, animProgress };
	constexpr float layer = 0.0f; //for future texture arrays

	//pos, uvw, uvmm, col
	rb.AddQuadTriangles(
		{ tl.pos, layer, mm, animInfo, std::move(tl.c) },
		{ tr.pos, layer, mm, animInfo, std::move(tr.c) },
		{ br.pos, layer, mm, animInfo, std::move(br.c) },
		{ bl.pos, layer, mm, animInfo, std::move(bl.c) }
	);
}