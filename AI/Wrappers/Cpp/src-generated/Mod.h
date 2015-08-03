/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_MOD_H
#define _CPPWRAPPER_MOD_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Mod {

public:
	virtual ~Mod(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the mod archive file name.
	 * CAUTION:
	 * Never use this as reference in eg. cache- or config-file names,
	 * as one and the same mod can be packaged in different ways.
	 * Use the human name instead.
	 * @see getHumanName()
	 * @deprecated
	 */
public:
	virtual const char* GetFileName() = 0;

	/**
	 * Returns the archive hash of the mod.
	 * Use this for reference to the mod, eg. in a cache-file, wherever human
	 * readability does not matter.
	 * This value will never be the same for two mods not having equal content.
	 * Tip: convert to 64 Hex chars for use in file names.
	 * @see getHumanName()
	 */
public:
	virtual int GetHash() = 0;

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
public:
	virtual const char* GetHumanName() = 0;

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
public:
	virtual const char* GetShortName() = 0;

public:
	virtual const char* GetVersion() = 0;

public:
	virtual const char* GetMutator() = 0;

public:
	virtual const char* GetDescription() = 0;

public:
	virtual bool GetAllowTeamColors() = 0;

	/**
	 * Should constructions without builders decay?
	 */
public:
	virtual bool GetConstructionDecay() = 0;

	/**
	 * How long until they start decaying?
	 */
public:
	virtual int GetConstructionDecayTime() = 0;

	/**
	 * How fast do they decay?
	 */
public:
	virtual float GetConstructionDecaySpeed() = 0;

	/**
	 * 0 = 1 reclaimer per feature max, otherwise unlimited
	 */
public:
	virtual int GetMultiReclaim() = 0;

	/**
	 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	 */
public:
	virtual int GetReclaimMethod() = 0;

	/**
	 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
	 */
public:
	virtual int GetReclaimUnitMethod() = 0;

	/**
	 * How much energy should reclaiming a unit cost, default 0.0
	 */
public:
	virtual float GetReclaimUnitEnergyCostFactor() = 0;

	/**
	 * How much metal should reclaim return, default 1.0
	 */
public:
	virtual float GetReclaimUnitEfficiency() = 0;

	/**
	 * How much should energy should reclaiming a feature cost, default 0.0
	 */
public:
	virtual float GetReclaimFeatureEnergyCostFactor() = 0;

	/**
	 * Allow reclaiming enemies? default true
	 */
public:
	virtual bool GetReclaimAllowEnemies() = 0;

	/**
	 * Allow reclaiming allies? default true
	 */
public:
	virtual bool GetReclaimAllowAllies() = 0;

	/**
	 * How much should energy should repair cost, default 0.0
	 */
public:
	virtual float GetRepairEnergyCostFactor() = 0;

	/**
	 * How much should energy should resurrect cost, default 0.5
	 */
public:
	virtual float GetResurrectEnergyCostFactor() = 0;

	/**
	 * How much should energy should capture cost, default 0.0
	 */
public:
	virtual float GetCaptureEnergyCostFactor() = 0;

	/**
	 * 0 = all ground units cannot be transported,
	 * 1 = all ground units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 1.
	 */
public:
	virtual int GetTransportGround() = 0;

	/**
	 * 0 = all hover units cannot be transported,
	 * 1 = all hover units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
public:
	virtual int GetTransportHover() = 0;

	/**
	 * 0 = all naval units cannot be transported,
	 * 1 = all naval units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
public:
	virtual int GetTransportShip() = 0;

	/**
	 * 0 = all air units cannot be transported,
	 * 1 = all air units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
public:
	virtual int GetTransportAir() = 0;

	/**
	 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	 */
public:
	virtual int GetFireAtKilled() = 0;

	/**
	 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
	 */
public:
	virtual int GetFireAtCrashing() = 0;

	/**
	 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	 */
public:
	virtual int GetFlankingBonusModeDefault() = 0;

	/**
	 * miplevel for los
	 */
public:
	virtual int GetLosMipLevel() = 0;

	/**
	 * miplevel to use for airlos
	 */
public:
	virtual int GetAirMipLevel() = 0;

	/**
	 * units sightdistance will be multiplied with this, for testing purposes
	 */
public:
	virtual float GetLosMul() = 0;

	/**
	 * units airsightdistance will be multiplied with this, for testing purposes
	 */
public:
	virtual float GetAirLosMul() = 0;

	/**
	 * when underwater, units are not in LOS unless also in sonar
	 */
public:
	virtual bool GetRequireSonarUnderWater() = 0;

}; // class Mod

}  // namespace springai

#endif // _CPPWRAPPER_MOD_H

