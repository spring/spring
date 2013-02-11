/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HELPER_H
#define LUA_HELPER_H

#include "Sim/Misc/TeamHandler.h"
#include "System/EventClient.h"
#include "LuaHandle.h"

class CFeature;

/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline bool FullCtrl(const lua_State *L)
{
	return CLuaHandle::GetHandleFullCtrl(L);
}


static inline int CtrlTeam(const lua_State *L)
{
	return CLuaHandle::GetHandleCtrlTeam(L);
}


static inline int CtrlAllyTeam(const lua_State *L)
{
	const int ctrlTeam = CtrlTeam(L);
	if (ctrlTeam < 0) {
		return ctrlTeam;
	}
	return teamHandler->AllyTeam(ctrlTeam);
}


static inline bool CanControlTeam(const lua_State *L, int teamID)
{
	const int ctrlTeam = CtrlTeam(L);
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == teamID);
}


static inline bool CanControlAllyTeam(const lua_State *L, int allyTeamID)
{
	const int ctrlTeam = CtrlTeam(L);
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyTeamID);
}


static inline bool CanControlUnit(const lua_State *L, const CUnit* unit)
{
	const int ctrlTeam = CtrlTeam(L);
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == unit->team);
}


static inline bool CanControlFeatureAllyTeam(const lua_State *L, int allyTeamID)
{
	const int ctrlTeam = CtrlTeam(L);
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	if (allyTeamID < 0) {
		return (ctrlTeam == teamHandler->GaiaTeamID());
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyTeamID);
}


static inline bool CanControlFeature(const lua_State *L, const CFeature* feature)
{
	return CanControlFeatureAllyTeam(L, feature->allyteam);
}


static inline bool CanControlProjectileAllyTeam(const lua_State *L, int allyTeamID)
{
	const int ctrlTeam = CtrlTeam(L);
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	if (allyTeamID < 0) {
		return false;
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyTeamID);
}

#endif // LUA_HELPER_H
