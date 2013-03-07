/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SyncedGameCommands.h"

#include "Action.h"
#include "Game.h"
#include "GlobalUnsynced.h"
#include "InMapDraw.h"
#include "Player.h"
#include "PlayerHandler.h"
#include "SelectedUnits.h"
#include "SyncedActionExecutor.h"
#ifdef _WIN32
#  include "winerror.h" // TODO someone on windows (MinGW? VS?) please check if this is required
#endif

#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUI.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Unit.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Log/ILog.h"
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

	bool Execute(const SyncedAction& action) const {
		SetBoolArg(gs->cheatEnabled, action.GetArgs());
		LogSystemStatus("Cheating", gs->cheatEnabled);
		return true;
	}
};


class NoHelpActionExecutor : public ISyncedActionExecutor {
public:
	NoHelpActionExecutor() : ISyncedActionExecutor("NoHelp",
			"Enables/Disables widgets (LuaUI control)") {}

	bool Execute(const SyncedAction& action) const {
		SetBoolArg(gs->noHelperAIs, action.GetArgs());
		selectedUnits.PossibleCommandChange(NULL);
		LogSystemStatus("LuaUI control", gs->noHelperAIs);
		return true;
	}
};


class NoSpecDrawActionExecutor : public ISyncedActionExecutor {
public:
	NoSpecDrawActionExecutor() : ISyncedActionExecutor("NoSpecDraw",
			"Allows/Disallows spectators to draw on the map") {}

	bool Execute(const SyncedAction& action) const {
		bool buf;
		SetBoolArg(buf, action.GetArgs());
		inMapDrawer->SetSpecMapDrawingAllowed(buf);
		return true;
	}
};


class GodModeActionExecutor : public ISyncedActionExecutor {
public:
	GodModeActionExecutor() : ISyncedActionExecutor("GodMode",
			"Enables/Disables god-mode, which allows all players "
			"(even spectators) to control all units (even during "
			"replays, which will DESYNC them)", true) {}

	bool Execute(const SyncedAction& action) const {
		SetBoolArg(gs->godMode, action.GetArgs());
		CLuaUI::UpdateTeams();
		LogSystemStatus("God-Mode", gs->godMode);
		CPlayer::UpdateControlledTeams();
		return true;
	}
};


class GlobalLosActionExecutor : public ISyncedActionExecutor {
public:
	GlobalLosActionExecutor() : ISyncedActionExecutor("GlobalLOS",
			"Enables/Disables global line-of-sight, which makes the whole map"
			" permanently visible to everyone or to a specific allyteam", true) {}

	bool Execute(const SyncedAction& action) const {
		const std::string& args = action.GetArgs();
		const unsigned int argAllyTeam = atoi(args.c_str());
		const unsigned int maxAllyTeam = teamHandler->ActiveAllyTeams();
		// const unsigned int maxAllyTeams = sizeof(gs->globalLOS) / sizeof(gs->globalLOS[0]);

		if (args.empty()) {
			for (unsigned int n = 0; n < maxAllyTeam; n++) {
				gs->globalLOS[n] = !gs->globalLOS[n];
			}

			LOG("[GlobalLosActionExecutor] global LOS toggled for all allyteams");
			return true;
		}
		if (argAllyTeam < maxAllyTeam) {
			gs->globalLOS[argAllyTeam] = !gs->globalLOS[argAllyTeam];

			LOG("[GlobalLosActionExecutor] global LOS toggled for allyteam %u", argAllyTeam);
			return true;
		}

		LOG("[GlobalLosActionExecutor] bad allyteam %u", argAllyTeam);
		return false;
	}
};


class NoCostActionExecutor : public ISyncedActionExecutor {
public:
	NoCostActionExecutor() : ISyncedActionExecutor("NoCost",
			"Enables/Disables everything-for-free, which allows everyone"
			" to build everything for zero resource costs", true) {}

	bool Execute(const SyncedAction& action) const {
		const bool isFree = unitDefHandler->ToggleNoCost();
		LogSystemStatus("Everything-for-free (no resource costs for building)",
				isFree);
		return true;
	}
};


class GiveActionExecutor : public ISyncedActionExecutor {
public:
	GiveActionExecutor() : ISyncedActionExecutor("Give",
			"Places one or multiple units of a single or multiple types on the"
			" map, instantly; by default to your own team", true) {}

	bool Execute(const SyncedAction& action) const {
		const std::vector<std::string>& parsedArgs = CSimpleParser::Tokenize(action.GetArgs(), 0);
		unitLoader->ParseAndExecuteGiveUnitsCommand(parsedArgs, playerHandler->Player(action.GetPlayerID())->team);
		return true;
	}
};


class DestroyActionExecutor : public ISyncedActionExecutor {
public:
	DestroyActionExecutor() : ISyncedActionExecutor("Destroy",
			"Destroys one or multiple units by unit-ID, instantly", true) {}

	bool Execute(const SyncedAction& action) const {
		std::stringstream argsStream(action.GetArgs());
		LOG("Killing units: %s", action.GetArgs().c_str());

		unsigned int unitId;
		do {
			argsStream >> unitId;

			if (!argsStream) {
				break;
			}

			CUnit* unit = unitHandler->GetUnit(unitId);

			if (unit != NULL) {
				unit->KillUnit(false, false, 0);
			} else {
				LOG("Wrong unitID: %i", unitId);
			}
		} while (true);

		return true;
	}
};


class NoSpectatorChatActionExecutor : public ISyncedActionExecutor {
public:
	NoSpectatorChatActionExecutor() : ISyncedActionExecutor("NoSpectatorChat",
			"Enables/Disables spectators to use the chat") {}

	bool Execute(const SyncedAction& action) const {
		SetBoolArg(game->noSpectatorChat, action.GetArgs());
		LogSystemStatus("Spectators chat", !game->noSpectatorChat);
		return true;
	}
};


class ReloadCobActionExecutor : public ISyncedActionExecutor {
public:
	ReloadCobActionExecutor() : ISyncedActionExecutor("ReloadCOB",
			"Reloads COB scripts", true) {}

	bool Execute(const SyncedAction& action) const {
		game->ReloadCOB(action.GetArgs(), action.GetPlayerID());
		return true;
	}
};


class ReloadCegsActionExecutor : public ISyncedActionExecutor {
public:
	ReloadCegsActionExecutor() : ISyncedActionExecutor("ReloadCEGs",
			"Reloads CEG scripts", true) {}

	bool Execute(const SyncedAction& action) const {
		explGenHandler->ReloadGenerators(action.GetArgs());
		return true;
	}
};


class DevLuaActionExecutor : public ISyncedActionExecutor {
public:
	DevLuaActionExecutor() : ISyncedActionExecutor("DevLua",
			"Enables/Disables Lua dev-mode (can cause desyncs if enabled)",
			true) {}

	bool Execute(const SyncedAction& action) const {
		bool devMode = CLuaHandle::GetDevMode();
		SetBoolArg(devMode, action.GetArgs());
		CLuaHandle::SetDevMode(devMode);
		LogSystemStatus("Lua dev-mode (can cause desyncs if enabled)", devMode);
		return true;
	}
};


class EditDefsActionExecutor : public ISyncedActionExecutor {
public:
	EditDefsActionExecutor() : ISyncedActionExecutor("EditDefs",
			"Allows/Disallows editing of unit-, feature- and weapon-defs"
			" through Lua", true) {}

	bool Execute(const SyncedAction& action) const {
		SetBoolArg(gs->editDefsEnabled, action.GetArgs());
		LogSystemStatus("Unit-, Feature- & Weapon-Def editing",
				gs->editDefsEnabled);
		return true;
	}
};


class LuaRulesActionExecutor : public ISyncedActionExecutor {
public:
	LuaRulesActionExecutor() : ISyncedActionExecutor("LuaRules",
			"Allows one to reload or disable Lua-rules, or alternatively to send"
			" a chat message to Lua-rules") {}

	bool Execute(const SyncedAction& action) const {
		if (gs->frameNum <= 1) {
			// TODO still needed???
			LOG_L(L_WARNING, "/%s: cannot be called before gameframe #1", GetCommand().c_str());
			return false;
		}

		// NOTE:
		//     previously only the host player (ID == 0) was allowed to issue these actions
		//     prior to some server changes they worked even in demos with that restriction,
		//     but this is no longer the case so we now let any player execute them (for MP
		//     it does not matter who does so since they are not meant to be used there ITFP
		//     and no less sync-safe)
		if (action.GetArgs() == "reload") {
			if (!gs->cheatEnabled) {
				LOG_L(L_WARNING, "Cheating required to reload synced scripts");
			} else {

				GML_MSTMUTEX_DOUNLOCK(sim); // temporarily unlock this mutex to prevent a deadlock
				{
					GML_STDMUTEX_LOCK(draw); // the draw thread accesses luaRules in too many places, so we lock the entire draw thread

					CLuaRules::FreeHandler();
					CLuaRules::LoadHandler();
				}
				GML_MSTMUTEX_DOLOCK(sim); // restore unlocked mutex

				if (luaRules) {
					LOG("LuaRules reloaded");
				} else {
					LOG_L(L_ERROR, "LuaRules reload failed");
				}
			}
		} else if (action.GetArgs() == "disable") {
			if (!gs->cheatEnabled) {
				LOG_L(L_WARNING, "Cheating required to disable synced scripts");
			} else {
				GML_MSTMUTEX_DOUNLOCK(sim); // temporarily unlock this mutex to prevent a deadlock
				{
					GML_STDMUTEX_LOCK(draw); // the draw thread accesses luaRules in too many places, so we lock the entire draw thread

					CLuaRules::FreeHandler();
				}
				GML_MSTMUTEX_DOLOCK(sim); // restore unlocked mutex

				LOG("LuaRules disabled");
			}
		} else if (luaRules) {
			luaRules->GotChatMsg(action.GetArgs(), action.GetPlayerID());
		}

		return true;
	}
};


class LuaGaiaActionExecutor : public ISyncedActionExecutor {
public:
	LuaGaiaActionExecutor() : ISyncedActionExecutor("LuaGaia",
			"Allows one to reload or disable Lua-Gaia, or alternatively to send"
			" a chat message to Lua-Gaia") {}

	bool Execute(const SyncedAction& action) const {
		if (gs->frameNum <= 1) {
			// TODO still needed???
			LOG_L(L_WARNING, "/%s: cannot be called before gameframe #1", GetCommand().c_str());
			return false;
		}

		if (!gs->useLuaGaia) {
			return false;
		}

		if (action.GetArgs() == "reload") {
			if (!gs->cheatEnabled) {
				LOG_L(L_WARNING, "Cheating required to reload synced scripts");
			} else {
				CLuaGaia::FreeHandler();
				CLuaGaia::LoadHandler();

				if (luaGaia) {
					LOG("LuaGaia reloaded");
				} else {
					LOG_L(L_ERROR, "LuaGaia reload failed");
				}
			}
		} else if (action.GetArgs() == "disable") {
			if (!gs->cheatEnabled) {
				LOG_L(L_WARNING, "Cheating required to disable synced scripts");
			} else {
				CLuaGaia::FreeHandler();
				LOG("LuaGaia disabled");
			}
		} else if (luaGaia) {
			luaGaia->GotChatMsg(action.GetArgs(), action.GetPlayerID());
		} else {
			LOG("LuaGaia disabled");
		}

		return true;
	}
};


#ifdef DEBUG
class DesyncActionExecutor : public ISyncedActionExecutor {
public:
	DesyncActionExecutor() : ISyncedActionExecutor("Desync",
			"Allows one to create an artificial desync of the local client with"
			" the rest of the participating hosts", true) {}

	bool Execute(const SyncedAction& action) const {
		ASSERT_SYNCED(gu->myPlayerNum * 123.0f);
		ASSERT_SYNCED(gu->myPlayerNum * 123);
		ASSERT_SYNCED((short)(gu->myPlayerNum * 123 + 123));
		//ASSERT_SYNCED(float3(gu->myPlayerNum, gu->myPlayerNum, gu->myPlayerNum));

		for (size_t i = unitHandler->MaxUnits() - 1; i >= 0; --i) {
			if (unitHandler->units[i]) {
				if (action.GetPlayerID() == gu->myPlayerNum) {
					++unitHandler->units[i]->midPos.x; // and desync...
					++unitHandler->units[i]->midPos.x;
				} else {
					// execute the same amount of flops on any other player,
					// but do not desync (it is a NOP)
					++unitHandler->units[i]->midPos.x;
					--unitHandler->units[i]->midPos.x;
				}
				break;
			}
		}
		LOG_L(L_ERROR, "Desyncing in frame %d.", gs->frameNum);
		return true;
	}
};
#endif // defined DEBUG


class AtmActionExecutor : public ISyncedActionExecutor {
public:
	AtmActionExecutor() : ISyncedActionExecutor("Atm",
			"Gives 1000 metal and 1000 energy to the issuing players team",
			true) {}

	bool Execute(const SyncedAction& action) const {
		const int team = playerHandler->Player(action.GetPlayerID())->team;
		teamHandler->Team(team)->AddMetal(1000);
		teamHandler->Team(team)->AddEnergy(1000);
		return true;
	}
};


class TakeActionExecutor : public ISyncedActionExecutor {
public:
	TakeActionExecutor() : ISyncedActionExecutor("Take",
			"Transfers all units of allied teams without any "
			"active players to the team of the issuing player") {}

	bool Execute(const SyncedAction& action) const {
		const CPlayer* actionPlayer = playerHandler->Player(action.GetPlayerID());

		if (actionPlayer->spectator && !gs->cheatEnabled) {
			return false;
		}

		if (!game->playing) {
			return true;
		}

		for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
			if (!teamHandler->AlliedTeams(a, actionPlayer->team)) {
				continue;
			}

			bool hasPlayer = false;

			for (int b = 0; b < playerHandler->ActivePlayers(); ++b) {
				const CPlayer* teamPlayer = playerHandler->Player(b);

				if (!teamPlayer->active) { continue; }
				if (teamPlayer->spectator) { continue; }
				if (teamPlayer->team != a) { continue; }

				hasPlayer = true;
				break;
			}

			if (!hasPlayer) {
				teamHandler->Team(a)->GiveEverythingTo(actionPlayer->team);
			}
		}

		return true;
	}
};


class SkipActionExecutor : public ISyncedActionExecutor {
public:
	SkipActionExecutor() : ISyncedActionExecutor("Skip",
			"Fast-forwards to a given frame, or stops fast-forwarding") {}

	bool Execute(const SyncedAction& action) const {
		if (action.GetArgs().find_first_of("start") == 0) {
			std::istringstream buf(action.GetArgs().substr(6));
			int targetFrame;
			buf >> targetFrame;
			game->StartSkip(targetFrame);
		}
		else if (action.GetArgs() == "end") {
			game->EndSkip();
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
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
		LOG_L(L_WARNING, "SyncedGameCommands singleton was not initialized or is already destroyed");
	}
}
