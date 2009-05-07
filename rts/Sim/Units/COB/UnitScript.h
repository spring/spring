/* Author: Tobi Vollebregt */
/* heavily based on CobInstance.h */

#ifndef UNITSCRIPT_H
#define UNITSCRIPT_H

#include <string>
#include <vector>
#include <list>

#include "Object.h"
#include "Rendering/UnitModels/3DModel.h"


#define PACKXZ(x,z) (((int)(x) << 16)+((int)(z) & 0xffff))
#define UNPACKX(xz) ((signed short)((boost::uint32_t)(xz) >> 16))
#define UNPACKZ(xz) ((signed short)((boost::uint32_t)(xz) & 0xffff))


static const int COBSCALE = 65536;
static const int COBSCALEHALF = COBSCALE / 2;
static const float CORDDIV   = 1.0f / COBSCALE;
static const float RAD2TAANG = COBSCALEHALF / PI;
static const float TAANG2RAD = PI / COBSCALEHALF;


class CUnit;

typedef void (*CBCobThreadFinish) (int retCode, void *p1, void *p2);


class CUnitScript : public CObject
{
public:
	enum AnimType {ATurn, ASpin, AMove};

	struct IAnimListener {
		virtual ~IAnimListener() {}
		virtual void AnimFinished(AnimType type, int piece, int axis) = 0;
	};

public:
	static const int UNIT_VAR_COUNT   = 8;
	static const int TEAM_VAR_COUNT   = 64;
	static const int ALLY_VAR_COUNT   = 64;
	static const int GLOBAL_VAR_COUNT = 4096;

	static const int UNIT_VAR_START   = 1024;
	static const int TEAM_VAR_START   = 2048;
	static const int ALLY_VAR_START   = 3072;
	static const int GLOBAL_VAR_START = 4096;

	static const int UNIT_VAR_END   = UNIT_VAR_START   + UNIT_VAR_COUNT   - 1;
	static const int TEAM_VAR_END   = TEAM_VAR_START   + TEAM_VAR_COUNT   - 1;
	static const int ALLY_VAR_END   = ALLY_VAR_START   + ALLY_VAR_COUNT   - 1;
	static const int GLOBAL_VAR_END = GLOBAL_VAR_START + GLOBAL_VAR_COUNT - 1;

	static void InitVars(int numTeams, int numAllyTeams);

public:
	const int* GetUnitVars() const { return unitVars; };

	static const int* GetTeamVars(int team) { return &teamVars[team][0]; }
	static const int* GetAllyVars(int ally) { return &allyVars[ally][0]; }
	static const int* GetGlobalVars()       { return globalVars; }

protected:
	static std::vector<int> teamVars[TEAM_VAR_COUNT];
	static std::vector<int> allyVars[ALLY_VAR_COUNT];
	static int globalVars[GLOBAL_VAR_COUNT];

	int unitVars[UNIT_VAR_COUNT];

protected:
	CUnit* unit;
	bool yardOpen;
	bool busy;

	struct AnimInfo {
		AnimType type;
		int axis;
		int piece;
		float speed;
		float dest;		//means final position when turning or moving, final speed when spinning
		float accel;		//used for spinning, can be negative
		bool interpolated;	//true if this animation is a result of interpolating a direct move/turn
		std::list<IAnimListener *> listeners;
	};

	std::list<AnimInfo*> anims;
	const std::vector<int>& scriptIndex;

	void UnblockAll(struct AnimInfo * anim);

	int MoveToward(float &cur, float dest, float speed);
	int TurnToward(float &cur, float dest, float speed);
	int DoSpin(float &cur, float dest, float &speed, float accel, int divisor);

	struct AnimInfo *FindAnim(AnimType anim, int piece, int axis);
	void RemoveAnim(AnimType anim, int piece, int axis);
	void AddAnim(AnimType type, int piece, int axis, float speed, float dest, float accel, bool interpolated = false);

	// these _must_ be implemented by any type of UnitScript
	virtual int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2) = 0;
	virtual void ShowScriptError(const std::string& msg) = 0;
	virtual void ShowScriptWarning(const std::string& msg) = 0;

public:
	// subclass is responsible for populating this with script pieces
	const std::vector<LocalModelPiece*>& pieces;

	LocalModelPiece* GetLocalModelPiece(int scriptnum) const {
		if (scriptnum >= 0 && (size_t)scriptnum < pieces.size()) {
			return pieces[scriptnum];
		}else{
			return NULL;
		}
	};

	int ScriptToModel(int scriptnum) const {
		LocalModelPiece* p = GetLocalModelPiece(scriptnum);

		if (p == NULL) return -1;

		int i = 0;
		std::vector<LocalModelPiece*> *modelpieces = &unit->localmodel->pieces;
		for(std::vector<LocalModelPiece*>::iterator pm=modelpieces->begin();pm!=modelpieces->end();pm++,i++) {
			if (p == *pm) return i;
		}
		return -1;
	};

	bool PieceExists(int scriptnum) const {
		return GetLocalModelPiece(scriptnum) != NULL;
	};

#define SCRIPT_TO_LOCALPIECE_FUNC(x,y,z,w) \
	x y(int scriptnum) const { \
		LocalModelPiece* p = GetLocalModelPiece(scriptnum); \
		if (p != NULL) return p->z(); \
		return w; \
	};

	SCRIPT_TO_LOCALPIECE_FUNC(float3,     GetPiecePos,       GetPos,       float3(0.0f,0.0f,0.0f))
	SCRIPT_TO_LOCALPIECE_FUNC(CMatrix44f, GetPieceMatrix,    GetMatrix,    CMatrix44f())
	SCRIPT_TO_LOCALPIECE_FUNC(float3,     GetPieceDirection, GetDirection, float3(1.0f,1.0f,1.0f))
	//SCRIPT_TO_LOCALPIECE_FUNC(int,        GetPieceVertCount, GetVertCount, 0)

	bool GetEmitDirPos(int scriptnum, float3 &pos, float3 &dir) const {
		LocalModelPiece* p = GetLocalModelPiece(scriptnum);
		if (p != NULL) {
			return p->GetEmitDirPos(pos, dir);
		} else {
			return true;
		}
	};

public:
	CUnitScript(CUnit* unit, const std::vector<int>& scriptIndex, const std::vector<LocalModelPiece*>& pieces);
	virtual ~CUnitScript();

	bool IsBusy() const        { return busy; }

	      CUnit* GetUnit()       { return unit; }
	const CUnit* GetUnit() const { return unit; }

	// first one takes COBFN_* constant as argument
	bool HasFunction(int id) const                   { return scriptIndex[id] >= 0; }
	bool HasFunction(const std::string& fname) const { return GetFunctionId(fname) >= 0; }

	// returns function number as expected by RawCall, but not Call
	// returns -1 if the function does not exist
	virtual int GetFunctionId(const std::string& fname) const = 0;

	// call overloads, they all call RealCall
	int Call(const std::string &fname);
	int Call(const std::string &fname, int p1);
	int Call(const std::string &fname, std::vector<int> &args);
	int Call(const std::string &fname, CBCobThreadFinish cb, void *p1, void *p2);
	int Call(const std::string &fname, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	// these take a COBFN_* constant as argument, which is then translated to the actual function number
	int Call(int id);
	int Call(int id, std::vector<int> &args);
	int Call(int id, int p1);
	int Call(int id, CBCobThreadFinish cb, void *p1, void *p2);
	int Call(int id, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	// these take the raw function number
	int RawCall(int fn, std::vector<int> &args);
	int RawCall(int fn, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);

	int Tick(int deltaTime);

	// animation, used by CCobThread
	void Spin(int piece, int axis, float speed, float accel);
	void StopSpin(int piece, int axis, float decel);
	void Turn(int piece, int axis, float speed, float destination, bool interpolated = false);
	void Move(int piece, int axis, float speed, float destination, bool interpolated = false);
	void MoveNow(int piece, int axis, float destination);
	void TurnNow(int piece, int axis, float destination);

	// for smoothing turn-now / move-now animations
	void MoveSmooth(int piece, int axis, float destination, int delta, int deltaTime);
	void TurnSmooth(int piece, int axis, float destination, int delta, int deltaTime);

	int AddTurnListener(int piece, int axis, IAnimListener* listener);
	int AddMoveListener(int piece, int axis, IAnimListener* listener);

	// misc, used by CCobThread
	void SetVisibility(int piece, bool visible);
	void EmitSfx(int type, int piece);
	void AttachUnit(int piece, int unit);
	void DropUnit(int unit);
	void Explode(int piece, int flags);
	void ShowFlare(int piece);
	int GetUnitVal(int val, int p1, int p2, int p3, int p4);
	void SetUnitVal(int val, int param);

	bool IsInAnimation(AnimType type, int piece, int axis) {
		const AnimInfo* ai = FindAnim(type, piece, axis);
		return ai && !ai->interpolated;
	}

	// callins, called throughout sim
	virtual void Create() = 0;
	// Killed must cause unit->deathScriptFinished and unit->delayedWreckLevel to be set!
	virtual void Killed() = 0;
	virtual void SetDirection(int heading) = 0;
	virtual void SetSpeed(float speed) = 0;
	virtual void RockUnit(const float3& rockDir) = 0;
	virtual void HitByWeapon(const float3& hitDir) = 0;
	virtual void HitByWeaponId(const float3& hitDir, int weaponDefId, float& inout_damage) = 0;
	virtual void SetSFXOccupy(int curTerrainType) = 0;
	// doubles as QueryLandingPadCount and QueryLandingPad
	// in COB, the first one determines the number of arguments to the second one
	// in Lua, we can just count the number of return values
	virtual void QueryLandingPads(std::vector<int>& out_pieces) = 0;
	virtual void BeginTransport(CUnit* unit) = 0;
	virtual int  QueryTransport(CUnit* unit) = 0; // returns piece
	virtual void TransportPickup(CUnit* unit) = 0;
	virtual void EndTransport() = 0;
	virtual void TransportDrop(CUnit* unit, const float3& pos) = 0;
	virtual void StartBuilding(int heading, int pitch) = 0;
	virtual void StopBuilding() = 0;
	virtual int  QueryNanoPiece() = 0; // returns piece

	// weapon callins
	virtual int   QueryWeapon(int weaponNum) = 0; // returns piece, former QueryPrimary
	virtual void  AimWeapon(int weaponNum, int heading, int pitch) = 0;
	virtual int   AimFromWeapon(int weaponNum) = 0; // returns piece, former AimFromPrimary
	virtual void  Shot(int weaponNum) = 0;
	virtual bool  BlockShot(int weaponNum, CUnit* targetUnit, bool userTarget) = 0; // returns whether shot should be blocked
	virtual float TargetWeight(int weaponNum, CUnit* targetUnit) = 0; // returns target weight
};

#endif
