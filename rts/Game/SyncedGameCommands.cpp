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
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
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
void CGame::ActionReceived(const Action& action, int playernum)
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
		std::string s = "give "; //FIXME lazyness
		s += action.extra;

		// .give [amount] <unitName> [team] <@x,y,z>
		const vector<string> &args = CSimpleParser::Tokenize(s, 0);

		if (args.size() < 3) {
			logOutput.Print("Someone is spoofing invalid .give messages!");
			return;
		}

		float3 pos;
		if (sscanf(args[args.size() - 1].c_str(), "@%f,%f,%f", &pos.x, &pos.y, &pos.z) != 3) {
			logOutput.Print("Someone is spoofing invalid .give messages!");
			return;
		}

		int amount = 1;
		int team = playerHandler->Player(playernum)->team;

		int amountArgIdx = -1;
		int teamArgIdx = -1;

		if (args.size() == 5) {
			amountArgIdx = 1;
			teamArgIdx = 3;
		}
		else if (args.size() == 4) {
			if (args[1].find_first_not_of("0123456789") == string::npos) {
				amountArgIdx = 1;
			} else {
				teamArgIdx = 2;
			}
		}

		if (amountArgIdx >= 0) {
			const string& amountStr = args[amountArgIdx];
			amount = atoi(amountStr.c_str());
			if ((amount < 0) || (amountStr.find_first_not_of("0123456789") != string::npos)) {
				logOutput.Print("Bad give amount: %s", amountStr.c_str());
				return;
			}
		}

		if (teamArgIdx >= 0) {
			const string& teamStr = args[teamArgIdx];
			team = atoi(teamStr.c_str());
			if ((!teamHandler->IsValidTeam(team)) || (teamStr.find_first_not_of("0123456789") != string::npos)) {
				logOutput.Print("Bad give team: %s", teamStr.c_str());
				return;
			}
		}

		const string unitName = (amountArgIdx >= 0) ? args[2] : args[1];

		if (unitName == "all") {
			// player entered ".give all"
			int numRequestedUnits = unitDefHandler->unitDefs.size() - 1; /// defid=0 is not valid
			int currentNumUnits = teamHandler->Team(team)->units.size();
			int sqSize = (int) streflop::ceil(streflop::sqrt((float) numRequestedUnits));

			// make sure team unit-limit not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->MaxUnitsPerTeam()) {
				numRequestedUnits = uh->MaxUnitsPerTeam() - currentNumUnits;
			}

			// make sure square is entirely on the map
			float sqHalfMapSize = sqSize / 2 * 10 * SQUARE_SIZE;
			pos.x = std::max(sqHalfMapSize, std::min(pos.x, float3::maxxpos - sqHalfMapSize - 1));
			pos.z = std::max(sqHalfMapSize, std::min(pos.z, float3::maxzpos - sqHalfMapSize - 1));

			for (int a = 1; a <= numRequestedUnits; ++a) {
				float posx = pos.x + (a % sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float posz = pos.z + (a / sqSize - sqSize / 2) * 10 * SQUARE_SIZE;
				float3 pos2 = float3(posx, pos.y, posz);
				const UnitDef* ud = unitDefHandler->GetUnitDefByID(a);
				if (ud) {
					const CUnit* unit = unitLoader->LoadUnit(ud, pos2, team, false, 0, NULL);
					if (unit) {
						unitLoader->FlattenGround(unit);
					}
				}
			}
		}
		else if (!unitName.empty()) {
			int numRequestedUnits = amount;
			int currentNumUnits = teamHandler->Team(team)->units.size();

			if (currentNumUnits >= uh->MaxUnitsPerTeam()) {
				LogObject() << "Unable to give any more units to team " << team << "(current: " << currentNumUnits << ", max: " << uh->MaxUnits() << ")";
				return;
			}

			// make sure team unit-limit is not exceeded
			if ((currentNumUnits + numRequestedUnits) > uh->MaxUnitsPerTeam()) {
				numRequestedUnits = uh->MaxUnitsPerTeam() - currentNumUnits;
			}

			const UnitDef* unitDef = unitDefHandler->GetUnitDefByName(unitName);

			if (unitDef != NULL) {
				int xsize = unitDef->xsize;
				int zsize = unitDef->zsize;
				int squareSize = (int) streflop::ceil(streflop::sqrt((float) numRequestedUnits));
				int total = numRequestedUnits;

				float3 minpos = pos;
				minpos.x -= ((squareSize - 1) * xsize * SQUARE_SIZE) / 2;
				minpos.z -= ((squareSize - 1) * zsize * SQUARE_SIZE) / 2;

				for (int z = 0; z < squareSize; ++z) {
					for (int x = 0; x < squareSize && total > 0; ++x) {
						float minposx = minpos.x + x * xsize * SQUARE_SIZE;
						float minposz = minpos.z + z * zsize * SQUARE_SIZE;
						const float3 upos(minposx, minpos.y, minposz);
						const CUnit* unit = unitLoader->LoadUnit(unitDef, upos, team, false, 0, NULL);

						if (unit) {
							unitLoader->FlattenGround(unit);
						}
						--total;
					}
				}

				logOutput.Print("Giving %i %s to team %i", numRequestedUnits, unitName.c_str(), team);
			} else {
				int allyteam = -1;
				if (teamArgIdx < 0) {
					team = -1; // default to world features
					allyteam = -1;
				} else {
					allyteam = teamHandler->AllyTeam(team);
				}

				const FeatureDef* featureDef = featureHandler->GetFeatureDef(unitName);
				if (featureDef) {
					int xsize = featureDef->xsize;
					int zsize = featureDef->zsize;
					int squareSize = (int) streflop::ceil(streflop::sqrt((float) numRequestedUnits));
					int total = amount; // FIXME -- feature count limit?

					float3 minpos = pos;
					minpos.x -= ((squareSize - 1) * xsize * SQUARE_SIZE) / 2;
					minpos.z -= ((squareSize - 1) * zsize * SQUARE_SIZE) / 2;

					for (int z = 0; z < squareSize; ++z) {
						for (int x = 0; x < squareSize && total > 0; ++x) {
							float minposx = minpos.x + x * xsize * SQUARE_SIZE;
							float minposz = minpos.z + z * zsize * SQUARE_SIZE;
							float minposy = ground->GetHeightReal(minposx, minposz);
							const float3 upos(minposx, minposy, minposz);

							CFeature* feature = new CFeature();
							// Initialize() adds the feature to the FeatureHandler -> no memory-leak
							feature->Initialize(upos, featureDef, 0, 0, team, allyteam, "");
							--total;
						}
					}

					logOutput.Print("Giving %i %s (feature) to team %i",
									numRequestedUnits, unitName.c_str(), team);
				}
				else {
					logOutput.Print(unitName + " is not a valid unitname");
				}
			}
		}
	}
	else if (action.command == "destroy" && gs->cheatEnabled) {
		std::stringstream ss(action.extra);
		logOutput.Print("Killing units: %s", action.extra.c_str());
		do {
			unsigned id;
			ss >> id;
			if (!ss)
				break;
			if (id >= uh->units.size())
				continue;
			if (uh->units[id] == NULL)
				continue;
			uh->units[id]->KillUnit(false, false, 0);
		} while (true);
	}
	else if (action.command == "nospectatorchat") {
		SetBoolArg(noSpectatorChat, action.extra);
		logOutput.Print("Spectators %s chat", noSpectatorChat ? "can not" : "can");
	}
	else if (action.command == "reloadcob" && gs->cheatEnabled) {
		ReloadCOB(action.extra, playernum);
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
		if ((action.extra == "reload") && (playernum == 0)) {
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
		else if ((action.extra == "disable") && (playernum == 0)) {
			if (!gs->cheatEnabled) {
				logOutput.Print("Cheating required to disable synced scripts");
			} else {
				CLuaRules::FreeHandler();
				logOutput.Print("LuaRules disabled");
			}
		}
		else {
			if (luaRules) luaRules->GotChatMsg(action.extra, playernum);
		}
	}
	else if ((action.command == "luagaia") && (gs->frameNum > 1)) {
		if (gs->useLuaGaia) {
			if ((action.extra == "reload") && (playernum == 0)) {
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
			else if ((action.extra == "disable") && (playernum == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts");
				} else {
					CLuaGaia::FreeHandler();
					logOutput.Print("LuaGaia disabled");
				}
			}
			else if (luaGaia) {
				luaGaia->GotChatMsg(action.extra, playernum);
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
				if (playernum == gu->myPlayerNum) {
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
		int team = playerHandler->Player(playernum)->team;
		teamHandler->Team(team)->AddMetal(1000);
		teamHandler->Team(team)->AddEnergy(1000);
	}
	else if (action.command == "take" && (!playerHandler->Player(playernum)->spectator || gs->cheatEnabled)) {
		const int sendTeam = playerHandler->Player(playernum)->team;
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
		if (luaRules) luaRules->SyncedActionFallback(action.rawline, playernum);
		if (luaGaia)  luaGaia->SyncedActionFallback(action.rawline, playernum);
	}
}
