/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <SDL_events.h>

#include "Game.h"
#include "CameraHandler.h"
#include "ConsoleHistory.h"
#include "GameServer.h"
#include "CommandMessage.h"
#include "GameSetup.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "PlayerRoster.h"
#include "TimeProfiler.h"
#include "IVideoCapturing.h"
#include "Game/UI/UnitTracker.h"
#ifdef _WIN32
#  include "winerror.h"
#endif
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/glFont.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/Screenshot.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/VerticalSync.h"
#include "Lua/LuaOpenGL.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "UI/CommandColors.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/KeyBindings.h"
#include "UI/LuaUI.h"
#include "UI/MiniMap.h"
#include "UI/QuitBox.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/ShareBox.h"
#include "UI/TooltipConsole.h"
#include "UI/ProfileDrawer.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/NetProtocol.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISound.h"
#include "System/Sound/IEffectChannel.h"
#include "System/Sound/IMusicChannel.h"

static std::vector<std::string> _local_strSpaceTokenize(const std::string& text) {

	std::vector<std::string> tokens;

#define SPACE_DELIMS " \t"
	// Skip delimiters at beginning.
	std::string::size_type lastPos = text.find_first_not_of(SPACE_DELIMS, 0);
	// Find first "non-delimiter".
	std::string::size_type pos     = text.find_first_of(SPACE_DELIMS, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(text.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = text.find_first_not_of(SPACE_DELIMS, pos);
		// Find next "non-delimiter"
		pos = text.find_first_of(SPACE_DELIMS, lastPos);
	}
#undef SPACE_DELIMS

	return tokens;
}


// FOR UNSYNCED MESSAGES
bool CGame::ActionPressed(const Action& action,
                          const CKeySet& ks, bool isRepeat)
{
	// we may need these later
	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();

	const string& cmd = action.command;
	bool notfound1=false;

	// process the action
	if (cmd == "select") {
		selectionKeys->DoSelection(action.extra);
	}
	else if (cmd == "selectunits") {
		SelectUnits(action.extra);
	}
	else if (cmd == "selectcycle") {
		SelectCycle(action.extra);
	}
	else if (cmd == "deselect") {
		selectedUnits.ClearSelected();
	}
	else if (cmd == "shadows") {
		const int current = configHandler->Get("Shadows", 0);
		if (current < 0) {
			logOutput.Print("Shadows have been disabled with %i", current);
			logOutput.Print("Change your configuration and restart to use them");
			return true;
		}
		else if (!shadowHandler->canUseShadows) {
			logOutput.Print("Your hardware/driver setup does not support shadows");
			return true;
		}

		delete shadowHandler;
		int next = 0;
		if (!action.extra.empty()) {
			int mapsize = 2048;
			const char* args = action.extra.c_str();
			const int argcount = sscanf(args, "%i %i", &next, &mapsize);
			if (argcount > 1) {
				configHandler->Set("ShadowMapSize", mapsize);
			}
		} else {
			next = (current+1)%3;
		}
		configHandler->Set("Shadows", next);
		logOutput.Print("Set Shadows to %i", next);
		shadowHandler = new CShadowHandler();
	}
	else if (cmd == "water") {
		int nextWaterRendererMode = 0;

		if (!action.extra.empty()) {
			nextWaterRendererMode = std::max(0, atoi(action.extra.c_str()) % CBaseWater::NUM_WATER_RENDERERS);
		} else {
			nextWaterRendererMode = (std::max(0, water->GetID()) + 1) % CBaseWater::NUM_WATER_RENDERERS;
		}

		water = CBaseWater::GetWater(water, nextWaterRendererMode);
		logOutput.Print("Set water rendering mode to %i (%s)", nextWaterRendererMode, water->GetName());
	}
	else if (cmd == "advshading") {
		static bool canUse = unitDrawer->advShading;
		if (canUse) {
			if (!action.extra.empty()) {
				unitDrawer->advShading = !!atoi(action.extra.c_str());
			} else {
				unitDrawer->advShading = !unitDrawer->advShading;
			}
			logOutput.Print("Advanced shading %s",
			                unitDrawer->advShading ? "enabled" : "disabled");
		}
	}
	else if (cmd == "say") {
		SendNetChat(action.extra);
	}
	else if (cmd == "w") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			const int playernum = playerHandler->Player(action.extra.substr(0, pos));
			if (playernum >= 0) {
				SendNetChat(action.extra.substr(pos+1), playernum);
			} else {
				logOutput.Print("Player not found: %s", action.extra.substr(0, pos).c_str());
			}
		}
	}
	else if (cmd == "wbynum") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			std::istringstream buf(action.extra.substr(0, pos));
			int playernum;
			buf >> playernum;
			if (playernum >= 0) {
				SendNetChat(action.extra.substr(pos+1), playernum);
			} else {
				logOutput.Print("Playernumber invalid: %i", playernum);
			}
		}
	}
	else if (cmd == "echo") {
		logOutput.Print(action.extra);
	}
	else if (cmd == "set") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.extra.substr(0, pos);
			configHandler->SetString(varName, action.extra.substr(pos+1));
		}
	}
	else if (cmd == "tset") {
		const std::string::size_type pos = action.extra.find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.extra.substr(0, pos);
			configHandler->SetOverlay(varName, action.extra.substr(pos+1));
		}
	}
	else if (cmd == "drawinmap") {
		inMapDrawer->keyPressed = true;
	}
	else if (cmd == "drawlabel") {
		float3 pos = inMapDrawer->GetMouseMapPos();
		if (pos.x >= 0) {
			inMapDrawer->keyPressed = false;
			inMapDrawer->PromptLabel(pos);
			if ((ks.Key() >= SDLK_SPACE) && (ks.Key() <= SDLK_DELETE)) {
				ignoreNextChar=true;
			}
		}
	}

	else if (!isRepeat && cmd == "mouse1") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 1);
	}
	else if (!isRepeat && cmd == "mouse2") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 2);
	}
	else if (!isRepeat && cmd == "mouse3") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 3);
	}
	else if (!isRepeat && cmd == "mouse4") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 4);
	}
	else if (!isRepeat && cmd == "mouse5") {
		mouse->MousePress (mouse->lastx, mouse->lasty, 5);
	}
	else if (cmd == "viewselection") {

		GML_RECMUTEX_LOCK(sel); // ActionPressed

		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		if (!selUnits.empty()) {
			float3 pos(0.0f, 0.0f, 0.0f);
			CUnitSet::const_iterator it;
			for (it = selUnits.begin(); it != selUnits.end(); ++it) {
				pos += (*it)->midPos;
			}
			pos /= (float)selUnits.size();
			camHandler->GetCurrentController().SetPos(pos);
			camHandler->CameraTransition(0.6f);
		}
	}

	else if (cmd == "moveforward") {
		camMove[0]=true;
	}
	else if (cmd == "moveback") {
		camMove[1]=true;
	}
	else if (cmd == "moveleft") {
		camMove[2]=true;
	}
	else if (cmd == "moveright") {
		camMove[3]=true;
	}
	else if (cmd == "moveup") {
		camMove[4]=true;
	}
	else if (cmd == "movedown") {
		camMove[5]=true;
	}
	else if (cmd == "movefast") {
		camMove[6]=true;
	}
	else if (cmd == "moveslow") {
		camMove[7]=true;
	}
	else if ((cmd == "aikill") || (cmd == "aireload")) {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool hasArgs            = (action.extra.size() > 0);
		const std::vector<std::string> &args = _local_strSpaceTokenize(action.extra);
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);
		const std::string actionName  = cmd.substr(2);

		if (hasArgs) {
			size_t skirmishAIId           = 0; // will only be used if !badArgs
			bool share = false;
			int teamToKillId         = -1;
			int teamToReceiveUnitsId = -1;

			if (args.size() >= 1) {
				teamToKillId = atoi(args[0].c_str());
			}
			if ((args.size() >= 2) && (actionName == "kill")) {
				teamToReceiveUnitsId = atoi(args[1].c_str());
				share = true;
			}

			CTeam* teamToKill = teamHandler->IsActiveTeam(teamToKillId) ?
			                          teamHandler->Team(teamToKillId) : NULL;
			const CTeam* teamToReceiveUnits = teamHandler->Team(teamToReceiveUnitsId) ?
			                                  teamHandler->Team(teamToReceiveUnitsId) : NULL;

			if (teamToKill == NULL) {
				logOutput.Print("Team to %s: not a valid team number: \"%s\"", actionName.c_str(), args[0].c_str());
				badArgs = true;
			}
			if (share && teamToReceiveUnits == NULL) {
				logOutput.Print("Team to receive units: not a valid team number: \"%s\"", args[1].c_str());
				badArgs = true;
			}
			if (!badArgs && (skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId).size() == 0)) {
				logOutput.Print("Team to %s: not a Skirmish AI team: %i", actionName.c_str(), teamToKillId);
				badArgs = true;
			} else {
				const CSkirmishAIHandler::ids_t skirmishAIIds = skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId, gu->myPlayerNum);
				if (skirmishAIIds.size() > 0) {
					skirmishAIId = skirmishAIIds[0];
				} else {
					logOutput.Print("Team to %s: not a local Skirmish AI team: %i", actionName.c_str(), teamToKillId);
					badArgs = true;
				}
			}
			if (!badArgs && skirmishAIHandler.GetSkirmishAI(skirmishAIId)->isLuaAI) {
				logOutput.Print("Team to %s: it is not yet supported to %s Lua AIs", actionName.c_str(), actionName.c_str());
				badArgs = true;
			}
			if (!badArgs) {
				const bool weAreAllied  = teamHandler->AlliedTeams(fromTeamId, teamToKillId);
				const bool weAreAIHost  = (skirmishAIHandler.GetSkirmishAI(skirmishAIId)->hostPlayer == gu->myPlayerNum);
				const bool weAreLeader  = (teamToKill->leader == gu->myPlayerNum);
				if (!(weAreAIHost || weAreLeader || singlePlayer || (weAreAllied && cheating))) {
					logOutput.Print("Team to %s: player %s is not allowed to %s Skirmish AI controlling team %i (try with /cheat)",
							actionName.c_str(), fromPlayer->name.c_str(), actionName.c_str(), teamToKillId);
					badArgs = true;
				}
			}
			if (!badArgs && teamToKill->isDead) {
				logOutput.Print("Team to %s: is a dead team already: %i", actionName.c_str(), teamToKillId);
				badArgs = true;
			}

			if (!badArgs) {
				if (actionName == "kill") {
					if (share) {
						net->Send(CBaseNetProtocol::Get().SendGiveAwayEverything(gu->myPlayerNum, teamToReceiveUnitsId, teamToKillId));
						// when the AIs team has no units left,
						// the AI will be destroyed automatically
					} else {
						const SkirmishAIData* sai = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
						const bool isLocalSkirmishAI = (sai->hostPlayer == gu->myPlayerNum);
						if (isLocalSkirmishAI) {
							skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 3 /* = AI killed */);
						}
					}
				} else if (actionName == "reload") {
					net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_RELOADING));
				}

				logOutput.Print("Skirmish AI controlling team %i is beeing %sed ...", teamToKillId, actionName.c_str());
			}
		} else {
			logOutput.Print("/%s: missing mandatory argument \"teamTo%s\"", action.command.c_str(), actionName.c_str());
			badArgs = true;
		}

		if (badArgs) {
			if (actionName == "kill") {
				logOutput.Print("description: "
				                "Kill a Skirmish AI controlling a team. The team itsself will remain alive, "
				                "unless a second argument is given, which specifies an active team "
				                "that will receive all the units of the AI team.");
				logOutput.Print("usage:   /%s teamToKill [teamToReceiveUnits]", action.command.c_str());
			} else if (actionName == "reload") {
				logOutput.Print("description: "
				                "Reload a Skirmish AI controlling a team."
				                "The team itsself will remain alive during the process.");
				logOutput.Print("usage:   /%s teamToReload", action.command.c_str());
			}
		}
	}
	else if (cmd == "aicontrol") {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool hasArgs            = (action.extra.size() > 0);
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);

		if (hasArgs) {
			int         teamToControlId = -1;
			std::string aiShortName     = "";
			std::string aiVersion       = "";
			std::string aiName          = "";
			std::map<std::string, std::string> aiOptions;

			const std::vector<std::string> &args = _local_strSpaceTokenize(action.extra);
			if (args.size() >= 1) {
				teamToControlId = atoi(args[0].c_str());
			}
			if (args.size() >= 2) {
				aiShortName = args[1];
			} else {
				logOutput.Print("/%s: missing mandatory argument \"aiShortName\"", action.command.c_str());
			}
			if (args.size() >= 3) {
				aiVersion = args[2];
			}
			if (args.size() >= 4) {
				aiName = args[3];
			}

			CTeam* teamToControl = teamHandler->IsActiveTeam(teamToControlId) ?
			                             teamHandler->Team(teamToControlId) : NULL;

			if (teamToControl == NULL) {
				logOutput.Print("Team to control: not a valid team number: \"%s\"", args[0].c_str());
				badArgs = true;
			}
			if (!badArgs) {
				const bool weAreAllied  = teamHandler->AlliedTeams(fromTeamId, teamToControlId);
				const bool weAreLeader  = (teamToControl->leader == gu->myPlayerNum);
				const bool noLeader     = (teamToControl->leader == -1);
				if (!(weAreLeader || singlePlayer || (weAreAllied && (cheating || noLeader)))) {
					logOutput.Print("Team to control: player %s is not allowed to let a Skirmish AI take over control of team %i (try with /cheat)",
							fromPlayer->name.c_str(), teamToControlId);
					badArgs = true;
				}
			}
			if (!badArgs && teamToControl->isDead) {
				logOutput.Print("Team to control: is a dead team: %i", teamToControlId);
				badArgs = true;
			}
			// TODO: FIXME: remove this, if support for multiple Skirmish AIs per team is in place
			if (!badArgs && (skirmishAIHandler.GetSkirmishAIsInTeam(teamToControlId).size() > 0)) {
				logOutput.Print("Team to control: there is already an AI controlling this team: %i", teamToControlId);
				badArgs = true;
			}
			if (!badArgs && (skirmishAIHandler.GetLocalSkirmishAIInCreation(teamToControlId) != NULL)) {
				logOutput.Print("Team to control: there is already an AI beeing created for team: %i", teamToControlId);
				badArgs = true;
			}
			if (!badArgs) {
				const std::set<std::string>& luaAIImplShortNames = skirmishAIHandler.GetLuaAIImplShortNames();
				if (luaAIImplShortNames.find(aiShortName) != luaAIImplShortNames.end()) {
					logOutput.Print("Team to control: it is currently not supported to initialize Lua AIs mid-game");
					badArgs = true;
				}
			}

			if (!badArgs) {
				SkirmishAIKey aiKey(aiShortName, aiVersion);
				aiKey = aiLibManager->ResolveSkirmishAIKey(aiKey);
				if (aiKey.IsUnspecified()) {
					logOutput.Print("Skirmish AI: not a valid Skirmish AI: %s %s",
							aiShortName.c_str(), aiVersion.c_str());
					badArgs = true;
				} else {
					const CSkirmishAILibraryInfo* aiLibInfo = aiLibManager->GetSkirmishAIInfos().find(aiKey)->second;

					SkirmishAIData aiData;
					aiData.name       = (aiName != "") ? aiName : aiShortName;
					aiData.team       = teamToControlId;
					aiData.hostPlayer = gu->myPlayerNum;
					aiData.shortName  = aiShortName;
					aiData.version    = aiVersion;
					std::map<std::string, std::string>::const_iterator o;
					for (o = aiOptions.begin(); o != aiOptions.end(); ++o) {
						aiData.optionKeys.push_back(o->first);
					}
					aiData.options    = aiOptions;
					aiData.isLuaAI    = aiLibInfo->IsLuaAI();

					skirmishAIHandler.CreateLocalSkirmishAI(aiData);
				}
			}
		} else {
			logOutput.Print("/%s: missing mandatory arguments \"teamToControl\" and \"aiShortName\"", action.command.c_str());
			badArgs = true;
		}

		if (badArgs) {
			logOutput.Print("description: Let a Skirmish AI take over control of a team.");
			logOutput.Print("usage:   /%s teamToControl aiShortName [aiVersion] [name] [options...]", action.command.c_str());
			logOutput.Print("example: /%s 1 RAI 0.601 my_RAI_Friend difficulty=2 aggressiveness=3", action.command.c_str());
		}
	}
	else if (cmd == "ailist") {
		const CSkirmishAIHandler::id_ai_t& ais  = skirmishAIHandler.GetAllSkirmishAIs();
		if (ais.size() > 0) {
			CSkirmishAIHandler::id_ai_t::const_iterator ai;
			logOutput.Print("ID | Team | Local | Lua | Name | (Hosting player name) or (Short name & Version)");
			for (ai = ais.begin(); ai != ais.end(); ++ai) {
				const bool isLocal = (ai->second.hostPlayer == gu->myPlayerNum);
				std::string lastPart;
				if (isLocal) {
					lastPart = "(Key:)  " + ai->second.shortName + " " + ai->second.version;
				} else {
					lastPart = "(Host:) " + playerHandler->Player(gu->myPlayerNum)->name;
				}
				LogObject() << ai->first << " | " <<  ai->second.team << " | " << (isLocal ? "yes" : "no ") << " | " << (ai->second.isLuaAI ? "yes" : "no ") << " | " << ai->second.name << " | " << lastPart;
			}
		} else {
			logOutput.Print("<There are no active Skirmish AIs in this game>");
		}
	}
	else if (cmd == "team") {
		if (gs->cheatEnabled) {
			const int teamId = atoi(action.extra.c_str());
			if (teamHandler->IsValidTeam(teamId)) {
				net->Send(CBaseNetProtocol::Get().SendJoinTeam(gu->myPlayerNum, teamId));
			}
		}
	}
	else if (cmd == "spectator"){
		if (!gu->spectating)
			net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
	}
	else if ((cmd == "specteam") && gu->spectating) {
		const int teamId = atoi(action.extra.c_str());
		if (teamHandler->IsValidTeam(teamId)) {
			gu->myTeam = teamId;
			gu->myAllyTeam = teamHandler->AllyTeam(teamId);
		}
		CLuaUI::UpdateTeams();
	}
	else if ((cmd == "specfullview") && gu->spectating) {
		if (!action.extra.empty()) {
			const int mode = atoi(action.extra.c_str());
			gu->spectatingFullView   = !!(mode & 1);
			gu->spectatingFullSelect = !!(mode & 2);
		} else {
			gu->spectatingFullView = !gu->spectatingFullView;
			gu->spectatingFullSelect = gu->spectatingFullView;
		}
		CLuaUI::UpdateTeams();
	}
	else if (cmd == "ally" && !gu->spectating) {
		if (action.extra.size() > 0) {
			if (!gameSetup->fixedAllies) {
				std::istringstream is(action.extra);
				int otherAllyTeam = -1;
				is >> otherAllyTeam;
				int state = -1;
				is >> state;
				if (state >= 0 && state < 2 && otherAllyTeam >= 0 && otherAllyTeam != gu->myAllyTeam)
					net->Send(CBaseNetProtocol::Get().SendSetAllied(gu->myPlayerNum, otherAllyTeam, state));
				else
					logOutput.Print("/ally: wrong parameters (usage: /ally <other team> [0|1])");
			}
			else {
				logOutput.Print("No ingame alliances are allowed");
			}
		}
	}

	else if (cmd == "group") {
		const char* c = action.extra.c_str();
		const int t = c[0];
		if ((t >= '0') && (t <= '9')) {
			const int team = (t - '0');
			do { c++; } while ((c[0] != 0) && isspace(c[0]));
			grouphandlers[gu->myTeam]->GroupCommand(team, c);
		}
	}
	else if (cmd == "group0") {
		grouphandlers[gu->myTeam]->GroupCommand(0);
	}
	else if (cmd == "group1") {
		grouphandlers[gu->myTeam]->GroupCommand(1);
	}
	else if (cmd == "group2") {
		grouphandlers[gu->myTeam]->GroupCommand(2);
	}
	else if (cmd == "group3") {
		grouphandlers[gu->myTeam]->GroupCommand(3);
	}
	else if (cmd == "group4") {
		grouphandlers[gu->myTeam]->GroupCommand(4);
	}
	else if (cmd == "group5") {
		grouphandlers[gu->myTeam]->GroupCommand(5);
	}
	else if (cmd == "group6") {
		grouphandlers[gu->myTeam]->GroupCommand(6);
	}
	else if (cmd == "group7") {
		grouphandlers[gu->myTeam]->GroupCommand(7);
	}
	else if (cmd == "group8") {
		grouphandlers[gu->myTeam]->GroupCommand(8);
	}
	else if (cmd == "group9") {
		grouphandlers[gu->myTeam]->GroupCommand(9);
	}

	else if (cmd == "lastmsgpos") {
		// cycle through the positions
		camHandler->GetCurrentController().SetPos(infoConsole->GetMsgPos());
		camHandler->CameraTransition(0.6f);
	}
	else if (((cmd == "chat")     || (cmd == "chatall") ||
	         (cmd == "chatally") || (cmd == "chatspec")) &&
	         // if chat is bound to enter and we're waiting for user to press enter to start game, ignore.
				  (ks.Key() != SDLK_RETURN || playing || !keyInput->IsKeyPressed(SDLK_LCTRL))) {

		if (cmd == "chatall")  { userInputPrefix = ""; }
		if (cmd == "chatally") { userInputPrefix = "a:"; }
		if (cmd == "chatspec") { userInputPrefix = "s:"; }
		userWriting = true;
		userPrompt = "Say: ";
		userInput = userInputPrefix;
		writingPos = (int)userInput.length();
		chatting = true;

		if (ks.Key() != SDLK_RETURN) {
			ignoreNextChar = true;
		}

		consoleHistory->ResetPosition();
	}
	else if (cmd == "track") {
		unitTracker.Track();
	}
	else if (cmd == "trackoff") {
		unitTracker.Disable();
	}
	else if (cmd == "trackmode") {
		unitTracker.IncMode();
	}
#ifdef USE_GML
	else if (cmd == "showhealthbars") {
		if (action.extra.empty()) {
			unitDrawer->showHealthBars = !unitDrawer->showHealthBars;
		} else {
			unitDrawer->showHealthBars = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "showrezbars") {
		if (action.extra.empty()) {
			featureDrawer->SetShowRezBars(!featureDrawer->GetShowRezBars());
		} else {
			featureDrawer->SetShowRezBars(!!atoi(action.extra.c_str()));
		}
	}
#endif
	else if (cmd == "pause") {
		// disallow pausing prior to start of game proper
		if (playing) {
			bool newPause;
			if (action.extra.empty()) {
				newPause = !gs->paused;
			} else {
				newPause = !!atoi(action.extra.c_str());
			}
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, newPause));
			lastframe = SDL_GetTicks(); // this required here?
		}
	}
	else if (cmd == "debug") {
		if (globalRendering->drawdebug)
		{
			ProfileDrawer::Disable();
			globalRendering->drawdebug = false;
		}
		else
		{
			ProfileDrawer::Enable();
			globalRendering->drawdebug = true;
		}
	}
	else if (cmd == "nosound") {
		if (sound->Mute()) {
			logOutput.Print("Sound disabled");
		} else {
			logOutput.Print("Sound enabled");
		}
	}
	else if (cmd == "soundchannelenable") {
		std::string channel;
		int enableInt, enable;
		std::istringstream buf(action.extra);
		buf >> channel;
		buf >> enableInt;
		if (enableInt == 0)
			enable = false;
		else
			enable = true;

		if (channel == "UnitReply")
			Channels::UnitReply.Enable(enable);
		else if (channel == "General")
			Channels::General.Enable(enable);
		else if (channel == "Battle")
			Channels::Battle.Enable(enable);
		else if (channel == "UserInterface")
			Channels::UserInterface.Enable(enable);
		else if (channel == "Music")
			Channels::BGMusic.Enable(enable);
	}

	else if (cmd == "createvideo") {
		if (videoCapturing->IsCapturing()) {
			videoCapturing->StopCapturing();
		} else {
			videoCapturing->StartCapturing();
		}
	}
	else if (cmd == "drawtrees") {
		if (action.extra.empty()) {
			treeDrawer->drawTrees = !treeDrawer->drawTrees;
		} else {
			treeDrawer->drawTrees = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "dynamicsky") {
		if (action.extra.empty()) {
			sky->dynamicSky = !sky->dynamicSky;
		} else {
			sky->dynamicSky = !!atoi(action.extra.c_str());
		}
	}
#ifdef USE_GML
	else if (cmd == "multithreaddrawground") {
		if (action.extra.empty()) {
			gd->multiThreadDrawGround = !gd->multiThreadDrawGround;
		} else {
			gd->multiThreadDrawGround = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded ground rendering is %s", gd->multiThreadDrawGround?"enabled":"disabled");
	}
	else if (cmd == "multithreaddrawgroundshadow") {
		if (action.extra.empty()) {
			gd->multiThreadDrawGroundShadow = !gd->multiThreadDrawGroundShadow;
		} else {
			gd->multiThreadDrawGroundShadow = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded ground shadow rendering is %s", gd->multiThreadDrawGroundShadow?"enabled":"disabled");
	}
	else if (cmd == "multithreaddrawunit") {
		if (action.extra.empty()) {
			unitDrawer->multiThreadDrawUnit = !unitDrawer->multiThreadDrawUnit;
		} else {
			unitDrawer->multiThreadDrawUnit = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded unit rendering is %s", unitDrawer->multiThreadDrawUnit?"enabled":"disabled");
	}
	else if (cmd == "multithreaddrawunitshadow") {
		if (action.extra.empty()) {
			unitDrawer->multiThreadDrawUnitShadow = !unitDrawer->multiThreadDrawUnitShadow;
		} else {
			unitDrawer->multiThreadDrawUnitShadow = !!atoi(action.extra.c_str());
		}
		logOutput.Print("Multithreaded unit shadow rendering is %s", unitDrawer->multiThreadDrawUnitShadow?"enabled":"disabled");
	}
	else if (cmd == "multithread" || cmd == "multithreaddraw" || cmd == "multithreadsim") {
		const int mtenabled = gd->multiThreadDrawGround + unitDrawer->multiThreadDrawUnit + unitDrawer->multiThreadDrawUnitShadow > 1;
		if (cmd == "multithread" || cmd == "multithreaddraw") {
			if (action.extra.empty()) {
				gd->multiThreadDrawGround = !mtenabled;
				unitDrawer->multiThreadDrawUnit = !mtenabled;
				unitDrawer->multiThreadDrawUnitShadow = !mtenabled;
			} else {
				gd->multiThreadDrawGround = !!atoi(action.extra.c_str());
				unitDrawer->multiThreadDrawUnit = !!atoi(action.extra.c_str());
				unitDrawer->multiThreadDrawUnitShadow = !!atoi(action.extra.c_str());
			}
			if(!gd->multiThreadDrawGround)
				gd->multiThreadDrawGroundShadow=0;
			logOutput.Print("Multithreaded rendering is %s", gd->multiThreadDrawGround?"enabled":"disabled");
		}
#	if GML_ENABLE_SIM
		if (cmd == "multithread" || cmd == "multithreadsim") {
			extern volatile int gmlMultiThreadSim;
			extern volatile int gmlStartSim;
			if (action.extra.empty()) {
				gmlMultiThreadSim = (cmd == "multithread") ? !mtenabled : !gmlMultiThreadSim;
			} else {
				gmlMultiThreadSim = !!atoi(action.extra.c_str());
			}
			gmlStartSim=1;
			logOutput.Print("Simulation threading is %s", gmlMultiThreadSim?"enabled":"disabled");
		}
#	endif
	}
#endif
	else if (cmd == "speedcontrol") {
		if (action.extra.empty()) {
			++speedControl;
			if(speedControl > 2)
				speedControl = -2;
		}
		else {
			speedControl = atoi(action.extra.c_str());
		}
		speedControl = std::max(-2, std::min(speedControl, 2));
		net->Send(CBaseNetProtocol::Get().SendSpeedControl(gu->myPlayerNum, speedControl));
		logOutput.Print("Speed Control: %s%s",
			(speedControl == 0) ? "Default" : ((speedControl == 1 || speedControl == -1) ? "Average CPU" : "Maximum CPU"),
			(speedControl < 0) ? " (server voting disabled)" : "");
		configHandler->Set("SpeedControl", speedControl);
		if (gameServer)
			gameServer->UpdateSpeedControl(speedControl);
	}
	else if (!isRepeat && (cmd == "gameinfo")) {
		if (!CGameInfo::IsActive()) {
			CGameInfo::Enable();
		} else {
			CGameInfo::Disable();
		}
	}
	else if (cmd == "hideinterface") {
		if (action.extra.empty()) {
			hideInterface = !hideInterface;
		} else {
			hideInterface = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "hardwarecursor") {
		if (action.extra.empty()) {
			mouse->hardwareCursor = !mouse->hardwareCursor;
		} else {
			mouse->hardwareCursor = !!atoi(action.extra.c_str());
		}
		mouse->UpdateHwCursor();
		configHandler->Set("HardwareCursor", (int)mouse->hardwareCursor);
	}
	else if (cmd == "increaseviewradius") {
		gd->IncreaseDetail();
	}
	else if (cmd == "decreaseviewradius") {
		gd->DecreaseDetail();
	}
	else if (cmd == "moretrees") {
		treeDrawer->baseTreeDistance += 0.2f;
		LogObject() << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (cmd == "lesstrees") {
		treeDrawer->baseTreeDistance -= 0.2f;
		LogObject() << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
	else if (cmd == "moreclouds") {
		sky->cloudDensity*=0.95f;
		LogObject() << "Cloud density " << 1/sky->cloudDensity << "\n";
	}
	else if (cmd == "lessclouds") {
		sky->cloudDensity*=1.05f;
		LogObject() << "Cloud density " << 1/sky->cloudDensity << "\n";
	}

	// Break up the if/else chain to workaround MSVC compiler limit
	// "fatal error C1061: compiler limit : blocks nested too deeply"
	else { notfound1=true; }
	if (notfound1) { // BEGINN: MSVC limit workaround

	if (cmd == "speedup") {
		float speed = gs->userSpeedFactor;
		if(speed < 5) {
			speed += (speed < 2) ? 0.1f : 0.2f;
			float fpart = speed - (int)speed;
			if(fpart < 0.01f || fpart > 0.99f)
				speed = round(speed);
		} else if (speed < 10) {
			speed += 0.5f;
		} else {
			speed += 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
	}
	else if (cmd == "slowdown") {
		float speed = gs->userSpeedFactor;
		if (speed <= 5) {
			speed -= (speed <= 2) ? 0.1f : 0.2f;
			float fpart = speed - (int)speed;
			if(fpart < 0.01f || fpart > 0.99f)
				speed = round(speed);
			if (speed < 0.1f)
				speed = 0.1f;
		} else if (speed <= 10) {
			speed -= 0.5f;
		} else {
			speed -= 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
	}

	else if (cmd == "controlunit") {
		if (!gu->spectating) {
			Command c;
			c.id=CMD_STOP;
			c.options=0;
			selectedUnits.GiveCommand(c,false);		//force it to update selection and clear order que
			net->Send(CBaseNetProtocol::Get().SendDirectControl(gu->myPlayerNum));
		}
	}
	else if (cmd == "showstandard") {
		gd->DisableExtraTexture();
	}
	else if (cmd == "showelevation") {
		gd->SetHeightTexture();
	}
	else if (cmd == "toggleradarandjammer"){
		gd->ToggleRadarAndJammer();
	}
	else if (cmd == "showmetalmap") {
		gd->SetMetalTexture(readmap->metalMap);
	}

	else if (cmd == "showpathtraversability") {
		gd->TogglePathTraversabilityTexture();
	}
	else if (cmd == "showpathheat") {
		if (gs->cheatEnabled) {
			gd->TogglePathHeatTexture();
		}
	}
	else if (cmd == "showpathcost") {
		if (gs->cheatEnabled) {
			gd->TogglePathCostTexture();
		}
	}

	else if (cmd == "togglelos") {
		gd->ToggleLosTexture();
	}
	else if (cmd == "sharedialog") {
		if(!inputReceivers.empty() && dynamic_cast<CShareBox*>(inputReceivers.front())==0 && !gu->spectating)
			new CShareBox();
	}
	else if (cmd == "quitmessage") {
		if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == 0) {
			CKeyBindings::HotkeyList quitlist = keyBindings->GetHotkeys("quitmenu");
			std::string quitkey = quitlist.empty() ? "<none>" : quitlist.front();
			logOutput.Print(std::string("Press ") + quitkey + " to access the quit menu");
		}
	}
	else if (cmd == "quitmenu") {
		if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == 0)
			new CQuitBox();
	}
	else if (cmd == "quitforce" || cmd == "quit") {
		logOutput.Print("User exited");
		gu->globalQuit = true;
	}
	else if (cmd == "incguiopacity") {
		CInputReceiver::guiAlpha = std::min(CInputReceiver::guiAlpha+0.1f,1.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}
	else if (cmd == "decguiopacity") {
		CInputReceiver::guiAlpha = std::max(CInputReceiver::guiAlpha-0.1f,0.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}

	else if (cmd == "screenshot") {
		TakeScreenshot(action.extra);
	}

	else if (cmd == "grabinput") {
		SDL_GrabMode newMode;
		if (action.extra.empty()) {
			const SDL_GrabMode curMode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
			switch (curMode) {
				default: // make compiler happy
				case SDL_GRAB_OFF: newMode = SDL_GRAB_ON;  break;
				case SDL_GRAB_ON:  newMode = SDL_GRAB_OFF; break;
			}
		} else {
			if (atoi(action.extra.c_str())) {
				newMode = SDL_GRAB_ON;
			} else {
				newMode = SDL_GRAB_OFF;
			}
		}
		SDL_WM_GrabInput(newMode);
		logOutput.Print("Input grabbing %s",
		                (newMode == SDL_GRAB_ON) ? "enabled" : "disabled");
	}
	else if (cmd == "clock") {
		if (action.extra.empty()) {
			showClock = !showClock;
		} else {
			showClock = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowClock", showClock ? 1 : 0);
	}
	else if (cmd == "cross") {
		if (action.extra.empty()) {
			if (mouse->crossSize > 0.0f) {
				mouse->crossSize = -mouse->crossSize;
			} else {
				mouse->crossSize = std::max(1.0f, -mouse->crossSize);
			}
		} else {
			float size, alpha, scale;
			const char* args = action.extra.c_str();
			const int argcount = sscanf(args, "%f %f %f", &size, &alpha, &scale);
			if (argcount > 1) {
				mouse->crossAlpha = alpha;
				configHandler->Set("CrossAlpha", alpha);
			}
			if (argcount > 2) {
				mouse->crossMoveScale = scale;
				configHandler->Set("CrossMoveScale", scale);
			}
			mouse->crossSize = size;
		}
		configHandler->Set("CrossSize", mouse->crossSize);
	}
	else if (cmd == "fps") {
		if (action.extra.empty()) {
			showFPS = !showFPS;
		} else {
			showFPS = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowFPS", showFPS ? 1 : 0);
	}
	else if (cmd == "speed") {
		if (action.extra.empty()) {
			showSpeed = !showSpeed;
		} else {
			showSpeed = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowSpeed", showSpeed ? 1 : 0);
	}
	else if (cmd == "mtinfo") {
		if (action.extra.empty()) {
			showMTInfo = !showMTInfo;
		} else {
			showMTInfo = !!atoi(action.extra.c_str());
		}
		configHandler->Set("ShowMTInfo", showMTInfo ? 1 : 0);
	}
	else if (cmd == "teamhighlight") {
		if (action.extra.empty()) {
			gc->teamHighlight = abs(gc->teamHighlight + 1) % 3;
		} else {
			gc->teamHighlight = abs(atoi(action.extra.c_str())) % 3;
		}
		logOutput.Print("Team highlighting: %s", ((gc->teamHighlight == 1) ? "Players only" : ((gc->teamHighlight == 2) ? "Players and spectators" : "Disabled")));
		configHandler->Set("TeamHighlight", gc->teamHighlight);
	}
	else if (cmd == "info") {
		if (action.extra.empty()) {
			if (playerRoster.GetSortType() == PlayerRoster::Disabled) {
				playerRoster.SetSortTypeByCode(PlayerRoster::Allies);
			} else {
				playerRoster.SetSortTypeByCode(PlayerRoster::Disabled);
			}
		} else {
			playerRoster.SetSortTypeByName(action.extra);
		}
		if (playerRoster.GetSortType() != PlayerRoster::Disabled) {
			logOutput.Print("Sorting roster by %s", playerRoster.GetSortName());
		}
		configHandler->Set("ShowPlayerInfo", (int)playerRoster.GetSortType());
	}
	else if (cmd == "cmdcolors") {
		const string name = action.extra.empty() ? "cmdcolors.txt" : action.extra;
		cmdColors.LoadConfig(name);
		logOutput.Print("Reloaded cmdcolors with: " + name);
	}
	else if (cmd == "ctrlpanel") {
		const string name = action.extra.empty() ? "ctrlpanel.txt" : action.extra;
		guihandler->ReloadConfig(name);
		logOutput.Print("Reloaded ctrlpanel with: " + name);
	}
	else if (cmd == "font") {
		CglFont *newFont = NULL, *newSmallFont = NULL;
		try {
			const int fontSize = configHandler->Get("FontSize", 23);
			const int smallFontSize = configHandler->Get("SmallFontSize", 14);
			const int outlineWidth = configHandler->Get("FontOutlineWidth", 3);
			const float outlineWeight = configHandler->Get("FontOutlineWeight", 25.0f);
			const int smallOutlineWidth = configHandler->Get("SmallFontOutlineWidth", 2);
			const float smallOutlineWeight = configHandler->Get("SmallFontOutlineWeight", 10.0f);

			newFont = CglFont::LoadFont(action.extra, fontSize, outlineWidth, outlineWeight);
			newSmallFont = CglFont::LoadFont(action.extra, smallFontSize, smallOutlineWidth, smallOutlineWeight);
		} catch (std::exception e) {
			delete newFont;
			delete newSmallFont;
			newFont = newSmallFont = NULL;
			logOutput.Print(string("font error: ") + e.what());
		}
		if (newFont != NULL && newSmallFont != NULL) {
			delete font;
			delete smallFont;
			font = newFont;
			smallFont = newSmallFont;
			logOutput.Print("Loaded font: %s\n", action.extra.c_str());
			configHandler->SetString("FontFile", action.extra);
			configHandler->SetString("SmallFontFile", action.extra);
		}
	}
	else if (cmd == "vsync") {
		if (action.extra.empty()) {
			VSync.SetFrames((VSync.GetFrames() <= 0) ? 1 : 0);
		} else {
			VSync.SetFrames(atoi(action.extra.c_str()));
		}
	}
	else if (cmd == "safegl") {
		if (action.extra.empty()) {
			LuaOpenGL::SetSafeMode(!LuaOpenGL::GetSafeMode());
		} else {
			LuaOpenGL::SetSafeMode(!!atoi(action.extra.c_str()));
		}
	}
	else if (cmd == "resbar") {
		if (resourceBar) {
			if (action.extra.empty()) {
				resourceBar->disabled = !resourceBar->disabled;
			} else {
				resourceBar->disabled = !atoi(action.extra.c_str());
			}
		}
	}
	else if (cmd == "tooltip") {
		if (tooltip) {
			if (action.extra.empty()) {
				tooltip->disabled = !tooltip->disabled;
			} else {
				tooltip->disabled = !atoi(action.extra.c_str());
			}
		}
	}
	else if (cmd == "console") {
		if (infoConsole) {
			if (action.extra.empty()) {
				infoConsole->disabled = !infoConsole->disabled;
			} else {
				infoConsole->disabled = !atoi(action.extra.c_str());
			}
		}
	}
	else if (cmd == "endgraph") {
		if (action.extra.empty()) {
			CEndGameBox::disabled = !CEndGameBox::disabled;
		} else {
			CEndGameBox::disabled = !atoi(action.extra.c_str());
		}
	}
	else if (cmd == "fpshud") {
		if (action.extra.empty()) {
			hudDrawer->SetDraw(!hudDrawer->GetDraw());
		} else {
			hudDrawer->SetDraw(!!atoi(action.extra.c_str()));
		}
	}
	else if (cmd == "debugdrawai") {
		if (action.extra.empty()) {
			debugDrawerAI->SetDraw(!debugDrawerAI->GetDraw());
		} else {
			debugDrawerAI->SetDraw(!!atoi(action.extra.c_str()));
		}

		logOutput.Print("SkirmishAI debug drawing %s", (debugDrawerAI->GetDraw()? "enabled": "disabled"));
	}

	else if (cmd == "mapmarks") {
		if (action.extra.empty()) {
			globalRendering->drawMapMarks = !globalRendering->drawMapMarks;
		} else {
			globalRendering->drawMapMarks = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "allmapmarks") {
		if (gs->cheatEnabled) {
			if (action.extra.empty()) {
				inMapDrawer->ToggleAllVisible();
			} else {
				inMapDrawer->SetAllVisible(!!atoi(action.extra.c_str()));
			}
		}
	}
	else if (cmd == "noluadraw") {
		if (action.extra.empty()) {
			inMapDrawer->SetLuaMapDrawingAllowed(!inMapDrawer->GetLuaMapDrawingAllowed());
		} else {
			inMapDrawer->SetLuaMapDrawingAllowed(!!atoi(action.extra.c_str()));
		}
	}

	else if (cmd == "luaui") {
		if (guihandler != NULL) {
			guihandler->PushLayoutCommand(action.extra);
		}
	}
	else if (cmd == "luamoduictrl") {
		bool modUICtrl;
		if (action.extra.empty()) {
			modUICtrl = !CLuaHandle::GetModUICtrl();
		} else {
			modUICtrl = !!atoi(action.extra.c_str());
		}
		CLuaHandle::SetModUICtrl(modUICtrl);
		configHandler->Set("LuaModUICtrl", modUICtrl ? 1 : 0);
	}
	else if (cmd == "minimap") {
		if (minimap != NULL) {
			minimap->ConfigCommand(action.extra);
		}
	}
	else if (cmd == "grounddecals") {
		if (groundDecals) {
			if (action.extra.empty()) {
				groundDecals->SetDrawDecals(!groundDecals->GetDrawDecals());
			} else {
				groundDecals->SetDrawDecals(!!atoi(action.extra.c_str()));
			}
		}
		logOutput.Print("Ground decals are %s",
		                groundDecals->GetDrawDecals() ? "enabled" : "disabled");
	}
	else if (cmd == "maxparticles") {
		if (ph && !action.extra.empty()) {
			const int value = std::max(1, atoi(action.extra.c_str()));
			ph->SetMaxParticles(value);
			logOutput.Print("Set maximum particles to: %i", value);
		}
	}
	else if (cmd == "maxnanoparticles") {
		if (ph && !action.extra.empty()) {
			const int value = std::max(1, atoi(action.extra.c_str()));
			ph->SetMaxNanoParticles(value);
			logOutput.Print("Set maximum nano-particles to: %i", value);
		}
	}
	else if (cmd == "gathermode") {
		if (guihandler != NULL) {
			bool gatherMode;
			if (action.extra.empty()) {
				gatherMode = !guihandler->GetGatherMode();
			} else {
				gatherMode = !!atoi(action.extra.c_str());
			}
			guihandler->SetGatherMode(gatherMode);
			logOutput.Print("gathermode %s", gatherMode ? "enabled" : "disabled");
		}
	}
	else if (cmd == "pastetext") {
		if (userWriting){
			if (!action.extra.empty()) {
				userInput.insert(writingPos, action.extra);
				writingPos += action.extra.length();
			} else {
				PasteClipboard();
			}
			return 0;
		}
	}
	else if (cmd == "buffertext") {
		if (!action.extra.empty()) {
			consoleHistory->AddLine(action.extra);
		}
	}
	else if (cmd == "inputtextgeo") {
		if (!action.extra.empty()) {
			ParseInputTextGeometry(action.extra);
		}
	}
	else if (cmd == "disticon") {
		if (!action.extra.empty()) {
			const int iconDist = atoi(action.extra.c_str());
			unitDrawer->SetUnitIconDist((float)iconDist);
			configHandler->Set("UnitIconDist", iconDist);
			logOutput.Print("Set UnitIconDist to %i", iconDist);
		}
	}
	else if (cmd == "distdraw") {
		if (!action.extra.empty()) {
			const int drawDist = atoi(action.extra.c_str());
			unitDrawer->SetUnitDrawDist((float)drawDist);
			configHandler->Set("UnitLodDist", drawDist);
			logOutput.Print("Set UnitLodDist to %i", drawDist);
		}
	}
	else if (cmd == "lodscale") {
		if (!action.extra.empty()) {
			const vector<string> &args = CSimpleParser::Tokenize(action.extra, 0);
			if (args.size() == 1) {
				const float value = (float)atof(args[0].c_str());
				unitDrawer->LODScale = value;
			}
			else if (args.size() == 2) {
				const float value = (float)atof(args[1].c_str());
				if (args[0] == "shadow") {
					unitDrawer->LODScaleShadow = value;
				} else if (args[0] == "reflection") {
					unitDrawer->LODScaleReflection = value;
				} else if (args[0] == "refraction") {
					unitDrawer->LODScaleRefraction = value;
				}
			}
		}
	}
	else if (cmd == "wiremap") {
		if (action.extra.empty()) {
			gd->wireframe  = !gd->wireframe;
			sky->wireframe = gd->wireframe;
		} else {
			gd->wireframe  = !!atoi(action.extra.c_str());
			sky->wireframe = gd->wireframe;
		}
	}
	else if (cmd == "airmesh") {
		if (action.extra.empty()) {
			smoothGround->drawEnabled = !smoothGround->drawEnabled;
		} else {
			smoothGround->drawEnabled = !!atoi(action.extra.c_str());
		}
	}
	else if (cmd == "setgamma") {
		float r, g, b;
		const int count = sscanf(action.extra.c_str(), "%f %f %f", &r, &g, &b);
		if (count == 1) {
			SDL_SetGamma(r, r, r);
			logOutput.Print("Set gamma value");
		}
		else if (count == 3) {
			SDL_SetGamma(r, g, b);
			logOutput.Print("Set gamma values");
		}
		else {
			logOutput.Print("Unknown gamma format");
		}
	}
	else if (cmd == "crash" && gs->cheatEnabled) {
		int *a=0;
		*a=0;
	}
	else if (cmd == "exception" && gs->cheatEnabled) {
		throw std::runtime_error("Exception test");
	}
	else if (cmd == "divbyzero" && gs->cheatEnabled) {
		float a = 0;
		logOutput.Print("Result: %f", 1.0f/a);
	}
	else if (cmd == "give" && gs->cheatEnabled) {
		if (action.extra.find('@') == string::npos) {
			std::string msg = "give "; //FIXME lazyness
			msg += action.extra;
			float3 p;
			CInputReceiver* ir = NULL;
			if (!hideInterface)
				ir = CInputReceiver::GetReceiverAt(mouse->lastx, mouse->lasty);
			if (ir == minimap)
				p = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			else {
				const float3& pos = camera->pos;
				const float3& dir = mouse->dir;
				const float dist = ground->LineGroundCol(pos, pos + (dir * 30000.0f));
				p = pos + (dir * dist);
			}
			char buf[128];
			SNPRINTF(buf, sizeof(buf), " @%.0f,%.0f,%.0f", p.x, p.y, p.z);
			msg += buf;
			CommandMessage pckt(msg, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}
		else {
			CommandMessage pckt(action, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}
	}
	else if (cmd == "destroy" && gs->cheatEnabled) {
		// kill selected units
		std::stringstream ss;
		ss << "destroy";
		for (CUnitSet::iterator it = selectedUnits.selectedUnits.begin();
				it != selectedUnits.selectedUnits.end();
				++it) {
			ss << " " << (*it)->id;
		}
		CommandMessage pckt(ss.str(), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
	else if (cmd == "send") {
		CommandMessage pckt(Action(action.extra), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
	else if (cmd == "savegame"){
		SaveGame("Saves/QuickSave.ssf", true);
	}
	else if (cmd == "save") {// /save [-y ]<savename>
		bool saveoverride = action.extra.find("-y ") == 0;
		std::string savename(action.extra.c_str() + (saveoverride ? 3 : 0));
		savename = "Saves/" + savename + ".ssf";
		SaveGame(savename, saveoverride);
	}
	else if (cmd == "reloadgame") {
		ReloadGame();
	}
	else if (cmd == "debuginfo") {
		if (action.extra == "sound") {
			sound->PrintDebugInfo();
		} else if (action.extra == "profiling") {
			profiler.PrintProfilingInfo();
		}
	}
	else if (cmd == "benchmark-script") {
		CUnitScript::BenchmarkScript(action.extra);
	}
	else if (cmd == "atm" ||
#ifdef DEBUG
			cmd == "desync" ||
#endif
			cmd == "resync" ||
			cmd == "take" ||
			cmd == "luarules") {
		//these are synced commands, forward only
		CommandMessage pckt(action, gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
	else {
		static std::set<std::string> serverCommands = std::set<std::string>(commands, commands+numCommands);
		if (serverCommands.find(cmd) != serverCommands.end())
		{
			CommandMessage pckt(action, gu->myPlayerNum);
			net->Send(pckt.Pack());
		}

		if (!Console::Instance().ExecuteAction(action))
		{
			if (guihandler != NULL) // maybe a widget is interested?
				guihandler->PushLayoutCommand(action.rawline, false);
			return false;
		}
	}

	} // END: MSVC limit workaround

	return true;
}


