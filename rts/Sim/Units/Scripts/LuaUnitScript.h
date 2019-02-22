/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUAUNITSCRIPT_H
#define LUAUNITSCRIPT_H

#include "UnitScript.h"
#include "NullUnitScript.h"
#include "LuaScriptNames.h"
#include "System/UnorderedMap.hpp"

class CLuaHandle;
class CUnit;
struct lua_State;

// Hack for creg:
// Since CLuaUnitScript isn't creged, during loading it will
// construct a CNullUnitScript instead.
class CLuaUnitScript : public CUnitScript
{
	CR_DECLARE_DERIVED(CLuaUnitScript)
private:
	static CUnit* activeUnit;
	static CUnitScript* activeScript;

	// remember whether we are running in LuaRules or LuaGaia
	CLuaHandle* handle = nullptr;

	// needed to luaL_unref our refs in ~CLuaUnitScript
	lua_State* L = nullptr;

	// contrary to COB the list of functions may differ per unit,
	// so the LUAFN_* -> function mapping can differ per unit too.
	std::array<int, LUAFN_Last> scriptIndex;
	spring::unordered_map<std::string, int> scriptNames;

	// used to enforce SetDeathScriptFinished can only be used inside Killed
	bool inKilled = false;

public:
	// for creg use only
	CLuaUnitScript() : CUnitScript(nullptr) {}

	// note: instance can not logically be created from outside CreateScript
	// the ctor and dtor still have to be public because scripts are pooled
	CLuaUnitScript(lua_State* L, CUnit* unit);
	~CLuaUnitScript();

	void PostLoad();
	void Serialize(creg::ISerializer* s);

protected:
	void ShowScriptError(const std::string& msg) override;

	int UpdateCallIn();
	void UpdateCallIn(const std::string& fname, int ref);
	void RemoveCallIn(const std::string& fname);

	float PopNumber(int fn, float def);
	bool PopBoolean(int fn, bool def);

	int  RunQueryCallIn(int fn);
	int  RunQueryCallIn(int fn, float arg1);
	void Call(int fn) { RawCall(scriptIndex[fn]); }
	void Call(int fn, float arg1);
	void Call(int fn, float arg1, float arg2);
	void Call(int fn, float arg1, float arg2, float arg3);

	void RawPushFunction(int functionId);
	void PushFunction(int id);
	void PushUnit(const CUnit* targetUnit);

	bool RunCallIn(int id, int inArgs, int outArgs);
	bool RawRunCallIn(int functionId, int inArgs, int outArgs);

	std::string GetScriptName(int functionId) const;

public:

	// takes LUAFN_* constant as argument
	bool HasFunction(int id) const { return scriptIndex[id] >= 0; }

	bool HasBlockShot(int weaponNum) const override;
	bool HasTargetWeight(int weaponNum) const override;

	// callins, called throughout sim
	void RawCall(int functionId) override;
	void Create() override;
	void Killed() override;
	void WindChanged(float heading, float speed) override;
	void ExtractionRateChanged(float speed) override;

	void WorldRockUnit(const float3& rockDir) override;
	void RockUnit(const float3& rockDir) override;

	void WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) override;
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
	void StartSkidding(const float3& vel) override;
	void StopSkidding() override;
	void ChangeHeading(short deltaHeading) override;
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

	// special callin to allow Lua to resume threads blocking on this anim
	void AnimFinished(AnimType type, int piece, int axis) override;

public:
	static void HandleFreed(CLuaHandle* handle);
	static bool PushEntries(lua_State* L);

private:
	static int CreateScript(lua_State* L);
	static int UpdateCallIn(lua_State* L);

	// other call-outs are stateful
	static int CallAsUnit(lua_State* L);

	// Lua COB replacement support funcs (+SpawnCEG, PlaySoundFile, etc.)
	static int GetUnitValue(lua_State* L, CUnitScript* script, int arg);
	static int GetUnitValue(lua_State* L);
	static int GetUnitCOBValue(lua_State* L); // backward compat
	static int SetUnitValue(lua_State* L, CUnitScript* script, int arg);
	static int SetUnitValue(lua_State* L);
	static int SetUnitCOBValue(lua_State* L); // backward compat
	static int SetPieceVisibility(lua_State* L);
	static int EmitSfx(lua_State* L);       // TODO: better names?
	static int AttachUnit(lua_State* L);
	static int DropUnit(lua_State* L);
	static int Explode(lua_State* L);
	static int ShowFlare(lua_State* L);

	// Lua COB replacement animation support funcs
	static int Spin(lua_State* L);
	static int StopSpin(lua_State* L);
	static int Turn(lua_State* L);
	static int Move(lua_State* L);
	static int IsInAnimation(lua_State* L, const char* caller, AnimType type);
	static int IsInTurn(lua_State* L);
	static int IsInMove(lua_State* L);
	static int IsInSpin(lua_State* L);
	static int WaitForAnimation(lua_State* L, const char* caller, AnimType type);
	static int WaitForTurn(lua_State* L);
	static int WaitForMove(lua_State* L);

	// Lua COB function to work around lack of working CBCobThreadFinish
	static int SetDeathScriptFinished(lua_State* L);

	static int GetPieceTranslation(lua_State* L); // matches Move
	static int GetPieceRotation(lua_State* L);    // matches Turn
	static int GetPiecePosDir(lua_State* L);      // EmitDirPos (in unit space)

	static int GetActiveUnitID(lua_State* L);
};

#endif
