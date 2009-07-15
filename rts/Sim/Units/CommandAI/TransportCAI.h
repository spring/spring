#ifndef TRANSPORTCAI_H
#define TRANSPORTCAI_H

#include "MobileCAI.h"
#include "Sim/MoveTypes/TAAirMoveType.h"

#define UNLOAD_LAND 0
#define UNLOAD_DROP 1
#define UNLOAD_LANDFLOOD 2
#define UNLOAD_CRASHFLOOD 3

class CTransportUnit;

class CTransportCAI :
	public CMobileCAI
{
public:
	CR_DECLARE(CTransportCAI);
	CTransportCAI(CUnit* owner);
	CTransportCAI();
	~CTransportCAI(void);
	void SlowUpdate(void);

	int unloadType;
	bool CanTransport(CUnit* unit);
	bool FindEmptySpot(float3 center, float radius,float emptyRadius, float3& found, CUnit* unitToUnload);
	bool FindEmptyDropSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots);
	bool FindEmptyFloodSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots, std::vector<float3> exitDirs);
	CUnit* FindUnitToTransport(float3 center, float radius);
	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void DrawCommands(void);
	void FinishCommand(void);
	bool LoadStillValid(CUnit* unit);

	virtual void ExecuteUnloadUnit(Command &c);
	virtual void ExecuteUnloadUnits(Command &c);
	virtual void ExecuteLoadUnits(Command &c);

	int toBeTransportedUnitId;
	int lastCall;

private:
	void UnloadUnits_Land(Command& c, CTransportUnit* transport);
	void UnloadUnits_Drop(Command& c, CTransportUnit* transport);
	void UnloadUnits_LandFlood(Command& c, CTransportUnit* transport);
	void UnloadUnits_CrashFlood(Command& c, CTransportUnit* transport); //incomplete

	void UnloadNormal(Command& c);
	void UnloadLand(Command& c);
	void UnloadDrop(Command& c);	//parachute drop units
	void UnloadLandFlood(Command& c); //land and dispatch units all at once
	void UnloadCrashFlood(Command& c); //slam into landscape abruptly and dispatch units all at once (incomplete)

	virtual bool AllowedCommand(const Command& c, bool fromSynced);
	bool SpotIsClear(float3 pos, CUnit* u);
	bool SpotIsClearIgnoreSelf(float3 pos,CUnit* unitToUnload);
	std::list<float3> dropSpots;
	bool isFirstIteration;
	float3 startingDropPos;
	float3 lastDropPos;
	float3 approachVector; //direction from which we travel to drop point
	float3 endDropPos;
};


#endif /* TRANSPORTCAI_H */
