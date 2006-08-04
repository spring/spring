
#ifndef MOD_INFO_H
#define MOD_INFO_H

class CModInfo
{
public:
	CModInfo(const char *modname);
	~CModInfo();

	std::string name;
	bool allowTeamColors;
};

extern CModInfo *modInfo;


#endif
