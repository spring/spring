/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_INSTANCE_H
#define COB_INSTANCE_H

#include "UnitScript.h"
#include "Sim/Units/Unit.h"


#define PACKXZ(x,z) (((int)(x) << 16)+((int)(z) & 0xffff))
#define UNPACKX(xz) ((signed short)((std::uint32_t)(xz) >> 16))
#define UNPACKZ(xz) ((signed short)((std::uint32_t)(xz) & 0xffff))


// most callins only need two args, some three or four
// QueryLandingPads is unbounded (in principle) but 16
// should generally be enough
static constexpr unsigned int MAX_COB_ARGS = 16;

static constexpr   int COBSCALE      = 65536;
static constexpr   int COBSCALE_HALF = COBSCALE / 2;
static constexpr float COBSCALE_INV  = 1.0f / COBSCALE;

static const float RAD2TAANG = COBSCALE_HALF / math::PI;
static const float TAANG2RAD = math::PI / COBSCALE_HALF;


class CCobThread;
class CCobFile;


class CCobInstance : public CUnitScript
{
	CR_DECLARE_DERIVED(CCobInstance)

public:
	enum ThreadCallbackType { CBNone, CBKilled, CBAimWeapon, CBAimShield };

protected:
	void MapScriptToModelPieces(LocalModel* lmodel);

	int RealCall(int functionId, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode);

	void ShowScriptError(const std::string& msg) override;

public:
	CCobFile* cobFile;

	std::vector<int> staticVars;
	std::vector<int> threadIDs;

public:
	// creg only
	CCobInstance(): CUnitScript(nullptr), cobFile(nullptr) {}
	CCobInstance(CCobFile* cob, CUnit* unit): CUnitScript(unit), cobFile(cob) { Init(); }
	~CCobInstance();

	void Init();
	void PostLoad();

	void AddThreadID(int threadID) { threadIDs.push_back(threadID); }
	bool RemoveThreadID(int threadID)
	{
		const auto it = std::find(threadIDs.begin(), threadIDs.end(), threadID);

		if (it == threadIDs.end())
			return false;

		threadIDs.erase(it);
		return true;
	}


	// takes COBFN_* constant as argument
	bool HasFunction(int id) const;

	bool HasBlockShot(int weaponNum) const override;
	bool HasTargetWeight(int weaponNum) const override;

	// call overloads, they all call RealCall
	int Call(const std::string& fname);
	int Call(const std::string& fname, int arg1);
	int Call(const std::string& fname, std::array<int, 1 + MAX_COB_ARGS>& args);
	int Call(const std::string& fname, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode);
	// these take a COBFN_* constant as argument, which is then translated to the actual function number
	int Call(int id);
	int Call(int id, std::array<int, 1 + MAX_COB_ARGS>& args);
	int Call(int id, int arg1);
	int Call(int id, std::array<int, 1 + MAX_COB_ARGS>& args, ThreadCallbackType cb, int cbParam, int* retCode);
	// these take the raw function number
	int RawCall(int fn, std::array<int, 1 + MAX_COB_ARGS>& args);

	void ThreadCallback(ThreadCallbackType type, int retCode, int cbParam);
	// returns function number as expected by RawCall, but not Call
	// returns -1 if the function does not exist
	int GetFunctionId(const std::string& fname) const;

	const CCobFile* GetFile() const { return cobFile; }

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
		CUnitScript::Move(piece, axis, speed * COBSCALE_INV, destination * COBSCALE_INV);
	}
	void MoveNow(int piece, int axis, int destination) {
		// COBWTF
		if (axis == 0)
			destination = -destination;
		CUnitScript::MoveNow(piece, axis, destination * COBSCALE_INV);
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
		RockUnit(unit->GetObjectSpaceVec(rockDir) * 500.0f);
	}
	void RockUnit(const float3& rockDir) override;
	void WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) override {
		HitByWeapon(unit->GetObjectSpaceVec(hitDir) * 500.0f, weaponDefId, inoutDamage);
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
	void StartSkidding(const float3&) override { /* LUS-only */ }
	void StopSkidding() override { /* LUS-only */ }
	void ChangeHeading(short deltaHeading) override { /* LUS-only */ }
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
