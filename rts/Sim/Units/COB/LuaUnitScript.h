/* Author: Tobi Vollebregt */

#ifndef LUAUNITSCRIPT_H
#define LUAUNITSCRIPT_H

#include "UnitScript.h"


class CLuaHandle;
struct lua_State;


class CLuaUnitScript : public CUnitScript, CUnitScript::IAnimListener
{
private:
	// remember whether we are running in LuaRules or LuaGaia
	CLuaHandle* handle;

	// needed to luaL_unref our refs in ~CLuaUnitScript
	lua_State* L;

	// contrary to COB the list of functions may differ per unit,
	// so the COBFN_* -> function mapping can differ per unit too.
	std::vector<int> scriptIndex;
	std::map<std::string, int> scriptNames;

	// used to enforce SetDeathScriptFinished can only be used inside Killed
	bool inKilled;

protected:
	virtual void ShowScriptError(const std::string& msg);

	// only called from CreateScript, instance can not be created from C++
	CLuaUnitScript(lua_State* L, CUnit* unit);
	~CLuaUnitScript();

	int UpdateCallIn();
	void UpdateCallIn(const std::string& fname, int ref);
	void RemoveCallIn(const std::string& fname);

	float PopNumber(int fn, float def);
	bool PopBoolean(int fn, bool def);

	int  RunQueryCallIn(int fn);
	void Call(int fn) { RawCall(scriptIndex[fn]); }
	void Call(int fn, float arg1);
	void Call(int fn, float arg1, float arg2);

	void RawPushFunction(int functionId);
	void PushFunction(int id);
	void PushUnit(const CUnit* targetUnit);

	bool RunCallIn(int id, int inArgs, int outArgs);
	std::string GetScriptName(int functionId) const;
	bool RawRunCallIn(int functionId, int inArgs, int outArgs);

public:

	// callins, called throughout sim
	virtual void RawCall(int functionId);
	virtual void Create();
	virtual void Killed();
	virtual void SetDirection(float heading);
	virtual void SetSpeed(float speed, float);
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

	// special callin to allow Lua to resume threads blocking on this anim
	virtual void AnimFinished(AnimType type, int piece, int axis);

public:
	static void HandleFreed(CLuaHandle* handle);
	static bool PushEntries(lua_State* L);

private:
	static int CreateScript(lua_State* L);
	static int UpdateCallIn(lua_State* L);

	// Lua COB replacement support funcs (+SpawnCEG, PlaySoundFile, etc.)
	static int GetUnitCOBValue(lua_State* L);
	static int SetUnitCOBValue(lua_State* L);
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

	// Lua COB functions to work around lack of working CBCobThreadFinish
	static int SetDeathScriptFinished(lua_State* L);
};

#endif
