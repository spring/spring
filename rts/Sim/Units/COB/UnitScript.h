/* Author: Tobi Vollebregt */
/* heavily based on CobInstance.h */

#ifndef UNITSCRIPT_H
#define UNITSCRIPT_H

#include <string>
#include <vector>
#include <list>

#include "Object.h"
#include "Rendering/UnitModels/3DModel.h"
#include "UnitScriptNames.h"


class CUnit;
class CPlasmaRepulser;


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

	bool MoveToward(float &cur, float dest, float speed);
	bool TurnToward(float &cur, float dest, float speed);
	bool DoSpin(float &cur, float dest, float &speed, float accel, int divisor);

	struct AnimInfo *FindAnim(AnimType anim, int piece, int axis);
	void RemoveAnim(AnimType anim, int piece, int axis);
	void AddAnim(AnimType type, int piece, int axis, float speed, float dest, float accel, bool interpolated = false);

	virtual void ShowScriptError(const std::string& msg) = 0;

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

	bool IsBusy() const { return busy; }

	      CUnit* GetUnit()       { return unit; }
	const CUnit* GetUnit() const { return unit; }

	// takes COBFN_* constant as argument
	bool HasFunction(int id) const { return scriptIndex[id] >= 0; }

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

	bool AddAnimListener(AnimType type, int piece, int axis, IAnimListener* listener);

	// misc, used by CCobThread and callouts for Lua unitscripts
	void SetVisibility(int piece, bool visible);
	void EmitSfx(int type, int piece);
	void AttachUnit(int piece, int unit);
	void DropUnit(int unit);
	void Explode(int piece, int flags);
	void Shatter(int piece, const float3& pos, const float3& speed);
	void ShowFlare(int piece);
	int GetUnitVal(int val, int p1, int p2, int p3, int p4);
	void SetUnitVal(int val, int param);

	bool IsInAnimation(AnimType type, int piece, int axis) {
		const AnimInfo* ai = FindAnim(type, piece, axis);
		return ai && !ai->interpolated;
	}

	// callins, called throughout sim
	virtual void RawCall(int functionId) = 0;
	virtual void Create() = 0;
	// Killed must cause unit->deathScriptFinished and unit->delayedWreckLevel to be set!
	virtual void Killed() = 0;
	virtual void SetDirection(float heading) = 0;
	virtual void SetSpeed(float speed, float cob_mult) = 0;
	virtual void RockUnit(const float3& rockDir) = 0;
	virtual void HitByWeapon(const float3& hitDir) = 0;
	virtual void HitByWeaponId(const float3& hitDir, int weaponDefId, float& inout_damage) = 0;
	virtual void SetSFXOccupy(int curTerrainType) = 0;
	// doubles as QueryLandingPadCount and QueryLandingPad
	// in COB, the first one determines the number of arguments to the second one
	// in Lua, we can just count the number of return values
	virtual void QueryLandingPads(std::vector<int>& out_pieces) = 0;
	virtual void BeginTransport(const CUnit* unit) = 0;
	virtual int  QueryTransport(const CUnit* unit) = 0; // returns piece
	virtual void TransportPickup(const CUnit* unit) = 0;
	virtual void TransportDrop(const CUnit* unit, const float3& pos) = 0;
	virtual void StartBuilding(float heading, float pitch) = 0;
	virtual int  QueryNanoPiece() = 0; // returns piece
	virtual int  QueryBuildInfo() = 0; // returns piece

	// weapon callins
	virtual int   QueryWeapon(int weaponNum) = 0; // returns piece, former QueryPrimary
	virtual void  AimWeapon(int weaponNum, float heading, float pitch) = 0;
	virtual void  AimShieldWeapon(CPlasmaRepulser* weapon) = 0;
	virtual int   AimFromWeapon(int weaponNum) = 0; // returns piece, former AimFromPrimary
	virtual void  Shot(int weaponNum) = 0;
	virtual bool  BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget) = 0; // returns whether shot should be blocked
	virtual float TargetWeight(int weaponNum, const CUnit* targetUnit) = 0; // returns target weight

	// inlined callins, un-inline and make virtual when different behaviour is
	// desired between the different unit script implementations (COB, Lua).
    void Call(int id)    { RawCall(scriptIndex[id]); }
	void StartMoving()   { Call(COBFN_StartMoving); }
	void StopMoving()    { Call(COBFN_StopMoving); }
	void StartUnload()   { Call(COBFN_StartUnload); }
	void EndTransport()  { Call(COBFN_EndTransport); }
	void StartBuilding() { Call(COBFN_StartBuilding); }
	void StopBuilding()  { Call(COBFN_StopBuilding); }
	void Falling()       { Call(COBFN_Falling); }
	void Landed()        { Call(COBFN_Landed); }
	void Activate()      { Call(COBFN_Activate); }
	void Deactivate()    { Call(COBFN_Deactivate); }
	void Go()            { Call(COBFN_Go); }
	void MoveRate(int curRate)     { Call(COBFN_MoveRate0 + curRate); }
	void FireWeapon(int weaponNum) { Call(COBFN_FirePrimary + COBFN_Weapon_Funcs * weaponNum); }
	void EndBurst(int weaponNum)   { Call(COBFN_EndBurst + COBFN_Weapon_Funcs * weaponNum); }

	// not necessary for normal operation, useful to measure callin speed
	static void BenchmarkScript(CUnitScript* script);
	static void BenchmarkScript(const std::string& unitname);
};

#endif
