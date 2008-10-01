// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_POWERMANAGER_H
#define RAI_POWERMANAGER_H

struct sPowerUnitInfo;
class cPowerManager;

//#include "LogFile.h"
#include "RAI.h"
//#include "ExternalAI/IAICallback.h"
//#include <map>

struct sPowerUnitInfo
{
	sPowerUnitInfo(bool on)
	{
		active = on;
	}
	bool active;
};

class cPowerManager
{
public:
	cPowerManager(IAICallback *callback, cRAI* global);
	~cPowerManager();

	void UnitFinished(int unit, UnitInfo *U);	// called by all units
	void UnitDestroyed(int unit, UnitInfo *U);	// called by all units
	void Update();

	// these variables are used to aid the build class in it's decisions
	int NeededCloakPower;		// Amount of energy need to turn on all Cloak Units, does not include units already on
	int NeededOnOffPower;		// Amount of energy need to turn on all EtoMconverter & OnOff Units, does not include units already on
	int NeededWeaponPower;		// Amount of energy drain from all unit weapons passed through this class, per 30 frames
	int MetalProductionEtoM;	// Amount of metal that would be produced if all metal makers where on, does not include units already on
	int MetalProduction;		// Amount of metal production lost from extractors or other effective metal productions

private:
	cLogFile *l;
	IAICallback *cb;
	cRAI *G;
	map<int,UnitInfo*> mCloakUnits;		// key = unit id
	map<int,UnitInfo*> mMtoEConverter;	// key = unit id, Metal to Energy
	map<int,UnitInfo*> mEtoMConverter;	// key = unit id, Energy to Metal
	map<int,UnitInfo*> mOnOffUnits;		// key = unit id, units that should rarely be turned off
};

#endif
