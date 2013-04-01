/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <fstream>
#include <stdexcept>
#include <cassert>
#include <boost/cstdint.hpp>

#include "ExplosionGenerator.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/BubbleProjectile.h"
#include "Sim/Projectiles/Unsynced/DirtProjectile.h"
#include "Sim/Projectiles/Unsynced/ExploSpikeProjectile.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile2.h"
#include "Sim/Projectiles/Unsynced/SpherePartProjectile.h"
#include "Sim/Projectiles/Unsynced/WakeProjectile.h"
#include "Sim/Projectiles/Unsynced/WreckProjectile.h"

#include "System/creg/STL_Map.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/creg/VarTypes.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Util.h"

CR_BIND_DERIVED_INTERFACE(CExpGenSpawnable, CWorldObject);
CR_REG_METADATA(CExpGenSpawnable, );

CR_BIND_INTERFACE(IExplosionGenerator);
CR_REG_METADATA(IExplosionGenerator, (
	CR_MEMBER(generatorID)
));

CR_BIND_DERIVED(CStdExplosionGenerator, IExplosionGenerator, );

CR_BIND(CCustomExplosionGenerator::ProjectileSpawnInfo, )
CR_REG_METADATA_SUB(CCustomExplosionGenerator, ProjectileSpawnInfo, (
	//CR_MEMBER(projectileClass), FIXME is pointer
	CR_MEMBER(code),
	CR_MEMBER(count),
	CR_MEMBER(flags)
));

CR_BIND(CCustomExplosionGenerator::GroundFlashInfo, )
CR_REG_METADATA_SUB(CCustomExplosionGenerator, GroundFlashInfo, (
	CR_MEMBER(flashSize),
	CR_MEMBER(flashAlpha),
	CR_MEMBER(circleGrowth),
	CR_MEMBER(circleAlpha),
	CR_MEMBER(ttl),
	CR_MEMBER(color),
	CR_MEMBER(flags)
));

CR_BIND(CCustomExplosionGenerator::CEGData, )
CR_REG_METADATA_SUB(CCustomExplosionGenerator, CEGData, (
	CR_MEMBER(projectileSpawn),
	CR_MEMBER(groundFlash),
	CR_MEMBER(useDefaultExplosions)
));

CR_BIND_DERIVED(CCustomExplosionGenerator, CStdExplosionGenerator, );
CR_REG_METADATA(CCustomExplosionGenerator, (
	CR_MEMBER(explosionIDs),
	CR_MEMBER(explosionData)//,
	//CR_MEMBER(spawnExplGens) FIXME
));


CExplosionGeneratorHandler* explGenHandler = NULL;
CCustomExplosionGenerator* gCEG = NULL;

CExpGenSpawnable::CExpGenSpawnable(): CWorldObject() { GML_EXPGEN_CHECK() }
CExpGenSpawnable::CExpGenSpawnable(const float3& pos): CWorldObject(pos) { GML_EXPGEN_CHECK() }



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

unsigned int CCustomExplosionGenerator::GetFlagsFromHeight(float height, float altitude)
{
	unsigned int flags = 0;

	// note: ranges do not overlap, although code in
	// *ExplosionGenerator::Explosion assumes they can
	     if (height >    0.0f && altitude >= 20.0f) { flags |= CCustomExplosionGenerator::SPW_AIR;        }
	else if (height >    0.0f && altitude >= -1.0f) { flags |= CCustomExplosionGenerator::SPW_GROUND;     }
	else if (height >   -5.0f && altitude >= -1.0f) { flags |= CCustomExplosionGenerator::SPW_WATER;      }
	else if (height <=  -5.0f && altitude >= -1.0f) { flags |= CCustomExplosionGenerator::SPW_UNDERWATER; }

	return flags;
}



void ClassAliasList::Load(const LuaTable& aliasTable)
{
	map<string, string> aliasList;
	aliasTable.GetMap(aliasList);
	aliases.insert(aliasList.begin(), aliasList.end());
}

creg::Class* ClassAliasList::GetClass(const string& name)
{
	string n = name;
	for (;;) {
		map<string, string>::iterator i = aliases.find(n);
		if (i == aliases.end()) {
			break;
		}
		n = i->second;
	}
	creg::Class* cls = creg::System::GetClass(n);
	if (!cls) {
		throw content_error("Unknown class: " + name);
	}
	return cls;
}

string ClassAliasList::FindAlias(const string& className)
{
	for (map<string, string>::iterator i = aliases.begin(); i != aliases.end(); ++i) {
		if (i->second == className) return i->first;
	}
	return className;
}






CExplosionGeneratorHandler::CExplosionGeneratorHandler()
{
	numLoadedGenerators = 0;

	exploParser = NULL;
	aliasParser = NULL;
	explTblRoot = NULL;

	gCEG = new CCustomExplosionGenerator();

	ParseExplosionTables();
}

CExplosionGeneratorHandler::~CExplosionGeneratorHandler()
{
	delete exploParser; exploParser = NULL;
	delete aliasParser; aliasParser = NULL;
	delete explTblRoot; explTblRoot = NULL;

	explosionGenerators.clear();

	gCEG->Unload(this);
	gCEG->ClearCache();

	delete gCEG; gCEG = NULL;
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
		generatorClasses.Clear();
		generatorClasses.Load(aliasRoot.SubTable("generators"));
	}

	if (!exploParser->Execute()) {
		LOG_L(L_ERROR, "Failed to parse explosions: %s",
				exploParser->GetErrorLog().c_str());
	} else {
		explTblRoot = new LuaTable(exploParser->GetRoot());
	}
}

IExplosionGenerator* CExplosionGeneratorHandler::LoadGenerator(const string& tag)
{
	string prefix = tag;
	string postfix = "";
	string::size_type seppos = tag.find(':');

	if (seppos != string::npos) {
		// grab the "custom" prefix (the only supported value)
		prefix = tag.substr(0, seppos);
		postfix = tag.substr(seppos + 1);
		assert(prefix == "custom");
	}

	creg::Class* cls = generatorClasses.GetClass(prefix);

	if (!cls->IsSubclassOf(IExplosionGenerator::StaticClass())) {
		throw content_error(prefix + " is not a subclass of IExplosionGenerator");
	}

	IExplosionGenerator* explGen = static_cast<IExplosionGenerator*>(cls->CreateInstance());
	explGen->SetGeneratorID(++numLoadedGenerators);

	assert(gCEG != explGen);
	assert(gCEG->GetGeneratorID() == 0);

	if (seppos != string::npos) {
		explGen->Load(this, postfix);
	}

	explosionGenerators[explGen->GetGeneratorID()] = explGen;
	return explGen;
}

void CExplosionGeneratorHandler::UnloadGenerator(IExplosionGenerator* explGen)
{
	assert(gCEG != explGen);
	assert(gCEG->GetGeneratorID() == 0);

	explGen->Unload(this);
	explosionGenerators.erase(explGen->GetGeneratorID());

	creg::Class* cls = explGen->GetClass();
	cls->DeleteInstance(explGen);
}

void CExplosionGeneratorHandler::ReloadGenerators(const std::string& tag) {
	// re-parse the projectile and generator tables
	ParseExplosionTables();

	std::map<unsigned int, IExplosionGenerator*>& egs = explosionGenerators;
	std::map<unsigned int, IExplosionGenerator*>::iterator egsIt;

	for (egsIt = egs.begin(); egsIt != egs.end(); ++egsIt) {
		(egsIt->second)->Reload(this, tag);
	}

	gCEG->Reload(this, tag);
}






bool CStdExplosionGenerator::Explosion(
	unsigned int explosionID,
	const float3& pos,
	float damage,
	float radius,
	CUnit* owner,
	float gfxMod,
	CUnit* hit,
	const float3& dir
) {
	const float groundHeight = ground->GetHeightReal(pos.x, pos.z);
	const float altitude = pos.y - groundHeight;

	float3 camVect = camera->pos - pos;

	const unsigned int flags = CCustomExplosionGenerator::GetFlagsFromHeight(pos.y, altitude);
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

	new CHeatCloudProjectile(npos, float3(0.0f, 0.3f, 0.0f), 8 + sqrtDmg * 0.5f, 7 + damage * 2.8f, owner);

	if (projectileHandler->particleSaturation < 1.0f) {
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

			const float h = ground->GetApproximateHeight(npos.x, npos.z);
			const float time = (40.0f + smokeDamageSQRT * 15.0f) * (0.8f + gu->RandFloat() * 0.7f);

			float3 npos = pos + gu->RandVector() * smokeDamage;
			npos.y = std::max(npos.y, h);

			new CSmokeProjectile2(pos, npos, speed, time, smokeDamageSQRT * 4.0f, 0.4f, owner, 0.6f);
		}

		if (groundExplosion) {
			const int numDirt = std::min(20.0f, damage * 0.8f);
			const float3 color(0.15f, 0.1f, 0.05f);

			for (int a = 0; a < numDirt; ++a) {
				float3 speed(
					(0.5f - gu->RandFloat()) * 1.5f,
					 1.7f + gu->RandFloat()  * 1.6f,
					(0.5f - gu->RandFloat()) * 1.5f
				);
				speed *= (0.7f + std::min(30.0f, damage) / GAME_SPEED);

				const float3 npos(
					pos.x - (0.5f - gu->RandFloat()) * (radius * 0.6f),
					pos.y -  2.0f - damage * 0.2f,
					pos.z - (0.5f - gu->RandFloat()) * (radius * 0.6f)
				);

				new CDirtProjectile(npos, speed, 90 + damage * 2, 2.0f + sqrtDmg * 1.5f, 0.4f, 0.999f, owner, color);
			}
		}

		if (!airExplosion && !uwExplosion && waterExplosion) {
			const int numDirt = std::min(40.f, damage*0.8f);
			const float3 color(1.0f, 1.0f, 1.0f);

			for (int a = 0; a < numDirt; ++a) {
				float3 speed((0.5f - gu->RandFloat()) * 0.2f, a * 0.1f + gu->RandFloat()*0.8f, (0.5f - gu->RandFloat()) * 0.2f);
				speed *= 0.7f + std::min(30.0f, damage) / GAME_SPEED;
				float3 npos(pos.x-(0.5f-gu->RandFloat())*(radius*0.2f), pos.y - 2.0f - sqrtDmg * 2.0f, pos.z-(0.5f-gu->RandFloat())*(radius*0.2f));
				new CDirtProjectile(npos, speed, 90 + damage*2, 2.0f + sqrtDmg * 2.0f, 0.3f, 0.99f, owner, color);
			}
		}
		if (damage >= 20.0f && !uwExplosion && !airExplosion) {
			const int numDebris = (gu->RandInt() % 6) + 3 + int(damage * 0.04f);

			for (int a = 0; a < numDebris; ++a) {
				float3 speed;
				if (altitude < 4.0f) {
					speed = float3((0.5f-gu->RandFloat())*2.0f,1.8f+gu->RandFloat()*1.8f,(0.5f-gu->RandFloat())*2.0f);
				} else {
					speed = float3(gu->RandVector() * 2);
				}
				speed *= 0.7f + std::min(30.0f, damage) / 23;
				float3 npos(pos.x - (0.5f - gu->RandFloat()) * (radius * 1), pos.y, pos.z - (0.5f - gu->RandFloat()) * (radius * 1));
				new CWreckProjectile(npos, speed, 90 + damage*2, owner);
			}
		}
		if (uwExplosion) {
			const int numBubbles = (damage * 0.7f);

			for (int a = 0; a < numBubbles; ++a) {
				new CBubbleProjectile(pos + gu->RandVector()*radius*0.5f,
						gu->RandVector()*0.2f + float3(0.0f, 0.2f, 0.0f),
						damage*2 + gu->RandFloat()*damage,
						1 + gu->RandFloat()*2,
						0.02f,
						owner,
						0.5f + gu->RandFloat() * 0.3f);
			}
		}
		if (waterExplosion && !uwExplosion && !airExplosion) {
			const int numWake = (damage * 0.5f);

			for (int a = 0; a < numWake; ++a) {
				new CWakeProjectile(pos + gu->RandVector()*radius*0.2f,
					gu->RandVector()*radius*0.003f,
					sqrtDmg * 4,
					damage * 0.03f,
					owner,
					0.3f + gu->RandFloat()*0.2f,
					0.8f / (sqrtDmg * 3 + 50 + gu->RandFloat()*90),
					1);
			}
		}
		if (radius > 10 && damage > 4) {
			const int numSpike = int(sqrtDmg) + 8;

			for (int a = 0; a < numSpike; ++a) {
				float3 speed = gu->RandVector();
				speed.SafeNormalize();
				speed *= (8 + damage * 3.0f) / (9 + sqrtDmg * 0.7f) * 0.35f;

				if (!airExplosion && !waterExplosion && (speed.y < 0)) {
					speed.y=-speed.y;
				}
				new CExploSpikeProjectile(pos + speed,
					speed * (0.9f + gu->RandFloat()*0.4f),
					radius * 0.1f,
					radius * 0.1f,
					0.6f,
					0.8f / (8 + sqrtDmg),
					owner);
			}
		}
	}

	if (radius > 20 && damage > 6 && altitude < (radius * 0.7f)) {
		const float flashSize = std::max(radius, damage * 2);
		const float ttl = 8 + sqrtDmg * 0.8f;
		if (flashSize > 5.f && ttl > 15.f) {
			const float flashAlpha = std::min(0.8f, damage * 0.01f);

			float circleAlpha = 0;
			float circleGrowth = 0;
			if (radius > 40 && damage > 12) {
				circleAlpha = std::min(0.5f, damage * 0.01f);
				circleGrowth = (8 + damage*2.5f) / (9 + sqrtDmg * 0.7f) * 0.55f;
			}

			new CStandardGroundFlash(pos, circleAlpha, flashAlpha, flashSize, circleGrowth, ttl);
		}
	}

	if (radius > 40 && damage > 12) {
		CSpherePartProjectile::CreateSphere(pos,
				std::min(0.7f, damage * 0.02f),
				5 + int(sqrtDmg * 0.7f),
				(8 + damage * 2.5f) / (9 + sqrtDmg * 0.7f) * 0.5f,
				owner);
	}

	return true;
}



void CCustomExplosionGenerator::ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3& dir, bool synced)
{
	float val = 0.0f;
	void* ptr = NULL;
	float buffer[16];

	for (;;) {
		switch (*(code++)) {
			case OP_END: {
				return;
			}
			case OP_STOREI: {
				boost::uint16_t offset = *(boost::uint16_t*) code;
				code += 2;
				*(int*) (instance + offset) = (int) val;
				val = 0.0f;
				break;
			}
			case OP_STOREF: {
				boost::uint16_t offset = *(boost::uint16_t*) code;
				code += 2;
				*(float*) (instance + offset) = val;
				val = 0.0f;
				break;
			}
			case OP_STOREC: {
				boost::uint16_t offset = *(boost::uint16_t*) code;
				code += 2;
				*(unsigned char*) (instance + offset) = (int) val;
				val = 0.0f;
				break;
			}
			case OP_ADD: {
				val += *(float*) code;
				code += 4;
				break;
			}
			case OP_RAND: {
				if (synced) {
					val += gs->randFloat() * (*(float*) code);
				} else {
					val += gu->RandFloat() * (*(float*) code);
				}

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
	const int offset,
	const boost::shared_ptr<creg::IType> type,
	const string& script,
	string& code)
{
	// strtod&co expect C-style strings with NULLs,
	// c_str() is guaranteed to be NULL-terminated
	// (whether .data() == .c_str() depends on the
	// implementation of std::string)
	const char* scriptStr = script.c_str();

	string::size_type end = script.find(';', 0);
	string vastr = script.substr(0, end);

	if (vastr == "dir") { // first see if we can match any keywords
		// if the user uses a keyword assume he knows that it is put on the right datatype for now
		code += OP_DIR;
		boost::uint16_t ofs = offset;
		code.append((char*) &ofs, (char*) &ofs + 2);
	}
	else if (dynamic_cast<creg::BasicType*>(type.get())) {
		const creg::BasicType* basicType = (creg::BasicType*) type.get();
		const bool legalType =
			(basicType->id == creg::crInt  ) ||
			(basicType->id == creg::crFloat) ||
			(basicType->id == creg::crUChar) ||
			(basicType->id == creg::crBool );

		if (!legalType) {
			throw content_error("[CCEG::ParseExplosionCode] projectile type-properties other than int, float, uchar, or bool are not supported (" + script + ")");
		}

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
				LOG_L(L_WARNING, fmt, c, scriptStr, p);
				continue;
			}

			// be sure to exit cleanly if there are no more operators or operands
			if (p >= script.size())
				continue;

			char* endp = NULL;

			if (!useInt) {
				const float v = (float)strtod(&scriptStr[p], &endp);

				p += (endp - &scriptStr[p]);
				code += opcode;
				code.append((char*) &v, ((char*) &v) + 4);
			} else {
				const int v = std::max(0, std::min(16, (int)strtol(&scriptStr[p], &endp, 10)));

				p += (endp - &scriptStr[p]);
				code += opcode;
				code.append((char*) &v, ((char*) &v) + 4);
			}
		}

		switch (basicType->id) {
			case creg::crInt:   code.push_back(OP_STOREI); break;
			case creg::crBool:  code.push_back(OP_STOREI); break;
			case creg::crFloat: code.push_back(OP_STOREF); break;
			case creg::crUChar: code.push_back(OP_STOREC); break;
			default: break;
		}

		boost::uint16_t ofs = offset;
		code.append((char*)&ofs, (char*)&ofs + 2);
	}
	else if (dynamic_cast<creg::ObjectInstanceType*>(type.get())) {
		creg::ObjectInstanceType *oit = (creg::ObjectInstanceType *)type.get();

		string::size_type start = 0;
		for (creg::Class* c = oit->objectClass; c; c=c->base) {
			for (int a = 0; a < c->members.size(); a++) {
				string::size_type end = script.find(',', start+1);
				ParseExplosionCode(psi, offset + c->members [a]->offset, c->members[a]->type, script.substr(start,end-start), code);
				start = end+1;
				if (start >= script.length()) { break; }
			}
			if (start >= script.length()) { break; }
		}
	}
	else if (dynamic_cast<creg::StaticArrayBaseType*>(type.get())) {
		creg::StaticArrayBaseType *sat = (creg::StaticArrayBaseType*)type.get();

		string::size_type start = 0;
		for (unsigned int i=0; i < sat->size; i++) {
			string::size_type end = script.find(',', start+1);
			ParseExplosionCode(psi, offset + sat->elemSize * i, sat->elemType, script.substr(start, end-start), code);
			start = end+1;
			if (start >= script.length()) { break; }
		}
	}
	else {
		if (type->GetName() == "AtlasedTexture*") {
			string::size_type end = script.find(';', 0);
			string texname = script.substr(0, end);
			// this memory is managed by textureAtlas (CTextureAtlas)
			void* tex = &projectileDrawer->textureAtlas->GetTexture(texname);
			code += OP_LOADP;
			code.append((char*)(&tex), ((char*)(&tex)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		} else if (type->GetName() == "GroundFXTexture*") {
			string::size_type end = script.find(';', 0);
			string texname = script.substr(0, end);
			// this memory is managed by groundFXAtlas (CTextureAtlas)
			void* tex = &projectileDrawer->groundFXAtlas->GetTexture(texname);
			code += OP_LOADP;
			code.append((char*)(&tex), ((char*)(&tex)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		} else if (type->GetName() == "CColorMap*") {
			string::size_type end = script.find(';', 0);
			string colorstring = script.substr(0, end);
			// gets stored and deleted at game end from inside CColorMap
			void* colormap = CColorMap::LoadFromDefString(colorstring);
			code += OP_LOADP;
			code.append((char*)(&colormap), ((char*)(&colormap)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		} else if (type->GetName() == "IExplosionGenerator*") {
			string::size_type end = script.find(';', 0);
			string name = script.substr(0, end);

			IExplosionGenerator* explGen = explGenHandler->LoadGenerator(name);
			spawnExplGens.push_back(explGen); // these will be unloaded in ~CCustomExplosionGenerator()

			void* explGenRaw = (void*) explGen;
			code += OP_LOADP;
			code.append((char*)(&explGenRaw), ((char*)(&explGenRaw)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		}
	}
}



unsigned int CCustomExplosionGenerator::Load(CExplosionGeneratorHandler* handler, const string& tag)
{
	unsigned int explosionID = -1U;

	if (tag.empty()) {
		return explosionID;
	}

	const std::map<std::string, unsigned int>::const_iterator it = explosionIDs.find(tag);

	if (it == explosionIDs.end()) {
		CEGData cegData;

		const LuaTable* root = handler->GetExplosionTableRoot();
		const LuaTable& expTable = (root != NULL)? root->SubTable(tag): LuaTable();

		if (!expTable.IsValid()) {
			// not a fatal error: any calls to ::Explosion will just return early
			LOG_L(L_WARNING,
					"[CCEG::Load] table for CEG \"%s\" invalid (parse errors?)",
					tag.c_str());
			return explosionID;
		}

		vector<string> spawns;
		expTable.GetKeys(spawns);

		for (vector<string>::iterator si = spawns.begin(); si != spawns.end(); ++si) {
			ProjectileSpawnInfo psi;

			const string& spawnName = *si;
			const LuaTable spawnTable = expTable.SubTable(spawnName);

			if (!spawnTable.IsValid() || spawnName == "groundflash") {
				continue;
			}

			const string className = spawnTable.GetString("class", spawnName);

			psi.projectileClass = handler->projectileClasses.GetClass(className);
			psi.flags = GetFlagsFromTable(spawnTable);
			psi.count = spawnTable.GetInt("count", 1);

			if (psi.projectileClass->binder->flags & creg::CF_Synced) {
				psi.flags |= SPW_SYNCED;
			}

			string code;
			map<string, string> props;
			map<string, string>::const_iterator propIt;
			spawnTable.SubTable("properties").GetMap(props);

			for (propIt = props.begin(); propIt != props.end(); ++propIt) {
				creg::Class::Member* m = psi.projectileClass->FindMember(propIt->first.c_str());
				if (m && (m->flags & creg::CM_Config)) {
					ParseExplosionCode(&psi, m->offset, m->type, propIt->second, code);
				}
			}

			code += (char)OP_END;
			psi.code.resize(code.size());
			copy(code.begin(), code.end(), psi.code.begin());

			cegData.projectileSpawn.push_back(psi);
		}

		const LuaTable gndTable = expTable.SubTable("groundflash");
		const int ttl = gndTable.GetInt("ttl", 0);
		if (ttl > 0) {
			cegData.groundFlash.circleAlpha  = gndTable.GetFloat("circleAlpha",  0.0f);
			cegData.groundFlash.flashSize    = gndTable.GetFloat("flashSize",    0.0f);
			cegData.groundFlash.flashAlpha   = gndTable.GetFloat("flashAlpha",   0.0f);
			cegData.groundFlash.circleGrowth = gndTable.GetFloat("circleGrowth", 0.0f);
			cegData.groundFlash.color        = gndTable.GetFloat3("color", float3(1.0f, 1.0f, 0.8f));

			cegData.groundFlash.flags = SPW_GROUND | GetFlagsFromTable(gndTable);
			cegData.groundFlash.ttl = ttl;
		}

		cegData.useDefaultExplosions = expTable.GetBool("useDefaultExplosions", false);

		explosionID = explosionData.size();
		explosionData.push_back(cegData);
		explosionIDs[tag] = explosionID;
	} else {
		explosionID = it->second;
	}

	return explosionID;
}

void CCustomExplosionGenerator::Reload(CExplosionGeneratorHandler* handler, const std::string& tag) {
	if (tag.empty()) {
		std::map<std::string, unsigned int> oldExplosionIDs(explosionIDs);
		std::map<std::string, unsigned int>::const_iterator it;

		Unload(handler);
		ClearCache();

		// reload all currently cached CEGs by tag
		// (ID's of active CEGs will remain valid)
		for (it = oldExplosionIDs.begin(); it != oldExplosionIDs.end(); ++it) {
			const std::string& tmpTag = it->first;
			const char* fmt = "[%s][generatorID=%u] reloading CEG \"%s\" (tagID %u)";

			LOG(fmt, __FUNCTION__, generatorID, tmpTag.c_str(), it->second);
			Load(explGenHandler, tmpTag);
		}
	} else {
		// reload a single CEG
		const std::map<std::string, unsigned int>::const_iterator it = explosionIDs.find(tag);

		if (it == explosionIDs.end()) {
			//LOG_L(L_WARNING, "[%s][generatorID=%u] unknown CEG-tag \"%s\"",
			//		__FUNCTION__, generatorID, tag.c_str());
			return;
		}

		const unsigned int numCEGs = explosionData.size();
		const unsigned int cegIndex = it->second;

		// note: if numCEGs == 1, these refer to the same data
		CEGData oldCEG = explosionData[cegIndex];
		CEGData tmpCEG = explosionData[numCEGs - 1];

		// get rid of the old data
		explosionIDs.erase(tag);
		explosionData[cegIndex] = tmpCEG;
		explosionData.pop_back();

		LOG("[%s][generatorID=%u] reloading single CEG \"%s\" (tagID %u)",
			__FUNCTION__, generatorID, tag.c_str(), cegIndex);

		if (Load(explGenHandler, tag) == -1U) {
			LOG_L(L_ERROR, "[%s][generatorID=%u] failed to reload single CEG \"%s\" (tagID %u)",
				__FUNCTION__, generatorID, tag.c_str(), cegIndex);

			// reload failed, keep the old CEG
			explosionIDs[tag] = cegIndex;

			explosionData.push_back(tmpCEG);
			explosionData[cegIndex] = oldCEG;
			return;
		}

		// re-map the old ID to the new data
		explosionIDs[tag] = cegIndex;

		if (numCEGs > 1) {
			explosionData[cegIndex] = explosionData[numCEGs - 1];
			explosionData[numCEGs - 1] = tmpCEG;
		}
	}
}

bool CCustomExplosionGenerator::Explosion(
	unsigned int explosionID,
	const float3& pos,
	float damage,
	float radius,
	CUnit* owner,
	float gfxMod,
	CUnit* hit,
	const float3& dir
) {
	if (explosionID == -1U || explosionID >= explosionData.size()) {
		// invalid CEG ID
		return false;
	}

	const float groundHeight = ground->GetHeightReal(pos.x, pos.z);
	const float altitude = pos.y - groundHeight;

	unsigned int flags = GetFlagsFromHeight(pos.y, altitude);
	const bool groundExplosion = ((flags & CCustomExplosionGenerator::SPW_GROUND) != 0);

	if (hit) flags |= SPW_UNIT;
	else     flags |= SPW_NO_UNIT;

	const CEGData& cegData = explosionData[explosionID];
	const std::vector<ProjectileSpawnInfo>& spawnInfo = cegData.projectileSpawn;
	const GroundFlashInfo& groundFlash = cegData.groundFlash;

	for (int a = 0; a < spawnInfo.size(); a++) {
		const ProjectileSpawnInfo& psi = spawnInfo[a];

		if (!(psi.flags & flags)) {
			continue;
		}

		// If we're saturated, spawn only synced projectiles.
		// Whether a class is synced is determined by the creg::CF_Synced flag.
		if (projectileHandler->particleSaturation > 1 && !(psi.flags & SPW_SYNCED)) {
			continue;
		}

		for (int c = 0; c < psi.count; c++) {
			CExpGenSpawnable* projectile = static_cast<CExpGenSpawnable*>((psi.projectileClass)->CreateInstance());

			ExecuteExplosionCode(&psi.code[0], damage, (char*) projectile, c, dir, (psi.flags & SPW_SYNCED) != 0);
			projectile->Init(pos, owner);
		}
	}

	if (groundExplosion && (groundFlash.ttl > 0) && (groundFlash.flashSize > 1)) {
		new CStandardGroundFlash(pos, groundFlash.circleAlpha, groundFlash.flashAlpha,
			groundFlash.flashSize, groundFlash.circleGrowth, groundFlash.ttl, groundFlash.color);
	}

	if (cegData.useDefaultExplosions) {
		return CStdExplosionGenerator::Explosion(-1U, pos, damage, radius, owner, gfxMod, hit, dir);
	}

	return true;
}


void CCustomExplosionGenerator::OutputProjectileClassInfo()
{
	const vector<creg::Class*>& classes = creg::System::GetClasses();
	std::ofstream fs("projectiles.txt");
	CExplosionGeneratorHandler egh;

	if (fs.bad() || !fs.is_open()) {
		return;
	}

	for (vector<creg::Class*>::const_iterator ci = classes.begin(); ci != classes.end(); ++ci) {
		if (!(*ci)->IsSubclassOf (CExpGenSpawnable::StaticClass()) || (*ci) == CExpGenSpawnable::StaticClass()) {
			continue;
		}

		creg::Class *klass = *ci;
		fs << "Class: " << klass->name << ".  Scriptname: " << egh.projectileClasses.FindAlias(klass->name) << std::endl;
		for (; klass; klass = klass->base) {
			for (unsigned int a = 0; a < klass->members.size(); a++) {
				if (klass->members[a]->flags & creg::CM_Config) {
					fs << "\t" << klass->members[a]->name << ": " << klass->members[a]->type->GetName() << "\n";
				}
			}
		}
		fs << "\n\n";
	}
}

void CCustomExplosionGenerator::Unload(CExplosionGeneratorHandler* handler) {
	std::vector<IExplosionGenerator*>::iterator egi;

	for (egi = spawnExplGens.begin(); egi != spawnExplGens.end(); ++egi) {
		handler->UnloadGenerator(*egi);
	}
}

void CCustomExplosionGenerator::ClearCache()
{
	spawnExplGens.clear();
	explosionIDs.clear();
	explosionData.clear();
}

