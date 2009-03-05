
#ifndef MOD_INFO_H
#define MOD_INFO_H

class CModInfo
{
public:
	CModInfo() {};
	~CModInfo() {};

	void Init(const char* modname);

	/// archive filename
	std::string filename;

	std::string humanName;
	std::string shortName;
	std::string version;
	std::string mutator;
	std::string description;

	bool allowTeamColors;

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
};

extern CModInfo modInfo;


#endif
