/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include <fstream>
#include <stdexcept>
#include <cassert>
#include <boost/cstdint.hpp>

#include "ExplosionGenerator.h"
#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/myGL.h"
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

#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"
#include "System/creg/VarTypes.h"
#include "System/FileSystem/FileHandler.h"

CR_BIND_DERIVED_INTERFACE(CExpGenSpawnable, CWorldObject);
CR_REG_METADATA(CExpGenSpawnable, );

CR_BIND_INTERFACE(IExplosionGenerator);
CR_BIND_DERIVED(CStdExplosionGenerator, IExplosionGenerator, );
CR_BIND_DERIVED(CCustomExplosionGenerator, CStdExplosionGenerator, );

CExplosionGeneratorHandler* explGenHandler = NULL;
CCustomExplosionGenerator* gCEG = NULL;

CExpGenSpawnable::CExpGenSpawnable(): CWorldObject() { GML_EXPGEN_CHECK() }
CExpGenSpawnable::CExpGenSpawnable(const float3& pos): CWorldObject(pos) { GML_EXPGEN_CHECK() }



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
	exploParser = NULL;
	aliasParser = NULL;
	explTblRoot = NULL;

	ParseExplosionTables();
}

CExplosionGeneratorHandler::~CExplosionGeneratorHandler()
{
	delete exploParser; exploParser = NULL;
	delete aliasParser; aliasParser = NULL;
	delete explTblRoot; explTblRoot = NULL;
}

void CExplosionGeneratorHandler::ParseExplosionTables() {
	delete exploParser;
	delete aliasParser;
	delete explTblRoot;

	exploParser = new LuaParser("gamedata/explosions.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	aliasParser = new LuaParser("gamedata/explosion_alias.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	explTblRoot = NULL;

	if (!aliasParser->Execute()) {
		logOutput.Print(aliasParser->GetErrorLog());
	} else {
		const LuaTable& aliasRoot = aliasParser->GetRoot();

		projectileClasses.Clear();
		projectileClasses.Load(aliasRoot.SubTable("projectiles"));
		generatorClasses.Clear();
		generatorClasses.Load(aliasRoot.SubTable("generators"));
	}

	if (!exploParser->Execute()) {
		logOutput.Print(exploParser->GetErrorLog());
	} else {
		explTblRoot = new LuaTable(exploParser->GetRoot());
	}
}

IExplosionGenerator* CExplosionGeneratorHandler::LoadGenerator(const string& tag)
{
	string klass;
	string::size_type seppos = tag.find(':');

	if (seppos == string::npos) {
		klass = tag;
	} else {
		klass = tag.substr(0, seppos);
	}

	creg::Class* cls = generatorClasses.GetClass(klass);

	if (!cls->IsSubclassOf(IExplosionGenerator::StaticClass())) {
		throw content_error(klass + " is not a subclass of IExplosionGenerator");
	}

	IExplosionGenerator* eg = (IExplosionGenerator*) cls->CreateInstance();

	if (seppos != string::npos) {
		eg->Load(this, tag.substr(seppos + 1));
	}

	return eg;
}






bool CStdExplosionGenerator::Explosion(
	unsigned int explosionID,
	const float3& pos,
	float damage,
	float radius,
	CUnit* owner,
	float gfxMod,
	CUnit* hit,
	const float3 &dir
) {
	const float h2 = ground->GetHeightReal(pos.x, pos.z);
	const float height = std::max(0.0f, pos.y - h2);

	const bool waterExplosion = (h2 < -3.0f);
	const bool uwExplosion = (pos.y < -15.0f);

	damage = damage / 20.0f;

	// limit the visual effects based on the radius
	if (damage > radius * 1.5f) {
		damage = radius * 1.5f;
	}

	damage *= gfxMod;


	float3 camVect = camera->pos - pos;
	const float camLength = camVect.Length();
	float moveLength = radius * 0.03f;

	if (camLength > 0.0f) { camVect /= camLength; }
	if (camLength < moveLength + 2) { moveLength = camLength - 2; }

	const float3 npos = pos + camVect * moveLength;

	new CHeatCloudProjectile(npos, float3(0.0f, 0.3f, 0.0f), 8 + sqrt(damage) * 0.5f, 7 + damage * 2.8f, owner);

	if (ph->particleSaturation < 1.0f) {
		const bool airExplosion = (pos.y - std::max(0.0f, h2) > 20.0f);

		// turn off lots of graphic only particles when we have more particles than we want
		float smokeDamage = damage;

		if (uwExplosion) {
			smokeDamage *= 0.3f;
		}
		if (airExplosion || waterExplosion) {
			smokeDamage *= 0.6f;
		}

		float invSqrtsmokeDamage = 1.0f;
		if (smokeDamage > 0.0f) {
			invSqrtsmokeDamage /= (sqrt(smokeDamage) * 0.35f);
		}

		for (int a = 0; a < smokeDamage * 0.6f; ++a) {
			const float3 speed(
				(-0.1f + gu->usRandFloat() * 0.2f),
				( 0.1f + gu->usRandFloat() * 0.3f) * invSqrtsmokeDamage,
				(-0.1f + gu->usRandFloat() * 0.2f)
			);

			const float h = ground->GetApproximateHeight(npos.x, npos.z);
			const float time = (40 + sqrt(smokeDamage) * 15) * (0.8f + gu->usRandFloat() * 0.7f);

			float3 npos(pos + gu->usRandVector() * (smokeDamage * 1.0f));
			if (npos.y < h) {
				npos.y = h;
			}

			new CSmokeProjectile2(pos, npos, speed, time, sqrt(smokeDamage) *4, 0.4f, owner, 0.6f);
		}

		if (!airExplosion && !uwExplosion && !waterExplosion) {
			const int numDirt = std::min(20.0f, damage * 0.8f);
			const float3 color(0.15f, 0.1f, 0.05f);

			for (int a = 0; a < numDirt; ++a) {
				float3 speed(
					(0.5f - gu->usRandFloat()) * 1.5f,
					 1.7f + gu->usRandFloat()  * 1.6f,
					(0.5f - gu->usRandFloat()) * 1.5f
				);
				speed *= (0.7f + std::min(30.0f, damage) / 30.0f);

				const float3 npos(
					pos.x - (0.5f - gu->usRandFloat()) * (radius * 0.6f),
					pos.y -  2.0f - damage * 0.2f,
					pos.z - (0.5f - gu->usRandFloat()) * (radius * 0.6f)
				);

				new CDirtProjectile(npos, speed, 90 + damage * 2, 2.0f + sqrt(damage) * 1.5f, 0.4f, 0.999f, owner, color);
			}
		}
		if (!airExplosion && !uwExplosion && waterExplosion) {
			int numDirt = std::min(40.f, damage*0.8f);
			float3 color(1.0f, 1.0f, 1.0f);
			for (int a = 0; a < numDirt; ++a) {
				float3 speed((0.5f - gu->usRandFloat()) * 0.2f, a * 0.1f + gu->usRandFloat()*0.8f, (0.5f - gu->usRandFloat()) * 0.2f);
				speed *= 0.7f + std::min((float)30, damage) / 30;
				float3 npos(pos.x-(0.5f-gu->usRandFloat())*(radius*0.2f),pos.y-2.0f-sqrt(damage)*2.0f,pos.z-(0.5f-gu->usRandFloat())*(radius*0.2f));
				new CDirtProjectile(npos, speed, 90 + damage*2, 2.0f + sqrt(damage)*2.0f, 0.3f, 0.99f, owner, color);
			}
		}
		if (damage>=20 && !uwExplosion && !airExplosion) {
			int numDebris = gu->usRandInt() % 6;
			if (numDebris > 0) {
				numDebris += 3 + (int) (damage * 0.04f);
			}
			for (int a = 0; a < numDebris; ++a) {
				float3 speed;
				if (height < 4) {
					speed=float3((0.5f-gu->usRandFloat())*2.0f,1.8f+gu->usRandFloat()*1.8f,(0.5f-gu->usRandFloat())*2.0f);
				} else {
					speed = float3(gu->usRandVector() * 2);
				}
				speed *= 0.7f + std::min(30.0f, damage) / 23;
				float3 npos(pos.x - (0.5f - gu->usRandFloat()) * (radius * 1), pos.y, pos.z - (0.5f - gu->usRandFloat()) * (radius * 1));
				new CWreckProjectile(npos, speed, 90 + damage*2, owner);
			}
		}
		if (uwExplosion) {
			int numBubbles = (int) (damage * 0.7f);
			for (int a = 0 ;a < numBubbles; ++a) {
				new CBubbleProjectile(pos + gu->usRandVector()*radius*0.5f,
						gu->usRandVector()*0.2f + float3(0.0f, 0.2f, 0.0f),
						damage*2 + gu->usRandFloat()*damage,
						1 + gu->usRandFloat()*2,
						0.02f,
						owner,
						0.5f + gu->usRandFloat() * 0.3f);
			}
		}
		if (waterExplosion && !uwExplosion && !airExplosion) {
			int numWake = (int) (damage * 0.5f);
			for (int a = 0; a < numWake; ++a) {
				new CWakeProjectile(pos + gu->usRandVector()*radius*0.2f,
						gu->usRandVector()*radius*0.003f,
						sqrt(damage) * 4,
						damage * 0.03f,
						owner,
						0.3f + gu->usRandFloat()*0.2f,
						0.8f / (sqrt(damage)*3 + 50 + gu->usRandFloat()*90),
						1);
			}
		}
		if (radius>10 && damage>4) {
			int numSpike = (int) sqrt(damage) + 8;
			for (int a = 0; a < numSpike; ++a) {
				float3 speed = gu->usRandVector();
				speed.Normalize();
				speed *= (8 + damage*3.0f) / (9 + sqrt(damage)*0.7f) * 0.35f;
				if (!airExplosion && !waterExplosion && (speed.y < 0)) {
					speed.y=-speed.y;
				}
				new CExploSpikeProjectile(pos + speed,
						speed * (0.9f + gu->usRandFloat()*0.4f),
						radius * 0.1f,
						radius * 0.1f,
						0.6f,
						0.8f / (8 + sqrt(damage)),
						owner);
			}
		}
	}

	if (radius > 20 && damage > 6 && height < radius * 0.7f) {
		float modSize = std::max(radius, damage * 2);
		float circleAlpha = 0;
		float circleGrowth = 0;
		float ttl = 8 + sqrt(damage)*0.8f;
		if (radius > 40 && damage > 12) {
			circleAlpha = std::min(0.5f, damage * 0.01f);
			circleGrowth = (8 + damage*2.5f) / (9 + sqrt(damage)*0.7f) * 0.55f;
		}
		float flashSize = modSize;
		float flashAlpha = std::min(0.8f, damage * 0.01f);
		new CStandardGroundFlash(pos, circleAlpha, flashAlpha, flashSize, circleGrowth, ttl);
	}

	if (radius > 40 && damage > 12) {
		CSpherePartProjectile::CreateSphere(pos,
				std::min(0.7f, damage * 0.02f),
				5 + (int) (sqrt(damage) * 0.7f),
				(8 + damage*2.5f) / (9 + sqrt(damage)*0.7f) * 0.5f,
				owner);
	}

	return true;
}



void CCustomExplosionGenerator::ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3 &dir)
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
				val += gu->usRandFloat() * (*(float*) code);
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
				*(float3*) (instance + offset) = dir;
				break;
			}
			case OP_SAWTOOTH: {
				// this translates to modulo except it works with floats
				val -= (*(float*) code) * floor(val / (*(float*) code));
				code += 4;
				break;
			}
			case OP_DISCRETE: {
				val = (*(float*) code) * floor(val / (*(float*) code));
				code += 4;
				break;
			}
			case OP_SINE: {
				val = (*(float*) code) * sin(val);
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
				val = pow(val, (*(float*) code));
				code += 4;
				break;
			}
			case OP_POWBUFF: {
				val = pow(val, buffer[(*(int*) code)]);
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
	int offset, boost::shared_ptr<creg::IType> type, const string& script, string& code)
{

	string::size_type end = script.find(';', 0);
	string vastr = script.substr(0, end);


	if (vastr == "dir") { // first see if we can match any keywords
		// if the user uses a keyword assume he knows that it is put on the right datatype for now
		code += OP_DIR;
		boost::uint16_t ofs = offset;
		code.append((char*) &ofs, (char*) &ofs + 2);
	}
	else if (dynamic_cast<creg::BasicType*>(type.get())) {
		creg::BasicType *bt = (creg::BasicType*)type.get();

		if (bt->id != creg::crInt && bt->id != creg::crFloat && bt->id != creg::crUChar && bt->id != creg::crBool) {
			throw content_error("Projectile properties other than int, float and uchar, are not supported (" + script + ")");
			return;
		}

		int p = 0;
		while (p < script.length()) {
			char opcode;
			char c;
			do { c = script[p++]; } while (c == ' ');

			bool useInt = false;

			if (c == 'i')      opcode = OP_INDEX;
			else if (c == 'r') opcode = OP_RAND;
			else if (c == 'd') opcode = OP_DAMAGE;
			else if (c == 'm') opcode = OP_SAWTOOTH;
			else if (c == 'k') opcode = OP_DISCRETE;
			else if (c == 's') opcode = OP_SINE;
			else if (c == 'y') {opcode = OP_YANK; useInt = true;}
			else if (c == 'x') {opcode = OP_MULTIPLY; useInt = true;}
			else if (c == 'a') {opcode = OP_ADDBUFF; useInt = true;}
			else if (c == 'p') opcode = OP_POW;
			else if (c == 'q') {opcode = OP_POWBUFF; useInt = true;}
			else if (isdigit(c) || c == '.' || c == '-') { opcode = OP_ADD; p--; }
			else {
				logOutput.Print("[CCEG::ParseExplosionCode] WARNING: unknown op-code \"" + string(1, c) + "\" in \"" + script + "\"");
				continue;
			}

			char* endp;
			if (!useInt) {
				const float v = (float)strtod(&script[p], &endp);

				p += endp - &script[p];
				code += opcode;
				code.append((char*) &v, ((char*) &v) + 4);
			} else {
				const int v = std::max(0, std::min(16, (int)strtol(&script[p], &endp, 10)));

				p += endp - &script[p];
				code += opcode;
				code.append((char*) &v, ((char*) &v) + 4);
			}
		}

		switch (bt->id) {
			case creg::crInt: code.push_back(OP_STOREI); break;
			case creg::crBool: code.push_back(OP_STOREI); break;
			case creg::crFloat: code.push_back(OP_STOREF); break;
			case creg::crUChar: code.push_back(OP_STOREC); break;
			default:
				throw content_error("Explosion script variable is of unsupported type. "
					"Contact the Spring team to fix this.");
					break;
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
			void* tex = projectileDrawer->textureAtlas->GetTexturePtr(texname);
			code += OP_LOADP;
			code.append((char*)(&tex), ((char*)(&tex)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		} else if (type->GetName() == "GroundFXTexture*") {
			string::size_type end = script.find(';', 0);
			string texname = script.substr(0, end);
			void* tex = projectileDrawer->groundFXAtlas->GetTexturePtr(texname);
			code += OP_LOADP;
			code.append((char*)(&tex), ((char*)(&tex)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		} else if (type->GetName() == "CColorMap*") {
			string::size_type end = script.find(';', 0);
			string colorstring = script.substr(0, end);
			void* colormap = CColorMap::LoadFromDefString(colorstring);
			code += OP_LOADP;
			code.append((char*)(&colormap), ((char*)(&colormap)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		} else if (type->GetName() == "IExplosionGenerator*") {
			string::size_type end = script.find(';', 0);
			string name = script.substr(0, end);
			void* explgen = explGenHandler->LoadGenerator(name);
			code += OP_LOADP;
			code.append((char*)(&explgen), ((char*)(&explgen)) + sizeof(void*));
			code += OP_STOREP;
			boost::uint16_t ofs = offset;
			code.append((char*)&ofs, (char*)&ofs + 2);
		}
	}
}


static unsigned int GetFlagsFromTable(const LuaTable& table)
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


unsigned int CCustomExplosionGenerator::Load(CExplosionGeneratorHandler* h, const string& tag)
{
	unsigned int explosionID = -1U;

	if (tag.empty()) {
		return explosionID;
	}

	const std::map<std::string, unsigned int>::const_iterator it = explosionIDs.find(tag);

	if (it == explosionIDs.end()) {
		CEGData cegData;

		const LuaTable* root = h->GetExplosionTableRoot();
		const LuaTable& expTable = (root != NULL)? root->SubTable(tag): LuaTable();

		if (!expTable.IsValid()) {
			// not a fatal error: any calls to ::Explosion will just return early
			logOutput.Print("[CCEG::Load] WARNING: table for CEG \"" + tag + "\" invalid (parse errors?)");
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

			psi.projectileClass = h->projectileClasses.GetClass(className);
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

void CCustomExplosionGenerator::RefreshCache(const std::string& tag) {
	// re-parse the projectile and generator tables
	explGenHandler->ParseExplosionTables();

	if (tag.empty()) {
		std::map<std::string, unsigned int> oldExplosionIDs(explosionIDs);
		std::map<std::string, unsigned int>::const_iterator it;

		ClearCache();

		// reload all currently cached CEGs by tag
		// (ID's of active CEGs will remain valid)
		for (it = oldExplosionIDs.begin(); it != oldExplosionIDs.end(); ++it) {
			const std::string& tmpTag = it->first;

			logOutput.Print("[%s] reloading CEG \"%s\" (ID %u)", __FUNCTION__, tmpTag.c_str(), it->second);
			Load(explGenHandler, tmpTag);
		}
	} else {
		// reload a single CEG
		const std::map<std::string, unsigned int>::const_iterator it = explosionIDs.find(tag);

		if (it == explosionIDs.end()) {
			logOutput.Print("[%s] unknown CEG-tag \"%s\"", __FUNCTION__, tag.c_str());
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

		logOutput.Print("[%s] reloading single CEG \"%s\" (ID %u)", __FUNCTION__, tag.c_str(), cegIndex);

		if (Load(explGenHandler, tag) == -1U) {
			logOutput.Print("[%s] failed to reload single CEG \"%s\" (ID %u)", __FUNCTION__, tag.c_str(), cegIndex);

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

	float h2 = ground->GetHeightReal(pos.x, pos.z);
	unsigned int flags = 0;

	if (pos.y - std::max(0.0f, h2) > 20.0f) flags = SPW_AIR;
	else if (h2 < -3.0f)                    flags = SPW_WATER;
	else if (pos.y < -15.0f)                flags = SPW_UNDERWATER;
	else                                    flags = SPW_GROUND;

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
		if (ph->particleSaturation > 1 && !(psi.flags & SPW_SYNCED)) {
			continue;
		}

		for (int c = 0; c < psi.count; c++) {
			CExpGenSpawnable* projectile = (CExpGenSpawnable*) (psi.projectileClass)->CreateInstance();

			ExecuteExplosionCode(&psi.code[0], damage, (char*) projectile, c, dir);
			projectile->Init(pos, owner);
		}
	}

	if ((flags & SPW_GROUND) && (groundFlash.ttl > 0)) {
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
