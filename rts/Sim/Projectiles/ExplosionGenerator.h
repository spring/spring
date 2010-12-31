/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPLOSION_GENERATOR_H
#define EXPLOSION_GENERATOR_H

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Lua/LuaParser.h"
#include "Sim/Objects/WorldObject.h"

class float3;
class CUnit;
class CExplosionGenerator;


class CExpGenSpawnable: public CWorldObject
{
	CR_DECLARE(CExpGenSpawnable);
public:
	CExpGenSpawnable();
	CExpGenSpawnable(const float3& pos);
	virtual ~CExpGenSpawnable() {}

	virtual void Init(const float3& pos, CUnit* owner) = 0;
};


class ClassAliasList
{
public:
	ClassAliasList();

	void Load(const LuaTable&);
	creg::Class* GetClass(const std::string& name);
	std::string FindAlias(const std::string& className);

protected:
	std::map<std::string, std::string> aliases;
};


class CExplosionGeneratorHandler
{
public:
	CExplosionGeneratorHandler();

	CExplosionGenerator* LoadGenerator(const std::string& tag);
	const LuaTable& GetTable() const { return luaTable; }

	ClassAliasList projectileClasses, generatorClasses;

protected:
	std::map<std::string, CExplosionGenerator*> generators;
	LuaParser luaParser;
	LuaTable  luaTable;
};


class CExplosionGenerator
{
	CR_DECLARE(CExplosionGenerator);

public:
	CExplosionGenerator();
	virtual ~CExplosionGenerator();

	virtual void Explosion(const float3& pos, float damage, float radius, CUnit* owner, float gfxMod, CUnit* hit, const float3& dir) = 0;
	virtual void Load(CExplosionGeneratorHandler* loader, const std::string& tag) = 0;
};


class CStdExplosionGenerator: public CExplosionGenerator
{
	CR_DECLARE(CStdExplosionGenerator);

public:
	CStdExplosionGenerator();
	virtual ~CStdExplosionGenerator();

	void Explosion(const float3& pos, float damage, float radius, CUnit* owner, float gfxMod, CUnit* hit, const float3& dir);
	void Load(CExplosionGeneratorHandler* loader, const std::string& tag);
};


/** Defines the result of an explosion as a series of new projectiles */
class CCustomExplosionGenerator: public CStdExplosionGenerator
{
	CR_DECLARE(CCustomExplosionGenerator);

protected:
	struct ProjectileSpawnInfo {
		ProjectileSpawnInfo()
			: projectileClass(NULL)
			, count(0)
			, flags(0)
		{}

		creg::Class* projectileClass;
		std::vector<char> code;
		/// number of projectiles spawned of this type
		int count;
		unsigned int flags;
	};

	// TODO: Handle ground flashes with more flexibility like the projectiles
	struct GroundFlashInfo {
		GroundFlashInfo()
			: flashSize(0.0f)
			, flashAlpha(0.0f)
			, circleGrowth(0.0f)
			, circleAlpha(0.0f)
			, ttl(0)
			, color(0.0f, 0.0f, 0.0f)
			, flags(0)
		{}

		float flashSize;
		float flashAlpha;
		float circleGrowth;
		float circleAlpha;
		int ttl;
		float3 color;
		unsigned int flags;
	};

	struct CEGData {
		std::vector<ProjectileSpawnInfo> projectileSpawn;
		GroundFlashInfo groundFlash;
		bool useDefaultExplosions;
	};

	std::map<string, CEGData> cachedCEGs;
	std::map<string, CEGData>::iterator currentCEG;

	void ParseExplosionCode(ProjectileSpawnInfo* psi, int baseOffset, boost::shared_ptr<creg::IType> type, const std::string& script, std::string& code);
	void ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3& dir);

public:
	CCustomExplosionGenerator();
	~CCustomExplosionGenerator();

	static void OutputProjectileClassInfo();

	/// @throws content_error/runtime_error on errors
	void Load(CExplosionGeneratorHandler* loader, const std::string& tag);
	void Explosion(const float3& pos, float damage, float radius, CUnit* owner, float gfxMod, CUnit* hit, const float3& dir);
};


extern CExplosionGeneratorHandler* explGenHandler;


#endif // EXPLOSION_GENERATOR_H
