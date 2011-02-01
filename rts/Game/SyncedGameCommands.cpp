/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "Game.h"
#include "CommandMessage.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#ifdef _WIN32
#  include "winerror.h"
#endif

#include "Rendering/InMapDraw.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Unit.h"
#include "UI/GameInfo.h"
#include "UI/LuaUI.h"
#include "UI/MouseHandler.h"
#include "System/FPUCheck.h"
#include "System/FileSystem/SimpleParser.h"

void SetBoolArg(bool& value, const std::string& str)
{
	if (str.empty()) // toggle
	{
		value = !value;
	}
	else // set
	{
		const int num = atoi(str.c_str());
		value = (num != 0);
	}
}



bool CGame::ActionReleased(const Action& action)
{
	const string& cmd = action.command;

	if (cmd == "drawinmap"){
		inMapDrawer->keyPressed=false;
	}
	else if (cmd == "moveforward") {
		camMove[0]=false;
	}
	else if (cmd == "moveback") {
		camMove[1]=false;
	}
	else if (cmd == "moveleft") {
		camMove[2]=false;
	}
	else if (cmd == "moveright") {
		camMove[3]=false;
	}
	else if (cmd == "moveup") {
		camMove[4]=false;
	}
	else if (cmd == "movedown") {
		camMove[5]=false;
	}
	else if (cmd == "movefast") {
		camMove[6]=false;
	}
	else if (cmd == "moveslow") {
		camMove[7]=false;
	}
	else if (cmd == "mouse1") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 1);
	}
	else if (cmd == "mouse2") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 2);
	}
	else if (cmd == "mouse3") {
		mouse->MouseRelease (mouse->lastx, mouse->lasty, 3);
	}
	else if (cmd == "mousestate") {
			mouse->ToggleState();
	}
	else if (cmd == "gameinfoclose") {
		CGameInfo::Disable();
	}
	// HACK   somehow weird things happen when MouseRelease is called for button 4 and 5.
	// Note that SYS_WMEVENT on windows also only sends MousePress events for these buttons.
// 	else if (cmd == "mouse4") {
// 		mouse->MouseRelease (mouse->lastx, mouse->lasty, 4);
//	}
// 	else if (cmd == "mouse5") {
// 		mouse->MouseRelease (mouse->lastx, mouse->lasty, 5);
//	}

	return 0;
}

// FOR SYNCED MESSAGES
void CGame::ActionReceived(const Action& action, int playerNum)
{
	if (action.command == "cheat") {
		SetBoolArg(gs->cheatEnabled, action.extra);
		if (gs->cheatEnabled)
			logOutput.Print("Cheating!");
		else
			logOutput.Print("No more cheating");
	}
	else if (action.command == "nohelp") {
		SetBoolArg(gs->noHelperAIs, action.extra);
		selectedUnits.PossibleCommandChange(NULL);
		logOutput.Print("LuaUI control is %s", gs->noHelperAIs ? "disabled" : "enabled");
	}
	else if (action.command == "nospecdraw") {
		bool buf;
		SetBoolArg(buf, action.extra);
		inMapDrawer->SetSpecMapDrawingAllowed(buf);
	}
	else if (action.command == "godmode") {
		if (!gs->cheatEnabled)
			logOutput.Print("godmode requires /cheat");
		else {
			SetBoolArg(gs->godMode, action.extra);
			CLuaUI::UpdateTeams();
			if (gs->godMode) {
				logOutput.Print("God Mode Enabled");
			} else {
				logOutput.Print("God Mode Disabled");
			}
			CPlayer::UpdateControlledTeams();
		}
	}
	else if (action.command == "globallos") {
		if (!gs->cheatEnabled) {
			logOutput.Print("globallos requires /cheat");
		} else {
			SetBoolArg(gs->globalLOS, action.extra);
			if (gs->globalLOS) {
				logOutput.Print("Global LOS Enabled");
			} else {
				logOutput.Print("Global LOS Disabled");
			}
		}
	}
	else if (action.command == "nocost" && gs->cheatEnabled) {
		if (unitDefHandler->ToggleNoCost()) {
			logOutput.Print("Everything is for free!");
		} else {
			logOutput.Print("Everything costs resources again!");
		}
	}
	else if (action.command == "give" && gs->cheatEnabled) {
		// FIXME lazyness
		const std::string cmd = action.command + " " + action.extra;
		const vector<string>& args = CSimpleParser::Tokenize(cmd, 0);

		unitLoader->GiveUnits(args, playerHandler->Player(playerNum)->team);
	}
	else if (action.command == "destroy" && gs->cheatEnabled) {
		std::stringstream ss(action.extra);
		logOutput.Print("Killing units: %s", action.extra.c_str());

		do {
			unsigned int id; ss >> id;

			if (!ss)
				break;

			CUnit* u = uh->GetUnit(id);

			if (u != NULL)
				u->KillUnit(false, false, 0);
		} while (true);
	}
	else if (action.command == "nospectatorchat") {
		SetBoolArg(noSpectatorChat, action.extra);
		logOutput.Print("Spectators %s chat", noSpectatorChat ? "can not" : "can");
	}
	else if (action.command == "reloadcob" && gs->cheatEnabled) {
		ReloadCOB(action.extra, playerNum);
	}
	else if (action.command == "reloadcegs" && gs->cheatEnabled) {
		ReloadCEGs(action.extra);
	}
	else if (action.command == "devlua" && gs->cheatEnabled) {
		bool devMode = CLuaHandle::GetDevMode();
		SetBoolArg(devMode, action.extra);
		CLuaHandle::SetDevMode(devMode);
		if (devMode) {
			logOutput.Print("Lua devmode enabled, this can cause desyncs");
		} else {
			logOutput.Print("Lua devmode disabled");
		}
	}
	else if (action.command == "editdefs" && gs->cheatEnabled) {
		SetBoolArg(gs->editDefsEnabled, action.extra);
		if (gs->editDefsEnabled)
			logOutput.Print("Definition Editing!");
		else
			logOutput.Print("No definition Editing");
	}
	else if ((action.command == "luarules") && (gs->frameNum > 1)) {
		if ((action.extra == "reload") && (playerNum == 0)) {
			if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to reload synced scripts");
			} else {
				CLuaRules::FreeHandler();
				CLuaRules::LoadHandler();
				if (luaRules) {
					logOutput.Print("LuaRules reloaded");
				} else {
					logOutput.Print("LuaRules reload failed");
				}
			}
		}
		else if ((action.extra == "disable") && (playerNum == 0)) {
			if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to disable synced scripts");
			} else {
				CLuaRules::FreeHandler();
				logOutput.Print("LuaRules disabled");
			}
		}
		else {
			if (luaRules) luaRules->GotChatMsg(action.extra, playerNum);
		}
	}
	else if ((action.command == "luagaia") && (gs->frameNum > 1)) {
		if (gs->useLuaGaia) {
			if ((action.extra == "reload") && (playerNum == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to reload synced scripts");
				} else {
					CLuaGaia::FreeHandler();
					CLuaGaia::LoadHandler();
					if (luaGaia) {
						logOutput.Print("LuaGaia reloaded");
					} else {
						logOutput.Print("LuaGaia reload failed");
					}
				}
			}
			else if ((action.extra == "disable") && (playerNum == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts");
				} else {
					CLuaGaia::FreeHandler();
					logOutput.Print("LuaGaia disabled");
				}
			}
			else if (luaGaia) {
				luaGaia->GotChatMsg(action.extra, playerNum);
			}
			else {
				logOutput.Print("LuaGaia is not enabled");
			}
		}
	}
#ifdef DEBUG
	else if (action.command == "desync" && gs->cheatEnabled) {
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123.0f);
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123);
		ASSERT_SYNCED_PRIMITIVE((short)(gu->myPlayerNum * 123 + 123));
		ASSERT_SYNCED_FLOAT3(float3(gu->myPlayerNum, gu->myPlayerNum, gu->myPlayerNum));

		for (size_t i = uh->MaxUnits() - 1; i >= 0; --i) {
			if (uh->units[i]) {
				if (playerNum == gu->myPlayerNum) {
					++uh->units[i]->midPos.x; // and desync...
					++uh->units[i]->midPos.x;
				} else {
					// execute the same amount of flops on any other player, but don't desync (it's a NOP)...
					++uh->units[i]->midPos.x;
					--uh->units[i]->midPos.x;
				}
				break;
			}
		}
		logOutput.Print("Desyncing in frame %d.", gs->frameNum);
	}
#endif // defined DEBUG
	else if (action.command == "atm" && gs->cheatEnabled) {
		const int team = playerHandler->Player(playerNum)->team;
		teamHandler->Team(team)->AddMetal(1000);
		teamHandler->Team(team)->AddEnergy(1000);
	}
	else if (action.command == "take" && (!playerHandler->Player(playerNum)->spectator || gs->cheatEnabled)) {
		const int sendTeam = playerHandler->Player(playerNum)->team;
		for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
			if (teamHandler->AlliedTeams(a, sendTeam)) {
				bool hasPlayer = false;
				for (int b = 0; b < playerHandler->ActivePlayers(); ++b) {
					if (playerHandler->Player(b)->active && playerHandler->Player(b)->team==a && !playerHandler->Player(b)->spectator) {
						hasPlayer = true;
						break;
					}
				}
				if (!hasPlayer) {
					teamHandler->Team(a)->GiveEverythingTo(sendTeam);
				}
			}
		}
	}
	else if (action.command == "skip") {
		if (action.extra.find_first_of("start") == 0) {
			std::istringstream buf(action.extra.substr(6));
			int targetframe;
			buf >> targetframe;
			StartSkip(targetframe);
		}
		else if (action.extra == "end") {
			EndSkip();
		}
	}
	else if (gs->frameNum > 1) {
		if (luaRules) luaRules->SyncedActionFallback(action.rawline, playerNum);
		if (luaGaia) luaGaia->SyncedActionFallback(action.rawline, playerNum);
	}
}
