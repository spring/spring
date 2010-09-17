/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TRANSPORT_CAI_H
#define TRANSPORT_CAI_H

#include "MobileCAI.h"
#include "System/float3.h"

#include <list>
#include <vector>

#define UNLOAD_LAND 0
#define UNLOAD_DROP 1
#define UNLOAD_LANDFLOOD 2
#define UNLOAD_CRASHFLOOD 3

class CTransportUnit;
class CUnit;
class CFeature;
class Command;

class CTransportCAI : public CMobileCAI
{
public:
	CR_DECLARE(CTransportCAI);
	CTransportCAI(CUnit* owner);
	CTransportCAI();
	~CTransportCAI();

	void SlowUpdate();

	bool CanTransport(const CUnit* unit);
	bool FindEmptySpot(float3 center, float radius, float emptyRadius, float3& found, CUnit* unitToUnload);
	bool FindEmptyDropSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots);
	bool FindEmptyFloodSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots, std::vector<float3> exitDirs);
	CUnit* FindUnitToTransport(float3 center, float radius);
	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void DrawCommands();
	void FinishCommand();
	bool LoadStillValid(CUnit* unit);

	virtual void ExecuteUnloadUnit(Command& c);
	virtual void ExecuteUnloadUnits(Command& c);
	virtual void ExecuteLoadUnits(Command& c);

	int unloadType;
	int toBeTransportedUnitId;
	int lastCall;

private:
	void UnloadUnits_Land(Command& c, CTransportUnit* transport);
	void UnloadUnits_Drop(Command& c, CTransportUnit* transport);
	void UnloadUnits_LandFlood(Command& c, CTransportUnit* transport);
	void UnloadUnits_CrashFlood(Command& c, CTransportUnit* transport); // FIXME incomplete

	void UnloadNormal(Command& c);
	void UnloadLand(Command& c);
	/// parachute drop units
	void UnloadDrop(Command& c);
	/// land and dispatch units all at once
	void UnloadLandFlood(Command& c);
	/// slam into landscape abruptly and dispatch units all at once (incomplete)
	void UnloadCrashFlood(Command& c);

	virtual bool AllowedCommand(const Command& c, bool fromSynced);
	bool SpotIsClear(float3 pos, CUnit* u);
	/**
	 * This is a version of SpotIsClear that ignores the tranport and all
	 * units it carries.
	 */
	bool SpotIsClearIgnoreSelf(float3 pos, CUnit* unitToUnload);
	std::list<float3> dropSpots;
	bool isFirstIteration;
	float3 startingDropPos;
	float3 lastDropPos;
	/// direction from which we travel to drop point
	float3 approachVector;
	float3 endDropPos;
};

#endif /* TRANSPORT_CAI_H */
