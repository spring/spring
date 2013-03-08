/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPLOSION_GENERATOR_H
#define EXPLOSION_GENERATOR_H

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Sim/Objects/WorldObject.h"

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
	CExpGenSpawnable(const float3& pos);
	virtual ~CExpGenSpawnable() {}

	virtual void Init(const float3& pos, CUnit* owner) = 0;
};



//! Finds C++ classes with class aliases
class ClassAliasList
{
public:
	void Load(const LuaTable&);
	void Clear() { aliases.clear(); }

	creg::Class* GetClass(const std::string& name);
	std::string FindAlias(const std::string& className);

private:
	std::map<std::string, std::string> aliases;
};



//! loads and stores a list of explosion generators
class CExplosionGeneratorHandler
{
public:
	CExplosionGeneratorHandler();
	~CExplosionGeneratorHandler();

	void ParseExplosionTables();
	IExplosionGenerator* LoadGenerator(const std::string& tag);
	void UnloadGenerator(IExplosionGenerator* explGen);
	const LuaTable* GetExplosionTableRoot() const { return explTblRoot; }
	void ReloadGenerators(const std::string&);

	ClassAliasList projectileClasses;
	ClassAliasList generatorClasses;

protected:
	LuaParser* exploParser;
	LuaParser* aliasParser;
	LuaTable*  explTblRoot;

	// number of times a CEG instance was requested via LoadGenerator()
	// used to assign a unique ID to each CEG (zero is reserved for gCEG)
	unsigned int numLoadedGenerators;

	std::map<unsigned int, IExplosionGenerator*> explosionGenerators;
};




//! Base explosion generator class
class IExplosionGenerator
{
	CR_DECLARE(IExplosionGenerator);

	IExplosionGenerator(): generatorID(0) {}
	virtual ~IExplosionGenerator() {}

	virtual unsigned int Load(CExplosionGeneratorHandler* handler, const std::string& tag) = 0;
	virtual void Reload(CExplosionGeneratorHandler* handler, const std::string& tag) {}
	virtual void Unload(CExplosionGeneratorHandler* handler) {}
	virtual bool Explosion(unsigned int explosionID, const float3& pos, float damage, float radius, CUnit* owner, float gfxMod, CUnit* hit, const float3& dir) = 0;

	unsigned int GetGeneratorID() const { return generatorID; }
	void SetGeneratorID(unsigned int id) { generatorID = id; }

protected:
	unsigned int generatorID;
};


//! everything is calculated from damage and radius
class CStdExplosionGenerator: public IExplosionGenerator
{
	CR_DECLARE(CStdExplosionGenerator);

public:
	CStdExplosionGenerator(): IExplosionGenerator() {}
	virtual ~CStdExplosionGenerator() {}

	unsigned int Load(CExplosionGeneratorHandler* handler, const std::string& tag) { return -1U; }
	bool Explosion(unsigned int explosionID, const float3& pos, float damage, float radius, CUnit* owner, float gfxMod, CUnit* hit, const float3& dir);
};


//! Uses explosion info from a script file; defines the
//! result of an explosion as a series of new projectiles
class CCustomExplosionGenerator: public CStdExplosionGenerator
{
	CR_DECLARE(CCustomExplosionGenerator);
	CR_DECLARE_SUB(ProjectileSpawnInfo);
	CR_DECLARE_SUB(GroundFlashInfo);
	CR_DECLARE_SUB(CEGData);

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
		int count;
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
		CR_DECLARE_STRUCT(CEGData);

		std::vector<ProjectileSpawnInfo> projectileSpawn;
		GroundFlashInfo groundFlash;
		bool useDefaultExplosions;
	};

public:
	CCustomExplosionGenerator(): CStdExplosionGenerator() {}
	~CCustomExplosionGenerator() { ClearCache(); }

	static void OutputProjectileClassInfo();
	static unsigned int GetFlagsFromTable(const LuaTable& table);
	static unsigned int GetFlagsFromHeight(float height, float altitude);

	/// @throws content_error/runtime_error on errors
	unsigned int Load(CExplosionGeneratorHandler* handler, const std::string& tag);
	void Reload(CExplosionGeneratorHandler* handler, const std::string& tag);
	void Unload(CExplosionGeneratorHandler* handler);
	bool Explosion(unsigned int explosionID, const float3& pos, float damage, float radius, CUnit* owner, float gfxMod, CUnit* hit, const float3& dir);

	void ClearCache();

	enum {
		SPW_WATER      =  1,
		SPW_GROUND     =  2,
		SPW_AIR        =  4,
		SPW_UNDERWATER =  8,
		SPW_UNIT       = 16,  // only execute when the explosion hits a unit
		SPW_NO_UNIT    = 32,  // only execute when the explosion doesn't hit a unit (environment)
		SPW_SYNCED     = 64,  // spawn this projectile even if particleSaturation > 1
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
	void ExecuteExplosionCode(const char* code, float damage, char* instance, int spawnIndex, const float3& dir, bool synced);

protected:
	//! maps cegTags to explosion handles
	std::map<std::string, unsigned int> explosionIDs;
	//! indexed by explosion handles
	std::vector<CEGData> explosionData;

	/**
	 * Explosion generators used by explosionData.projectileSpawn.
	 * We only need this for unloading them later.
	 * @see #ClearCache
	 */
	std::vector<IExplosionGenerator*> spawnExplGens;
};


extern CExplosionGeneratorHandler* explGenHandler;
extern CCustomExplosionGenerator* gCEG;

#endif // EXPLOSION_GENERATOR_H
