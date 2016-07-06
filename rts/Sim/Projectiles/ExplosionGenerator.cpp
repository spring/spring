/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <boost/cstdint.hpp>

#include "ExplosionGenerator.h"
#include "ExpGenSpawner.h" //!!
#include "ExpGenSpawnable.h"
#include "ExpGenSpawnableMemberInfo.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/DirtProjectile.h"
#include "Rendering/Env/Particles/Classes/ExploSpikeProjectile.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile2.h"
#include "Rendering/Env/Particles/Classes/SpherePartProjectile.h"
#include "Rendering/Env/Particles/Classes/WakeProjectile.h"
#include "Rendering/Env/Particles/Classes/WreckProjectile.h"

#include "System/creg/STL_Map.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Util.h"
#include "System/Sync/HsiehHash.h"


CR_BIND_INTERFACE(IExplosionGenerator)
CR_REG_METADATA(IExplosionGenerator, (
	CR_MEMBER(generatorID)
))

CR_BIND_DERIVED(CStdExplosionGenerator, IExplosionGenerator, )
CR_REG_METADATA(CStdExplosionGenerator, )

CR_BIND(CCustomExplosionGenerator::ProjectileSpawnInfo, )
CR_REG_METADATA_SUB(CCustomExplosionGenerator, ProjectileSpawnInfo, (
	CR_MEMBER(spawnableID),
	CR_MEMBER(code),
	CR_MEMBER(count),
	CR_MEMBER(flags)
))

CR_BIND(GroundFlashInfo, )
CR_REG_METADATA(GroundFlashInfo, (
	CR_MEMBER(flashSize),
	CR_MEMBER(flashAlpha),
	CR_MEMBER(circleGrowth),
	CR_MEMBER(circleAlpha),
	CR_MEMBER(ttl),
	CR_MEMBER(flags),
	CR_MEMBER(color)
))

CR_BIND(CCustomExplosionGenerator::ExpGenParams, )
CR_REG_METADATA_SUB(CCustomExplosionGenerator, ExpGenParams, (
	CR_MEMBER(projectiles),
	CR_MEMBER(groundFlash),
	CR_MEMBER(useDefaultExplosions)
))

CR_BIND_DERIVED(CCustomExplosionGenerator, CStdExplosionGenerator, )
CR_REG_METADATA(CCustomExplosionGenerator, (
	CR_MEMBER(expGenParams)
))


CExplosionGeneratorHandler* explGenHandler = NULL;



unsigned int CCustomExplosionGenerator::GetFlagsFromTable(const LuaTable& table)
{
	unsigned int flags = 0;

	if (table.GetBool("ground",     false)) { flags |= CCustomExplosionGenerator::SPW_GROUND;     }
	if (table.GetBool("water",      false)) { flags |= CCustomExplosionGenerator::SPW_WATER;      }
	if (table.GetBool("air",        false)) { flags |= CCustomExplosionGenerator::SPW_AIR;        }
	if (table.GetBool("underwater", false)) { flags |= CCustomExplosionGenerator::SPW_UNDERWATER; }
	if (table.GetBool("unit",       false)) { flags |= CCustomExplosionGenerator::SPW_UNIT;       }
	if (table.GetBool("nounit",     false)) { flags |= CCustomExplosionGenerator::SPW_NO_UNIT;    }

	return flags;
}

unsigned int CCustomExplosionGenerator::GetFlagsFromHeight(float height, float groundHeight)
{
	unsigned int flags = 0;

	const float waterDist = std::abs(height);
	const float altitude  = height - groundHeight;

	// note: ranges do not overlap, although code in
	// *ExplosionGenerator::Explosion assumes they can
	if (altitude  < -1.0f) {
		/* underground! don't spawn CEG! */
	} else
	if (height   >= 5.0f) { // above water
		if (altitude >= 20.0f) {
			flags |= CCustomExplosionGenerator::SPW_AIR;    // air
		} else {
			flags |= CCustomExplosionGenerator::SPW_GROUND; // ground
		}
	} else
	if (waterDist < 5.0f) { // water surface
		if (groundHeight > -2.0f) {
			flags |= CCustomExplosionGenerator::SPW_GROUND; // shallow water (use ground fx)
		} else {
			flags |= CCustomExplosionGenerator::SPW_WATER;  // water (surface)
		}
	} else
	/*if (height <= -5.0f) */ { // under water
		flags |= CCustomExplosionGenerator::SPW_UNDERWATER;     // underwater
	}

	return flags;
}



void ClassAliasList::Load(const LuaTable& aliasTable)
{
	map<string, string> aliasList;
	aliasTable.GetMap(aliasList);
	aliases.insert(aliasList.begin(), aliasList.end());
}


std::string ClassAliasList::ResolveAlias(const string& name) const
{
	string n = name;

	for (;;) {
		map<string, string>::const_iterator i = aliases.find(n);
		if (i == aliases.end()) {
			break;
		}
		n = i->second;
	}

	return n;
}


string ClassAliasList::FindAlias(const string& className) const
{
	for (map<string, string>::const_iterator i = aliases.begin(); i != aliases.end(); ++i) {
		if (i->second == className) {
			return i->first;
		}
	}

	return className;
}






CExplosionGeneratorHandler::CExplosionGeneratorHandler()
{
	explosionGenerators.reserve(32);
	explosionGenerators.push_back(new CStdExplosionGenerator()); // id=0
	explosionGenerators[0]->SetGeneratorID(EXPGEN_ID_STANDARD);

	exploParser = nullptr;
	aliasParser = nullptr;
	explTblRoot = nullptr;

	ParseExplosionTables();
}

CExplosionGeneratorHandler::~CExplosionGeneratorHandler()
{
	SafeDelete(exploParser);
	SafeDelete(aliasParser);
	SafeDelete(explTblRoot);

	// delete CStdExplGen
	SafeDelete(explosionGenerators[0]);

	for (unsigned int n = 1; n < explosionGenerators.size(); n++) {
		SafeDelete(explosionGenerators[n]);
	}

	explosionGenerators.clear();

	expGenTagIdentMap.clear();
	expGenIdentTagMap.clear();
}

void CExplosionGeneratorHandler::ParseExplosionTables() {
	delete exploParser;
	delete aliasParser;
	delete explTblRoot;

	exploParser = new LuaParser("gamedata/explosions.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	aliasParser = new LuaParser("gamedata/explosion_alias.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	explTblRoot = NULL;

	if (!aliasParser->Execute()) {
		LOG_L(L_ERROR, "Failed to parse explosion aliases: %s",
				aliasParser->GetErrorLog().c_str());
	} else {
		const LuaTable& aliasRoot = aliasParser->GetRoot();

		projectileClasses.Clear();
		projectileClasses.Load(aliasRoot.SubTable("projectiles"));
	}

	if (!exploParser->Execute()) {
		LOG_L(L_ERROR, "Failed to parse explosions: %s", exploParser->GetErrorLog().c_str());
	} else {
		explTblRoot = new LuaTable(exploParser->GetRoot());
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
	if (tag.empty()) {
		for (unsigned int n = 1; n < explosionGenerators.size(); n++) {
			IExplosionGenerator* eg = explosionGenerators[n];

			// standard EG's (empty postfix) do not need to be reloaded
			if (expGenIdentTagMap.find(n) == expGenIdentTagMap.end())
				continue;

			assert(eg->GetGeneratorID() == n);
			LOG(preFmt, __FUNCTION__, n, expGenIdentTagMap[n].c_str());

			if (!eg->Reload(this, expGenIdentTagMap[n].substr(7))) {
				LOG_L(L_WARNING, pstFmt, __FUNCTION__, n, expGenIdentTagMap[n].c_str());
			}
		}
	} else {
		const TagIdentMapConstIt it = expGenTagIdentMap.find(tag);

		if (it == expGenTagIdentMap.end()) {
			LOG_L(L_WARNING, "[%s] no CEG named \"%s\" (forgot the \"%s\" prefix?)", __FUNCTION__, tag.c_str(), CEG_PREFIX_STRING);
			return;
		}

		assert(explosionGenerators[it->second]->GetGeneratorID() == it->second);
		LOG(preFmt, __FUNCTION__, it->second, tag.c_str());

		if (!explosionGenerators[it->second]->Reload(this, tag.substr(7))) {
			LOG_L(L_WARNING, pstFmt, __FUNCTION__, it->second, tag.c_str());
		}
	}
}



unsigned int CExplosionGeneratorHandler::LoadGeneratorID(const std::string& tag)
{
	IExplosionGenerator* eg = LoadGenerator(tag);

	if (eg == NULL)
		return EXPGEN_ID_INVALID;

	return (eg->GetGeneratorID());
}

// creates either a standard or a custom explosion generator instance
// NOTE:
//   can be called recursively for custom instances (LoadGenerator ->
//   Load -> ParseExplosionCode -> LoadGenerator -> ...), generators
//   must NOT be overwritten
IExplosionGenerator* CExplosionGeneratorHandler::LoadGenerator(const string& tag)
{
	const TagIdentMapConstIt it = expGenTagIdentMap.find(tag);

	if (it != expGenTagIdentMap.end())
		return explosionGenerators[it->second];

	// tag is either "CStdExplosionGenerator" (or some sub-string, eg.
	// "std") which maps to CStdExplosionGenerator or "custom:postfix"
	// which maps to CCustomExplosionGenerator, all others cause NULL
	// to be returned
	string prefix;
	string postfix;

	const string::size_type seppos = tag.find(':');

	IExplosionGenerator* explGen;

	if (seppos != string::npos) {
		// grab the "custom" prefix (the only supported value)
		prefix = tag.substr(0, seppos);
		postfix = tag.substr(seppos + 1);
		assert((prefix + ":") == CEG_PREFIX_STRING);
		explGen = new CCustomExplosionGenerator();
	} else {
		prefix = tag;
		explGen = new CStdExplosionGenerator();
	}

	explGen->SetGeneratorID(explosionGenerators.size());

	// can still be a standard generator, but with non-zero ID
	assert(explGen->GetGeneratorID() != EXPGEN_ID_STANDARD);

	// save generator so ID is valid *before* possible recursion
	explosionGenerators.push_back(explGen);

	if (!postfix.empty()) {
		// standard EG's have no postfix (nor always a prefix)
		// custom EG's always have CEG_PREFIX_STRING in front
		expGenTagIdentMap[tag] = explGen->GetGeneratorID();
		expGenIdentTagMap[explGen->GetGeneratorID()] = tag;

		explGen->Load(this, postfix);
	}

	return explGen;
}

IExplosionGenerator* CExplosionGeneratorHandler::GetGenerator(unsigned int expGenID)
{
	if (expGenID == EXPGEN_ID_INVALID)
		return NULL;
	if (expGenID >= explosionGenerators.size())
		return NULL;

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
	if (expGenID == EXPGEN_ID_INVALID)
		return false;

	assert(expGenID < explosionGenerators.size());

	return explosionGenerators[expGenID]->Explosion(pos, dir, damage, radius, gfxMod, owner, hit);
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
	const bool airExplosion    = ((flags & CCustomExplosionGenerator::SPW_AIR       ) != 0);
	const bool groundExplosion = ((flags & CCustomExplosionGenerator::SPW_GROUND    ) != 0);
	const bool waterExplosion  = ((flags & CCustomExplosionGenerator::SPW_WATER     ) != 0);
	const bool uwExplosion     = ((flags & CCustomExplosionGenerator::SPW_UNDERWATER) != 0);

	// limit the visual effects based on the radius
	damage /= 20.0f;
	damage = std::min(damage, radius * 1.5f);
	damage *= gfxMod;
	damage = std::max(damage, 0.0f);

	const float sqrtDmg = math::sqrt(damage);
	const float camLength = camVect.Length();
	float moveLength = radius * 0.03f;

	if (camLength > 0.0f) { camVect /= camLength; }
	if (camLength < moveLength + 2) { moveLength = camLength - 2; }

	const float3 npos = pos + camVect * moveLength;

	new CHeatCloudProjectile(owner, npos, float3(0.0f, 0.3f, 0.0f), 8.0f + sqrtDmg * 0.5f, 7 + damage * 2.8f);

	if (projectileHandler->GetParticleSaturation() < 1.0f) {
		// turn off lots of graphic only particles when we have more particles than we want
		float smokeDamage      = damage;
		float smokeDamageSQRT  = 0.0f;
		float smokeDamageISQRT = 0.0f;

		if (uwExplosion) { smokeDamage *= 0.3f; }
		if (airExplosion || waterExplosion) { smokeDamage *= 0.6f; }

		if (smokeDamage > 0.01f) {
			smokeDamageSQRT = math::sqrt(smokeDamage);
			smokeDamageISQRT = 1.0f / (smokeDamageSQRT * 0.35f);
		}

		for (int a = 0; a < smokeDamage * 0.6f; ++a) {
			const float3 speed(
				(-0.1f + gu->RandFloat() * 0.2f),
				( 0.1f + gu->RandFloat() * 0.3f) * smokeDamageISQRT,
				(-0.1f + gu->RandFloat() * 0.2f)
			);

			const float time = (40.0f + smokeDamageSQRT * 15.0f) * (0.8f + gu->RandFloat() * 0.7f);

			float3 dir = gu->RandVector() * smokeDamage;
			dir.y = std::abs(dir.y);
			const float3 npos = pos + dir;

			new CSmokeProjectile2(owner, pos, npos, speed, time, smokeDamageSQRT * 4.0f, 0.4f, 0.6f);
		}

		if (groundExplosion) {
			const int numDirt = std::min(20.0f, damage * 0.8f);
			const float explSpeedMod = 0.7f + std::min(30.0f, damage) / GAME_SPEED;
			const float3 color(0.15f, 0.1f, 0.05f);

			for (int a = 0; a < numDirt; ++a) {
				const float3 explSpeed = float3(
					(0.5f - gu->RandFloat()) * 1.5f,
					 1.7f + gu->RandFloat()  * 1.6f,
					(0.5f - gu->RandFloat()) * 1.5f
				);

				const float3 npos(
					pos.x - (0.5f - gu->RandFloat()) * (radius * 0.6f),
					pos.y -  2.0f - damage * 0.2f,
					pos.z - (0.5f - gu->RandFloat()) * (radius * 0.6f)
				);

				new CDirtProjectile(owner, npos, explSpeed  * explSpeedMod, 90.0f + damage * 2.0f, 2.0f + sqrtDmg * 1.5f, 0.4f, 0.999f, color);
			}
		}

		if (!airExplosion && !uwExplosion && waterExplosion) {
			const int numDirt = std::min(40.f, damage * 0.8f);
			const float3 color(1.0f, 1.0f, 1.0f);

			for (int a = 0; a < numDirt; ++a) {
				const float3 speed(
					(    0.5f - gu->RandFloat()) * 0.2f,
					(a * 0.1f + gu->RandFloat()  * 0.8f),
					(    0.5f - gu->RandFloat()) * 0.2f
				);
				const float3 npos(
					pos.x - (0.5f - gu->RandFloat()) * (radius * 0.2f),
					pos.y -  2.0f - sqrtDmg          *           2.0f,
					pos.z - (0.5f - gu->RandFloat()) * (radius * 0.2f)
				);

				new CDirtProjectile(
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
			const int numDebris = (gu->RandInt() % 6) + 3 + int(damage * 0.04f);
			const float explSpeedMod = (0.7f + std::min(30.0f, damage) / 23);

			for (int a = 0; a < numDebris; ++a) {
				const float3 explSpeed = (altitude < 4.0f)?
					float3((0.5f - gu->RandFloat()) * 2.0f, 1.8f + gu->RandFloat() * 1.8f, (0.5f - gu->RandFloat()) * 2.0f):
					float3(gu->RandVector() * 2);

				const float3 npos(
					pos.x - (0.5f - gu->RandFloat()) * (radius * 1),
					pos.y,
					pos.z - (0.5f - gu->RandFloat()) * (radius * 1)
				);

				new CWreckProjectile(owner, npos, explSpeed * explSpeedMod, 90.0f + damage * 2.0f);
			}
		}
		if (uwExplosion) {
			const int numBubbles = (damage * 0.7f);

			for (int a = 0; a < numBubbles; ++a) {
				new CBubbleProjectile(
					owner,
					pos + gu->RandVector() * radius * 0.5f,
					gu->RandVector() * 0.2f + float3(0.0f, 0.2f, 0.0f),
					damage * 2.0f + gu->RandFloat() * damage,
					1.0f + gu->RandFloat() * 2.0f,
					0.02f,
					0.5f + gu->RandFloat() * 0.3f
				);
			}
		}
		if (waterExplosion && !uwExplosion && !airExplosion) {
			const int numWake = (damage * 0.5f);

			for (int a = 0; a < numWake; ++a) {
				new CWakeProjectile(
					owner,
					pos + gu->RandVector() * radius * 0.2f,
					gu->RandVector() * radius * 0.003f,
					sqrtDmg * 4.0f,
					damage * 0.03f,
					0.3f + gu->RandFloat() * 0.2f,
					0.8f / (sqrtDmg * 3 + 50 + gu->RandFloat() * 90.0f),
					1
				);
			}
		}
		if (radius > 10.0f && damage > 4.0f) {
			const int numSpike = int(sqrtDmg) + 8;
			const float explSpeedMod = (8 + damage * 3.0f) / (9 + sqrtDmg * 0.7f) * 0.35f;

			for (int a = 0; a < numSpike; ++a) {
				float3 explSpeed = (gu->RandVector()).SafeNormalize() * explSpeedMod;

				if (!airExplosion && !waterExplosion && (explSpeed.y < 0.0f)) {
					explSpeed.y = -explSpeed.y;
				}

				new CExploSpikeProjectile(
					owner,
					pos + explSpeed,
					explSpeed * (0.9f + gu->RandFloat() * 0.4f),
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

			float circleAlpha = 0;
			float circleGrowth = 0;

			if (radius > 40.0f && damage > 12.0f) {
				circleAlpha = std::min(0.5f, damage * 0.01f);
				circleGrowth = (8.0f + damage * 2.5f) / (9.0f + sqrtDmg * 0.7f) * 0.55f;
			}

			new CStandardGroundFlash(pos, circleAlpha, flashAlpha, flashSize, circleGrowth, ttl);
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
	void* ptr = NULL;
	float buffer[16];
	for(int i=0; i<16; i++) buffer[i] = 0.0f; //initialize buffer

	for (;;) {
		switch (*(code++)) {
			case OP_END: {
				return;
			}
			case OP_STOREI: {
				boost::uint8_t  size   = *(boost::uint8_t*)  code; code++;
				boost::uint16_t offset = *(boost::uint16_t*) code; code += 2;
				switch (size) {
					case 1: { *(boost::int8_t*)  (instance + offset) = (int) val; } break;
					case 2: { *(boost::int16_t*) (instance + offset) = (int) val; } break;
					case 4: { *(boost::int32_t*) (instance + offset) = (int) val; } break;
					case 8: { *(boost::int64_t*) (instance + offset) = (int) val; } break;
					default: { /*no op*/ } break;
				}
				val = 0.0f;
				break;
			}
			case OP_STOREF: {
				boost::uint8_t  size   = *(boost::uint8_t*)  code; code++;
				boost::uint16_t offset = *(boost::uint16_t*) code; code += 2;
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
				val += gu->RandFloat() * (*(float*) code);
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
				boost::uint16_t offset = *(boost::uint16_t*) code;
				code += 2;
				*(void**) (instance + offset) = ptr;
				ptr = NULL;
				break;
			}
			case OP_DIR: {
				boost::uint16_t offset = *(boost::uint16_t*) code;
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
				val = (*(float*) code) * math::floor(SafeDivide(val, (*(float*) code)));
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
	string& code)
{
	const std::string content = script.substr(0, script.find(';', 0));
	const bool isFloat = memberInfo.type == SExpGenSpawnableMemberInfo::TYPE_FLOAT;

	if (content == "dir") { // first see if we can match any keywords
		// if the user uses a keyword assume he knows that it is put on the right datatype for now
		if (memberInfo.length < 3 || !isFloat) // dir has to be float3
			throw content_error("[CCEG::ParseExplosionCode] incorrect use of \"dir\" (" + script + ")");

		code += OP_DIR;
		boost::uint16_t ofs = memberInfo.offset;
		code.append((char*) &ofs, (char*) &ofs + sizeof(ofs));

		return;
	}

	//Arrays (float3 or float4)
	if (memberInfo.length > 1) {
		string::size_type start = 0;
		SExpGenSpawnableMemberInfo subInfo = memberInfo;
		subInfo.length = 1;
		for (unsigned int i = 0; i < memberInfo.length && start < script.length(); ++i) {
			string::size_type subEnd = script.find(',', start+1);
			ParseExplosionCode(psi, script.substr(start, subEnd-start), subInfo, code);
			start = subEnd + 1;
			subInfo.offset += subInfo.size;
		}

		return;
	}

	//Textures, Colormaps, etc.
	if (memberInfo.type == SExpGenSpawnableMemberInfo::TYPE_PTR) {
		// Memory is managed by whomever this callback belongs to
		void* ptr = memberInfo.ptrCallback(content);
		code += OP_LOADP;
		code.append((char*)(&ptr), ((char*)(&ptr)) + sizeof(void*));
		code += OP_STOREP;
		boost::uint16_t ofs = memberInfo.offset;
		code.append((char*)&ofs, (char*)&ofs + sizeof(ofs));

		return;
	}


	//Floats or Ints

	assert (isFloat || memberInfo.type == SExpGenSpawnableMemberInfo::TYPE_INT);

	static const std::set<boost::uint8_t> allowedSizeInt = {1,2,4 /*,0,8*/};
	static const std::set<boost::uint8_t> allowedSizeFlt = {4 /*,8*/};
	if (isFloat) {
		if (allowedSizeFlt.find(memberInfo.size) == allowedSizeFlt.end())
			throw content_error("[CCEG::ParseExplosionCode] incompatible float size \"" + IntToString(memberInfo.size) + "\" (" + script + ")");
	} else {
		if (allowedSizeInt.find(memberInfo.size) == allowedSizeInt.end())
			throw content_error("[CCEG::ParseExplosionCode] incompatible integer size \"" + IntToString(memberInfo.size) + "\" (" + script + ")");
	}

	// parse the code
	int p = 0;
	while (p < script.length()) {
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
			const char* fmt = "[CCEG::ParseExplosionCode] unknown op-code \"%c\" in \"%s\" at index %d";
			LOG_L(L_WARNING, fmt, c, script.c_str(), p);
			continue;
		}

		// be sure to exit cleanly if there are no more operators or operands
		if (p >= script.size())
			continue;

		char* endp = NULL;

		if (!useInt) {
			// strtod&co expect C-style strings with NULLs,
			// c_str() is guaranteed to be NULL-terminated
			// (whether .data() == .c_str() depends on the
			// implementation of std::string)
			const float v = (float)strtod(&script.c_str()[p], &endp);

			p += (endp - &script.c_str()[p]);
			code += opcode;
			code.append((char*) &v, ((char*) &v) + sizeof(v));
		} else {
			const int v = std::max(0, std::min(16, (int)strtol(&script.c_str()[p], &endp, 10)));

			p += (endp - &script.c_str()[p]);
			code += opcode;
			code.append((char*) &v, ((char*) &v) + + sizeof(v));
		}
	}

	// store the final value
	code.push_back(isFloat ? OP_STOREF : OP_STOREI);
	code.push_back(memberInfo.size);
	boost::uint16_t ofs = memberInfo.offset;
	code.append((char*)&ofs, (char*)&ofs + sizeof(ofs));
}



bool CCustomExplosionGenerator::Load(CExplosionGeneratorHandler* handler, const string& tag)
{
	const LuaTable* root = handler->GetExplosionTableRoot();
	const LuaTable& expTable = (root != NULL)? root->SubTable(tag): LuaTable();

	if (!expTable.IsValid()) {
		// not a fatal error: any calls to Explosion will just return early
		LOG_L(L_WARNING, "[CCEG::%s] table for CEG \"%s\" invalid (parse errors?)", __FUNCTION__, tag.c_str());
		return false;
	}

	expGenParams.projectiles.clear();
	expGenParams.groundFlash = GroundFlashInfo();

	vector<string> spawns;
	expTable.GetKeys(spawns);

	for (unsigned int n = 0; n < spawns.size(); n++) {
		ProjectileSpawnInfo psi;

		const string& spawnName = spawns[n];
		const LuaTable& spawnTable = expTable.SubTable(spawnName);

		// NOTE:
		//   *every* CEG table contains a spawn called "filename"
		//   see springcontent/gamedata/explosions.lua::LoadTDFs
		if (!spawnTable.IsValid())
			continue;
		if (spawnName == "groundflash" || spawnName == "filename")
			continue;

		const string& className = handler->GetProjectileClasses().ResolveAlias(spawnTable.GetString("class", spawnName));
		psi.spawnableID = CExpGenSpawnable::GetSpawnableID(className);
		if (psi.spawnableID == -1u) {
			LOG_L(L_WARNING, "[CCEG::%s] %s: Unknown class \"%s\"", __FUNCTION__, tag.c_str(), className.c_str());
			continue;
		}

		psi.flags = GetFlagsFromTable(spawnTable);
		psi.count = std::max(0, spawnTable.GetInt("count", 1));

		string code;
		map<string, string> props;

		spawnTable.SubTable("properties").GetMap(props);

		for (const auto& propIt: props) {
			SExpGenSpawnableMemberInfo memberInfo = {0, 0, 0, STRING_HASH(std::move(StringToLower(propIt.first))), SExpGenSpawnableMemberInfo::TYPE_INT, nullptr};

			if (CExpGenSpawnable::GetSpawnableMemberInfo(className, memberInfo)) {
				ParseExplosionCode(&psi, propIt.second, memberInfo, code);
			} else {
				LOG_L(L_WARNING, "[CCEG::%s] %s: Unknown tag %s::%s", __FUNCTION__, tag.c_str(), className.c_str(), propIt.first.c_str());
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

		expGenParams.groundFlash.flags = SPW_GROUND | GetFlagsFromTable(gndTable);
		expGenParams.groundFlash.ttl = ttl;
	}

	expGenParams.useDefaultExplosions = expTable.GetBool("useDefaultExplosions", false);
	return true;
}

bool CCustomExplosionGenerator::Reload(CExplosionGeneratorHandler* handler, const std::string& tag) {
	const ExpGenParams oldParams = expGenParams;

	if (!Load(explGenHandler, tag)) {
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
	const float groundHeight = CGround::GetHeightReal(pos.x, pos.z);

	unsigned int flags = GetFlagsFromHeight(pos.y, groundHeight);
	const bool groundExplosion = ((flags & CCustomExplosionGenerator::SPW_GROUND) != 0);

	if (hit) flags |= SPW_UNIT;
	else     flags |= SPW_NO_UNIT;

	const std::vector<ProjectileSpawnInfo>& spawnInfo = expGenParams.projectiles;
	const GroundFlashInfo& groundFlash = expGenParams.groundFlash;

	for (int a = 0; a < spawnInfo.size(); a++) {
		const ProjectileSpawnInfo& psi = spawnInfo[a];

		if ((psi.flags & flags) == 0)
			continue;

		// no new projectiles if we're saturated
		if (projectileHandler->GetParticleSaturation() > 1.0f)
			break;

		for (unsigned int c = 0; c < psi.count; c++) {
			CExpGenSpawnable* projectile = CExpGenSpawnable::CreateSpawnable(psi.spawnableID);
			ExecuteExplosionCode(&psi.code[0], damage, (char*) projectile, c, dir);
			projectile->Init(owner, pos);
		}
	}

	if (groundExplosion && (groundFlash.ttl > 0) && (groundFlash.flashSize > 1)) {
		new CStandardGroundFlash(pos, groundFlash);
	}

	if (expGenParams.useDefaultExplosions) {
		return (explGenHandler->GenExplosion(CExplosionGeneratorHandler::EXPGEN_ID_STANDARD, pos, dir, damage, radius, gfxMod, owner, hit));
	}

	return true;
}


bool CCustomExplosionGenerator::OutputProjectileClassInfo()
{
#ifdef USING_CREG
	LOG_DISABLE();
		// we need to load basecontent for class aliases
		FileSystemInitializer::Initialize();
		vfsHandler->AddArchiveWithDeps(archiveScanner->ArchiveFromName("Spring content v1"), false);
	LOG_ENABLE();

	const vector<creg::Class*>& classes = creg::System::GetClasses();
	CExplosionGeneratorHandler egh;

	std::cout << "{" << std::endl;

	for (vector<creg::Class*>::const_iterator ci = classes.begin(); ci != classes.end(); ++ci) {
		creg::Class* c = *ci;

		if (!c->IsSubclassOf(CExpGenSpawnable::StaticClass()) || c == CExpGenSpawnable::StaticClass())
			continue;

		if (c->binder->flags & creg::CF_Synced)
			continue;

		if (ci != classes.begin())
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

