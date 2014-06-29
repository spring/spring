/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPLOSION_GENERATOR_H
#define EXPLOSION_GENERATOR_H

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Sim/Objects/WorldObject.h"

#define CEG_PREFIX_STRING "custom:"

class LuaParser;
class LuaTable;
class float3;
class CUnit;
class IExplosionGenerator;


class CExpGenSpawnable: public CWorldObject
{
	CR_DECLARE(CExpGenSpawnable);
public:
	CExpGenSpawnable();
	CExpGenSpawnable(const float3& pos, const float3& spd);

	virtual ~CExpGenSpawnable() {}
	virtual void Init(const CUnit* owner, const float3& offset) = 0;
};



// Finds C++ classes with class aliases
class ClassAliasList
{
public:
	void Load(const LuaTable&);
	void Clear() { aliases.clear(); }

	creg::Class* GetClass(const std::string& name) const;
	std::string FindAlias(const std::string& className) const;

private:
	std::map<std::string, std::string> aliases;
};



// loads and stores a list of explosion generators
class CExplosionGeneratorHandler
{
public:
	enum {
		EXPGEN_ID_INVALID  = -1u,
		EXPGEN_ID_STANDARD =  0u,
	};

	CExplosionGeneratorHandler();
	~CExplosionGeneratorHandler();

	void ParseExplosionTables();
	void ReloadGenerators(const std::string&);

	unsigned int LoadGeneratorID(const std::string& tag);
	IExplosionGenerator* LoadGenerator(const std::string& tag);
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
	const ClassAliasList& GetGeneratorClasses() const { return generatorClasses; }

protected:
	ClassAliasList projectileClasses;
	ClassAliasList generatorClasses;

	LuaParser* exploParser;
	LuaParser* aliasParser;
	LuaTable*  explTblRoot;

	std::vector<IExplosionGenerator*> explosionGenerators;

	std::map<std::string, unsigned int> expGenTagIdentMap;
	std::map<unsigned int, std::string> expGenIdentTagMap;

	typedef std::map<std::string, unsigned int>::const_iterator TagIdentMapConstIt;
	typedef std::map<unsigned int, std::string>::const_iterator IdentTagMapConstIt;
};




// Base explosion generator class
class IExplosionGenerator
{
	CR_DECLARE(IExplosionGenerator);

	IExplosionGenerator(): generatorID(CExplosionGeneratorHandler::EXPGEN_ID_INVALID) {}
	virtual ~IExplosionGenerator() {}

	virtual bool Load(CExplosionGeneratorHandler* handler, const std::string& tag) = 0;
	virtual bool Reload(CExplosionGeneratorHandler* handler, const std::string& tag) { return true; }
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
	CR_DECLARE(CStdExplosionGenerator);

public:
	CStdExplosionGenerator(): IExplosionGenerator() {}

	bool Load(CExplosionGeneratorHandler* handler, const std::string& tag) { return false; }
	bool Explosion(
		const float3& pos,
		const float3& dir,
		float damage,
		float radius,
		float gfxMod,
		CUnit* owner,
		CUnit* hit
	);
};


// Uses explosion info from a script file; defines the
// result of an explosion as a series of new projectiles
class CCustomExplosionGenerator: public IExplosionGenerator
{
	CR_DECLARE(CCustomExplosionGenerator);
	CR_DECLARE_SUB(ProjectileSpawnInfo);
	CR_DECLARE_SUB(GroundFlashInfo);
	CR_DECLARE_SUB(ExpGenParams);

protected:
	struct ProjectileSpawnInfo {
		CR_DECLARE_STRUCT(ProjectileSpawnInfo);

		ProjectileSpawnInfo()
			: projectileClass(NULL)
			, count(0)
			, flags(0)
		{}
		ProjectileSpawnInfo(const ProjectileSpawnInfo& psi)
			: projectileClass(psi.projectileClass)
			, code(psi.code)
			, count(psi.count)
			, flags(psi.flags)
		{}

		creg::Class* projectileClass;

		/// parsed explosion script code
		std::vector<char> code;

		/// number of projectiles spawned of this type
		unsigned int count;
		unsigned int flags;
	};

	// TODO: Handle ground flashes with more flexibility like the projectiles
	struct GroundFlashInfo {
		CR_DECLARE_STRUCT(GroundFlashInfo);

		GroundFlashInfo()
			: flashSize(0.0f)
			, flashAlpha(0.0f)
			, circleGrowth(0.0f)
			, circleAlpha(0.0f)
			, ttl(0)
			, flags(0)
			, color(ZeroVector)
		{}

		float flashSize;
		float flashAlpha;
		float circleGrowth;
		float circleAlpha;
		int ttl;
		unsigned int flags;
		float3 color;
	};

	struct ExpGenParams {
		CR_DECLARE_STRUCT(ExpGenParams);

		std::vector<ProjectileSpawnInfo> projectiles;

		GroundFlashInfo groundFlash;

		bool useDefaultExplosions;
	};

public:
	CCustomExplosionGenerator(): IExplosionGenerator() {}

	static bool OutputProjectileClassInfo();
	static unsigned int GetFlagsFromTable(const LuaTable& table);
	static unsigned int GetFlagsFromHeight(float height, float altitude);

	/// @throws content_error/runtime_error on errors
	bool Load(CExplosionGeneratorHandler* handler, const std::string& tag);
	bool Reload(CExplosionGeneratorHandler* handler, const std::string& tag);
	bool Explosion(const float3& pos, const float3& dir, float damage, float radius, float gfxMod, CUnit* owner, CUnit* hit);


	enum {
		SPW_WATER      =  1,
		SPW_GROUND     =  2,
		SPW_AIR        =  4,
		SPW_UNDERWATER =  8,
		SPW_UNIT       = 16,  // only execute when the explosion hits a unit
		SPW_NO_UNIT    = 32,  // only execute when the explosion doesn't hit a unit (environment)
	};

	enum {
		OP_END      =  0,
		OP_STOREI   =  1, // int
		OP_STOREF   =  2, // float
		OP_STOREC   =  3, // char
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
	void ParseExplosionCode(ProjectileSpawnInfo* psi, const int offset, const boost::shared_ptr<creg::IType> type, const std::string& script, std::string& code);
	void ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3& dir);

protected:
	ExpGenParams expGenParams;
};


extern CExplosionGeneratorHandler* explGenHandler;

#endif // EXPLOSION_GENERATOR_H
