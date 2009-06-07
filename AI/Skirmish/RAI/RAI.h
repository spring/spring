// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_IGLOBALAI_H
#define RAI_IGLOBALAI_H

#if _MSC_VER > 1000
#pragma once
#endif

struct UnitInfo;
struct EnemyInfo;
class cRAI;

#include "ExternalAI/IGlobalAI.h"
using std::map;
#include "BasicArray.h"
#include "UnitDefHandler.h"
#include "BuilderPlacement.h"
#include "Builder.h"
#include "UnitManager.h"
#include "SWeaponManager.h"
#include "PowerManager.h"
#include "CombatManager.h"
using std::set;

const char AI_NAME[]="RAI v0.601";
const bool RAIDEBUGGING = false;

// These are for anything that can occur in Update() but not on a regular basis
struct UpdateEvent
{
	int type;	// (1=unit-time-out,2=unit-monitoring,3=initialize), also used as a ranking of importance
	int frame;	// when the event will trigger
	int index;
	int unitID;
	UnitInfo* unitI;
	float3* lastPosition; // only used by unit-monitoring
};

struct UnitInfo
{
	UnitInfo(sRAIUnitDef* rUnitDef);

	const UnitDef* ud;		// Always valid
	sRAIUnitDef* udr;		// Always valid
	sRAIUnitDefBL* udrBL;	// Valid if AIDisabled=false
	TerrainMapArea* area;	// Not always valid, but definitely = 0 if a unit flies
	sBuildQuarry* BuildQ;	// Valid if building something, otherwise = 0
	UnitInfoPower* CloakUI; // I can not rely on cb->IsUnitCloaked(unit), since I need to keep track of power variables.
	UnitInfoPower* OnOffUI; // I can not rely on cb->IsUnitActivated(unit), same reason
	sSWeaponUnitInfo* SWeaponUI; // Valid if the unit has a stock weapon, otherwise = 0
	sRAIGroup* group;
	EnemyInfo* E;	// valid if 'enemyID' & 'Group' are set, otherwise this must be updated before used
	sWeaponEfficiency* enemyEff;
	UpdateEvent* UE;
	bool AIDisabled;
	bool humanCommand;	// Set true if a unit receives an order through HandleEvent(), set false on UnitIdle()
	bool unitBeingBuilt;// Work Around: for cb->UnitBeingBuilt(), which will not work if UnitDestroyed() fails to trigger immediately after a unit's death   Spring-Version(v0.74b3)
	bool inCombat;
	int lastUnitIdleFrame;		// Guard: Prevent UnitIdle from executing too many times in rapid succession
	int lastUnitDamagedFrame;	// Guard: Prevent UnitDamaged from executing too many times in rapid succession

	// Buildings Only
	ResourceSiteExt* RS;			// Extractor/Geo, otherwise = 0
	map<int,UnitInfo*> UGuards;		// Hubs/Nanos guarding this unit
	map<int,UnitInfo*> UGuarding;	// Hubs/Nanos only, Units being guarded
//	map<int,UnitInfo*> UAssist;		// Hubs/Nanos only, Units it will assist with building
	map<int,UnitInfo*> URepair;		// Hubs/Nanos only, Units that are in need of repairs
	map<int,UnitInfo*> UDefences;	// Defences build near this unit
	map<int,UnitInfo*> UDefending;	// Defences Only, buildings near this unit
//	map<int,ResourceSiteExt*> Resources;	// Hubs only, the only resources this unit can build at

	// old
//	int commandTimeOut;			// Work Around: for the currently broken command.timeout   Spring-Version(v0.72b1)
	int enemyID;
};

struct EnemyInfo
{
	EnemyInfo();

	bool inLOS;
	bool inRadar;
	int baseThreatFrame;// last frame it attacked one of our immobile units
	int baseThreatID;	// what unit of ours did it last attack
	const UnitDef* ud;	// valid if the enemy was in LOS at least once, otherwise = 0
	sRAIUnitDef* udr;	// same as above
	ResourceSiteExt* RS;	// Enemy Extractor/Geo, otherwise = 0
	set<sRAIGroup*> attackGroups;

	float3 position;	// last known position, used if not in LOS or Radar
	bool posLocked;		// This unit does not move and its 'position' was saved while in LOS
};

using namespace std;

#include "GTerrainMap.h"
#include "GResourceMap.h"

// how often lists are updated by frame interval.  NOTE: this is different from in-game graphics frames and will always be trigger 30x per second (assuming the game speed is 1.00)
#define FUPDATE_MINIMAL 90		// 3 seconds, the minimal time for an update.  NOTE: excluding FUPDATE_UNITS, this value needs to be 'a factor' of the other FUPDATE values
#define FUPDATE_UNITS 900		// 60 seconds, unit is checked for idleness - should never happen, but it does.
#define FUPDATE_POWER 450		// 15 seconds, all units are checked for on/off or cloak/uncloak tasks
#define FUPDATE_BUILDLIST 1800	// 600 seconds, redetermines the available build options - half unnecessary since this is also called by events, hence the longer delay

class cRAI : public IGlobalAI
{
public:
	cRAI();
	virtual ~cRAI();
	void InitAI(IGlobalAICallback* callback, int team);
	void UnitCreated(int unit, int builder);		// called when a new unit is created on an ai team
	void UnitFinished(int unit);					// called when a unit has finished being build
	void UnitDestroyed(int unit,int attacker);		// called when a unit is destroyed
	void EnemyEnterLOS(int enemy);					// called when an enemy unit enters the "line of sight" of you or your ally
	void EnemyLeaveLOS(int enemy);					// called when an enemy unit exits the "line of sight" of you and your ally
	void EnemyEnterRadar(int enemy);				// called when an enemy unit enters the radar of you or your ally
	void EnemyLeaveRadar(int enemy);				// called when an enemy unit exits the radar of you and your ally
	void EnemyDestroyed(int enemy,int attacker);	// called if an enemy inside los or radar dies
	void EnemyRemove(int enemy, EnemyInfo* E);
	void EnemyDamaged(int damaged,int attacker,float damage,float3 dir); // called when an enemy inside los or radar is damaged
	void UnitIdle(int unit);						// called when a unit go idle and is not assigned to any group
	void GotChatMsg(const char* msg,int player);	// called when someone writes a chat msg
	void UnitDamaged(int damaged,int attacker,float damage,float3 dir); // called when one of your units are damaged
	void UnitMoveFailed(int unit);
	int HandleEvent(int msg,const void* data);
	void Update();			// called every frame
	void UpdateEventAdd(const int &eventType, const int &eventFrame, int uID=0, UnitInfo* uI=0);
	void CorrectPosition(float3& position);
	TerrainMapArea* GetCurrentMapArea(sRAIUnitDef* udr, float3& position); // returns MapB index if >=0, -1 = any mb, -2 = error(unit can not exist at this position)
	float3 GetRandomPosition(TerrainMapArea* area);
	bool IsHumanControled(const int& unit,UnitInfo* U); // this function will crash if the 'unit' id is invalid

	// used to check if the UnitDestroyed() event failed to trigger properly for units,  Work Around:  Spring-Version(v0.74b3-0.75b2)
	bool ValidateUnit(const int& unitID); // returns true if the unitID exists in-game, unitID is assumed to have been valid up to this point
	bool ValidateUnitList(map<int,UnitInfo*>* UL); // returns true if at least one unit on the list is valid
	void ValidateAllUnits();

	map<int,UnitInfo> Units;	// Complete record of all owned units, key value = unit id
	map<int,UnitInfo*> UImmobile;
	map<int,UnitInfo*> UMobile;
	map<int,EnemyInfo> Enemies;
	map<int,EnemyInfo*> EThreat;	// These enemies have attacked our immobile units

	cLogFile* l;
	cBuilder* B;
	cCombatManager* CM;
	cRAIUnitDefHandler* UDH;
	cUnitManager* UM;
	GlobalResourceMap* RM; // = static RM
	GlobalTerrainMap* TM; // = static TM

	void DebugDrawLine(float3 StartPos, float distance, int direction, float xposoffset=0, float zposoffset=0, float yposoffset=50, int lifetime=9000, int arrow=0, float width=5.0, int group=1); // direction 0-3, counter-clock wise, 0=right
	void DebugDrawShape(float3 CenterPos, float linelength, float width=5.0, int arrow=0, float yposoffset=50, int lifetime=9000, int sides=4, int group=1); // incomplete, sides is always 4

	typedef pair<int,UnitInfo*> iupPair;	// used to access UImmobile,UMobile, also used by different classes
	typedef pair<int,EnemyInfo*> iepPair;

private:
	void UpdateEventRemove(UpdateEvent* e);
	void UpdateEventReorderFirst();
	UpdateEvent* eventList[10000];
	int eventSize;

	int DebugEnemyEnterLOS;
	int DebugEnemyLeaveLOS;
	int DebugEnemyEnterRadar;
	int DebugEnemyLeaveRadar;
	int DebugEnemyDestroyedLOS;
	int DebugEnemyDestroyedRadar;
	int DebugEnemyEnterLOSError;
	int DebugEnemyLeaveLOSError;
	int DebugEnemyEnterRadarError;
	int DebugEnemyLeaveRadarError;
	int frame;

	typedef pair<int,UnitInfo> iuPair;		// used to access Units
	typedef pair<int,EnemyInfo> iePair;
	void ClearLogFiles();
	IAICallback* cb;
	cSWeaponManager* SWM;
};

#endif
