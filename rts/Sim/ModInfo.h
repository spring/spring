
#ifndef MOD_INFO_H
#define MOD_INFO_H

class CModInfo
{
public:
	CModInfo(const char *modname);
	~CModInfo();

	std::string name;
	std::string humanName;
	bool allowTeamColors;

	// Reclaim behaviour
	int multiReclaim;	// 0 = 1 reclaimer per feature max, otherwise unlimited
	int reclaimMethod;	// 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks

	//Transportation behaviour
	int transportGround;	//0 = all ground units cannot be transported, 1 = all ground units can be transported (mass and size restrictions still apply). Defaults to 1.
	int transportHover;	//0 = all hover units cannot be transported, 1 = all hover units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportShip;	//0 = all naval units cannot be transported, 1 = all naval units can be transported (mass and size restrictions still apply). Defaults to 0.
	int transportAir;	//0 = all air units cannot be transported, 1 = all air units can be transported (mass and size restrictions still apply). Defaults to 0.

};

extern CModInfo *modInfo;


#endif
