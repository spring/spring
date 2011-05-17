/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "SyncedGameCommands.h"
#include "Game.h"
#include "Action.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "InMapDraw.h"
#ifdef _WIN32
#  include "winerror.h"
#endif

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
#include "System/LogOutput.h"
#include "System/FileSystem/SimpleParser.h"

#include <string>
#include <vector>

static void SetBoolArg(bool& value, const std::string& str)
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



class CheatActionExecutor : public IActionExecutor {
public:
	CheatActionExecutor() : IActionExecutor("Cheat") {}

	void Execute(const std::string& args, int playerID) const {
		SetBoolArg(gs->cheatEnabled, args);
		if (gs->cheatEnabled)
			logOutput.Print("Cheating!");
		else
			logOutput.Print("No more cheating");
	}
};


class NoHelpActionExecutor : public IActionExecutor {
public:
	NoHelpActionExecutor() : IActionExecutor("NoHelp") {}

	void Execute(const std::string& args, int playerID) const {
		SetBoolArg(gs->noHelperAIs, args);
		selectedUnits.PossibleCommandChange(NULL);
		logOutput.Print("LuaUI control is %s", gs->noHelperAIs ? "disabled" : "enabled");
	}
};


class NoSpecDrawActionExecutor : public IActionExecutor {
public:
	NoSpecDrawActionExecutor() : IActionExecutor("NoSpecDraw") {}

	void Execute(const std::string& args, int playerID) const {
		bool buf;
		SetBoolArg(buf, args);
		inMapDrawer->SetSpecMapDrawingAllowed(buf);
	}
};


class GodModeActionExecutor : public IActionExecutor {
public:
	GodModeActionExecutor() : IActionExecutor("GodMode", true) {}

	void Execute(const std::string& args, int playerID) const {
		SetBoolArg(gs->godMode, args);
		CLuaUI::UpdateTeams();
		if (gs->godMode) {
			logOutput.Print("God Mode Enabled");
		} else {
			logOutput.Print("God Mode Disabled");
		}
		CPlayer::UpdateControlledTeams();
	}
};


class GlobalLosActionExecutor : public IActionExecutor {
public:
	GlobalLosActionExecutor() : IActionExecutor("GlobalLOS", true) {}

	void Execute(const std::string& args, int playerID) const {
		SetBoolArg(gs->globalLOS, args);
		if (gs->globalLOS) {
			logOutput.Print("Global LOS Enabled");
		} else {
			logOutput.Print("Global LOS Disabled");
		}
	}
};


class NoCostActionExecutor : public IActionExecutor {
public:
	NoCostActionExecutor() : IActionExecutor("NoCost", true) {}

	void Execute(const std::string& args, int playerID) const {
		if (unitDefHandler->ToggleNoCost()) {
			logOutput.Print("Everything is for free!");
		} else {
			logOutput.Print("Everything costs resources again!");
		}
	}
};


class GiveActionExecutor : public IActionExecutor {
public:
	GiveActionExecutor() : IActionExecutor("Give", true) {}

	void Execute(const std::string& args, int playerID) const {
		const std::vector<std::string>& parsedArgs = CSimpleParser::Tokenize(args, 0);
		unitLoader->ParseAndExecuteGiveUnitsCommand(parsedArgs, playerHandler->Player(playerID)->team);
	}
};


class DestroyActionExecutor : public IActionExecutor {
public:
	DestroyActionExecutor() : IActionExecutor("Destroy", true) {}

	void Execute(const std::string& args, int playerID) const {
		std::stringstream ss(args);
		logOutput.Print("Killing units: %s", args.c_str());

		do {
			unsigned int id; ss >> id;

			if (!ss)
				break;

			CUnit* u = uh->GetUnit(id);

			if (u != NULL)
				u->KillUnit(false, false, 0);
		} while (true);
	}
};


class NoSpectatorChatActionExecutor : public IActionExecutor {
public:
	NoSpectatorChatActionExecutor() : IActionExecutor("NoSpectatorChat") {}

	void Execute(const std::string& args, int playerID) const {
		SetBoolArg(game->noSpectatorChat, args);
		logOutput.Print("Spectators %s chat", game->noSpectatorChat ? "can not" : "can");
	}
};


class ReloadCobActionExecutor : public IActionExecutor {
public:
	ReloadCobActionExecutor() : IActionExecutor("ReloadCOB", true) {}

	void Execute(const std::string& args, int playerID) const {
		game->ReloadCOB(args, playerID);
	}
};


class ReloadCegsActionExecutor : public IActionExecutor {
public:
	ReloadCegsActionExecutor() : IActionExecutor("ReloadCegs", true) {}

	void Execute(const std::string& args, int playerID) const {
		game->ReloadCEGs(args);
	}
};


class DevLuaActionExecutor : public IActionExecutor {
public:
	DevLuaActionExecutor() : IActionExecutor("DevLua", true) {}

	void Execute(const std::string& args, int playerID) const {
		bool devMode = CLuaHandle::GetDevMode();
		SetBoolArg(devMode, args);
		CLuaHandle::SetDevMode(devMode);
		if (devMode) {
			logOutput.Print("Lua devmode enabled, this can cause desyncs");
		} else {
			logOutput.Print("Lua devmode disabled");
		}
	}
};


class EditDefsActionExecutor : public IActionExecutor {
public:
	EditDefsActionExecutor() : IActionExecutor("EditDefs", true) {}

	void Execute(const std::string& args, int playerID) const {
		SetBoolArg(gs->editDefsEnabled, args);
		if (gs->editDefsEnabled)
			logOutput.Print("Definition Editing!");
		else
			logOutput.Print("No definition Editing");
	}
};


class LuaRulesActionExecutor : public IActionExecutor {
public:
	LuaRulesActionExecutor() : IActionExecutor("LuaRules") {}

	void Execute(const std::string& args, int playerID) const {
		if (gs->frameNum > 1) {
			if ((args == "reload") && (playerID == 0)) {
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
			else if ((args == "disable") && (playerID == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts");
				} else {
					CLuaRules::FreeHandler();
					logOutput.Print("LuaRules disabled");
				}
			}
			else {
				if (luaRules) luaRules->GotChatMsg(args, playerID);
			}
		}
	}
};


class LuaGaiaActionExecutor : public IActionExecutor {
public:
	LuaGaiaActionExecutor() : IActionExecutor("LuaGaia") {}

	void Execute(const std::string& args, int playerID) const {
		if (gs->frameNum > 1) {
			if (gs->useLuaGaia) {
				if ((args == "reload") && (playerID == 0)) {
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
				else if ((args == "disable") && (playerID == 0)) {
					if (!gs->cheatEnabled) {
						logOutput.Print("Cheating required to disable synced scripts");
					} else {
						CLuaGaia::FreeHandler();
						logOutput.Print("LuaGaia disabled");
					}
				}
				else if (luaGaia) {
					luaGaia->GotChatMsg(args, playerID);
				}
				else {
					logOutput.Print("LuaGaia is not enabled");
				}
			}
		}
	}
};


#ifdef DEBUG
class DesyncActionExecutor : public IActionExecutor {
public:
	DesyncActionExecutor() : IActionExecutor("Desync", true) {}

	void Execute(const std::string& args, int playerID) const {
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123.0f);
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123);
		ASSERT_SYNCED_PRIMITIVE((short)(gu->myPlayerNum * 123 + 123));
		ASSERT_SYNCED_FLOAT3(float3(gu->myPlayerNum, gu->myPlayerNum, gu->myPlayerNum));

		for (size_t i = uh->MaxUnits() - 1; i >= 0; --i) {
			if (uh->units[i]) {
				if (playerID == gu->myPlayerNum) {
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
};
#endif // defined DEBUG


class AtmActionExecutor : public IActionExecutor {
public:
	AtmActionExecutor() : IActionExecutor("Atm", true) {}

	void Execute(const std::string& args, int playerID) const {
		const int team = playerHandler->Player(playerID)->team;
		teamHandler->Team(team)->AddMetal(1000);
		teamHandler->Team(team)->AddEnergy(1000);
	}
};


class TakeActionExecutor : public IActionExecutor {
public:
	TakeActionExecutor() : IActionExecutor("Take") {}

	void Execute(const std::string& args, int playerID) const {
		if (!playerHandler->Player(playerID)->spectator || gs->cheatEnabled) {
			const int sendTeam = playerHandler->Player(playerID)->team;
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
	}
};


class SkipActionExecutor : public IActionExecutor {
public:
	SkipActionExecutor() : IActionExecutor("Skip") {}

	void Execute(const std::string& args, int playerID) const {
		if (args.find_first_of("start") == 0) {
			std::istringstream buf(args.substr(6));
			int targetFrame;
			buf >> targetFrame;
			game->StartSkip(targetFrame);
		}
		else if (args == "end") {
			game->EndSkip();
		}
	}
};



void SyncedGameCommands::RegisterDefaultExecutors(CGame* game) {
	
	game->RegisterSyncedActionExecutor(new CheatActionExecutor());
	game->RegisterSyncedActionExecutor(new NoHelpActionExecutor());
	game->RegisterSyncedActionExecutor(new NoSpecDrawActionExecutor());
	game->RegisterSyncedActionExecutor(new GodModeActionExecutor());
	game->RegisterSyncedActionExecutor(new GlobalLosActionExecutor());
	game->RegisterSyncedActionExecutor(new NoCostActionExecutor());
	game->RegisterSyncedActionExecutor(new GiveActionExecutor());
	game->RegisterSyncedActionExecutor(new DestroyActionExecutor());
	game->RegisterSyncedActionExecutor(new NoSpectatorChatActionExecutor());
	game->RegisterSyncedActionExecutor(new ReloadCobActionExecutor());
	game->RegisterSyncedActionExecutor(new ReloadCegsActionExecutor());
	game->RegisterSyncedActionExecutor(new DevLuaActionExecutor());
	game->RegisterSyncedActionExecutor(new EditDefsActionExecutor());
	game->RegisterSyncedActionExecutor(new LuaRulesActionExecutor());
	game->RegisterSyncedActionExecutor(new LuaGaiaActionExecutor());
#ifdef DEBUG
	game->RegisterSyncedActionExecutor(new DesyncActionExecutor());
#endif // defined DEBUG
	game->RegisterSyncedActionExecutor(new AtmActionExecutor());
	game->RegisterSyncedActionExecutor(new TakeActionExecutor());
	game->RegisterSyncedActionExecutor(new SkipActionExecutor());
}
