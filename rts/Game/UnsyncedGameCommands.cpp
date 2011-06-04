/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <SDL_events.h>

#include "UnsyncedGameCommands.h"
#include "UnsyncedActionExecutor.h"
#include "Game.h"
#include "Action.h"
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
#include "WordCompletion.h"
#include "InMapDraw.h"
#include "InMapDrawModel.h"
#ifdef _WIN32
#  include "winerror.h" // TODO someone on windows (MinGW? VS?) please check if this is required
#endif
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/glFont.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/Screenshot.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/VerticalSync.h"
#include "Lua/LuaOpenGL.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "UI/CommandColors.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/InputReceiver.h"
#include "UI/KeyBindings.h"
#include "UI/LuaUI.h"
#include "UI/MiniMap.h"
#include "UI/QuitBox.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/ShareBox.h"
#include "UI/TooltipConsole.h"
#include "UI/UnitTracker.h"
#include "UI/ProfileDrawer.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/NetProtocol.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/Util.h"


UnsyncedGameCommands::~UnsyncedGameCommands() {
	RemoveAllActionExecutors();
}


static std::vector<std::string> _local_strSpaceTokenize(const std::string& text) {
	static const char* const SPACE_DELIMS = " \t";

	std::vector<std::string> tokens;

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

	return tokens;
}



// TODO CGame stuff in UnsyncedGameCommands: refactor (or move)
bool CGame::ProcessCommandText(unsigned int key, const std::string& command) {
	if (command.size() <= 2)
		return false;

	if ((command[0] == '/') && (command[1] != '/')) {
		const string actionLine = command.substr(1); // strip the '/'

		Action fakeAction(actionLine);
		ActionPressed(key, fakeAction, false);
		return true;
	}

	return false;
}

// TODO CGame stuff in UnsyncedGameCommands: refactor (or move)
bool CGame::ProcessKeyPressAction(unsigned int key, const Action& action) {
	if (action.command == "edit_return") {
		userWriting = false;
		writingPos = 0;

		if (key == SDLK_RETURN) {
			// prevent game start when host enters a chat message
			keyInput->SetKeyState(key, 0);
		}
		if (chatting) {
			string command;

			if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
				command = userInput.substr(2);
			} else {
				command = userInput;
			}

			if (ProcessCommandText(key, command)) {
				// execute an action
				consoleHistory->AddLine(command);
				logOutput.Print(command);

				chatting = false;
				userInput = "";
				writingPos = 0;
			}
		}
		return true;
	}
	else if ((action.command == "edit_escape") && (chatting || inMapDrawer->IsWantLabel())) {
		if (chatting) {
			consoleHistory->AddLine(userInput);
		}
		userWriting = false;
		chatting = false;
		inMapDrawer->SetWantLabel(false);
		userInput = "";
		writingPos = 0;
		return true;
	}
	else if (action.command == "edit_complete") {
		string head = userInput.substr(0, writingPos);
		string tail = userInput.substr(writingPos);
		const vector<string> &partials = wordCompletion->Complete(head);
		userInput = head + tail;
		writingPos = (int)head.length();

		if (!partials.empty()) {
			string msg;
			for (unsigned int i = 0; i < partials.size(); i++) {
				msg += "  ";
				msg += partials[i];
			}
			logOutput.Print(msg);
		}
		return true;
	}
	else if (action.command == "chatswitchall") {
		if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
			userInput = userInput.substr(2);
			writingPos = std::max(0, writingPos - 2);
		}

		userInputPrefix = "";
		return true;
	}
	else if (action.command == "chatswitchally") {
		if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
			userInput[0] = 'a';
		} else {
			userInput = "a:" + userInput;
			writingPos += 2;
		}

		userInputPrefix = "a:";
		return true;
	}
	else if (action.command == "chatswitchspec") {
		if ((userInput.find_first_of("aAsS") == 0) && (userInput[1] == ':')) {
			userInput[0] = 's';
		} else {
			userInput = "s:" + userInput;
			writingPos += 2;
		}

		userInputPrefix = "s:";
		return true;
	}
	else if (action.command == "pastetext") {
		if (!action.extra.empty()) {
			userInput.insert(writingPos, action.extra);
			writingPos += action.extra.length();
		} else {
			PasteClipboard();
		}
		return true;
	}

	else if (action.command == "edit_backspace") {
		if (!userInput.empty() && (writingPos > 0)) {
			userInput.erase(writingPos - 1, 1);
			writingPos--;
		}
		return true;
	}
	else if (action.command == "edit_delete") {
		if (!userInput.empty() && (writingPos < (int)userInput.size())) {
			userInput.erase(writingPos, 1);
		}
		return true;
	}
	else if (action.command == "edit_home") {
		writingPos = 0;
		return true;
	}
	else if (action.command == "edit_end") {
		writingPos = (int)userInput.length();
		return true;
	}
	else if (action.command == "edit_prev_char") {
		writingPos = std::max(0, std::min((int)userInput.length(), writingPos - 1));
		return true;
	}
	else if (action.command == "edit_next_char") {
		writingPos = std::max(0, std::min((int)userInput.length(), writingPos + 1));
		return true;
	}
	else if (action.command == "edit_prev_word") {
		// prev word
		const char* s = userInput.c_str();
		int p = writingPos;
		while ((p > 0) && !isalnum(s[p - 1])) { p--; }
		while ((p > 0) &&  isalnum(s[p - 1])) { p--; }
		writingPos = p;
		return true;
	}
	else if (action.command == "edit_next_word") {
		const int len = (int)userInput.length();
		const char* s = userInput.c_str();
		int p = writingPos;
		while ((p < len) && !isalnum(s[p])) { p++; }
		while ((p < len) &&  isalnum(s[p])) { p++; }
		writingPos = p;
		return true;
	}
	else if ((action.command == "edit_prev_line") && chatting) {
		userInput = consoleHistory->PrevLine(userInput);
		writingPos = (int)userInput.length();
		return true;
	}
	else if ((action.command == "edit_next_line") && chatting) {
		userInput = consoleHistory->NextLine(userInput);
		writingPos = (int)userInput.length();
		return true;
	}

	return false;
}



#if       GML_ENABLE_SIM
extern volatile int gmlMultiThreadSim;
extern volatile int gmlStartSim;
#endif // GML_ENABLE_SIM

namespace unsyncedActionExecutors {

/* XXX
 * An alternative way of dealing with the commands.
 * Less overall code, but more ugly.
 * In an extension not shown in this code, this could also be used to
 * assemble a list of commands to register, by undefining and redefining a list
 * object.
 */
/*
#define ActExSpecial(CLS_NAME, command) \
	class CLS_NAME##ActionExecutor : public IUnsyncedActionExecutor { \
	public: \
		CLS_NAME##ActionExecutor() : IUnsyncedActionExecutor(command) {} \
		void Execute(const UnsyncedAction& action) const {

#define ActEx(CLS_NAME) \
	ActExSpecial(CLS_NAME, #CLS_NAME)

#define ActExClose() \
		} \
	};


ActEx(Select)
		selectionKeys->DoSelection(action.GetArgs());
ActExClose()

ActExSpecial(SetOverlay, "TSet")
		selectionKeys->DoSelection(action.GetArgs());
ActExClose()
*/


/**
 * Special case executor which is used for creating aliases to other commands.
 * The inner executor will be delet'ed in this executors dtor.
 */
class AliasActionExecutor : public IUnsyncedActionExecutor {
public:
	AliasActionExecutor(IUnsyncedActionExecutor* innerExecutor, const std::string& commandAlias)
		: IUnsyncedActionExecutor(commandAlias)
		, innerExecutor(innerExecutor)
	{
		assert(innerExecutor != NULL);
	}
	virtual ~AliasActionExecutor() {
		delete innerExecutor;
	}

	void Execute(const UnsyncedAction& action) const {
		innerExecutor->ExecuteAction(action);
	}

private:
	IUnsyncedActionExecutor* innerExecutor;
};



/**
 * Special case executor which allows to combine multiple commands into one,
 * by calling them sequentially.
 * The inner executors will be delet'ed in this executors dtor.
 */
class SequentialActionExecutor : public IUnsyncedActionExecutor {
public:
	SequentialActionExecutor(const std::string& command)
		: IUnsyncedActionExecutor(command)
	{}
	virtual ~SequentialActionExecutor() {

		std::vector<IUnsyncedActionExecutor*>::iterator ei;
		for (ei = innerExecutors.begin(); ei != innerExecutors.end(); ++ei) {
			delete *ei;
		}
	}

	void AddExecutor(IUnsyncedActionExecutor* innerExecutor) {
		innerExecutors.push_back(innerExecutor);
	}

	void Execute(const UnsyncedAction& action) const {

		std::vector<IUnsyncedActionExecutor*>::const_iterator ei;
		for (ei = innerExecutors.begin(); ei != innerExecutors.end(); ++ei) {
			(*ei)->ExecuteAction(action);
		}
	}

private:
	std::vector<IUnsyncedActionExecutor*> innerExecutors;
};



class SelectActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectActionExecutor() : IUnsyncedActionExecutor("Select") {}

	void Execute(const UnsyncedAction& action) const {
		selectionKeys->DoSelection(action.GetArgs());
	}
};



class SelectUnitsActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectUnitsActionExecutor() : IUnsyncedActionExecutor("SelectUnits") {}

	void Execute(const UnsyncedAction& action) const {
		game->SelectUnits(action.GetArgs());
	}
};



class SelectCycleActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectCycleActionExecutor() : IUnsyncedActionExecutor("SelectCycle") {}

	void Execute(const UnsyncedAction& action) const {
		game->SelectCycle(action.GetArgs());
	}
};



class DeselectActionExecutor : public IUnsyncedActionExecutor {
public:
	DeselectActionExecutor() : IUnsyncedActionExecutor("Deselect") {}

	void Execute(const UnsyncedAction& action) const {
		selectedUnits.ClearSelected();
	}
};



class ShadowsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShadowsActionExecutor() : IUnsyncedActionExecutor("Shadows") {}

	void Execute(const UnsyncedAction& action) const {
		const int current = configHandler->Get("Shadows", 0);
		if (current < 0) {
			logOutput.Print("Shadows have been disabled with %i", current);
			logOutput.Print("Change your configuration and restart to use them");
			return;
		}
		else if (!shadowHandler->shadowsSupported) {
			logOutput.Print("Your hardware/driver setup does not support shadows");
			return;
		}

		delete shadowHandler; // XXX use SafeDelete() ?
		int next = 0;
		if (!action.GetArgs().empty()) {
			int mapsize = 2048;
			const char* args = action.GetArgs().c_str();
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
};



class WaterActionExecutor : public IUnsyncedActionExecutor {
public:
	WaterActionExecutor() : IUnsyncedActionExecutor("Water") {}

	void Execute(const UnsyncedAction& action) const {
		int nextWaterRendererMode = 0;

		if (!action.GetArgs().empty()) {
			nextWaterRendererMode = std::max(0, atoi(action.GetArgs().c_str()) % CBaseWater::NUM_WATER_RENDERERS);
		} else {
			nextWaterRendererMode = -1;
		}

		CBaseWater::PushWaterMode(nextWaterRendererMode);
	}
};



class AdvModelShadingActionExecutor : public IUnsyncedActionExecutor {
public:
	AdvModelShadingActionExecutor() : IUnsyncedActionExecutor("AdvModelShading") {}

	void Execute(const UnsyncedAction& action) const {
		static bool canUseShaders = unitDrawer->advShading;

		if (canUseShaders) {
			if (!action.GetArgs().empty()) {
				unitDrawer->advShading = !!atoi(action.GetArgs().c_str());
			} else {
				unitDrawer->advShading = !unitDrawer->advShading;
			}

			logOutput.Print("model shaders %sabled", (unitDrawer->advShading? "en": "dis"));
		}
	}
};



class AdvMapShadingActionExecutor : public IUnsyncedActionExecutor {
public:
	AdvMapShadingActionExecutor() : IUnsyncedActionExecutor("AdvMapShading") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		static bool canUseShaders = gd->advShading;

		if (canUseShaders) {
			if (!action.GetArgs().empty()) {
				gd->advShading = !!atoi(action.GetArgs().c_str());
			} else {
				gd->advShading = !gd->advShading;
			}

			logOutput.Print("map shaders %sabled", (gd->advShading? "en": "dis"));
		}
	}
};



class SayActionExecutor : public IUnsyncedActionExecutor {
public:
	SayActionExecutor() : IUnsyncedActionExecutor("Say") {}

	void Execute(const UnsyncedAction& action) const {
		game->SendNetChat(action.GetArgs());
	}
};



class SayPrivateActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateActionExecutor() : IUnsyncedActionExecutor("W") {}

	void Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const int playernum = playerHandler->Player(action.GetArgs().substr(0, pos));
			if (playernum >= 0) {
				game->SendNetChat(action.GetArgs().substr(pos+1), playernum);
			} else {
				logOutput.Print("Player not found: %s", action.GetArgs().substr(0, pos).c_str());
			}
		}
	}
};



class SayPrivateByPlayerIDActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateByPlayerIDActionExecutor() : IUnsyncedActionExecutor("WByNum") {}

	void Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			std::istringstream buf(action.GetArgs().substr(0, pos));
			int playernum;
			buf >> playernum;
			if (playernum >= 0) {
				game->SendNetChat(action.GetArgs().substr(pos+1), playernum);
			} else {
				logOutput.Print("Player-ID invalid: %i", playernum);
			}
		}
	}
};



class EchoActionExecutor : public IUnsyncedActionExecutor {
public:
	EchoActionExecutor() : IUnsyncedActionExecutor("Echo") {}

	void Execute(const UnsyncedAction& action) const {
		logOutput.Print(action.GetArgs());
	}
};



class SetActionExecutor : public IUnsyncedActionExecutor {
public:
	SetActionExecutor() : IUnsyncedActionExecutor("Set") {}

	void Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.GetArgs().substr(0, pos);
			configHandler->SetString(varName, action.GetArgs().substr(pos+1));
		}
	}
};



class SetOverlayActionExecutor : public IUnsyncedActionExecutor {
public:
	SetOverlayActionExecutor() : IUnsyncedActionExecutor("TSet") {}

	void Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.GetArgs().substr(0, pos);
			configHandler->SetOverlay(varName, action.GetArgs().substr(pos+1));
		}
	}
};



class EnableDrawInMapActionExecutor : public IUnsyncedActionExecutor {
public:
	EnableDrawInMapActionExecutor() : IUnsyncedActionExecutor("DrawInMap") {}

	void Execute(const UnsyncedAction& action) const {
		inMapDrawer->SetDrawMode(true);
	}
};



class DrawLabelActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawLabelActionExecutor() : IUnsyncedActionExecutor("DrawLabel") {}

	void Execute(const UnsyncedAction& action) const {
		float3 pos = inMapDrawer->GetMouseMapPos();
		if (pos.x >= 0) {
			inMapDrawer->SetDrawMode(false);
			inMapDrawer->PromptLabel(pos);
			if ((action.GetKey() >= SDLK_SPACE) && (action.GetKey() <= SDLK_DELETE)) {
				game->ignoreNextChar=true;
			}
		}
	}
};



class MouseActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseActionExecutor(int button)
		: IUnsyncedActionExecutor("Mouse" + IntToString(button))
		, button(button)
	{}

	void Execute(const UnsyncedAction& action) const {
		if (!action.IsRepeat()) {
			mouse->MousePress(mouse->lastx, mouse->lasty, button);
		}
	}

private:
	int button;
};



class ViewSelectionActionExecutor : public IUnsyncedActionExecutor {
public:
	ViewSelectionActionExecutor() : IUnsyncedActionExecutor("ViewSelection") {}

	void Execute(const UnsyncedAction& action) const {
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
};



class CameraMoveActionExecutor : public IUnsyncedActionExecutor {
public:
	CameraMoveActionExecutor(int dimension, const std::string& commandPostfix)
		: IUnsyncedActionExecutor("Move" + commandPostfix)
		, dimension(dimension)
	{}

	void Execute(const UnsyncedAction& action) const {
		game->camMove[dimension] = true;
	}

private:
	int dimension;
};



class AIKillReloadActionExecutor : public IUnsyncedActionExecutor {
public:
	/**
	 * @param kill whether this executor should function as the kill-
	 * or the reload-AI command
	 */
	AIKillReloadActionExecutor(bool kill)
		: IUnsyncedActionExecutor(kill ? "AIKill" : "AIReload")
		, kill(kill)
	{}

	void Execute(const UnsyncedAction& action) const {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool hasArgs            = (action.GetArgs().size() > 0);
		const std::vector<std::string> &args = _local_strSpaceTokenize(action.GetArgs());
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);
		const std::string actionName  = StringToLower(GetCommand()).substr(2);

		if (hasArgs) {
			size_t skirmishAIId           = 0; // will only be used if !badArgs
			bool share = false;
			int teamToKillId         = -1;
			int teamToReceiveUnitsId = -1;

			if (args.size() >= 1) {
				teamToKillId = atoi(args[0].c_str());
			}
			if ((args.size() >= 2) && kill) {
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
				if (!skirmishAIIds.empty()) {
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
				if (kill) {
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
				} else {
					// reload
					net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_RELOADING));
				}

				logOutput.Print("Skirmish AI controlling team %i is beeing %sed ...", teamToKillId, actionName.c_str());
			}
		} else {
			logOutput.Print("/%s: missing mandatory argument \"teamTo%s\"", GetCommand().c_str(), actionName.c_str());
			badArgs = true;
		}

		if (badArgs) {
			if (kill) {
				logOutput.Print("description: "
				                "Kill a Skirmish AI controlling a team. The team itsself will remain alive, "
				                "unless a second argument is given, which specifies an active team "
				                "that will receive all the units of the AI team.");
				logOutput.Print("usage:   /%s teamToKill [teamToReceiveUnits]", GetCommand().c_str());
			} else {
				// reload
				logOutput.Print("description: "
				                "Reload a Skirmish AI controlling a team."
				                "The team itsself will remain alive during the process.");
				logOutput.Print("usage:   /%s teamToReload", GetCommand().c_str());
			}
		}
	}

private:
	bool kill;
};



class AIControlActionExecutor : public IUnsyncedActionExecutor {
public:
	AIControlActionExecutor() : IUnsyncedActionExecutor("AIControl") {}

	void Execute(const UnsyncedAction& action) const {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool hasArgs            = (action.GetArgs().size() > 0);
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);

		if (hasArgs) {
			int         teamToControlId = -1;
			std::string aiShortName     = "";
			std::string aiVersion       = "";
			std::string aiName          = "";
			std::map<std::string, std::string> aiOptions;

			const std::vector<std::string> &args = _local_strSpaceTokenize(action.GetArgs());
			if (args.size() >= 1) {
				teamToControlId = atoi(args[0].c_str());
			}
			if (args.size() >= 2) {
				aiShortName = args[1];
			} else {
				logOutput.Print("/%s: missing mandatory argument \"aiShortName\"", GetCommand().c_str());
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
			logOutput.Print("/%s: missing mandatory arguments \"teamToControl\" and \"aiShortName\"", GetCommand().c_str());
			badArgs = true;
		}

		if (badArgs) {
			logOutput.Print("description: Let a Skirmish AI take over control of a team.");
			logOutput.Print("usage:   /%s teamToControl aiShortName [aiVersion] [name] [options...]", GetCommand().c_str());
			logOutput.Print("example: /%s 1 RAI 0.601 my_RAI_Friend difficulty=2 aggressiveness=3", GetCommand().c_str());
		}
	}
};



class AIListActionExecutor : public IUnsyncedActionExecutor {
public:
	AIListActionExecutor() : IUnsyncedActionExecutor("AIList") {}

	void Execute(const UnsyncedAction& action) const {
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
};



class TeamActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamActionExecutor() : IUnsyncedActionExecutor("Team", true) {}

	void Execute(const UnsyncedAction& action) const {
		const int teamId = atoi(action.GetArgs().c_str());
		if (teamHandler->IsValidTeam(teamId)) {
			net->Send(CBaseNetProtocol::Get().SendJoinTeam(gu->myPlayerNum, teamId));
		}
	}
};



class SpectatorActionExecutor : public IUnsyncedActionExecutor {
public:
	SpectatorActionExecutor() : IUnsyncedActionExecutor("Spectator") {}

	void Execute(const UnsyncedAction& action) const {
		if (!gu->spectating) {
			net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
		}
	}
};



class SpecTeamActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecTeamActionExecutor() : IUnsyncedActionExecutor("SpecTeam") {}

	void Execute(const UnsyncedAction& action) const {
		if (gu->spectating) {
			const int teamId = atoi(action.GetArgs().c_str());
			if (teamHandler->IsValidTeam(teamId)) {
				gu->myTeam = teamId;
				gu->myAllyTeam = teamHandler->AllyTeam(teamId);
			}
			CLuaUI::UpdateTeams();
		}
	}
};



class SpecFullViewActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecFullViewActionExecutor() : IUnsyncedActionExecutor("SpecFullView") {}

	void Execute(const UnsyncedAction& action) const {
		if (gu->spectating) {
			if (!action.GetArgs().empty()) {
				const int mode = atoi(action.GetArgs().c_str());
				gu->spectatingFullView   = !!(mode & 1);
				gu->spectatingFullSelect = !!(mode & 2);
			} else {
				gu->spectatingFullView = !gu->spectatingFullView;
				gu->spectatingFullSelect = gu->spectatingFullView;
			}
			CLuaUI::UpdateTeams();
		}
	}
};



class AllyActionExecutor : public IUnsyncedActionExecutor {
public:
	AllyActionExecutor() : IUnsyncedActionExecutor("Ally") {}

	void Execute(const UnsyncedAction& action) const {
		if (!gu->spectating) {
			if (action.GetArgs().size() > 0) {
				if (!gameSetup->fixedAllies) {
					std::istringstream is(action.GetArgs());
					int otherAllyTeam = -1;
					is >> otherAllyTeam;
					int state = -1;
					is >> state;
					if (state >= 0 && state < 2 && otherAllyTeam >= 0 && otherAllyTeam != gu->myAllyTeam)
						net->Send(CBaseNetProtocol::Get().SendSetAllied(gu->myPlayerNum, otherAllyTeam, state));
					else
						logOutput.Print("/%s: wrong parameters (usage: /%s <other team> [0|1])", GetCommand().c_str(), GetCommand().c_str());
				}
				else {
					logOutput.Print("No in-game alliances are allowed");
				}
			}
		}
	}
};



class GroupActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupActionExecutor() : IUnsyncedActionExecutor("Group") {}

	void Execute(const UnsyncedAction& action) const {
		// XXX ug.. code, at least document
		const char* c = action.GetArgs().c_str();
		const int t = c[0];
		if ((t >= '0') && (t <= '9')) {
			const int team = (t - '0');
			do { c++; } while ((c[0] != 0) && isspace(c[0]));
			grouphandlers[gu->myTeam]->GroupCommand(team, c);
		}
	}
};



class GroupIDActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupIDActionExecutor(int groupId)
		: IUnsyncedActionExecutor("Group" + IntToString(groupId))
		, groupId(groupId)
	{}

	void Execute(const UnsyncedAction& action) const {
		if (!action.IsRepeat()) {
		grouphandlers[gu->myTeam]->GroupCommand(groupId);
		}
	}

private:
	int groupId;
};



class LastMessagePositionActionExecutor : public IUnsyncedActionExecutor {
public:
	LastMessagePositionActionExecutor() : IUnsyncedActionExecutor("LastMsgPos") {}

	void Execute(const UnsyncedAction& action) const {
		// cycle through the positions
		camHandler->GetCurrentController().SetPos(game->infoConsole->GetMsgPos());
		camHandler->CameraTransition(0.6f);
	}
};



class ChatActionExecutor : public IUnsyncedActionExecutor {

	ChatActionExecutor(const std::string& command, const std::string& userInputPrefix, bool setUserInputPrefix)
		: IUnsyncedActionExecutor(command)
		, userInputPrefix(userInputPrefix)
		, setUserInputPrefix(setUserInputPrefix)
	{}

public:
	void Execute(const UnsyncedAction& action) const {
		// if chat is bound to enter and we're waiting for user to press enter to start game, ignore.
		if (action.GetKey() != SDLK_RETURN || game->playing || !keyInput->IsKeyPressed(SDLK_LCTRL)) {
			if (setUserInputPrefix) {
				game->userInputPrefix = userInputPrefix;
			}
			game->userWriting = true;
			game->userPrompt = "Say: ";
			game->userInput = game->userInputPrefix;
			game->writingPos = (int)game->userInput.length();
			game->chatting = true;

			if (action.GetKey() != SDLK_RETURN) {
				game->ignoreNextChar = true;
			}

			game->consoleHistory->ResetPosition();
		}
	}

	static void RegisterCommandVariants() {
		using namespace unsyncedActionExecutors;

		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("Chat",     "",   false));
		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("ChatAll",  "",   true));
		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("ChatAlly", "a:", true));
		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("ChatSpec", "s:", true));
	}

private:
	const std::string userInputPrefix;
	const bool setUserInputPrefix;
};



// TODO merge together with "TrackOff" to "Track 0|1", and deprecate the two old ones
class TrackActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackActionExecutor() : IUnsyncedActionExecutor("Track") {}

	void Execute(const UnsyncedAction& action) const {
		unitTracker.Track();
	}
};



class TrackOffActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackOffActionExecutor() : IUnsyncedActionExecutor("TrackOff") {}

	void Execute(const UnsyncedAction& action) const {
		unitTracker.Disable();
	}
};



class TrackModeActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackModeActionExecutor() : IUnsyncedActionExecutor("TrackMode") {}

	void Execute(const UnsyncedAction& action) const {
		unitTracker.IncMode();
	}
};



#ifdef USE_GML
class ShowHealthBarsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowHealthBarsActionExecutor() : IUnsyncedActionExecutor("ShowHealthBars") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			unitDrawer->showHealthBars = !unitDrawer->showHealthBars;
		} else {
			unitDrawer->showHealthBars = !!atoi(action.GetArgs().c_str());
		}
	}
};



class ShowRezurectionBarsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowRezurectionBarsActionExecutor() : IUnsyncedActionExecutor("ShowRezBars") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			featureDrawer->SetShowRezBars(!featureDrawer->GetShowRezBars());
		} else {
			featureDrawer->SetShowRezBars(!!atoi(action.GetArgs().c_str()));
		}
	}
};
#endif // USE_GML



class PauseActionExecutor : public IUnsyncedActionExecutor {
public:
	PauseActionExecutor() : IUnsyncedActionExecutor("Pause") {}

	void Execute(const UnsyncedAction& action) const {
		// disallow pausing prior to start of game proper
		if (game->playing) {
			bool newPause;
			if (action.GetArgs().empty()) {
				newPause = !gs->paused;
			} else {
				newPause = !!atoi(action.GetArgs().c_str());
			}
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, newPause));
			game->lastframe = SDL_GetTicks(); // this required here?
		}
	}

};



class DebugActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugActionExecutor() : IUnsyncedActionExecutor("Debug") {}

	void Execute(const UnsyncedAction& action) const {
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
};



// XXX unlucky name; maybe make this "Sound {0|1}" instead (bool arg or toggle)
class NoSoundActionExecutor : public IUnsyncedActionExecutor {
public:
	NoSoundActionExecutor() : IUnsyncedActionExecutor("NoSound") {}

	void Execute(const UnsyncedAction& action) const {
		if (sound->Mute()) {
			logOutput.Print("Sound disabled");
		} else {
			logOutput.Print("Sound enabled");
		}
	}
};



class SoundChannelEnableActionExecutor : public IUnsyncedActionExecutor {
public:
	SoundChannelEnableActionExecutor() : IUnsyncedActionExecutor("SoundChannelEnable") {}

	void Execute(const UnsyncedAction& action) const {
		std::string channel;
		int enableInt, enable;
		std::istringstream buf(action.GetArgs());
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
};



class CreateVideoActionExecutor : public IUnsyncedActionExecutor {
public:
	CreateVideoActionExecutor() : IUnsyncedActionExecutor("CreateVideo") {}

	void Execute(const UnsyncedAction& action) const {
		if (videoCapturing->IsCapturing()) {
			videoCapturing->StopCapturing();
		} else {
			videoCapturing->StartCapturing();
		}
	}
};



class DrawTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawTreesActionExecutor() : IUnsyncedActionExecutor("DrawTrees") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			treeDrawer->drawTrees = !treeDrawer->drawTrees;
		} else {
			treeDrawer->drawTrees = !!atoi(action.GetArgs().c_str());
		}
	}
};



class DynamicSkyActionExecutor : public IUnsyncedActionExecutor {
public:
	DynamicSkyActionExecutor() : IUnsyncedActionExecutor("DynamicSky") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			sky->dynamicSky = !sky->dynamicSky;
		} else {
			sky->dynamicSky = !!atoi(action.GetArgs().c_str());
		}
	}
};



class DynamicSunActionExecutor : public IUnsyncedActionExecutor {
public:
	DynamicSunActionExecutor() : IUnsyncedActionExecutor("DynamicSun") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			sky->SetLight(!sky->GetLight()->IsDynamic());
		} else {
			sky->SetLight(!!atoi(action.GetArgs().c_str()));
		}
	}
};



#ifdef USE_GML
class MultiThreadDrawGroundActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawGroundActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawGround") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		if (action.GetArgs().empty()) {
			gd->multiThreadDrawGround = !gd->multiThreadDrawGround;
		} else {
			gd->multiThreadDrawGround = !!atoi(action.GetArgs().c_str());
		}
		logOutput.Print("Multi threaded ground rendering is %s", gd->multiThreadDrawGround?"enabled":"disabled");
	}
};



class MultiThreadDrawGroundShadowActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawGroundShadowActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawGroundShadow") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		if (action.GetArgs().empty()) {
			gd->multiThreadDrawGroundShadow = !gd->multiThreadDrawGroundShadow;
		} else {
			gd->multiThreadDrawGroundShadow = !!atoi(action.GetArgs().c_str());
		}
		logOutput.Print("Multi threaded ground shadow rendering is %s", gd->multiThreadDrawGroundShadow?"enabled":"disabled");
	}
};



class MultiThreadDrawUnitActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawUnitActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawUnit") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			unitDrawer->multiThreadDrawUnit = !unitDrawer->multiThreadDrawUnit;
		} else {
			unitDrawer->multiThreadDrawUnit = !!atoi(action.GetArgs().c_str());
		}
		logOutput.Print("Multi threaded unit rendering is %s", unitDrawer->multiThreadDrawUnit?"enabled":"disabled");
	}
};



class MultiThreadDrawUnitShadowActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawUnitShadowActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawUnitShadow") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			unitDrawer->multiThreadDrawUnitShadow = !unitDrawer->multiThreadDrawUnitShadow;
		} else {
			unitDrawer->multiThreadDrawUnitShadow = !!atoi(action.GetArgs().c_str());
		}
		logOutput.Print("Multi threaded unit shadow rendering is %s", unitDrawer->multiThreadDrawUnitShadow?"enabled":"disabled");
	}
};



class MultiThreadDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawActionExecutor() : IUnsyncedActionExecutor("MultiThreadDraw") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		const bool mtEnabled = IsMTEnabled();
		if (action.GetArgs().empty()) {
			gd->multiThreadDrawGround = !mtEnabled;
			unitDrawer->multiThreadDrawUnit = !mtEnabled;
			unitDrawer->multiThreadDrawUnitShadow = !mtEnabled;
		} else {
			gd->multiThreadDrawGround = !!atoi(action.GetArgs().c_str());
			unitDrawer->multiThreadDrawUnit = !!atoi(action.GetArgs().c_str());
			unitDrawer->multiThreadDrawUnitShadow = !!atoi(action.GetArgs().c_str());
		}
		if (!gd->multiThreadDrawGround)
			gd->multiThreadDrawGroundShadow = 0;
		logOutput.Print("Multithreaded rendering is %s", gd->multiThreadDrawGround ? "enabled" : "disabled");
	}

	static bool IsMTEnabled() {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		return gd->multiThreadDrawGround + unitDrawer->multiThreadDrawUnit + unitDrawer->multiThreadDrawUnitShadow > 1;
	}
};



class MultiThreadSimActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadSimActionExecutor() : IUnsyncedActionExecutor("MultiThreadSim") {}

	void Execute(const UnsyncedAction& action) const {
		const bool mtEnabled = MultiThreadDrawActionExecutor::IsMTEnabled();
#	if GML_ENABLE_SIM
		if (action.GetArgs().empty()) {
			// HACK GetInnerAction() should not be used here
			gmlMultiThreadSim = (StringToLower(action.GetInnerAction().command) == "multithread") ? !mtEnabled : !gmlMultiThreadSim;
		} else {
			gmlMultiThreadSim = !!atoi(action.GetArgs().c_str());
		}
		gmlStartSim=1;
		logOutput.Print("Simulation threading is %s", gmlMultiThreadSim ? "enabled" : "disabled");
#	endif // GML_ENABLE_SIM
	}
};



class MultiThreadActionExecutor : public SequentialActionExecutor {
public:
	MultiThreadActionExecutor() : SequentialActionExecutor("MultiThread") {
		AddExecutor(new MultiThreadDrawActionExecutor());
		AddExecutor(new MultiThreadSimActionExecutor());
	}
};
#endif // USE_GML



class SpeedControlActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedControlActionExecutor() : IUnsyncedActionExecutor("SpeedControl") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			++game->speedControl;
			if (game->speedControl > 2)
				game->speedControl = -2;
		}
		else {
			game->speedControl = atoi(action.GetArgs().c_str());
		}
		game->speedControl = std::max(-2, std::min(game->speedControl, 2));
		net->Send(CBaseNetProtocol::Get().SendSpeedControl(gu->myPlayerNum, game->speedControl));
		logOutput.Print("Speed Control: %s%s",
			(game->speedControl == 0) ? "Default" : ((game->speedControl == 1 || game->speedControl == -1) ? "Average CPU" : "Maximum CPU"),
			(game->speedControl < 0) ? " (server voting disabled)" : "");
		configHandler->Set("SpeedControl", game->speedControl);
		if (gameServer)
			gameServer->UpdateSpeedControl(game->speedControl);
	}
};



class GameInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	GameInfoActionExecutor() : IUnsyncedActionExecutor("GameInfo") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.IsRepeat()) {
			if (!CGameInfo::IsActive()) {
				CGameInfo::Enable();
			} else {
				CGameInfo::Disable();
			}
		}
	}
};



class HideInterfaceActionExecutor : public IUnsyncedActionExecutor {
public:
	HideInterfaceActionExecutor() : IUnsyncedActionExecutor("HideInterface") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			game->hideInterface = !game->hideInterface;
		} else {
			game->hideInterface = !!atoi(action.GetArgs().c_str());
		}
	}
};



class HardwareCursorActionExecutor : public IUnsyncedActionExecutor {
public:
	HardwareCursorActionExecutor() : IUnsyncedActionExecutor("HardwareCursor") {}

	void Execute(const UnsyncedAction& action) const {
// XXX one setting stored in two places (mouse->hardwareCursor & configHandler["HardwareCursor"]) -> refactor?
		if (action.GetArgs().empty()) {
			mouse->hardwareCursor = !mouse->hardwareCursor;
		} else {
			mouse->hardwareCursor = !!atoi(action.GetArgs().c_str());
		}
		mouse->UpdateHwCursor();
		configHandler->Set("HardwareCursor", (int)mouse->hardwareCursor);
	}
};



class IncreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("IncreaseViewRadius") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->IncreaseDetail();
	}
};



class DecreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("DecreaseViewRadius") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->DecreaseDetail();
	}
};



class MoreTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreTreesActionExecutor() : IUnsyncedActionExecutor("MoreTrees") {}

	void Execute(const UnsyncedAction& action) const {
		treeDrawer->baseTreeDistance += 0.2f;
		ReportTreeDistance();
	}
	static void ReportTreeDistance() {
		LogObject() << "Base tree distance " << treeDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}
};



class LessTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	LessTreesActionExecutor() : IUnsyncedActionExecutor("LessTrees") {}

	void Execute(const UnsyncedAction& action) const {
		treeDrawer->baseTreeDistance -= 0.2f;
		MoreTreesActionExecutor::ReportTreeDistance();
	}
};



class MoreCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreCloudsActionExecutor() : IUnsyncedActionExecutor("MoreClouds") {}

	void Execute(const UnsyncedAction& action) const {
		sky->IncreaseCloudDensity();

		ReportCloudDensity();
	}
	static void ReportCloudDensity() {
		LogObject() << "Cloud density " << 1.0f / sky->GetCloudDensity() << "\n";
	}
};



class LessCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	LessCloudsActionExecutor() : IUnsyncedActionExecutor("LessClouds") {}

	void Execute(const UnsyncedAction& action) const {
		sky->DecreaseCloudDensity();

		MoreCloudsActionExecutor::ReportCloudDensity();
	}
};



class SpeedUpActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedUpActionExecutor() : IUnsyncedActionExecutor("SpeedUp") {}

	void Execute(const UnsyncedAction& action) const {
		float speed = gs->userSpeedFactor;
		if (speed < 5) {
			speed += (speed < 2) ? 0.1f : 0.2f;
			float fpart = speed - (int)speed;
			if (fpart < 0.01f || fpart > 0.99f)
				speed = round(speed);
		} else if (speed < 10) {
			speed += 0.5f;
		} else {
			speed += 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
	}
};



class SlowDownActionExecutor : public IUnsyncedActionExecutor {
public:
	SlowDownActionExecutor() : IUnsyncedActionExecutor("SlowDown") {}

	void Execute(const UnsyncedAction& action) const {
		float speed = gs->userSpeedFactor;
		if (speed <= 5) {
			speed -= (speed <= 2) ? 0.1f : 0.2f;
			float fpart = speed - (int)speed;
			if (fpart < 0.01f || fpart > 0.99f)
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
};



class ControlUnitActionExecutor : public IUnsyncedActionExecutor {
public:
	ControlUnitActionExecutor() : IUnsyncedActionExecutor("ControlUnit") {}

	void Execute(const UnsyncedAction& action) const {
		if (!gu->spectating) {
			Command c(CMD_STOP);
			// force it to update selection and clear order queue
			selectedUnits.GiveCommand(c, false);
			net->Send(CBaseNetProtocol::Get().SendDirectControl(gu->myPlayerNum));
		}
	}
};



class ShowStandardActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowStandardActionExecutor() : IUnsyncedActionExecutor("ShowStandard") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->DisableExtraTexture();
	}
};



class ShowElevationActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowElevationActionExecutor() : IUnsyncedActionExecutor("ShowElevation") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->SetHeightTexture();
	}
};



class ToggleRadarAndJammerActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleRadarAndJammerActionExecutor() : IUnsyncedActionExecutor("ToggleRadarAndJammer") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->ToggleRadarAndJammer();
	}
};



class ShowMetalMapActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowMetalMapActionExecutor() : IUnsyncedActionExecutor("ShowMetalMap") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->SetMetalTexture(readmap->metalMap);
	}
};



class ShowPathTraversabilityActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathTraversabilityActionExecutor() : IUnsyncedActionExecutor("ShowPathTraversability") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->TogglePathTraversabilityTexture();
	}
};



class ShowPathHeatActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathHeatActionExecutor() : IUnsyncedActionExecutor("ShowPathHeat", true) {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->TogglePathHeatTexture();
	}
};



class ShowPathCostActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathCostActionExecutor() : IUnsyncedActionExecutor("ShowPathCost", true) {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->TogglePathCostTexture();
	}
};



class ToggleLOSActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleLOSActionExecutor() : IUnsyncedActionExecutor("ToggleLOS") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		gd->ToggleLosTexture();
	}
};



class ShareDialogActionExecutor : public IUnsyncedActionExecutor {
public:
	ShareDialogActionExecutor() : IUnsyncedActionExecutor("ShareDialog") {}

	void Execute(const UnsyncedAction& action) const {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty() && dynamic_cast<CShareBox*>(inputReceivers.front()) == NULL && !gu->spectating)
			new CShareBox();
	}
};



class QuitMessageActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMessageActionExecutor() : IUnsyncedActionExecutor("QuitMessage") {}

	void Execute(const UnsyncedAction& action) const {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == NULL) {
			CKeyBindings::HotkeyList quitlist = keyBindings->GetHotkeys("quitmenu");
			std::string quitkey = quitlist.empty() ? "<none>" : quitlist.front();
			logOutput.Print(std::string("Press ") + quitkey + " to access the quit menu");
		}
	}
};



class QuitMenuActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMenuActionExecutor() : IUnsyncedActionExecutor("QuitMenu") {}

	void Execute(const UnsyncedAction& action) const {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == NULL)
			new CQuitBox();
	}
};



class QuitActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitActionExecutor() : IUnsyncedActionExecutor("Quit") {}

	void Execute(const UnsyncedAction& action) const {
		Exit();
	}
	static void Exit() {
		logOutput.Print("User exited");
		gu->globalQuit = true;
	}
};



class IncreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("IncGUIOpacity") {}

	void Execute(const UnsyncedAction& action) const {
		CInputReceiver::guiAlpha = std::min(CInputReceiver::guiAlpha+0.1f,1.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}
};



class DecreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("DecGUIOpacity") {}

	void Execute(const UnsyncedAction& action) const {
		CInputReceiver::guiAlpha = std::max(CInputReceiver::guiAlpha-0.1f,0.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}
};



class ScreenShotActionExecutor : public IUnsyncedActionExecutor {
public:
	ScreenShotActionExecutor() : IUnsyncedActionExecutor("ScreenShot") {}

	void Execute(const UnsyncedAction& action) const {
		TakeScreenshot(action.GetArgs());
	}
};



class GrabInputActionExecutor : public IUnsyncedActionExecutor {
public:
	GrabInputActionExecutor() : IUnsyncedActionExecutor("GrabInput") {}

	void Execute(const UnsyncedAction& action) const {
		SDL_GrabMode newMode;
		if (action.GetArgs().empty()) {
			const SDL_GrabMode curMode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
			switch (curMode) {
				default: // make compiler happy
				case SDL_GRAB_OFF: newMode = SDL_GRAB_ON;  break;
				case SDL_GRAB_ON:  newMode = SDL_GRAB_OFF; break;
			}
		} else {
			if (atoi(action.GetArgs().c_str())) {
				newMode = SDL_GRAB_ON;
			} else {
				newMode = SDL_GRAB_OFF;
			}
		}
		SDL_WM_GrabInput(newMode);
		logOutput.Print("Input grabbing %s",
		                (newMode == SDL_GRAB_ON) ? "enabled" : "disabled");
	}
};



class ClockActionExecutor : public IUnsyncedActionExecutor {
public:
	ClockActionExecutor() : IUnsyncedActionExecutor("Clock") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			game->showClock = !game->showClock;
		} else {
			game->showClock = !!atoi(action.GetArgs().c_str());
		}
		configHandler->Set("ShowClock", game->showClock ? 1 : 0);
	}
};



class CrossActionExecutor : public IUnsyncedActionExecutor {
public:
	CrossActionExecutor() : IUnsyncedActionExecutor("Cross") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			if (mouse->crossSize > 0.0f) {
				mouse->crossSize = -mouse->crossSize;
			} else {
				mouse->crossSize = std::max(1.0f, -mouse->crossSize);
			}
		} else {
			float size, alpha, scale;
			const char* args = action.GetArgs().c_str();
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
};



class FPSActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSActionExecutor() : IUnsyncedActionExecutor("FPS") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			game->showFPS = !game->showFPS;
		} else {
			game->showFPS = !!atoi(action.GetArgs().c_str());
		}
		configHandler->Set("ShowFPS", game->showFPS ? 1 : 0);
	}
};



class SpeedActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedActionExecutor() : IUnsyncedActionExecutor("Speed") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			game->showSpeed = !game->showSpeed;
		} else {
			game->showSpeed = !!atoi(action.GetArgs().c_str());
		}
		configHandler->Set("ShowSpeed", game->showSpeed ? 1 : 0);
	}
};



class MTInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	MTInfoActionExecutor() : IUnsyncedActionExecutor("MTInfo") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			game->showMTInfo = !game->showMTInfo;
		} else {
			game->showMTInfo = !!atoi(action.GetArgs().c_str());
		}
		configHandler->Set("ShowMTInfo", game->showMTInfo ? 1 : 0);
		game->showMTInfo = (game->showMTInfo && gc->GetMultiThreadLua() <= 3) ? gc->GetMultiThreadLua() : 0;
	}
};



class TeamHighlightActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamHighlightActionExecutor() : IUnsyncedActionExecutor("TeamHighlight") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			gc->teamHighlight = abs(gc->teamHighlight + 1) % 3;
		} else {
			gc->teamHighlight = abs(atoi(action.GetArgs().c_str())) % 3;
		}
		logOutput.Print("Team highlighting: %s", ((gc->teamHighlight == 1) ? "Players only" : ((gc->teamHighlight == 2) ? "Players and spectators" : "Disabled")));
		configHandler->Set("TeamHighlight", gc->teamHighlight);
	}
};



class InfoActionExecutor : public IUnsyncedActionExecutor {
public:
	InfoActionExecutor() : IUnsyncedActionExecutor("Info") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			if (playerRoster.GetSortType() == PlayerRoster::Disabled) {
				playerRoster.SetSortTypeByCode(PlayerRoster::Allies);
			} else {
				playerRoster.SetSortTypeByCode(PlayerRoster::Disabled);
			}
		} else {
			playerRoster.SetSortTypeByName(action.GetArgs());
		}
		if (playerRoster.GetSortType() != PlayerRoster::Disabled) {
			logOutput.Print("Sorting roster by %s", playerRoster.GetSortName());
		}
		configHandler->Set("ShowPlayerInfo", (int)playerRoster.GetSortType());
	}
};



class CmdColorsActionExecutor : public IUnsyncedActionExecutor {
public:
	CmdColorsActionExecutor() : IUnsyncedActionExecutor("CmdColors") {}

	void Execute(const UnsyncedAction& action) const {
		const string name = action.GetArgs().empty() ? "cmdcolors.txt" : action.GetArgs();
		cmdColors.LoadConfig(name);
		logOutput.Print("Reloaded cmdcolors with: " + name);
	}
};



class CtrlPanelActionExecutor : public IUnsyncedActionExecutor {
public:
	CtrlPanelActionExecutor() : IUnsyncedActionExecutor("CtrlPanel") {}

	void Execute(const UnsyncedAction& action) const {
		const string name = action.GetArgs().empty() ? "ctrlpanel.txt" : action.GetArgs();
		guihandler->ReloadConfig(name);
		logOutput.Print("Reloaded ctrlpanel with: " + name);
	}
};



class FontActionExecutor : public IUnsyncedActionExecutor {
public:
	FontActionExecutor() : IUnsyncedActionExecutor("Font") {}

	void Execute(const UnsyncedAction& action) const {
		CglFont *newFont = NULL, *newSmallFont = NULL;
		try {
			const int fontSize = configHandler->Get("FontSize", 23);
			const int smallFontSize = configHandler->Get("SmallFontSize", 14);
			const int outlineWidth = configHandler->Get("FontOutlineWidth", 3);
			const float outlineWeight = configHandler->Get("FontOutlineWeight", 25.0f);
			const int smallOutlineWidth = configHandler->Get("SmallFontOutlineWidth", 2);
			const float smallOutlineWeight = configHandler->Get("SmallFontOutlineWeight", 10.0f);

			newFont = CglFont::LoadFont(action.GetArgs(), fontSize, outlineWidth, outlineWeight);
			newSmallFont = CglFont::LoadFont(action.GetArgs(), smallFontSize, smallOutlineWidth, smallOutlineWeight);
		} catch (std::exception& e) {
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
			logOutput.Print("Loaded font: %s\n", action.GetArgs().c_str());
			configHandler->SetString("FontFile", action.GetArgs());
			configHandler->SetString("SmallFontFile", action.GetArgs());
		}
	}
};



class VSyncActionExecutor : public IUnsyncedActionExecutor {
public:
	VSyncActionExecutor() : IUnsyncedActionExecutor("VSync") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			VSync.SetFrames((VSync.GetFrames() <= 0) ? 1 : 0);
		} else {
			VSync.SetFrames(atoi(action.GetArgs().c_str()));
		}
	}
};



class SafeGLActionExecutor : public IUnsyncedActionExecutor {
public:
	SafeGLActionExecutor() : IUnsyncedActionExecutor("SafeGL") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			LuaOpenGL::SetSafeMode(!LuaOpenGL::GetSafeMode());
		} else {
			LuaOpenGL::SetSafeMode(!!atoi(action.GetArgs().c_str()));
		}
	}
};



class ResBarActionExecutor : public IUnsyncedActionExecutor {
public:
	ResBarActionExecutor() : IUnsyncedActionExecutor("ResBar") {}

	void Execute(const UnsyncedAction& action) const {
		if (resourceBar) {
			if (action.GetArgs().empty()) {
				resourceBar->disabled = !resourceBar->disabled;
			} else {
				resourceBar->disabled = !atoi(action.GetArgs().c_str());
			}
		}
	}
};



class ToolTipActionExecutor : public IUnsyncedActionExecutor {
public:
	ToolTipActionExecutor() : IUnsyncedActionExecutor("ToolTip") {}

	void Execute(const UnsyncedAction& action) const {
		if (tooltip) {
			if (action.GetArgs().empty()) {
				tooltip->disabled = !tooltip->disabled;
			} else {
				tooltip->disabled = !atoi(action.GetArgs().c_str());
			}
		}
	}
};



class ConsoleActionExecutor : public IUnsyncedActionExecutor {
public:
	ConsoleActionExecutor() : IUnsyncedActionExecutor("Console") {}

	void Execute(const UnsyncedAction& action) const {
		if (game->infoConsole) {
			if (action.GetArgs().empty()) {
				game->infoConsole->disabled = !game->infoConsole->disabled;
			} else {
				game->infoConsole->disabled = !atoi(action.GetArgs().c_str());
			}
		}
	}
};



class EndGraphActionExecutor : public IUnsyncedActionExecutor {
public:
	EndGraphActionExecutor() : IUnsyncedActionExecutor("EndGraph") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			CEndGameBox::disabled = !CEndGameBox::disabled;
		} else {
			CEndGameBox::disabled = !atoi(action.GetArgs().c_str());
		}
	}
};



class FPSHudActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSHudActionExecutor() : IUnsyncedActionExecutor("FPSHud") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			hudDrawer->SetDraw(!hudDrawer->GetDraw());
		} else {
			hudDrawer->SetDraw(!!atoi(action.GetArgs().c_str()));
		}
	}
};



class DebugDrawAIActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugDrawAIActionExecutor() : IUnsyncedActionExecutor("DebugDrawAI") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			debugDrawerAI->SetDraw(!debugDrawerAI->GetDraw());
		} else {
			debugDrawerAI->SetDraw(!!atoi(action.GetArgs().c_str()));
		}

		logOutput.Print("SkirmishAI debug drawing %s", (debugDrawerAI->GetDraw()? "enabled": "disabled"));
	}
};



class MapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	MapMarksActionExecutor() : IUnsyncedActionExecutor("MapMarks") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			globalRendering->drawMapMarks = !globalRendering->drawMapMarks;
		} else {
			globalRendering->drawMapMarks = !!atoi(action.GetArgs().c_str());
		}
	}
};



class AllMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	AllMapMarksActionExecutor() : IUnsyncedActionExecutor("AllMapMarks", true) {}

	void Execute(const UnsyncedAction& action) const {
		bool allMarksVisible;
		if (action.GetArgs().empty()) {
			allMarksVisible = !inMapDrawerModel->GetAllMarksVisible();
		} else {
			allMarksVisible = !!atoi(action.GetArgs().c_str());
		}
		inMapDrawerModel->SetAllMarksVisible(allMarksVisible);
	}
};



class ClearMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	ClearMapMarksActionExecutor() : IUnsyncedActionExecutor("ClearMapMarks") {}

	void Execute(const UnsyncedAction& action) const {
		inMapDrawerModel->EraseAll();
	}
};



class NoLuaDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	NoLuaDrawActionExecutor() : IUnsyncedActionExecutor("NoLuaDraw") {}

	void Execute(const UnsyncedAction& action) const {
		bool luaMapDrawingAllowed;
		if (action.GetArgs().empty()) {
			luaMapDrawingAllowed = !inMapDrawer->GetLuaMapDrawingAllowed();
		} else {
			luaMapDrawingAllowed = !!atoi(action.GetArgs().c_str());
		}
		inMapDrawer->SetLuaMapDrawingAllowed(luaMapDrawingAllowed);
	}
};



class LuaUIActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaUIActionExecutor() : IUnsyncedActionExecutor("LuaUI") {}

	void Execute(const UnsyncedAction& action) const {
		if (guihandler != NULL) {
			guihandler->PushLayoutCommand(action.GetArgs());
		}
	}
};



class LuaModUICtrlActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaModUICtrlActionExecutor() : IUnsyncedActionExecutor("LuaModUICtrl") {}

	void Execute(const UnsyncedAction& action) const {
		bool modUICtrl;
		if (action.GetArgs().empty()) {
			modUICtrl = !CLuaHandle::GetModUICtrl();
		} else {
			modUICtrl = !!atoi(action.GetArgs().c_str());
		}
		CLuaHandle::SetModUICtrl(modUICtrl);
		configHandler->Set("LuaModUICtrl", modUICtrl ? 1 : 0);
	}
};



class MiniMapActionExecutor : public IUnsyncedActionExecutor {
public:
	MiniMapActionExecutor() : IUnsyncedActionExecutor("MiniMap") {}

	void Execute(const UnsyncedAction& action) const {
		if (minimap != NULL) {
			minimap->ConfigCommand(action.GetArgs());
		}
	}
};



class GroundDecalsActionExecutor : public IUnsyncedActionExecutor {
public:
	GroundDecalsActionExecutor() : IUnsyncedActionExecutor("GroundDecals") {}

	void Execute(const UnsyncedAction& action) const {
		if (groundDecals) {
			if (action.GetArgs().empty()) {
				groundDecals->SetDrawDecals(!groundDecals->GetDrawDecals());
			} else {
				groundDecals->SetDrawDecals(!!atoi(action.GetArgs().c_str()));
			}
		}
		logOutput.Print("Ground decals are %s",
		                groundDecals->GetDrawDecals() ? "enabled" : "disabled");
	}
};



class MaxParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxParticlesActionExecutor() : IUnsyncedActionExecutor("MaxParticles") {}

	void Execute(const UnsyncedAction& action) const {
		if (ph && !action.GetArgs().empty()) {
			const int value = std::max(1, atoi(action.GetArgs().c_str()));
			ph->SetMaxParticles(value);
			logOutput.Print("Set maximum particles to: %i", value);
		}
	}
};



class MaxNanoParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxNanoParticlesActionExecutor() : IUnsyncedActionExecutor("MaxNanoParticles") {}

	void Execute(const UnsyncedAction& action) const {
		if (ph && !action.GetArgs().empty()) {
			const int value = std::max(1, atoi(action.GetArgs().c_str()));
			ph->SetMaxNanoParticles(value);
			logOutput.Print("Set maximum nano-particles to: %i", value);
		}
	}
};



class GatherModeActionExecutor : public IUnsyncedActionExecutor {
public:
	GatherModeActionExecutor() : IUnsyncedActionExecutor("GatherMode") {}

	void Execute(const UnsyncedAction& action) const {
		if (guihandler != NULL) {
			bool gatherMode;
			if (action.GetArgs().empty()) {
				gatherMode = !guihandler->GetGatherMode();
			} else {
				gatherMode = !!atoi(action.GetArgs().c_str());
			}
			guihandler->SetGatherMode(gatherMode);
			logOutput.Print("gathermode %s", gatherMode ? "enabled" : "disabled");
		}
	}
};



class PasteTextActionExecutor : public IUnsyncedActionExecutor {
public:
	PasteTextActionExecutor() : IUnsyncedActionExecutor("PasteText") {}

	void Execute(const UnsyncedAction& action) const {
		if (game->userWriting) {
			if (!action.GetArgs().empty()) {
				game->userInput.insert(game->writingPos, action.GetArgs());
				game->writingPos += action.GetArgs().length();
			} else {
				game->PasteClipboard();
			}
		}
	}
};



class BufferTextActionExecutor : public IUnsyncedActionExecutor {
public:
	BufferTextActionExecutor() : IUnsyncedActionExecutor("BufferText") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			game->consoleHistory->AddLine(action.GetArgs());
		}
	}
};



class InputTextGeoActionExecutor : public IUnsyncedActionExecutor {
public:
	InputTextGeoActionExecutor() : IUnsyncedActionExecutor("InputTextGeo") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			game->ParseInputTextGeometry(action.GetArgs());
		}
	}
};



class DistIconActionExecutor : public IUnsyncedActionExecutor {
public:
	DistIconActionExecutor() : IUnsyncedActionExecutor("DistIcon") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			const int iconDist = atoi(action.GetArgs().c_str());
			unitDrawer->SetUnitIconDist((float)iconDist);
			configHandler->Set("UnitIconDist", iconDist);
			logOutput.Print("Set UnitIconDist to %i", iconDist);
		}
	}
};



class DistDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	DistDrawActionExecutor() : IUnsyncedActionExecutor("DistDraw") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			const int drawDist = atoi(action.GetArgs().c_str());
			unitDrawer->SetUnitDrawDist((float)drawDist);
			configHandler->Set("UnitLodDist", drawDist);
			logOutput.Print("Set UnitLodDist to %i", drawDist);
		}
	}
};



class LODScaleActionExecutor : public IUnsyncedActionExecutor {
public:
	LODScaleActionExecutor() : IUnsyncedActionExecutor("LODScale") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			const vector<string> &args = CSimpleParser::Tokenize(action.GetArgs(), 0);
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
};



class WireMapActionExecutor : public IUnsyncedActionExecutor {
public:
	WireMapActionExecutor() : IUnsyncedActionExecutor("WireMap") {}

	void Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		if (action.GetArgs().empty()) {
			gd->wireframe  = !gd->wireframe;
			sky->wireframe = gd->wireframe;
		} else {
			gd->wireframe  = !!atoi(action.GetArgs().c_str());
			sky->wireframe = gd->wireframe;
		}
	}
};



class AirMeshActionExecutor : public IUnsyncedActionExecutor {
public:
	AirMeshActionExecutor() : IUnsyncedActionExecutor("AirMesh") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			smoothGround->drawEnabled = !smoothGround->drawEnabled;
		} else {
			smoothGround->drawEnabled = !!atoi(action.GetArgs().c_str());
		}
	}
};



class SetGammaActionExecutor : public IUnsyncedActionExecutor {
public:
	SetGammaActionExecutor() : IUnsyncedActionExecutor("SetGamma") {}

	void Execute(const UnsyncedAction& action) const {
		float r, g, b;
		const int count = sscanf(action.GetArgs().c_str(), "%f %f %f", &r, &g, &b);
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
};



class CrashActionExecutor : public IUnsyncedActionExecutor {
public:
	CrashActionExecutor() : IUnsyncedActionExecutor("Crash", true) {}

	void Execute(const UnsyncedAction& action) const {
		int *a=0;
		*a=0;
	}
};



class ExceptionActionExecutor : public IUnsyncedActionExecutor {
public:
	ExceptionActionExecutor() : IUnsyncedActionExecutor("Exception", true) {}

	void Execute(const UnsyncedAction& action) const {
		throw std::runtime_error("Exception test");
	}
};



class DivByZeroActionExecutor : public IUnsyncedActionExecutor {
public:
	DivByZeroActionExecutor() : IUnsyncedActionExecutor("DivByZero", true) {}

	void Execute(const UnsyncedAction& action) const {
		float a = 0;
		logOutput.Print("Result: %f", 1.0f/a);
	}
};



class GiveActionExecutor : public IUnsyncedActionExecutor {
public:
	GiveActionExecutor() : IUnsyncedActionExecutor("Give", true) {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().find('@') == string::npos) {
			CInputReceiver* ir = NULL;
			if (!game->hideInterface) {
				ir = CInputReceiver::GetReceiverAt(mouse->lastx, mouse->lasty);
			}

			float3 givePos;
			if (ir == minimap) {
				givePos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			} else {
				const float3& pos = camera->pos;
				const float3& dir = mouse->dir;
				const float dist = ground->LineGroundCol(pos, pos + (dir * 30000.0f));
				givePos = pos + (dir * dist);
			}

			char message[256];
			SNPRINTF(message, sizeof(message),
					"%s %s @%.0f,%.0f,%.0f",
					GetCommand().c_str(), action.GetArgs().c_str(),
					givePos.x, givePos.y, givePos.z);

			CommandMessage pckt(message, gu->myPlayerNum);
			net->Send(pckt.Pack());
		} else {
			// forward (as synced command)
			CommandMessage pckt(action.GetInnerAction(), gu->myPlayerNum);
			net->Send(pckt.Pack());
		}
	}
};



class DestroyActionExecutor : public IUnsyncedActionExecutor {
public:
	DestroyActionExecutor() : IUnsyncedActionExecutor("Destroy", true) {}

	void Execute(const UnsyncedAction& action) const {
		// kill selected units
		std::stringstream ss;
		ss << GetCommand();
		for (CUnitSet::iterator it = selectedUnits.selectedUnits.begin();
				it != selectedUnits.selectedUnits.end(); ++it)
		{
			ss << " " << (*it)->id;
		}
		CommandMessage pckt(ss.str(), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
};



class SendActionExecutor : public IUnsyncedActionExecutor {
public:
	SendActionExecutor() : IUnsyncedActionExecutor("Send") {}

	void Execute(const UnsyncedAction& action) const {
		CommandMessage pckt(Action(action.GetArgs()), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
};



class SaveGameActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveGameActionExecutor() : IUnsyncedActionExecutor("SaveGame") {}

	void Execute(const UnsyncedAction& action) const {
		game->SaveGame("Saves/QuickSave.ssf", true);
	}
};



/// /save [-y ]<savename>
class SaveActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveActionExecutor() : IUnsyncedActionExecutor("Save") {}

	void Execute(const UnsyncedAction& action) const {
		bool saveoverride = action.GetArgs().find("-y ") == 0;
		std::string savename(action.GetArgs().c_str() + (saveoverride ? 3 : 0));
		savename = "Saves/" + savename + ".ssf";
		game->SaveGame(savename, saveoverride);
	}
};



class ReloadGameActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadGameActionExecutor() : IUnsyncedActionExecutor("ReloadGame") {}

	void Execute(const UnsyncedAction& action) const {
		game->ReloadGame();
	}
};



class DebugInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugInfoActionExecutor() : IUnsyncedActionExecutor("DebugInfo") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs() == "sound") {
			sound->PrintDebugInfo();
		} else if (action.GetArgs() == "profiling") {
			profiler.PrintProfilingInfo();
		}
	}
};



class BenchmarkScriptActionExecutor : public IUnsyncedActionExecutor {
public:
	BenchmarkScriptActionExecutor() : IUnsyncedActionExecutor("Benchmark-Script") {}

	void Execute(const UnsyncedAction& action) const {
		CUnitScript::BenchmarkScript(action.GetArgs());
	}
};



class RedirectToSyncedActionExecutor : public IUnsyncedActionExecutor {
public:
	RedirectToSyncedActionExecutor(const std::string& command) : IUnsyncedActionExecutor(command) {}

	void Execute(const UnsyncedAction& action) const {
		// redirect as a synced command
		CommandMessage pckt(action.GetInnerAction(), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
};

} // namespace syncedActionExecutors





// TODO CGame stuff in UnsyncedGameCommands: refactor (or move)
bool CGame::ActionReleased(const Action& action)
{
	const string& cmd = action.command;

	if (cmd == "drawinmap") {
		inMapDrawer->SetDrawMode(false);
	}
	else if (cmd == "moveforward") {
		camMove[0] = false;
	}
	else if (cmd == "moveback") {
		camMove[1] = false;
	}
	else if (cmd == "moveleft") {
		camMove[2] = false;
	}
	else if (cmd == "moveright") {
		camMove[3] = false;
	}
	else if (cmd == "moveup") {
		camMove[4] = false;
	}
	else if (cmd == "movedown") {
		camMove[5] = false;
	}
	else if (cmd == "movefast") {
		camMove[6] = false;
	}
	else if (cmd == "moveslow") {
		camMove[7] = false;
	}
	else if (cmd == "mouse1") {
		mouse->MouseRelease(mouse->lastx, mouse->lasty, 1);
	}
	else if (cmd == "mouse2") {
		mouse->MouseRelease(mouse->lastx, mouse->lasty, 2);
	}
	else if (cmd == "mouse3") {
		mouse->MouseRelease(mouse->lastx, mouse->lasty, 3);
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




void UnsyncedGameCommands::AddActionExecutor(IUnsyncedActionExecutor* executor)
{
	const std::string commandLower = StringToLower(executor->GetCommand());
	const std::map<std::string, IUnsyncedActionExecutor*>::const_iterator aei
			= actionExecutors.find(commandLower);

	if (aei != actionExecutors.end()) {
		throw std::logic_error("Tried to register a duplicate UnsyncedActionExecutor for command: " + commandLower);
	} else {
		actionExecutors[commandLower] = executor;
	}
}

void UnsyncedGameCommands::RemoveActionExecutor(const std::string& command)
{
	const std::string commandLower = StringToLower(command);
	const std::map<std::string, IUnsyncedActionExecutor*>::iterator aei
			= actionExecutors.find(commandLower);

	if (aei != actionExecutors.end()) {
		// an executor for this command is registered
		// -> remove and delete
		IUnsyncedActionExecutor* executor = aei->second;
		actionExecutors.erase(aei);
		delete executor;
	}
}

void UnsyncedGameCommands::RemoveAllActionExecutors()
{
	// by copy - clear - delete in copy,
	// we ensure that no deleted executor may be used.
	std::map<std::string, IUnsyncedActionExecutor*> actionExecutorsCopy = actionExecutors;
	actionExecutors.clear();
	std::map<std::string, IUnsyncedActionExecutor*>::iterator aei;
	for (aei = actionExecutorsCopy.begin(); aei != actionExecutorsCopy.end(); ++aei) {
		SafeDelete(aei->second);
	}
}

const IUnsyncedActionExecutor* UnsyncedGameCommands::GetActionExecutor(const std::string& command) const
{
	const IUnsyncedActionExecutor* executor = NULL;

	const std::string commandLower = StringToLower(command);
	const std::map<std::string, IUnsyncedActionExecutor*>::const_iterator aei
			= actionExecutors.find(commandLower);

	if (aei != actionExecutors.end()) {
		// an executor for this command is registered
		executor = aei->second;
	}

	return executor;
}


void UnsyncedGameCommands::AddDefaultActionExecutors() {

	using namespace unsyncedActionExecutors;

	AddActionExecutor(new SelectActionExecutor());
	AddActionExecutor(new SelectUnitsActionExecutor());
	AddActionExecutor(new SelectCycleActionExecutor());
	AddActionExecutor(new DeselectActionExecutor());
	AddActionExecutor(new ShadowsActionExecutor());
	AddActionExecutor(new WaterActionExecutor());
	AddActionExecutor(new AdvModelShadingActionExecutor());
	AddActionExecutor(new AdvMapShadingActionExecutor());
	AddActionExecutor(new SayActionExecutor());
	AddActionExecutor(new SayPrivateActionExecutor());
	AddActionExecutor(new SayPrivateByPlayerIDActionExecutor());
	AddActionExecutor(new EchoActionExecutor());
	AddActionExecutor(new SetActionExecutor());
	AddActionExecutor(new SetOverlayActionExecutor());
	AddActionExecutor(new EnableDrawInMapActionExecutor());
	AddActionExecutor(new DrawLabelActionExecutor());
	AddActionExecutor(new MouseActionExecutor(1));
	AddActionExecutor(new MouseActionExecutor(2));
	AddActionExecutor(new MouseActionExecutor(3));
	AddActionExecutor(new MouseActionExecutor(4));
	AddActionExecutor(new MouseActionExecutor(5));
	AddActionExecutor(new ViewSelectionActionExecutor());
	AddActionExecutor(new CameraMoveActionExecutor(0, "Forward"));
	AddActionExecutor(new CameraMoveActionExecutor(1, "Back"));
	AddActionExecutor(new CameraMoveActionExecutor(2, "Left"));
	AddActionExecutor(new CameraMoveActionExecutor(3, "Right"));
	AddActionExecutor(new CameraMoveActionExecutor(4, "Up"));
	AddActionExecutor(new CameraMoveActionExecutor(5, "Down"));
	AddActionExecutor(new CameraMoveActionExecutor(6, "Fast"));
	AddActionExecutor(new CameraMoveActionExecutor(7, "Slow"));
	AddActionExecutor(new AIKillReloadActionExecutor(true));
	AddActionExecutor(new AIKillReloadActionExecutor(false));
	AddActionExecutor(new AIControlActionExecutor());
	AddActionExecutor(new AIListActionExecutor());
	AddActionExecutor(new TeamActionExecutor());
	AddActionExecutor(new SpectatorActionExecutor());
	AddActionExecutor(new SpecTeamActionExecutor());
	AddActionExecutor(new SpecFullViewActionExecutor());
	AddActionExecutor(new AllyActionExecutor());
	AddActionExecutor(new GroupActionExecutor());
	AddActionExecutor(new GroupIDActionExecutor(0));
	AddActionExecutor(new GroupIDActionExecutor(1));
	AddActionExecutor(new GroupIDActionExecutor(2));
	AddActionExecutor(new GroupIDActionExecutor(3));
	AddActionExecutor(new GroupIDActionExecutor(4));
	AddActionExecutor(new GroupIDActionExecutor(5));
	AddActionExecutor(new GroupIDActionExecutor(6));
	AddActionExecutor(new GroupIDActionExecutor(7));
	AddActionExecutor(new GroupIDActionExecutor(8));
	AddActionExecutor(new GroupIDActionExecutor(9));
	AddActionExecutor(new LastMessagePositionActionExecutor());
	ChatActionExecutor::RegisterCommandVariants();
	AddActionExecutor(new TrackActionExecutor());
	AddActionExecutor(new TrackOffActionExecutor());
	AddActionExecutor(new TrackModeActionExecutor());
#ifdef USE_GML
	AddActionExecutor(new ShowHealthBarsActionExecutor());
	AddActionExecutor(new ShowRezurectionBarsActionExecutor());
#endif // USE_GML
	AddActionExecutor(new PauseActionExecutor());
	AddActionExecutor(new DebugActionExecutor());
	AddActionExecutor(new NoSoundActionExecutor());
	AddActionExecutor(new SoundChannelEnableActionExecutor());
	AddActionExecutor(new CreateVideoActionExecutor());
	AddActionExecutor(new DrawTreesActionExecutor());
	AddActionExecutor(new DynamicSkyActionExecutor());
	AddActionExecutor(new DynamicSunActionExecutor());
#ifdef USE_GML
	AddActionExecutor(new MultiThreadDrawGroundActionExecutor());
	AddActionExecutor(new MultiThreadDrawGroundShadowActionExecutor());
	AddActionExecutor(new MultiThreadDrawUnitActionExecutor());
	AddActionExecutor(new MultiThreadDrawUnitShadowActionExecutor());
	AddActionExecutor(new MultiThreadDrawActionExecutor());
	AddActionExecutor(new MultiThreadSimActionExecutor());
	AddActionExecutor(new MultiThreadActionExecutor());
#endif // USE_GML
	AddActionExecutor(new SpeedControlActionExecutor());
	AddActionExecutor(new GameInfoActionExecutor());
	AddActionExecutor(new HideInterfaceActionExecutor());
	AddActionExecutor(new HardwareCursorActionExecutor());
	AddActionExecutor(new IncreaseViewRadiusActionExecutor());
	AddActionExecutor(new DecreaseViewRadiusActionExecutor());
	AddActionExecutor(new MoreTreesActionExecutor());
	AddActionExecutor(new LessTreesActionExecutor());
	AddActionExecutor(new MoreCloudsActionExecutor());
	AddActionExecutor(new LessCloudsActionExecutor());
	AddActionExecutor(new SpeedUpActionExecutor());
	AddActionExecutor(new SlowDownActionExecutor());
	AddActionExecutor(new ControlUnitActionExecutor());
	AddActionExecutor(new ShowStandardActionExecutor());
	AddActionExecutor(new ShowElevationActionExecutor());
	AddActionExecutor(new ToggleRadarAndJammerActionExecutor());
	AddActionExecutor(new ShowMetalMapActionExecutor());
	AddActionExecutor(new ShowPathTraversabilityActionExecutor());
	AddActionExecutor(new ShowPathHeatActionExecutor());
	AddActionExecutor(new ShowPathCostActionExecutor());
	AddActionExecutor(new ToggleLOSActionExecutor());
	AddActionExecutor(new ShareDialogActionExecutor());
	AddActionExecutor(new QuitMessageActionExecutor());
	AddActionExecutor(new QuitMenuActionExecutor());
	AddActionExecutor(new QuitActionExecutor());
	AddActionExecutor(new AliasActionExecutor(new QuitActionExecutor(), "QuitForce"));
	AddActionExecutor(new IncreaseGUIOpacityActionExecutor());
	AddActionExecutor(new DecreaseGUIOpacityActionExecutor());
	AddActionExecutor(new ScreenShotActionExecutor());
	AddActionExecutor(new GrabInputActionExecutor());
	AddActionExecutor(new ClockActionExecutor());
	AddActionExecutor(new CrossActionExecutor());
	AddActionExecutor(new FPSActionExecutor());
	AddActionExecutor(new SpeedActionExecutor());
	AddActionExecutor(new MTInfoActionExecutor());
	AddActionExecutor(new TeamHighlightActionExecutor());
	AddActionExecutor(new InfoActionExecutor());
	AddActionExecutor(new CmdColorsActionExecutor());
	AddActionExecutor(new CtrlPanelActionExecutor());
	AddActionExecutor(new FontActionExecutor());
	AddActionExecutor(new VSyncActionExecutor());
	AddActionExecutor(new SafeGLActionExecutor());
	AddActionExecutor(new ResBarActionExecutor());
	AddActionExecutor(new ToolTipActionExecutor());
	AddActionExecutor(new ConsoleActionExecutor());
	AddActionExecutor(new EndGraphActionExecutor());
	AddActionExecutor(new FPSHudActionExecutor());
	AddActionExecutor(new DebugDrawAIActionExecutor());
	AddActionExecutor(new MapMarksActionExecutor());
	AddActionExecutor(new AllMapMarksActionExecutor());
	AddActionExecutor(new ClearMapMarksActionExecutor());
	AddActionExecutor(new NoLuaDrawActionExecutor());
	AddActionExecutor(new LuaUIActionExecutor());
	AddActionExecutor(new LuaModUICtrlActionExecutor());
	AddActionExecutor(new MiniMapActionExecutor());
	AddActionExecutor(new GroundDecalsActionExecutor());
	AddActionExecutor(new MaxParticlesActionExecutor());
	AddActionExecutor(new MaxNanoParticlesActionExecutor());
	AddActionExecutor(new GatherModeActionExecutor());
	AddActionExecutor(new PasteTextActionExecutor());
	AddActionExecutor(new BufferTextActionExecutor());
	AddActionExecutor(new InputTextGeoActionExecutor());
	AddActionExecutor(new DistIconActionExecutor());
	AddActionExecutor(new DistDrawActionExecutor());
	AddActionExecutor(new LODScaleActionExecutor());
	AddActionExecutor(new WireMapActionExecutor());
	AddActionExecutor(new AirMeshActionExecutor());
	AddActionExecutor(new SetGammaActionExecutor());
	AddActionExecutor(new CrashActionExecutor());
	AddActionExecutor(new ExceptionActionExecutor());
	AddActionExecutor(new DivByZeroActionExecutor());
	AddActionExecutor(new GiveActionExecutor());
	AddActionExecutor(new DestroyActionExecutor());
	AddActionExecutor(new SendActionExecutor());
	AddActionExecutor(new SaveGameActionExecutor());
	AddActionExecutor(new SaveActionExecutor());
	AddActionExecutor(new ReloadGameActionExecutor());
	AddActionExecutor(new DebugInfoActionExecutor());
	AddActionExecutor(new BenchmarkScriptActionExecutor());
	// XXX are these redirects really required?
	AddActionExecutor(new RedirectToSyncedActionExecutor("ATM"));
#ifdef DEBUG
	AddActionExecutor(new RedirectToSyncedActionExecutor("Desync"));
#endif
	AddActionExecutor(new RedirectToSyncedActionExecutor("Resync"));
	AddActionExecutor(new RedirectToSyncedActionExecutor("Take"));
	AddActionExecutor(new RedirectToSyncedActionExecutor("LuaRules"));
	AddActionExecutor(new RedirectToSyncedActionExecutor("LuaGaia"));
}


UnsyncedGameCommands* UnsyncedGameCommands::singleton = NULL;

void UnsyncedGameCommands::CreateInstance() {

	if (singleton == NULL) {
		singleton = new UnsyncedGameCommands();
	} else {
		throw std::logic_error("UnsyncedGameCommands singleton is already initialized");
	}
}

void UnsyncedGameCommands::DestroyInstance() {

	if (singleton != NULL) {
		// SafeDelete
		UnsyncedGameCommands* tmp = singleton;
		singleton = NULL;
		delete tmp;
	} else {
		throw std::logic_error("UnsyncedGameCommands singleton is already destroyed");
	}
}
