/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULLUNITSCRIPT_H
#define NULLUNITSCRIPT_H

#include "UnitScript.h"

class CNullUnitScript : public CUnitScript
{
	CR_DECLARE_DERIVED(CNullUnitScript)
protected:
	CNullUnitScript(CUnit* u);

	void ShowScriptError(const std::string& msg) override;
	void PostLoad();

public:
	static CNullUnitScript value;

	// callins
	void RawCall(int functionId) override {}
	void Create() override {}
	void Killed() override {}
	void WindChanged(float heading, float speed) override {}
	void ExtractionRateChanged(float speed) override {}
	void WorldRockUnit(const float3& rockDir) override {}
	void RockUnit(const float3& rockDir) override {}
	void WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) override {}
	void HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) override {}
	void SetSFXOccupy(int curTerrainType) override {}
	void QueryLandingPads(std::vector<int>& out_pieces) override {}
	void BeginTransport(const CUnit* unit) override {}
	int  QueryTransport(const CUnit* unit) override { return -1; }
	void TransportPickup(const CUnit* unit) override {}
	void TransportDrop(const CUnit* unit, const float3& pos) override {}
	void StartBuilding(float heading, float pitch) override {}
	int  QueryNanoPiece() override { return -1; }
	int  QueryBuildInfo() override { return -1; }

	void Destroy() override {}
	void StartMoving(bool reversing) override {}
	void StopMoving() override {}
	void StartSkidding(const float3&) override {}
	void StopSkidding() override {}
	void ChangeHeading(short deltaHeading) override {}
	void StartUnload() override {}
	void EndTransport() override {}
	void StartBuilding() override {}
	void StopBuilding() override {}
	void Falling() override {}
	void Landed() override {}
	void Activate() override {}
	void Deactivate() override {}
	void MoveRate(int curRate) override {}
	void FireWeapon(int weaponNum) override {}
	void EndBurst(int weaponNum) override {}

	// weapon callins
	int   QueryWeapon(int weaponNum) override { return -1; }
	void  AimWeapon(int weaponNum, float heading, float pitch) override {}
	void  AimShieldWeapon(CPlasmaRepulser* weapon) override {}
	int   AimFromWeapon(int weaponNum) override { return -1; }
	void  Shot(int weaponNum) override {}
	bool  BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget) override { return false; }
	float TargetWeight(int weaponNum, const CUnit* targetUnit) override { return 1.0f; }
	void AnimFinished(AnimType type, int piece, int axis) override { };
};

#endif
