/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaUnsyncedCtrl.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "LuaTextures.h"

#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/InMapDraw.h"
#include "Game/InMapDrawModel.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/BaseGroundTextures.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/NetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Util.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/WindowManagerHelper.h"

#include <boost/cstdint.hpp>
#include "System/Platform/Misc.h"
#include "LuaHelper.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
	#include "System/Sound/EFX.h"
	#include "System/Sound/EFXPresets.h"
#endif

#include <map>
#include <set>
#include <cctype>
#include <cfloat>

#include <fstream>

#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

using std::min;
using std::max;

// MinGW defines this for a WINAPI function
#undef SendMessage
#undef CreateDirectory

const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions


/******************************************************************************/
/******************************************************************************/

std::set<int> drawCmdQueueUnits;

/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedCtrl::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(Echo);
	REGISTER_LUA_CFUNC(Log);

	REGISTER_LUA_CFUNC(SendMessage);
	REGISTER_LUA_CFUNC(SendMessageToPlayer);
	REGISTER_LUA_CFUNC(SendMessageToTeam);
	REGISTER_LUA_CFUNC(SendMessageToAllyTeam);
	REGISTER_LUA_CFUNC(SendMessageToSpectators);

	REGISTER_LUA_CFUNC(LoadSoundDef);
	REGISTER_LUA_CFUNC(PlaySoundFile);
	REGISTER_LUA_CFUNC(PlaySoundStream);
	REGISTER_LUA_CFUNC(StopSoundStream);
	REGISTER_LUA_CFUNC(PauseSoundStream);
	REGISTER_LUA_CFUNC(SetSoundStreamVolume);
	REGISTER_LUA_CFUNC(SetSoundEffectParams);

	REGISTER_LUA_CFUNC(SetCameraState);
	REGISTER_LUA_CFUNC(SetCameraTarget);

	REGISTER_LUA_CFUNC(SelectUnitMap);
	REGISTER_LUA_CFUNC(SelectUnitArray);

	REGISTER_LUA_CFUNC(AddWorldIcon);
	REGISTER_LUA_CFUNC(AddWorldText);
	REGISTER_LUA_CFUNC(AddWorldUnit);

	REGISTER_LUA_CFUNC(DrawUnitCommands);

	REGISTER_LUA_CFUNC(SetTeamColor);

	REGISTER_LUA_CFUNC(AssignMouseCursor);
	REGISTER_LUA_CFUNC(ReplaceMouseCursor);

	REGISTER_LUA_CFUNC(SetCustomCommandDrawData);

	REGISTER_LUA_CFUNC(SetDrawSky);
	REGISTER_LUA_CFUNC(SetDrawWater);
	REGISTER_LUA_CFUNC(SetDrawGround);

	REGISTER_LUA_CFUNC(SetWaterParams);

	REGISTER_LUA_CFUNC(AddMapLight);
	REGISTER_LUA_CFUNC(AddModelLight);
	REGISTER_LUA_CFUNC(UpdateMapLight);
	REGISTER_LUA_CFUNC(UpdateModelLight);
	REGISTER_LUA_CFUNC(SetMapLightTrackingState);
	REGISTER_LUA_CFUNC(SetModelLightTrackingState);
	REGISTER_LUA_CFUNC(SetMapSquareTexture);

	REGISTER_LUA_CFUNC(SetUnitNoDraw);
	REGISTER_LUA_CFUNC(SetUnitNoMinimap);
	REGISTER_LUA_CFUNC(SetUnitNoSelect);
	REGISTER_LUA_CFUNC(SetUnitLeaveTracks);

	REGISTER_LUA_CFUNC(AddUnitIcon);
	REGISTER_LUA_CFUNC(FreeUnitIcon);

	REGISTER_LUA_CFUNC(ExtractModArchiveFile);

	// moved from LuaUI

//FIXME	REGISTER_LUA_CFUNC(SetShockFrontFactors);

	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);

	REGISTER_LUA_CFUNC(CreateDir);

	REGISTER_LUA_CFUNC(SendCommands);
	REGISTER_LUA_CFUNC(GiveOrder);
	REGISTER_LUA_CFUNC(GiveOrderToUnit);
	REGISTER_LUA_CFUNC(GiveOrderToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderToUnitArray);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitArray);

	REGISTER_LUA_CFUNC(SendLuaUIMsg);
	REGISTER_LUA_CFUNC(SendLuaGaiaMsg);
	REGISTER_LUA_CFUNC(SendLuaRulesMsg);

	REGISTER_LUA_CFUNC(LoadCmdColorsConfig);
	REGISTER_LUA_CFUNC(LoadCtrlPanelConfig);

	REGISTER_LUA_CFUNC(SetActiveCommand);
	REGISTER_LUA_CFUNC(ForceLayoutUpdate);

	REGISTER_LUA_CFUNC(SetMouseCursor);
	REGISTER_LUA_CFUNC(WarpMouse);

	REGISTER_LUA_CFUNC(SetCameraOffset);

	REGISTER_LUA_CFUNC(SetLosViewColors);

	REGISTER_LUA_CFUNC(Restart);
	REGISTER_LUA_CFUNC(SetWMIcon);
	REGISTER_LUA_CFUNC(SetWMCaption);

	REGISTER_LUA_CFUNC(SetUnitDefIcon);
	REGISTER_LUA_CFUNC(SetUnitDefImage);

	REGISTER_LUA_CFUNC(SetUnitGroup);

	REGISTER_LUA_CFUNC(SetShareLevel);
	REGISTER_LUA_CFUNC(ShareResources);

	REGISTER_LUA_CFUNC(SetLastMessagePosition);

	REGISTER_LUA_CFUNC(MarkerAddPoint);
	REGISTER_LUA_CFUNC(MarkerAddLine);
	REGISTER_LUA_CFUNC(MarkerErasePosition);

	REGISTER_LUA_CFUNC(SetDrawSelectionInfo);

	REGISTER_LUA_CFUNC(SetBuildSpacing);
	REGISTER_LUA_CFUNC(SetBuildFacing);

	REGISTER_LUA_CFUNC(SetSunParameters);
	REGISTER_LUA_CFUNC(SetSunManualControl);
	REGISTER_LUA_CFUNC(SetSunDirection);

	REGISTER_LUA_CFUNC(SendSkirmishAIMessage);

	REGISTER_LUA_CFUNC(ClearWatchDogTimer);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CProjectile* ParseRawProjectile(lua_State* L, const char* caller, int index, bool synced)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "[%s] projectile ID parameter in %s() not a number\n", __FUNCTION__, caller);
		}
		return NULL;
	}

	const int projID = lua_toint(L, index);

	const ProjectileMapValPair* pp = NULL;
	if (synced) {
		pp = projectileHandler->GetMapPairBySyncedID(projID);
	} else {
		pp = projectileHandler->GetMapPairByUnsyncedID(projID);
	}

	return (pp) ? pp->first : NULL;
}

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "[%s] unit ID parameter in %s() not a number\n", __FUNCTION__, caller);
		}
		return NULL;
	}

	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}

	return unitHandler->units[unitID];
}


static inline CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		return CLuaHandle::GetHandleFullRead(L) ? unit : NULL;
	}
	return (unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L)) ? unit : NULL;
}


static inline CUnit* ParseCtrlUnit(lua_State* L,
                                     const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return (CanControlTeam(L, unit->team) ? unit : NULL);
}


static inline CUnit* ParseSelectUnit(lua_State* L,
                                     const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL || unit->noSelect) {
		return NULL;
	}
	const int selectTeam = CLuaHandle::GetHandleSelectTeam(L);
	if (selectTeam < 0) {
		return (selectTeam == CEventClient::AllAccessTeam) ? unit : NULL;
	}
	if (selectTeam == unit->team) {
		return unit;
	}
	return NULL;
}


/******************************************************************************/
/******************************************************************************/

void LuaUnsyncedCtrl::DrawUnitCommandQueues()
{
	if (drawCmdQueueUnits.empty()) {
		return;
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	lineDrawer.Configure(cmdColors.UseColorRestarts(),
	                     cmdColors.UseRestartColor(),
	                     cmdColors.restart,
	                     cmdColors.RestartAlpha());
	lineDrawer.SetupLineStipple();

	glEnable(GL_BLEND);
	glBlendFunc((GLenum)cmdColors.QueuedBlendSrc(),
	            (GLenum)cmdColors.QueuedBlendDst());

	glLineWidth(cmdColors.QueuedLineWidth());

	GML_RECMUTEX_LOCK(unit); // DrawUnitCommandQueues
	GML_RECMUTEX_LOCK(feat); // DrawUnitCommandQueues
	GML_STDMUTEX_LOCK(cai); // DrawUnitCommandQueues
	GML_STDMUTEX_LOCK(dque); // DrawUnitCommandQueues

	std::set<int>::const_iterator ui;

	for (ui = drawCmdQueueUnits.begin(); ui != drawCmdQueueUnits.end(); ++ui) {
		const CUnit* unit = unitHandler->GetUnit(*ui);

		if (unit == NULL || unit->commandAI == NULL) {
			continue;
		}

		commandDrawer->Draw(unit->commandAI);
	}

	glLineWidth(1.0f);
	glEnable(GL_DEPTH_TEST);
}


void LuaUnsyncedCtrl::ClearUnitCommandQueues()
{
	GML_STDMUTEX_LOCK(dque); // ClearUnitCommandQueues

	drawCmdQueueUnits.clear();
}


/******************************************************************************/
/******************************************************************************/
//
//  The call-outs
//

int LuaUnsyncedCtrl::Echo(lua_State* L)
{
	return LuaUtils::Echo(L);
}

int LuaUnsyncedCtrl::Log(lua_State* L)
{
	return LuaUtils::Log(L);
}

static string ParseMessage(lua_State* L, const string& msg)
{
	string::size_type start = msg.find("<PLAYER");
	if (start == string::npos) {
		return msg;
	}

	const char* number = msg.c_str() + start + strlen("<PLAYER");
	char* endPtr;
	const int playerID = (int)strtol(number, &endPtr, 10);
	if ((endPtr == number) || (*endPtr != '>')) {
		luaL_error(L, "Bad message format: %s", msg.c_str());
	}

	if (!playerHandler->IsValidPlayer(playerID)) {
		luaL_error(L, "Invalid message playerID: %i", playerID);
	}
	const CPlayer* player = playerHandler->Player(playerID);
	if ((player == NULL) || !player->active || player->name.empty()) {
		luaL_error(L, "Invalid message playerID: %i", playerID);
	}

	const string head = msg.substr(0, start);
	const string tail = msg.substr(endPtr - msg.c_str() + 1);

	return head + player->name + ParseMessage(L, tail);
}


static void PrintMessage(lua_State* L, const string& msg)
{
	LOG("%s", ParseMessage(L, msg).c_str());
}


int LuaUnsyncedCtrl::SendMessage(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to SendMessage()");
	}
	PrintMessage(L, lua_tostring(L, 1));
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToSpectators(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to SendMessageToSpectators()");
	}
	if (gu->spectating) {
		PrintMessage(L, lua_tostring(L, 1));
	}
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToPlayer(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SendMessageToPlayer()");
	}
	const int playerID = lua_toint(L, 1);
	if (playerID == gu->myPlayerNum) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToTeam(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SendMessageToTeam()");
	}
	const int teamID = lua_toint(L, 1);
	if (teamID == gu->myTeam) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToAllyTeam(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SendMessageToAllyTeam()");
	}
	const int allyTeamID = lua_toint(L, 1);
	if (allyTeamID == gu->myAllyTeam) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::LoadSoundDef(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to LoadSoundDef()");
	}

	const string soundFile = lua_tostring(L, 1);
	bool success = sound->LoadSoundDefs(soundFile);

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, success);
		return 1;
	} else {
		return 0;
	}
}

int LuaUnsyncedCtrl::PlaySoundFile(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to PlaySoundFile()");
	}
	bool success = false;
	const string soundFile = lua_tostring(L, 1);
	const unsigned int soundID = sound->GetSoundId(soundFile);
	if (soundID > 0) {
		float volume = luaL_optfloat(L, 2, 1.0f);
		float3 pos;
		float3 speed;
		bool pos_given = false;
		bool speed_given = false;

		int index = 3;
		if (args >= 5 && lua_isnumber(L, 3) && lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
			pos = float3(lua_tofloat(L, 3), lua_tofloat(L, 4), lua_tofloat(L, 5));
			pos_given = true;
			index += 3;

			if (args >= 8 && lua_isnumber(L, 6) && lua_isnumber(L, 7) && lua_isnumber(L, 8))
			{
				speed = float3(lua_tofloat(L, 6), lua_tofloat(L, 7), lua_tofloat(L, 8));
				speed_given = true;
				index += 3;
			}
		}

		//! last argument (with and without pos/speed arguments) is the optional `sfx channel`
		AudioChannelImpl* channel = &Channels::General;
		if (args >= index) {
			if (lua_isstring(L, index)) {
				string channelStr = lua_tostring(L, index);
				StringToLowerInPlace(channelStr);

				if (channelStr == "battle" || channelStr == "sfx") {
					channel = &Channels::Battle;
				}
				else if (channelStr == "unitreply" || channelStr == "voice") {
					channel = &Channels::UnitReply;
				}
				else if (channelStr == "userinterface" || channelStr == "ui") {
					channel = &Channels::UserInterface;
				}
			} else if (lua_isnumber(L, index)) {
				const int channelNum = lua_toint(L, index);

				if (channelNum == 1) {
					channel = &Channels::Battle;
				}
				else if (channelNum == 2) {
					channel = &Channels::UnitReply;
				}
				else if (channelNum == 3) {
					channel = &Channels::UserInterface;
				}
			}
		}

		if (pos_given) {
			if (speed_given) {
				channel->PlaySample(soundID, pos, speed, volume);
			} else {
				channel->PlaySample(soundID, pos, volume);
			}
		} else
			channel->PlaySample(soundID, volume);

		success = true;
	}

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, success);
		return 1;
	} else {
		return 0;
	}
}


int LuaUnsyncedCtrl::PlaySoundStream(lua_State* L)
{
	const int args = lua_gettop(L);

	const string soundFile = luaL_checkstring(L, 1);
	const float volume = luaL_optnumber(L, 2, 1.0f);
	bool enqueue = false;
	if (args >= 3)
		enqueue = lua_toboolean(L, 3);

	Channels::BGMusic.StreamPlay(soundFile, volume, enqueue);

	// .ogg files don't have sound ID's generated
	// for them (yet), so we always succeed here
	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, true);
		return 1;
	} else {
		return 0;
	}
}

int LuaUnsyncedCtrl::StopSoundStream(lua_State*)
{
	Channels::BGMusic.StreamStop();
	return 0;
}
int LuaUnsyncedCtrl::PauseSoundStream(lua_State*)
{
	Channels::BGMusic.StreamPause();
	return 0;
}
int LuaUnsyncedCtrl::SetSoundStreamVolume(lua_State* L)
{
	Channels::BGMusic.SetVolume(luaL_checkfloat(L, 1));
	return 0;
}


int LuaUnsyncedCtrl::SetSoundEffectParams(lua_State* L)
{
#if !defined(HEADLESS) && !defined(NO_SOUND)
	if (!efx)
		return 0;

	//! only a preset name given?
	if (lua_israwstring(L, 1)) {
		const std::string presetname = lua_tostring(L, 1);
		efx->SetPreset(presetname, false);
		return 0;
	}

	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetSoundEffectParams()");
	}

	//! first parse the 'preset' key (so all following params use it as base and override it)
	lua_pushliteral(L, "preset");
	lua_gettable(L, -2);
	if (lua_israwstring(L, -1)) {
		std::string presetname = lua_tostring(L, -1);
		efx->SetPreset(presetname, false, false);
	}
	lua_pop(L, 1);


	if (!efx->sfxProperties)
		return 0;

	EAXSfxProps* efxprops = efx->sfxProperties;


	//! parse pass filter
	lua_pushliteral(L, "passfilter");
	lua_gettable(L, -2);
	if (lua_istable(L, -1)) {
		for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string key = StringToLower(lua_tostring(L, -2));
				std::map<std::string, ALuint>::iterator it = nameToALFilterParam.find(key);
				if (it != nameToALFilterParam.end()) {
					ALuint param = it->second;
					if (lua_isnumber(L, -1)) {
						if (alParamType[param] == EFXParamTypes::FLOAT) {
							const float value = lua_tofloat(L, -1);
							efxprops->filter_properties_f[param] = value;
						}
					}
				}
			}
		}
	}
	lua_pop(L, 1);

	//! parse EAX Reverb
	lua_pushliteral(L, "reverb");
	lua_gettable(L, -2);
	if (lua_istable(L, -1)) {
		for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string key = StringToLower(lua_tostring(L, -2));
				std::map<std::string, ALuint>::iterator it = nameToALParam.find(key);
				if (it != nameToALParam.end()) {
					ALuint param = it->second;
					if (lua_istable(L, -1)) {
						if (alParamType[param] == EFXParamTypes::VECTOR) {
							float3 v;
							const int size = LuaUtils::ParseFloatArray(L, -1, &v[0], 3);
							if (size >= 3) {
								efxprops->properties_v[param] = v;
							}
						}
					}
					else if (lua_isnumber(L, -1)) {
						if (alParamType[param] == EFXParamTypes::FLOAT) {
							const float value = lua_tofloat(L, -1);
							efxprops->properties_f[param] = value;
						}
					}
					else if (lua_isboolean(L, -1)) {
						if (alParamType[param] == EFXParamTypes::BOOL) {
							const bool value = lua_toboolean(L, -1);
							efxprops->properties_i[param] = value;
						}
					}
				}
			}
		}
	}
	lua_pop(L, 1);

	//! commit effects
	efx->CommitEffects();
#endif /// !defined(HEADLESS) && !defined(NO_SOUND)

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::AddWorldIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to AddWorldIcon(id, x, y, z");
	}
	const int cmdID = lua_toint(L, 1);
	const float3 pos(lua_tofloat(L, 2),
	                 lua_tofloat(L, 3),
	                 lua_tofloat(L, 4));
	cursorIcons.AddIcon(cmdID, pos);
	return 0;
}


int LuaUnsyncedCtrl::AddWorldText(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to AddWorldIcon(text, x, y, z");
	}
	const string text = lua_tostring(L, 1);
	const float3 pos(lua_tofloat(L, 2),
	                 lua_tofloat(L, 3),
	                 lua_tofloat(L, 4));
	cursorIcons.AddIconText(text, pos);
	return 0;
}


int LuaUnsyncedCtrl::AddWorldUnit(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 6) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		luaL_error(L,
			"Incorrect arguments to AddWorldUnit(unitDefID, x, y, z, team, facing)");
	}
	const int unitDefID = lua_toint(L, 1);
	if (!unitDefHandler->IsValidUnitDefID(unitDefID)) {
		return 0;
	}
	const float3 pos(lua_tofloat(L, 2),
	                 lua_tofloat(L, 3),
	                 lua_tofloat(L, 4));
	const int teamId = lua_toint(L, 5);
	if (!teamHandler->IsValidTeam(teamId)) {
		return 0;
	}
	const int facing = lua_toint(L, 6);
	cursorIcons.AddBuildIcon(-unitDefID, pos, teamId, facing);
	return 0;
}


int LuaUnsyncedCtrl::DrawUnitCommands(lua_State* L)
{
	GML_STDMUTEX_LOCK(dque); // DrawUnitCommands

	if (lua_istable(L, 1)) {
		const bool isMap = lua_isboolean(L, 2) && lua_toboolean(L, 2);
		const int unitArg = isMap ? -2 : -1;
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwnumber(L, -2)) {
				CUnit* unit = ParseAllyUnit(L, __FUNCTION__, unitArg);
				if (unit != NULL) {
					drawCmdQueueUnits.insert(unit->id);
				}
			}
		}
		return 0;
	}

	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit != NULL) {
		drawCmdQueueUnits.insert(unit->id);
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetCameraTarget(lua_State* L)
{
	if (mouse == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetCameraTarget(x, y, z)");
	}
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));

	float transTime = 0.5f;
	if ((args >= 4) && lua_isnumber(L, 4)) {
		transTime = lua_tofloat(L, 4);
	}

	camHandler->CameraTransition(transTime);
	camHandler->GetCurrentController().SetPos(pos);

	return 0;
}


int LuaUnsyncedCtrl::SetCameraState(lua_State* L)
{
	if (mouse == NULL) {
		return 0;
	}

	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetCameraState(table, camTime)");
	}

	const float camTime = luaL_checkfloat(L, 2);

	CCameraController::StateMap camState;

	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const string key = lua_tostring(L, -2);
			if (lua_isnumber(L, -1)) {
				camState[key] = lua_tofloat(L, -1);
			}
			else if (lua_isboolean(L, -1)) {
				camState[key] = lua_toboolean(L, -1) ? +1.0f : -1.0f;
			}
		}
	}

	camHandler->CameraTransition(camTime);
	lua_pushboolean(L, camHandler->SetState(camState));

	if (!CLuaHandle::GetHandleSynced(L)) {
		return 1;
	} else {
		return 0;
	}
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SelectUnitArray(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_istable(L, 1) ||
	    ((args >= 2) && !lua_isboolean(L, 2))) {
		luaL_error(L, "Incorrect arguments to SelectUnitArray()");
	}

	// clear the current units, unless the append flag is present
	if ((args < 2) || !lua_toboolean(L, 2)) {
		selectedUnitsHandler.ClearSelected();
	}

	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {     // avoid 'n'
			CUnit* unit = ParseSelectUnit(L, __FUNCTION__, -1); // the value
			if (unit != NULL) {
				selectedUnitsHandler.AddUnit(unit);
			}
		}
	}
	return 0;
}


int LuaUnsyncedCtrl::SelectUnitMap(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_istable(L, 1) ||
	    ((args >= 2) && !lua_isboolean(L, 2))) {
		luaL_error(L, "Incorrect arguments to SelectUnitMap()");
	}

	// clear the current units, unless the append flag is present
	if ((args < 2) || !lua_toboolean(L, 2)) {
		selectedUnitsHandler.ClearSelected();
	}

	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseSelectUnit(L, __FUNCTION__, -2); // the key
			if (unit != NULL) {
				selectedUnitsHandler.AddUnit(unit);
			}
		}
	}

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetTeamColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}
	const float r = max(0.0f, min(1.0f, luaL_checkfloat(L, 2)));
	const float g = max(0.0f, min(1.0f, luaL_checkfloat(L, 3)));
	const float b = max(0.0f, min(1.0f, luaL_checkfloat(L, 4)));
	team->color[0] = (unsigned char)(r * 255.0f);
	team->color[1] = (unsigned char)(g * 255.0f);
	team->color[2] = (unsigned char)(b * 255.0f);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::AssignMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to AssignMouseCursor()");
	}

	const string cmdName  = lua_tostring(L, 1);
	const string fileName = lua_tostring(L, 2);

	bool overwrite = true;
	if ((args >= 3) && lua_isboolean(L, 3)) {
		overwrite = lua_toboolean(L, 3);
	}

	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;
	if ((args >= 4) && lua_isboolean(L, 4)) {
		if (lua_toboolean(L, 4)) {
			hotSpot = CMouseCursor::TopLeft;
		}
	}

	const bool worked = mouse->AssignMouseCursor(cmdName, fileName, hotSpot, overwrite);

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, worked);
		return 1;
	}

	return 0;
}


int LuaUnsyncedCtrl::ReplaceMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to ReplaceMouseCursor()");
	}

	const string oldName = lua_tostring(L, 1);
	const string newName = lua_tostring(L, 2);

	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;
	if ((args >= 3) && lua_isboolean(L, 3)) {
		if (lua_toboolean(L, 3)) {
			hotSpot = CMouseCursor::TopLeft;
		}
	}

	const bool worked = mouse->ReplaceMouseCursor(oldName, newName, hotSpot);

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, worked);
		return 1;
	}

	return 0;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetCustomCommandDrawData(lua_State* L)
{
	const int cmdID = luaL_checkint(L, 1);

	int iconID = 0;
	if (lua_israwnumber(L, 2)) {
		iconID = lua_toint(L, 2);
	}
	else if (lua_israwstring(L, 2)) {
		iconID = cmdID;
		const string icon = lua_tostring(L, 2);
		cursorIcons.SetCustomType(cmdID, icon);
	}
	else if (lua_isnil(L, 2)) {
		cursorIcons.SetCustomType(cmdID, "");
		cmdColors.ClearCustomCmdData(cmdID);
		return 0;
	}
	else {
		luaL_error(L, "Incorrect arguments to SetCustomCommandDrawData");
	}

	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	const int table = 3;
	if (lua_istable(L, table)) {
		for (int i = 0; i < 4; i++) {
			lua_rawgeti(L, table, i + 1);
			if (lua_israwnumber(L, -1)) {
				color[i] = lua_tofloat(L, -1);
				lua_pop(L, 1);
			} else {
				lua_pop(L, 1);
				break;
			}
		}
	}

	const bool showArea = lua_isboolean(L, 4) && lua_toboolean(L, 4);

	cmdColors.SetCustomCmdData(cmdID, iconID, color, showArea);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetDrawSky(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawSky()");
	}
	globalRendering->drawSky = !!lua_toboolean(L, 1);
	return 0;
}


int LuaUnsyncedCtrl::SetDrawWater(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawWater()");
	}
	globalRendering->drawWater = !!lua_toboolean(L, 1);
	return 0;
}


int LuaUnsyncedCtrl::SetDrawGround(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawGround()");
	}
	globalRendering->drawGround = !!lua_toboolean(L, 1);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetWaterParams(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!gs->cheatEnabled) {
		LOG("SetWaterParams() needs cheating enabled");
		return 0;
	}
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetWaterParams()");
	}

	CMapInfo::water_t& w = const_cast<CMapInfo*>(mapInfo)->water;
	for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const string key = lua_tostring(L, -2);
			if (lua_istable(L, -1)) {
				float color[3];
				const int size = LuaUtils::ParseFloatArray(L, -1, color, 3);
				if (size >= 3) {
					if (key == "absorb") {
						w.absorb = color;
					} else if (key == "baseColor") {
						w.baseColor = color;
					} else if (key == "minColor") {
						w.minColor = color;
					} else if (key == "surfaceColor") {
						w.surfaceColor = color;
					} else if (key == "diffuseColor") {
						w.diffuseColor = color;
					} else if (key == "specularColor") {
						w.specularColor = color;
 					} else if (key == "planeColor") {
						w.planeColor.x = color[0];
						w.planeColor.y = color[1];
						w.planeColor.z = color[2];
					}
				}
			}
			else if (lua_israwstring(L, -1)) {
				const std::string value = lua_tostring(L, -1);
				if (key == "texture") {
					w.texture = value;
				} else if (key == "foamTexture") {
					w.foamTexture = value;
				} else if (key == "normalTexture") {
					w.normalTexture = value;
				}
			}
			else if (lua_isnumber(L, -1)) {
				const float value = lua_tofloat(L, -1);
				if (key == "damage") {
					w.damage = value;
				} else if (key == "repeatX") {
					w.repeatX = value;
				} else if (key == "repeatY") {
					w.repeatY = value;
				} else if (key == "surfaceAlpha") {
					w.surfaceAlpha = value;
				} else if (key == "ambientFactor") {
					w.ambientFactor = value;
				} else if (key == "diffuseFactor") {
					w.diffuseFactor = value;
				} else if (key == "specularFactor") {
					w.specularFactor = value;
				} else if (key == "specularPower") {
					w.specularPower = value;
				} else if (key == "fresnelMin") {
					w.fresnelMin = value;
				} else if (key == "fresnelMax") {
					w.fresnelMax = value;
				} else if (key == "fresnelPower") {
					w.fresnelPower = value;
				} else if (key == "reflectionDistortion") {
					w.reflDistortion = value;
				} else if (key == "blurBase") {
					w.blurBase = value;
				} else if (key == "blurExponent") {
					w.blurExponent = value;
				} else if (key == "perlinStartFreq") {
					w.perlinStartFreq = value;
				} else if (key == "perlinLacunarity") {
					w.perlinLacunarity = value;
				} else if (key == "perlinAmplitude") {
					w.perlinAmplitude = value;
				} else if (key == "numTiles") {
					w.numTiles = (unsigned char)value;
				}
			}
			else if (lua_isboolean(L, -1)) {
				const bool value = lua_toboolean(L, -1);
				if (key == "shoreWaves") {
					w.shoreWaves = value;
				} else if (key == "forceRendering") {
					w.forceRendering = value;
				} else if (key == "hasWaterPlane") {
					w.hasWaterPlane = value;
				}
			}
		}
	}

	return 0;
}



static bool ParseLight(lua_State* L, GL::Light& light, const int tblIdx, const char* caller)
{
	if (!lua_istable(L, tblIdx)) {
		luaL_error(L, "[%s] argument %i must be a table!", caller, tblIdx);
		return false;
	}

	for (lua_pushnil(L); lua_next(L, tblIdx) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const std::string& key = lua_tostring(L, -2);

			if (lua_istable(L, -1)) {
				float array[3] = {0.0f, 0.0f, 0.0f};

				if (LuaUtils::ParseFloatArray(L, -1, array, 3) == 3) {
					if (key == "position") {
						light.SetPosition(array);
					} else if (key == "direction") {
						light.SetDirection(array);
					} else if (key == "ambientColor") {
						light.SetAmbientColor(array);
					} else if (key == "diffuseColor") {
						light.SetDiffuseColor(array);
					} else if (key == "specularColor") {
						light.SetSpecularColor(array);
					} else if (key == "intensityWeight") {
						light.SetIntensityWeight(array);
					} else if (key == "attenuation") {
						light.SetAttenuation(array);
					} else if (key == "ambientDecayRate") {
						light.SetAmbientDecayRate(array);
					} else if (key == "diffuseDecayRate") {
						light.SetDiffuseDecayRate(array);
					} else if (key == "specularDecayRate") {
						light.SetSpecularDecayRate(array);
					} else if (key == "decayFunctionType") {
						light.SetDecayFunctionType(array);
					}
				}

				continue;
			}

			if (lua_isnumber(L, -1)) {
				if (key == "radius") {
					light.SetRadius(std::max(1.0f, lua_tofloat(L, -1)));
				} else if (key == "fov") {
					light.SetFOV(std::max(0.0f, std::min(180.0f, lua_tofloat(L, -1))));
				} else if (key == "ttl") {
					light.SetTTL(lua_tofloat(L, -1));
				} else if (key == "priority") {
					light.SetPriority(lua_tofloat(L, -1));
				} else if (key == "ignoreLOS") {
					light.SetIgnoreLOS(lua_toboolean(L, -1));
				}

				continue;
			}
		}
	}

	return true;
}


int LuaUnsyncedCtrl::AddMapLight(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}

	GL::LightHandler* lightHandler = readmap->GetGroundDrawer()->GetLightHandler();
	GL::Light light;

	unsigned int lightHandle = -1U;

	if (lightHandler != NULL) {
		if (ParseLight(L, light, 1, __FUNCTION__)) {
			lightHandle = lightHandler->AddLight(light);
		}
	}

	lua_pushnumber(L, lightHandle);
	return 1;
}

int LuaUnsyncedCtrl::AddModelLight(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}

	GL::LightHandler* lightHandler = unitDrawer->GetLightHandler();
	GL::Light light;

	unsigned int lightHandle = -1U;

	if (lightHandler != NULL) {
		if (ParseLight(L, light, 1, __FUNCTION__)) {
			lightHandle = lightHandler->AddLight(light);
		}
	}

	lua_pushnumber(L, lightHandle);
	return 1;
}


int LuaUnsyncedCtrl::UpdateMapLight(lua_State* L)
{
	const unsigned int lightHandle = luaL_checkint(L, 1);

	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}

	GL::LightHandler* lightHandler = readmap->GetGroundDrawer()->GetLightHandler();
	GL::Light* light = (lightHandler != NULL)? lightHandler->GetLight(lightHandle): NULL;
	bool ret = false;

	if (light != NULL) {
		ret = ParseLight(L, *light, 2, __FUNCTION__);
	}

	lua_pushboolean(L, ret);
	return 1;
}

int LuaUnsyncedCtrl::UpdateModelLight(lua_State* L)
{
	const unsigned int lightHandle = luaL_checkint(L, 1);

	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}

	GL::LightHandler* lightHandler = unitDrawer->GetLightHandler();
	GL::Light* light = (lightHandler != NULL)? lightHandler->GetLight(lightHandle): NULL;
	bool ret = false;

	if (light != NULL) {
		ret = ParseLight(L, *light, 2, __FUNCTION__);
	}

	lua_pushboolean(L, ret);
	return 1;
}


static bool AddLightTrackingTarget(lua_State* L, GL::Light* light, bool trackEnable, bool trackUnit, const char* caller)
{
	bool ret = false;

	if (trackUnit) {
		// interpret argument #2 as a unit ID
		CUnit* unit = ParseAllyUnit(L, caller, 2);

		if (unit != NULL) {
			if (trackEnable) {
				if (light->GetTrackPosition() == NULL) {
					light->AddDeathDependence(unit, DEPENDENCE_LIGHT);
					light->SetTrackPosition(&unit->drawPos);
					light->SetTrackDirection(&unit->speed); //! non-normalized
					ret = true;
				}
			} else {
				// assume <light> was tracking <unit>
				if (light->GetTrackPosition() == &unit->drawPos) {
					light->DeleteDeathDependence(unit, DEPENDENCE_LIGHT);
					light->SetTrackPosition(NULL);
					light->SetTrackDirection(NULL);
					ret = true;
				}
			}
		}
	} else {
		// interpret argument #2 as a projectile ID
		//
		// only track synced projectiles (LuaSynced
		// does not know about unsynced ID's anyway)
		CProjectile* proj = ParseRawProjectile(L, caller, 2, true);

		if (proj != NULL) {
			if (trackEnable) {
				if (light->GetTrackPosition() == NULL) {
					light->AddDeathDependence(proj, DEPENDENCE_LIGHT);
					light->SetTrackPosition(&proj->drawPos);
					light->SetTrackDirection(&proj->dir);
					ret = true;
				}
			} else {
				// assume <light> was tracking <proj>
				if (light->GetTrackPosition() == &proj->drawPos) {
					light->DeleteDeathDependence(proj, DEPENDENCE_LIGHT);
					light->SetTrackPosition(NULL);
					light->SetTrackDirection(NULL);
					ret = true;
				}
			}
		}
	}

	return ret;
}

// set a map-illuminating light to start/stop tracking
// the position of a moving object (unit or projectile)
int LuaUnsyncedCtrl::SetMapLightTrackingState(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}
	if (!lua_isnumber(L, 2)) {
		luaL_error(L, "[%s] 1st and 2nd arguments should be numbers, 3rd and 4th should be booleans", __FUNCTION__);
		return 0;
	}

	const unsigned int lightHandle = luaL_checkint(L, 1);
	const bool trackEnable = lua_isboolean(L, 3)? lua_toboolean(L, 3): true;
	const bool trackUnit = lua_isboolean(L, 4)? lua_toboolean(L, 4): true;

	GL::LightHandler* lightHandler = readmap->GetGroundDrawer()->GetLightHandler();
	GL::Light* light = (lightHandler != NULL)? lightHandler->GetLight(lightHandle): NULL;
	bool ret = false;

	if (light != NULL) {
		ret = AddLightTrackingTarget(L, light, trackEnable, trackUnit, __FUNCTION__);
	}

	lua_pushboolean(L, ret);
	return 1;
}

// set a model-illuminating light to start/stop tracking
// the position of a moving object (unit or projectile)
int LuaUnsyncedCtrl::SetModelLightTrackingState(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}
	if (!lua_isnumber(L, 2)) {
		luaL_error(L, "[%s] 1st and 2nd arguments should be numbers, 3rd and 4th should be booleans", __FUNCTION__);
		return 0;
	}

	const unsigned int lightHandle = luaL_checkint(L, 1);
	const bool trackEnable = lua_isboolean(L, 3)? lua_toboolean(L, 3): true;
	const bool trackUnit = lua_isboolean(L, 4)? lua_toboolean(L, 4): true;

	GL::LightHandler* lightHandler = unitDrawer->GetLightHandler();
	GL::Light* light = (lightHandler != NULL)? lightHandler->GetLight(lightHandle): NULL;
	bool ret = false;

	if (light != NULL) {
		ret = AddLightTrackingTarget(L, light, trackEnable, trackUnit, __FUNCTION__);
	}

	lua_pushboolean(L, ret);
	return 1;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetMapSquareTexture(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}

	const int texSquareX = luaL_checkint(L, 1);
	const int texSquareY = luaL_checkint(L, 2);
	const std::string& texName = luaL_checkstring(L, 3);

	CBaseGroundDrawer* groundDrawer = readmap->GetGroundDrawer();
	CBaseGroundTextures* groundTextures = groundDrawer->GetGroundTextures();

	if (groundTextures == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (texName.empty()) {
		// restore default texture for this square
		lua_pushboolean(L, groundTextures->SetSquareLuaTexture(texSquareX, texSquareY, 0));
		return 1;
	}

	// TODO: leaking ID's like this means we need to guard against texture deletion
	const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(texName);
	const CNamedTextures::TexInfo* namedTexture = CNamedTextures::GetInfo(texName);

	if (luaTexture != NULL) {
		if (luaTexture->xsize != luaTexture->ysize) {
			// square textures only
			lua_pushboolean(L, false);
			return 1;
		}

		lua_pushboolean(L, groundTextures->SetSquareLuaTexture(texSquareX, texSquareY, luaTexture->id));
		return 1;
	}

	if (namedTexture != NULL) {
		if (namedTexture->xsize != namedTexture->ysize) {
			// square textures only
			lua_pushboolean(L, false);
			return 1;
		}

		lua_pushboolean(L, groundTextures->SetSquareLuaTexture(texSquareX, texSquareY, namedTexture->id));
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitNoDraw(lua_State* L)
{
	if (CLuaHandle::GetHandleUserMode(L)) {
		return 0;
	}
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitNoDraw()");
		return 0;
	}
	unit->noDraw = lua_toboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitNoMinimap(lua_State* L)
{
	if (CLuaHandle::GetHandleUserMode(L)) {
		return 0;
	}
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitNoMinimap()");
		return 0;
	}
	unit->noMinimap = lua_toboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitNoSelect(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // SetUnitNoSelect

	if (CLuaHandle::GetHandleUserMode(L)) {
		return 0;
	}
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitNoSelect()");
		return 0;
	}
	unit->noSelect = lua_toboolean(L, 2);

	// deselect the unit if it's selected and shouldn't be
	if (unit->noSelect) {
		const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
		if (selUnits.find(unit) != selUnits.end()) {
			selectedUnitsHandler.RemoveUnit(unit);
		}
	}
	return 0;
}


int LuaUnsyncedCtrl::SetUnitLeaveTracks(lua_State* L)
{
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	unit->leaveTracks = lua_toboolean(L, 2);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::AddUnitIcon(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}
	const string iconName  = luaL_checkstring(L, 1);
	const string texName   = luaL_checkstring(L, 2);
	const float  size      = luaL_optnumber(L, 3, 1.0f);
	const float  dist      = luaL_optnumber(L, 4, 1.0f);
	const bool   radAdjust = (lua_isboolean(L, 5) && lua_toboolean(L, 5));
	lua_pushboolean(L, icon::iconHandler->AddIcon(iconName, texName,
	                                        size, dist, radAdjust));
	return 1;
}


int LuaUnsyncedCtrl::FreeUnitIcon(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}
	const string iconName  = luaL_checkstring(L, 1);
	lua_pushboolean(L, icon::iconHandler->FreeIcon(iconName));
	return 1;
}


/******************************************************************************/

// TODO: move this to LuaVFS?
int LuaUnsyncedCtrl::ExtractModArchiveFile(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}

	const string path = luaL_checkstring(L, 1);

	CFileHandler fhVFS(path, SPRING_VFS_MOD);
	CFileHandler fhRAW(path, SPRING_VFS_RAW);

	if (!fhVFS.FileExists()) {
		luaL_error(L, "file \"%s\" not found in mod archive", path.c_str());
		return 0;
	}

	if (fhRAW.FileExists()) {
		luaL_error(L, "cannot extract file \"%s\": already exists", path.c_str());
		return 0;
	}


	string dname = FileSystem::GetDirectory(path);
	string fname = FileSystem::GetFilename(path);

#ifdef WIN32
	const size_t s = dname.size();
	// get rid of any trailing slashes (CreateDirectory()
	// fails on at least XP and Vista if they are present,
	// ie. it creates the dir but actually returns false)
	if ((s > 0) && ((dname[s - 1] == '/') || (dname[s - 1] == '\\'))) {
		dname = dname.substr(0, s - 1);
	}
#endif

	if (!dname.empty() && !FileSystem::CreateDirectory(dname)) {
		luaL_error(L, "Could not create directory \"%s\" for file \"%s\"",
		           dname.c_str(), fname.c_str());
	}

	const int numBytes = fhVFS.FileSize();
	char* buffer = new char[numBytes];

	fhVFS.Read(buffer, numBytes);

	std::fstream fstr(path.c_str(), std::ios::out | std::ios::binary);
	fstr.write((const char*) buffer, numBytes);
	fstr.close();

	if (!dname.empty()) {
		LOG("Extracted file \"%s\" to directory \"%s\"",
				fname.c_str(), dname.c_str());
	} else {
		LOG("Extracted file \"%s\"", fname.c_str());
	}

	delete[] buffer;

	lua_pushboolean(L, true);
	return 1;
}


/******************************************************************************/
/******************************************************************************/
//
// moved from LuaUI
//
/******************************************************************************/
/******************************************************************************/


int LuaUnsyncedCtrl::SendCommands(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if ((guihandler == NULL) || gs->noHelperAIs) {
		return 0;
	}

	vector<string> cmds;

	if (lua_istable(L, 1)) { // old style -- table
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -1)) {
				string action = lua_tostring(L, -1);
				if (action[0] != '@') {
					action = "@@" + action;
				}
				cmds.push_back(action);
			}
		}
	}
	else if (lua_israwstring(L, 1)) { // new style -- function parameters
		for (int i = 1; lua_israwstring(L, i); i++) {
			string action = lua_tostring(L, i);
			if (action[0] != '@') {
				action = "@@" + action;
			}
			cmds.push_back(action);
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SendCommands()");
	}

	lua_settop(L, 0); // pop the input arguments

	configHandler->EnableWriting(globalConfig->luaWritableConfigFile);
	guihandler->RunCustomCommands(cmds, false);
	configHandler->EnableWriting(true);

	return 0;
}


/******************************************************************************/

static int SetActiveCommandByIndex(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const int cmdIndex = lua_toint(L, 1) - CMD_INDEX_OFFSET;
	int button = 1; // LMB
	if ((args >= 2) && lua_isnumber(L, 2)) {
		button = lua_toint(L, 2);
	}

	if (args <= 2) {
		const bool rmb = (button == SDL_BUTTON_LEFT) ? false : true;
		const bool success = guihandler->SetActiveCommand(cmdIndex, rmb);
		lua_pushboolean(L, success);
		return 1;
	}

	// cmdIndex, button, lmb, rmb, alt, ctrl, meta, shift
	if ((args < 8) ||
	    !lua_isboolean(L, 3) || !lua_isboolean(L, 4) || !lua_isboolean(L, 5) ||
	    !lua_isboolean(L, 6) || !lua_isboolean(L, 7) || !lua_isboolean(L, 8)) {
		luaL_error(L, "Incorrect arguments to SetActiveCommand()");
	}
	const bool lmb   = lua_toboolean(L, 3);
	const bool rmb   = lua_toboolean(L, 4);
	const bool alt   = lua_toboolean(L, 5);
	const bool ctrl  = lua_toboolean(L, 6);
	const bool meta  = lua_toboolean(L, 7);
	const bool shift = lua_toboolean(L, 8);

	const bool success = guihandler->SetActiveCommand(cmdIndex, button, lmb, rmb,
	                                                  alt, ctrl, meta, shift);
	lua_pushboolean(L, success);
	return 1;
}


static int SetActiveCommandByAction(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const string text = lua_tostring(L, 1);
	const Action action(text);
	CKeySet ks;
	if (args >= 2) {
		const string ksText = lua_tostring(L, 2);
		ks.Parse(ksText);
	}
	const bool success = guihandler->SetActiveCommand(action, ks, 0);
	lua_pushboolean(L, success);
	return 1;
}


int LuaUnsyncedCtrl::SetActiveCommand(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args < 1) {
		luaL_error(L, "Incorrect arguments to SetActiveCommand()");
	}
	if (lua_isnumber(L, 1)) {
		return SetActiveCommandByIndex(L);
	}
	if (lua_isstring(L, 1)) {
		return SetActiveCommandByAction(L);
	}
	luaL_error(L, "Incorrect arguments to SetActiveCommand()");
	return 0;
}


int LuaUnsyncedCtrl::LoadCmdColorsConfig(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string cfg = luaL_checkstring(L, 1);
	cmdColors.LoadConfigFromString(cfg);
	return 0;
}


int LuaUnsyncedCtrl::LoadCtrlPanelConfig(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const string cfg = luaL_checkstring(L, 1);
	guihandler->ReloadConfigFromString(cfg);
	return 0;
}


int LuaUnsyncedCtrl::ForceLayoutUpdate(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	guihandler->ForceLayoutUpdate();
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::WarpMouse(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to WarpMouse()");
	}
	const int x = lua_toint(L, 1);
	const int y = globalRendering->viewSizeY - lua_toint(L, 2) - 1;
	mouse->WarpMouse(x, y);
	return 0;
}


int LuaUnsyncedCtrl::SetMouseCursor(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}

	const std::string& cursorName = luaL_checkstring(L, 1);
	const float cursorScale = luaL_optfloat(L, 2, 1.0f);

	mouse->ChangeCursor(cursorName, cursorScale);

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetCameraOffset(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (camera == NULL) {
		return 0;
	}
	const float px = luaL_optfloat(L, 1, 0.0f);
	const float py = luaL_optfloat(L, 2, 0.0f);
	const float pz = luaL_optfloat(L, 3, 0.0f);
	const float tx = luaL_optfloat(L, 4, 0.0f);
	const float ty = luaL_optfloat(L, 5, 0.0f);
	const float tz = luaL_optfloat(L, 6, 0.0f);
	camera->posOffset = float3(px, py, pz);
	camera->tiltOffset = float3(tx, ty, tz);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetLosViewColors(lua_State* L)
{
	float red[4];
	float green[4];
	float blue[4];
	if ((LuaUtils::ParseFloatArray(L, 1, red,   4) != 4) ||
	    (LuaUtils::ParseFloatArray(L, 2, green, 4) != 4) ||
	    (LuaUtils::ParseFloatArray(L, 3, blue,  4) != 4)) {
		luaL_error(L, "Incorrect arguments to SetLosViewColors()");
	}
	const int scale = CBaseGroundDrawer::losColorScale;
	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	gd->alwaysColor[0] = (int)(scale *   red[0]);
	gd->alwaysColor[1] = (int)(scale * green[0]);
	gd->alwaysColor[2] = (int)(scale *  blue[0]);
	gd->losColor[0]    = (int)(scale *   red[1]);
	gd->losColor[1]    = (int)(scale * green[1]);
	gd->losColor[2]    = (int)(scale *  blue[1]);
	gd->radarColor[0]  = (int)(scale *   red[2]);
	gd->radarColor[1]  = (int)(scale * green[2]);
	gd->radarColor[2]  = (int)(scale *  blue[2]);
	gd->jamColor[0]    = (int)(scale *   red[3]);
	gd->jamColor[1]    = (int)(scale * green[3]);
	gd->jamColor[2]    = (int)(scale *  blue[3]);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

#define SET_IN_OVERLAY_WARNING \
	if (lua_isboolean(L, 3)) { \
		static bool shown = false; \
		if (!shown) { \
			LOG_L(L_WARNING, "%s: third parameter \"setInOverlay\" is deprecated", __FUNCTION__); \
			shown = true; \
		} \
	}

int LuaUnsyncedCtrl::GetConfigInt(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		// FIXME: why not put this in UnsyncedRead without the ModUICtrl restriction?
		lua_pushnumber(L, 0);
		return 1;
	}
	const string name = luaL_checkstring(L, 1);
	const int def     = luaL_optint(L, 2, 0);
	SET_IN_OVERLAY_WARNING;
	const int value = configHandler->IsSet(name) ? configHandler->GetInt(name) : def;
	lua_pushnumber(L, value);
	return 1;
}

int LuaUnsyncedCtrl::SetConfigInt(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string name = luaL_checkstring(L, 1);
	const int value   = luaL_checkint(L, 2);
	const bool useOverlay = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;
	// don't allow to change a read-only variable
	if (configHandler->IsReadOnly(name)) {
		LOG_L(L_ERROR, "tried to set readonly (int) %s = %d", name.c_str(), value);
		return 0;
	}
	configHandler->EnableWriting(globalConfig->luaWritableConfigFile);
	configHandler->Set(name, value, useOverlay);
	configHandler->EnableWriting(true);
	return 0;
}

int LuaUnsyncedCtrl::GetConfigString(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		// FIXME: why not put this in UnsyncedRead without the ModUICtrl restriction?
		lua_pushstring(L, "");
		return 1;
	}
	const string name = luaL_checkstring(L, 1);
	const string def  = luaL_optstring(L, 2, "");
	SET_IN_OVERLAY_WARNING;
	const string value = configHandler->IsSet(name) ? configHandler->GetString(name) : def;
	lua_pushsstring(L, value);
	return 1;
}


int LuaUnsyncedCtrl::SetConfigString(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string name  = luaL_checkstring(L, 1);
	const string value = luaL_checkstring(L, 2);
	const bool useOverlay = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;
	// don't allow to change a read-only variable
	if (configHandler->IsReadOnly(name)) {
		LOG_L(L_ERROR, "tried to set readonly (string) %s = %s", name.c_str(), value.c_str());
		return 0;
	}
	configHandler->EnableWriting(globalConfig->luaWritableConfigFile);
	configHandler->SetString(name, value, useOverlay);
	configHandler->EnableWriting(true);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::CreateDir(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string dir = luaL_checkstring(L, 1);

	// keep directories within the Spring directory
	if ((dir[0] == '/') || (dir[0] == '\\') ||
	    (strstr(dir.c_str(), "..") != NULL) ||
	    ((dir.size() > 0) && (dir[1] == ':'))) {
		luaL_error(L, "Invalid CreateDir() access: %s", dir.c_str());
	}
	const bool success = FileSystem::CreateDirectory(dir);
	lua_pushboolean(L, success);
	return 1;
}


/******************************************************************************/

int LuaUnsyncedCtrl::Restart(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to Restart(arguments, script)");
	}

	const std::string arguments = luaL_checkstring(L, 1);
	const std::string script = luaL_checkstring(L, 2);

	const std::string springFullName = (Platform::GetProcessExecutableFile());

	std::vector<std::string> processArgs;

	// Arguments given by Lua code, if any
	if (!arguments.empty()) {
		processArgs.push_back(arguments);
	}

	// script.txt, if content for it is given by Lua code
	const std::string scriptFullName = dataDirLocater.GetWriteDirPath() + "script.txt";
	if (!script.empty()) {
		std::ofstream scriptfile(scriptFullName.c_str());
		scriptfile << script;
		scriptfile.close();

		processArgs.push_back(scriptFullName);
	}

#ifdef _WIN32
		//! else OpenAL soft crashes when using execvp
		ISound::Shutdown();
#endif

	const std::string execError = Platform::ExecuteProcess(springFullName, processArgs);
	const bool execOk = execError.empty();

	if (execOk) {
		LOG("The game should restart");
	} else {
		LOG_L(L_ERROR, "Error in Restart: %s", execError.c_str());
	}

	lua_pushboolean(L, execOk);
	return 1;
}

int LuaUnsyncedCtrl::SetWMIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetWMIcon(iconFileName)");
	}

	const std::string iconFileName = luaL_checkstring(L, 1);

	CBitmap iconTexture;
	const bool loaded = iconTexture.Load(iconFileName);
	if (loaded) {
		WindowManagerHelper::SetIcon(&iconTexture);
	} else {
		luaL_error(L, "Failed to load image from file \"%s\"", iconFileName.c_str());
	}

	return 0;
}

int LuaUnsyncedCtrl::SetWMCaption(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)
			|| (args > 2) || ((args >= 2) && !lua_isstring(L, 2)))
	{
		luaL_error(L, "Incorrect arguments to SetWMCaption(title[, titleShort])");
	}

	const std::string title = luaL_checkstring(L, 1);

	std::string titleShort;
	if (args >= 2) {
		titleShort = luaL_checkstring(L, 2);
	} else {
		titleShort = title;
	}

	WindowManagerHelper::SetCaption(title, titleShort);

	return 0;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitDefIcon(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to SetUnitDefIcon(unitDefID, \"icon\")");
	}
	const int unitDefID = lua_toint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	ud->iconType = icon::iconHandler->GetIcon(lua_tostring(L, 2));

	// set decoys to the same icon
	std::map<int, std::set<int> >::const_iterator fit;

	if (ud->decoyDef) {
		ud->decoyDef->iconType = ud->iconType;
		fit = unitDefHandler->decoyMap.find(ud->decoyDef->id);
	} else {
		fit = unitDefHandler->decoyMap.find(ud->id);
	}
	if (fit != unitDefHandler->decoyMap.end()) {
		const std::set<int>& decoySet = fit->second;
		std::set<int>::const_iterator dit;
		for (dit = decoySet.begin(); dit != decoySet.end(); ++dit) {
  		const UnitDef* decoyDef = unitDefHandler->GetUnitDefByID(*dit);
			decoyDef->iconType = ud->iconType;
		}
	}

	return 0;
}


int LuaUnsyncedCtrl::SetUnitDefImage(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}

	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	if (lua_isnoneornil(L, 2)) {
		// reset to default texture
		unitDefHandler->SetUnitDefImage(ud, ud->buildPicName);
		return 0;
	}

	if (!lua_israwstring(L, 2)) {
		return 0;
	}
	const string texName = lua_tostring(L, 2);

	if (texName[0] == LuaTextures::prefix) { // '!'
		LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* tex = textures.GetInfo(texName);
		if (tex == NULL) {
			return 0;
		}
		unitDefHandler->SetUnitDefImage(ud, tex->id, tex->xsize, tex->ysize);
	} else {
		unitDefHandler->SetUnitDefImage(ud, texName);
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitGroup(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // SetUnitGroup

	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (gs->noHelperAIs) {
		return 0;
	}

	CUnit* unit = ParseRawUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L);
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitGroup()");
	}
	const int groupID = lua_toint(L, 2);

	if (groupID == -1) {
		unit->SetGroup(NULL);
		return 0;
	}

	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || (groupID >= (int)groups.size())) {
		return 0;
	}

	CGroup* group = groups[groupID];
	if (group != NULL) {
		unit->SetGroup(group);
	}
	return 0;
}


/******************************************************************************/

static void ParseUnitMap(lua_State* L, const char* caller,
                         int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit map", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, -2); // the key
			if (unit != NULL && !unit->noSelect) {
				unitIDs.push_back(unit->id);
			}
		}
	}
}


static void ParseUnitArray(lua_State* L, const char* caller,
                           int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit array", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {   // avoid 'n'
			CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, -1); // the value
			if (unit != NULL && !unit->noSelect) {
				unitIDs.push_back(unit->id);
			}
		}
	}
	return;
}


/******************************************************************************/

static bool CanGiveOrders(const lua_State *L)
{
	if (gs->frameNum <= 0) {
		return false;
	}
	if (gs->noHelperAIs) {
		return false;
	}
	if (gs->godMode) {
		return true;
	}
	if (gu->spectating) {
		return false;
	}
	const int ctrlTeam = CLuaHandle::GetHandleCtrlTeam(L);
	// FIXME ? (correct? warning / error?)
	if ((ctrlTeam != gu->myTeam) || (ctrlTeam < 0)) {
		return false;
	}
	return true;
}


int LuaUnsyncedCtrl::GiveOrder(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (!CanGiveOrders(L)) {
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 1);

	selectedUnitsHandler.GiveCommand(cmd);

	lua_pushboolean(L, true);

	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnit(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL || unit->noSelect) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 2);

	net->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, skirmishAIHandler.GetCurrentAIID(), unit->id, cmd.GetID(), cmd.aiCommandId, cmd.options, cmd.params));

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnitMap(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __FUNCTION__, 1, unitIDs);
	const int count = (int)unitIDs.size();

	if (count <= 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 2);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnitsHandler.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnitArray(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __FUNCTION__, 1, unitIDs);
	const int count = (int)unitIDs.size();

	if (count <= 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 2);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnitsHandler.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderArrayToUnitMap(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __FUNCTION__, 1, unitIDs);

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);

	if (unitIDs.empty() || commands.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	selectedUnitsHandler.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderArrayToUnitArray(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	const int args = lua_gettop(L); // number of arguments

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __FUNCTION__, 1, unitIDs);

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);

	bool pairwise = false;
	if (args >= 3)
		pairwise = lua_toboolean(L, 3);

	if (unitIDs.empty() || commands.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	selectedUnitsHandler.SendCommandsToUnits(unitIDs, commands, pairwise);

	lua_pushboolean(L, true);
	return 1;
}


/******************************************************************************/

static string GetRawMsg(lua_State* L, const char* caller, int index)
{
	if (!lua_israwstring(L, index)) {
		luaL_error(L, "Incorrect arguments to %s", caller);
	}
	size_t len;
	const char* str = lua_tolstring(L, index, &len);
	const string tmpMsg(str, len);
	return tmpMsg;
}


int LuaUnsyncedCtrl::SendLuaUIMsg(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string msg = GetRawMsg(L, __FUNCTION__, 1);
	std::vector<boost::uint8_t> data(msg.size());
	std::copy(msg.begin(), msg.end(), data.begin());
	const string mode = luaL_optstring(L, 2, "");
	unsigned char modeNum = 0;
	if ((mode == "s") || (mode == "specs")) {
		modeNum = 's';
	}
	else if ((mode == "a") || (mode == "allies")) {
		modeNum = 'a';
	}
	else if (!mode.empty()) {
		luaL_error(L, "Unknown SendLuaUIMsg() mode");
	}
	try {
		net->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_UI, modeNum, data));
	} catch (const netcode::PackPacketException& ex) {
		luaL_error(L, "SendLuaUIMsg() packet error: %s", ex.what());
	}
	return 0;
}


int LuaUnsyncedCtrl::SendLuaGaiaMsg(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string msg = GetRawMsg(L, __FUNCTION__, 1);
	std::vector<boost::uint8_t> data(msg.size());
	std::copy(msg.begin(), msg.end(), data.begin());
	try {
		net->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_GAIA, 0, data));
	} catch (const netcode::PackPacketException& ex) {
		luaL_error(L, "SendLuaGaiaMsg() packet error: %s", ex.what());
	}
	return 0;
}


int LuaUnsyncedCtrl::SendLuaRulesMsg(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	const string msg = GetRawMsg(L, __FUNCTION__, 1);
	std::vector<boost::uint8_t> data(msg.size());
	std::copy(msg.begin(), msg.end(), data.begin());
	try {
		net->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_RULES, 0, data));
	} catch (const netcode::PackPacketException& ex) {
		luaL_error(L, "SendLuaRulesMsg() packet error: %s", ex.what());
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetShareLevel(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetShareLevel(\"type\", level");
	}

	const string shareType = lua_tostring(L, 1);
	const float shareLevel = max(0.0f, min(1.0f, lua_tofloat(L, 2)));

	if (shareType == "metal") {
		net->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, shareLevel, teamHandler->Team(gu->myTeam)->energyShare));
	}
	else if (shareType == "energy") {
		net->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam,	teamHandler->Team(gu->myTeam)->metalShare, shareLevel));
	}
	else {
		LOG_L(L_WARNING, "SetShareLevel() unknown resource: %s", shareType.c_str());
	}
	return 0;
}


int LuaUnsyncedCtrl::ShareResources(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2) ||
	    ((args >= 3) && !lua_isnumber(L, 3))) {
		luaL_error(L, "Incorrect arguments to ShareResources()");
	}
	const int teamID = lua_toint(L, 1);
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if ((team == NULL) || team->isDead) {
		return 0;
	}
	const string& type = lua_tostring(L, 2);
	if (type == "units") {
		// update the selection, and clear the unit command queues
		Command c(CMD_STOP);
		selectedUnitsHandler.GiveCommand(c, false);
		net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 1, 0.0f, 0.0f));
		selectedUnitsHandler.ClearSelected();
	}
	else if (args >= 3) {
		const float amount = lua_tofloat(L, 3);
		if (type == "metal") {
			net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 0, amount, 0.0f));
		}
		else if (type == "energy") {
			net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 0, 0.0f, amount));
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/


int LuaUnsyncedCtrl::SetLastMessagePosition(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetLastMessagePosition(x, y, z)");
	}
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));

	eventHandler.LastMessagePosition(pos);

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::MarkerAddPoint(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (inMapDrawer == NULL) {
		return 0;
	}
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));
	const string text = luaL_optstring(L, 4, "");
	const bool onlyLocal = (lua_isnumber(L, 5)) ? bool(luaL_optnumber(L, 5, 1)) : luaL_optboolean(L, 5, true);

	if (onlyLocal) {
		inMapDrawerModel->AddPoint(pos, text, gu->myPlayerNum);
	} else {
		inMapDrawer->SendPoint(pos, text, true);
	}

	return 0;
}


int LuaUnsyncedCtrl::MarkerAddLine(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (inMapDrawer == NULL) {
		return 0;
	}
	const float3 pos1(luaL_checkfloat(L, 1),
	                  luaL_checkfloat(L, 2),
	                  luaL_checkfloat(L, 3));
	const float3 pos2(luaL_checkfloat(L, 4),
	                  luaL_checkfloat(L, 5),
	                  luaL_checkfloat(L, 6));
	const bool onlyLocal = (lua_isnumber(L, 7)) ? bool(luaL_optnumber(L, 7, 0)) : luaL_optboolean(L, 7, false);

	if (onlyLocal) {
		inMapDrawerModel->AddLine(pos1, pos2, gu->myPlayerNum);
	} else {
		inMapDrawer->SendLine(pos1, pos2, true);
	}

	return 0;
}


int LuaUnsyncedCtrl::MarkerErasePosition(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}
	if (inMapDrawer == NULL) {
		return 0;
	}
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	inMapDrawer->SendErase(pos);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetDrawSelectionInfo(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawSelectionInfo(bool)");
	}

	if (guihandler)
		guihandler->SetDrawSelectionInfo(lua_toboolean(L, 1));

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetBuildSpacing(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetBuildSpacing(int)");
	}

	if (guihandler)
		guihandler->SetBuildSpacing(lua_tointeger(L, 1));

	return 0;
}

int LuaUnsyncedCtrl::SetBuildFacing(lua_State* L)
{
	if (!CLuaHandle::CheckModUICtrl(L)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetBuildFacing(int)");
	}

	if (guihandler)
		guihandler->SetBuildFacing(lua_tointeger(L, 1));

	return 0;
}
/******************************************************************************/
/******************************************************************************/



int LuaUnsyncedCtrl::SetSunParameters(lua_State* L)
{
	DynamicSkyLight* dynSkyLight = dynamic_cast<DynamicSkyLight*>(sky->GetLight());

	if (dynSkyLight == NULL)
		return 0;

	const int args = lua_gettop(L); // number of arguments
	if (args != 6 ||
		!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
		!lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		luaL_error(L, "Incorrect arguments to SetSunParameters(float, float, float, float, float, float)");
	}

	const float4 sunDir(lua_tofloat(L, 1), lua_tofloat(L, 2), lua_tofloat(L, 3), lua_tofloat(L, 4));
	const float startAngle = lua_tofloat(L, 5);
	const float orbitTime = lua_tofloat(L, 6);

	dynSkyLight->SetLightParams(sunDir, startAngle, orbitTime);
	return 0;
}

int LuaUnsyncedCtrl::SetSunManualControl(lua_State* L)
{
	DynamicSkyLight* dynSkyLight = dynamic_cast<DynamicSkyLight*>(sky->GetLight());

	if (dynSkyLight == NULL)
		return 0;

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetSunManualControl(bool)");
	}

	dynSkyLight->SetLuaControl(lua_toboolean(L, 1));
	return 0;
}

int LuaUnsyncedCtrl::SetSunDirection(lua_State* L)
{
	DynamicSkyLight* dynSkyLight = dynamic_cast<DynamicSkyLight*>(sky->GetLight());

	if (dynSkyLight == NULL)
		return 0;
	if (!dynSkyLight->GetLuaControl())
		return 0;

	const int args = lua_gettop(L); // number of arguments
	if (args != 3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetSunDirection(float, float, float)");
	}

	dynSkyLight->SetLightDir(float3(lua_tofloat(L, 1), lua_tofloat(L, 2), lua_tofloat(L, 3)));
	return 0;
}



/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SendSkirmishAIMessage(lua_State* L) {
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}
	if (lua_gettop(L) != 2) {
		return 0;
	}

	const int aiTeam = luaL_checkint(L, 1);
	const char* inData = luaL_checkstring(L, 2);

	std::vector<const char*> outData;

	lua_checkstack(L, 2);
	lua_pushboolean(L, eoh->SendLuaMessages(aiTeam, inData, outData));
	lua_createtable(L, outData.size(), 0);

	// push the AI response(s)
	for (unsigned int n = 0; n < outData.size(); n++) {
		lua_pushnumber(L, n + 1);
		lua_pushstring(L, outData[n]);
		lua_rawset(L, -3);
	}

	return 2;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::ClearWatchDogTimer(lua_State* L) {
	const int args = lua_gettop(L); // number of arguments

	if (args == 0) {
		Watchdog::ClearTimer();
	} else {
		std::string threadname = "main";
		if (lua_isstring(L, 1))
			threadname = lua_tostring(L, 1);
		Watchdog::ClearTimer(threadname);
	}

	return 0;
}
