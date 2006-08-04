
#ifndef MOD_INFO_H
#define MOD_INFO_H

class CModInfo
{
public:
	CModInfo(const char *modname);
	~CModInfo();

	std::string name;
	bool allowTeamColors;

	// Reclaim behaviour
	int multiReclaim;	// 0 = 1 reclaimer per feature max, otherwise unlimited
	int reclaimMethod;	// 0 = gradual reclaim, 1 = all reclaimed at end, otherwise reclaim in reclaimMethod chunks

};

extern CModInfo *modInfo;


#endif
