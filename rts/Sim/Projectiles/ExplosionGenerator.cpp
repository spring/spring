/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cinttypes>

#include "ExplosionGenerator.h"
#include "ExpGenSpawner.h" //!!
#include "ExpGenSpawnable.h"
#include "ExpGenSpawnableMemberInfo.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h" // guRNG
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/DirtProjectile.h"
#include "Rendering/Env/Particles/Classes/ExploSpikeProjectile.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile2.h"
#include "Rendering/Env/Particles/Classes/SpherePartProjectile.h"
#include "Rendering/Env/Particles/Classes/WakeProjectile.h"
#include "Rendering/Env/Particles/Classes/WreckProjectile.h"

#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"

#include "System/creg/STL_Map.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringHash.h"

static DynMemPool<sizeof(CCustomExplosionGenerator)> egMemPool;

static uint8_t exploParserMem[sizeof(LuaParser)];
static uint8_t aliasParserMem[sizeof(LuaParser)];
static uint8_t explTblRootMem[sizeof(LuaTable )];

static constexpr size_t CEG_PREFIX_STRLEN = sizeof(CEG_PREFIX_STRING) - 1;

CExplosionGeneratorHandler explGenHandler;



unsigned int CCustomExplosionGenerator::GetFlagsFromTable(const LuaTable& table)
{
	unsigned int flags = 0;

	flags |= (CEG_SPWF_GROUND     * table.GetBool(    "ground", false));
	flags |= (CEG_SPWF_WATER      * table.GetBool(    "water" , false));
	flags |= (CEG_SPWF_VOIDGROUND * table.GetBool("voidground", false));
	flags |= (CEG_SPWF_VOIDWATER  * table.GetBool("voidwater" , false));
	flags |= (CEG_SPWF_AIR        * table.GetBool(       "air", false));
	flags |= (CEG_SPWF_UNDERWATER * table.GetBool("underwater", false));
	flags |= (CEG_SPWF_UNIT       * table.GetBool(      "unit", false));
	flags |= (CEG_SPWF_NO_UNIT    * table.GetBool(    "nounit", false));

	return flags;
}

unsigned int CCustomExplosionGenerator::GetFlagsFromHeight(float height, float groundHeight)
{
	unsigned int flags = 0;

	const unsigned int voidGroundMask = 1 - mapRendering->voidGround;
	const unsigned int voidWaterMask  = 1 - mapRendering->voidWater ;

	const float waterDistance = math::fabsf(height);
	const float exploAltitude = height - groundHeight;

	constexpr float  waterLenienceDist =  5.0f;
	constexpr float groundLenienceDist = 20.0f;

	// note: ranges do not overlap, although code in
	// *ExplosionGenerator::Explosion assumes they can
	//
	// underground, don't spawn CEG at all
	if (exploAltitude < -groundLenienceDist)
		return flags;

	// above water, (void)ground or air
	if (height >= waterLenienceDist) {
		flags |= (CEG_SPWF_AIR        * (exploAltitude >= groundLenienceDist)                       );
		flags |= (CEG_SPWF_GROUND     * (exploAltitude <  groundLenienceDist) * (    voidGroundMask));
		flags |= (CEG_SPWF_VOIDGROUND * (exploAltitude <  groundLenienceDist) * (1 - voidGroundMask));
		return flags;
	}

	// ground (h > -2) or (void)water surface
	if (waterDistance < waterLenienceDist) {
		flags |= (CEG_SPWF_GROUND     * (groundHeight >  -2.0f) * (    voidGroundMask));
		flags |= (CEG_SPWF_VOIDGROUND * (groundHeight >  -2.0f) * (1 - voidGroundMask));
		flags |= (CEG_SPWF_WATER      * (groundHeight <= -2.0f) * (    voidWaterMask ));
		flags |= (CEG_SPWF_VOIDWATER  * (groundHeight <= -2.0f) * (1 - voidWaterMask ));
		return flags;
	}

	flags |= (CEG_SPWF_UNDERWATER     /* * (height <= -5.0f)*/ * (    voidWaterMask));
	// flags |= (CEG_SPWF_UNDERVOIDWATER /* * (height <= -5.0f)*/ * (1 - voidWaterMask));
	return flags;
}



void ClassAliasList::Load(const LuaTable& aliasTable)
{
	decltype(aliases) aliasList;
	aliasTable.GetMap(aliasList);
	aliases.insert(aliasList.begin(), aliasList.end());
}


std::string ClassAliasList::ResolveAlias(const std::string& name) const
{
	std::string n = name;

	for (;;) {
		const auto i = aliases.find(n);

		if (i == aliases.end())
			break;

		n = i->second;
	}

	return n;
}


std::string ClassAliasList::FindAlias(const std::string& className) const
{
	for (const auto& p: aliases) {
		if (p.second == className)
			return p.first;
	}

	return className;
}






void CExplosionGeneratorHandler::Init()
{
	egMemPool.clear();
	egMemPool.reserve(32);

	explosionGenerators.reserve(32);
	explosionGenerators.push_back(egMemPool.alloc<CStdExplosionGenerator>()); // id=0
	explosionGenerators[0]->SetGeneratorID(EXPGEN_ID_STANDARD);

	exploParser = nullptr;
	aliasParser = nullptr;
	explTblRoot = nullptr;

	ParseExplosionTables();
}

void CExplosionGeneratorHandler::Kill()
{
	spring::SafeDestruct(exploParser);
	spring::SafeDestruct(aliasParser);
	spring::SafeDestruct(explTblRoot);

	std::memset(exploParserMem, 0, sizeof(exploParserMem));
	std::memset(aliasParserMem, 0, sizeof(aliasParserMem));
	std::memset(explTblRootMem, 0, sizeof(explTblRootMem));

	for (IExplosionGenerator*& eg: explosionGenerators) {
		egMemPool.free(eg);
	}

	explosionGenerators.clear();

	expGenHashIdentMap.clear(); // never iterated
	expGenIdentNameMap.clear(); // never iterated
}

void CExplosionGeneratorHandler::ParseExplosionTables()
{
	static_assert(sizeof(LuaParser) <= sizeof(exploParserMem), "");
	static_assert(sizeof(LuaTable ) <= sizeof(explTblRootMem), "");

	spring::SafeDestruct(exploParser);
	spring::SafeDestruct(aliasParser);
	spring::SafeDestruct(explTblRoot);

	exploParser = new (exploParserMem) LuaParser("gamedata/explosions.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	aliasParser = new (aliasParserMem) LuaParser("gamedata/explosion_alias.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	explTblRoot = nullptr;

	if (!aliasParser->Execute()) {
		LOG_L(L_ERROR, "[%s] failed to parse explosion aliases: %s", __func__, (aliasParser->GetErrorLog()).c_str());
	} else {
		const LuaTable& aliasRoot = aliasParser->GetRoot();

		projectileClasses.Clear();
		projectileClasses.Load(aliasRoot.SubTable("projectiles"));
	}

	if (!exploParser->Execute()) {
		LOG_L(L_ERROR, "[%s] failed to parse explosions: %s", __func__, (exploParser->GetErrorLog()).c_str());
	} else {
		explTblRoot = new (explTblRootMem) LuaTable(exploParser->GetRoot());
	}
}

void CExplosionGeneratorHandler::ReloadGenerators(const std::string& tag) {
	// re-parse the projectile and generator tables
	ParseExplosionTables();

	const char* preFmt = "[%s][generatorID=%u] reloading CEG \"%s\"";
	const char* pstFmt = "[%s][generatorID=%u] failed to reload CEG \"%s\"";

	// NOTE:
	//   maps store tags inclusive of CEG_PREFIX_STRING
	//   but the Lua subtables that define each CEG are
	//   only indexed by tag postfix
	if (!tag.empty()) {
		const auto it = expGenHashIdentMap.find(hashString(tag.c_str()));

		if (it == expGenHashIdentMap.end()) {
			LOG_L(L_WARNING, "[%s] no CEG named \"%s\" (forgot the \"%s\" prefix?)", __func__, tag.c_str(), CEG_PREFIX_STRING);
			return;
		}

		assert(explosionGenerators[it->second]->GetGeneratorID() == it->second);
		LOG(preFmt, __func__, it->second, tag.c_str());

		if (!explosionGenerators[it->second]->Reload(this, tag.c_str() + CEG_PREFIX_STRLEN))
			LOG_L(L_WARNING, pstFmt, __func__, it->second, tag.c_str());

		return;
	}

	for (unsigned int n = 1; n < explosionGenerators.size(); n++) {
		IExplosionGenerator* eg = explosionGenerators[n];

		// standard EG's (empty postfix) do not need to be reloaded
		if (expGenIdentNameMap.find(n) == expGenIdentNameMap.end())
			continue;

		assert(eg->GetGeneratorID() == n);
		LOG(preFmt, __func__, n, expGenIdentNameMap[n].data());

		if (eg->Reload(this, expGenIdentNameMap[n].data() + CEG_PREFIX_STRLEN))
			continue;

		LOG_L(L_WARNING, pstFmt, __func__, n, expGenIdentNameMap[n]);
	}
}



unsigned int CExplosionGeneratorHandler::LoadGeneratorID(const char* tag, const char* pre)
{
	IExplosionGenerator* eg = LoadGenerator(tag, pre);

	if (eg == nullptr)
		return EXPGEN_ID_INVALID;

	return (eg->GetGeneratorID());
}

// creates either a standard or a custom explosion generator instance
// NOTE:
//   can be called recursively for custom instances (LoadGenerator ->
//   Load -> ParseExplosionCode -> LoadGenerator -> ...), generators
//   must NOT be overwritten
IExplosionGenerator* CExplosionGeneratorHandler::LoadGenerator(const char* tag, const char* pre)
{
	decltype(expGenIdentNameMap)::mapped_type key = {{0}};

	char* ptr = key.data();
	char* sep = nullptr;

	ptr += snprintf(ptr, sizeof(key) - (ptr - key.data()), "%s", pre);
	ptr += snprintf(ptr, sizeof(key) - (ptr - key.data()), "%s", tag);

	const auto hash = hashString(key.data());
	const auto iter = expGenHashIdentMap.find(hash);

	if (iter != expGenHashIdentMap.end())
		return explosionGenerators[iter->second];

	// tag is either "CStdExplosionGenerator" (or some sub-string, eg.
	// "std") which maps to CStdExplosionGenerator or "custom:postfix"
	// which maps to CCustomExplosionGenerator, all others cause NULL
	// to be returned
	IExplosionGenerator* explGen = nullptr;

	if ((sep = strstr(key.data(), ":")) != nullptr) {
		// grab the "custom" prefix (the only supported value)
		explGen = egMemPool.alloc<CCustomExplosionGenerator>();
	} else {
		explGen = egMemPool.alloc<CStdExplosionGenerator>();
	}

	explGen->SetGeneratorID(explosionGenerators.size());

	// can still be a standard generator, but with non-zero ID
	assert(explGen->GetGeneratorID() != EXPGEN_ID_STANDARD);

	// save generator so ID is valid *before* possible recursion
	explosionGenerators.push_back(explGen);

	if (sep != nullptr) {
		// standard EG's have no postfix (nor always a prefix)
		// custom EG's always have CEG_PREFIX_STRING in front
		expGenHashIdentMap.insert(hash, explGen->GetGeneratorID());
		expGenIdentNameMap.insert(explGen->GetGeneratorID(), key);

		explGen->Load(this, sep + 1);
	}

	return explGen;
}

IExplosionGenerator* CExplosionGeneratorHandler::GetGenerator(unsigned int expGenID)
{
	if (expGenID == EXPGEN_ID_INVALID)
		return nullptr;
	if (expGenID >= explosionGenerators.size())
		return nullptr;

	return explosionGenerators[expGenID];
}

bool CExplosionGeneratorHandler::GenExplosion(
	unsigned int expGenID,
	const float3& pos,
	const float3& dir,
	float damage,
	float radius,
	float gfxMod,
	CUnit* owner,
	CUnit* hit
) {
	IExplosionGenerator* expGen = GetGenerator(expGenID);

	if (expGen == nullptr)
		return false;

	return (expGen->Explosion(pos, dir, damage, radius, gfxMod, owner, hit));
}



bool CStdExplosionGenerator::Explosion(
	const float3& pos,
	const float3& dir,
	float damage,
	float radius,
	float gfxMod,
	CUnit* owner,
	CUnit* hit
) {
	const float groundHeight = CGround::GetHeightReal(pos.x, pos.z);
	const float altitude = pos.y - groundHeight;

	float3 camVect = camera->GetPos() - pos;

	const unsigned int flags = CCustomExplosionGenerator::GetFlagsFromHeight(pos.y, groundHeight);
	const bool    airExplosion = ((flags & CCustomExplosionGenerator::CEG_SPWF_AIR       ) != 0);
	const bool groundExplosion = ((flags & CCustomExplosionGenerator::CEG_SPWF_GROUND    ) != 0);
	const bool  waterExplosion = ((flags & CCustomExplosionGenerator::CEG_SPWF_WATER     ) != 0);
	const bool     uwExplosion = ((flags & CCustomExplosionGenerator::CEG_SPWF_UNDERWATER) != 0);

	// limit the visual effects based on the radius
	damage = std::min(damage * 0.05f, radius * 1.5f);
	damage = std::max(damage * gfxMod, 0.0f);

	const float sqrtDmg = math::sqrt(damage);
	const float camLength = camVect.LengthNormalize();
	float moveLength = radius * 0.03f;

	if (camLength < (moveLength + 2.0f))
		moveLength = camLength - 2.0f;

	const float3 npos = pos + camVect * moveLength;

	projMemPool.alloc<CHeatCloudProjectile>(owner, npos, UpVector * 0.3f, 8.0f + sqrtDmg * 0.5f, 7 + damage * 2.8f);

	if (projectileHandler.GetParticleSaturation() < 1.0f) {
		// turn off lots of graphic only particles when we have more particles than we want
		float smokeDamage      = damage;
		float smokeDamageSQRT  = 0.0f;
		float smokeDamageISQRT = 0.0f;

		smokeDamage *= (1.0f - (                   uwExplosion) * 0.7f);
		smokeDamage *= (1.0f - (airExplosion || waterExplosion) * 0.4f);

		if (smokeDamage > 0.01f) {
			smokeDamageSQRT = math::sqrt(smokeDamage);
			smokeDamageISQRT = 1.0f / (smokeDamageSQRT * 0.35f);
		}

		for (int a = 0, n = smokeDamage * 0.6f; a < n; ++a) {
			const float3 speed(
				(-0.1f + guRNG.NextFloat() * 0.2f),
				( 0.1f + guRNG.NextFloat() * 0.3f) * smokeDamageISQRT,
				(-0.1f + guRNG.NextFloat() * 0.2f)
			);

			// smoke always goes up
			float3 dir = guRNG.NextVector() * smokeDamage;
			dir.y = math::fabsf(dir.y);

			const float time = 40.0f + smokeDamageSQRT * 15.0f;
			const float mult = 0.8f + guRNG.NextFloat() * 0.7f;

			projMemPool.alloc<CSmokeProjectile2>(owner, pos, pos + dir, speed, time * mult, smokeDamageSQRT * 4.0f, 0.4f, 0.6f);
		}

		if (groundExplosion) {
			constexpr float3 color(0.15f, 0.1f, 0.05f);

			const float explSpeedMod = 0.7f + std::min(30.0f, damage) / GAME_SPEED;

			for (int a = 0, n = std::min(20.0f, damage * 0.8f); a < n; ++a) {
				const float3 explSpeed = float3(
					(0.5f - guRNG.NextFloat()) * 1.5f,
					 1.7f + guRNG.NextFloat()  * 1.6f,
					(0.5f - guRNG.NextFloat()) * 1.5f
				);

				const float3 npos(
					pos.x - (0.5f - guRNG.NextFloat()) * (radius * 0.6f),
					pos.y -  2.0f - damage * 0.2f,
					pos.z - (0.5f - guRNG.NextFloat()) * (radius * 0.6f)
				);

				projMemPool.alloc<CDirtProjectile>(owner, npos, explSpeed  * explSpeedMod, 90.0f + damage * 2.0f, 2.0f + sqrtDmg * 1.5f, 0.4f, 0.999f, color);
			}
		}

		if (!airExplosion && !uwExplosion && waterExplosion) {
			constexpr float3 color(1.0f, 1.0f, 1.0f);

			for (int a = 0, n = std::min(40.0f, damage * 0.8f); a < n; ++a) {
				const float3 speed(
					(    0.5f - guRNG.NextFloat()) * 0.2f,
					(a * 0.1f + guRNG.NextFloat()  * 0.8f),
					(    0.5f - guRNG.NextFloat()) * 0.2f
				);
				const float3 npos(
					pos.x - (0.5f - guRNG.NextFloat()) * (radius * 0.2f),
					pos.y -  2.0f - sqrtDmg          *           2.0f,
					pos.z - (0.5f - guRNG.NextFloat()) * (radius * 0.2f)
				);

				projMemPool.alloc<CDirtProjectile>(
					owner,
					npos,
					speed * (0.7f + std::min(30.0f, damage) / GAME_SPEED),
					90.0f + damage * 2.0f,
					2.0f + sqrtDmg * 2.0f,
					0.3f,
					0.99f,
					color
				);
			}
		}
		if (damage >= 20.0f && !uwExplosion && !airExplosion) {
			const float explSpeedMod = (0.7f + std::min(30.0f, damage) / 23);

			for (int a = 0, n = (guRNG.NextInt(6)) + 3 + int(damage * 0.04f); a < n; ++a) {
				const float3 explSpeed = (altitude < 4.0f)?
					float3((0.5f - guRNG.NextFloat()) * 2.0f, 1.8f + guRNG.NextFloat() * 1.8f, (0.5f - guRNG.NextFloat()) * 2.0f):
					float3(guRNG.NextVector() * 2);

				const float3 npos(
					pos.x - (0.5f - guRNG.NextFloat()) * (radius * 1),
					pos.y,
					pos.z - (0.5f - guRNG.NextFloat()) * (radius * 1)
				);

				projMemPool.alloc<CWreckProjectile>(owner, npos, explSpeed * explSpeedMod, 90.0f + damage * 2.0f);
			}
		}
		if (uwExplosion) {
			for (int a = 0, n = (damage * 0.7f); a < n; ++a) {
				projMemPool.alloc<CBubbleProjectile>(
					owner,
					pos + guRNG.NextVector() * radius * 0.5f,
					guRNG.NextVector() * 0.2f + float3(0.0f, 0.2f, 0.0f),
					damage * 2.0f + guRNG.NextFloat() * damage,
					1.0f + guRNG.NextFloat() * 2.0f,
					0.02f,
					0.5f + guRNG.NextFloat() * 0.3f
				);
			}
		}
		if (waterExplosion && !uwExplosion && !airExplosion) {
			for (int a = 0, n = (damage * 0.5f); a < n; ++a) {
				projMemPool.alloc<CWakeProjectile>(
					owner,
					pos + guRNG.NextVector() * radius * 0.2f,
					guRNG.NextVector() * radius * 0.003f,
					sqrtDmg * 4.0f,
					damage * 0.03f,
					0.3f + guRNG.NextFloat() * 0.2f,
					0.8f / (sqrtDmg * 3 + 50 + guRNG.NextFloat() * 90.0f),
					1
				);
			}
		}
		if (radius > 10.0f && damage > 4.0f) {
			const float explSpeedMod = (8 + damage * 3.0f) / (9 + sqrtDmg * 0.7f) * 0.35f;

			for (int a = 0, n = int(sqrtDmg) + 8; a < n; ++a) {
				float3 explSpeed = (guRNG.NextVector()).SafeNormalize() * explSpeedMod;

				if (!airExplosion && !waterExplosion && (explSpeed.y < 0.0f))
					explSpeed.y = -explSpeed.y;

				projMemPool.alloc<CExploSpikeProjectile>(
					owner,
					pos + explSpeed,
					explSpeed * (0.9f + guRNG.NextFloat() * 0.4f),
					radius * 0.1f,
					radius * 0.1f,
					0.6f,
					0.8f / (8.0f + sqrtDmg)
				);
			}
		}
	}

	if (radius > 20.0f && damage > 6.0f && altitude < (radius * 0.7f)) {
		const float flashSize = std::max(radius, damage * 2);
		const float ttl = 8 + sqrtDmg * 0.8f;

		if (flashSize > 5.f && ttl > 15.f) {
			const float flashAlpha = std::min(0.8f, damage * 0.01f);

			float circleAlpha = 0.0f;
			float circleGrowth = 0.0f;

			if (radius > 40.0f && damage > 12.0f) {
				circleAlpha = std::min(0.5f, damage * 0.01f);
				circleGrowth = (8.0f + damage * 2.5f) / (9.0f + sqrtDmg * 0.7f) * 0.55f;
			}

			projMemPool.alloc<CStandardGroundFlash>(pos, circleAlpha, flashAlpha, flashSize, circleGrowth, ttl);
		}
	}

	if (radius > 40.0f && damage > 12.0f) {
		CSpherePartProjectile::CreateSphere(
			owner,
			5.0f + int(sqrtDmg * 0.7f),
			std::min(0.7f, damage * 0.02f),
			(8.0f + damage * 2.5f) / (9.0f + sqrtDmg * 0.7f) * 0.5f,
			pos
		);
	}

	return true;
}



void CCustomExplosionGenerator::ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3& dir)
{
	float val = 0.0f;
	float buffer[16];
	void* ptr = nullptr;

	std::memset(&buffer[0], 0, sizeof(buffer));

	for (;;) {
		switch (*(code++)) {
			case OP_END: {
				return;
			}
			case OP_STOREI: {
				std::uint8_t  size   = *(std::uint8_t*)  code; code++;
				std::uint16_t offset = *(std::uint16_t*) code; code += 2;
				switch (size) {
					case 1: { *(std::int8_t*)  (instance + offset) = (int) val; } break;
					case 2: { *(std::int16_t*) (instance + offset) = (int) val; } break;
					case 4: { *(std::int32_t*) (instance + offset) = (int) val; } break;
					case 8: { *(std::int64_t*) (instance + offset) = (int) val; } break;
					default: { /*no op*/ } break;
				}
				val = 0.0f;
				break;
			}
			case OP_STOREF: {
				std::uint8_t  size   = *(std::uint8_t*)  code; code++;
				std::uint16_t offset = *(std::uint16_t*) code; code += 2;
				switch (size) {
					case 4: { *(float*)  (instance + offset) = val; } break;
					case 8: { *(double*) (instance + offset) = val; } break;
					default: { /*no op*/ } break;
				}
				val = 0.0f;
				break;
			}
			case OP_ADD: {
				val += *(float*) code;
				code += 4;
				break;
			}
			case OP_RAND: {
				val += guRNG.NextFloat() * (*(float*) code);
				code += 4;
				break;
			}
			case OP_DAMAGE: {
				val += damage * (*(float*) code);
				code += 4;
				break;
			}
			case OP_INDEX: {
				val += spawnIndex * (*(float*) code);
				code += 4;
				break;
			}

			case OP_LOADP: {
				ptr = *(void**) code;
				code += sizeof(void*);
				break;
			}
			case OP_STOREP: {
				std::uint16_t offset = *(std::uint16_t*) code;
				code += 2;
				*(void**) (instance + offset) = ptr;
				ptr = nullptr;
				break;
			}

			case OP_DIR: {
				std::uint16_t offset = *(std::uint16_t*) code;
				code += 2;
				*reinterpret_cast<float3*>(instance + offset) = dir;
				break;
			}
			case OP_SAWTOOTH: {
				// this translates to modulo except it works with floats
				val -= (*(float*) code) * math::floor(val / (*(float*) code));
				code += 4;
				break;
			}
			case OP_DISCRETE: {
				val = (*(float*) code) * math::floor(spring::SafeDivide(val, (*(float*) code)));
				code += 4;
				break;
			}
			case OP_SINE: {
				val = (*(float*) code) * math::sin(val);
				code += 4;
				break;
			}
			case OP_YANK: {
				buffer[(*(int*) code)] = val;
				val = 0;
				code += 4;
				break;
			}
			case OP_MULTIPLY: {
				val *= buffer[(*(int*) code)];
				code += 4;
				break;
			}
			case OP_ADDBUFF: {
				val += buffer[(*(int*) code)];
				code += 4;
				break;
			}
			case OP_POW: {
				val = math::pow(val, (*(float*) code));
				code += 4;
				break;
			}
			case OP_POWBUFF: {
				val = math::pow(val, buffer[(*(int*) code)]);
				code += 4;
				break;
			}
			default: {
				assert(false);
				break;
			}
		}
	}
}



void CCustomExplosionGenerator::ParseExplosionCode(
	CCustomExplosionGenerator::ProjectileSpawnInfo* psi,
	const string& script,
	SExpGenSpawnableMemberInfo& memberInfo,
	string& code
) {
	const std::string content = script.substr(0, script.find(';', 0));

	const bool isFloat = (memberInfo.type == SExpGenSpawnableMemberInfo::TYPE_FLOAT);
	const bool isInt   = (memberInfo.type == SExpGenSpawnableMemberInfo::TYPE_INT  );

	if (content == "dir") {
		// dir keyword; type has to be float3
		if (!isFloat || memberInfo.length < 3)
			throw content_error("[CCEG::ParseExplosionCode] incorrect use of \"dir\" (" + script + ")");

		const std::uint16_t ofs = memberInfo.offset;

		code.append(1, OP_DIR);
		code.append((char*) &ofs, (char*) &ofs + sizeof(ofs));
		return;
	}

	// arrays (float3 or float4)
	if (memberInfo.length > 1) {
		string::size_type start = 0;
		SExpGenSpawnableMemberInfo subInfo = memberInfo;
		subInfo.length = 1;

		for (unsigned int i = 0; i < memberInfo.length && start < script.length(); ++i) {
			const string::size_type subEnd = script.find(',', start + 1);

			ParseExplosionCode(psi, script.substr(start, subEnd - start), subInfo, code);

			start = subEnd + 1;
			subInfo.offset += subInfo.size;
		}

		return;
	}

	// textures, colormaps, etc.
	if (memberInfo.type == SExpGenSpawnableMemberInfo::TYPE_PTR) {
		// Memory is managed by whomever this callback belongs to
		void* ptr = memberInfo.ptrCallback(content);

		code.append(1, OP_LOADP);
		code.append((char*)(&ptr), ((char*)(&ptr)) + sizeof(void*));

		const std::uint16_t ofs = memberInfo.offset;

		code.append(1, OP_STOREP);
		code.append((char*)&ofs, (char*)&ofs + sizeof(ofs));
		return;
	}


	// Floats or Ints
	assert(isFloat || isInt);

	if (isFloat) {
		switch (memberInfo.size) {
			case 4: {} break;
			default: { throw content_error("[CCEG::ParseExplosionCode] incompatible float size \"" + IntToString(memberInfo.size) + "\" (" + script + ")"); } break;
		}
	} else {
		switch (memberInfo.size) {
			case 1: case 2: case 4: {} break;
			default: { throw content_error("[CCEG::ParseExplosionCode] incompatible integer size \"" + IntToString(memberInfo.size) + "\" (" + script + ")"); } break;
		}
	}

	// parse the code
	for (size_t p = 0, len = script.length(); p < len; ) {
		char opcode = OP_END;
		char c = script[p++];

		// consume whitespace
		if (c == ' ')
			continue;

		bool useInt = false;

		     if (c == 'i')   opcode = OP_INDEX;
		else if (c == 'r')   opcode = OP_RAND;
		else if (c == 'd')   opcode = OP_DAMAGE;
		else if (c == 'm')   opcode = OP_SAWTOOTH;
		else if (c == 'k')   opcode = OP_DISCRETE;
		else if (c == 's')   opcode = OP_SINE;
		else if (c == 'p')   opcode = OP_POW;
		else if (c == 'y') { opcode = OP_YANK;     useInt = true; }
		else if (c == 'x') { opcode = OP_MULTIPLY; useInt = true; }
		else if (c == 'a') { opcode = OP_ADDBUFF;  useInt = true; }
		else if (c == 'q') { opcode = OP_POWBUFF;  useInt = true; }
		else if (isdigit(c) || c == '.' || c == '-') { opcode = OP_ADD; p--; }
		else {
			LOG_L(L_WARNING, "[CCEG::%s] unknown op-code \"%c\" in \"%s\" at index " _STPF_ "", __func__, c, script.c_str(), p);
			continue;
		}

		// be sure to exit cleanly if there are no more operators or operands
		if (p >= script.size())
			continue;

		char* endp = nullptr;

		if (!useInt) {
			const float v = (float)strtod(&script[p], &endp);

			p += (endp - &script[p]);

			code.append(1, opcode);
			code.append((char*) &v, ((char*) &v) + sizeof(v));
		} else {
			const int v = Clamp(int(strtol(&script[p], &endp, 10)), 0, 16);

			p += (endp - &script[p]);

			code.append(1, opcode);
			code.append((char*) &v, ((char*) &v) + sizeof(v));
		}
	}

	// store the final value
	const std::uint16_t ofs = memberInfo.offset;

	code.push_back(isFloat ? OP_STOREF : OP_STOREI);
	code.push_back(memberInfo.size);
	code.append((char*)&ofs, (char*)&ofs + sizeof(ofs));
}



bool CCustomExplosionGenerator::Load(CExplosionGeneratorHandler* handler, const char* tag)
{
	const LuaTable* root = handler->GetExplosionTableRoot();
	const LuaTable& expTable = (root != nullptr)? root->SubTable(tag): LuaTable();

	if (!expTable.IsValid()) {
		// not a fatal error: any calls to Explosion will just return early
		LOG_L(L_WARNING, "[CCEG::%s] table for CEG \"%s\" invalid (parse errors?)", __func__, tag);
		return false;
	}

	expGenParams.projectiles.clear();
	expGenParams.groundFlash = GroundFlashInfo();

	vector<string> spawns;
	expTable.GetKeys(spawns);

	for (unsigned int n = 0; n < spawns.size(); n++) {
		ProjectileSpawnInfo psi;

		const std::string& spawnName = spawns[n];
		const LuaTable& spawnTable = expTable.SubTable(spawnName);

		// NOTE:
		//   *every* CEG table contains a spawn called "filename"
		//   see springcontent/gamedata/explosions.lua::LoadTDFs
		if (!spawnTable.IsValid())
			continue;
		if (spawnName == "groundflash" || spawnName == "filename")
			continue;

		const string& className = handler->GetProjectileClasses().ResolveAlias(spawnTable.GetString("class", spawnName));

		if ((psi.spawnableID = CExpGenSpawnable::GetSpawnableID(className)) == -1u) {
			LOG_L(L_WARNING, "[CCEG::%s] %s: unknown class \"%s\"", __func__, tag, className.c_str());
			continue;
		}

		psi.flags = GetFlagsFromTable(spawnTable);
		psi.count = std::max(0, spawnTable.GetInt("count", 1));

		std::string code;
		spring::unordered_map<string, string> props;

		spawnTable.SubTable("properties").GetMap(props);

		for (const auto& propIt: props) {
			SExpGenSpawnableMemberInfo memberInfo = {0, 0, 0, STRING_HASH(std::move(StringToLower(propIt.first))), SExpGenSpawnableMemberInfo::TYPE_INT, nullptr};

			if (CExpGenSpawnable::GetSpawnableMemberInfo(className, memberInfo)) {
				ParseExplosionCode(&psi, propIt.second, memberInfo, code);
			} else {
				LOG_L(L_WARNING, "[CCEG::%s] unknown field %s::%s in spawn-table \"%s\" for CEG \"%s\"", __func__, tag, propIt.first.c_str(), spawnName.c_str(), className.c_str());
			}
		}

		code += (char)OP_END;
		psi.code.resize(code.size());
		copy(code.begin(), code.end(), psi.code.begin());

		expGenParams.projectiles.push_back(psi);
	}

	const LuaTable gndTable = expTable.SubTable("groundflash");
	const int ttl = gndTable.GetInt("ttl", 0);

	if (ttl > 0) {
		expGenParams.groundFlash.circleAlpha  = gndTable.GetFloat("circleAlpha",  0.0f);
		expGenParams.groundFlash.flashSize    = gndTable.GetFloat("flashSize",    0.0f);
		expGenParams.groundFlash.flashAlpha   = gndTable.GetFloat("flashAlpha",   0.0f);
		expGenParams.groundFlash.circleGrowth = gndTable.GetFloat("circleGrowth", 0.0f);
		expGenParams.groundFlash.color        = gndTable.GetFloat3("color", float3(1.0f, 1.0f, 0.8f));

		expGenParams.groundFlash.flags = CEG_SPWF_GROUND | GetFlagsFromTable(gndTable);
		expGenParams.groundFlash.ttl = ttl;
	}

	expGenParams.useDefaultExplosions = expTable.GetBool("useDefaultExplosions", false);
	return true;
}

bool CCustomExplosionGenerator::Reload(CExplosionGeneratorHandler* handler, const char* tag) {
	const ExpGenParams oldParams = expGenParams;

	if (!Load(handler, tag)) {
		expGenParams = oldParams;
		return false;
	}

	return true;
}

bool CCustomExplosionGenerator::Explosion(
	const float3& pos,
	const float3& dir,
	float damage,
	float radius,
	float gfxMod,
	CUnit* owner,
	CUnit* hit
) {
	unsigned int flags = GetFlagsFromHeight(pos.y, CGround::GetHeightReal(pos.x, pos.z));

	const bool   unitCollision = (hit != nullptr);
	const bool groundExplosion = ((flags & CEG_SPWF_GROUND) != 0);

	flags |= (CEG_SPWF_UNIT    * (    unitCollision));
	flags |= (CEG_SPWF_NO_UNIT * (1 - unitCollision));

	const std::vector<ProjectileSpawnInfo>& spawnInfo = expGenParams.projectiles;
	const GroundFlashInfo& groundFlash = expGenParams.groundFlash;

	for (int a = 0; a < spawnInfo.size(); a++) {
		const ProjectileSpawnInfo& psi = spawnInfo[a];

		// spawn projectiles only if at least one bit matches
		if ((psi.flags & flags) == 0)
			continue;

		// no new projectiles if we're saturated
		if (projectileHandler.GetParticleSaturation() > 1.0f)
			break;

		for (unsigned int c = 0; c < psi.count; c++) {
			CExpGenSpawnable* projectile = CExpGenSpawnable::CreateSpawnable(psi.spawnableID);
			ExecuteExplosionCode(&psi.code[0], damage, (char*) projectile, c, dir);
			projectile->Init(owner, pos);
		}
	}

	if (groundExplosion && (groundFlash.ttl > 0) && (groundFlash.flashSize > 1))
		projMemPool.alloc<CStandardGroundFlash>(pos, groundFlash);

	if (expGenParams.useDefaultExplosions)
		return (explGenHandler.GenExplosion(CExplosionGeneratorHandler::EXPGEN_ID_STANDARD, pos, dir, damage, radius, gfxMod, owner, hit));

	return true;
}


bool CCustomExplosionGenerator::OutputProjectileClassInfo()
{
#ifdef USING_CREG
	LOG_DISABLE();
		// we need to load basecontent for class aliases
		FileSystemInitializer::Initialize();
		vfsHandler->AddArchiveWithDeps(archiveScanner->ArchiveFromName(CArchiveScanner::GetSpringBaseContentName()), false);
	LOG_ENABLE();

	const vector<creg::Class*>& classes = creg::System::GetClasses();
	CExplosionGeneratorHandler egh;

	std::cout << "{" << std::endl;

	for (creg::Class* c: classes) {
		if (!c->IsSubclassOf(CExpGenSpawnable::StaticClass()) || c == CExpGenSpawnable::StaticClass())
			continue;

		if (c->flags & creg::CF_Synced)
			continue;

		if (c != classes.front())
			std::cout << "," << std::endl;

		std::cout << "  \"" << c->name << "\": {" << std::endl;
		std::cout << "    \"alias\": \"" << (egh.GetProjectileClasses()).FindAlias(c->name) << "\"";
		for (creg::Class* cb = c; cb; cb = cb->base()) {
			for (creg::Class::Member& m: cb->members) {
				if (m.flags & creg::CM_Config) {
					std::cout << "," << std::endl;
					std::cout << "    \"" << m.name << "\": \"" << m.type->GetName() << "\"";
				}
			}
		}

		std::cout << std::endl << "  }";
	}
	std::cout << std::endl << "}" << std::endl;

	FileSystemInitializer::Cleanup();
	return true;
#else //USING_CREG
	std::cout << "Unable to output CEG info since creg is disabled" << std::endl;
	return false;
#endif
}

