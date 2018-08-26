/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaUnsyncedCtrl.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaMenu.h"
#include "LuaOpenGLUtils.h"
#include "LuaParser.h"
#include "LuaTextures.h"
#include "LuaUtils.h"

#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/IVideoCapturing.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
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
#include "Game/UI/Groups/Group.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/BaseGroundTextures.h"
#include "Net/Protocol/NetProtocol.h"
#include "Net/GameServer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/DecalsDrawerGL4.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/ILog.h"
#include "System/Net/PackPacket.h"
#include "System/Platform/Misc.h"
#include "System/SafeUtil.h"
#include "System/UnorderedMap.hpp"
#include "System/StringUtil.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Sync/HsiehHash.h"


#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <cctype>
#include <cfloat>
#include <cinttypes>

#include <fstream>

#include <SDL_keyboard.h>
#include <SDL_clipboard.h>
#include <SDL_mouse.h>

using std::min;
using std::max;

// MinGW defines this for a WINAPI function
#undef SendMessage
#undef CreateDirectory

const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions


/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedCtrl::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(Ping);
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
	REGISTER_LUA_CFUNC(SetDrawGroundDeferred);
	REGISTER_LUA_CFUNC(SetDrawModelsDeferred);
	REGISTER_LUA_CFUNC(SetVideoCapturingMode);
	REGISTER_LUA_CFUNC(SetVideoCapturingTimeOffset);

	REGISTER_LUA_CFUNC(SetWaterParams);

	REGISTER_LUA_CFUNC(AddMapLight);
	REGISTER_LUA_CFUNC(AddModelLight);
	REGISTER_LUA_CFUNC(UpdateMapLight);
	REGISTER_LUA_CFUNC(UpdateModelLight);
	REGISTER_LUA_CFUNC(SetMapLightTrackingState);
	REGISTER_LUA_CFUNC(SetModelLightTrackingState);
	REGISTER_LUA_CFUNC(SetMapShader);
	REGISTER_LUA_CFUNC(SetMapSquareTexture);
	REGISTER_LUA_CFUNC(SetMapShadingTexture);
	REGISTER_LUA_CFUNC(SetSkyBoxTexture);

	REGISTER_LUA_CFUNC(SetUnitNoDraw);
	REGISTER_LUA_CFUNC(SetUnitNoMinimap);
	REGISTER_LUA_CFUNC(SetUnitNoSelect);
	REGISTER_LUA_CFUNC(SetUnitLeaveTracks);
	REGISTER_LUA_CFUNC(SetUnitSelectionVolumeData);
	REGISTER_LUA_CFUNC(SetFeatureNoDraw);
	REGISTER_LUA_CFUNC(SetFeatureFade);
	REGISTER_LUA_CFUNC(SetFeatureSelectionVolumeData);

	REGISTER_LUA_CFUNC(AddUnitIcon);
	REGISTER_LUA_CFUNC(FreeUnitIcon);

	REGISTER_LUA_CFUNC(ExtractModArchiveFile);

	// moved from LuaUI

//FIXME	REGISTER_LUA_CFUNC(SetShockFrontFactors);

	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigFloat);
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
	REGISTER_LUA_CFUNC(SendLuaMenuMsg);

	REGISTER_LUA_CFUNC(LoadCmdColorsConfig);
	REGISTER_LUA_CFUNC(LoadCtrlPanelConfig);

	REGISTER_LUA_CFUNC(SetActiveCommand);
	REGISTER_LUA_CFUNC(ForceLayoutUpdate);

	REGISTER_LUA_CFUNC(SetMouseCursor);
	REGISTER_LUA_CFUNC(WarpMouse);

	REGISTER_LUA_CFUNC(SetClipboard);

	REGISTER_LUA_CFUNC(SetCameraOffset);

	REGISTER_LUA_CFUNC(SetLosViewColors);

	REGISTER_LUA_CFUNC(Reload);
	REGISTER_LUA_CFUNC(Restart);
	REGISTER_LUA_CFUNC(Start);
	REGISTER_LUA_CFUNC(Quit);

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

	REGISTER_LUA_CFUNC(SetAtmosphere);
	REGISTER_LUA_CFUNC(SetSunLighting);
	REGISTER_LUA_CFUNC(SetSunDirection);
	REGISTER_LUA_CFUNC(SetMapRenderingParams);

	REGISTER_LUA_CFUNC(SendSkirmishAIMessage);

	REGISTER_LUA_CFUNC(SetLogSectionFilterLevel);

	REGISTER_LUA_CFUNC(ClearWatchDogTimer);

	REGISTER_LUA_CFUNC(PreloadUnitDefModel);
	REGISTER_LUA_CFUNC(PreloadFeatureDefModel);

	REGISTER_LUA_CFUNC(CreateDecal);
	REGISTER_LUA_CFUNC(DestroyDecal);
	REGISTER_LUA_CFUNC(SetDecalPos);
	REGISTER_LUA_CFUNC(SetDecalSize);
	REGISTER_LUA_CFUNC(SetDecalRotation);
	REGISTER_LUA_CFUNC(SetDecalTexture);
	REGISTER_LUA_CFUNC(SetDecalAlpha);

	REGISTER_LUA_CFUNC(SDLSetTextInputRect);
	REGISTER_LUA_CFUNC(SDLStartTextInput);
	REGISTER_LUA_CFUNC(SDLStopTextInput);

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
		luaL_error(L, "[%s] projectile ID parameter in %s() not a number\n", __func__, caller);
		return nullptr;
	}

	CProjectile* p = nullptr;

	if (synced) {
		p = projectileHandler.GetProjectileBySyncedID(lua_toint(L, index));
	} else {
		p = projectileHandler.GetProjectileByUnsyncedID(lua_toint(L, index));
	}

	return p;
}


static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] ID parameter in %s() not a number\n", __func__, caller);
		return nullptr;
	}

	return (unitHandler.GetUnit(lua_toint(L, index)));
}

static inline CFeature* ParseRawFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] ID parameter in %s() not a number\n", __func__, caller);
		return nullptr;
	}

	return (featureHandler.GetFeature(luaL_checkint(L, index)));
}


static inline CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return (CLuaHandle::GetHandleFullRead(L)? unit: nullptr);

	if (unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L))
		return unit;

	return nullptr;
}


static inline CUnit* ParseCtrlUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (CanControlTeam(L, unit->team))
		return unit;

	return nullptr;
}

static inline CFeature* ParseCtrlFeature(lua_State* L, const char* caller, int index)
{
	CFeature* feature = ParseRawFeature(L, caller, index);

	if (feature == nullptr)
		return nullptr;

	if (CanControlTeam(L, feature->team))
		return feature;

	return nullptr;
}


static inline CUnit* ParseSelectUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr || unit->noSelect)
		return nullptr;

	if (gs->godMode)
		return unit;

	const int selectTeam = CLuaHandle::GetHandleSelectTeam(L);

	if (selectTeam < 0)
		return ((selectTeam == CEventClient::AllAccessTeam)? unit: nullptr);

	if (selectTeam == unit->team)
		return unit;

	return nullptr;
}



/******************************************************************************/
/******************************************************************************/
//
//  The call-outs
//

int LuaUnsyncedCtrl::Ping(lua_State* L)
{
	// pre-game ping would not be handled properly, send via GUI
	if (guihandler == nullptr)
		return 0;

	guihandler->RunCustomCommands({"@@netping " + IntToString(luaL_optint(L, 1, 0), "%u")}, false);
	return 0;
}

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
	if (start == string::npos)
		return msg;

	const char* number = msg.c_str() + start + strlen("<PLAYER");
	char* endPtr;
	const int playerID = (int)strtol(number, &endPtr, 10);

	if ((endPtr == number) || (*endPtr != '>'))
		luaL_error(L, "Bad message format: %s", msg.c_str());

	if (!playerHandler.IsValidPlayer(playerID))
		luaL_error(L, "Invalid message playerID: %c", playerID); //FIXME

	const CPlayer* player = playerHandler.Player(playerID);
	if ((player == nullptr) || !player->active || player->name.empty())
		luaL_error(L, "Invalid message playerID: %c", playerID);

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
	PrintMessage(L, luaL_checksstring(L, 1));
	return 0;
}

int LuaUnsyncedCtrl::SendMessageToSpectators(lua_State* L)
{
	if (gu->spectating)
		PrintMessage(L, luaL_checksstring(L, 1));

	return 0;
}

int LuaUnsyncedCtrl::SendMessageToPlayer(lua_State* L)
{
	if (luaL_checkint(L, 1) == gu->myPlayerNum)
		PrintMessage(L, luaL_checksstring(L, 2));

	return 0;
}

int LuaUnsyncedCtrl::SendMessageToTeam(lua_State* L)
{
	if (luaL_checkint(L, 1) == gu->myTeam)
		PrintMessage(L, luaL_checksstring(L, 2));

	return 0;
}

int LuaUnsyncedCtrl::SendMessageToAllyTeam(lua_State* L)
{
	if (luaL_checkint(L, 1) == gu->myAllyTeam)
		PrintMessage(L, luaL_checksstring(L, 2));

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::LoadSoundDef(lua_State* L)
{
	LuaParser soundDefsParser(luaL_checksstring(L, 1), SPRING_VFS_ZIP_FIRST, SPRING_VFS_ZIP_FIRST);

	const bool success = sound->LoadSoundDefs(&soundDefsParser);

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, success);
		return 1;
	}

	return 0;
}

int LuaUnsyncedCtrl::PlaySoundFile(lua_State* L)
{
	const int args = lua_gettop(L);

	const unsigned int soundID = sound->GetSoundId(luaL_checksstring(L, 1));

	if (soundID > 0) {
		float volume = luaL_optfloat(L, 2, 1.0f);

		float3 pos;
		float3 speed;

		int index = 3;

		if (args >= 5 && lua_isnumber(L, 3) && lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
			pos = float3(lua_tofloat(L, 3), lua_tofloat(L, 4), lua_tofloat(L, 5));
			index += 3;

			if (args >= 8 && lua_isnumber(L, 6) && lua_isnumber(L, 7) && lua_isnumber(L, 8)) {
				speed = float3(lua_tofloat(L, 6), lua_tofloat(L, 7), lua_tofloat(L, 8));
				index += 3;
			}
		}

		// last argument (with and without pos/speed arguments) is the optional `sfx channel`
		IAudioChannel** channel = &Channels::General;

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

		if (index >= 6) {
			if (index >= 9) {
				channel[0]->PlaySample(soundID, pos, speed, volume);
			} else {
				channel[0]->PlaySample(soundID, pos, volume);
			}
		} else
			channel[0]->PlaySample(soundID, volume);
	}

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, soundID > 0);
		return 1;
	}

	return 0;
}


int LuaUnsyncedCtrl::PlaySoundStream(lua_State* L)
{
	// file, volume, enqueue
	Channels::BGMusic->StreamPlay(luaL_checksstring(L, 1), luaL_optnumber(L, 2, 1.0f), luaL_optboolean(L, 3, false));

	// .ogg files don't have sound ID's generated
	// for them (yet), so we always succeed here
	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, true);
		return 1;
	}

	return 0;
}

int LuaUnsyncedCtrl::StopSoundStream(lua_State*)
{
	Channels::BGMusic->StreamStop();
	return 0;
}
int LuaUnsyncedCtrl::PauseSoundStream(lua_State*)
{
	Channels::BGMusic->StreamPause();
	return 0;
}
int LuaUnsyncedCtrl::SetSoundStreamVolume(lua_State* L)
{
	Channels::BGMusic->SetVolume(luaL_checkfloat(L, 1));
	return 0;
}


int LuaUnsyncedCtrl::SetSoundEffectParams(lua_State* L)
{
#if !defined(HEADLESS) && !defined(NO_SOUND)
	if (!efx.Supported())
		return 0;

	//! only a preset name given?
	if (lua_israwstring(L, 1)) {
		const std::string presetname = lua_tostring(L, 1);
		efx.SetPreset(presetname, false);
		return 0;
	}

	if (!lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to SetSoundEffectParams()");

	//! first parse the 'preset' key (so all following params use it as base and override it)
	lua_pushliteral(L, "preset");
	lua_gettable(L, -2);
	if (lua_israwstring(L, -1)) {
		std::string presetname = lua_tostring(L, -1);
		efx.SetPreset(presetname, false, false);
	}
	lua_pop(L, 1);


	EAXSfxProps& efxprops = efx.sfxProperties;


	//! parse pass filter
	lua_pushliteral(L, "passfilter");
	lua_gettable(L, -2);
	if (lua_istable(L, -1)) {
		for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2))
				continue;

			const string key = StringToLower(lua_tostring(L, -2));
			const auto it = nameToALFilterParam.find(key);

			if (it == nameToALFilterParam.end())
				continue;

			ALuint param = it->second;

			if (!lua_isnumber(L, -1))
				continue;
			if (alParamType[param] != EFXParamTypes::FLOAT)
				continue;

			efxprops.filter_props_f[param] = lua_tofloat(L, -1);
		}
	}
	lua_pop(L, 1);

	//! parse EAX Reverb
	lua_pushliteral(L, "reverb");
	lua_gettable(L, -2);
	if (lua_istable(L, -1)) {
		for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2))
				continue;

			const string key = StringToLower(lua_tostring(L, -2));
			const auto it = nameToALParam.find(key);

			if (it == nameToALParam.end())
				continue;

			const ALuint param = it->second;

			if (lua_istable(L, -1)) {
				if (alParamType[param] == EFXParamTypes::VECTOR) {
					float3 v;

					if (LuaUtils::ParseFloatArray(L, -1, &v[0], 3) >= 3) {
						efxprops.reverb_props_v[param] = v;
					}
				}
			}
			else if (lua_isnumber(L, -1)) {
				if (alParamType[param] == EFXParamTypes::FLOAT) {
					efxprops.reverb_props_f[param] = lua_tofloat(L, -1);
				}
			}
			else if (lua_isboolean(L, -1)) {
				if (alParamType[param] == EFXParamTypes::BOOL) {
					efxprops.reverb_props_i[param] = lua_toboolean(L, -1);
				}
			}
		}
	}
	lua_pop(L, 1);

	efx.CommitEffects();
#endif /// !defined(HEADLESS) && !defined(NO_SOUND)

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::AddWorldIcon(lua_State* L)
{
	const int cmdID = luaL_checkint(L, 1);
	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	cursorIcons.AddIcon(cmdID, pos);
	return 0;
}


int LuaUnsyncedCtrl::AddWorldText(lua_State* L)
{
	const string text = luaL_checksstring(L, 1);
	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	cursorIcons.AddIconText(text, pos);
	return 0;
}


int LuaUnsyncedCtrl::AddWorldUnit(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);

	if (!unitDefHandler->IsValidUnitDefID(unitDefID))
		return 0;

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	const int teamId = luaL_checkint(L, 5);

	if (!teamHandler.IsValidTeam(teamId))
		return 0;

	const int facing = luaL_checkint(L, 6);
	cursorIcons.AddBuildIcon(-unitDefID, pos, teamId, facing);
	return 0;
}


int LuaUnsyncedCtrl::DrawUnitCommands(lua_State* L)
{
	if (lua_istable(L, 1)) {
		const bool isMap = luaL_optboolean(L, 2, false);
		const int unitArg = isMap ? -2 : -1;
		const int table = 1;

		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwnumber(L, -2))
				continue;

			const CUnit* unit = ParseAllyUnit(L, __func__, unitArg);

			if (unit == nullptr)
				continue;

			commandDrawer->AddLuaQueuedUnit(unit);
		}
	} else {
		const CUnit* unit = ParseAllyUnit(L, __func__, 1);

		if (unit != nullptr)
			commandDrawer->AddLuaQueuedUnit(unit);
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/

static CCameraController::StateMap ParseCamStateMap(lua_State* L, int tableIdx)
{
	CCameraController::StateMap camState;

	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string key = lua_tostring(L, -2);

		if (lua_isnumber(L, -1)) {
			camState[key] = lua_tofloat(L, -1);
		}
		else if (lua_isboolean(L, -1)) {
			camState[key] = lua_toboolean(L, -1) ? +1.0f : -1.0f;
		}
	}

	return camState;
}


int LuaUnsyncedCtrl::SetCameraTarget(lua_State* L)
{
	if (mouse == nullptr)
		return 0;

	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	camHandler->CameraTransition(luaL_optfloat(L, 4, 0.5f));
	camHandler->GetCurrentController().SetPos(pos);
	return 0;
}

int LuaUnsyncedCtrl::SetCameraState(lua_State* L)
{
	// ??
	if (mouse == nullptr)
		return 0;

	if (!lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to SetCameraState(table[, camTime])");

	camHandler->CameraTransition(luaL_optfloat(L, 2, 0.0f));

	const bool retval = camHandler->SetState(ParseCamStateMap(L, 1));
	const bool synced = CLuaHandle::GetHandleSynced(L);

	// always push false in synced
	lua_pushboolean(L, retval && !synced);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SelectUnitArray(lua_State* L)
{
	if (!lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to SelectUnitArray()");

	// clear the current units, unless the append flag is present
	if (!luaL_optboolean(L, 2, false))
		selectedUnitsHandler.ClearSelected();

	constexpr int tableIdx = 1;
	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {     // avoid 'n'
			CUnit* unit = ParseSelectUnit(L, __func__, -1); // the value

			if (unit != nullptr)
				selectedUnitsHandler.AddUnit(unit);
		}
	}

	return 0;
}


int LuaUnsyncedCtrl::SelectUnitMap(lua_State* L)
{
	if (!lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to SelectUnitMap()");

	// clear the current units, unless the append flag is present
	if (!luaL_optboolean(L, 2, false))
		selectedUnitsHandler.ClearSelected();

	constexpr int tableIdx = 1;
	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseSelectUnit(L, __func__, -2); // the key

			if (unit != nullptr)
				selectedUnitsHandler.AddUnit(unit);
		}
	}

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetTeamColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	team->color[0] = (unsigned char)(Clamp(luaL_checkfloat(L, 2      ), 0.0f, 1.0f) * 255.0f);
	team->color[1] = (unsigned char)(Clamp(luaL_checkfloat(L, 3      ), 0.0f, 1.0f) * 255.0f);
	team->color[2] = (unsigned char)(Clamp(luaL_checkfloat(L, 4      ), 0.0f, 1.0f) * 255.0f);
	team->color[3] = (unsigned char)(Clamp(luaL_optfloat  (L, 5, 1.0f), 0.0f, 1.0f) * 255.0f);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::AssignMouseCursor(lua_State* L)
{
	const std::string& cmdName  = luaL_checksstring(L, 1);
	const std::string& fileName = luaL_checksstring(L, 2);

	const CMouseCursor::HotSpot hotSpot = (luaL_optboolean(L, 4, false))? CMouseCursor::TopLeft: CMouseCursor::Center;

	const bool overwrite = luaL_optboolean(L, 3, true);
	const bool assigned = mouse->AssignMouseCursor(cmdName, fileName, hotSpot, overwrite);

	if (!CLuaHandle::GetHandleSynced(L)) {
		lua_pushboolean(L, assigned);
		return 1;
	}

	return 0;
}


int LuaUnsyncedCtrl::ReplaceMouseCursor(lua_State* L)
{
	const string oldName = luaL_checksstring(L, 1);
	const string newName = luaL_checksstring(L, 2);

	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;
	if (luaL_optboolean(L, 3, false)) {
		hotSpot = CMouseCursor::TopLeft;
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
	else if (lua_isnoneornil(L, 2)) {
		cursorIcons.SetCustomType(cmdID, "");
		cmdColors.ClearCustomCmdData(cmdID);
		return 0;
	}
	else {
		luaL_error(L, "Incorrect arguments to SetCustomCommandDrawData");
	}

	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	/*const int size =*/ LuaUtils::ParseFloatArray(L, 3, color, 4);

	const bool showArea = luaL_optboolean(L, 4, false);

	cmdColors.SetCustomCmdData(cmdID, iconID, color, showArea);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetDrawSky(lua_State* L)
{
	globalRendering->drawSky = !!luaL_checkboolean(L, 1);
	return 0;
}

int LuaUnsyncedCtrl::SetDrawWater(lua_State* L)
{
	globalRendering->drawWater = !!luaL_checkboolean(L, 1);
	return 0;
}

int LuaUnsyncedCtrl::SetDrawGround(lua_State* L)
{
	globalRendering->drawGround = !!luaL_checkboolean(L, 1);
	return 0;
}


int LuaUnsyncedCtrl::SetDrawGroundDeferred(lua_State* L)
{
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	gd->SetDrawDeferredPass(luaL_checkboolean(L, 1));
	gd->SetDrawForwardPass(luaL_optboolean(L, 2, gd->DrawForward()));

	lua_pushboolean(L, gd->DrawDeferred());
	lua_pushboolean(L, gd->DrawForward());
	return 2;
}

int LuaUnsyncedCtrl::SetDrawModelsDeferred(lua_State* L)
{
	// NOTE the argument ordering
	unitDrawer->SetDrawDeferredPass(luaL_checkboolean(L, 1));
	unitDrawer->SetDrawForwardPass(luaL_optboolean(L, 3, unitDrawer->DrawForward()));

	featureDrawer->SetDrawDeferredPass(luaL_checkboolean(L, 2));
	featureDrawer->SetDrawForwardPass(luaL_optboolean(L, 4, featureDrawer->DrawForward()));

	lua_pushboolean(L,    unitDrawer->DrawDeferred());
	lua_pushboolean(L, featureDrawer->DrawDeferred());
	lua_pushboolean(L,    unitDrawer->DrawForward());
	lua_pushboolean(L, featureDrawer->DrawForward());
	return 4;
}


int LuaUnsyncedCtrl::SetVideoCapturingMode(lua_State* L)
{
	videoCapturing->SetAllowRecord(luaL_checkboolean(L, 1));
	return 0;
}

int LuaUnsyncedCtrl::SetVideoCapturingTimeOffset(lua_State* L)
{
	videoCapturing->SetTimeOffset(luaL_checkfloat(L, 1));
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetWaterParams(lua_State* L)
{
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetWaterParams()");
	}

	for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string key = lua_tostring(L, -2);

		if (lua_istable(L, -1)) {
			float color[3];
			const int size = LuaUtils::ParseFloatArray(L, -1, color, 3);
			if (size >= 3) {
				if (key == "absorb") {
					waterRendering->absorb = color;
				} else if (key == "baseColor") {
					waterRendering->baseColor = color;
				} else if (key == "minColor") {
					waterRendering->minColor = color;
				} else if (key == "surfaceColor") {
					waterRendering->surfaceColor = color;
				} else if (key == "diffuseColor") {
					waterRendering->diffuseColor = color;
				} else if (key == "specularColor") {
					waterRendering->specularColor = color;
					} else if (key == "planeColor") {
					waterRendering->planeColor.x = color[0];
					waterRendering->planeColor.y = color[1];
					waterRendering->planeColor.z = color[2];
				} else {
					luaL_error(L, "Unknown array key %s", key.c_str());
				}
			} else {
				luaL_error(L, "Unexpected size of %d for array key %s", size,  key.c_str());
			}
		}

		else if (lua_israwstring(L, -1)) {
			const std::string value = lua_tostring(L, -1);
			if (key == "texture") {
				waterRendering->texture = value;
			} else if (key == "foamTexture") {
				waterRendering->foamTexture = value;
			} else if (key == "normalTexture") {
				waterRendering->normalTexture = value;
			} else {
				luaL_error(L, "Unknown string key %s", key.c_str());
			}
		}

		else if (lua_isnumber(L, -1)) {
			const float value = lua_tofloat(L, -1);
			if (key == "repeatX") {
				waterRendering->repeatX = value;
			} else if (key == "repeatY") {
				waterRendering->repeatY = value;
			} else if (key == "surfaceAlpha") {
				waterRendering->surfaceAlpha = value;
			} else if (key == "ambientFactor") {
				waterRendering->ambientFactor = value;
			} else if (key == "diffuseFactor") {
				waterRendering->diffuseFactor = value;
			} else if (key == "specularFactor") {
				waterRendering->specularFactor = value;
			} else if (key == "specularPower") {
				waterRendering->specularPower = value;
			} else if (key == "fresnelMin") {
				waterRendering->fresnelMin = value;
			} else if (key == "fresnelMax") {
				waterRendering->fresnelMax = value;
			} else if (key == "fresnelPower") {
				waterRendering->fresnelPower = value;
			} else if (key == "reflectionDistortion") {
				waterRendering->reflDistortion = value;
			} else if (key == "blurBase") {
				waterRendering->blurBase = value;
			} else if (key == "blurExponent") {
				waterRendering->blurExponent = value;
			} else if (key == "perlinStartFreq") {
				waterRendering->perlinStartFreq = value;
			} else if (key == "perlinLacunarity") {
				waterRendering->perlinLacunarity = value;
			} else if (key == "perlinAmplitude") {
				waterRendering->perlinAmplitude = value;
			} else if (key == "numTiles") {
				waterRendering->numTiles = (unsigned char)value;
			} else {
				luaL_error(L, "Unknown scalar key %s", key.c_str());
			}
		}

		else if (lua_isboolean(L, -1)) {
			const bool value = lua_toboolean(L, -1);
			if (key == "shoreWaves") {
				waterRendering->shoreWaves = value;
			} else if (key == "forceRendering") {
				waterRendering->forceRendering = value;
			} else if (key == "hasWaterPlane") {
				waterRendering->hasWaterPlane = value;
			} else {
				luaL_error(L, "Unknown boolean key %s", key.c_str());
			}
		}
	}

	return 0;
}



static bool ParseLight(lua_State* L, GL::Light& light, const int tblIdx, const char* caller)
{
	if (!lua_istable(L, tblIdx)) {
		luaL_error(L, "[%s] argument %c must be a table!", caller, tblIdx);
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
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L))
		return 0;

	GL::LightHandler* lightHandler = readMap->GetGroundDrawer()->GetLightHandler();
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
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L))
		return 0;

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

	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L))
		return 0;

	GL::LightHandler* lightHandler = readMap->GetGroundDrawer()->GetLightHandler();
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

	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L))
		return 0;

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
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L))
		return 0;

	if (!lua_isnumber(L, 2)) {
		luaL_error(L, "[%s] 1st and 2nd arguments should be numbers, 3rd and 4th should be booleans", __FUNCTION__);
		return 0;
	}

	const unsigned int lightHandle = luaL_checkint(L, 1);
	const bool trackEnable = luaL_optboolean(L, 3, true);
	const bool trackUnit = luaL_optboolean(L, 4, true);

	GL::LightHandler* lightHandler = readMap->GetGroundDrawer()->GetLightHandler();
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
	if (CLuaHandle::GetHandleSynced(L) || !CLuaHandle::GetHandleFullRead(L))
		return 0;

	if (!lua_isnumber(L, 2)) {
		luaL_error(L, "[%s] 1st and 2nd arguments should be numbers, 3rd and 4th should be booleans", __FUNCTION__);
		return 0;
	}

	const unsigned int lightHandle = luaL_checkint(L, 1);
	const bool trackEnable = luaL_optboolean(L, 3, true);
	const bool trackUnit = luaL_optboolean(L, 4, true);

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

int LuaUnsyncedCtrl::SetMapShader(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	// SMF_RENDER_STATE_LUA only accepts GLSL shaders
	if (!globalRendering->haveGLSL)
		return 0;

	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	CBaseGroundDrawer* groundDrawer = readMap->GetGroundDrawer();
	LuaMapShaderData luaMapShaderData;

	for (unsigned int i = 0; i < 2; i++) {
		luaMapShaderData.shaderIDs[i] = shaders.GetProgramName(lua_tonumber(L, i + 1));
	}

	luaMapShaderData.setDefaultUniforms = luaL_optboolean(L, 3, false);

	groundDrawer->SetLuaShader(&luaMapShaderData);
	return 0;
}

int LuaUnsyncedCtrl::SetMapSquareTexture(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	const int texSquareX = luaL_checkint(L, 1);
	const int texSquareY = luaL_checkint(L, 2);
	const std::string& texName = luaL_checkstring(L, 3);

	CBaseGroundDrawer* groundDrawer = readMap->GetGroundDrawer();
	CBaseGroundTextures* groundTextures = groundDrawer->GetGroundTextures();

	if (groundTextures == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (texName.empty()) {
		// restore default texture for this square
		lua_pushboolean(L, groundTextures->SetSquareLuaTexture(texSquareX, texSquareY, 0));
		return 1;
	}

	const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(L);

	const    LuaTextures::Texture*   luaTexture = nullptr;
	const CNamedTextures::TexInfo* namedTexture = nullptr;

	if ((luaTexture = luaTextures.GetInfo(texName)) != nullptr) {
		if (luaTexture->xsize != luaTexture->ysize) {
			// square textures only
			lua_pushboolean(L, false);
			return 1;
		}

		lua_pushboolean(L, groundTextures->SetSquareLuaTexture(texSquareX, texSquareY, luaTexture->id));
		return 1;
	}

	if ((namedTexture = CNamedTextures::GetInfo(texName)) != nullptr) {
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


static MapTextureData ParseLuaTextureData(lua_State* L, bool mapTex)
{
	MapTextureData luaTexData;

	const std::string& texType = mapTex? luaL_checkstring(L, 1): "";
	const std::string& texName = mapTex? luaL_checkstring(L, 2): luaL_checkstring(L, 1);

	if (mapTex) {
		// convert type=LUATEX_* to MAP_*
		luaTexData.type = LuaOpenGLUtils::GetLuaMatTextureType(texType) - LuaMatTexture::LUATEX_SMF_GRASS;
		// MAP_SSMF_SPLAT_NORMAL_TEX needs a num
		luaTexData.num = luaL_optint(L, 3, 0);
	}

	// empty name causes a revert to default
	if (!texName.empty()) {
		const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(L);

		const    LuaTextures::Texture*   luaTexture = nullptr;
		const CNamedTextures::TexInfo* namedTexture = nullptr;

		if ((luaTexData.id == 0) && ((luaTexture = luaTextures.GetInfo(texName)) != nullptr)) {
			luaTexData.id     = luaTexture->id;
			luaTexData.size.x = luaTexture->xsize;
			luaTexData.size.y = luaTexture->ysize;
		}
		if ((luaTexData.id == 0) && ((namedTexture = CNamedTextures::GetInfo(texName)) != nullptr)) {
			luaTexData.id     = namedTexture->id;
			luaTexData.size.x = namedTexture->xsize;
			luaTexData.size.y = namedTexture->ysize;
		}
	}

	return luaTexData;
}


int LuaUnsyncedCtrl::SetMapShadingTexture(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	if (readMap != nullptr) {
		lua_pushboolean(L, readMap->SetLuaTexture(ParseLuaTextureData(L, true)));
	} else {
		lua_pushboolean(L, false);
	}

	return 1;
}

int LuaUnsyncedCtrl::SetSkyBoxTexture(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	if (sky != nullptr) {
		sky->SetLuaTexture(ParseLuaTextureData(L, false));
	}

	return 0;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitNoDraw(lua_State* L)
{
	if (CLuaHandle::GetHandleUserMode(L))
		return 0;

	CUnit* unit = ParseCtrlUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->noDraw = luaL_checkboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitNoMinimap(lua_State* L)
{
	if (CLuaHandle::GetHandleUserMode(L))
		return 0;

	CUnit* unit = ParseCtrlUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->noMinimap = luaL_checkboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitNoSelect(lua_State* L)
{
	if (CLuaHandle::GetHandleUserMode(L))
		return 0;

	CUnit* unit = ParseCtrlUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->noSelect = luaL_checkboolean(L, 2);

	// deselect the unit if it's selected and shouldn't be
	if (unit->noSelect) {
		const auto& selUnits = selectedUnitsHandler.selectedUnits;

		if (selUnits.find(unit->id) != selUnits.end()) {
			selectedUnitsHandler.RemoveUnit(unit);
		}
	}
	return 0;
}


int LuaUnsyncedCtrl::SetUnitLeaveTracks(lua_State* L)
{
	CUnit* unit = ParseCtrlUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->leaveTracks = lua_toboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitSelectionVolumeData(lua_State* L)
{
	CUnit* unit = ParseCtrlUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return LuaUtils::ParseColVolData(L, 2, &unit->selectionVolume);
}


int LuaUnsyncedCtrl::SetFeatureNoDraw(lua_State* L)
{
	if (CLuaHandle::GetHandleUserMode(L))
		return 0;

	CFeature* feature = ParseCtrlFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->noDraw = luaL_checkboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetFeatureFade(lua_State* L)
{
	CFeature* feature = ParseCtrlFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->alphaFade = luaL_checkboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetFeatureSelectionVolumeData(lua_State* L)
{
	CFeature* feature = ParseCtrlFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;


	return LuaUtils::ParseColVolData(L, 2, &feature->selectionVolume);
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
	const bool   radAdjust = luaL_optboolean(L, 5, false);
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
	const string path = luaL_checkstring(L, 1);

	CFileHandler fhVFS(path, SPRING_VFS_ZIP);
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

	if (!dname.empty() && !FileSystem::CreateDirectory(dname))
		luaL_error(L, "Could not create directory \"%s\" for file \"%s\"", dname.c_str(), fname.c_str());

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
	if ((guihandler == nullptr) || gs->noHelperAIs)
		return 0;

	vector<string> cmds;

	if (lua_istable(L, 1)) { // old style -- table
		constexpr int tableIdx = 1;
		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -1)) {
				string action = lua_tostring(L, -1);
				if (action[0] != '@')
					action = "@@" + action;

				cmds.push_back(action);
			}
		}
	}
	else if (lua_israwstring(L, 1)) { // new style -- function parameters
		for (int i = 1; lua_israwstring(L, i); i++) {
			string action = lua_tostring(L, i);
			if (action[0] != '@')
				action = "@@" + action;

			cmds.push_back(action);
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SendCommands()");
	}

	lua_settop(L, 0); // pop the input arguments

	configHandler->EnableWriting(globalConfig.luaWritableConfigFile);
	guihandler->RunCustomCommands(cmds, false);
	configHandler->EnableWriting(true);
	return 0;
}


/******************************************************************************/

static int SetActiveCommandByIndex(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int args = lua_gettop(L); // number of arguments
	const int cmdIndex = lua_toint(L, 1) - CMD_INDEX_OFFSET;
	int button = luaL_optint(L, 2, 1); // LMB

	if (args <= 2) {
		const bool rmb = (button == SDL_BUTTON_LEFT) ? false : true;
		const bool success = guihandler->SetActiveCommand(cmdIndex, rmb);
		lua_pushboolean(L, success);
		return 1;
	}

	const bool lmb   = luaL_checkboolean(L, 3);
	const bool rmb   = luaL_checkboolean(L, 4);
	const bool alt   = luaL_checkboolean(L, 5);
	const bool ctrl  = luaL_checkboolean(L, 6);
	const bool meta  = luaL_checkboolean(L, 7);
	const bool shift = luaL_checkboolean(L, 8);

	const bool success = guihandler->SetActiveCommand(cmdIndex, button, lmb, rmb,
	                                                  alt, ctrl, meta, shift);
	lua_pushboolean(L, success);
	return 1;
}


static int SetActiveCommandByAction(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int args = lua_gettop(L); // number of arguments
	const string text = lua_tostring(L, 1);
	const Action action(text);

	CKeySet ks;
	if (args >= 2)
		ks.Parse(lua_tostring(L, 2));

	lua_pushboolean(L, guihandler->SetActiveCommand(action, ks, 0));
	return 1;
}


int LuaUnsyncedCtrl::SetActiveCommand(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	if (lua_gettop(L) < 1)
		luaL_error(L, "[%s] one argument required", __func__);

	if (lua_isnumber(L, 1))
		return SetActiveCommandByIndex(L);

	if (lua_isstring(L, 1))
		return SetActiveCommandByAction(L);

	luaL_error(L, "[%s] incorrect argument type", __func__);
	return 0;
}


int LuaUnsyncedCtrl::LoadCmdColorsConfig(lua_State* L)
{
	cmdColors.LoadConfigFromString(luaL_checkstring(L, 1));
	return 0;
}


int LuaUnsyncedCtrl::LoadCtrlPanelConfig(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	guihandler->ReloadConfigFromString(luaL_checkstring(L, 1));
	return 0;
}


int LuaUnsyncedCtrl::ForceLayoutUpdate(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	guihandler->ForceLayoutUpdate();
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::WarpMouse(lua_State* L)
{
	const int x = luaL_checkint(L, 1);
	const int y = globalRendering->viewSizeY - luaL_checkint(L, 2) - 1;
	mouse->WarpMouse(x, y);
	return 0;
}


int LuaUnsyncedCtrl::SetMouseCursor(lua_State* L)
{
	const std::string& cursorName = luaL_checkstring(L, 1);
	const float cursorScale = luaL_optfloat(L, 2, 1.0f);

	mouse->ChangeCursor(cursorName, cursorScale);

	return 0;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetClipboard(lua_State* L)
{
	SDL_SetClipboardText(luaL_checkstring(L, 1));
	return 0;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetCameraOffset(lua_State* L)
{
	camera->posOffset.x = luaL_optfloat(L, 1, 0.0f);
	camera->posOffset.y = luaL_optfloat(L, 2, 0.0f);
	camera->posOffset.z = luaL_optfloat(L, 3, 0.0f);
	camera->tiltOffset.x = luaL_optfloat(L, 4, 0.0f);
	camera->tiltOffset.y = luaL_optfloat(L, 5, 0.0f);
	camera->tiltOffset.z = luaL_optfloat(L, 6, 0.0f);

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetLosViewColors(lua_State* L)
{
	float alwaysColor[3];
	float losColor[3];
	float radarColor[3];
	float jamColor[3];
	float radarColor2[3];

	if ((LuaUtils::ParseFloatArray(L, 1, alwaysColor, 3) != 3) ||
	    (LuaUtils::ParseFloatArray(L, 2, losColor, 3) != 3) ||
	    (LuaUtils::ParseFloatArray(L, 3, radarColor, 3) != 3) ||
		(LuaUtils::ParseFloatArray(L, 4, jamColor, 3) != 3) ||
		(LuaUtils::ParseFloatArray(L, 5, radarColor2, 3) != 3)) {
		luaL_error(L, "Incorrect arguments to SetLosViewColors()");
	}
	const int scale = CBaseGroundDrawer::losColorScale;

	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();
	gd->alwaysColor[0]  = (int)(scale * alwaysColor[0]);
	gd->alwaysColor[1]  = (int)(scale * alwaysColor[1]);
	gd->alwaysColor[2]  = (int)(scale * alwaysColor[2]);
	gd->losColor[0]     = (int)(scale * losColor[0]);
	gd->losColor[1]     = (int)(scale * losColor[1]);
	gd->losColor[2]     = (int)(scale * losColor[2]);
	gd->radarColor[0]   = (int)(scale * radarColor[0]);
	gd->radarColor[1]   = (int)(scale * radarColor[1]);
	gd->radarColor[2]   = (int)(scale * radarColor[2]);
	gd->jamColor[0]     = (int)(scale * jamColor[0]);
	gd->jamColor[1]     = (int)(scale * jamColor[1]);
	gd->jamColor[2]     = (int)(scale * jamColor[2]);
	gd->radarColor2[0]  = (int)(scale * radarColor2[0]);
	gd->radarColor2[1]  = (int)(scale * radarColor2[1]);
	gd->radarColor2[2]  = (int)(scale * radarColor2[2]);
	infoTextureHandler->SetMode(infoTextureHandler->GetMode());
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetConfigInt(lua_State* L)
{
	const std::string& key = luaL_checkstring(L, 1);

	// don't allow to change a read-only variable
	if (configHandler->IsReadOnly(key)) {
		LOG_L(L_ERROR, "[%s] key \"%s\" is read-only", __func__, key.c_str());
		return 0;
	}

	configHandler->EnableWriting(globalConfig.luaWritableConfigFile);
	configHandler->Set(key, luaL_checkint(L, 2), luaL_optboolean(L, 3, false));
	configHandler->EnableWriting(true);
	return 0;
}

int LuaUnsyncedCtrl::SetConfigFloat(lua_State* L)
{
	const std::string& key = luaL_checkstring(L, 1);

	if (configHandler->IsReadOnly(key)) {
		LOG_L(L_ERROR, "[%s] key \"%s\" is read-only", __func__, key.c_str());
		return 0;
	}

	configHandler->EnableWriting(globalConfig.luaWritableConfigFile);
	configHandler->Set(key, luaL_checkfloat(L, 2), luaL_optboolean(L, 3, false));
	configHandler->EnableWriting(true);
	return 0;
}

int LuaUnsyncedCtrl::SetConfigString(lua_State* L)
{
	const std::string& key = luaL_checkstring(L, 1);
	const std::string& val = luaL_checkstring(L, 2);

	if (configHandler->IsReadOnly(key)) {
		LOG_L(L_ERROR, "[%s] key \"%s\" is read-only", __func__, key.c_str());
		return 0;
	}

	configHandler->EnableWriting(globalConfig.luaWritableConfigFile);
	configHandler->SetString(key, val, luaL_optboolean(L, 3, false));
	configHandler->EnableWriting(true);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::CreateDir(lua_State* L)
{
	const std::string& dir = luaL_checkstring(L, 1);

	// keep directories within the Spring directory
	if (dir[0] == '/' || dir[0] == '\\' || dir[0] == '~')
		luaL_error(L, "[%s][1] invalid access: %s", __func__, dir.c_str());
	if (dir[0] == ' ' || dir[0] == '\t')
		luaL_error(L, "[%s][2] invalid access: %s", __func__, dir.c_str());

	if (strstr(dir.c_str(), "..") != nullptr)
		luaL_error(L, "[%s][3] invalid access: %s", __func__, dir.c_str());

	if (dir.size() > 1 && dir[1] == ':')
		luaL_error(L, "[%s][4] invalid access: %s", __func__, dir.c_str());

	lua_pushboolean(L, FileSystem::CreateDirectory(dir));
	return 1;
}


/******************************************************************************/

static int ReloadOrRestart(const std::string& springArgs, const std::string& scriptText, bool newProcess) {
	const std::string springFullName = Platform::GetProcessExecutableFile();
	const std::string scriptFullName = dataDirLocater.GetWriteDirPath() + "script.txt";

	if (!newProcess) {
		// signal SpringApp
		gameSetup->reloadScript = scriptText;
		gu->globalReload = true;

		LOG("[%s] Spring \"%s\" should be reloading", __func__, springFullName.c_str());
		return 0;
	}

	std::vector<std::string> processArgs;

	#if 0
	// arguments to Spring binary given by Lua code, if any
	if (!springArgs.empty())
		processArgs.push_back(springArgs);
	#endif

	if (!scriptText.empty()) {
		// create file 'script.txt' with contents given by Lua code
		std::ofstream scriptFile(scriptFullName.c_str());

		scriptFile.write(scriptText.c_str(), scriptText.size());
		scriptFile.close();

		processArgs.push_back(scriptFullName);
	}

	#ifdef _WIN32
	// else OpenAL crashes when using execvp
	ISound::Shutdown(false);
	#endif
	// close local socket to avoid "bind: Address already in use"
	spring::SafeDelete(gameServer);

	LOG("[%s] Spring \"%s\" should be restarting", __func__, springFullName.c_str());
	Platform::ExecuteProcess(springFullName, processArgs, newProcess);

	// only reached on execvp failure
	return 1;
}


int LuaUnsyncedCtrl::Reload(lua_State* L)
{
	return (ReloadOrRestart("", luaL_checkstring(L, 1), false));
}

int LuaUnsyncedCtrl::Restart(lua_State* L)
{
	// same as Reload now, cl-args are always ignored
	return (ReloadOrRestart(luaL_checkstring(L, 1), luaL_checkstring(L, 2), false));
}

int LuaUnsyncedCtrl::Start(lua_State* L)
{
	if (ReloadOrRestart(luaL_checkstring(L, 1), luaL_checkstring(L, 2), true) != 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	return 0;
}


int LuaUnsyncedCtrl::Quit(lua_State* L)
{
	gu->globalQuit = true;
	return 0;
}


int LuaUnsyncedCtrl::SetWMIcon(lua_State* L)
{
	const std::string iconFileName = luaL_checksstring(L, 1);

	CBitmap iconTexture;

	if (iconTexture.Load(iconFileName)) {
		WindowManagerHelper::SetIcon(&iconTexture);
	} else {
		luaL_error(L, "Failed to load image from file \"%s\"", iconFileName.c_str());
	}

	return 0;
}

int LuaUnsyncedCtrl::SetWMCaption(lua_State* L)
{
	globalRendering->SetWindowTitle(luaL_checksstring(L, 1));
	return 0;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitDefIcon(lua_State* L)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1));

	if (ud == nullptr)
		return 0;

	ud->iconType = icon::iconHandler->GetIcon(luaL_checksstring(L, 2));

	// set decoys to the same icon
	if (ud->decoyDef != nullptr)
		ud->decoyDef->iconType = ud->iconType;

	// spring::unordered_map<int, std::vector<int> >
	const auto& decoyMap = unitDefHandler->GetDecoyDefIDs();
	const auto decoyMapIt = decoyMap.find((ud->decoyDef != nullptr)? ud->decoyDef->id: ud->id);

	if (decoyMapIt == decoyMap.end())
		return 0;

	const auto& decoySet = decoyMapIt->second;

	for (const int decoyDefID: decoySet) {
		const UnitDef* decoyDef = unitDefHandler->GetUnitDefByID(decoyDefID);
		decoyDef->iconType = ud->iconType;
	}

	return 0;
}


int LuaUnsyncedCtrl::SetUnitDefImage(lua_State* L)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1));

	if (ud == nullptr)
		return 0;

	if (lua_isnoneornil(L, 2)) {
		// reset to default texture
		unitDrawer->SetUnitDefImage(ud, ud->buildPicName);
		return 0;
	}

	if (!lua_israwstring(L, 2))
		return 0;

	const std::string& texName = lua_tostring(L, 2);

	if (texName[0] != LuaTextures::prefix) { // '!'
		unitDrawer->SetUnitDefImage(ud, texName);
		return 0;
	}

	const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* tex = textures.GetInfo(texName);

	if (tex == nullptr)
		return 0;

	unitDrawer->SetUnitDefImage(ud, tex->id, tex->xsize, tex->ysize);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitGroup(lua_State* L)
{
	if (gs->noHelperAIs)
		return 0;

	CUnit* unit = ParseRawUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int groupID = luaL_checkint(L, 2);

	if (groupID == -1) {
		unit->SetGroup(nullptr);
		return 0;
	}

	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;

	if ((groupID < 0) || (groupID >= (int)groups.size()))
		return 0;

	CGroup* group = groups[groupID];

	if (group != nullptr)
		unit->SetGroup(group);

	return 0;
}


/******************************************************************************/

static void ParseUnitMap(lua_State* L, const char* caller,
                         int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table))
		luaL_error(L, "%s(): error parsing unit map", caller);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseCtrlUnit(L, __func__, -2); // the key
			if (unit != nullptr && !unit->noSelect) {
				unitIDs.push_back(unit->id);
			}
		}
	}
}


static void ParseUnitArray(lua_State* L, const char* caller,
                           int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table))
		luaL_error(L, "%s(): error parsing unit array", caller);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {   // avoid 'n'
			CUnit* unit = ParseCtrlUnit(L, __func__, -1); // the value
			if (unit != NULL && !unit->noSelect) {
				unitIDs.push_back(unit->id);
			}
		}
	}
	return;
}


/******************************************************************************/

static bool CanGiveOrders(const lua_State* L)
{
	if (gs->PreSimFrame())
		return false;

	if (gs->noHelperAIs)
		return false;

	if (gs->godMode)
		return true;

	if (gu->spectating)
		return false;

	const int ctrlTeam = CLuaHandle::GetHandleCtrlTeam(L);

	// FIXME ? (correct? warning / error?)
	return ((ctrlTeam == gu->myTeam) && (ctrlTeam >= 0));
}


int LuaUnsyncedCtrl::GiveOrder(lua_State* L)
{
	if (!CanGiveOrders(L))
		return 1;

	Command cmd = LuaUtils::ParseCommand(L, __func__, 1);

	selectedUnitsHandler.GiveCommand(cmd);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnit(lua_State* L)
{
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	CUnit* unit = ParseCtrlUnit(L, __func__, 1);
	if (unit == nullptr || unit->noSelect) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __func__, 2);

	clientNet->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, skirmishAIHandler.GetCurrentAIID(), unit->id, cmd.GetID(false), cmd.GetID(true), cmd.GetOpts(), cmd.GetNumParams(), cmd.GetParams()));

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnitMap(lua_State* L)
{
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __func__, 1, unitIDs);
	const int count = (int)unitIDs.size();

	if (count <= 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __func__, 2);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnitsHandler.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnitArray(lua_State* L)
{
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __func__, 1, unitIDs);
	const int count = (int)unitIDs.size();

	if (count <= 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __func__, 2);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnitsHandler.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderArrayToUnitMap(lua_State* L)
{
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __func__, 1, unitIDs);

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __func__, 2, commands);

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
	if (!CanGiveOrders(L)) {
		lua_pushboolean(L, false);
		return 1;
	}

	const int args = lua_gettop(L); // number of arguments

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __func__, 1, unitIDs);

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __func__, 2, commands);

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

int LuaUnsyncedCtrl::SendLuaUIMsg(lua_State* L)
{
	const std::string msg = luaL_checksstring(L, 1);
	const std::vector<std::uint8_t> data(msg.begin(), msg.end());

	const char* mode = luaL_optstring(L, 2, "");

	if (mode[0] != 0 && mode[0] != 'a' && mode[0] != 's')
		luaL_error(L, "Unknown SendLuaUIMsg() mode");

	try {
		clientNet->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_UI, mode[0], data));
	} catch (const netcode::PackPacketException& ex) {
		luaL_error(L, "SendLuaUIMsg() packet error: %s", ex.what());
	}

	return 0;
}


int LuaUnsyncedCtrl::SendLuaGaiaMsg(lua_State* L)
{
	const std::string msg = luaL_checksstring(L, 1);
	const std::vector<std::uint8_t> data(msg.begin(), msg.end());

	try {
		clientNet->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_GAIA, 0, data));
	} catch (const netcode::PackPacketException& ex) {
		luaL_error(L, "SendLuaGaiaMsg() packet error: %s", ex.what());
	}

	return 0;
}


int LuaUnsyncedCtrl::SendLuaRulesMsg(lua_State* L)
{
	const std::string msg = luaL_checksstring(L, 1);
	const std::vector<std::uint8_t> data(msg.begin(), msg.end());

	try {
		clientNet->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_RULES, 0, data));
	} catch (const netcode::PackPacketException& ex) {
		luaL_error(L, "SendLuaRulesMsg() packet error: %s", ex.what());
	}
	return 0;
}

int LuaUnsyncedCtrl::SendLuaMenuMsg(lua_State* L)
{
	if (luaMenu != nullptr)
		luaMenu->RecvLuaMsg(luaL_checksstring(L, 1), gu->myPlayerNum);

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetShareLevel(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || gs->PreSimFrame())
		return 0;


	const char* shareType = lua_tostring(L, 1);
	const float shareLevel = Clamp(luaL_checkfloat(L, 2), 0.0f, 1.0f);

	if (shareType[0] == 'm') {
		clientNet->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, shareLevel, teamHandler.Team(gu->myTeam)->resShare.energy));
		return 0;
	}
	if (shareType[0] == 'e') {
		clientNet->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, teamHandler.Team(gu->myTeam)->resShare.metal, shareLevel));
		return 0;
	}

	LOG_L(L_WARNING, "[%s] unknown resource-type \"%s\"", __func__, shareType);
	return 0;
}


int LuaUnsyncedCtrl::ShareResources(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || gs->PreSimFrame())
		return 0;

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2) || ((args >= 3) && !lua_isnumber(L, 3)))
		luaL_error(L, "Incorrect arguments to ShareResources()");

	const int teamID = lua_toint(L, 1);
	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if ((team == nullptr) || team->isDead)
		return 0;

	const char* type = lua_tostring(L, 2);
	if (type[0] == 'u') {
		// update the selection, and clear the unit command queues
		selectedUnitsHandler.GiveCommand(Command(CMD_STOP), false);
		clientNet->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 1, 0.0f, 0.0f));
		selectedUnitsHandler.ClearSelected();
		return 0;
	}

	if (args < 3)
		return 0;

	if (type[0] == 'm') {
		clientNet->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 0, lua_tofloat(L, 3), 0.0f));
		return 0;
	}
	if (type[0] == 'e') {
		clientNet->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 0, 0.0f, lua_tofloat(L, 3)));
		return 0;
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/


int LuaUnsyncedCtrl::SetLastMessagePosition(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	eventHandler.LastMessagePosition(pos);
	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::MarkerAddPoint(lua_State* L)
{
	if (inMapDrawer == nullptr)
		return 0;

	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));
	const string text = luaL_optstring(L, 4, "");
	const bool onlyLocal = (lua_isnumber(L, 5)) ? bool(luaL_optnumber(L, 5, 1)) : luaL_optboolean(L, 5, true);

	if (onlyLocal) {
		inMapDrawerModel->AddPoint(pos, text, luaL_optnumber(L, 6, gu->myPlayerNum));
	} else {
		inMapDrawer->SendPoint(pos, text, true);
	}

	return 0;
}


int LuaUnsyncedCtrl::MarkerAddLine(lua_State* L)
{
	if (inMapDrawer == nullptr)
		return 0;

	const float3 pos1(luaL_checkfloat(L, 1),
	                  luaL_checkfloat(L, 2),
	                  luaL_checkfloat(L, 3));
	const float3 pos2(luaL_checkfloat(L, 4),
	                  luaL_checkfloat(L, 5),
	                  luaL_checkfloat(L, 6));
	const bool onlyLocal = (lua_isnumber(L, 7)) ? bool(luaL_optnumber(L, 7, 0)) : luaL_optboolean(L, 7, false);

	if (onlyLocal) {
		inMapDrawerModel->AddLine(pos1, pos2, luaL_optnumber(L, 8, gu->myPlayerNum));
	} else {
		inMapDrawer->SendLine(pos1, pos2, true);
	}

	return 0;
}


int LuaUnsyncedCtrl::MarkerErasePosition(lua_State* L)
{
	if (inMapDrawer == nullptr)
		return 0;

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
	if (guihandler != nullptr)
		guihandler->SetDrawSelectionInfo(luaL_checkboolean(L, 1));

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetBuildSpacing(lua_State* L)
{
	if (guihandler != nullptr)
		guihandler->SetBuildSpacing(luaL_checkinteger(L, 1));

	return 0;
}

int LuaUnsyncedCtrl::SetBuildFacing(lua_State* L)
{
	if (guihandler != nullptr)
		guihandler->SetBuildFacing(luaL_checkint(L, 1));

	return 0;
}
/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetAtmosphere(lua_State* L)
{
	if (!lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to SetAtmosphere()");

	for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string& key = lua_tostring(L, -2);

		if (lua_istable(L, -1)) {
			float4 color;
			LuaUtils::ParseFloatArray(L, -1, &color[0], 4);

			if (key == "fogColor") {
				sky->fogColor = color;
			} else if (key == "skyColor") {
				sky->skyColor = color;
			} else if (key == "skyDir") {
// 				sky->skyDir = color;
			} else if (key == "sunColor") {
				sky->sunColor = color;
			} else if (key == "cloudColor") {
				sky->cloudColor = color;
			} else {
				luaL_error(L, "Unknown array key %s", key.c_str());
			}
		}

		else if (lua_isnumber(L, -1)) {
			if (key == "fogStart") {
				sky->fogStart = lua_tofloat(L, -1);
			} else if (key == "fogEnd") {
				sky->fogEnd = lua_tofloat(L, -1);
			} else {
				luaL_error(L, "Unknown scalar key %s", key.c_str());
			}
		}
	}

	return 0;
}

int LuaUnsyncedCtrl::SetSunDirection(lua_State* L)
{
	sky->GetLight()->SetLightDir(float4(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_optfloat(L, 4, 1.0f)));
	return 0;
}

int LuaUnsyncedCtrl::SetSunLighting(lua_State* L)
{
	if (!lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to SetSunLighting()");

	CSunLighting sl = *sunLighting;

	for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string& key = lua_tostring(L, -2);

		if (lua_istable(L, -1)) {
			float4 color;
			LuaUtils::ParseFloatArray(L, -1, &color[0], 4);

			if (sl.SetValue(HsiehHash(key.c_str(), key.size(), 0), color))
				continue;

			luaL_error(L, "Unknown array key %s", key.c_str());
		}

		if (lua_isnumber(L, -1)) {
			if (sl.SetValue(HsiehHash(key.c_str(), key.size(), 0), lua_tofloat(L, -1)))
				continue;

			luaL_error(L, "Unknown scalar key %s", key.c_str());
		}
	}

	*sunLighting = sl;
	return 0;
}

int LuaUnsyncedCtrl::SetMapRenderingParams(lua_State* L)
{
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetMapRenderingParams()");
	}

	for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string key = lua_tostring(L, -2);

		if (lua_istable(L, -1)) {
			float values[4];
			const int size = LuaUtils::ParseFloatArray(L, -1, values, 4);
			if (size == 4) {
				if (key == "splatTexScales") {
					mapRendering->splatTexScales = values;
				} else if (key == "splatTexMults") {
					mapRendering->splatTexMults = values;
				} else {
					luaL_error(L, "Unknown array key %s", key.c_str());
				}
			} else {
				luaL_error(L, "Unexpected size of %d for array key %s", size,  key.c_str());
			}
		}

		else if (lua_isboolean(L, -1)) {
			const bool value = lua_toboolean(L, -1);
			if (key == "voidWater") {
				mapRendering->voidWater = value;
			} else if (key == "voidGround") {
				mapRendering->voidGround = value;
			} else if (key == "splatDetailNormalDiffuseAlpha") {
				mapRendering->splatDetailNormalDiffuseAlpha = value;
			} else {
				luaL_error(L, "Unknown boolean key %s", key.c_str());
			}
		}
	}

	CBaseGroundDrawer* groundDrawer = readMap->GetGroundDrawer();
	groundDrawer->UpdateRenderState();

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SendSkirmishAIMessage(lua_State* L) {
	if (CLuaHandle::GetHandleSynced(L))
		return 0;

	const int aiTeam = luaL_checkint(L, 1);
	const char* inData = luaL_checkstring(L, 2);

	std::vector<const char*> outData;

	luaL_checkstack(L, 2, __func__);
	lua_pushboolean(L, eoh->SendLuaMessages(aiTeam, inData, outData));

	// push the AI response(s)
	lua_createtable(L, outData.size(), 0);
	for (unsigned int n = 0; n < outData.size(); n++) {
		lua_pushstring(L, outData[n]);
		lua_rawseti(L, -2, n + 1);
	}

	return 2;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetLogSectionFilterLevel(lua_State* L) {
	const int loglevel = LuaUtils::ParseLogLevel(L, 2);

	if (loglevel < 0)
		return luaL_error(L, "Incorrect arguments to Spring.SetLogSectionFilterLevel(logsection, loglevel)");

	log_frontend_register_runtime_section(loglevel, luaL_checkstring(L, 1));
	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::ClearWatchDogTimer(lua_State* L) {
	if (lua_gettop(L) == 0) {
		Watchdog::ClearTimer();
		return 0;
	}

	if (lua_isstring(L, 1)) {
		Watchdog::ClearTimer(lua_tostring(L, 1));
	} else {
		Watchdog::ClearTimer("main");
	}

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::PreloadUnitDefModel(lua_State* L) {
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1));

	if (ud == nullptr)
		return 0;

	ud->PreloadModel();
	return 0;
}

int LuaUnsyncedCtrl::PreloadFeatureDefModel(lua_State* L) {
	const FeatureDef* fd = featureDefHandler->GetFeatureDefByID(luaL_checkint(L, 1));

	if (fd == nullptr)
		return 0;

	fd->PreloadModel();
	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::CreateDecal(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const int idx = decalsGl4->CreateLuaDecal();
	if (idx > 0) {
		lua_pushnumber(L, idx);
		return 1;
	}
	return 0;
}


int LuaUnsyncedCtrl::DestroyDecal(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	decal.Free();
	return 0;
}


int LuaUnsyncedCtrl::SetDecalPos(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const float3 newPos(luaL_checkfloat(L, 2),
	luaL_checkfloat(L, 3),
	luaL_checkfloat(L, 4));

	auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	decal.pos = newPos;
	lua_pushboolean(L, decal.InvalidateExtents());
	return 1;
}


int LuaUnsyncedCtrl::SetDecalSize(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const float2 newSize(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));

	auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	decal.size = newSize;
	lua_pushboolean(L, decal.InvalidateExtents());
	return 1;
}


int LuaUnsyncedCtrl::SetDecalRotation(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	decal.rot = luaL_checkfloat(L, 2);
	lua_pushboolean(L, decal.InvalidateExtents());
	return 1;
}


int LuaUnsyncedCtrl::SetDecalTexture(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	decal.SetTexture(luaL_checksstring(L, 2));
	decal.Invalidate();
	return 0;
}


int LuaUnsyncedCtrl::SetDecalAlpha(lua_State* L)
{
	auto decalsGl4 = dynamic_cast<CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	decal.alpha = luaL_checkfloat(L, 2);
	decal.Invalidate();
	return 0;
}

int LuaUnsyncedCtrl::SDLSetTextInputRect(lua_State* L)
{
	SDL_Rect textWindow;
	textWindow.x = luaL_checkint(L, 1);
	textWindow.y = luaL_checkint(L, 2);
	textWindow.w = luaL_checkint(L, 3);
	textWindow.h = luaL_checkint(L, 4);
	SDL_SetTextInputRect(&textWindow);
	return 0;
}

int LuaUnsyncedCtrl::SDLStartTextInput(lua_State* L)
{
	SDL_StartTextInput();
	return 0;
}

int LuaUnsyncedCtrl::SDLStopTextInput(lua_State* L)
{
	SDL_StopTextInput();
	return 0;
}
