#ifndef LUAHELPER_H
#define LUAHELPER_H
#include "Sim/Misc/TeamHandler.h"

/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline bool FullCtrl(const lua_State *L)
{
	return CLuaHandle::GetFullCtrl(L);
}


static inline int CtrlTeam(const lua_State *L)
{
	return CLuaHandle::GetCtrlTeam(L);
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

#endif // LUAHELPER_H
