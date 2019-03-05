/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobInstance.h */

#ifndef UNIT_SCRIPT_H
#define UNIT_SCRIPT_H

#include <string>
#include <vector>

#include "Rendering/Models/3DModel.h"
#include "System/creg/creg_cond.h"


class CUnit;
class CPlasmaRepulser;

class CUnitScript
{
	CR_DECLARE(CUnitScript)
	CR_DECLARE_SUB(AnimInfo)
public:
	enum AnimType {ANone = -1, ATurn = 0, ASpin = 1, AMove = 2};

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

protected:
	CUnit* unit;
	bool busy;

	struct AnimInfo {
		CR_DECLARE_STRUCT(AnimInfo)
		int axis;
		int piece;
		float speed;
		float dest;     // means final position when turning or moving, final speed when spinning
		float accel;    // used for spinning, can be negative
		bool done;
		bool hasWaiting;
	};

	typedef std::vector<AnimInfo> AnimContainerType;
	typedef AnimContainerType::iterator AnimContainerTypeIt;

	typedef bool(CUnitScript::*TickAnimFunc)(int, LocalModelPiece&, AnimInfo&);

	AnimContainerType anims[AMove + 1];


	bool hasSetSFXOccupy;
	bool hasRockUnit;
	bool hasStartBuilding;

	bool MoveToward(float& cur, float dest, float speed);
	bool TurnToward(float& cur, float dest, float speed);
	bool DoSpin(float& cur, float dest, float& speed, float accel, int divisor);

	AnimContainerTypeIt FindAnim(AnimType type, int piece, int axis);
	void RemoveAnim(AnimType type, const AnimContainerTypeIt& animInfoIt);
	void AddAnim(AnimType type, int piece, int axis, float speed, float dest, float accel);

	virtual void ShowScriptError(const std::string& msg) = 0;

	void ShowUnitScriptError(const std::string& msg);

public:
	// subclass is responsible for populating this with script pieces
	std::vector<LocalModelPiece*> pieces;

	bool PieceExists(unsigned int scriptPieceNum) const {
		// NOTE: there can be NULL pieces present from the remapping in CobInstance
		return ((scriptPieceNum < pieces.size()) && (pieces[scriptPieceNum] != nullptr));
	}

	LocalModelPiece* GetScriptLocalModelPiece(unsigned int scriptPieceNum) const {
		assert(PieceExists(scriptPieceNum));
		return pieces[scriptPieceNum];
	}

	int ScriptToModel(int scriptPieceNum) const;
	int ModelToScript(int lmodelPieceNum) const;

#define SCRIPT_TO_LOCALPIECE_FUNC(RetType, ScriptFunc, PieceFunc)       \
	RetType ScriptFunc(int scriptPieceNum) const {                      \
		if (!PieceExists(scriptPieceNum))                               \
			return {};                                                  \
		LocalModelPiece* p = GetScriptLocalModelPiece(scriptPieceNum);  \
		return (p->PieceFunc());                                        \
	}

	SCRIPT_TO_LOCALPIECE_FUNC(    float3, GetPiecePos,    GetAbsolutePos     )
	SCRIPT_TO_LOCALPIECE_FUNC(CMatrix44f, GetPieceMatrix, GetModelSpaceMatrix)

	bool GetEmitDirPos(int scriptPieceNum, float3& pos, float3& dir) const {
		if (!PieceExists(scriptPieceNum))
			return true;

		LocalModelPiece* p = GetScriptLocalModelPiece(scriptPieceNum);
		return (p->GetEmitDirPos(pos, dir));
	}

public:
	CUnitScript(CUnit* unit);
	virtual ~CUnitScript();

	bool IsBusy() const { return busy; }

	      CUnit* GetUnit()       { return unit; }
	const CUnit* GetUnit() const { return unit; }

	bool Tick(int tickRate);
	// note: must copy-and-set here (LMP dirty flag, etc)
	bool TickMoveAnim(int tickRate, LocalModelPiece& lmp, AnimInfo& ai) { float3 pos = lmp.GetPosition(); const bool ret = MoveToward(pos[ai.axis], ai.dest, ai.speed / tickRate); lmp.SetPosition(pos); return ret; }
	bool TickTurnAnim(int tickRate, LocalModelPiece& lmp, AnimInfo& ai) { float3 rot = lmp.GetRotation(); const bool ret = TurnToward(rot[ai.axis], ai.dest, ai.speed / tickRate); lmp.SetRotation(rot); return ret; }
	bool TickSpinAnim(int tickRate, LocalModelPiece& lmp, AnimInfo& ai) { float3 rot = lmp.GetRotation(); const bool ret = DoSpin(rot[ai.axis], ai.dest, ai.speed, ai.accel, tickRate); lmp.SetRotation(rot); return ret; }
	void TickAnims(int tickRate, const TickAnimFunc& tickAnimFunc, AnimContainerType& liveAnims, AnimContainerType& doneAnims);

	// animation, used by CCobThread
	void Spin(int piece, int axis, float speed, float accel);
	void StopSpin(int piece, int axis, float decel);
	void Turn(int piece, int axis, float speed, float destination);
	void Move(int piece, int axis, float speed, float destination);
	void MoveNow(int piece, int axis, float destination);
	void TurnNow(int piece, int axis, float destination);

	bool NeedsWait(AnimType type, int piece, int axis);

	// misc, used by CCobThread and callouts for Lua unitscripts
	void SetVisibility(int piece, bool visible);

	bool EmitSfx(int sfxType, int sfxPiece);
	bool EmitRelSFX(int sfxType, const float3& relPos, const float3& relDir);
	bool EmitAbsSFX(int sfxType, const float3& absPos, const float3& absDir, const float3& relDir = FwdVector);

	void AttachUnit(int piece, int unit);
	void DropUnit(int unit);
	void Explode(int piece, int flags);
	void Shatter(int piece, const float3& pos, const float3& speed);
	void ShowFlare(int piece);
	int GetUnitVal(int val, int p1, int p2, int p3, int p4);
	void SetUnitVal(int val, int param);

	bool IsInAnimation(AnimType type, int piece, int axis) {
		return (FindAnim(type, piece, axis) != anims[type].end());
	}
	bool HaveAnimations() const {
		return (!anims[ATurn].empty() || !anims[ASpin].empty() || !anims[AMove].empty());
	}

	// checks for callin existence
	bool HasSetSFXOccupy () const { return hasSetSFXOccupy; }
	bool HasRockUnit     () const { return hasRockUnit; }
	bool HasStartBuilding() const { return hasStartBuilding; }

	virtual bool HasBlockShot   (int weaponNum) const { return false; }
	virtual bool HasTargetWeight(int weaponNum) const { return false; }

	// callins, called throughout sim
	virtual void RawCall(int functionId) = 0;
	virtual void Create() = 0;
	// Killed must cause unit->deathScriptFinished and unit->delayedWreckLevel to be set!
	virtual void Killed() = 0;
	virtual void WindChanged(float heading, float speed) = 0;
	virtual void ExtractionRateChanged(float speed) = 0;
	virtual void WorldRockUnit(const float3& rockDir) = 0;
	virtual void RockUnit(const float3& rockDir) = 0;
	virtual void WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) = 0;
	virtual void HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) = 0;
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

	virtual void Destroy() = 0;
	virtual void StartMoving(bool reversing) = 0;
	virtual void StopMoving() = 0;
	virtual void StartSkidding(const float3&) = 0;
	virtual void StopSkidding() = 0;
	virtual void ChangeHeading(short deltaHeading) = 0;
	virtual void StartUnload() = 0;
	virtual void EndTransport() = 0;
	virtual void StartBuilding() = 0;
	virtual void StopBuilding() = 0;
	virtual void Falling() = 0;
	virtual void Landed() = 0;
	virtual void Activate() = 0;
	virtual void Deactivate() = 0;
	virtual void MoveRate(int curRate) = 0;
	virtual void FireWeapon(int weaponNum) = 0;
	virtual void EndBurst(int weaponNum) = 0;

	// weapon callins
	virtual int   QueryWeapon(int weaponNum) = 0; // returns piece, former QueryPrimary
	virtual void  AimWeapon(int weaponNum, float heading, float pitch) = 0;
	virtual void  AimShieldWeapon(CPlasmaRepulser* weapon) = 0;
	virtual int   AimFromWeapon(int weaponNum) = 0; // returns piece, former AimFromPrimary
	virtual void  Shot(int weaponNum) = 0;
	virtual bool  BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget) = 0; // returns whether shot should be blocked
	virtual float TargetWeight(int weaponNum, const CUnit* targetUnit) = 0; // returns target weight
	virtual void AnimFinished(AnimType type, int piece, int axis) = 0;
};

#endif // UNIT_SCRIPT_H
