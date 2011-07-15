/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include <SDL_events.h>

#include "UnsyncedGameCommands.h"
#include "UnsyncedActionExecutor.h"
#include "SyncedGameCommands.h"
#include "SyncedActionExecutor.h"
#include "Game.h"
#include "Action.h"
#include "CameraHandler.h"
#include "ConsoleHistory.h"
#include "GameServer.h"
#include "CommandMessage.h"
#include "GameSetup.h"
#include "GlobalUnsynced.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "PlayerRoster.h"
#include "System/TimeProfiler.h"
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
#include "System/GlobalConfig.h"
#include "System/NetProtocol.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/Util.h"


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

namespace { // prevents linking problems in case of duplicate symbols

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
		: IUnsyncedActionExecutor(commandAlias, "Alias for command \"" + commandAlias + "\"")
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
		: IUnsyncedActionExecutor(command, "Executes the following commands in order:")
	{}
	virtual ~SequentialActionExecutor() {

		std::vector<IUnsyncedActionExecutor*>::iterator ei;
		for (ei = innerExecutors.begin(); ei != innerExecutors.end(); ++ei) {
			delete *ei;
		}
	}

	void AddExecutor(IUnsyncedActionExecutor* innerExecutor) {

		innerExecutors.push_back(innerExecutor);
		SetDescription(GetDescription() + " " + innerExecutor->GetCommand());
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
	SelectActionExecutor() : IUnsyncedActionExecutor("Select",
			"<chat command description: Select>") {} // TODO

	void Execute(const UnsyncedAction& action) const {
		selectionKeys->DoSelection(action.GetArgs());
	}
};



class SelectUnitsActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectUnitsActionExecutor() : IUnsyncedActionExecutor("SelectUnits",
			"<chat command description: SelectUnits>") {} // TODO

	void Execute(const UnsyncedAction& action) const {
		game->SelectUnits(action.GetArgs());
	}
};



class SelectCycleActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectCycleActionExecutor() : IUnsyncedActionExecutor("SelectCycle",
			"<chat command description: SelectUnits>") {} // TODO

	void Execute(const UnsyncedAction& action) const {
		game->SelectCycle(action.GetArgs());
	}
};



class DeselectActionExecutor : public IUnsyncedActionExecutor {
public:
	DeselectActionExecutor() : IUnsyncedActionExecutor("Deselect",
			"Deselects all currently selected units") {}

	void Execute(const UnsyncedAction& action) const {
		selectedUnits.ClearSelected();
	}
};



class ShadowsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShadowsActionExecutor() : IUnsyncedActionExecutor("Shadows",
			"Disables/Enables shadows rendering: -1=disabled, 0=off,"
			" 1=unit&feature-shadows, 2=+terrain-shadows") {}

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
			int mapSize = 2048;
			const char* args = action.GetArgs().c_str();
			const int argcount = sscanf(args, "%i %i", &next, &mapSize);
			if (argcount > 1) {
				configHandler->Set("ShadowMapSize", mapSize);
			}
		} else {
			next = (current + 1) % 3;
		}
		configHandler->Set("Shadows", next);
		logOutput.Print("Set Shadows to %i", next);
		shadowHandler = new CShadowHandler();
	}
};



class WaterActionExecutor : public IUnsyncedActionExecutor {
public:
	WaterActionExecutor() : IUnsyncedActionExecutor("Water",
			"Set water rendering mode: 0=basic, 1=reflective, 2=dynamic"
			", 3=reflective&refractive?, 4=bump-mapped") {}

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
	AdvModelShadingActionExecutor() : IUnsyncedActionExecutor("AdvModelShading",
			"Set or toggle advanced model shading mode") {}

	void Execute(const UnsyncedAction& action) const {

		static bool canUseShaders = unitDrawer->advShading;
		if (canUseShaders) {
			SetBoolArg(unitDrawer->advShading, action.GetArgs());
			LogSystemStatus("model shaders", unitDrawer->advShading);
		}
	}
};



class AdvMapShadingActionExecutor : public IUnsyncedActionExecutor {
public:
	AdvMapShadingActionExecutor() : IUnsyncedActionExecutor("AdvMapShading",
			"Set or toggle advanced map shading mode") {}

	void Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		static bool canUseShaders = gd->advShading;
		if (canUseShaders) {
			SetBoolArg(gd->advShading, action.GetArgs());
			LogSystemStatus("map shaders", gd->advShading);
		}
	}
};



class SayActionExecutor : public IUnsyncedActionExecutor {
public:
	SayActionExecutor() : IUnsyncedActionExecutor("Say",
			"Say something in (public) chat") {}

	void Execute(const UnsyncedAction& action) const {
		game->SendNetChat(action.GetArgs());
	}
};



class SayPrivateActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateActionExecutor() : IUnsyncedActionExecutor("W",
			"Say something in private to a specific player, by player-name") {}

	void Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const int playerID = playerHandler->Player(action.GetArgs().substr(0, pos));
			if (playerID >= 0) {
				game->SendNetChat(action.GetArgs().substr(pos+1), playerID);
			} else {
				logOutput.Print("Player not found: %s", action.GetArgs().substr(0, pos).c_str());
			}
		}
	}
};



class SayPrivateByPlayerIDActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateByPlayerIDActionExecutor() : IUnsyncedActionExecutor("WByNum",
			"Say something in private to a specific player, by player-ID") {}

	void Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			std::istringstream buf(action.GetArgs().substr(0, pos));
			int playerID;
			buf >> playerID;
			if (playerID >= 0) {
				game->SendNetChat(action.GetArgs().substr(pos+1), playerID);
			} else {
				logOutput.Print("Player-ID invalid: %i", playerID);
			}
		}
	}
};



class EchoActionExecutor : public IUnsyncedActionExecutor {
public:
	EchoActionExecutor() : IUnsyncedActionExecutor("Echo",
			"Write a string to the log file") {}

	void Execute(const UnsyncedAction& action) const {
		logOutput.Print(action.GetArgs());
	}
};



class SetActionExecutor : public IUnsyncedActionExecutor {
public:
	SetActionExecutor() : IUnsyncedActionExecutor("Set",
			"Set a config key=value pair") {}

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
	SetOverlayActionExecutor() : IUnsyncedActionExecutor("TSet",
			"Set a config key=value pair in the overlay, meaning it will not be"
			" persisted for future games") {}

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
	EnableDrawInMapActionExecutor() : IUnsyncedActionExecutor("DrawInMap",
			"Enables drawing on the map") {}

	void Execute(const UnsyncedAction& action) const {
		inMapDrawer->SetDrawMode(true);
	}
};



class DrawLabelActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawLabelActionExecutor() : IUnsyncedActionExecutor("DrawLabel",
			"Draws a label on the map at the current mouse-pointer position") {}

	void Execute(const UnsyncedAction& action) const {
		const float3 pos = inMapDrawer->GetMouseMapPos();
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
		: IUnsyncedActionExecutor("Mouse" + IntToString(button),
			"Simulates a mouse button press of button " + IntToString(button))
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
	ViewSelectionActionExecutor() : IUnsyncedActionExecutor("ViewSelection",
			"Moves the camera to the center of the currently selected units") {}

	void Execute(const UnsyncedAction& action) const {
		GML_RECMUTEX_LOCK(sel); // ActionPressed

		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		if (!selUnits.empty()) {
			// XXX this code is duplicated in CGroup::CalculateCenter(), move to CUnitSet maybe
			float3 pos(0.0f, 0.0f, 0.0f);
			CUnitSet::const_iterator ui;
			for (ui = selUnits.begin(); ui != selUnits.end(); ++ui) {
				pos += (*ui)->midPos;
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
		: IUnsyncedActionExecutor("Move" + commandPostfix,
			"Moves the camera " + commandPostfix + " a bit")
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
		: IUnsyncedActionExecutor((kill ? "AIKill" : "AIReload"),
			std::string(kill ? "Kills" : "Reloads")
				+ " the Skirmish AI controlling a specified team")
		, kill(kill)
	{}

	void Execute(const UnsyncedAction& action) const {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const std::vector<std::string>& args = _local_strSpaceTokenize(action.GetArgs());
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);
		const std::string actionName  = StringToLower(GetCommand()).substr(2);

		if (!args.empty()) {
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
	AIControlActionExecutor() : IUnsyncedActionExecutor("AIControl",
			"Creates a new instance of a Skirmish AI, to let it controll"
			" a specific team") {}

	void Execute(const UnsyncedAction& action) const {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;
		const bool cheating           = gs->cheatEnabled;
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);
		const std::vector<std::string>& args = _local_strSpaceTokenize(action.GetArgs());

		if (!args.empty()) {
			int         teamToControlId = -1;
			std::string aiShortName     = "";
			std::string aiVersion       = "";
			std::string aiName          = "";
			std::map<std::string, std::string> aiOptions;

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
	AIListActionExecutor() : IUnsyncedActionExecutor("AIList",
			"Prints a list of all currently active Skirmish AIs") {}

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
	TeamActionExecutor() : IUnsyncedActionExecutor("Team",
			"Lets the local user change to an other team", true) {}

	void Execute(const UnsyncedAction& action) const {
		const int teamId = atoi(action.GetArgs().c_str());
		if (teamHandler->IsValidTeam(teamId)) {
			net->Send(CBaseNetProtocol::Get().SendJoinTeam(gu->myPlayerNum, teamId));
		}
	}
};



class SpectatorActionExecutor : public IUnsyncedActionExecutor {
public:
	SpectatorActionExecutor() : IUnsyncedActionExecutor("Spectator",
			"Lets the local user give up controll over a team, and start spectating") {}

	void Execute(const UnsyncedAction& action) const {
		if (!gu->spectating) {
			net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
		}
	}
};



class SpecTeamActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecTeamActionExecutor() : IUnsyncedActionExecutor("SpecTeam",
			"Lets the local user specify the team to follow, if he is a spectator") {}

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
	SpecFullViewActionExecutor() : IUnsyncedActionExecutor("SpecFullView",
			"Sets or toggles between full LOS or ally-team LOS, if the local"
			" user is a spectator") {}

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
	AllyActionExecutor() : IUnsyncedActionExecutor("Ally",
			"Starts/Ends alliance of the local players ally-team with an other ally-team") {}

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
					logOutput.Print("In-game alliances are not allowed");
				}
			}
		}
	}
};



class GroupActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupActionExecutor() : IUnsyncedActionExecutor("Group",
			"Allows modifying the members of a group") {}

	void Execute(const UnsyncedAction& action) const {

		const char firstChar = action.GetArgs()[0];
		if ((firstChar >= '0') && (firstChar <= '9')) {
			const int teamId = (int) (firstChar - '0');
			size_t firstCmdChar = action.GetArgs().find_first_not_of(" \t\n\r", 1);
			if (firstCmdChar != std::string::npos) {
				const std::string command = action.GetArgs().substr(firstCmdChar);
				grouphandlers[gu->myTeam]->GroupCommand(teamId, command);
			}
		}
	}
};



class GroupIDActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupIDActionExecutor(int groupId)
		: IUnsyncedActionExecutor("Group" + IntToString(groupId),
			"Allows modifying the members of group " + IntToString(groupId))
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
	LastMessagePositionActionExecutor() : IUnsyncedActionExecutor("LastMsgPos",
			"Moves the camera to show the position of the last message") {}

	void Execute(const UnsyncedAction& action) const {
		// cycle through the positions
		camHandler->GetCurrentController().SetPos(game->infoConsole->GetMsgPos());
		camHandler->CameraTransition(0.6f);
	}
};



class ChatActionExecutor : public IUnsyncedActionExecutor {

	ChatActionExecutor(const std::string& commandPostfix, const std::string& userInputPrefix, bool setUserInputPrefix)
		: IUnsyncedActionExecutor("Chat" + commandPostfix,
			"Starts waiting for intput to be sent to " + commandPostfix)
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

		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("",     "",   false));
		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("All",  "",   true));
		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("Ally", "a:", true));
		unsyncedGameCommands->AddActionExecutor(new ChatActionExecutor("Spec", "s:", true));
	}

private:
	const std::string userInputPrefix;
	const bool setUserInputPrefix;
};



// TODO merge together with "TrackOff" to "Track 0|1", and deprecate the two old ones
class TrackActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackActionExecutor() : IUnsyncedActionExecutor("Track",
			"Start following the selected unit(s) with the camera") {}

	void Execute(const UnsyncedAction& action) const {
		unitTracker.Track();
	}
};



class TrackOffActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackOffActionExecutor() : IUnsyncedActionExecutor("TrackOff",
			"Stop following the selected unit(s) with the camera") {}

	void Execute(const UnsyncedAction& action) const {
		unitTracker.Disable();
	}
};



class TrackModeActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackModeActionExecutor() : IUnsyncedActionExecutor("TrackMode",
			"Shift through different ways of following selected unit(s)") {}

	void Execute(const UnsyncedAction& action) const {
		unitTracker.IncMode();
	}
};



#ifdef USE_GML
class ShowHealthBarsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowHealthBarsActionExecutor() : IUnsyncedActionExecutor("ShowHealthBars",
			"Enable/Disable rendering of health-bars for units") {}

	void Execute(const UnsyncedAction& action) const {
		SetBoolArg(unitDrawer->showHealthBars, action.GetArgs());
		LogSystemStatus("rendering of health-bars", unitDrawer->showHealthBars);
	}
};



class ShowRezurectionBarsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowRezurectionBarsActionExecutor() : IUnsyncedActionExecutor("ShowRezBars",
			"Enable/Disable rendering of resource-bars for features") {}

	void Execute(const UnsyncedAction& action) const {

		bool showResBars = featureDrawer->GetShowRezBars();
		SetBoolArg(showResBars, action.GetArgs());
		featureDrawer->SetShowRezBars(showResBars);
		LogSystemStatus("rendering of resource-bars for features", featureDrawer->GetShowRezBars());
	}
};
#endif // USE_GML



class PauseActionExecutor : public IUnsyncedActionExecutor {
public:
	PauseActionExecutor() : IUnsyncedActionExecutor("Pause",
			"Pause/Unpause the game") {}

	void Execute(const UnsyncedAction& action) const {
		// disallow pausing prior to start of game proper
		if (game->playing) {
			bool newPause = gs->paused;
			SetBoolArg(newPause, action.GetArgs());
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, newPause));
			game->lastframe = SDL_GetTicks(); // XXX this required here?
		}
	}

};



class DebugActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugActionExecutor() : IUnsyncedActionExecutor("Debug",
			"Enable/Disable debug info rendering mode") {}

	void Execute(const UnsyncedAction& action) const {

		// toggle
		const bool drawDebug = !globalRendering->drawdebug;
		ProfileDrawer::SetEnabled(drawDebug);
		globalRendering->drawdebug = drawDebug;

		LogSystemStatus("debug info rendering mode", globalRendering->drawdebug);
	}
};



// XXX unlucky name; maybe make this "Sound {0|1}" instead (bool arg or toggle)
class NoSoundActionExecutor : public IUnsyncedActionExecutor {
public:
	NoSoundActionExecutor() : IUnsyncedActionExecutor("NoSound",
			"Enable/Disable the sound system") {}

	void Execute(const UnsyncedAction& action) const {

		// toggle
		sound->Mute();

		LogSystemStatus("Sound", !sound->IsMuted());
	}
};



class SoundChannelEnableActionExecutor : public IUnsyncedActionExecutor {
public:
	SoundChannelEnableActionExecutor() : IUnsyncedActionExecutor("SoundChannelEnable",
			"Enable/Disable specific sound channels:"
			" UnitReply, General, Battle, UserInterface, Music") {}

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
	CreateVideoActionExecutor() : IUnsyncedActionExecutor("CreateVideo",
			"Start/Stop capturing a video of the game in progress") {}

	void Execute(const UnsyncedAction& action) const {

		// toggle
		videoCapturing->SetCapturing(!videoCapturing->IsCapturing());
		LogSystemStatus("Video capturing", videoCapturing->IsCapturing());
	}
};



class DrawTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawTreesActionExecutor() : IUnsyncedActionExecutor("DrawTrees",
			"Enable/Disable rendering of engine trees") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(treeDrawer->drawTrees, action.GetArgs());
		LogSystemStatus("rendering of engine trees", treeDrawer->drawTrees);
	}
};



class DynamicSkyActionExecutor : public IUnsyncedActionExecutor {
public:
	DynamicSkyActionExecutor() : IUnsyncedActionExecutor("DynamicSky",
			"Enable/Disable rendering of the dynamic sky") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(sky->dynamicSky, action.GetArgs());
		LogSystemStatus("rendering of the dynamic sky", sky->dynamicSky);
	}
};



class DynamicSunActionExecutor : public IUnsyncedActionExecutor {
public:
	DynamicSunActionExecutor() : IUnsyncedActionExecutor("DynamicSun",
			"Enable/Disable rendering of dynamic sun") {}

	void Execute(const UnsyncedAction& action) const {

		bool dynamicSun = sky->GetLight()->IsDynamic();
		SetBoolArg(dynamicSun, action.GetArgs());
		sky->SetLight(dynamicSun);
		LogSystemStatus("rendering of the dynamic sun", sky->GetLight()->IsDynamic());
	}
};



#ifdef USE_GML
class MultiThreadDrawGroundActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawGroundActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawGround",
			"Enable/Disable multi threaded ground rendering") {}

	void Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		SetBoolArg(gd->multiThreadDrawGround, action.GetArgs());
		LogSystemStatus("Multi threaded ground rendering", gd->multiThreadDrawGround);
	}
};



class MultiThreadDrawGroundShadowActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawGroundShadowActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawGroundShadow",
			"Enable/Disable multi threaded ground shadow rendering") {}

	void Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		SetBoolArg(gd->multiThreadDrawGroundShadow, action.GetArgs());
		LogSystemStatus("Multi threaded ground shadow rendering", gd->multiThreadDrawGroundShadow);
	}
};



class MultiThreadDrawUnitActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawUnitActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawUnit",
			"Enable/Disable multi threaded unit rendering") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(unitDrawer->multiThreadDrawUnit, action.GetArgs());
		LogSystemStatus("Multi threaded unit rendering", unitDrawer->multiThreadDrawUnit);
	}
};



class MultiThreadDrawUnitShadowActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawUnitShadowActionExecutor() : IUnsyncedActionExecutor("MultiThreadDrawUnitShadow",
			"Enable/Disable multi threaded unit shadow rendering") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(unitDrawer->multiThreadDrawUnitShadow, action.GetArgs());
		LogSystemStatus("Multi threaded unit shadow rendering", unitDrawer->multiThreadDrawUnitShadow);
	}
};



class MultiThreadDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadDrawActionExecutor() : IUnsyncedActionExecutor("MultiThreadDraw",
			"Enable/Disable multi threaded rendering") {}

	void Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

		bool mtEnabled = IsMTEnabled();
		SetBoolArg(mtEnabled, action.GetArgs());
		gd->multiThreadDrawGround = mtEnabled;
		unitDrawer->multiThreadDrawUnit = mtEnabled;
		unitDrawer->multiThreadDrawUnitShadow = mtEnabled;
		if (!mtEnabled) {
			gd->multiThreadDrawGroundShadow = false;
		}
		LogSystemStatus("Multithreaded rendering", IsMTEnabled());
	}

	static bool IsMTEnabled() {
		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		// XXX does this make sense? why would we need a random 2 of these 3?
		return
				((gd->multiThreadDrawGround ? 1 : 0)
				+ (unitDrawer->multiThreadDrawUnit ? 1 : 0)
				+ (unitDrawer->multiThreadDrawUnitShadow ? 1 : 0))
				> 1;
	}
};



class MultiThreadSimActionExecutor : public IUnsyncedActionExecutor {
public:
	MultiThreadSimActionExecutor() : IUnsyncedActionExecutor("MultiThreadSim",
			"Enable/Disable simulation multi threading") {} // FIXME misleading description

	void Execute(const UnsyncedAction& action) const {

#	if GML_ENABLE_SIM
		const bool mtEnabled = MultiThreadDrawActionExecutor::IsMTEnabled();

		// HACK GetInnerAction() should not be used here
		bool mtSim = (StringToLower(action.GetInnerAction().command) == "multithread") ? mtEnabled : gmlMultiThreadSim;
		SetBoolArg(mtSim, action.GetArgs());
		gmlMultiThreadSim = mtSim;
		gmlStartSim = 1;

		LogSystemStatus("Simulation threading", gmlMultiThreadSim);
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
	SpeedControlActionExecutor() : IUnsyncedActionExecutor("SpeedControl",
			"Set the speed-control mode to one of: 0=Default, 1=Average_CPU, 2=Maximum_CPU") {}

	void Execute(const UnsyncedAction& action) const {

		if (action.GetArgs().empty()) {
			// switch to next value
			++game->speedControl;
			if (game->speedControl > 2) {
				game->speedControl = -2;
			}
		} else {
			// set value
			game->speedControl = atoi(action.GetArgs().c_str());
		}
		// constrain to bounds
		game->speedControl = std::max(-2, std::min(game->speedControl, 2));

		net->Send(CBaseNetProtocol::Get().SendSpeedControl(gu->myPlayerNum, game->speedControl));
		logOutput.Print("Speed Control: %s", CGameServer::SpeedControlToString(game->speedControl).c_str());
		configHandler->Set("SpeedControl", game->speedControl);
		if (gameServer) {
			gameServer->UpdateSpeedControl(game->speedControl);
		}
	}
};



class GameInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	GameInfoActionExecutor() : IUnsyncedActionExecutor("GameInfo",
			"Enables/Disables game-info panel rendering") {}

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
	HideInterfaceActionExecutor() : IUnsyncedActionExecutor("HideInterface",
			"Hide/Show the GUI controlls") {}

	void Execute(const UnsyncedAction& action) const {
		SetBoolArg(game->hideInterface, action.GetArgs());
	}
};



class HardwareCursorActionExecutor : public IUnsyncedActionExecutor {
public:
	HardwareCursorActionExecutor() : IUnsyncedActionExecutor("HardwareCursor",
			"Enables/Disables hardware mouse-cursor support") {}

	void Execute(const UnsyncedAction& action) const {
// XXX one setting stored in two places (mouse->hardwareCursor & configHandler["HardwareCursor"]) -> refactor?
		SetBoolArg(mouse->hardwareCursor, action.GetArgs());
		mouse->UpdateHwCursor();
		configHandler->Set("HardwareCursor", (int)mouse->hardwareCursor);
		LogSystemStatus("Hardware mouse-cursor", mouse->hardwareCursor);
	}
};



class IncreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("IncreaseViewRadius",
			"Increase the view radius (lower performance, nicer view)") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->IncreaseDetail();
	}
};



class DecreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("DecreaseViewRadius",
			"Decrease the view radius (higher performance, uglier view)") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->DecreaseDetail();
	}
};



class MoreTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreTreesActionExecutor() : IUnsyncedActionExecutor("MoreTrees",
			"Increases the distance to the camera, in which trees are still"
			" shown (lower performance)") {}

	void Execute(const UnsyncedAction& action) const {

		treeDrawer->baseTreeDistance += 0.2f;
		ReportTreeDistance();
	}
	static void ReportTreeDistance() {
		logOutput.Print("Base tree distance %f",
				treeDrawer->baseTreeDistance * 2 * SQUARE_SIZE * TREE_SQUARE_SIZE);
	}
};



class LessTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	LessTreesActionExecutor() : IUnsyncedActionExecutor("LessTrees",
			"Decreases the distance to the camera, in which trees are still"
			" shown (higher performance)") {}

	void Execute(const UnsyncedAction& action) const {

		treeDrawer->baseTreeDistance -= 0.2f;
		MoreTreesActionExecutor::ReportTreeDistance();
	}
};



class MoreCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreCloudsActionExecutor() : IUnsyncedActionExecutor("MoreClouds",
			"Increases the density of clouds (lower performance)") {}

	void Execute(const UnsyncedAction& action) const {

		sky->IncreaseCloudDensity();
		ReportCloudDensity();
	}

	static void ReportCloudDensity() {
		logOutput.Print("Cloud density %f", 1.0f / sky->GetCloudDensity());
	}
};



class LessCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	LessCloudsActionExecutor() : IUnsyncedActionExecutor("LessClouds",
			"Decreases the density of clouds (higher performance)") {}

	void Execute(const UnsyncedAction& action) const {

		sky->DecreaseCloudDensity();
		MoreCloudsActionExecutor::ReportCloudDensity();
	}
};



class SpeedUpActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedUpActionExecutor() : IUnsyncedActionExecutor("SpeedUp",
			"Increases the simulation speed."
			" The engine will try to simulate more frames per second") {}

	void Execute(const UnsyncedAction& action) const {
		float speed = gs->userSpeedFactor;
		if (speed < 5) {
			speed += (speed < 2) ? 0.1f : 0.2f;
			float fPart = speed - (int)speed;
			if (fPart < 0.01f || fPart > 0.99f)
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
	SlowDownActionExecutor() : IUnsyncedActionExecutor("SlowDown",
			"Decreases the simulation speed."
			" The engine will try to simulate less frames per second") {}

	void Execute(const UnsyncedAction& action) const {
		float speed = gs->userSpeedFactor;
		if (speed <= 5) {
			speed -= (speed <= 2) ? 0.1f : 0.2f;
			float fPart = speed - (int)speed;
			if (fPart < 0.01f || fPart > 0.99f)
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
	ControlUnitActionExecutor() : IUnsyncedActionExecutor("ControlUnit",
			"Start to first-person-control a unit") {}

	void Execute(const UnsyncedAction& action) const {
		if (!gu->spectating) {
			// we must cause the to-be-controllee to be put in
			// netSelected[myPlayerNum] by giving it an order
			selectedUnits.SendCommand(Command(CMD_STOP));
			net->Send(CBaseNetProtocol::Get().SendDirectControl(gu->myPlayerNum));
		}
	}
};



class ShowStandardActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowStandardActionExecutor() : IUnsyncedActionExecutor("ShowStandard",
			"Disable rendering of all auxiliary map overlays") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->DisableExtraTexture();
	}
};



class ShowElevationActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowElevationActionExecutor() : IUnsyncedActionExecutor("ShowElevation",
			"Enable rendering of the auxiliary height-map overlay") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->SetHeightTexture();
	}
};



class ToggleRadarAndJammerActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleRadarAndJammerActionExecutor() : IUnsyncedActionExecutor("ToggleRadarAndJammer",
			"Enable/Disable rendering of the auxiliary radar- & jammer-map overlay") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->ToggleRadarAndJammer();
	}
};



class ShowMetalMapActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowMetalMapActionExecutor() : IUnsyncedActionExecutor("ShowMetalMap",
			"Enable rendering of the auxiliary metal-map overlay") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->SetMetalTexture(readmap->metalMap);
	}
};



class ShowPathTraversabilityActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathTraversabilityActionExecutor() : IUnsyncedActionExecutor("ShowPathTraversability",
			"Enable rendering of the auxiliary traversability-map (slope-map) overlay") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->TogglePathTraversabilityTexture();
	}
};



class ShowPathHeatActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathHeatActionExecutor() : IUnsyncedActionExecutor("ShowPathHeat",
			"Enable/Disable rendering of the path heat map overlay", true) {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->TogglePathHeatTexture();
	}
};



class ShowPathCostActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathCostActionExecutor() : IUnsyncedActionExecutor("ShowPathCost",
			"Enable rendering of the path costs map overlay", true) {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->TogglePathCostTexture();
	}
};



class ToggleLOSActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleLOSActionExecutor() : IUnsyncedActionExecutor("ToggleLOS",
			"Enable rendering of the auxiliary LOS-map overlay") {}

	void Execute(const UnsyncedAction& action) const {
		readmap->GetGroundDrawer()->ToggleLosTexture();
	}
};



class ShareDialogActionExecutor : public IUnsyncedActionExecutor {
public:
	ShareDialogActionExecutor() : IUnsyncedActionExecutor("ShareDialog",
			"Opens the share dialog, which allows you to send units and"
			" resources to other players") {}

	void Execute(const UnsyncedAction& action) const {

		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty() && (dynamic_cast<CShareBox*>(inputReceivers.front()) == NULL) && !gu->spectating) {
			new CShareBox();
		}
	}
};



class QuitMessageActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMessageActionExecutor() : IUnsyncedActionExecutor("QuitMessage",
			"Deprecated, see /Quit instead (was used to quite the game immediately)") {}

	void Execute(const UnsyncedAction& action) const {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty() && (dynamic_cast<CQuitBox*>(inputReceivers.front())) == NULL) {
			const CKeyBindings::HotkeyList quitList = keyBindings->GetHotkeys("quitmenu");
			const std::string quitKey = quitList.empty() ? "<none>" : quitList.front();
			logOutput.Print("Press %s to access the quit menu", quitKey.c_str());
		}
	}
};



class QuitMenuActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMenuActionExecutor() : IUnsyncedActionExecutor("QuitMenu",
			"Opens the quit-menu, if it is not already open") {}

	void Execute(const UnsyncedAction& action) const {
		std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (!inputReceivers.empty() && dynamic_cast<CQuitBox*>(inputReceivers.front()) == NULL) {
			new CQuitBox();
		}
	}
};



class QuitActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitActionExecutor() : IUnsyncedActionExecutor("Quit",
			"Exits the game immediately") {}

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
	IncreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("IncGUIOpacity",
			"Increases the the opacity(see-through-ness) of GUI elements") {}

	void Execute(const UnsyncedAction& action) const {

		CInputReceiver::guiAlpha = std::min(CInputReceiver::guiAlpha + 0.1f, 1.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}
};



class DecreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("DecGUIOpacity",
			"Decreases the the opacity(see-through-ness) of GUI elements") {}

	void Execute(const UnsyncedAction& action) const {

		CInputReceiver::guiAlpha = std::max(CInputReceiver::guiAlpha - 0.1f, 0.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
	}
};



class ScreenShotActionExecutor : public IUnsyncedActionExecutor {
public:
	ScreenShotActionExecutor() : IUnsyncedActionExecutor("ScreenShot",
			"Take a screen-shot of the current view") {}

	void Execute(const UnsyncedAction& action) const {
		TakeScreenshot(action.GetArgs());
	}
};



class GrabInputActionExecutor : public IUnsyncedActionExecutor {
public:
	GrabInputActionExecutor() : IUnsyncedActionExecutor("GrabInput",
			"Prevents/Enables the mouse from leaving the game window (windowed mode only)") {}

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
		LogSystemStatus("Input grabbing", (newMode == SDL_GRAB_ON));
	}
};



class ClockActionExecutor : public IUnsyncedActionExecutor {
public:
	ClockActionExecutor() : IUnsyncedActionExecutor("Clock",
			"Shows a small digital clock indicating the local time") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(game->showClock, action.GetArgs());
		configHandler->Set("ShowClock", game->showClock ? 1 : 0);
		LogSystemStatus("small digital clock", game->showClock);
	}
};



class CrossActionExecutor : public IUnsyncedActionExecutor {
public:
	CrossActionExecutor() : IUnsyncedActionExecutor("Cross",
			"Allows to exchange and modify the appearance of the"
			" cross/mouse-pointer in first-person-control view") {}

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
	FPSActionExecutor() : IUnsyncedActionExecutor("FPS",
			"Shows/Hides the frames-per-second indicator") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(game->showFPS, action.GetArgs());
		configHandler->Set("ShowFPS", game->showFPS ? 1 : 0);
		LogSystemStatus("frames-per-second indicator", game->showFPS);
	}
};



class SpeedActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedActionExecutor() : IUnsyncedActionExecutor("Speed",
			"Shows/Hides the simulation speed indicator") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(game->showSpeed, action.GetArgs());
		configHandler->Set("ShowSpeed", game->showSpeed ? 1 : 0);
		LogSystemStatus("simulation speed indicator", game->showSpeed);
	}
};



class MTInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	MTInfoActionExecutor() : IUnsyncedActionExecutor("MTInfo",
			"Shows/Hides the multi threading info panel") {}

	void Execute(const UnsyncedAction& action) const {

		bool showMTInfo = (game->showMTInfo != 0);
		SetBoolArg(showMTInfo, action.GetArgs());
		configHandler->Set("ShowMTInfo", showMTInfo ? 1 : 0);
		game->showMTInfo = (showMTInfo && (globalConfig->GetMultiThreadLua() <= 3)) ? globalConfig->GetMultiThreadLua() : 0;
	}
};



class TeamHighlightActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamHighlightActionExecutor() : IUnsyncedActionExecutor("TeamHighlight",
			"Enables/Disables uncontrolled team blinking") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			globalConfig->teamHighlight = abs(globalConfig->teamHighlight + 1) % 3;
		} else {
			globalConfig->teamHighlight = abs(atoi(action.GetArgs().c_str())) % 3;
		}
		logOutput.Print("Team highlighting: %s",
				((globalConfig->teamHighlight == 1) ? "Players only"
				: ((globalConfig->teamHighlight == 2) ? "Players and spectators"
				: "Disabled")));
		configHandler->Set("TeamHighlight", globalConfig->teamHighlight);
	}
};



class InfoActionExecutor : public IUnsyncedActionExecutor {
public:
	InfoActionExecutor() : IUnsyncedActionExecutor("Info",
			"Shows/Hides the player roster") {}

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
	CmdColorsActionExecutor() : IUnsyncedActionExecutor("CmdColors",
			"Reloads cmdcolors.txt") {}

	void Execute(const UnsyncedAction& action) const {

		const std::string fileName = action.GetArgs().empty() ? "cmdcolors.txt" : action.GetArgs();
		cmdColors.LoadConfig(fileName);
		logOutput.Print("Reloaded cmdcolors from file: %s", fileName.c_str());
	}
};



class CtrlPanelActionExecutor : public IUnsyncedActionExecutor {
public:
	CtrlPanelActionExecutor() : IUnsyncedActionExecutor("CtrlPanel",
			"Reloads ctrlpanel.txt") {}

	void Execute(const UnsyncedAction& action) const {

		const std::string fileName = action.GetArgs().empty() ? "ctrlpanel.txt" : action.GetArgs();
		guihandler->ReloadConfig(fileName);
		logOutput.Print("Reloaded ctrlpanel from file: %s", fileName.c_str());
	}
};



class FontActionExecutor : public IUnsyncedActionExecutor {
public:
	FontActionExecutor() : IUnsyncedActionExecutor("Font",
			"Reloads the fonts") {}

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
			logOutput.Print("Loaded font: %s", action.GetArgs().c_str());
			configHandler->SetString("FontFile", action.GetArgs());
			configHandler->SetString("SmallFontFile", action.GetArgs());
		}
	}
};



class VSyncActionExecutor : public IUnsyncedActionExecutor {
public:
	VSyncActionExecutor() : IUnsyncedActionExecutor("VSync",
			"Enables/Disables vertical-sync (Graphics setting)") {}

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
	SafeGLActionExecutor() : IUnsyncedActionExecutor("SafeGL",
			"Enables/Disables OpenGL save-mode") {}

	void Execute(const UnsyncedAction& action) const {

		bool saveMode = LuaOpenGL::GetSafeMode();
		SetBoolArg(saveMode, action.GetArgs());
		LuaOpenGL::SetSafeMode(saveMode);
		LogSystemStatus("OpenGL save-mode", LuaOpenGL::GetSafeMode());
	}
};



class ResBarActionExecutor : public IUnsyncedActionExecutor {
public:
	ResBarActionExecutor() : IUnsyncedActionExecutor("ResBar",
			"Shows/Hides team resource storage indicator bar") {}

	void Execute(const UnsyncedAction& action) const {
		if (resourceBar) {
			SetBoolArg(resourceBar->enabled, action.GetArgs());
		}
	}
};



class ToolTipActionExecutor : public IUnsyncedActionExecutor {
public:
	ToolTipActionExecutor() : IUnsyncedActionExecutor("ToolTip",
			"Enables/Disables the general tool-tips, displayed when hovering"
			" over units. features or the map") {}

	void Execute(const UnsyncedAction& action) const {
		if (tooltip) {
			SetBoolArg(tooltip->enabled, action.GetArgs());
		}
	}
};



class ConsoleActionExecutor : public IUnsyncedActionExecutor {
public:
	ConsoleActionExecutor() : IUnsyncedActionExecutor("Console",
			"Enables/Disables the in-game console") {}

	void Execute(const UnsyncedAction& action) const {
		if (game->infoConsole) {
			SetBoolArg(game->infoConsole->enabled, action.GetArgs());
		}
	}
};



class EndGraphActionExecutor : public IUnsyncedActionExecutor {
public:
	EndGraphActionExecutor() : IUnsyncedActionExecutor("EndGraph",
			"Enables/Disables the statistics graphs shown at the end of the game") {}

	void Execute(const UnsyncedAction& action) const {
		SetBoolArg(CEndGameBox::enabled, action.GetArgs());
	}
};



class FPSHudActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSHudActionExecutor() : IUnsyncedActionExecutor("FPSHud",
			"Enables/Disables HUD (GUI interface) shown in first-person-control mode") {}

	void Execute(const UnsyncedAction& action) const {

		bool drawHUD = hudDrawer->GetDraw();
		SetBoolArg(drawHUD, action.GetArgs());
		hudDrawer->SetDraw(drawHUD);
	}
};



class DebugDrawAIActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugDrawAIActionExecutor() : IUnsyncedActionExecutor("DebugDrawAI",
			"Enables/Disables debug drawing for AIs") {}

	void Execute(const UnsyncedAction& action) const {

		bool aiDebugDraw = debugDrawerAI->GetDraw();
		SetBoolArg(aiDebugDraw, action.GetArgs());
		debugDrawerAI->SetDraw(aiDebugDraw);
		LogSystemStatus("SkirmishAI debug drawing", debugDrawerAI->GetDraw());
	}
};



class MapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	MapMarksActionExecutor() : IUnsyncedActionExecutor("MapMarks",
			"Enables/Disables map marks rendering") {}

	void Execute(const UnsyncedAction& action) const {

		SetBoolArg(globalRendering->drawMapMarks, action.GetArgs());
		LogSystemStatus("map marks rendering", globalRendering->drawMapMarks);
	}
};



class AllMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	AllMapMarksActionExecutor() : IUnsyncedActionExecutor("AllMapMarks",
			"Show/Hide all map marks drawn so far", true) {}

	void Execute(const UnsyncedAction& action) const {

		bool allMarksVisible = inMapDrawerModel->GetAllMarksVisible();
		SetBoolArg(allMarksVisible, action.GetArgs());
		inMapDrawerModel->SetAllMarksVisible(allMarksVisible);
	}
};



class ClearMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	ClearMapMarksActionExecutor() : IUnsyncedActionExecutor("ClearMapMarks",
			"Remove all map marks drawn so far") {}

	void Execute(const UnsyncedAction& action) const {
		inMapDrawerModel->EraseAll();
	}
};



// XXX unlucky command-name, remove the "No"
class NoLuaDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	NoLuaDrawActionExecutor() : IUnsyncedActionExecutor("NoLuaDraw",
			"Allow/Disallow Lua to draw on the map") {}

	void Execute(const UnsyncedAction& action) const {

		bool luaMapDrawingAllowed = inMapDrawer->GetLuaMapDrawingAllowed();
		SetBoolArg(luaMapDrawingAllowed, action.GetArgs());
		inMapDrawer->SetLuaMapDrawingAllowed(luaMapDrawingAllowed);
	}
};



class LuaUIActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaUIActionExecutor() : IUnsyncedActionExecutor("LuaUI",
			"Allow/Disallow Lua to draw (GUI elements)") {}

	void Execute(const UnsyncedAction& action) const {
		if (guihandler != NULL) {
			guihandler->PushLayoutCommand(action.GetArgs());
		}
	}
};



class LuaModUICtrlActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaModUICtrlActionExecutor() : IUnsyncedActionExecutor("LuaModUICtrl",
			"Allow/Disallow Lua to receive UI control events, like mouse-,"
			" keyboard- and joystick-events") {}

	void Execute(const UnsyncedAction& action) const {

		bool modUICtrl = CLuaHandle::GetModUICtrl();
		SetBoolArg(modUICtrl, action.GetArgs());
		CLuaHandle::SetModUICtrl(modUICtrl);
		configHandler->Set("LuaModUICtrl", modUICtrl ? 1 : 0);
	}
};



class MiniMapActionExecutor : public IUnsyncedActionExecutor {
public:
	MiniMapActionExecutor() : IUnsyncedActionExecutor("MiniMap",
			"Show/Hide the mini-map provided by the engine") {}

	void Execute(const UnsyncedAction& action) const {
		if (minimap != NULL) {
			minimap->ConfigCommand(action.GetArgs());
		}
	}
};



class GroundDecalsActionExecutor : public IUnsyncedActionExecutor {
public:
	GroundDecalsActionExecutor() : IUnsyncedActionExecutor("GroundDecals",
			"Disable/Enable ground-decals rendering."
			" Ground-decals are things like scars appearing on the map after an"
			" explosion.") {}

	void Execute(const UnsyncedAction& action) const {
		if (groundDecals) {
			bool drawDecals = groundDecals->GetDrawDecals();
			SetBoolArg(drawDecals, action.GetArgs());
			groundDecals->SetDrawDecals(drawDecals);
		}
		LogSystemStatus("Ground-decals rendering",
				(groundDecals != NULL) ? groundDecals->GetDrawDecals() : false);
	}
};



class MaxParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxParticlesActionExecutor() : IUnsyncedActionExecutor("MaxParticles",
			"Set the maximum number of particles (Graphics setting)") {}

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
	MaxNanoParticlesActionExecutor() : IUnsyncedActionExecutor("MaxNanoParticles",
			"Set the maximum number of nano-particles (Graphic setting)") {}

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
	GatherModeActionExecutor() : IUnsyncedActionExecutor("GatherMode",
			"Enter/Leave gather-wait command mode") {}

	void Execute(const UnsyncedAction& action) const {
		if (guihandler != NULL) {
			bool gatherMode = guihandler->GetGatherMode();
			SetBoolArg(gatherMode, action.GetArgs());
			guihandler->SetGatherMode(gatherMode);
			LogSystemStatus("Gather-Mode", guihandler->GetGatherMode());
		}
	}
};



class PasteTextActionExecutor : public IUnsyncedActionExecutor {
public:
	PasteTextActionExecutor() : IUnsyncedActionExecutor("PasteText",
			"Paste either the argument string(s) or if none given, the content of the clip-board to chat input") {}

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
	BufferTextActionExecutor() : IUnsyncedActionExecutor("BufferText",
			"Write the argument string(s) directly to the console history,"
			" but not anywhere else. This is useful for fast manual issuing"
			" of the command, later on") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			game->consoleHistory->AddLine(action.GetArgs());
		}
	}
};



class InputTextGeoActionExecutor : public IUnsyncedActionExecutor {
public:
	InputTextGeoActionExecutor() : IUnsyncedActionExecutor("InputTextGeo",
			"Move and/or resize the input-text field (the \"Say: \" thing)") {}

	void Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			game->ParseInputTextGeometry(action.GetArgs());
		}
	}
};



class DistIconActionExecutor : public IUnsyncedActionExecutor {
public:
	DistIconActionExecutor() : IUnsyncedActionExecutor("DistIcon",
			"Set the distance between units and camera, at which they turn"
			" into icons (Graphic setting)") {}

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
	DistDrawActionExecutor() : IUnsyncedActionExecutor("DistDraw",
			"Set the distance between units and camera, at which they turn"
			" into far-textures (flat/texture-only representation) (Graphic setting)") {}

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
	LODScaleActionExecutor() : IUnsyncedActionExecutor("LODScale",
			"Set the scale for either of: LOD (level-of-detail),"
			" shadow-LOD, reflection-LOD, refraction-LOD") {}

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
	WireMapActionExecutor() : IUnsyncedActionExecutor("WireMap",
			"Enable/Disable drawing of the map as wire-frame (no textures) (Graphic setting)") {}

	void Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
		SetBoolArg(gd->wireframe, action.GetArgs());
		sky->wireframe = gd->wireframe;
		LogSystemStatus("drawing of the map as wire-frame", gd->wireframe);
	}
};



class SetGammaActionExecutor : public IUnsyncedActionExecutor {
public:
	SetGammaActionExecutor() : IUnsyncedActionExecutor("SetGamma",
			"Set the rendering gamma value(s) through SDL") {}

	void Execute(const UnsyncedAction& action) const {
		float r, g, b;
		const int count = sscanf(action.GetArgs().c_str(), "%f %f %f", &r, &g, &b);
		if (count == 1) {
			SDL_SetGamma(r, r, r);
			logOutput.Print("Set gamma value");
		} else if (count == 3) {
			SDL_SetGamma(r, g, b);
			logOutput.Print("Set gamma values");
		} else {
			logOutput.Print("Unknown gamma format");
		}
	}
};



class CrashActionExecutor : public IUnsyncedActionExecutor {
public:
	CrashActionExecutor() : IUnsyncedActionExecutor("Crash",
			"Invoke an artificial crash through a NULL-pointer dereference (SIGSEGV)", true) {}

	void Execute(const UnsyncedAction& action) const {
		int* a = 0;
		*a = 0;
	}
};



class ExceptionActionExecutor : public IUnsyncedActionExecutor {
public:
	ExceptionActionExecutor() : IUnsyncedActionExecutor("Exception",
			"Invoke an artificial crash by throwing an std::runtime_error", true) {}

	void Execute(const UnsyncedAction& action) const {
		throw std::runtime_error("Exception test");
	}
};



class DivByZeroActionExecutor : public IUnsyncedActionExecutor {
public:
	DivByZeroActionExecutor() : IUnsyncedActionExecutor("DivByZero",
			"Invoke an artificial crash by performing a division-by-Zero", true) {}

	void Execute(const UnsyncedAction& action) const {
		float a = 0;
		logOutput.Print("Result: %f", 1.0f/a);
	}
};



class GiveActionExecutor : public IUnsyncedActionExecutor {
public:
	GiveActionExecutor() : IUnsyncedActionExecutor("Give",
			"Places one or multiple units of a single or multiple types on the"
			" map, instantly; by default to your own team", true) {}

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
	DestroyActionExecutor() : IUnsyncedActionExecutor("Destroy",
			"Destroys one or multiple units by unit-ID, instantly", true) {}

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
	SendActionExecutor() : IUnsyncedActionExecutor("Send",
			"Send a string as raw network message to the game host (for debugging only)") {}

	void Execute(const UnsyncedAction& action) const {
		CommandMessage pckt(Action(action.GetArgs()), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
};



class SaveGameActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveGameActionExecutor() : IUnsyncedActionExecutor("SaveGame",
			"Save the game state to QuickSave.ssf (BROKEN)") {}

	void Execute(const UnsyncedAction& action) const {
		game->SaveGame("Saves/QuickSave.ssf", true);
	}
};



class DumpStateActionExecutor: public IUnsyncedActionExecutor {
public:
	DumpStateActionExecutor(): IUnsyncedActionExecutor("DumpState", "dump game-state to file") {}

	void Execute(const UnsyncedAction& action) const {
		const std::vector<std::string>& args = _local_strSpaceTokenize(action.GetArgs());

		switch (args.size()) {
			case 2: { game->DumpState(atoi(args[0].c_str()), atoi(args[1].c_str()),                     1); } break;
			case 3: { game->DumpState(atoi(args[0].c_str()), atoi(args[1].c_str()), atoi(args[2].c_str())); } break;
			default: {} break;
		}
	}
};



/// /save [-y ]<savename>
class SaveActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveActionExecutor() : IUnsyncedActionExecutor("Save",
			"Save the game state to a specific file (BROKEN)") {}

	void Execute(const UnsyncedAction& action) const {

		const bool saveOverride = action.GetArgs().find("-y ") == 0;
		const std::string saveName(action.GetArgs().c_str() + (saveOverride ? 3 : 0));
		const std::string saveFileName = "Saves/" + saveName + ".ssf";
		game->SaveGame(saveFileName, saveOverride);
	}
};



class ReloadGameActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadGameActionExecutor() : IUnsyncedActionExecutor("ReloadGame",
			"Restarts the game with the initially provided start-script") {}

	void Execute(const UnsyncedAction& action) const {
		game->ReloadGame();
	}
};



class DebugInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugInfoActionExecutor() : IUnsyncedActionExecutor("DebugInfo",
			"Print debug info to the chat/log-file about either:"
			" sound, profiling") {}

	void Execute(const UnsyncedAction& action) const {
		if (action.GetArgs() == "sound") {
			sound->PrintDebugInfo();
		} else if (action.GetArgs() == "profiling") {
			profiler.PrintProfilingInfo();
		} else {
			logOutput.Print("Give either of these as argument: sound, profiling");
		}
	}
};



class BenchmarkScriptActionExecutor : public IUnsyncedActionExecutor {
public:
	// XXX '-' in command name is inconsistent with the rest of the commands, which only use "[a-zA-Z]" -> remove it
	BenchmarkScriptActionExecutor() : IUnsyncedActionExecutor("Benchmark-Script",
			"Runs the benchmark-script for a given unit-type") {}

	void Execute(const UnsyncedAction& action) const {
		CUnitScript::BenchmarkScript(action.GetArgs());
	}
};



class RedirectToSyncedActionExecutor : public IUnsyncedActionExecutor {
public:
	RedirectToSyncedActionExecutor(const std::string& command)
		: IUnsyncedActionExecutor(command,
			"Redirects command /" + command + " to its synced processor")
	{}

	void Execute(const UnsyncedAction& action) const {
		// redirect as a synced command
		CommandMessage pckt(action.GetInnerAction(), gu->myPlayerNum);
		net->Send(pckt.Pack());
	}
};



class CommandListActionExecutor : public IUnsyncedActionExecutor {
public:
	CommandListActionExecutor() : IUnsyncedActionExecutor("CommandList",
			"Prints all the available chat commands with description"
			" (if available) to the console") {}

	void Execute(const UnsyncedAction& action) const {

		logOutput.Print("Chat commands plus description");
		logOutput.Print("==============================");
		PrintToConsole(syncedGameCommands->GetActionExecutors());
		PrintToConsole(unsyncedGameCommands->GetActionExecutors());
	}

	template<class action_t, bool synced_v>
	static void PrintExecutorToConsole(const IActionExecutor<action_t, synced_v>* executor) {

		logOutput.Print("/%-30s  %s  %s",
				executor->GetCommand().c_str(),
				(executor->IsSynced() ? "(synced)  " : "(unsynced)"),
				executor->GetDescription().c_str());
	}

private:
	void PrintToConsole(const std::map<std::string, ISyncedActionExecutor*>& executors) const {

		std::map<std::string, ISyncedActionExecutor*>::const_iterator aei;
		for (aei = executors.begin(); aei != executors.end(); ++aei) {
			PrintExecutorToConsole(aei->second);
		}
	}

	void PrintToConsole(const std::map<std::string, IUnsyncedActionExecutor*>& executors) const {

		std::map<std::string, IUnsyncedActionExecutor*>::const_iterator aei;
		for (aei = executors.begin(); aei != executors.end(); ++aei) {
			PrintExecutorToConsole(aei->second);
		}
	}
};



class CommandHelpActionExecutor : public IUnsyncedActionExecutor {
public:
	CommandHelpActionExecutor() : IUnsyncedActionExecutor("CommandHelp",
			"Prints info about a specific chat command"
			" (so far only synced/unsynced and the description)") {}

	void Execute(const UnsyncedAction& action) const {

		const std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs(), 1);
		if (!args.empty()) {
			const std::string commandLower = StringToLower(args[0]);

			// try if an unsynced chat command with this name is available
			const IUnsyncedActionExecutor* unsyncedExecutor = unsyncedGameCommands->GetActionExecutor(commandLower);
			if (unsyncedExecutor != NULL) {
				PrintExecutorHelpToConsole(unsyncedExecutor);
				return;
			}

			// try if a synced chat command with this name is available
			const ISyncedActionExecutor* syncedExecutor = syncedGameCommands->GetActionExecutor(commandLower);
			if (syncedExecutor != NULL) {
				PrintExecutorHelpToConsole(syncedExecutor);
				return;
			}

			logOutput.Print("No chat command registered with name \"%s\" (case-insensitive)", args[0].c_str());
		} else {
			logOutput.Print("missing command-name");
		}
	}

private:
	template<class action_t, bool synced_v>
	static void PrintExecutorHelpToConsole(const IActionExecutor<action_t, synced_v>* executor) {

		// XXX extend this in case more info about commands are available (for example "Usage: name {args}")
		CommandListActionExecutor::PrintExecutorToConsole(executor);
	}
};

} // namespace (unnamed)





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




void UnsyncedGameCommands::AddDefaultActionExecutors() {

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
	AddActionExecutor(new SetGammaActionExecutor());
	AddActionExecutor(new CrashActionExecutor());
	AddActionExecutor(new ExceptionActionExecutor());
	AddActionExecutor(new DivByZeroActionExecutor());
	AddActionExecutor(new GiveActionExecutor());
	AddActionExecutor(new DestroyActionExecutor());
	AddActionExecutor(new SendActionExecutor());
	AddActionExecutor(new SaveGameActionExecutor());
	AddActionExecutor(new DumpStateActionExecutor());
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
	AddActionExecutor(new CommandListActionExecutor());
	AddActionExecutor(new CommandHelpActionExecutor());
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
		// this might happen during shutdown after an unclean init
		logOutput.Print("Warning: UnsyncedGameCommands singleton was not initialized or is already destroyed");
	}
}
