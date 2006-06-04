#ifndef EXPLOSION_GRAPHICS_H
#define EXPLOSION_GRAPHICS_H

#include "Sim/Misc/DamageArray.h"
#include "TdfParser.h"
#include <map>

class CUnit;
class CExplosionGenerator;

class ClassAliasList 
{
public:
	ClassAliasList();

	void Load(TdfParser& parser, const std::string& location);
	creg::Class* GetClass(const std::string& name);
	std::string FindAlias(const std::string& className);
protected:
	std::map<std::string, std::string> aliases;
};

class CExplosionGeneratorHandler
{
public:
	CExplosionGeneratorHandler ();

	CExplosionGenerator* LoadGenerator(const std::string& tag);
	TdfParser& GetParser() { return parser; }

	ClassAliasList projectileClasses, generatorClasses;
protected:
	std::map<std::string, CExplosionGenerator*> generators;
	TdfParser parser;
};


class CExplosionGenerator
{
public:
	CR_DECLARE(CExplosionGenerator);

	CExplosionGenerator();
	virtual ~CExplosionGenerator();

	virtual void Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod) = 0;
	virtual void Load(CExplosionGeneratorHandler* loader, const std::string& tag) = 0;
};

class CStdExplosionGenerator : public CExplosionGenerator
{
public:
	CR_DECLARE(CStdExplosionGenerator);

	CStdExplosionGenerator();
	virtual ~CStdExplosionGenerator();

	void Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod);
	void Load (CExplosionGeneratorHandler* loader, const std::string& tag);
};

/* Defines the result of an explosion as a series of new projectiles */
class CCustomExplosionGenerator : public CExplosionGenerator
{
protected:
	CR_DECLARE(CCustomExplosionGenerator);

	struct ProjectileSpawnInfo
	{
		ProjectileSpawnInfo() { projectileClass=0; }

		creg::Class* projectileClass;
		std::vector<char> code;
		int count;// number of projectiles spawned of this type
		unsigned int flags;
	};
	// TODO: Handle ground flashes with more flexibility like the projectiles
	struct GroundFlashInfo { 
		GroundFlashInfo() { ttl=0; }

		float flashSize;
		float flashAlpha;
		float circleGrowth;
		float circleAlpha;
		int ttl;
	} *groundFlash;

	std::vector<ProjectileSpawnInfo*> projectileSpawn;
	void ParseExplosionCode(ProjectileSpawnInfo *psi, int baseOffset, creg::IType *type, const std::string& script, std::string& code);
	void ExecuteExplosionCode (const char *code, float damage, char *instance, int spawnIndex);

public:
	CCustomExplosionGenerator();
	~CCustomExplosionGenerator();
	static void OutputProjectileClassInfo();

	void Load (CExplosionGeneratorHandler* loader, const std::string& tag);// throws content_error/runtime_error on errors
	void Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod);
};


#endif 
