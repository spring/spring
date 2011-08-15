// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_STOCKWEAPON_MANAGER_H
#define RAI_STOCKWEAPON_MANAGER_H

struct sSWeaponUnitInfo;
class cSWeaponManager;

#include "Builder.h"
using std::pair;
//#include "LogFile.h"
//#include "ExternalAI/IAICallback.h"
//#include <map>

class cSWeaponManager
{
public:
	cSWeaponManager(IAICallback *callback, cRAI* global);
	~cSWeaponManager();

	void UnitFinished(int unit, sRAIUnitDef* udr);
	void UnitDestroyed(int unit);
	void UnitIdle(int unit, sRAIUnitDef* udr);
	void Update();

private:
	cLogFile *l;
	IAICallback* cb;
	cRAI* G;
	map<int,sRAIUnitDef*> mWeapon;
	typedef pair<int,sRAIUnitDef*> irPair;
};

#endif
