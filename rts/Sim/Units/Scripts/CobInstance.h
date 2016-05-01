/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_INSTANCE_H
#define COB_INSTANCE_H

#include "UnitScript.h"
#include "Sim/Units/Unit.h"


#define PACKXZ(x,z) (((int)(x) << 16)+((int)(z) & 0xffff))
#define UNPACKX(xz) ((signed short)((boost::uint32_t)(xz) >> 16))
#define UNPACKZ(xz) ((signed short)((boost::uint32_t)(xz) & 0xffff))


static const int COBSCALE = 65536;
static const int COBSCALEHALF = COBSCALE / 2;
static const float CORDDIV   = 1.0f / COBSCALE;
static const float RAD2TAANG = COBSCALEHALF / PI;
static const float TAANG2RAD = PI / COBSCALEHALF;


class CCobThread;
class CCobFile;


class CCobInstance : public CUnitScript
{
	CR_DECLARE_DERIVED(CCobInstance)

public:
	enum ThreadCallbackType { CBNone, CBKilled, CBAimWeapon, CBAimShield };

protected:
	void MapScriptToModelPieces(LocalModel* lmodel);

	int RealCall(int functionId, std::vector<int> &args, ThreadCallbackType cb, int cbParam, int* retCode);

	void ShowScriptError(const std::string& msg) override;

public:
	CCobFile* script;
	std::vector<int> staticVars;
	std::vector<CCobThread *> threads;
	const CCobFile* GetScriptAddr() const { return script; }

public:
	//creg only
	CCobInstance();
	CCobInstance(CCobFile *script, CUnit *unit);
	virtual ~CCobInstance();

	void Init();
	void PostLoad();

	// takes COBFN_* constant as argument
	bool HasFunction(int id) const;

	bool HasBlockShot(int weaponNum) const override;
	bool HasTargetWeight(int weaponNum) const override;

	// call overloads, they all call RealCall
	int Call(const std::string &fname);
	int Call(const std::string &fname, int arg1);
	int Call(const std::string &fname, std::vector<int> &args);
	int Call(const std::string &fname, std::vector<int> &args, ThreadCallbackType cb, int cbParam, int* retCode);
	// these take a COBFN_* constant as argument, which is then translated to the actual function number
	int Call(int id);
	int Call(int id, std::vector<int> &args);
	int Call(int id, int arg1);
	int Call(int id, std::vector<int> &args, ThreadCallbackType cb, int cbParam, int* retCode);
	// these take the raw function number
	int RawCall(int fn, std::vector<int> &args);

	void ThreadCallback(ThreadCallbackType type, int retCode, int cbParam);
	// returns function number as expected by RawCall, but not Call
	// returns -1 if the function does not exist
	int GetFunctionId(const std::string& fname) const;

	// used by CCobThread
	void Signal(int signal);
	void PlayUnitSound(int snr, int attr);

	// translate cob piece coords into worldcoordinates
	void Spin(int piece, int axis, int speed, int accel) {
		// COBWTF
		if (axis == 2)
			speed = -speed;
		CUnitScript::Spin(piece, axis, speed * TAANG2RAD, accel * TAANG2RAD);
	}
	void StopSpin(int piece, int axis, int decel) {
		CUnitScript::StopSpin(piece, axis, decel * TAANG2RAD);
	}
	void Turn(int piece, int axis, int speed, int destination) {
		// COBWTF
		if (axis == 2)
			destination = -destination;
		CUnitScript::Turn(piece, axis, speed * TAANG2RAD, destination * TAANG2RAD);
	}
	void Move(int piece, int axis, int speed, int destination) {
		// COBWTF
		if (axis == 0)
			destination = -destination;
		CUnitScript::Move(piece, axis, speed * CORDDIV, destination * CORDDIV);
	}
	void MoveNow(int piece, int axis, int destination) {
		// COBWTF
		if (axis == 0)
			destination = -destination;
		CUnitScript::MoveNow(piece, axis, destination * CORDDIV);
	}
	void TurnNow(int piece, int axis, int destination) {
		// COBWTF
		if (axis == 2)
			destination = -destination;
		CUnitScript::TurnNow(piece, axis, destination * TAANG2RAD);
	}

	// callins, called throughout sim
	void RawCall(int functionId) override;
	void Create() override;
	void Killed() override;
	void WindChanged(float heading, float speed) override;
	void ExtractionRateChanged(float speed) override;
	void WorldRockUnit(const float3& rockDir) override {
		RockUnit(unit->GetObjectSpaceVec(rockDir) * 500.f);
	}
	void RockUnit(const float3& rockDir) override;
	void WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) override {
		HitByWeapon(unit->GetObjectSpaceVec(hitDir) * 500.f, weaponDefId, inoutDamage);
	}
	void HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) override;
	void SetSFXOccupy(int curTerrainType) override;
	void QueryLandingPads(std::vector<int>& out_pieces) override;
	void BeginTransport(const CUnit* unit) override;
	int  QueryTransport(const CUnit* unit) override;
	void TransportPickup(const CUnit* unit) override;
	void TransportDrop(const CUnit* unit, const float3& pos) override;
	void StartBuilding(float heading, float pitch) override;
	int  QueryNanoPiece() override;
	int  QueryBuildInfo() override;

	void Destroy() override;
	void StartMoving(bool reversing) override;
	void StopMoving() override;
	void StartUnload() override;
	void EndTransport() override;
	void StartBuilding() override;
	void StopBuilding() override;
	void Falling() override;
	void Landed() override;
	void Activate() override;
	void Deactivate() override;
	void MoveRate(int curRate) override;
	void FireWeapon(int weaponNum) override;
	void EndBurst(int weaponNum) override;

	// weapon callins
	int   QueryWeapon(int weaponNum) override;
	void  AimWeapon(int weaponNum, float heading, float pitch) override;
	void  AimShieldWeapon(CPlasmaRepulser* weapon) override;
	int   AimFromWeapon(int weaponNum) override;
	void  Shot(int weaponNum) override;
	bool  BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget) override;
	float TargetWeight(int weaponNum, const CUnit* targetUnit) override;
	void AnimFinished(AnimType type, int piece, int axis) override;
};

#endif // COB_INSTANCE_H
