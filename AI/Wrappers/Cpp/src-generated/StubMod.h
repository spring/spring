/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBMOD_H
#define _CPPWRAPPER_STUBMOD_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Mod.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubMod : public Mod {

protected:
	virtual ~StubMod();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the mod archive file name.
	 * CAUTION:
	 * Never use this as reference in eg. cache- or config-file names,
	 * as one and the same mod can be packaged in different ways.
	 * Use the human name instead.
	 * @see getHumanName()
	 * @deprecated
	 */
private:
	const char* fileName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFileName(const char* fileName);
	// @Override
	virtual const char* GetFileName();
	/**
	 * Returns the archive hash of the mod.
	 * Use this for reference to the mod, eg. in a cache-file, wherever human
	 * readability does not matter.
	 * This value will never be the same for two mods not having equal content.
	 * Tip: convert to 64 Hex chars for use in file names.
	 * @see getHumanName()
	 */
private:
	int hash;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHash(int hash);
	// @Override
	virtual int GetHash();
	/**
	 * Returns the human readable name of the mod, which includes the version.
	 * Use this for reference to the mod (including version), eg. in cache- or
	 * config-file names which are mod related, and wherever humans may come
	 * in contact with the reference.
	 * Be aware though, that this may contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Alternatively, you may use the short name only, or the short name plus
	 * version. You should generally never use the file name.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 * @see getHash()
	 * @see getShortName()
	 * @see getFileName()
	 * @see getVersion()
	 */
private:
	const char* humanName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHumanName(const char* humanName);
	// @Override
	virtual const char* GetHumanName();
	/**
	 * Returns the short name of the mod, which does not include the version.
	 * Use this for reference to the mod in general, eg. as version independent
	 * reference.
	 * Be aware though, that this still contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 * @see getVersion()
	 * @see getHumanName()
	 */
private:
	const char* shortName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShortName(const char* shortName);
	// @Override
	virtual const char* GetShortName();
private:
	const char* version;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetVersion(const char* version);
	// @Override
	virtual const char* GetVersion();
private:
	const char* mutator;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMutator(const char* mutator);
	// @Override
	virtual const char* GetMutator();
private:
	const char* description;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDescription(const char* description);
	// @Override
	virtual const char* GetDescription();
private:
	bool allowTeamColors;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAllowTeamColors(bool allowTeamColors);
	// @Override
	virtual bool GetAllowTeamColors();
	/**
	 * Should constructions without builders decay?
	 */
private:
	bool constructionDecay;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetConstructionDecay(bool constructionDecay);
	// @Override
	virtual bool GetConstructionDecay();
	/**
	 * How long until they start decaying?
	 */
private:
	int constructionDecayTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetConstructionDecayTime(int constructionDecayTime);
	// @Override
	virtual int GetConstructionDecayTime();
	/**
	 * How fast do they decay?
	 */
private:
	float constructionDecaySpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetConstructionDecaySpeed(float constructionDecaySpeed);
	// @Override
	virtual float GetConstructionDecaySpeed();
	/**
	 * 0 = 1 reclaimer per feature max, otherwise unlimited
	 */
private:
	int multiReclaim;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMultiReclaim(int multiReclaim);
	// @Override
	virtual int GetMultiReclaim();
	/**
	 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	 */
private:
	int reclaimMethod;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimMethod(int reclaimMethod);
	// @Override
	virtual int GetReclaimMethod();
	/**
	 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
	 */
private:
	int reclaimUnitMethod;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimUnitMethod(int reclaimUnitMethod);
	// @Override
	virtual int GetReclaimUnitMethod();
	/**
	 * How much energy should reclaiming a unit cost, default 0.0
	 */
private:
	float reclaimUnitEnergyCostFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimUnitEnergyCostFactor(float reclaimUnitEnergyCostFactor);
	// @Override
	virtual float GetReclaimUnitEnergyCostFactor();
	/**
	 * How much metal should reclaim return, default 1.0
	 */
private:
	float reclaimUnitEfficiency;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimUnitEfficiency(float reclaimUnitEfficiency);
	// @Override
	virtual float GetReclaimUnitEfficiency();
	/**
	 * How much should energy should reclaiming a feature cost, default 0.0
	 */
private:
	float reclaimFeatureEnergyCostFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimFeatureEnergyCostFactor(float reclaimFeatureEnergyCostFactor);
	// @Override
	virtual float GetReclaimFeatureEnergyCostFactor();
	/**
	 * Allow reclaiming enemies? default true
	 */
private:
	bool reclaimAllowEnemies;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimAllowEnemies(bool reclaimAllowEnemies);
	// @Override
	virtual bool GetReclaimAllowEnemies();
	/**
	 * Allow reclaiming allies? default true
	 */
private:
	bool reclaimAllowAllies;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimAllowAllies(bool reclaimAllowAllies);
	// @Override
	virtual bool GetReclaimAllowAllies();
	/**
	 * How much should energy should repair cost, default 0.0
	 */
private:
	float repairEnergyCostFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRepairEnergyCostFactor(float repairEnergyCostFactor);
	// @Override
	virtual float GetRepairEnergyCostFactor();
	/**
	 * How much should energy should resurrect cost, default 0.5
	 */
private:
	float resurrectEnergyCostFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetResurrectEnergyCostFactor(float resurrectEnergyCostFactor);
	// @Override
	virtual float GetResurrectEnergyCostFactor();
	/**
	 * How much should energy should capture cost, default 0.0
	 */
private:
	float captureEnergyCostFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCaptureEnergyCostFactor(float captureEnergyCostFactor);
	// @Override
	virtual float GetCaptureEnergyCostFactor();
	/**
	 * 0 = all ground units cannot be transported,
	 * 1 = all ground units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 1.
	 */
private:
	int transportGround;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportGround(int transportGround);
	// @Override
	virtual int GetTransportGround();
	/**
	 * 0 = all hover units cannot be transported,
	 * 1 = all hover units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
private:
	int transportHover;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportHover(int transportHover);
	// @Override
	virtual int GetTransportHover();
	/**
	 * 0 = all naval units cannot be transported,
	 * 1 = all naval units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
private:
	int transportShip;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportShip(int transportShip);
	// @Override
	virtual int GetTransportShip();
	/**
	 * 0 = all air units cannot be transported,
	 * 1 = all air units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
private:
	int transportAir;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTransportAir(int transportAir);
	// @Override
	virtual int GetTransportAir();
	/**
	 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	 */
private:
	int fireAtKilled;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFireAtKilled(int fireAtKilled);
	// @Override
	virtual int GetFireAtKilled();
	/**
	 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
	 */
private:
	int fireAtCrashing;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFireAtCrashing(int fireAtCrashing);
	// @Override
	virtual int GetFireAtCrashing();
	/**
	 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	 */
private:
	int flankingBonusModeDefault;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlankingBonusModeDefault(int flankingBonusModeDefault);
	// @Override
	virtual int GetFlankingBonusModeDefault();
	/**
	 * miplevel for los
	 */
private:
	int losMipLevel;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLosMipLevel(int losMipLevel);
	// @Override
	virtual int GetLosMipLevel();
	/**
	 * miplevel to use for airlos
	 */
private:
	int airMipLevel;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAirMipLevel(int airMipLevel);
	// @Override
	virtual int GetAirMipLevel();
	/**
	 * units sightdistance will be multiplied with this, for testing purposes
	 */
private:
	float losMul;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLosMul(float losMul);
	// @Override
	virtual float GetLosMul();
	/**
	 * units airsightdistance will be multiplied with this, for testing purposes
	 */
private:
	float airLosMul;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAirLosMul(float airLosMul);
	// @Override
	virtual float GetAirLosMul();
	/**
	 * when underwater, units are not in LOS unless also in sonar
	 */
private:
	bool requireSonarUnderWater;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRequireSonarUnderWater(bool requireSonarUnderWater);
	// @Override
	virtual bool GetRequireSonarUnderWater();
}; // class StubMod

}  // namespace springai

#endif // _CPPWRAPPER_STUBMOD_H

