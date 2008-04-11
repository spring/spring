// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_STOCKWEAPON_MANAGER_H
#define RAI_STOCKWEAPON_MANAGER_H

struct sSWeaponUnitInfo;
class cSWeaponManager;

#include "Builder.h"
//#include "LogFile.h"
//#include "ExternalAI/IAICallback.h"
//#include <map>

struct sSWeaponUnitInfo
{
	sSWeaponUnitInfo(sRAIUnitDef* udRAI)
	{
		StockPile=0;
		udr=udRAI;
	};
	int StockPile;
	sRAIUnitDef* udr;
};

class cSWeaponManager
{
public:
	cSWeaponManager(IAICallback *callback, cRAI* global);
	~cSWeaponManager();

	void UnitFinished(int unit, sRAIUnitDef* udRAI);
	void UnitDestroyed(int unit);
	void UnitIdle(int unit, sSWeaponUnitInfo* U);
	void Update();

private:
	cLogFile *l;
	IAICallback* cb;
	cRAI* G;
	map<int,sSWeaponUnitInfo> mWeapon;
};

#endif
