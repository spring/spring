/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULLUNITSCRIPT_H
#define NULLUNITSCRIPT_H

#include "UnitScript.h"

class CNullUnitScript : public CUnitScript
{
private:
	std::vector<LocalModelPiece*> pieces;

protected:
	CNullUnitScript();

	virtual void ShowScriptError(const std::string& msg);

public:
	static CNullUnitScript value;

	// callins
	virtual void RawCall(int functionId);
	virtual void Create();
	virtual void Killed();
	virtual void WindChanged(float heading, float speed);
	virtual void ExtractionRateChanged(float speed);
	virtual void RockUnit(const float3& rockDir);
	virtual void HitByWeapon(const float3& hitDir, int weaponDefId, float& inout_damage);
	virtual void SetSFXOccupy(int curTerrainType);
	virtual void QueryLandingPads(std::vector<int>& out_pieces);
	virtual void BeginTransport(const CUnit* unit);
	virtual int  QueryTransport(const CUnit* unit);
	virtual void TransportPickup(const CUnit* unit);
	virtual void TransportDrop(const CUnit* unit, const float3& pos);
	virtual void StartBuilding(float heading, float pitch);
	virtual int  QueryNanoPiece();
	virtual int  QueryBuildInfo();

	virtual void Destroy();
	virtual void StartMoving();
	virtual void StopMoving();
	virtual void StartUnload();
	virtual void EndTransport();
	virtual void StartBuilding();
	virtual void StopBuilding();
	virtual void Falling();
	virtual void Landed();
	virtual void Activate();
	virtual void Deactivate();
	virtual void MoveRate(int curRate);
	virtual void FireWeapon(int weaponNum);
	virtual void EndBurst(int weaponNum);

	// weapon callins
	virtual int   QueryWeapon(int weaponNum);
	virtual void  AimWeapon(int weaponNum, float heading, float pitch);
	virtual void  AimShieldWeapon(CPlasmaRepulser* weapon);
	virtual int   AimFromWeapon(int weaponNum);
	virtual void  Shot(int weaponNum);
	virtual bool  BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget);
	virtual float TargetWeight(int weaponNum, const CUnit* targetUnit);
};

#endif
