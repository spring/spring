// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_POWERMANAGER_H
#define RAI_POWERMANAGER_H

struct UnitInfoPower;
class cPowerManager;

//#include "LogFile.h"
#include "RAI.h"
//#include "ExternalAI/IAICallback.h"
//#include <map>

struct UnitInfoPower
{
	UnitInfoPower(int UID, UnitInfo *UI, bool isActive, int listType);

	int unitID;
	UnitInfo *U;
	bool active;		// used to keep track of the main class variables
	float importance;	// determines array order, negative for reverse converters
	int index;			// within its array
	int type;			// 0=EDrain(cloak),1=EDrain(on/off),2=MDrain(unused),3=EtoM,4=MtoE
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
	double EnergyDifference;	// Energy Income & Usage of all units that can not be turned off or are not managed by this class
	double MetalDifference;		// Metal Income & Usage of all units that can not be turned off or are not managed by this class
	double WeaponEnergyNeeded;	// - value, Amount of energy drain from all unit weapons passed through this class, per 30 frames

	double OffEnergyDifference;	// - value, EnergyUsage of off units & uncloaked units, excluding ExM
	double OnEnergyDifference;	// - value, EnergyUsage of on units & cloaked units, excluding ExM

	double ExMMetalDifference;	// Metal Income/Usage of all turned on ExM
	double ExMEnergyDifference;	// Energy Income/Usage of all turned on ExM
	double EtoMIncome;			// + value, Amount of [Metal/Energy] that would be produced if all [Energy/Metal] to [Metal/Energy] converters were on, does not include units already on
	double EtoMNeeded;			// - value, [Energy/Metal]-Usage if all [Energy/Metal] to [Metal/Energy] converters were turned on, does not include units already on
	double MtoEIncome;			// + value
	double MtoENeeded;			// - value

private:
	void InsertPI(UnitInfoPower **PIA, int &PIASize, UnitInfoPower *P);
	void RemovePI(UnitInfoPower **PIA, int &PIASize, UnitInfoPower *P);
	void GiveCloakOrder(const int &unitID, UnitInfo *U = 0, bool state = true );
	void GiveOnOffOrder(const int &unitID, UnitInfo *U = 0, bool state = true );

	cLogFile *l;
	IAICallback *cb;
	cRAI *G;

	UnitInfoPower **EDrain;
	int EDrainSize;	// the number of on/off units managed by this class (not including ExM)
	int EDrainActive;
	UnitInfoPower **EtoM;
	int EtoMSize;	// the number of metal makers
	int EtoMActive;
	UnitInfoPower **MtoE;
	int MtoESize;	// the number of energy makers
	int MtoEActive;

	int UIPLimit;	// unit limit, the max size of the lists above
	int DebugUnitFinished;
	int DebugUnitDestroyed;
};

#endif
