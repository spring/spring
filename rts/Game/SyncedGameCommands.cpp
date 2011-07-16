/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include "SyncedGameCommands.h"
#include "Action.h"
#include "Game.h"
#include "GlobalUnsynced.h"
#include "InMapDraw.h"
#include "PlayerHandler.h"
#include "SelectedUnits.h"
#include "SyncedActionExecutor.h"
#ifdef _WIN32
#  include "winerror.h" // TODO someone on windows (MinGW? VS?) please check if this is required
#endif

#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Unit.h"
#include "UI/LuaUI.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/LogOutput.h"
#include "System/Util.h"

#include <string>
#include <vector>
#include <stdexcept>


namespace { // prevents linking problems in case of duplicate symbols

class CheatActionExecutor : public ISyncedActionExecutor {
public:
	CheatActionExecutor() : ISyncedActionExecutor("Cheat",
			"Enables/Disables cheating, which is required for a lot of other"
			" commands to be usable") {}

	void Execute(const SyncedAction& action) const {
		SetBoolArg(gs->cheatEnabled, action.GetArgs());
		LogSystemStatus("Cheating", gs->cheatEnabled);
	}
};


class NoHelpActionExecutor : public ISyncedActionExecutor {
public:
	NoHelpActionExecutor() : ISyncedActionExecutor("NoHelp",
			"Enables/Disables widgets (LuaUI control)") {}

	void Execute(const SyncedAction& action) const {
		SetBoolArg(gs->noHelperAIs, action.GetArgs());
		selectedUnits.PossibleCommandChange(NULL);
		LogSystemStatus("LuaUI control", gs->noHelperAIs);
	}
};


class NoSpecDrawActionExecutor : public ISyncedActionExecutor {
public:
	NoSpecDrawActionExecutor() : ISyncedActionExecutor("NoSpecDraw",
			"Allows/Disallows spectators to draw on the map") {}

	void Execute(const SyncedAction& action) const {
		bool buf;
		SetBoolArg(buf, action.GetArgs());
		inMapDrawer->SetSpecMapDrawingAllowed(buf);
	}
};


class GodModeActionExecutor : public ISyncedActionExecutor {
public:
	GodModeActionExecutor() : ISyncedActionExecutor("GodMode",
			"Enables/Disables god-mode, which allows all players"
			" (even spectators) to control all units", true) {}

	void Execute(const SyncedAction& action) const {
		SetBoolArg(gs->godMode, action.GetArgs());
		CLuaUI::UpdateTeams();
		LogSystemStatus("God-Mode", gs->godMode);
		CPlayer::UpdateControlledTeams();
	}
};


class GlobalLosActionExecutor : public ISyncedActionExecutor {
public:
	GlobalLosActionExecutor() : ISyncedActionExecutor("GlobalLOS",
			"Enables/Disables global line-of-sight, which makes the whole map"
			" permanently visible to everyone", true) {}

	void Execute(const SyncedAction& action) const {
		SetBoolArg(gs->globalLOS, action.GetArgs());
		LogSystemStatus("Global LOS", gs->globalLOS);
	}
};


class NoCostActionExecutor : public ISyncedActionExecutor {
public:
	NoCostActionExecutor() : ISyncedActionExecutor("NoCost",
			"Enables/Disables everything-for-free, which allows everyone"
			" to build everything for zero resource costs", true) {}

	void Execute(const SyncedAction& action) const {
		const bool isFree = unitDefHandler->ToggleNoCost();
		LogSystemStatus("Everything-for-free (no resource costs for building)",
				isFree);
	}
};


class GiveActionExecutor : public ISyncedActionExecutor {
public:
	GiveActionExecutor() : ISyncedActionExecutor("Give",
			"Places one or multiple units of a single or multiple types on the"
			" map, instantly; by default to your own team", true) {}

	void Execute(const SyncedAction& action) const {
		const std::vector<std::string>& parsedArgs = CSimpleParser::Tokenize(action.GetArgs(), 0);
		unitLoader->ParseAndExecuteGiveUnitsCommand(parsedArgs, playerHandler->Player(action.GetPlayerID())->team);
	}
};


class DestroyActionExecutor : public ISyncedActionExecutor {
public:
	DestroyActionExecutor() : ISyncedActionExecutor("Destroy",
			"Destroys one or multiple units by unit-ID, instantly", true) {}

	void Execute(const SyncedAction& action) const {
		std::stringstream argsStream(action.GetArgs());
		logOutput.Print("Killing units: %s", action.GetArgs().c_str());

		unsigned int unitId;
		do {
			argsStream >> unitId;

			if (!argsStream) {
				break;
			}

			CUnit* unit = uh->GetUnit(unitId);

			if (unit != NULL) {
				unit->KillUnit(false, false, 0);
			}
		} while (true);
	}
};


class NoSpectatorChatActionExecutor : public ISyncedActionExecutor {
public:
	NoSpectatorChatActionExecutor() : ISyncedActionExecutor("NoSpectatorChat",
			"Enables/Disables spectators to use the chat") {}

	void Execute(const SyncedAction& action) const {
		SetBoolArg(game->noSpectatorChat, action.GetArgs());
		LogSystemStatus("Spectators chat", !game->noSpectatorChat);
	}
};


class ReloadCobActionExecutor : public ISyncedActionExecutor {
public:
	ReloadCobActionExecutor() : ISyncedActionExecutor("ReloadCOB",
			"Reloads COB scripts", true) {}

	void Execute(const SyncedAction& action) const {
		game->ReloadCOB(action.GetArgs(), action.GetPlayerID());
	}
};


class ReloadCegsActionExecutor : public ISyncedActionExecutor {
public:
	ReloadCegsActionExecutor() : ISyncedActionExecutor("ReloadCegs",
			"Reloads Ceg scripts", true) {}

	void Execute(const SyncedAction& action) const {
		game->ReloadCEGs(action.GetArgs());
	}
};


class DevLuaActionExecutor : public ISyncedActionExecutor {
public:
	DevLuaActionExecutor() : ISyncedActionExecutor("DevLua",
			"Enables/Disables Lua dev-mode (can cause desyncs if enabled)",
			true) {}

	void Execute(const SyncedAction& action) const {
		bool devMode = CLuaHandle::GetDevMode();
		SetBoolArg(devMode, action.GetArgs());
		CLuaHandle::SetDevMode(devMode);
		LogSystemStatus("Lua dev-mode (can cause desyncs if enabled)", devMode);
	}
};


class EditDefsActionExecutor : public ISyncedActionExecutor {
public:
	EditDefsActionExecutor() : ISyncedActionExecutor("EditDefs",
			"Allows/Disallows editing of unit-, feature- and weapon-defs"
			" through Lua", true) {}

	void Execute(const SyncedAction& action) const {
		SetBoolArg(gs->editDefsEnabled, action.GetArgs());
		LogSystemStatus("Unit-, Feature- & Weapon-Def editing",
				gs->editDefsEnabled);
	}
};


class LuaRulesActionExecutor : public ISyncedActionExecutor {
public:
	LuaRulesActionExecutor() : ISyncedActionExecutor("LuaRules",
			"Allows to reload or disable Lua-rules, or alternatively to send"
			" a chat message to Lua-rules") {}

	void Execute(const SyncedAction& action) const {
		if (gs->frameNum > 1) {
			if ((action.GetArgs() == "reload") && (action.GetPlayerID() == 0)) {
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
			} else if ((action.GetArgs() == "disable") && (action.GetPlayerID() == 0)) {
				if (!gs->cheatEnabled) {
					logOutput.Print("Cheating required to disable synced scripts");
				} else {
					CLuaRules::FreeHandler();
					logOutput.Print("LuaRules disabled");
				}
			} else {
				if (luaRules) luaRules->GotChatMsg(action.GetArgs(), action.GetPlayerID());
			}
		}
	}
};


class LuaGaiaActionExecutor : public ISyncedActionExecutor {
public:
	LuaGaiaActionExecutor() : ISyncedActionExecutor("LuaGaia",
			"Allows to reload or disable Lua-Gaia, or alternatively to send"
			" a chat message to Lua-Gaia") {}

	void Execute(const SyncedAction& action) const {
		if (gs->frameNum > 1) {
			if (gs->useLuaGaia) {
				if ((action.GetArgs() == "reload") && (action.GetPlayerID() == 0)) {
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
				} else if ((action.GetArgs() == "disable") && (action.GetPlayerID() == 0)) {
					if (!gs->cheatEnabled) {
						logOutput.Print("Cheating required to disable synced scripts");
					} else {
						CLuaGaia::FreeHandler();
						logOutput.Print("LuaGaia disabled");
					}
				} else if (luaGaia) {
					luaGaia->GotChatMsg(action.GetArgs(), action.GetPlayerID());
				} else {
					logOutput.Print("LuaGaia is not enabled");
				}
			}
		}
	}
};


#ifdef DEBUG
class DesyncActionExecutor : public ISyncedActionExecutor {
public:
	DesyncActionExecutor() : ISyncedActionExecutor("Desync",
			"Allows to create an artificial desync of the local client with"
			" the rest of the participating hosts", true) {}

	void Execute(const SyncedAction& action) const {
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123.0f);
		ASSERT_SYNCED_PRIMITIVE(gu->myPlayerNum * 123);
		ASSERT_SYNCED_PRIMITIVE((short)(gu->myPlayerNum * 123 + 123));
		ASSERT_SYNCED_FLOAT3(float3(gu->myPlayerNum, gu->myPlayerNum, gu->myPlayerNum));

		for (size_t i = uh->MaxUnits() - 1; i >= 0; --i) {
			if (uh->units[i]) {
				if (action.GetPlayerID() == gu->myPlayerNum) {
					++uh->units[i]->midPos.x; // and desync...
					++uh->units[i]->midPos.x;
				} else {
					// execute the same amount of flops on any other player,
					// but do not desync (it is a NOP)
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


class AtmActionExecutor : public ISyncedActionExecutor {
public:
	AtmActionExecutor() : ISyncedActionExecutor("Atm",
			"Gives 1000 metal and 1000 energy to the issuing players team",
			true) {}

	void Execute(const SyncedAction& action) const {
		const int team = playerHandler->Player(action.GetPlayerID())->team;
		teamHandler->Team(team)->AddMetal(1000);
		teamHandler->Team(team)->AddEnergy(1000);
	}
};


class TakeActionExecutor : public ISyncedActionExecutor {
public:
	TakeActionExecutor() : ISyncedActionExecutor("Take",
			"Transfers all units of allied teams without active controller to"
			" the issuing players team") {}

	void Execute(const SyncedAction& action) const {
		if (!playerHandler->Player(action.GetPlayerID())->spectator || gs->cheatEnabled) {
			const int sendTeam = playerHandler->Player(action.GetPlayerID())->team;
			for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
				if (teamHandler->AlliedTeams(a, sendTeam)) {
					bool hasPlayer = false;
					for (int b = 0; b < playerHandler->ActivePlayers(); ++b) {
						if (playerHandler->Player(b)->active
								&& (playerHandler->Player(b)->team == a)
								&& !playerHandler->Player(b)->spectator)
						{
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


class SkipActionExecutor : public ISyncedActionExecutor {
public:
	SkipActionExecutor() : ISyncedActionExecutor("Skip",
			"Fast-forwards to a given frame, or stops fast-forwarding") {}

	void Execute(const SyncedAction& action) const {
		if (action.GetArgs().find_first_of("start") == 0) {
			std::istringstream buf(action.GetArgs().substr(6));
			int targetFrame;
			buf >> targetFrame;
			game->StartSkip(targetFrame);
		}
		else if (action.GetArgs() == "end") {
			game->EndSkip();
		}
	}
};

} // namespace (unnamed)




void SyncedGameCommands::AddDefaultActionExecutors() {
	
	AddActionExecutor(new CheatActionExecutor());
	AddActionExecutor(new NoHelpActionExecutor());
	AddActionExecutor(new NoSpecDrawActionExecutor());
	AddActionExecutor(new GodModeActionExecutor());
	AddActionExecutor(new GlobalLosActionExecutor());
	AddActionExecutor(new NoCostActionExecutor());
	AddActionExecutor(new GiveActionExecutor());
	AddActionExecutor(new DestroyActionExecutor());
	AddActionExecutor(new NoSpectatorChatActionExecutor());
	AddActionExecutor(new ReloadCobActionExecutor());
	AddActionExecutor(new ReloadCegsActionExecutor());
	AddActionExecutor(new DevLuaActionExecutor());
	AddActionExecutor(new EditDefsActionExecutor());
	AddActionExecutor(new LuaRulesActionExecutor());
	AddActionExecutor(new LuaGaiaActionExecutor());
#ifdef DEBUG
	AddActionExecutor(new DesyncActionExecutor());
#endif // defined DEBUG
	AddActionExecutor(new AtmActionExecutor());
	AddActionExecutor(new TakeActionExecutor());
	AddActionExecutor(new SkipActionExecutor());
}


SyncedGameCommands* SyncedGameCommands::singleton = NULL;

void SyncedGameCommands::CreateInstance() {

	if (singleton == NULL) {
		singleton = new SyncedGameCommands();
	} else {
		throw std::logic_error("SyncedGameCommands singleton is already initialized");
	}
}

void SyncedGameCommands::DestroyInstance() {

	if (singleton != NULL) {
		// SafeDelete
		SyncedGameCommands* tmp = singleton;
		singleton = NULL;
		delete tmp;
	} else {
		// this might happen during shutdown after an unclean init
		logOutput.Print("Warning: SyncedGameCommands singleton was not initialized or is already destroyed");
	}
}
