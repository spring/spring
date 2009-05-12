#ifndef __COB_INSTANCE_H__
#define __COB_INSTANCE_H__

#include "UnitScript.h"


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
class CCobInstance;


typedef void (*CBCobThreadFinish) (int retCode, void *p1, void *p2);


class CCobInstance : public CUnitScript
{
protected:
	CCobFile& script;

	// because COB has a set of "script pieces", which isn't necessarily equal
	// to the set of all LocalModelPieces, we need to map script piece -> LMP.
	std::vector<LocalModelPiece*> pieces;

	void MapScriptToModelPieces(LocalModel* lmodel);

	int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);

	virtual void ShowScriptError(const std::string& msg);

public:
	std::vector<int> staticVars;
	std::list<CCobThread *> threads;
	bool smoothAnim;
	const CCobFile* GetScriptAddr() const { return &script; }

public:
	CCobInstance(CCobFile &script, CUnit *unit);
	virtual ~CCobInstance();

	// call overloads, they all call RealCall
	int Call(const std::string &fname);
	int Call(const std::string &fname, int p1);
	int Call(const std::string &fname, std::vector<int> &args);
	int Call(const std::string &fname, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	// these take a COBFN_* constant as argument, which is then translated to the actual function number
	int Call(int id);
	int Call(int id, std::vector<int> &args);
	int Call(int id, int p1);
	int Call(int id, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	// these take the raw function number
	int RawCall(int fn, std::vector<int> &args);

	// returns function number as expected by RawCall, but not Call
	// returns -1 if the function does not exist
	int GetFunctionId(const std::string& fname) const;

	// used by CCobThread
	void Signal(int signal);
	void PlayUnitSound(int snr, int attr);

	// translate cob piece coords into worldcoordinates
	void Spin(int piece, int axis, int speed, int accel) {
		CUnitScript::Spin(piece, axis, speed * TAANG2RAD, accel * TAANG2RAD);
	}
	void StopSpin(int piece, int axis, int decel) {
		CUnitScript::StopSpin(piece, axis, decel * TAANG2RAD);
	}
	void Turn(int piece, int axis, int speed, int destination, bool interpolated = false) {
		CUnitScript::Turn(piece, axis, speed * TAANG2RAD, destination * TAANG2RAD, interpolated);
	}
	void Move(int piece, int axis, int speed, int destination, bool interpolated = false) {
		// COBWTF
		if (axis == 0)
			destination = -destination;
		CUnitScript::Move(piece, axis, speed * CORDDIV, destination * CORDDIV, interpolated);
	}
	void MoveNow(int piece, int axis, int destination) {
		// COBWTF
		if (axis == 0)
			destination = -destination;
		CUnitScript::MoveNow(piece, axis, destination * CORDDIV);
	}
	void TurnNow(int piece, int axis, int destination) {
		CUnitScript::TurnNow(piece, axis, destination * TAANG2RAD);
	}
	void MoveSmooth(int piece, int axis, int destination, int delta, int deltaTime) {
		CUnitScript::MoveSmooth(piece, axis, destination * CORDDIV, delta, deltaTime);
	}
	void TurnSmooth(int piece, int axis, int destination, int delta, int deltaTime) {
		CUnitScript::TurnSmooth(piece, axis, destination * TAANG2RAD, delta, deltaTime);
	}

	// callins, called throughout sim
	virtual void RawCall(int functionId);
	virtual void Create();
	virtual void Killed();
	virtual void SetDirection(float heading);
	virtual void SetSpeed(float speed, float cob_mult);
	virtual void RockUnit(const float3& rockDir);
	virtual void HitByWeapon(const float3& hitDir);
	virtual void HitByWeaponId(const float3& hitDir, int weaponDefId, float& inout_damage);
	virtual void SetSFXOccupy(int curTerrainType);
	virtual void QueryLandingPads(std::vector<int>& out_pieces);
	virtual void BeginTransport(const CUnit* unit);
	virtual int  QueryTransport(const CUnit* unit);
	virtual void TransportPickup(const CUnit* unit);
	virtual void TransportDrop(const CUnit* unit, const float3& pos);
	virtual void StartBuilding(float heading, float pitch);
	virtual int  QueryNanoPiece();
	virtual int  QueryBuildInfo();

	// weapon callins
	virtual int   QueryWeapon(int weaponNum);
	virtual void  AimWeapon(int weaponNum, float heading, float pitch);
	virtual void  AimShieldWeapon(CPlasmaRepulser* weapon);
	virtual int   AimFromWeapon(int weaponNum);
	virtual void  Shot(int weaponNum);
	virtual bool  BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget);
	virtual float TargetWeight(int weaponNum, const CUnit* targetUnit);
};

#endif // __COB_INSTANCE_H__
