
#ifndef MOD_INFO_H
#define MOD_INFO_H

class CModInfo
{
public:
	CModInfo() {};
	~CModInfo() {};

	void Init(const char* modname);

	std::string filename; // archive filename

	std::string humanName;
	std::string shortName;
	std::string version;
	std::string mutator;
	std::string description;

	bool allowTeamColors;

	// Reclaim behaviour
	int multiReclaim;	// 0 = 1 reclaimer per feature max, otherwise unlimited
	int reclaimMethod;	// 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks

	//Transportation behaviour
	int transportGround;	//0 = all ground units cannot be transported, 1 = all ground units can be transported (mass and size restrictions still apply). Defaults to 1.
	int transportHover;	//0 = all hover units cannot be transported, 1 = all hover units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportShip;	//0 = all naval units cannot be transported, 1 = all naval units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportAir;	//0 = all air units cannot be transported, 1 = all air units can be transported (mass and size restrictions still apply). Defaults to 0.

	//Fire on dying units behaviour
	int fireAtKilled;	//1 = units fire at enemies running Killed() script, 0 = units ignore such enemies
	int fireAtCrashing;	//1 = units fire at crashing aircrafts, 0 = units ignore crashing aircrafts

	int flankingBonusModeDefault;	//0=no flanking bonus;  1=global coords, mobile;  2=unit coords, mobile;  3=unit coords, locked
};

extern CModInfo modInfo;


#endif
