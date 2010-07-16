/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOD_INFO_H
#define MOD_INFO_H

#include <string>

class CModInfo
{
public:
	CModInfo()
		: allowTeamColors(true)
		, allowAirPlanesToLeaveMap(true)
		, constructionDecay(true)
		, constructionDecayTime(1000)
		, constructionDecaySpeed(1.0f)
		, multiReclaim(1)
		, reclaimMethod(1)
		, reclaimUnitMethod(1)
		, reclaimUnitEnergyCostFactor(0.0f)
		, reclaimUnitEfficiency(1.0f)
		, reclaimFeatureEnergyCostFactor(0.0f)
		, reclaimAllowEnemies(true)
		, reclaimAllowAllies(true)
		, repairEnergyCostFactor(0.0f)
		, resurrectEnergyCostFactor(0.5f)
		, captureEnergyCostFactor(0.0f)
		, paralyzeOnMaxHealth(true)
		, transportGround(1)
		, transportHover(0)
		, transportShip(0)
		, transportAir(0)
		, fireAtKilled(1)
		, fireAtCrashing(1)
		, flankingBonusModeDefault(0)
		, losMipLevel(0)
		, airMipLevel(0)
		, losMul(1.0f)
		, airLosMul(1.0f)
		, requireSonarUnderWater(true)
		, featureVisibility(FEATURELOS_NONE)
	{}
	~CModInfo() {}

	void Init(const char* modArchive);

	/**
	 * The archive file name.
	 * examples: "Supreme Annihilation U32 V1.0.sdz", "BA704.sd7", "133855d253e657e9406122f346cfe8f1.sdp"
	 */
	std::string filename;

	/**
	 * The human readable name (including version).
	 * The lower-case version of this is used for dependency checking.
	 * examples: "Supreme Annihilation U32 V1.0", "Balanced Annihilation V7.04", "Balanced Annihilation V7.11"
	 */
	std::string humanName;
	/**
	 * The short name (not including version).
	 * examples: "SA", "BA", "BA"
	 */
	std::string shortName;
	/**
	 * The version
	 * examples: "U32 V1.0", "7.04", "7.11"
	 */
	std::string version;
	std::string mutator;
	std::string description;

	bool allowTeamColors;

	// Movement behaviour
	bool allowAirPlanesToLeaveMap;

	// Build behaviour
	/// Should constructions without builders decay?
	bool constructionDecay;
	/// How long until they start decaying?
	int constructionDecayTime;
	/// How fast do they decay?
	float constructionDecaySpeed;

	// Reclaim behaviour
	/// 0 = 1 reclaimer per feature max, otherwise unlimited
	int multiReclaim;
	/// 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks
	int reclaimMethod;
	/// 0 = Revert to wireframe, gradual reclaim, 1 = Subtract HP, give full metal at end, default 1
	int reclaimUnitMethod;
	/// How much energy should reclaiming a unit cost, default 0.0
	float reclaimUnitEnergyCostFactor;
	/// How much metal should reclaim return, default 1.0
	float reclaimUnitEfficiency;
	/// How much should energy should reclaiming a feature cost, default 0.0
	float reclaimFeatureEnergyCostFactor;
	/// Allow reclaiming enemies? default true
	bool reclaimAllowEnemies;
	/// Allow reclaiming allies? default true
	bool reclaimAllowAllies;

	// Repair behaviour
	/// How much should energy should repair cost, default 0.0
	float repairEnergyCostFactor;

	// Resurrect behaviour
	/// How much should energy should resurrect cost, default 0.5
	float resurrectEnergyCostFactor;

	// Capture behaviour
	/// How much should energy should capture cost, default 0.0
	float captureEnergyCostFactor;

	// Paralyze behaviour
	/// paralyze unit depending on maxHealth? if not depending on current health, default true
	bool paralyzeOnMaxHealth;

	// Transportation behaviour
	/// 0 = all ground units cannot be transported, 1 = all ground units can be transported (mass and size restrictions still apply). Defaults to 1.
	int transportGround;
	/// 0 = all hover units cannot be transported, 1 = all hover units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportHover;
	/// 0 = all naval units cannot be transported, 1 = all naval units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportShip;
	/// 0 = all air units cannot be transported, 1 = all air units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportAir;

	// Fire-on-dying-units behaviour
	/// 1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	int fireAtKilled;
	/// 1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts
	int fireAtCrashing;

	/// 0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
	int flankingBonusModeDefault;

	// Sensor behaviour
	/// miplevel for los
	int losMipLevel;
	/// miplevel to use for airlos
	int airMipLevel;
	/// units sightdistance will be multiplied with this, for testing purposes
	float losMul;
	/// units airsightdistance will be multiplied with this, for testing purposes
	float airLosMul;
	/// when underwater, units are not in LOS unless also in sonar
	bool requireSonarUnderWater;

	enum {
		FEATURELOS_NONE = 0, FEATURELOS_GAIAONLY, FEATURELOS_GAIAALLIED, FEATURELOS_ALL,
	};
	/// feature visibility style: 0 - no LOS for features, 1 - gaia features visible
	/// 2 - gaia/allied features visible, 3 - all features visible
	int featureVisibility;
};

extern CModInfo modInfo;


#endif
