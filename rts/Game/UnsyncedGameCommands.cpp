/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnsyncedGameCommands.h"

#include "UnsyncedActionExecutor.h"
#include "SyncedGameCommands.h"
#include "SyncedActionExecutor.h"
#include "Game.h"
#include "Action.h"
#include "CameraHandler.h"
#include "ConsoleHistory.h"
#include "CommandMessage.h"
#include "GameSetup.h"
#include "GlobalUnsynced.h"
#include "SelectedUnitsHandler.h"
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
#include "Game/GUI/PlayerRoster.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Net/GameServer.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Map/SMF/ROAM/Patch.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/HUDDrawer.h"
#include "Rendering/Screenshot.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/TeamHighlight.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/VerticalSync.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaUI.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "UI/CommandColors.h"
#include "UI/EndGameBox.h"
#include "UI/GameInfo.h"
#include "UI/GuiHandler.h"
#include "UI/InfoConsole.h"
#include "UI/InputReceiver.h"
#include "UI/KeyBindings.h"
#include "UI/KeyCodes.h"
#include "UI/MiniMap.h"
#include "UI/QuitBox.h"
#include "UI/ResourceBar.h"
#include "UI/SelectionKeyHandler.h"
#include "UI/ShareBox.h"
#include "UI/TooltipConsole.h"
#include "UI/UnitTracker.h"
#include "UI/ProfileDrawer.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/GlobalConfig.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/DumpState.h"
#include "System/Util.h"
#include "System/EventHandler.h"

#include <SDL_events.h>


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
		ProcessAction(fakeAction, key, false);
		return true;
	}

	return false;
}

// TODO CGame stuff in UnsyncedGameCommands: refactor (or move)
bool CGame::ProcessKeyPressAction(unsigned int key, const Action& action) {
	if (!userWriting)
		return false;

	if (action.command == "edit_return") {
		userWriting = false;
		writingPos = 0;

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
				LOG_L(L_DEBUG, "%s", command.c_str());

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
		const vector<string>& partials = wordCompletion->Complete(head);
		userInput = head + tail;
		writingPos = (int)head.length();

		if (!partials.empty()) {
			string msg;
			for (unsigned int i = 0; i < partials.size(); i++) {
				msg += "  ";
				msg += partials[i];
			}
			LOG("%s", msg.c_str());
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
		if ((userInput.find_first_of("aA") == 0) && (userInput[1] == ':')) {
			// already are in ally chat -> toggle it off
			userInput = userInput.substr(2);
			writingPos = std::max(0, writingPos - 2);
			userInputPrefix = "";
		}
		else if ((userInput.find_first_of("sS") == 0) && (userInput[1] == ':')) {
			// are in spec chat -> switch it to ally chat
			userInput[0] = 'a';
			userInputPrefix = "a:";
		} else {
			userInput = "a:" + userInput;
			writingPos += 2;
			userInputPrefix = "a:";
		}
		return true;
	}
	else if (action.command == "chatswitchspec") {
		if ((userInput.find_first_of("sS") == 0) && (userInput[1] == ':')) {
			// already are in spec chat -> toggle it off
			userInput = userInput.substr(2);
			writingPos = std::max(0, writingPos - 2);
			userInputPrefix = "";
		}
		else if ((userInput.find_first_of("aA") == 0) && (userInput[1] == ':')) {
			// are in ally chat -> switch it to spec chat
			userInput[0] = 's';
			userInputPrefix = "s:";
		} else {
			userInput = "s:" + userInput;
			writingPos += 2;
			userInputPrefix = "s:";
		}
		return true;
	}
	else if (action.command == "pastetext") {
		//we cannot use extra commands because tokenization strips multiple spaces
		//or even trailing spaces, the text should be copied verbatim
		const std::string pastecommand = "pastetext ";
		if (action.rawline.length() > pastecommand.length()) {
			userInput.insert(writingPos, action.rawline.substr(pastecommand.length(), action.rawline.length()-pastecommand.length()));
			writingPos += action.rawline.length()-pastecommand.length();
		} else {
			PasteClipboard();
		}
		return true;
	}

	else if (action.command == "edit_backspace") {
		if (!userInput.empty() && (writingPos > 0)) {
			int prev=Utf8PrevChar(userInput,writingPos);
			userInput.erase(prev, writingPos-prev);
			writingPos=prev;
		}
		return true;
	}
	else if (action.command == "edit_delete") {
		if (!userInput.empty() && (writingPos < (int)userInput.size())) {
			userInput.erase(writingPos, Utf8CharLen(userInput,writingPos));
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
		writingPos = Utf8PrevChar(userInput,writingPos);
		return true;
	}
	else if (action.command == "edit_next_char") {
		writingPos = Utf8NextChar(userInput,writingPos);
		return true;
	}
	else if (action.command == "edit_prev_word") { //TODO It don't seems to work correctly with utf-8
		// prev word
		const char* s = userInput.c_str();
		int p = writingPos;
		while ((p > 0) && !isalnum(s[p - 1])) { p--; }
		while ((p > 0) &&  isalnum(s[p - 1])) { p--; }
		writingPos = p;
		return true;
	}
	else if (action.command == "edit_next_word") { //TODO It don't seems to work correctly with utf-8
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


namespace { // prevents linking problems in case of duplicate symbols

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

	bool Execute(const UnsyncedAction& action) const {
		return innerExecutor->ExecuteAction(action);
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

	bool Execute(const UnsyncedAction& action) const {

		std::vector<IUnsyncedActionExecutor*>::const_iterator ei;
		for (ei = innerExecutors.begin(); ei != innerExecutors.end(); ++ei) {
			(*ei)->ExecuteAction(action);
		}
		return true;
	}

private:
	std::vector<IUnsyncedActionExecutor*> innerExecutors;
};



class SelectActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectActionExecutor() : IUnsyncedActionExecutor("Select",
			"<chat command description: Select>") {} // TODO

	bool Execute(const UnsyncedAction& action) const {
		selectionKeys->DoSelection(action.GetArgs()); //TODO give it a return argument?
		return true;
	}
};



class SelectUnitsActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectUnitsActionExecutor() : IUnsyncedActionExecutor("SelectUnits",
			"<chat command description: SelectUnits>") {} // TODO

	bool Execute(const UnsyncedAction& action) const {
		selectedUnitsHandler.SelectUnits(action.GetArgs()); //TODO give it a return argument?
		return true;
	}
};



class SelectCycleActionExecutor : public IUnsyncedActionExecutor {
public:
	SelectCycleActionExecutor() : IUnsyncedActionExecutor("SelectCycle",
			"<chat command description: SelectUnits>") {} // TODO

	bool Execute(const UnsyncedAction& action) const {
		selectedUnitsHandler.SelectCycle(action.GetArgs()); //TODO give it a return argument?
		return true;
	}
};



class DeselectActionExecutor : public IUnsyncedActionExecutor {
public:
	DeselectActionExecutor() : IUnsyncedActionExecutor("Deselect",
			"Deselects all currently selected units") {}

	bool Execute(const UnsyncedAction& action) const {
		selectedUnitsHandler.ClearSelected();
		return true;
	}
};



class RoamActionExecutor : public IUnsyncedActionExecutor {
public:
	RoamActionExecutor() : IUnsyncedActionExecutor("roam",
			"Disables/Enables ROAM mesh rendering: 0=off, 1=on") {}

	bool Execute(const UnsyncedAction& action) const {
		CSMFGroundDrawer* smfGD = dynamic_cast<CSMFGroundDrawer*>(readMap->GetGroundDrawer());
		if (!smfGD)
			return false;

		if (!action.GetArgs().empty()) {
			int useRoam = -1;
			int roamMode = -1;
			sscanf((action.GetArgs()).c_str(), "%i %i", &useRoam, &roamMode);

			smfGD->SwitchMeshDrawer(useRoam);
			if (useRoam && roamMode >= 0) {
				Patch::SwitchRenderMode(roamMode);
			}
		} else {
			smfGD->SwitchMeshDrawer();
		}
		return true;
	}
};


class MapBorderActionExecutor : public IUnsyncedActionExecutor {
public:
	MapBorderActionExecutor() : IUnsyncedActionExecutor("MapBorder",
			"Set or toggle map border rendering") {}

	bool Execute(const UnsyncedAction& action) const {
		CSMFGroundDrawer* smfGD = dynamic_cast<CSMFGroundDrawer*>(readMap->GetGroundDrawer());
		if (!smfGD)
			return false;

		if (!action.GetArgs().empty()) {
			bool enable = true;
			SetBoolArg(enable, action.GetArgs());

			if (enable != smfGD->ToggleMapBorder())
				smfGD->ToggleMapBorder();

		} else {
			smfGD->ToggleMapBorder();
		}
		return true;
	}
};


class ShadowsActionExecutor : public IUnsyncedActionExecutor {
public:
	ShadowsActionExecutor() : IUnsyncedActionExecutor("Shadows",
			"Disables/Enables shadows rendering: -1=disabled, 0=off,"
			" 1=full shadows, 2=skip terrain shadows") {}

	bool Execute(const UnsyncedAction& action) const {
		if (shadowHandler->shadowConfig < 0) {
			LOG_L(L_WARNING, "Shadows are disabled; change your configuration and restart to use them");
			return true;
		}
		if (!shadowHandler->shadowsSupported) {
			LOG_L(L_WARNING, "Your hardware/driver setup does not support shadows");
			return true;
		}

		shadowHandler->Reload(((action.GetArgs()).empty())? NULL: (action.GetArgs()).c_str());
		LOG("Set \"shadows\" config-parameter to %i", shadowHandler->shadowConfig);
		return true;
	}
};



class WaterActionExecutor : public IUnsyncedActionExecutor {
public:
	WaterActionExecutor() : IUnsyncedActionExecutor("Water",
			"Set water rendering mode: 0=basic, 1=reflective, 2=dynamic"
			", 3=reflective&refractive?, 4=bump-mapped") {}

	bool Execute(const UnsyncedAction& action) const {

		int nextWaterRendererMode = 0;
		if (!action.GetArgs().empty()) {
			nextWaterRendererMode = std::max(0, atoi(action.GetArgs().c_str()) % IWater::NUM_WATER_RENDERERS);
		} else {
			nextWaterRendererMode = -1;
		}

		IWater::PushWaterMode(nextWaterRendererMode);
		return true;
	}
};



class AdvModelShadingActionExecutor : public IUnsyncedActionExecutor {
public:
	AdvModelShadingActionExecutor() : IUnsyncedActionExecutor("AdvModelShading",
			"Set or toggle advanced model shading mode") {}

	bool Execute(const UnsyncedAction& action) const {
		static bool canUseShaders = unitDrawer->UseAdvShading();

		if (!canUseShaders)
			return false;

		SetBoolArg(unitDrawer->UseAdvShadingRef(), action.GetArgs());
		LogSystemStatus("model shaders", unitDrawer->UseAdvShading());
		return true;
	}
};



class AdvMapShadingActionExecutor : public IUnsyncedActionExecutor {
public:
	AdvMapShadingActionExecutor() : IUnsyncedActionExecutor("AdvMapShading",
			"Set or toggle advanced map shading mode") {}

	bool Execute(const UnsyncedAction& action) const {

		CBaseGroundDrawer* gd = readMap->GetGroundDrawer();
		static bool canUseShaders = gd->UseAdvShading();

		if (!canUseShaders)
			return false;

		SetBoolArg(gd->UseAdvShadingRef(), action.GetArgs());
		LogSystemStatus("map shaders", gd->UseAdvShading());
		return true;
	}
};



class SayActionExecutor : public IUnsyncedActionExecutor {
public:
	SayActionExecutor() : IUnsyncedActionExecutor("Say",
			"Say something in (public) chat") {}

	bool Execute(const UnsyncedAction& action) const {
		game->SendNetChat(action.GetArgs());
		return true;
	}
};



class SayPrivateActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateActionExecutor() : IUnsyncedActionExecutor("W",
			"Say something in private to a specific player, by player-name") {}

	bool Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string name = action.GetArgs().substr(0, pos);
			const int playerID = playerHandler->Player(name);
			if (playerID >= 0) {
				game->SendNetChat(action.GetArgs().substr(pos+1), playerID);
			} else {
				LOG_L(L_WARNING, "/w: Player not found: %s", name.c_str());
			}
		} else {
			LOG_L(L_WARNING, "/w: wrong syntax (which is '/w %%playername')");
		}
		return true;
	}
};



class SayPrivateByPlayerIDActionExecutor : public IUnsyncedActionExecutor {
public:
	SayPrivateByPlayerIDActionExecutor() : IUnsyncedActionExecutor("WByNum",
			"Say something in private to a specific player, by player-ID") {}

	bool Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			std::istringstream buf(action.GetArgs().substr(0, pos));
			int playerID;
			buf >> playerID;
			if (playerID >= 0) {
				game->SendNetChat(action.GetArgs().substr(pos+1), playerID);
			} else {
				LOG_L(L_WARNING, "Player-ID invalid: %i", playerID);
			}
		} else {
			LOG_L(L_WARNING, "/WByNum: wrong syntax (which is '/WByNum %%playerid')");
		}
		return true;
	}
};



class EchoActionExecutor : public IUnsyncedActionExecutor {
public:
	EchoActionExecutor() : IUnsyncedActionExecutor("Echo",
			"Write a string to the log file") {}

	bool Execute(const UnsyncedAction& action) const {
		LOG("%s", action.GetArgs().c_str());
		return true;
	}
};



class SetActionExecutor : public IUnsyncedActionExecutor {
public:
	SetActionExecutor() : IUnsyncedActionExecutor("Set",
			"Set a config key=value pair") {}

	bool Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.GetArgs().substr(0, pos);
			configHandler->SetString(varName, action.GetArgs().substr(pos+1));
		} else {
			LOG_L(L_WARNING, "/set: wrong syntax (which is '/set %%cfgtag %%cfgvalue')");
		}
		return true;
	}
};



class SetOverlayActionExecutor : public IUnsyncedActionExecutor {
public:
	SetOverlayActionExecutor() : IUnsyncedActionExecutor("TSet",
			"Set a config key=value pair in the overlay, meaning it will not be"
			" persisted for future games") {}

	bool Execute(const UnsyncedAction& action) const {
		const std::string::size_type pos = action.GetArgs().find_first_of(" ");
		if (pos != std::string::npos) {
			const std::string varName = action.GetArgs().substr(0, pos);
			configHandler->SetString(varName, action.GetArgs().substr(pos+1), true);
		} else {
			LOG_L(L_WARNING, "/tset: wrong syntax (which is '/tset %%cfgtag %%cfgvalue')");
		}
		return true;
	}
};



class EnableDrawInMapActionExecutor : public IUnsyncedActionExecutor {
public:
	EnableDrawInMapActionExecutor() : IUnsyncedActionExecutor("DrawInMap",
			"Enables drawing on the map") {}

	bool Execute(const UnsyncedAction& action) const {
		inMapDrawer->SetDrawMode(true);
		return true;
	}
};



class DrawLabelActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawLabelActionExecutor() : IUnsyncedActionExecutor("DrawLabel",
			"Draws a label on the map at the current mouse-pointer position") {}

	bool Execute(const UnsyncedAction& action) const {
		const float3 pos = inMapDrawer->GetMouseMapPos();
		if (pos.x >= 0) {
			inMapDrawer->SetDrawMode(false);
			inMapDrawer->PromptLabel(pos);
		} else {
			LOG_L(L_WARNING, "/DrawLabel: move mouse over the map");
		}
		return true;
	}
};



class MouseActionExecutor : public IUnsyncedActionExecutor {
public:
	MouseActionExecutor(int button)
		: IUnsyncedActionExecutor("Mouse" + IntToString(button),
			"Simulates a mouse button press of button " + IntToString(button))
		, button(button)
	{}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.IsRepeat()) {
			mouse->MousePress(mouse->lastx, mouse->lasty, button);
		}
		return true;
	}

private:
	int button;
};



class ViewSelectionActionExecutor : public IUnsyncedActionExecutor {
public:
	ViewSelectionActionExecutor() : IUnsyncedActionExecutor("ViewSelection",
			"Moves the camera to the center of the currently selected units") {}

	bool Execute(const UnsyncedAction& action) const {
		const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
		if (selUnits.empty())
			return false;

		// XXX this code is duplicated in CGroup::CalculateCenter(), move to CUnitSet maybe
		float3 pos;
		CUnitSet::const_iterator ui;
		for (ui = selUnits.begin(); ui != selUnits.end(); ++ui) {
			pos += (*ui)->midPos;
		}
		pos /= (float)selUnits.size();
		camHandler->CameraTransition(0.6f);
		camHandler->GetCurrentController().SetPos(pos);
		return true;
	}
};



class CameraMoveActionExecutor : public IUnsyncedActionExecutor {
public:
	CameraMoveActionExecutor(int moveStateIdx, const std::string& commandPostfix)
		: IUnsyncedActionExecutor("Move" + commandPostfix, "Moves the camera " + commandPostfix + " a bit")
		, moveStateIdx(moveStateIdx)
	{}

	bool Execute(const UnsyncedAction& action) const {
		camera->SetMovState(moveStateIdx, true);
		return true;
	}

private:
	int moveStateIdx;
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

	bool Execute(const UnsyncedAction& action) const {
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
			int teamToReceiveUnitsId = -1;
			int teamToKillId = atoi(args[0].c_str());

			if ((args.size() >= 2) && kill) {
				teamToReceiveUnitsId = atoi(args[1].c_str());
				share = true;
			}

			CTeam* teamToKill = (teamHandler->IsActiveTeam(teamToKillId))? teamHandler->Team(teamToKillId) : NULL;
			const CTeam* teamToReceiveUnits = (teamHandler->IsActiveTeam(teamToReceiveUnitsId))? teamHandler->Team(teamToReceiveUnitsId): NULL;

			if (teamToKill == NULL) {
				LOG_L(L_WARNING, "Team to %s: not a valid team number: \"%s\"", actionName.c_str(), args[0].c_str());
				badArgs = true;
			}
			if (share && teamToReceiveUnits == NULL) {
				LOG_L(L_WARNING, "Team to receive units: not a valid team number: \"%s\"", args[1].c_str());
				badArgs = true;
			}
			if (!badArgs && (skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId).size() == 0)) {
				LOG_L(L_WARNING, "Team to %s: not a Skirmish AI team: %i", actionName.c_str(), teamToKillId);
				badArgs = true;
			} else {
				const CSkirmishAIHandler::ids_t skirmishAIIds = skirmishAIHandler.GetSkirmishAIsInTeam(teamToKillId, gu->myPlayerNum);
				if (!skirmishAIIds.empty()) {
					skirmishAIId = skirmishAIIds[0];
				} else {
					LOG_L(L_WARNING, "Team to %s: not a local Skirmish AI team: %i", actionName.c_str(), teamToKillId);
					badArgs = true;
				}
			}
			if (!badArgs && skirmishAIHandler.GetSkirmishAI(skirmishAIId)->isLuaAI) {
				LOG_L(L_WARNING, "Team to %s: it is not yet supported to %s Lua AIs", actionName.c_str(), actionName.c_str());
				badArgs = true;
			}
			if (!badArgs) {
				const bool weAreAllied  = teamHandler->AlliedTeams(fromTeamId, teamToKillId);
				const bool weAreAIHost  = (skirmishAIHandler.GetSkirmishAI(skirmishAIId)->hostPlayer == gu->myPlayerNum);
				const bool weAreLeader  = (teamToKill->GetLeader() == gu->myPlayerNum);

				if (!(weAreAIHost || weAreLeader || singlePlayer || (weAreAllied && cheating))) {
					LOG_L(L_WARNING, "Team to %s: player %s is not allowed to %s Skirmish AI controlling team %i (try with /cheat)",
							actionName.c_str(), fromPlayer->name.c_str(), actionName.c_str(), teamToKillId);
					badArgs = true;
				}
			}
			if (!badArgs && teamToKill->isDead) {
				LOG_L(L_WARNING, "Team to %s: is a dead team already: %i", actionName.c_str(), teamToKillId);
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

				LOG("Skirmish AI controlling team %i is beeing %sed ...", teamToKillId, actionName.c_str());
			}
		} else {
			LOG_L(L_WARNING, "/%s: missing mandatory argument \"teamTo%s\"", GetCommand().c_str(), actionName.c_str());
			badArgs = true;
		}

		if (badArgs) {
			if (kill) {
				LOG("description: "
				                "Kill a Skirmish AI controlling a team. The team itsself will remain alive, "
				                "unless a second argument is given, which specifies an active team "
				                "that will receive all the units of the AI team.");
				LOG("usage:   /%s teamToKill [teamToReceiveUnits]", GetCommand().c_str());
			} else {
				// reload
				LOG("description: "
				                "Reload a Skirmish AI controlling a team."
				                "The team itsself will remain alive during the process.");
				LOG("usage:   /%s teamToReload", GetCommand().c_str());
			}
		}

		return true;
	}

private:
	bool kill;
};



class AIControlActionExecutor : public IUnsyncedActionExecutor {
public:
	AIControlActionExecutor() : IUnsyncedActionExecutor("AIControl",
			"Creates a new instance of a Skirmish AI, to let it controll"
			" a specific team") {}

	bool Execute(const UnsyncedAction& action) const {
		bool badArgs = false;

		const CPlayer* fromPlayer     = playerHandler->Player(gu->myPlayerNum);
		const int      fromTeamId     = (fromPlayer != NULL) ? fromPlayer->team : -1;

		const bool cheating           = gs->cheatEnabled;
		const bool singlePlayer       = (playerHandler->ActivePlayers() <= 1);

		const std::vector<std::string>& args = _local_strSpaceTokenize(action.GetArgs());

		if (!args.empty()) {
			std::string aiShortName;
			std::string aiVersion;
			std::string aiName;
			std::map<std::string, std::string> aiOptions;

			const int teamToControlId = atoi(args[0].c_str());
			const CTeam* teamToControl = teamHandler->IsActiveTeam(teamToControlId) ?
				teamHandler->Team(teamToControlId) : NULL;

			if (args.size() >= 2) {
				aiShortName = args[1];
			} else {
				LOG_L(L_WARNING, "/%s: missing mandatory argument \"aiShortName\"", GetCommand().c_str());
			}
			if (args.size() >= 3) {
				aiVersion = args[2];
			}
			if (args.size() >= 4) {
				aiName = args[3];
			}

			if (teamToControl == NULL) {
				LOG_L(L_WARNING, "Team to control: not a valid team number: \"%s\"", args[0].c_str());
				badArgs = true;
			}
			if (!badArgs) {
				const bool weAreAllied  = teamHandler->AlliedTeams(fromTeamId, teamToControlId);
				const bool weAreLeader  = (teamToControl->GetLeader() == gu->myPlayerNum);
				const bool noLeader     = (!teamToControl->HasLeader());

				if (!(weAreLeader || singlePlayer || (weAreAllied && (cheating || noLeader)))) {
					LOG_L(L_WARNING, "Team to control: player %s is not allowed to let a Skirmish AI take over control of team %i (try with /cheat)",
							fromPlayer->name.c_str(), teamToControlId);
					badArgs = true;
				}
			}
			if (!badArgs && teamToControl->isDead) {
				LOG_L(L_WARNING, "Team to control: is a dead team: %i", teamToControlId);
				badArgs = true;
			}
			// TODO remove this, if support for multiple Skirmish AIs per team is in place
			if (!badArgs && (!skirmishAIHandler.GetSkirmishAIsInTeam(teamToControlId).empty())) {
				LOG_L(L_WARNING, "Team to control: there is already an AI controlling this team: %i", teamToControlId);
				badArgs = true;
			}
			if (!badArgs && (skirmishAIHandler.GetLocalSkirmishAIInCreation(teamToControlId) != NULL)) {
				LOG_L(L_WARNING, "Team to control: there is already an AI beeing created for team: %i", teamToControlId);
				badArgs = true;
			}
			if (!badArgs) {
				const std::set<std::string>& luaAIImplShortNames = skirmishAIHandler.GetLuaAIImplShortNames();
				if (luaAIImplShortNames.find(aiShortName) != luaAIImplShortNames.end()) {
					LOG_L(L_WARNING, "Team to control: it is currently not supported to initialize Lua AIs mid-game");
					badArgs = true;
				}
			}

			if (!badArgs) {
				SkirmishAIKey aiKey(aiShortName, aiVersion);
				aiKey = aiLibManager->ResolveSkirmishAIKey(aiKey);

				if (aiKey.IsUnspecified()) {
					LOG_L(L_WARNING, "Skirmish AI: not a valid Skirmish AI: %s %s",
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
			LOG_L(L_WARNING, "/%s: missing mandatory arguments \"teamToControl\" and \"aiShortName\"", GetCommand().c_str());
			badArgs = true;
		}

		if (badArgs) {
			LOG("description: Let a Skirmish AI take over control of a team.");
			LOG("usage:   /%s teamToControl aiShortName [aiVersion] [name] [options...]", GetCommand().c_str());
			LOG("example: /%s 1 RAI 0.601 my_RAI_Friend difficulty=2 aggressiveness=3", GetCommand().c_str());
		}

		return true;
	}
};



class AIListActionExecutor : public IUnsyncedActionExecutor {
public:
	AIListActionExecutor() : IUnsyncedActionExecutor("AIList",
			"Prints a list of all currently active Skirmish AIs") {}

	bool Execute(const UnsyncedAction& action) const {
		const CSkirmishAIHandler::id_ai_t& ais = skirmishAIHandler.GetAllSkirmishAIs();
		CSkirmishAIHandler::id_ai_t::const_iterator ai;

		if (!ais.empty()) {
			LOG("%s | %s | %s | %s | %s | %s",
					"ID",
					"Team",
					"Local",
					"Lua",
					"Name",
					"(Hosting player name) or (Short name & Version)");

			for (ai = ais.begin(); ai != ais.end(); ++ai) {
				const bool isLocal = (ai->second.hostPlayer == gu->myPlayerNum);
				std::string lastPart;

				if (isLocal) {
					lastPart = "(Key:)  " + ai->second.shortName + " " + ai->second.version;
				} else {
					lastPart = "(Host:) " + playerHandler->Player(gu->myPlayerNum)->name;
				}

				LOG("%i | %i | %s | %s | %s | %s",
						ai->first,
						ai->second.team,
						(isLocal ? "yes" : "no "),
						(ai->second.isLuaAI ? "yes" : "no "),
						ai->second.name.c_str(),
						lastPart.c_str());
			}
		} else {
			LOG("<There are no active Skirmish AIs in this game>");
		}

		return true;
	}
};



class TeamActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamActionExecutor() : IUnsyncedActionExecutor("Team",
			"Lets the local user change to another team", true) {}

	bool Execute(const UnsyncedAction& action) const {
		const int teamId = atoi(action.GetArgs().c_str());
		if (teamHandler->IsValidTeam(teamId)) {
			net->Send(CBaseNetProtocol::Get().SendJoinTeam(gu->myPlayerNum, teamId));
		} else {
			LOG_L(L_WARNING, "/Team: wrong syntax (which is '/Team %%teamid')");
		}
		return true;
	}
};



class SpectatorActionExecutor : public IUnsyncedActionExecutor {
public:
	SpectatorActionExecutor() : IUnsyncedActionExecutor("Spectator",
			"Lets the local user give up controll over a team, and start spectating") {}

	bool Execute(const UnsyncedAction& action) const {
		if (gu->spectating)
			return false;

		net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
		return true;
	}
};



class SpecTeamActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecTeamActionExecutor() : IUnsyncedActionExecutor("SpecTeam",
			"Lets the local user specify the team to follow, if he is a spectator") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!gu->spectating)
			return false;

		const int teamId = atoi(action.GetArgs().c_str());
		if (!teamHandler->IsValidTeam(teamId))
			return false;

		gu->myTeam = teamId;
		gu->myAllyTeam = teamHandler->AllyTeam(teamId);
		CLuaUI::UpdateTeams();
		return true;
	}
};



class SpecFullViewActionExecutor : public IUnsyncedActionExecutor {
public:
	SpecFullViewActionExecutor() : IUnsyncedActionExecutor("SpecFullView",
			"Sets or toggles between full LOS or ally-team LOS, if the local"
			" user is a spectator") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!gu->spectating)
			return false;

		if (!action.GetArgs().empty()) {
			const int mode = atoi(action.GetArgs().c_str());
			gu->spectatingFullView   = !!(mode & 1);
			gu->spectatingFullSelect = !!(mode & 2);
		} else {
			gu->spectatingFullView = !gu->spectatingFullView;
			gu->spectatingFullSelect = gu->spectatingFullView;
		}
		CLuaUI::UpdateTeams();
		// NOTE: unsynced, so do not inform via eventHandler
		unitDrawer->PlayerChanged(gu->myPlayerNum);
		return true;
	}
};



class AllyActionExecutor : public IUnsyncedActionExecutor {
public:
	AllyActionExecutor() : IUnsyncedActionExecutor("Ally",
			"Starts/Ends alliance of the local players ally-team with another ally-team") {}

	bool Execute(const UnsyncedAction& action) const {
		if (gu->spectating)
			return false;

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
					LOG_L(L_WARNING, "/%s: wrong parameters (usage: /%s <other team> [0|1])", GetCommand().c_str(), GetCommand().c_str());
			}
			else {
				LOG_L(L_WARNING, "In-game alliances are not allowed");
			}
		} else {
			LOG_L(L_WARNING, "/%s: wrong parameters (usage: /%s <other team> [0|1])", GetCommand().c_str(), GetCommand().c_str());
		}
		return true;
	}
};



class GroupActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupActionExecutor() : IUnsyncedActionExecutor("Group",
			"Allows modifying the members of a group") {}

	bool Execute(const UnsyncedAction& action) const {

		const char firstChar = action.GetArgs()[0];
		if ((firstChar >= '0') && (firstChar <= '9')) {
			const int teamId = (int) (firstChar - '0');
			size_t firstCmdChar = action.GetArgs().find_first_not_of(" \t\n\r", 1);
			if (firstCmdChar != std::string::npos) {
				const std::string command = action.GetArgs().substr(firstCmdChar);
				grouphandlers[gu->myTeam]->GroupCommand(teamId, command);
			} else {
				LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
			}
		} else {
			LOG_L(L_WARNING, "/%s: wrong groupid", GetCommand().c_str());
		}
		return true;
	}
};



class GroupIDActionExecutor : public IUnsyncedActionExecutor {
public:
	GroupIDActionExecutor(int groupId)
		: IUnsyncedActionExecutor("Group" + IntToString(groupId),
			"Allows modifying the members of group " + IntToString(groupId))
		, groupId(groupId)
	{}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.IsRepeat()) {
			return grouphandlers[gu->myTeam]->GroupCommand(groupId);
		}
		return false;
	}

private:
	int groupId;
};



class LastMessagePositionActionExecutor : public IUnsyncedActionExecutor {
public:
	LastMessagePositionActionExecutor() : IUnsyncedActionExecutor("LastMsgPos",
			"Moves the camera to show the position of the last message") {}

	bool Execute(const UnsyncedAction& action) const {
		if (game->infoConsole->GetMsgPosCount() == 0)
			return false;

		// cycle through the positions
		camHandler->CameraTransition(0.6f);
		camHandler->GetCurrentController().SetPos(game->infoConsole->GetMsgPos());
		return true;
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
	bool Execute(const UnsyncedAction& action) const {
		if (setUserInputPrefix) {
			game->userInputPrefix = userInputPrefix;
		}
		game->userWriting = true;
		game->userPrompt = "Say: ";
		game->userInput = game->userInputPrefix;
		game->writingPos = (int)game->userInput.length();
		game->chatting = true;
		game->ignoreNextChar = true;
		game->consoleHistory->ResetPosition();
		return true;
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

	bool Execute(const UnsyncedAction& action) const {
		unitTracker.Track();
		return true;
	}
};



class TrackOffActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackOffActionExecutor() : IUnsyncedActionExecutor("TrackOff",
			"Stop following the selected unit(s) with the camera") {}

	bool Execute(const UnsyncedAction& action) const {
		unitTracker.Disable();
		return true;
	}
};



class TrackModeActionExecutor : public IUnsyncedActionExecutor {
public:
	TrackModeActionExecutor() : IUnsyncedActionExecutor("TrackMode",
			"Shift through different ways of following selected unit(s)") {}

	bool Execute(const UnsyncedAction& action) const {
		unitTracker.IncMode();
		return true;
	}
};



class PauseActionExecutor : public IUnsyncedActionExecutor {
public:
	PauseActionExecutor() : IUnsyncedActionExecutor("Pause",
			"Pause/Unpause the game") {}

	bool Execute(const UnsyncedAction& action) const {
		// disallow pausing prior to start of game proper
		if (!game->playing)
			return false;

		// do not need to update lastReadNetTime, gets
		// done when NETMSG_PAUSE makes the round-trip
		bool newPause = gs->paused;
		SetBoolArg(newPause, action.GetArgs());
		net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, newPause));
		return true;
	}

};



class DebugActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugActionExecutor() : IUnsyncedActionExecutor("Debug",
			"Enable/Disable debug info rendering mode") {}

	bool Execute(const UnsyncedAction& action) const {
		// toggle
		globalRendering->drawdebug = !globalRendering->drawdebug;

		ProfileDrawer::SetEnabled(globalRendering->drawdebug);
		LogSystemStatus("debug-info rendering mode", globalRendering->drawdebug);
		return true;
	}
};



// XXX unlucky name; maybe make this "Sound {0|1}" instead (bool arg or toggle)
class NoSoundActionExecutor : public IUnsyncedActionExecutor {
public:
	NoSoundActionExecutor() : IUnsyncedActionExecutor("NoSound",
			"Enable/Disable the sound system") {}

	bool Execute(const UnsyncedAction& action) const {

		// toggle
		sound->Mute();
		LogSystemStatus("Sound", !sound->IsMuted());
		return true;
	}
};



class SoundChannelEnableActionExecutor : public IUnsyncedActionExecutor {
public:
	SoundChannelEnableActionExecutor() : IUnsyncedActionExecutor("SoundChannelEnable",
			"Enable/Disable specific sound channels:"
			" UnitReply, General, Battle, UserInterface, Music") {}

	bool Execute(const UnsyncedAction& action) const {
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
		else
			LOG_L(L_WARNING, "/%s: wrong channel name \"%s\"", GetCommand().c_str(), channel.c_str());

		return true;
	}
};



class CreateVideoActionExecutor : public IUnsyncedActionExecutor {
public:
	CreateVideoActionExecutor() : IUnsyncedActionExecutor("CreateVideo",
			"Start/Stop capturing a video of the game in progress") {}

	bool Execute(const UnsyncedAction& action) const {

		// toggle
		videoCapturing->SetCapturing(!videoCapturing->IsCapturing());
		LogSystemStatus("Video capturing", videoCapturing->IsCapturing());
		return true;
	}
};



class DrawTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	DrawTreesActionExecutor() : IUnsyncedActionExecutor("DrawTrees",
			"Enable/Disable rendering of engine trees") {}

	bool Execute(const UnsyncedAction& action) const {

		SetBoolArg(treeDrawer->drawTrees, action.GetArgs());
		LogSystemStatus("rendering of engine trees", treeDrawer->drawTrees);
		return true;
	}
};



class DynamicSkyActionExecutor : public IUnsyncedActionExecutor {
public:
	DynamicSkyActionExecutor() : IUnsyncedActionExecutor("DynamicSky",
			"Enable/Disable dynamic-sky rendering") {}

	bool Execute(const UnsyncedAction& action) const {

		SetBoolArg(sky->dynamicSky, action.GetArgs());
		LogSystemStatus("dynamic-sky rendering", sky->dynamicSky);
		return true;
	}
};



class DynamicSunActionExecutor : public IUnsyncedActionExecutor {
public:
	DynamicSunActionExecutor() : IUnsyncedActionExecutor("DynamicSun",
			"Enable/Disable dynamic-sun rendering") {}

	bool Execute(const UnsyncedAction& action) const {

		bool dynamicSun = sky->GetLight()->IsDynamic();
		SetBoolArg(dynamicSun, action.GetArgs());
		sky->SetLight(dynamicSun);
		LogSystemStatus("dynamic-sun rendering", sky->GetLight()->IsDynamic());
		return true;
	}
};



class SpeedControlActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedControlActionExecutor() : IUnsyncedActionExecutor("SpeedControl",
			"Sets how server adjusts speed according to player's load (CPU), 1: use average, 2: use highest,") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!gameServer) {
			return false;
		}
		if (action.GetArgs().empty()) {
			// switch to next value
			++game->speedControl;
			if (game->speedControl > 2) {
				game->speedControl = 1;
			}
		} else {
			// set value
			game->speedControl = atoi(action.GetArgs().c_str());
		}
		// constrain to bounds
		game->speedControl = std::max(1, std::min(game->speedControl, 2));
		gameServer->UpdateSpeedControl(game->speedControl);
		return true;
	}
};



class GameInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	GameInfoActionExecutor() : IUnsyncedActionExecutor("GameInfo",
			"Enables/Disables game-info panel rendering") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.IsRepeat()) {
			if (!CGameInfo::IsActive()) {
				CGameInfo::Enable();
			} else {
				CGameInfo::Disable();
			}
		}
		return true;
	}
};



class HideInterfaceActionExecutor : public IUnsyncedActionExecutor {
public:
	HideInterfaceActionExecutor() : IUnsyncedActionExecutor("HideInterface",
			"Hide/Show the GUI controlls") {}

	bool Execute(const UnsyncedAction& action) const {
		SetBoolArg(game->hideInterface, action.GetArgs());
		return true;
	}
};



class HardwareCursorActionExecutor : public IUnsyncedActionExecutor {
public:
	HardwareCursorActionExecutor() : IUnsyncedActionExecutor("HardwareCursor",
			"Enables/Disables hardware mouse-cursor support") {}

	bool Execute(const UnsyncedAction& action) const {
		const bool enable = (atoi(action.GetArgs().c_str()) != 0);
		mouse->ToggleHwCursor(enable);
		configHandler->Set("HardwareCursor", enable);
		LogSystemStatus("Hardware mouse-cursor", enable);
		return true;
	}
};



class FullscreenActionExecutor : public IUnsyncedActionExecutor {
public:
	FullscreenActionExecutor() : IUnsyncedActionExecutor("Fullscreen",
			"Switches fullscreen mode") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			globalRendering->fullScreen = (atoi(action.GetArgs().c_str()) != 0);
		} else {
			globalRendering->fullScreen = !globalRendering->fullScreen;
		}

		const bool borderless = configHandler->GetBool("WindowBorderless");
		if (globalRendering->fullScreen) {
			if (borderless) {
				SDL_SetWindowFullscreen(globalRendering->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			} else {
				SDL_SetWindowFullscreen(globalRendering->window, SDL_WINDOW_FULLSCREEN);
			}
		} else {
			SDL_SetWindowFullscreen(globalRendering->window, 0);
			SDL_SetWindowBordered(globalRendering->window, borderless ? SDL_FALSE : SDL_TRUE);
		}
		return true;
	}
};



class IncreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("IncreaseViewRadius",
			"Increase the view radius (lower performance, nicer view)") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->IncreaseDetail();
		return true;
	}
};



class DecreaseViewRadiusActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseViewRadiusActionExecutor() : IUnsyncedActionExecutor("DecreaseViewRadius",
			"Decrease the view radius (higher performance, uglier view)") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->DecreaseDetail();
		return true;
	}
};



class MoreTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreTreesActionExecutor() : IUnsyncedActionExecutor("MoreTrees",
			"Increases the distance to the camera, in which trees are still"
			" shown (lower performance)") {}

	bool Execute(const UnsyncedAction& action) const {

		treeDrawer->baseTreeDistance += 0.2f;
		ReportTreeDistance();
		return true;
	}
	static void ReportTreeDistance() {
		LOG("Base tree distance %f",
				treeDrawer->baseTreeDistance * 2 * SQUARE_SIZE * TREE_SQUARE_SIZE);
	}
};



class LessTreesActionExecutor : public IUnsyncedActionExecutor {
public:
	LessTreesActionExecutor() : IUnsyncedActionExecutor("LessTrees",
			"Decreases the distance to the camera, in which trees are still"
			" shown (higher performance)") {}

	bool Execute(const UnsyncedAction& action) const {

		treeDrawer->baseTreeDistance -= 0.2f;
		MoreTreesActionExecutor::ReportTreeDistance();
		return true;
	}
};



class MoreCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	MoreCloudsActionExecutor() : IUnsyncedActionExecutor("MoreClouds",
			"Increases the density of clouds (lower performance)") {}

	bool Execute(const UnsyncedAction& action) const {

		sky->IncreaseCloudDensity();
		ReportCloudDensity();
		return true;
	}

	static void ReportCloudDensity() {
		LOG("Cloud density %f", 1.0f / sky->GetCloudDensity());
	}
};



class LessCloudsActionExecutor : public IUnsyncedActionExecutor {
public:
	LessCloudsActionExecutor() : IUnsyncedActionExecutor("LessClouds",
			"Decreases the density of clouds (higher performance)") {}

	bool Execute(const UnsyncedAction& action) const {

		sky->DecreaseCloudDensity();
		MoreCloudsActionExecutor::ReportCloudDensity();
		return true;
	}
};



class SpeedUpActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedUpActionExecutor() : IUnsyncedActionExecutor("SpeedUp",
			"Increases the simulation speed."
			" The engine will try to simulate more frames per second") {}

	bool Execute(const UnsyncedAction& action) const {
		float speed = gs->wantedSpeedFactor;
		if (speed < 5) {
			speed += (speed < 2) ? 0.1f : 0.2f;
			float fPart = speed - (int)speed;
			if (fPart < 0.01f || fPart > 0.99f)
				speed = math::round(speed);
		} else if (speed < 10) {
			speed += 0.5f;
		} else {
			speed += 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
		return true;
	}
};



class SlowDownActionExecutor : public IUnsyncedActionExecutor {
public:
	SlowDownActionExecutor() : IUnsyncedActionExecutor("SlowDown",
			"Decreases the simulation speed."
			" The engine will try to simulate less frames per second") {}

	bool Execute(const UnsyncedAction& action) const {
		float speed = gs->wantedSpeedFactor;
		if (speed <= 5) {
			speed -= (speed <= 2) ? 0.1f : 0.2f;
			float fPart = speed - (int)speed;
			if (fPart < 0.01f || fPart > 0.99f)
				speed = math::round(speed);
			if (speed < 0.1f)
				speed = 0.1f;
		} else if (speed <= 10) {
			speed -= 0.5f;
		} else {
			speed -= 1.0f;
		}
		net->Send(CBaseNetProtocol::Get().SendUserSpeed(gu->myPlayerNum, speed));
		return true;
	}
};



class ControlUnitActionExecutor : public IUnsyncedActionExecutor {
public:
	ControlUnitActionExecutor() : IUnsyncedActionExecutor("ControlUnit",
			"Start to first-person-control a unit") {}

	bool Execute(const UnsyncedAction& action) const {
		if (gu->spectating)
			return false;

		// we must cause the to-be-controllee to be put in
		// netSelected[myPlayerNum] by giving it an order
		selectedUnitsHandler.SendCommand(Command(CMD_STOP));
		net->Send(CBaseNetProtocol::Get().SendDirectControl(gu->myPlayerNum));
		return true;
	}
};



class ShowStandardActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowStandardActionExecutor() : IUnsyncedActionExecutor("ShowStandard",
			"Disable rendering of all auxiliary map overlays") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->DisableExtraTexture();
		return true;
	}
};



class ShowElevationActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowElevationActionExecutor() : IUnsyncedActionExecutor("ShowElevation",
			"Enable rendering of the auxiliary height-map overlay") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->SetHeightTexture();
		return true;
	}
};



class ToggleRadarAndJammerActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleRadarAndJammerActionExecutor() : IUnsyncedActionExecutor("ToggleRadarAndJammer",
			"Enable/Disable rendering of the auxiliary radar- & jammer-map overlay") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->ToggleRadarAndJammer();
		return true;
	}
};



class ShowMetalMapActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowMetalMapActionExecutor() : IUnsyncedActionExecutor("ShowMetalMap",
			"Enable rendering of the auxiliary metal-map overlay") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->SetMetalTexture();
		return true;
	}
};



class ShowPathTravActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathTravActionExecutor() : IUnsyncedActionExecutor("ShowPathTraversability",
			"Enable rendering of the path traversability-map overlay") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->TogglePathTexture(CBaseGroundDrawer::drawPathTrav);
		return true;
	}
};

class ShowPathHeatActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathHeatActionExecutor() : IUnsyncedActionExecutor("ShowPathHeat",
			"Enable/Disable rendering of the path heat-map overlay", true) {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->TogglePathTexture(CBaseGroundDrawer::drawPathHeat);
		return true;
	}
};

class ShowPathFlowActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathFlowActionExecutor() : IUnsyncedActionExecutor("ShowPathFlow",
			"Enable/Disable rendering of the path flow-map overlay", true) {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->TogglePathTexture(CBaseGroundDrawer::drawPathFlow);
		return true;
	}
};

class ShowPathCostActionExecutor : public IUnsyncedActionExecutor {
public:
	ShowPathCostActionExecutor() : IUnsyncedActionExecutor("ShowPathCost",
			"Enable rendering of the path cost-map overlay", true) {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->TogglePathTexture(CBaseGroundDrawer::drawPathCost);
		return true;
	}
};



class ToggleLOSActionExecutor : public IUnsyncedActionExecutor {
public:
	ToggleLOSActionExecutor() : IUnsyncedActionExecutor("ToggleLOS",
			"Enable rendering of the auxiliary LOS-map overlay") {}

	bool Execute(const UnsyncedAction& action) const {
		readMap->GetGroundDrawer()->ToggleLosTexture();
		return true;
	}
};



class ShareDialogActionExecutor : public IUnsyncedActionExecutor {
public:
	ShareDialogActionExecutor() : IUnsyncedActionExecutor("ShareDialog",
			"Opens the share dialog, which allows you to send units and"
			" resources to other players") {}

	bool Execute(const UnsyncedAction& action) const {
		if (gu->spectating)
			return false;

		// already shown?
		const std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (inputReceivers.empty() || (dynamic_cast<CShareBox*>(inputReceivers.front()) != NULL))
			return false;

		new CShareBox();
		return true;
	}
};



class QuitMessageActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMessageActionExecutor() : IUnsyncedActionExecutor("QuitMessage",
			"Deprecated, see /Quit instead (was used to quite the game immediately)") {}

	bool Execute(const UnsyncedAction& action) const {
		// already shown?
		std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if  (inputReceivers.empty() || dynamic_cast<CQuitBox*>(inputReceivers.front()))
			return false;

		const CKeyBindings::HotkeyList& quitList = keyBindings->GetHotkeys("quitmenu");
		const std::string quitKey = quitList.empty() ? "<none>" : *quitList.begin();
		LOG("Press %s to access the quit menu", quitKey.c_str());
		return true;
	}
};



class QuitMenuActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitMenuActionExecutor() : IUnsyncedActionExecutor("QuitMenu",
			"Opens the quit-menu, if it is not already open") {}

	bool Execute(const UnsyncedAction& action) const {
		// already shown?
		std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
		if (inputReceivers.empty() || dynamic_cast<CQuitBox*>(inputReceivers.front()))
			return false;

		new CQuitBox();
		return true;
	}
};



class QuitActionExecutor : public IUnsyncedActionExecutor {
public:
	QuitActionExecutor() : IUnsyncedActionExecutor("Quit",
			"Exits the game immediately") {}

	bool Execute(const UnsyncedAction& action) const {
		Exit();
		return true;
	}
	static void Exit() {
		LOG("User exited");
		gu->globalQuit = true;
	}
};



class IncreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	IncreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("IncGUIOpacity",
			"Increases the the opacity(see-through-ness) of GUI elements") {}

	bool Execute(const UnsyncedAction& action) const {

		CInputReceiver::guiAlpha = std::min(CInputReceiver::guiAlpha + 0.1f, 1.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
		return true;
	}
};



class DecreaseGUIOpacityActionExecutor : public IUnsyncedActionExecutor {
public:
	DecreaseGUIOpacityActionExecutor() : IUnsyncedActionExecutor("DecGUIOpacity",
			"Decreases the the opacity(see-through-ness) of GUI elements") {}

	bool Execute(const UnsyncedAction& action) const {

		CInputReceiver::guiAlpha = std::max(CInputReceiver::guiAlpha - 0.1f, 0.0f);
		configHandler->Set("GuiOpacity", CInputReceiver::guiAlpha);
		return true;
	}
};



class ScreenShotActionExecutor : public IUnsyncedActionExecutor {
public:
	ScreenShotActionExecutor() : IUnsyncedActionExecutor("ScreenShot",
			"Take a screen-shot of the current view") {}

	bool Execute(const UnsyncedAction& action) const {
		TakeScreenshot(action.GetArgs());
		return true;
	}
};



class GrabInputActionExecutor : public IUnsyncedActionExecutor {
public:
	GrabInputActionExecutor() : IUnsyncedActionExecutor("GrabInput",
			"Prevents/Enables the mouse from leaving the game window (windowed mode only)") {}

	bool Execute(const UnsyncedAction& action) const {
		SDL_bool newMode;
		if (action.GetArgs().empty()) {
			newMode = (SDL_GetWindowGrab(globalRendering->window)) ? SDL_FALSE : SDL_TRUE;
		} else {
			if (atoi(action.GetArgs().c_str())) {
				newMode = SDL_TRUE;
			} else {
				newMode = SDL_FALSE;
			}
		}
		SDL_SetWindowGrab(globalRendering->window, newMode);
		LogSystemStatus("Input grabbing", (newMode == SDL_TRUE));
		return true;
	}
};



class ClockActionExecutor : public IUnsyncedActionExecutor {
public:
	ClockActionExecutor() : IUnsyncedActionExecutor("Clock",
			"Shows a small digital clock indicating the local time") {}

	bool Execute(const UnsyncedAction& action) const {

		SetBoolArg(game->showClock, action.GetArgs());
		configHandler->Set("ShowClock", game->showClock ? 1 : 0);
		LogSystemStatus("small digital clock", game->showClock);
		return true;
	}
};



class CrossActionExecutor : public IUnsyncedActionExecutor {
public:
	CrossActionExecutor() : IUnsyncedActionExecutor("Cross",
			"Allows one to exchange and modify the appearance of the"
			" cross/mouse-pointer in first-person-control view") {}

	bool Execute(const UnsyncedAction& action) const {
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
		return true;
	}
};



class FPSActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSActionExecutor() : IUnsyncedActionExecutor("FPS",
			"Shows/Hides the frames-per-second indicator") {}

	bool Execute(const UnsyncedAction& action) const {

		SetBoolArg(game->showFPS, action.GetArgs());
		configHandler->Set("ShowFPS", game->showFPS ? 1 : 0);
		LogSystemStatus("frames-per-second indicator", game->showFPS);
		return true;
	}
};



class SpeedActionExecutor : public IUnsyncedActionExecutor {
public:
	SpeedActionExecutor() : IUnsyncedActionExecutor("Speed",
			"Shows/Hides the simulation speed indicator") {}

	bool Execute(const UnsyncedAction& action) const {

		SetBoolArg(game->showSpeed, action.GetArgs());
		configHandler->Set("ShowSpeed", game->showSpeed ? 1 : 0);
		LogSystemStatus("simulation speed indicator", game->showSpeed);
		return true;
	}
};


class TeamHighlightActionExecutor : public IUnsyncedActionExecutor {
public:
	TeamHighlightActionExecutor() : IUnsyncedActionExecutor("TeamHighlight",
			"Enables/Disables uncontrolled team blinking") {}

	bool Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			globalConfig->teamHighlight = abs(globalConfig->teamHighlight + 1) % CTeamHighlight::HIGHLIGHT_SIZE;
		} else {
			globalConfig->teamHighlight = abs(atoi(action.GetArgs().c_str())) % CTeamHighlight::HIGHLIGHT_SIZE;
		}
		LOG("Team highlighting: %s",
				((globalConfig->teamHighlight == CTeamHighlight::HIGHLIGHT_PLAYERS) ? "Players only"
				: ((globalConfig->teamHighlight == CTeamHighlight::HIGHLIGHT_ALL) ? "Players and spectators"
				: "Disabled")));
		configHandler->Set("TeamHighlight", globalConfig->teamHighlight);
		return true;
	}
};



class InfoActionExecutor : public IUnsyncedActionExecutor {
public:
	InfoActionExecutor() : IUnsyncedActionExecutor("Info",
			"Shows/Hides the player roster") {}

	bool Execute(const UnsyncedAction& action) const {
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
			LOG("Sorting roster by %s", playerRoster.GetSortName());
		}
		configHandler->Set("ShowPlayerInfo", (int)playerRoster.GetSortType());
		return true;
	}
};



class CmdColorsActionExecutor : public IUnsyncedActionExecutor {
public:
	CmdColorsActionExecutor() : IUnsyncedActionExecutor("CmdColors",
			"Reloads cmdcolors.txt") {}

	bool Execute(const UnsyncedAction& action) const {

		const std::string fileName = action.GetArgs().empty() ? "cmdcolors.txt" : action.GetArgs();
		cmdColors.LoadConfigFromFile(fileName);
		LOG("Reloaded cmdcolors from file: %s", fileName.c_str());
		return true;
	}
};



class CtrlPanelActionExecutor : public IUnsyncedActionExecutor {
public:
	CtrlPanelActionExecutor() : IUnsyncedActionExecutor("CtrlPanel",
			"Reloads ctrlpanel.txt") {}

	bool Execute(const UnsyncedAction& action) const {

		const std::string fileName = action.GetArgs().empty() ? "ctrlpanel.txt" : action.GetArgs();
		guihandler->ReloadConfigFromFile(fileName);
		LOG("Reloaded ctrlpanel from file: %s", fileName.c_str());
		return true;
	}
};



class FontActionExecutor : public IUnsyncedActionExecutor {
public:
	FontActionExecutor() : IUnsyncedActionExecutor("Font",
			"Reloads the fonts") {}

	bool Execute(const UnsyncedAction& action) const {
		CglFont *newFont = NULL, *newSmallFont = NULL;
		try {
			const int fontSize = configHandler->GetInt("FontSize");
			const int smallFontSize = configHandler->GetInt("SmallFontSize");
			const int outlineWidth = configHandler->GetInt("FontOutlineWidth");
			const float outlineWeight = configHandler->GetFloat("FontOutlineWeight");
			const int smallOutlineWidth = configHandler->GetInt("SmallFontOutlineWidth");
			const float smallOutlineWeight = configHandler->GetFloat("SmallFontOutlineWeight");

			newFont = CglFont::LoadFont(action.GetArgs(), fontSize, outlineWidth, outlineWeight);
			newSmallFont = CglFont::LoadFont(action.GetArgs(), smallFontSize, smallOutlineWidth, smallOutlineWeight);
		} catch (const std::exception& ex) {
			delete newFont;
			delete newSmallFont;
			newFont = newSmallFont = NULL;
			LOG_L(L_ERROR, "Font: %s", ex.what());
		}
		if (newFont != NULL && newSmallFont != NULL) {
			delete font;
			delete smallFont;
			font = newFont;
			smallFont = newSmallFont;
			LOG("Loaded font: %s", action.GetArgs().c_str());
			configHandler->SetString("FontFile", action.GetArgs());
			configHandler->SetString("SmallFontFile", action.GetArgs());
		}
		return true;
	}
};



class VSyncActionExecutor : public IUnsyncedActionExecutor {
public:
	VSyncActionExecutor() : IUnsyncedActionExecutor("VSync",
			"Enables/Disables vertical-sync (Graphics setting)") {}

	bool Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().empty()) {
			VSync.SetInterval((VSync.GetInterval() <= 0) ? 1 : 0);
		} else {
			VSync.SetInterval(atoi(action.GetArgs().c_str()));
		}
		return true;
	}
};



class SafeGLActionExecutor : public IUnsyncedActionExecutor {
public:
	SafeGLActionExecutor() : IUnsyncedActionExecutor("SafeGL",
			"Enables/Disables OpenGL safe-mode") {}

	bool Execute(const UnsyncedAction& action) const {

		bool safeMode = LuaOpenGL::GetSafeMode();
		SetBoolArg(safeMode, action.GetArgs());
		LuaOpenGL::SetSafeMode(safeMode);
		LogSystemStatus("OpenGL safe-mode", LuaOpenGL::GetSafeMode());
		return true;
	}
};



class ResBarActionExecutor : public IUnsyncedActionExecutor {
public:
	ResBarActionExecutor() : IUnsyncedActionExecutor("ResBar",
			"Shows/Hides team resource storage indicator bar") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!resourceBar)
			return false;

		SetBoolArg(resourceBar->enabled, action.GetArgs());
		return true;
	}
};



class ToolTipActionExecutor : public IUnsyncedActionExecutor {
public:
	ToolTipActionExecutor() : IUnsyncedActionExecutor("ToolTip",
			"Enables/Disables the general tool-tips, displayed when hovering"
			" over units. features or the map") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!tooltip)
			return false;

		SetBoolArg(tooltip->enabled, action.GetArgs());
		return true;
	}
};



class ConsoleActionExecutor : public IUnsyncedActionExecutor {
public:
	ConsoleActionExecutor() : IUnsyncedActionExecutor("Console",
			"Enables/Disables the in-game console") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!game->infoConsole)
			return false;

		SetBoolArg(game->infoConsole->enabled, action.GetArgs());
		return true;
	}
};



class EndGraphActionExecutor : public IUnsyncedActionExecutor {
public:
	EndGraphActionExecutor() : IUnsyncedActionExecutor("EndGraph",
			"Enables/Disables the statistics graphs shown at the end of the game") {}

	bool Execute(const UnsyncedAction& action) const {
		SetBoolArg(CEndGameBox::enabled, action.GetArgs());
		return true;
	}
};



class FPSHudActionExecutor : public IUnsyncedActionExecutor {
public:
	FPSHudActionExecutor() : IUnsyncedActionExecutor("FPSHud",
			"Enables/Disables HUD (GUI interface) shown in first-person-control mode") {}

	bool Execute(const UnsyncedAction& action) const {

		bool drawHUD = hudDrawer->GetDraw();
		SetBoolArg(drawHUD, action.GetArgs());
		hudDrawer->SetDraw(drawHUD);
		return true;
	}
};



class DebugDrawAIActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugDrawAIActionExecutor() : IUnsyncedActionExecutor("DebugDrawAI",
			"Enables/Disables debug drawing for AIs") {}

	bool Execute(const UnsyncedAction& action) const {

		bool aiDebugDraw = debugDrawerAI->GetDraw();
		SetBoolArg(aiDebugDraw, action.GetArgs());
		debugDrawerAI->SetDraw(aiDebugDraw);
		LogSystemStatus("SkirmishAI debug drawing", debugDrawerAI->GetDraw());
		return true;
	}
};



class MapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	MapMarksActionExecutor() : IUnsyncedActionExecutor("MapMarks",
			"Enables/Disables map marks rendering") {}

	bool Execute(const UnsyncedAction& action) const {

		SetBoolArg(globalRendering->drawMapMarks, action.GetArgs());
		LogSystemStatus("map marks rendering", globalRendering->drawMapMarks);
		return true;
	}
};



class AllMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	AllMapMarksActionExecutor() : IUnsyncedActionExecutor("AllMapMarks",
			"Show/Hide all map marks drawn so far", true) {}

	bool Execute(const UnsyncedAction& action) const {

		bool allMarksVisible = inMapDrawerModel->GetAllMarksVisible();
		SetBoolArg(allMarksVisible, action.GetArgs());
		inMapDrawerModel->SetAllMarksVisible(allMarksVisible);
		return true;
	}
};



class ClearMapMarksActionExecutor : public IUnsyncedActionExecutor {
public:
	ClearMapMarksActionExecutor() : IUnsyncedActionExecutor("ClearMapMarks",
			"Remove all map marks drawn so far") {}

	bool Execute(const UnsyncedAction& action) const {
		inMapDrawerModel->EraseAll();
		return true;
	}
};



// XXX unlucky command-name, remove the "No"
class NoLuaDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	NoLuaDrawActionExecutor() : IUnsyncedActionExecutor("NoLuaDraw",
			"Allow/Disallow Lua to draw on the map") {}

	bool Execute(const UnsyncedAction& action) const {

		bool luaMapDrawingAllowed = inMapDrawer->GetLuaMapDrawingAllowed();
		SetBoolArg(luaMapDrawingAllowed, action.GetArgs());
		inMapDrawer->SetLuaMapDrawingAllowed(luaMapDrawingAllowed);
		return true;
	}
};



class LuaUIActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaUIActionExecutor() : IUnsyncedActionExecutor("LuaUI",
			"Allows one to reload or disable LuaUI, or alternatively to send"
			" a chat message to LuaUI") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!guihandler)
			return false;

		const std::string& command = action.GetArgs();

		if (command == "reload" || command == "enable") {
			if (luaUI && luaUI->IsRunning()) {
				// NOTE: causes a SEGV through RunCallIn()
				LOG_L(L_WARNING, "Can not reload from within LuaUI, yet");
				return true;
			}
			if (luaUI == NULL) {
				LOG("Loading: \"%s\"", "luaui.lua"); // FIXME duplicate of below
				CLuaUI::LoadHandler();
				if (luaUI == NULL) {
					guihandler->LoadConfig("ctrlpanel.txt");
					LOG_L(L_WARNING, "Loading failed");
				}
			} else {
				if (command == "enable") {
					LOG_L(L_WARNING, "LuaUI is already enabled");
				} else {
					LOG("Reloading: \"%s\"", "luaui.lua"); // FIXME
					CLuaUI::FreeHandler();
					CLuaUI::LoadHandler();
					if (luaUI == NULL) {
						guihandler->LoadConfig("ctrlpanel.txt");
						LOG_L(L_WARNING, "Reloading failed");
					}
				}
			}
			guihandler->LayoutIcons(false);
		}
		else if (command == "disable") {
			if (luaUI && luaUI->IsRunning()) {
				// NOTE: might cause a SEGV through RunCallIn()
				LOG_L(L_WARNING, "Can not disable from within LuaUI, yet");
				return true;
			}
			if (luaUI != NULL) {
				CLuaUI::FreeHandler();
				LOG("Disabled LuaUI");
			}
			guihandler->LayoutIcons(false);
		}
		else if (luaUI) {
			luaUI->GotChatMsg(command, 0);
		} else {
			LOG_L(L_DEBUG, "LuaUI is not loaded");
		}

		return true;
	}
};



class LuaModUICtrlActionExecutor : public IUnsyncedActionExecutor {
public:
	LuaModUICtrlActionExecutor() : IUnsyncedActionExecutor("LuaModUICtrl",
			"Allow/Disallow Lua to receive UI control events, like mouse-,"
			" keyboard- and joystick-events") {}

	bool Execute(const UnsyncedAction& action) const {

		bool modUICtrl = CLuaHandle::GetModUICtrl();
		SetBoolArg(modUICtrl, action.GetArgs());
		CLuaHandle::SetModUICtrl(modUICtrl);
		configHandler->Set("LuaModUICtrl", modUICtrl ? 1 : 0);
		return true;
	}
};



class MiniMapActionExecutor : public IUnsyncedActionExecutor {
public:
	MiniMapActionExecutor() : IUnsyncedActionExecutor("MiniMap",
			"Show/Hide the mini-map provided by the engine") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!minimap)
			return false;

		minimap->ConfigCommand(action.GetArgs());
		return true;
	}
};



class GroundDecalsActionExecutor : public IUnsyncedActionExecutor {
public:
	GroundDecalsActionExecutor() : IUnsyncedActionExecutor("GroundDecals",
			"Disable/Enable ground-decals rendering."
			" Ground-decals are things like scars appearing on the map after an"
			" explosion.") {}

	bool Execute(const UnsyncedAction& action) const {
		bool drawDecals = IGroundDecalDrawer::GetDrawDecals();
		SetBoolArg(drawDecals, action.GetArgs());
		IGroundDecalDrawer::SetDrawDecals(drawDecals);

		LogSystemStatus("Ground-decals rendering", IGroundDecalDrawer::GetDrawDecals());
		return true;
	}
};



class MaxParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxParticlesActionExecutor() : IUnsyncedActionExecutor("MaxParticles",
			"Set the maximum number of particles (Graphics setting)") {}

	bool Execute(const UnsyncedAction& action) const {
		if (projectileHandler == NULL)
			return false;

		if (!action.GetArgs().empty()) {
			const int value = std::max(1, atoi(action.GetArgs().c_str()));
			projectileHandler->SetMaxParticles(value);
			LOG("Set maximum particles to: %i", value);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class MaxNanoParticlesActionExecutor : public IUnsyncedActionExecutor {
public:
	MaxNanoParticlesActionExecutor() : IUnsyncedActionExecutor("MaxNanoParticles",
			"Set the maximum number of nano-particles (Graphic setting)") {}

	bool Execute(const UnsyncedAction& action) const {
		if (projectileHandler == NULL)
			return false;

		if (!action.GetArgs().empty()) {
			const int value = std::max(1, atoi(action.GetArgs().c_str()));
			projectileHandler->SetMaxNanoParticles(value);
			LOG("Set maximum nano-particles to: %i", value);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class GatherModeActionExecutor : public IUnsyncedActionExecutor {
public:
	GatherModeActionExecutor() : IUnsyncedActionExecutor("GatherMode",
			"Enter/Leave gather-wait command mode") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!guihandler)
			return false;

		bool gatherMode = guihandler->GetGatherMode();
		SetBoolArg(gatherMode, action.GetArgs());
		guihandler->SetGatherMode(gatherMode);
		LogSystemStatus("Gather-Mode", guihandler->GetGatherMode());
		return true;
	}
};



class PasteTextActionExecutor : public IUnsyncedActionExecutor {
public:
	PasteTextActionExecutor() : IUnsyncedActionExecutor("PasteText",
			"Paste either the argument string(s) or if none given, the content of the clip-board to chat input") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!game->userWriting)
			return false;
		//we cannot use extra commands because tokenization strips multiple spaces
		//or even trailing spaces, the text should be copied verbatim
		const std::string pastecommand = "pastetext ";
		if (action.GetInnerAction().rawline.length() > pastecommand.length() ) {
			game->userInput.insert(game->writingPos, action.GetInnerAction().rawline.substr(pastecommand.length(), action.GetInnerAction().rawline.length()-pastecommand.length()));
			game->writingPos += action.GetInnerAction().rawline.length()-pastecommand.length();
		} else {
			game->PasteClipboard();
		}
		return true;
	}
};



class BufferTextActionExecutor : public IUnsyncedActionExecutor {
public:
	BufferTextActionExecutor() : IUnsyncedActionExecutor("BufferText",
			"Write the argument string(s) directly to the console history,"
			" but not anywhere else. This is useful for fast manual issuing"
			" of the command, later on") {}

	bool Execute(const UnsyncedAction& action) const {
		//we cannot use extra commands because tokenization strips multiple spaces
		//or even trailing spaces, the text should be copied verbatim
		const std::string buffercommand = "buffertext ";
		if (action.GetInnerAction().rawline.length() > buffercommand.length() ) {
			game->consoleHistory->AddLine(action.GetInnerAction().rawline.substr(buffercommand.length(), action.GetInnerAction().rawline.length()-buffercommand.length()));
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class InputTextGeoActionExecutor : public IUnsyncedActionExecutor {
public:
	InputTextGeoActionExecutor() : IUnsyncedActionExecutor("InputTextGeo",
			"Move and/or resize the input-text field (the \"Say: \" thing)") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			game->ParseInputTextGeometry(action.GetArgs());
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class DistIconActionExecutor : public IUnsyncedActionExecutor {
public:
	DistIconActionExecutor() : IUnsyncedActionExecutor("DistIcon",
			"Set the distance between units and camera, at which they turn"
			" into icons (Graphic setting)") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			const int iconDist = atoi(action.GetArgs().c_str());
			unitDrawer->SetUnitIconDist((float)iconDist);
			configHandler->Set("UnitIconDist", iconDist);
			LOG("Set UnitIconDist to %i", iconDist);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class DistDrawActionExecutor : public IUnsyncedActionExecutor {
public:
	DistDrawActionExecutor() : IUnsyncedActionExecutor("DistDraw",
			"Set the distance between units and camera, at which they turn"
			" into far-textures (flat/texture-only representation) (Graphic setting)") {}

	bool Execute(const UnsyncedAction& action) const {
		if (!action.GetArgs().empty()) {
			const int drawDist = atoi(action.GetArgs().c_str());
			unitDrawer->SetUnitDrawDist((float)drawDist);
			configHandler->Set("UnitLodDist", drawDist);
			LOG("Set UnitLodDist to %i", drawDist);
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class LODScaleActionExecutor : public IUnsyncedActionExecutor {
public:
	LODScaleActionExecutor() : IUnsyncedActionExecutor("LODScale",
			"Set the scale for either of: LOD (level-of-detail),"
			" shadow-LOD, reflection-LOD, refraction-LOD") {}

	bool Execute(const UnsyncedAction& action) const {
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
			} else {
				LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
			}
		} else {
			LOG_L(L_WARNING, "/%s: wrong syntax", GetCommand().c_str());
		}
		return true;
	}
};



class WireMapActionExecutor : public IUnsyncedActionExecutor {
public:
	WireMapActionExecutor() : IUnsyncedActionExecutor("WireMap",
			"Enable/Disable drawing of the map as wire-frame (no textures) (Graphic setting)") {}

	bool Execute(const UnsyncedAction& action) const {
		CBaseGroundDrawer* gd = readMap->GetGroundDrawer();
		SetBoolArg(gd->WireFrameModeRef(), action.GetArgs());
		// TODO: make this a separate action
		// sky->wireframe = gd->WireFrameMode();
		LogSystemStatus("wireframe map-drawing mode", gd->WireFrameMode());
		return true;
	}
};



class DebugColVolDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugColVolDrawerActionExecutor(): IUnsyncedActionExecutor("DebugColVol", "Enable/Disable drawing of collision volumes") {
	}

	bool Execute(const UnsyncedAction& action) const {
		SetBoolArg(DebugColVolDrawer::enable, action.GetArgs());
		return true;
	}
};


class DebugPathDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugPathDrawerActionExecutor(): IUnsyncedActionExecutor("DebugPath", "Enable/Disable drawing of pathfinder debug-data") {
	}

	bool Execute(const UnsyncedAction& action) const {
		LogSystemStatus("path-debug rendering mode", pathDrawer->ToggleEnabled());
		return true;
	}
};


class DebugTraceRayDrawerActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugTraceRayDrawerActionExecutor(): IUnsyncedActionExecutor("DebugTraceRay", "Enable/Disable drawing of traceray debug-data") {
	}

	bool Execute(const UnsyncedAction& action) const {
		globalRendering->drawdebugtraceray = !globalRendering->drawdebugtraceray;
		LogSystemStatus("traceray debug rendering mode", globalRendering->drawdebugtraceray);
		return true;
	}
};



class CrashActionExecutor : public IUnsyncedActionExecutor {
public:
	CrashActionExecutor() : IUnsyncedActionExecutor("Crash",
			"Invoke an artificial crash through a NULL-pointer dereference (SIGSEGV)", true) {}

	bool Execute(const UnsyncedAction& action) const {
		int* a = 0;
		*a = 0;
		return true;
	}
};



class ExceptionActionExecutor : public IUnsyncedActionExecutor {
public:
	ExceptionActionExecutor() : IUnsyncedActionExecutor("Exception",
			"Invoke an artificial crash by throwing an std::runtime_error", true) {}

	bool Execute(const UnsyncedAction& action) const {
		throw std::runtime_error("Exception test");
		return true;
	}
};



class DivByZeroActionExecutor : public IUnsyncedActionExecutor {
public:
	DivByZeroActionExecutor() : IUnsyncedActionExecutor("DivByZero",
			"Invoke an artificial crash by performing a division-by-Zero", true) {}

	bool Execute(const UnsyncedAction& action) const {
		float a = 0.0f;
		LOG("Result: %f", 1.0f/a);
		return true;
	}
};



class GiveActionExecutor : public IUnsyncedActionExecutor {
public:
	GiveActionExecutor() : IUnsyncedActionExecutor("Give",
			"Places one or multiple units of a single or multiple types on the"
			" map, instantly; by default to your own team", true) {}

	bool Execute(const UnsyncedAction& action) const {
		if (action.GetArgs().find('@') == string::npos) {
			CInputReceiver* ir = NULL;
			if (!game->hideInterface) {
				ir = CInputReceiver::GetReceiverAt(mouse->lastx, mouse->lasty);
			}

			float3 givePos;
			if (ir == minimap) {
				givePos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			} else {
				const float3& pos = camera->GetPos();
				const float3& dir = mouse->dir;
				const float dist = CGround::LineGroundCol(pos, pos + (dir * 30000.0f));
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
		return true;
	}
};



class DestroyActionExecutor : public IUnsyncedActionExecutor {
public:
	DestroyActionExecutor() : IUnsyncedActionExecutor("Destroy",
			"Destroys one or multiple units by unit-ID, instantly", true) {}

	bool Execute(const UnsyncedAction& action) const {
		if (selectedUnitsHandler.selectedUnits.empty())
			return false;

		// kill selected units
		std::stringstream ss;
		ss << GetCommand();
		for (CUnitSet::iterator it = selectedUnitsHandler.selectedUnits.begin();
				it != selectedUnitsHandler.selectedUnits.end(); ++it)
		{
			ss << " " << (*it)->id;
		}
		CommandMessage pckt(ss.str(), gu->myPlayerNum);
		net->Send(pckt.Pack());
		return true;
	}
};



class SendActionExecutor : public IUnsyncedActionExecutor {
public:
	SendActionExecutor() : IUnsyncedActionExecutor("Send",
			"Send a string as raw network message to the game host (for debugging only)") {}

	bool Execute(const UnsyncedAction& action) const {
		CommandMessage pckt(Action(action.GetArgs()), gu->myPlayerNum);
		net->Send(pckt.Pack());
		return true;
	}
};



class SaveGameActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveGameActionExecutor() : IUnsyncedActionExecutor("SaveGame",
			"Save the game state to QuickSave.ssf (BROKEN)") {}

	bool Execute(const UnsyncedAction& action) const {
		game->SaveGame("Saves/QuickSave.ssf", true);
		return true;
	}
};



class DumpStateActionExecutor: public IUnsyncedActionExecutor {
public:
	DumpStateActionExecutor(): IUnsyncedActionExecutor("DumpState", "dump game-state to file") {}

	bool Execute(const UnsyncedAction& action) const {
		const std::vector<std::string>& args = _local_strSpaceTokenize(action.GetArgs());

		switch (args.size()) {
			case 2: { DumpState(atoi(args[0].c_str()), atoi(args[1].c_str()),                     1); } break;
			case 3: { DumpState(atoi(args[0].c_str()), atoi(args[1].c_str()), atoi(args[2].c_str())); } break;
			default: { LOG_L(L_WARNING, "/DumpState: wrong syntax");  } break;
		}

		return true;
	}
};



/// /save [-y ]<savename>
class SaveActionExecutor : public IUnsyncedActionExecutor {
public:
	SaveActionExecutor() : IUnsyncedActionExecutor("Save",
			"Save the game state to a specific file (BROKEN)") {}

	bool Execute(const UnsyncedAction& action) const {

		const bool saveOverride = action.GetArgs().find("-y ") == 0;
		const std::string saveName(action.GetArgs().c_str() + (saveOverride ? 3 : 0));
		const std::string saveFileName = "Saves/" + saveName + ".ssf";
		game->SaveGame(saveFileName, saveOverride);
		return true;
	}
};



class ReloadGameActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadGameActionExecutor() : IUnsyncedActionExecutor("ReloadGame",
			"Restarts the game with the initially provided start-script") {}

	bool Execute(const UnsyncedAction& action) const {
		game->ReloadGame();
		return true;
	}
};



class ReloadShadersActionExecutor : public IUnsyncedActionExecutor {
public:
	ReloadShadersActionExecutor() : IUnsyncedActionExecutor("ReloadShaders",
			"Reloads all engine shaders") {}

	bool Execute(const UnsyncedAction& action) const {
		LOG("Reloading all engine shaders");
		//FIXME make threadsafe!
		shaderHandler->ReloadAll();
		return true;
	}
};



class DebugInfoActionExecutor : public IUnsyncedActionExecutor {
public:
	DebugInfoActionExecutor() : IUnsyncedActionExecutor("DebugInfo",
			"Print debug info to the chat/log-file about either:"
			" sound, profiling") {}

	bool Execute(const UnsyncedAction& action) const {
		if (action.GetArgs() == "sound") {
			sound->PrintDebugInfo();
		} else if (action.GetArgs() == "profiling") {
			profiler.PrintProfilingInfo();
		} else {
			LOG_L(L_WARNING, "Give either of these as argument: sound, profiling");
		}
		return true;
	}
};



class RedirectToSyncedActionExecutor : public IUnsyncedActionExecutor {
public:
	RedirectToSyncedActionExecutor(const std::string& command)
		: IUnsyncedActionExecutor(command,
			"Redirects command /" + command + " to its synced processor")
	{}

	bool Execute(const UnsyncedAction& action) const {
		// redirect as a synced command
		CommandMessage pckt(action.GetInnerAction(), gu->myPlayerNum);
		net->Send(pckt.Pack());
		return true;
	}
};



class CommandListActionExecutor : public IUnsyncedActionExecutor {
public:
	CommandListActionExecutor() : IUnsyncedActionExecutor("CommandList",
			"Prints all the available chat commands with description"
			" (if available) to the console") {}

	bool Execute(const UnsyncedAction& action) const {

		LOG("Chat commands plus description");
		LOG("==============================");
		PrintToConsole(syncedGameCommands->GetActionExecutors());
		PrintToConsole(unsyncedGameCommands->GetActionExecutors());
		return true;
	}

	template<class action_t, bool synced_v>
	static void PrintExecutorToConsole(const IActionExecutor<action_t, synced_v>* executor) {

		LOG("/%-30s  %s  %s",
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

	bool Execute(const UnsyncedAction& action) const {

		const std::vector<std::string> args = CSimpleParser::Tokenize(action.GetArgs(), 1);
		if (!args.empty()) {
			const std::string commandLower = StringToLower(args[0]);

			// try if an unsynced chat command with this name is available
			const IUnsyncedActionExecutor* unsyncedExecutor = unsyncedGameCommands->GetActionExecutor(commandLower);
			if (unsyncedExecutor != NULL) {
				PrintExecutorHelpToConsole(unsyncedExecutor);
				return true;
			}

			// try if a synced chat command with this name is available
			const ISyncedActionExecutor* syncedExecutor = syncedGameCommands->GetActionExecutor(commandLower);
			if (syncedExecutor != NULL) {
				PrintExecutorHelpToConsole(syncedExecutor);
				return true;
			}

			LOG_L(L_WARNING, "No chat command registered with name \"%s\" (case-insensitive)", args[0].c_str());
		} else {
			LOG_L(L_WARNING, "missing command-name");
		}
		return true;
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
		camera->SetMovState(CCamera::MOVE_STATE_FWD, false);
	}
	else if (cmd == "moveback") {
		camera->SetMovState(CCamera::MOVE_STATE_BCK, false);
	}
	else if (cmd == "moveleft") {
		camera->SetMovState(CCamera::MOVE_STATE_LFT, false);
	}
	else if (cmd == "moveright") {
		camera->SetMovState(CCamera::MOVE_STATE_RGT, false);
	}
	else if (cmd == "moveup") {
		camera->SetMovState(CCamera::MOVE_STATE_UP, false);
	}
	else if (cmd == "movedown") {
		camera->SetMovState(CCamera::MOVE_STATE_DWN, false);
	}
	else if (cmd == "movefast") {
		camera->SetMovState(CCamera::MOVE_STATE_FST, false);
	}
	else if (cmd == "moveslow") {
		camera->SetMovState(CCamera::MOVE_STATE_SLW, false);
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
		mouse->ToggleMiddleClickScroll();
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
	AddActionExecutor(new RoamActionExecutor());
	AddActionExecutor(new MapBorderActionExecutor());
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
	AddActionExecutor(new PauseActionExecutor());
	AddActionExecutor(new DebugActionExecutor());
	AddActionExecutor(new DebugColVolDrawerActionExecutor());
	AddActionExecutor(new DebugPathDrawerActionExecutor());
	AddActionExecutor(new DebugTraceRayDrawerActionExecutor());
	AddActionExecutor(new NoSoundActionExecutor());
	AddActionExecutor(new SoundChannelEnableActionExecutor());
	AddActionExecutor(new CreateVideoActionExecutor());
	AddActionExecutor(new DrawTreesActionExecutor());
	AddActionExecutor(new DynamicSkyActionExecutor());
	AddActionExecutor(new DynamicSunActionExecutor());
	AddActionExecutor(new SpeedControlActionExecutor());
	AddActionExecutor(new GameInfoActionExecutor());
	AddActionExecutor(new HideInterfaceActionExecutor());
	AddActionExecutor(new HardwareCursorActionExecutor());
	AddActionExecutor(new FullscreenActionExecutor());
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
	AddActionExecutor(new ShowPathTravActionExecutor());
	AddActionExecutor(new ShowPathHeatActionExecutor());
	AddActionExecutor(new ShowPathFlowActionExecutor());
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
	AddActionExecutor(new ReloadShadersActionExecutor());
	AddActionExecutor(new DebugInfoActionExecutor());

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
		LOG_L(L_WARNING, "UnsyncedGameCommands singleton was not initialized or is already destroyed");
	}
}
