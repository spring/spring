/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPMOD_H
#define _CPPWRAPPER_WRAPPMOD_H

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
class WrappMod : public Mod {

private:
	int skirmishAIId;

	WrappMod(int skirmishAIId);
	virtual ~WrappMod();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Mod* GetInstance(int skirmishAIId);

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
public:
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
public:
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
public:
	// @Override
	virtual const char* GetShortName();

public:
	// @Override
	virtual const char* GetVersion();

public:
	// @Override
	virtual const char* GetMutator();

public:
	// @Override
	virtual const char* GetDescription();

public:
	// @Override
	virtual bool GetAllowTeamColors();

	/**
	 * Should constructions without builders decay?
	 */
public:
	// @Override
	virtual bool GetConstructionDecay();

	/**
	 * How long until they start decaying?
	 */
public:
	// @Override
	virtual int GetConstructionDecayTime();

	/**
	 * How fast do they decay?
	 */
public:
	// @Override
	virtual float GetConstructionDecaySpeed();

	/**
	 * 0 = 1 reclaimer per feature max, otherwise unlimited
	 */
public:
	// @Override
	virtual int GetMultiReclaim();

	/**
	 * 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	 */
public:
	// @Override
	virtual int GetReclaimMethod();

	/**
	 * 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
	 */
public:
	// @Override
	virtual int GetReclaimUnitMethod();

	/**
	 * How much energy should reclaiming a unit cost, default 0.0
	 */
public:
	// @Override
	virtual float GetReclaimUnitEnergyCostFactor();

	/**
	 * How much metal should reclaim return, default 1.0
	 */
public:
	// @Override
	virtual float GetReclaimUnitEfficiency();

	/**
	 * How much should energy should reclaiming a feature cost, default 0.0
	 */
public:
	// @Override
	virtual float GetReclaimFeatureEnergyCostFactor();

	/**
	 * Allow reclaiming enemies? default true
	 */
public:
	// @Override
	virtual bool GetReclaimAllowEnemies();

	/**
	 * Allow reclaiming allies? default true
	 */
public:
	// @Override
	virtual bool GetReclaimAllowAllies();

	/**
	 * How much should energy should repair cost, default 0.0
	 */
public:
	// @Override
	virtual float GetRepairEnergyCostFactor();

	/**
	 * How much should energy should resurrect cost, default 0.5
	 */
public:
	// @Override
	virtual float GetResurrectEnergyCostFactor();

	/**
	 * How much should energy should capture cost, default 0.0
	 */
public:
	// @Override
	virtual float GetCaptureEnergyCostFactor();

	/**
	 * 0 = all ground units cannot be transported,
	 * 1 = all ground units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 1.
	 */
public:
	// @Override
	virtual int GetTransportGround();

	/**
	 * 0 = all hover units cannot be transported,
	 * 1 = all hover units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
public:
	// @Override
	virtual int GetTransportHover();

	/**
	 * 0 = all naval units cannot be transported,
	 * 1 = all naval units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
public:
	// @Override
	virtual int GetTransportShip();

	/**
	 * 0 = all air units cannot be transported,
	 * 1 = all air units can be transported
	 * (mass and size restrictions still apply).
	 * Defaults to 0.
	 */
public:
	// @Override
	virtual int GetTransportAir();

	/**
	 * 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	 */
public:
	// @Override
	virtual int GetFireAtKilled();

	/**
	 * 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
	 */
public:
	// @Override
	virtual int GetFireAtCrashing();

	/**
	 * 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	 */
public:
	// @Override
	virtual int GetFlankingBonusModeDefault();

	/**
	 * miplevel for los
	 */
public:
	// @Override
	virtual int GetLosMipLevel();

	/**
	 * miplevel to use for airlos
	 */
public:
	// @Override
	virtual int GetAirMipLevel();

	/**
	 * units sightdistance will be multiplied with this, for testing purposes
	 */
public:
	// @Override
	virtual float GetLosMul();

	/**
	 * units airsightdistance will be multiplied with this, for testing purposes
	 */
public:
	// @Override
	virtual float GetAirLosMul();

	/**
	 * when underwater, units are not in LOS unless also in sonar
	 */
public:
	// @Override
	virtual bool GetRequireSonarUnderWater();
}; // class WrappMod

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPMOD_H

