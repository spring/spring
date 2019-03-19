/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPLOSION_GENERATOR_H
#define EXPLOSION_GENERATOR_H

#include <string>
#include <vector>

#include "Rendering/GroundFlashInfo.h"
#include "System/UnorderedMap.hpp"

#define CEG_PREFIX_STRING "custom:"

class LuaParser;
class LuaTable;
class float3;
class CUnit;
class IExplosionGenerator;

struct SExpGenSpawnableMemberInfo;

// Finds C++ classes with class aliases
class ClassAliasList
{
public:
	void Load(const LuaTable&);
	void Clear() { aliases.clear(); }

	std::string ResolveAlias(const std::string& alias) const;
	std::string FindAlias(const std::string& className) const;

private:
	spring::unordered_map<std::string, std::string> aliases;
};



// loads and stores a list of explosion generators
class CExplosionGeneratorHandler
{
public:
	enum {
		EXPGEN_ID_INVALID  = -1u,
		EXPGEN_ID_STANDARD =  0u,
	};

	void Init();
	void Kill();

	void ParseExplosionTables();
	void ReloadGenerators(const std::string&);

	unsigned int LoadCustomGeneratorID(const char* tag) { return (LoadGeneratorID(tag, CEG_PREFIX_STRING)); }
	unsigned int LoadGeneratorID(const char* tag, const char* pre = "");

	IExplosionGenerator* LoadGenerator(const char* tag, const char* pre = "");
	IExplosionGenerator* GetGenerator(unsigned int expGenID);

	bool GenExplosion(
		unsigned int expGenID,
		const float3& pos,
		const float3& dir,
		float damage,
		float radius,
		float gfxMod,
		CUnit* owner,
		CUnit* hit
	);

	const LuaTable* GetExplosionTableRoot() const { return explTblRoot; }
	const ClassAliasList& GetProjectileClasses() const { return projectileClasses; }

protected:
	ClassAliasList projectileClasses;

	LuaParser* exploParser = nullptr;
	LuaParser* aliasParser = nullptr;
	LuaTable*  explTblRoot = nullptr;

	std::vector<IExplosionGenerator*> explosionGenerators;

	spring::unordered_map<unsigned int, unsigned int> expGenHashIdentMap; // hash->id
	spring::unordered_map<unsigned int, std::array<char, 64>> expGenIdentNameMap; // id->name
};




// Base explosion generator class
class IExplosionGenerator
{
public:
	IExplosionGenerator(): generatorID(CExplosionGeneratorHandler::EXPGEN_ID_INVALID) {}
	virtual ~IExplosionGenerator() {}

	virtual bool Load(CExplosionGeneratorHandler* handler, const char* tag) = 0;
	virtual bool Reload(CExplosionGeneratorHandler* handler, const char* tag) { return true; }
	virtual bool Explosion(
		const float3& pos,
		const float3& dir,
		float damage,
		float radius,
		float gfxMod,
		CUnit* owner,
		CUnit* hit
	) = 0;

	unsigned int GetGeneratorID() const { return generatorID; }
	void SetGeneratorID(unsigned int id) { generatorID = id; }

protected:
	unsigned int generatorID;
};


// spawns non-scriptable explosion effects via hardcoded rules
// has no internal state so we never need to allocate instances
class CStdExplosionGenerator: public IExplosionGenerator
{
public:
	CStdExplosionGenerator(): IExplosionGenerator() {}

	bool Load(CExplosionGeneratorHandler* handler, const char* tag) override { return false; }
	bool Explosion(
		const float3& pos,
		const float3& dir,
		float damage,
		float radius,
		float gfxMod,
		CUnit* owner,
		CUnit* hit
	) override;
};


// Uses explosion info from a script file; defines the
// result of an explosion as a series of new projectiles
class CCustomExplosionGenerator: public IExplosionGenerator
{
protected:
	struct ProjectileSpawnInfo {
		ProjectileSpawnInfo()
			: spawnableID(0)
			, count(0)
			, flags(0)
		{}
		ProjectileSpawnInfo(const ProjectileSpawnInfo& psi)
			: spawnableID(psi.spawnableID)
			, code(psi.code)
			, count(psi.count)
			, flags(psi.flags)
		{}

		unsigned int spawnableID;

		/// parsed explosion script code
		std::vector<char> code;

		/// number of projectiles spawned of this type
		unsigned int count;
		unsigned int flags;
	};

	struct ExpGenParams {
		std::vector<ProjectileSpawnInfo> projectiles;

		GroundFlashInfo groundFlash;

		bool useDefaultExplosions;
	};

public:
	CCustomExplosionGenerator(): IExplosionGenerator() {}

	static bool OutputProjectileClassInfo();
	static unsigned int GetFlagsFromTable(const LuaTable& table);
	static unsigned int GetFlagsFromHeight(float height, float groundHeight);

	/// @throws content_error/runtime_error on errors
	bool Load(CExplosionGeneratorHandler* handler, const char* tag) override;
	bool Reload(CExplosionGeneratorHandler* handler, const char* tag) override;
	bool Explosion(const float3& pos, const float3& dir, float damage, float radius, float gfxMod, CUnit* owner, CUnit* hit) override;

	// spawn-flags
	enum {
		CEG_SPWF_WATER      = 1 << 0,
		CEG_SPWF_GROUND     = 1 << 1,
		CEG_SPWF_VOIDWATER  = 1 << 2,
		CEG_SPWF_VOIDGROUND = 1 << 3,
		CEG_SPWF_AIR        = 1 << 4,
		CEG_SPWF_UNDERWATER = 1 << 5,  // TODO: UNDERVOIDWATER?
		CEG_SPWF_UNIT       = 1 << 6,  // only execute when the explosion hits a unit
		CEG_SPWF_NO_UNIT    = 1 << 7,  // only execute when the explosion doesn't hit a unit (environment)
	};

	enum {
		OP_END      =  0,
		OP_STOREI   =  1, // int
		OP_STOREF   =  2, // float
		OP_ADD      =  4,
		OP_RAND     =  5,
		OP_DAMAGE   =  6,
		OP_INDEX    =  7,
		OP_LOADP    =  8, // load a void* into the pointer register
		OP_STOREP   =  9, // store the pointer register into a void*
		OP_DIR      = 10, // store the float3 direction
		OP_SAWTOOTH = 11, // Performs a modulo to create a sawtooth wave
		OP_DISCRETE = 12, // Floors the value to a multiple of its parameter
		OP_SINE     = 13, // Uses val as the phase of a sine wave
		OP_YANK     = 14, // Moves the input value into a buffer, returns zero
		OP_MULTIPLY = 15, // Multiplies with buffer value
		OP_ADDBUFF  = 16, // Adds buffer value
		OP_POW      = 17, // Power with code as exponent
		OP_POWBUFF  = 18, // Power with buffer as exponent
	};

private:
	void ParseExplosionCode(ProjectileSpawnInfo* psi, const std::string& script, SExpGenSpawnableMemberInfo& memberInfo, std::string& code);
	void ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3& dir);

protected:
	ExpGenParams expGenParams;
};


extern CExplosionGeneratorHandler explGenHandler;

#endif // EXPLOSION_GENERATOR_H
