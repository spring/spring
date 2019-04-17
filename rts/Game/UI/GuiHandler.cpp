/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GuiHandler.h"

#include "CommandColors.h"
#include "KeyBindings.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/TraceRay.h"
#include "Lua/LuaTextures.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUI.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/IconHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"

#include <SDL_keycode.h>
#include <SDL_mouse.h>

CONFIG(bool, MiniMapMarker).defaultValue(true).headlessValue(false);
CONFIG(bool, InvertQueueKey).defaultValue(false);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CGuiHandler* guihandler = nullptr;


CGuiHandler::CGuiHandler()
{
	icons.resize(16);

	LoadDefaults();
	LoadDefaultConfig();

	miniMapMarker = configHandler->GetBool("MiniMapMarker");
	invertQueueKey = configHandler->GetBool("InvertQueueKey");

	failedSound = sound->GetDefSoundId("FailedCommand");

}

bool CGuiHandler::GetQueueKeystate() const
{
	return (!invertQueueKey && KeyInput::GetKeyModState(KMOD_SHIFT)) ||
	       (invertQueueKey && !KeyInput::GetKeyModState(KMOD_SHIFT));
}


static bool SafeAtoF(float& var, const std::string& value)
{
	char* endPtr;
	const char* startPtr = value.c_str();
	const float tmp = (float)strtod(startPtr, &endPtr);

	if (endPtr == startPtr)
		return false;

	var = tmp;
	return true;
}



bool CGuiHandler::EnableLuaUI(bool enableCommand)
{
	if (luaUI != nullptr) {
		if (enableCommand) {
			LOG_L(L_WARNING, "[GUIHandler] LuaUI is already enabled");
			return false;
		}

		if (luaUI->IsRunning()) {
			// has to be queued, corrupts the Lua VM if done inside a pcall
			// (LuaRules can reload itself inside callins because the action
			// SendCommands("luarules reload") goes over the network, and is
			// not when received)
			luaUI->QueueAction(CLuaUI::ACTION_RELOAD);
			return true;
		}
	}

	CLuaUI::ReloadHandler();

	if (luaUI != nullptr) {
		LayoutIcons(false);
		return true;
	}

	LOG_L(L_WARNING, "[GUIHandler] loading LuaUI failed; using default config");

	LoadDefaultConfig();
	return false;
}

bool CGuiHandler::DisableLuaUI(bool layoutIcons)
{
	if (luaUI == nullptr) {
		LOG_L(L_WARNING, "[GUIHandler] LuaUI is already disabled");
		return false;
	}

	if (luaUI->IsRunning()) {
		luaUI->QueueAction(CLuaUI::ACTION_DISABLE);
		return true;
	}

	CLuaUI::FreeHandler();

	if (luaUI == nullptr) {
		LoadDefaultConfig();

		// needed; LoadDefaultConfig does not reach ReloadConfigFromString
		if (layoutIcons)
			LayoutIcons(false);

		return true;
	}

	LOG_L(L_WARNING, "[GUIHandler] disabling LuaUI failed");
	return false;
}



bool CGuiHandler::LoadConfig(const std::string& cfg)
{
	CSimpleParser parser(cfg);

	std::string deadStr;
	std::string prevStr;
	std::string nextStr;
	std::string fillOrderStr;

	while (true) {
		const std::string& line = parser.GetCleanLine();

		if (line.empty())
			break;

		const std::vector<std::string>& words = parser.Tokenize(line, 1);

		if (words.size() <= 1)
			continue;

		const std::string& command = StringToLower(words[0]);

		switch (hashString(command.c_str())) {
			case hashString("dropshadows"): {
				dropShadows = !!atoi(words[1].c_str());
			} break;
			case hashString("useoptionleds"): {
				useOptionLEDs = !!atoi(words[1].c_str());
			} break;
			case hashString("selectgaps"): {
				selectGaps = !!atoi(words[1].c_str());
			} break;
			case hashString("selectthrough"): {
				selectThrough = !!atoi(words[1].c_str());
			} break;
			case hashString("deadiconslot"): {
				deadStr = StringToLower(words[1]);
			} break;
			case hashString("prevpageslot"): {
				prevStr = StringToLower(words[1]);
			} break;
			case hashString("nextpageslot"): {
				nextStr = StringToLower(words[1]);
			} break;
			case hashString("fillorder"): {
				fillOrderStr = StringToLower(words[1]);
			} break;
			case hashString("xicons"): {
				xIcons = atoi(words[1].c_str());
			} break;
			case hashString("yicons"): {
				yIcons = atoi(words[1].c_str());
			} break;
			case hashString("xiconsize"): {
				SafeAtoF(xIconSize, words[1]);
			} break;
			case hashString("yiconsize"): {
				SafeAtoF(yIconSize, words[1]);
			} break;
			case hashString("xpos"): {
				SafeAtoF(xPos, words[1]);
			} break;
			case hashString("ypos"): {
				SafeAtoF(yPos, words[1]);
			} break;
			case hashString("textborder"): {
				SafeAtoF(textBorder, words[1]);
			} break;
			case hashString("iconborder"): {
				SafeAtoF(iconBorder, words[1]);
			} break;
			case hashString("frameborder"): {
				SafeAtoF(frameBorder, words[1]);
			} break;
			case hashString("xselectionpos"): {
				SafeAtoF(xSelectionPos, words[1]);
			} break;
			case hashString("yselectionpos"): {
				SafeAtoF(ySelectionPos, words[1]);
			} break;
			case hashString("framealpha"): {
				SafeAtoF(frameAlpha, words[1]);
			} break;
			case hashString("texturealpha"): {
				SafeAtoF(textureAlpha, words[1]);
			} break;
			case hashString("outlinefont"): {
				outlineFonts = !!atoi(words[1].c_str());
			} break;
			case hashString("attackrect"): {
				attackRect = !!atoi(words[1].c_str());
			} break;
			case hashString("newattackmode"): {
				newAttackMode = !!atoi(words[1].c_str());
			} break;
			case hashString("invcolorselect"): {
				invColorSelect = !!atoi(words[1].c_str());
			} break;
			case hashString("frontbyends"): {
				frontByEnds = !!atoi(words[1].c_str());
			} break;
		}
	}

	// sane clamps
	xIcons      = std::max(2,      xIcons);
	yIcons      = std::max(2,      yIcons);
	xIconSize   = std::max(0.010f, xIconSize);
	yIconSize   = std::max(0.010f, yIconSize);
	iconBorder  = std::max(0.0f,   iconBorder);
	frameBorder = std::max(0.0f,   frameBorder);

	xIconStep = xIconSize + (iconBorder * 2.0f);
	yIconStep = yIconSize + (iconBorder * 2.0f);

	iconsPerPage = xIcons * yIcons;

	buttonBox.x1 = xPos;
	buttonBox.x2 = xPos + (frameBorder * 2.0f) + (xIcons * xIconStep);
	buttonBox.y1 = yPos;
	buttonBox.y2 = yPos + (frameBorder * 2.0f) + (yIcons * yIconStep);

	if (!deadStr.empty()) {
		deadIconSlot = ParseIconSlot(deadStr);
	} else {
		deadIconSlot = -1;
	}

	if (!prevStr.empty() && (prevStr != "auto")) {
		if (prevStr == "none") {
			prevPageSlot = -1;
		} else {
			prevPageSlot = ParseIconSlot(prevStr);
		}
	} else {
		prevPageSlot = iconsPerPage - 2;
	}

	if (!nextStr.empty() && (nextStr != "auto")) {
		if (nextStr == "none") {
			nextPageSlot = -1;
		} else {
			nextPageSlot = ParseIconSlot(nextStr);
		}
	} else {
		nextPageSlot = iconsPerPage - 1;
	}

	if (deadIconSlot >= 0) {
		const float fullBorder = frameBorder + iconBorder;
		const float fx = (float)(deadIconSlot % xIcons);
		const float fy = (float)(deadIconSlot / xIcons);
		xBpos = buttonBox.x1 + (fullBorder + (0.5 * xIconSize) + (fx * xIconStep));
		yBpos = buttonBox.y2 - (fullBorder + (0.5 * yIconSize) + (fy * yIconStep));
	}
	else if ((prevPageSlot >= 0) && (nextPageSlot >= 0)) {
		// place in the middle of adjacent paging icons
		const int delta = abs(prevPageSlot - nextPageSlot);
		if ((delta == 1) || (delta == xIcons)) {
			const float fullBorder = frameBorder + iconBorder;
			const float fxp = (float)(prevPageSlot % xIcons);
			const float fyp = (float)(prevPageSlot / xIcons);
			const float fxn = (float)(nextPageSlot % xIcons);
			const float fyn = (float)(nextPageSlot / xIcons);
			const float fx = 0.5f * (fxp + fxn);
			const float fy = 0.5f * (fyp + fyn);
			xBpos = buttonBox.x1 + (fullBorder + (0.5 * xIconSize) + (fx * xIconStep));
			yBpos = buttonBox.y2 - (fullBorder + (0.5 * yIconSize) + (fy * yIconStep));
		} else {
			xBpos = yBpos = -1.0f; // off screen
		}
	} else {
		xBpos = yBpos = -1.0f; // off screen
	}

	ParseFillOrder(fillOrderStr);
	return true;
}


void CGuiHandler::ParseFillOrder(const std::string& text)
{
	// setup the default order
	iconFillOrder.clear();
	iconFillOrder.reserve(iconsPerPage);

	for (int i = 0; i < iconsPerPage; i++)
		iconFillOrder.push_back(i);

	// split the std::string into slot names
	const std::vector<std::string>& slotNames = CSimpleParser::Tokenize(text, 0);

	if ((int)slotNames.size() != iconsPerPage)
		return;

	spring::unordered_set<int> slotSet;
	std::vector<int> slotVec;

	slotSet.reserve(iconsPerPage);
	slotVec.reserve(iconsPerPage);

	for (int s = 0; s < iconsPerPage; s++) {
		const int slotNumber = ParseIconSlot(slotNames[s]);

		if ((slotNumber < 0) || (slotNumber >= iconsPerPage) || (slotSet.find(slotNumber) != slotSet.end()))
			return;

		slotSet.insert(slotNumber);
		slotVec.push_back(slotNumber);
	}

	iconFillOrder = std::move(slotVec);
}


int CGuiHandler::ParseIconSlot(const std::string& text) const
{
	const char rowLetter = tolower(text[0]);

	if ((rowLetter < 'a') || (rowLetter > 'z'))
		return -1;

	const int row = rowLetter - 'a';

	if (row >= yIcons)
		return -1;


	char* endPtr;
	const char* startPtr = text.c_str() + 1;
	int column = strtol(startPtr, &endPtr, 10);

	if ((endPtr == startPtr) || (column < 0) || (column >= xIcons))
		return -1;

	return (row * xIcons) + column;
}


bool CGuiHandler::ReloadConfigFromFile(const std::string& fileName)
{
	if (fileName.empty())
		return (ReloadConfigFromFile(DEFAULT_GUI_CONFIG));

	std::string cfg;

	CFileHandler ifs(fileName);
	ifs.LoadStringData(cfg);

	LOG("Reloading GUI config from file: %s", fileName.c_str());
	return (ReloadConfigFromString(cfg));
}


bool CGuiHandler::ReloadConfigFromString(const std::string& cfg)
{
	LoadConfig(cfg);
	activePage = 0;
	selectedUnitsHandler.SetCommandPage(activePage);
	LayoutIcons(false);
	return true;
}


void CGuiHandler::ResizeIconArray(size_t size)
{
	if (icons.size() < size)
		icons.resize(std::max(icons.size() * 2, size));

	assert(iconsCount <= icons.size());
}


void CGuiHandler::AppendPrevAndNext(std::vector<SCommandDescription>& cmds)
{
	{
		cmds.emplace_back();

		SCommandDescription& cd = cmds.back();

		cd.id      = CMD_INTERNAL;
		cd.type    = CMDTYPE_PREV;

		cd.name    = "";
		cd.action  = "prevmenu";
		cd.tooltip = "Previous menu";
	}
	{
		cmds.emplace_back();

		SCommandDescription& cd = cmds.back();

		cd.id      = CMD_INTERNAL;
		cd.type    = CMDTYPE_NEXT;

		cd.name    = "";
		cd.action  = "nextmenu";
		cd.tooltip = "Next menu";
	}
}


int CGuiHandler::FindInCommandPage()
{
	if ((inCommand < 0) || ((size_t)inCommand >= commands.size()))
		return -1;

	for (int ii = 0; ii < iconsCount; ii++) {
		const IconInfo& icon = icons[ii];

		if (icon.commandsID == inCommand)
			return (ii / iconsPerPage);
	}

	return -1;
}


void CGuiHandler::RevertToCmdDesc(const SCommandDescription& cmdDesc,
                                  bool defaultCommand, bool samePage)
{
	for (size_t a = 0; a < commands.size(); ++a) {
		if (commands[a].id != cmdDesc.id)
			continue;

		if (defaultCommand) {
			defaultCmdMemory = a;
			return;
		}

		inCommand = a;

		if (commands[a].type == CMDTYPE_ICON_BUILDING) {
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(-commands[a].id);
			SetShowingMetal(ud->extractsMetal > 0);
		} else {
			SetShowingMetal(false);
		}

		if (!samePage)
			continue;

		for (int ii = 0; ii < iconsCount; ii++) {
			if (inCommand != icons[ii].commandsID)
				continue;

			activePage = std::min(maxPage, (ii / iconsPerPage));
			selectedUnitsHandler.SetCommandPage(activePage);
		}
	}
}


void CGuiHandler::LayoutIcons(bool useSelectionPage)
{
	bool defCmd;
	bool validInCommand;
	bool samePage;

	SCommandDescription cmdDesc;
	{
		defCmd =
			(mouse->buttons[SDL_BUTTON_RIGHT].pressed &&
			(defaultCmdMemory >= 0) && (inCommand < 0) &&
			((activeReceiver == this) || (minimap->ProxyMode())));

		const size_t activeCmd = defCmd ? defaultCmdMemory : inCommand;

		// copy the current command state
		if ((validInCommand = (activeCmd < commands.size())))
			cmdDesc = commands[activeCmd];

		samePage = validInCommand && (activePage == FindInCommandPage());
		useSelectionPage = useSelectionPage && !samePage;

		// reset some of our state
		inCommand = -1;
		defaultCmdMemory = -1;
		forceLayoutUpdate = false;

		commands.clear();
	}

	if (luaUI != nullptr && luaUI->WantsEvent("LayoutButtons")) {
		if (LayoutCustomIcons(useSelectionPage)) {
			if (validInCommand)
				RevertToCmdDesc(cmdDesc, defCmd, samePage);

			// LuaUI::LayoutButtons returned true, we are done
			return;
		}

		// note: if inside a callin, this only queues up a disable-action
		// we also want to load the default config, but doing that before
		// the handler is freed would make little sense (need to defer it
		// with ForceLayoutUpdate?) so just issue a warning
		// DisableLuaUI(false);

		if (luaUI->IsRunning()) {
			LOG_L(L_WARNING, "[GUIHandler] can not load default GUI from a LuaUI callin");
			return;
		}

		// otherwise revert to engine UI
		CLuaUI::FreeHandler();
		LoadDefaultConfig();
	}


	// get the commands to process
	CSelectedUnitsHandler::AvailableCommandsStruct ac = selectedUnitsHandler.GetAvailableCommands();
	ConvertCommands(ac.commands);

	std::vector<SCommandDescription> hidden;

	hidden.reserve(ac.commands.size());
	commands.reserve(ac.commands.size());

	// separate the visible/hidden icons
	for (const SCommandDescription& cd: ac.commands) {
		if (cd.hidden) {
			hidden.push_back(cd);
		} else {
			commands.push_back(cd);
		}
	}


	// assign extra icons for internal commands
	const int extraIcons = int(deadIconSlot >= 0) + int(prevPageSlot >= 0) + int(nextPageSlot >= 0);

	const int cmdCount        = (int)commands.size();
	const int cmdIconsPerPage = (iconsPerPage - extraIcons);
	const int pageCount       = ((cmdCount + (cmdIconsPerPage - 1)) / cmdIconsPerPage);

	const bool multiPage = (pageCount > 1);

	const int prevPageCmd = cmdCount + 0;
	const int nextPageCmd = cmdCount + 1;

	maxPage    = std::max(0, pageCount - 1);
	iconsCount = pageCount * iconsPerPage;

	// resize the icon array if required
	ResizeIconArray(iconsCount);

	int ci = 0; // command index

	for (int ii = 0; ii < iconsCount; ii++) {
		// map the icon order
		const int tmpSlot = (ii % iconsPerPage);
		const int mii = ii - tmpSlot + iconFillOrder[tmpSlot]; // mapped icon index

		IconInfo& icon = icons[mii];
		const int slot = (mii % iconsPerPage);

		if (slot == deadIconSlot) {
			icon.commandsID = -1;
		}
		else if (slot == nextPageSlot) {
			icon.commandsID = multiPage ? nextPageCmd : -1;
		}
		else if (slot == prevPageSlot) {
			icon.commandsID = multiPage ? prevPageCmd : -1;
		}
		else if (ci >= cmdCount) {
			icon.commandsID = -1;
		}
		else {
			icon.commandsID = ci;
			ci++;
		}

		// setup the geometry
		if (icon.commandsID >= 0) {
			const float fx = (float)(slot % xIcons);
			const float fy = (float)(slot / xIcons);

			const float fullBorder = frameBorder + iconBorder;
			icon.visual.x1 = buttonBox.x1 + (fullBorder + (fx * xIconStep));
			icon.visual.x2 = icon.visual.x1 + xIconSize;
			icon.visual.y1 = buttonBox.y2 - (fullBorder + (fy * yIconStep));
			icon.visual.y2 = icon.visual.y1 - yIconSize;

			const float noGap = selectGaps ? 0.0f : (iconBorder + 0.0005f);
			icon.selection.x1 = icon.visual.x1 - noGap;
			icon.selection.x2 = icon.visual.x2 + noGap;
			icon.selection.y1 = icon.visual.y1 + noGap;
			icon.selection.y2 = icon.visual.y2 - noGap;
		} else {
			// make sure this icon does not get selected
			icon.selection.x1 = icon.selection.x2 = -1.0f;
			icon.selection.y1 = icon.selection.y2 = -1.0f;
		}
	}

	// append the Prev and Next commands  (if required)
	if (multiPage)
		AppendPrevAndNext(commands);

	// append the hidden commands
	for (const SCommandDescription& cd: hidden) {
		commands.push_back(cd);
	}

	// try to setup the old command state
	// (inCommand, activePage, showingMetal)
	if (validInCommand) {
		RevertToCmdDesc(cmdDesc, defCmd, samePage);
	} else if (useSelectionPage) {
		activePage = std::min(maxPage, ac.commandPage);
	}

	activePage = std::min(maxPage, activePage);
}


bool CGuiHandler::LayoutCustomIcons(bool useSelectionPage)
{
	assert(luaUI != nullptr);

	// get the commands to process
	CSelectedUnitsHandler::AvailableCommandsStruct ac = selectedUnitsHandler.GetAvailableCommands();
	std::vector<SCommandDescription> cmds = ac.commands;

	if (!cmds.empty()) {
		ConvertCommands(cmds);
		AppendPrevAndNext(cmds);
	}

	// call for a custom layout
	int tmpXicons = xIcons;
	int tmpYicons = yIcons;

	std::vector<int> removeCmds;
	std::vector<SCommandDescription> customCmds;
	std::vector<int> onlyTextureCmds;
	std::vector<CLuaUI::ReStringPair> reTextureCmds;
	std::vector<CLuaUI::ReStringPair> reNamedCmds;
	std::vector<CLuaUI::ReStringPair> reTooltipCmds;
	std::vector<CLuaUI::ReParamsPair> reParamsCmds;
	spring::unordered_map<int, int> iconMap;

	if (!luaUI->LayoutButtons(
		tmpXicons, tmpYicons,
		cmds, removeCmds, customCmds,
		onlyTextureCmds, reTextureCmds,
		reNamedCmds, reTooltipCmds, reParamsCmds,
		iconMap, menuName)
	) {
		return false;
	}

	if ((tmpXicons < 2) || (tmpYicons < 2)) {
		LOG_L(L_ERROR, "[GUIHandler::%s] bad xIcons or yIcons (%i, %i)", __func__, tmpXicons, tmpYicons);
		return false;
	}


	spring::unordered_set<int> removeIDs;
	std::vector<SCommandDescription> tmpCmds;
	std::vector<int> iconList;

	removeIDs.reserve(removeCmds.size());
	tmpCmds.reserve(cmds.size());
	iconList.reserve(iconMap.size());


	// build a set to better find unwanted commands
	for (int removeCmd : removeCmds) {
		if (removeCmd < cmds.size()) {
			removeIDs.insert(removeCmd);
		} else {
			LOG_L(L_ERROR, "[GUIHandler::%s] skipping bad removeCmd (%i)", __func__, removeCmd);
		}
	}

	// remove unwanted commands  (and mark all as hidden)
	for (unsigned int i = 0; i < cmds.size(); i++) {
		if (removeIDs.find(i) == removeIDs.end()) {
			cmds[i].hidden = true;
			tmpCmds.push_back(cmds[i]);
		}
	}

	cmds = tmpCmds;

	// add the custom commands
	for (auto& customCmd : customCmds) {
		customCmd.hidden = true;
		cmds.push_back(customCmd);
	}
	const size_t cmdCount = cmds.size();

	// set commands to onlyTexture
	for (int onlyTextureCmd : onlyTextureCmds) {
		if (onlyTextureCmd < cmdCount) {
			cmds[onlyTextureCmd].onlyTexture = true;
		} else {
			LOG_L(L_ERROR, "[GUIHandler::%s] skipping bad onlyTexture (%i)", __func__, onlyTextureCmd);
		}
	}

	// retexture commands
	for (const auto& reTextureCmd : reTextureCmds) {
		const size_t index = reTextureCmd.cmdIndex;

		if (index < cmdCount) {
			cmds[index].iconname = reTextureCmd.texture;
		} else {
			LOG_L(L_ERROR, "[GUIHandler::%s] skipping bad reTexture (%i)", __func__, int(index));
		}
	}

	// reNamed commands
	for (const auto& reNamedCmd : reNamedCmds) {
		const size_t index = reNamedCmd.cmdIndex;

		if (index < cmdCount) {
			cmds[index].name = reNamedCmd.texture;
		} else {
			LOG_L(L_ERROR, "[GUIHandler::%s] skipping bad reNamed (%i)", __func__, int(index));
		}
	}

	// reTooltip commands
	for (const auto& reTooltipCmd : reTooltipCmds) {
		const size_t index = reTooltipCmd.cmdIndex;

		if (index < cmdCount) {
			cmds[index].tooltip = reTooltipCmd.texture;
		} else {
			LOG_L(L_ERROR, "[GUIHandler::%s] skipping bad reNamed (%i)", __func__, int(index));
		}
	}

	// reParams commands
	for (const auto& reParamsCmd: reParamsCmds) {
		const size_t index = reParamsCmd.cmdIndex;

		if (index < cmdCount) {
			for (const auto& param: reParamsCmd.params) {
				const size_t p = param.first;

				if (p < cmds[index].params.size()) {
					cmds[index].params[p] = param.second;
				}
			}
		} else {
			LOG_L(L_ERROR, "[GUIHandler::%s] skipping bad reParams (%i)", __func__, int(index));
		}
	}


	int nextPos = 0;
	// build the iconList from the map
	for (const auto& item: iconMap) {
		const int iconPos = item.first;

		if (iconPos < nextPos)
			continue;

		if (iconPos > nextPos) {
			// fill in the blanks
			for (int p = nextPos; p < iconPos; p++) {
				iconList.push_back(-1);
			}
		}

		iconList.push_back(item.second); // cmdIndex
		nextPos = iconPos + 1;
	}


	const size_t iconListCount = iconList.size();
	const size_t tmpIconsPerPage = (tmpXicons * tmpYicons); // always at least 4
	const size_t pageCount = ((iconListCount + (tmpIconsPerPage - 1)) / tmpIconsPerPage);
	const size_t tmpIconsCount = (pageCount * tmpIconsPerPage);

	// resize the icon array if required
	ResizeIconArray(tmpIconsCount);

	// build the iconList
	for (size_t ii = 0; ii < tmpIconsCount; ii++) {
		IconInfo& icon = icons[ii];

		const size_t index = (ii < iconList.size()) ? iconList[ii] : -1;

		if (index < cmdCount) {
			icon.commandsID = index;
			cmds[index].hidden = false;

			const int slot = ii % tmpIconsPerPage;
			const float fx = (float)(slot % tmpXicons);
			const float fy = (float)(slot / tmpXicons);

			const float fullBorder = frameBorder + iconBorder;
			icon.visual.x1 = buttonBox.x1 + (fullBorder + (fx * xIconStep));
			icon.visual.x2 = icon.visual.x1 + xIconSize;
			icon.visual.y1 = buttonBox.y2 - (fullBorder + (fy * yIconStep));
			icon.visual.y2 = icon.visual.y1 - yIconSize;

			const float noGap = selectGaps ? 0.0f : (iconBorder + 0.0005f);
			icon.selection.x1 = icon.visual.x1 - noGap;
			icon.selection.x2 = icon.visual.x2 + noGap;
			icon.selection.y1 = icon.visual.y1 + noGap;
			icon.selection.y2 = icon.visual.y2 - noGap;
		} else {
			// make sure this icon does not get selected
			icon.commandsID = -1;
			icon.selection.x1 = icon.selection.x2 = -1.0f;
			icon.selection.y1 = icon.selection.y2 = -1.0f;
		}
	}

	commands = cmds;

	xIcons       = tmpXicons;
	yIcons       = tmpYicons;
	iconsCount   = tmpIconsCount;
	iconsPerPage = tmpIconsPerPage;

	maxPage = std::max(size_t(0), pageCount - 1);
	if (useSelectionPage) {
		activePage = std::min(maxPage, ac.commandPage);
	} else {
		activePage = std::min(maxPage, activePage);
	}

	buttonBox.x1 = xPos;
	buttonBox.x2 = xPos + (frameBorder * 2.0f) + (xIcons * xIconStep);
	buttonBox.y1 = yPos;
	buttonBox.y2 = yPos + (frameBorder * 2.0f) + (yIcons * yIconStep);

	return true;
}


void CGuiHandler::GiveCommand(const Command& cmd, bool fromUser)
{
	commandsToGive.emplace_back(std::pair<const Command, bool>(cmd, fromUser));
}


void CGuiHandler::GiveCommandsNow() {
	std::vector< std::pair<Command, bool> > commandsToGiveTemp;
	{
		commandsToGiveTemp.swap(commandsToGive);
	}

	for (const std::pair<Command, bool>& i: commandsToGiveTemp) {
		const Command& cmd = i.first;

		if (eventHandler.CommandNotify(cmd))
			continue;

		selectedUnitsHandler.GiveCommand(cmd, i.second);

		if (!gatherMode)
			continue;

		if ((cmd.GetID() == CMD_MOVE) || (cmd.GetID() == CMD_FIGHT)) {
			GiveCommand(Command(CMD_GATHERWAIT), false);
		}
	}
}


void CGuiHandler::ConvertCommands(std::vector<SCommandDescription>& cmds)
{
	if (!newAttackMode)
		return;

	const int count = (int)cmds.size();
	for (int i = 0; i < count; i++) {
		SCommandDescription& cd = cmds[i];

		if ((cd.id != CMD_ATTACK) || (cd.type != CMDTYPE_ICON_UNIT_OR_MAP))
			continue;

		if (attackRect) {
			cd.type = CMDTYPE_ICON_UNIT_OR_RECTANGLE;
		} else {
			cd.type = CMDTYPE_ICON_UNIT_OR_AREA;
		}
	}
}


void CGuiHandler::SetShowingMetal(bool show)
{
	if (!show) {
		if (showingMetal)
			infoTextureHandler->DisableCurrentMode();

		showingMetal = false;
		return;
	}

	if (mapInfo->gui.autoShowMetal) {
		if (infoTextureHandler->GetMode() != "metal")
			infoTextureHandler->SetMode("metal");

		showingMetal = true;
	}
}


void CGuiHandler::Update()
{
	SetCursorIcon();

	{
		if (!invertQueueKey && (needShift && !KeyInput::GetKeyModState(KMOD_SHIFT))) {
			SetShowingMetal(false);
			inCommand = -1;
			needShift = false;
		}
	}

	GiveCommandsNow();

	if (selectedUnitsHandler.CommandsChanged()) {
		SetShowingMetal(false);
		LayoutIcons(true);
		return;
	}
	if (forceLayoutUpdate) {
		LayoutIcons(false);
	}
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::SetCursorIcon() const
{
	string newCursor = "cursornormal";
	float cursorScale = 1.0f;

	CInputReceiver* ir = nullptr;

	if (!game->hideInterface && !mouse->offscreen)
		ir = GetReceiverAt(mouse->lastx, mouse->lasty);

	if ((ir != nullptr) && (ir != minimap)) {
		mouse->ChangeCursor(newCursor, cursorScale);
		return;
	}

	if (ir == minimap)
		cursorScale = minimap->CursorScale();

	const bool useMinimap = (minimap->ProxyMode() || ((activeReceiver != this) && (ir == minimap)));

	if ((inCommand >= 0) && ((size_t)inCommand<commands.size())) {
		const SCommandDescription& cmdDesc = commands[inCommand];

		if (!cmdDesc.mouseicon.empty()) {
			newCursor = cmdDesc.mouseicon;
		} else {
			newCursor = cmdDesc.name;
		}

		if (useMinimap && (cmdDesc.id < 0)) {
			BuildInfo bi;
			bi.pos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			bi.buildFacing = buildFacing;
			bi.def = unitDefHandler->GetUnitDefByID(-cmdDesc.id);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);

			// if an unit (enemy), is not in LOS, then TestUnitBuildSquare()
			// does not consider it when checking for position blocking
			CFeature* feature = nullptr;

			if (!CGameHelper::TestUnitBuildSquare(bi, feature, gu->myAllyTeam, false)) {
				newCursor = "BuildBad";
			} else {
				newCursor = "BuildGood";
			}
		}

		if (!TryTarget(cmdDesc))
			newCursor = "AttackBad";
	}
	else if (!useMinimap || minimap->FullProxy()) {
		int defcmd;

		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed && ((activeReceiver == this) || (minimap->ProxyMode()))) {
			defcmd = defaultCmdMemory;
		} else {
			defcmd = GetDefaultCommand(mouse->lastx, mouse->lasty);
		}

		if ((defcmd >= 0) && ((size_t)defcmd < commands.size())) {
			const SCommandDescription& cmdDesc = commands[defcmd];

			if (!cmdDesc.mouseicon.empty()) {
				newCursor = cmdDesc.mouseicon;
			} else {
				newCursor = cmdDesc.name;
			}
		}
	}

	if (gatherMode && ((newCursor == "Move") || (newCursor == "Fight")))
		newCursor = "GatherWait";

	mouse->ChangeCursor(newCursor, cursorScale);
}


bool CGuiHandler::TryTarget(const SCommandDescription& cmdDesc) const
{
	if (cmdDesc.id != CMD_ATTACK)
		return true;

	if (selectedUnitsHandler.selectedUnits.empty())
		return true;

	// get mouse-hovered map pos
	const CUnit* targetUnit = nullptr;
	const CFeature* targetFeature = nullptr;

	const float viewRange = camera->GetFarPlaneDist() * 1.4f;
	const float dist = TraceRay::GuiTraceRay(camera->GetPos(), mouse->dir, viewRange, nullptr, targetUnit, targetFeature, true);
	const float3 groundPos = camera->GetPos() + mouse->dir * dist;

	if (dist <= 0.0f)
		return false;

	for (const int unitID: selectedUnitsHandler.selectedUnits) {
		const CUnit* u = unitHandler.GetUnit(unitID);

		// mobile kamikaze can always move into range
		//FIXME do a range check in case of immobile kamikaze (-> mines)
		if (u->unitDef->canKamikaze && !u->immobile)
			return true;

		for (const CWeapon* w: u->weapons) {
			float3 modGroundPos = groundPos;
			w->AdjustTargetPosToWater(modGroundPos, targetUnit == nullptr);
			const SWeaponTarget wtrg = SWeaponTarget(targetUnit, modGroundPos);

			if (u->immobile) {
				// immobile unit; check range and weapon target properties
				if (w->TryTarget(wtrg))
					return true;

				continue;
			}

			// mobile units can always move into range; only check
			// if weapon can shoot the target (i.e. anti-air/anti-sub)
			if (w->TestTarget(w->GetLeadTargetPos(wtrg), wtrg))
				return true;
		}
	}

	return false;
}


bool CGuiHandler::MousePress(int x, int y, int button)
{
	{
		if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT && button != -SDL_BUTTON_RIGHT && button != -SDL_BUTTON_LEFT)
			return false;

		if (button < 0) {
			// proxied click from the minimap
			button = -button;
			activeMousePress = true;
		}
		else if (AboveGui(x,y)) {
			activeMousePress = true;

			if ((curIconCommand < 0) && !game->hideInterface) {
				const int iconPos = IconAtPos(x, y);

				if (iconPos >= 0)
					curIconCommand = icons[iconPos].commandsID;

			}

			if (button == SDL_BUTTON_RIGHT)
				inCommand = defaultCmdMemory = -1;

			return true;
		}
		else if (minimap != nullptr && minimap->IsAbove(x, y)) {
			return false; // let the minimap do its job
		}

		if (inCommand >= 0) {
			if (invertQueueKey && (button == SDL_BUTTON_RIGHT) && !mouse->buttons[SDL_BUTTON_LEFT].pressed) {
				// for rocker gestures
				SetShowingMetal(false);
				inCommand = -1;

				return (needShift = false);
			}

			return (activeMousePress = true);
		}
	}
	if (button == SDL_BUTTON_RIGHT) {
		activeMousePress = true;
		defaultCmdMemory = GetDefaultCommand(x, y);
		return true;
	}

	return false;
}


void CGuiHandler::MouseRelease(int x, int y, int button, const float3& cameraPos, const float3& mouseDir)
{
	if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT && button != -SDL_BUTTON_RIGHT && button != -SDL_BUTTON_LEFT)
		return;

	int lastIconCmd = curIconCommand;
	int iconCmd = -1;

	curIconCommand = -1;
	explicitCommand = inCommand;

	if (!activeMousePress)
		return;

	activeMousePress = false;

	if (!invertQueueKey && needShift && !KeyInput::GetKeyModState(KMOD_SHIFT)) {
		SetShowingMetal(false);
		inCommand = -1;
		needShift = false;
	}

	if (button < 0) {
		button = -button; // proxied click from the minimap
	} else {
		// setup iconCmd
		if (!game->hideInterface) {
			const int iconPos = IconAtPos(x, y);

			// if mouse was pressed on one button and released on another, ignore the command
			if (iconPos >= 0 && (iconCmd = icons[iconPos].commandsID) != lastIconCmd)
				iconCmd = -1;
		}
	}

	if ((button == SDL_BUTTON_RIGHT) && (iconCmd == -1)) {
		// right click -> set the default cmd
		inCommand = defaultCmdMemory;
		defaultCmdMemory = -1;
	}

	if (size_t(iconCmd) < commands.size()) {
		SetActiveCommand(iconCmd, button == SDL_BUTTON_RIGHT);
		return;
	}

	// not over a button, try to execute a command
	// note: this executes GiveCommand for build-icon clicks if !preview
	const Command c = GetCommand(x, y, button, false, cameraPos, mouseDir);

	if (c.GetID() == CMD_FAILED) {
		// failed indicates we should not finish the current command
		Channels::UserInterface->PlaySample(failedSound, 5);
		return;
	}

	// stop indicates that no good command could be found
	if (c.GetID() != CMD_STOP) {
		GiveCommand(c);
		lastKeySet.Reset();
	}

	FinishCommand(button);
}


bool CGuiHandler::SetActiveCommand(int cmdIndex, bool rightMouseButton)
{
	if (cmdIndex >= (int)commands.size())
		return false;

	if (cmdIndex < 0) {
		// cancel the current command
		inCommand = -1;
		defaultCmdMemory = -1;

		needShift = false;
		activeMousePress = false;

		SetShowingMetal(false);
		return true;
	}

	SCommandDescription& cd = commands[cmdIndex];
	if (cd.disabled)
		return false;

	lastKeySet.Reset();

	switch (cd.type) {
		case CMDTYPE_ICON: {
			Command c(cd.id);

			if (cd.id != CMD_STOP) {
				c.SetOpts(CreateOptions(rightMouseButton));

				if (invertQueueKey && ((cd.id < 0) || (cd.id == CMD_STOCKPILE)))
					c.SetOpts(c.GetOpts() ^ SHIFT_KEY);
			}

			GiveCommand(c);
		} break;
		case CMDTYPE_ICON_MODE: {
			int newMode = atoi(cd.params[0].c_str()) + 1;

			if (newMode > (static_cast<int>(cd.params.size()) - 2))
				newMode = 0;

			// not really required
			char t[16];
			SNPRINTF(t, sizeof(t), "%d", newMode);
			cd.params[0] = t;

			GiveCommand(Command(cd.id, CreateOptions(rightMouseButton), newMode));
			forceLayoutUpdate = true;
		} break;
		case CMDTYPE_NUMBER:
		case CMDTYPE_ICON_MAP:
		case CMDTYPE_ICON_AREA:
		case CMDTYPE_ICON_UNIT:
		case CMDTYPE_ICON_UNIT_OR_MAP:
		case CMDTYPE_ICON_FRONT:
		case CMDTYPE_ICON_UNIT_OR_AREA:
		case CMDTYPE_ICON_UNIT_OR_RECTANGLE:
		case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA: {
			inCommand = cmdIndex;
			SetShowingMetal(false);
			activeMousePress = false;
		} break;
		case CMDTYPE_ICON_BUILDING: {
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(-cd.id);
			inCommand = cmdIndex;
			SetShowingMetal(ud->extractsMetal > 0);
			activeMousePress = false;
		} break;
		case CMDTYPE_NEXT: {
			activePage += 1;
			activePage  = mix(activePage, 0, activePage > maxPage);

			selectedUnitsHandler.SetCommandPage(activePage);
		} break;
		case CMDTYPE_PREV: {
			activePage -= 1;
			activePage  = mix(activePage, maxPage, activePage < 0);

			selectedUnitsHandler.SetCommandPage(activePage);
		} break;
		case CMDTYPE_CUSTOM: {
			RunCustomCommands(cd.params, rightMouseButton);
		} break;
		default:
			break;
	}

	return true;
}


bool CGuiHandler::SetActiveCommand(
	int cmdIndex,
	int button,
	bool leftMouseButton,
	bool rightMouseButton,
	bool alt,
	bool ctrl,
	bool meta,
	bool shift
) {
	// use the button value instead of rightMouseButton
	const bool effectiveRMB = button != SDL_BUTTON_LEFT;

	// setup the mouse and key states
	const bool  prevLMB   = mouse->buttons[SDL_BUTTON_LEFT].pressed;
	const bool  prevRMB   = mouse->buttons[SDL_BUTTON_RIGHT].pressed;
	const std::uint8_t prevAlt   = KeyInput::GetKeyModState(KMOD_ALT);
	const std::uint8_t prevCtrl  = KeyInput::GetKeyModState(KMOD_CTRL);
	const std::uint8_t prevMeta  = KeyInput::GetKeyModState(KMOD_GUI);
	const std::uint8_t prevShift = KeyInput::GetKeyModState(KMOD_SHIFT);

	mouse->buttons[SDL_BUTTON_LEFT].pressed  = leftMouseButton;
	mouse->buttons[SDL_BUTTON_RIGHT].pressed = rightMouseButton;

	KeyInput::SetKeyModState(KMOD_ALT,   alt);
	KeyInput::SetKeyModState(KMOD_CTRL,  ctrl);
	KeyInput::SetKeyModState(KMOD_GUI,   meta);
	KeyInput::SetKeyModState(KMOD_SHIFT, shift);

	const bool retval = SetActiveCommand(cmdIndex, effectiveRMB);

	// revert the mouse and key states
	KeyInput::SetKeyModState(KMOD_SHIFT, prevShift);
	KeyInput::SetKeyModState(KMOD_GUI,   prevMeta);
	KeyInput::SetKeyModState(KMOD_CTRL,  prevCtrl);
	KeyInput::SetKeyModState(KMOD_ALT,   prevAlt);

	mouse->buttons[SDL_BUTTON_RIGHT].pressed = prevRMB;
	mouse->buttons[SDL_BUTTON_LEFT].pressed  = prevLMB;

	return retval;
}


int CGuiHandler::IconAtPos(int x, int y) // GetToolTip --> IconAtPos
{
	const float fx = MouseX(x);
	const float fy = MouseY(y);

	if ((fx < buttonBox.x1) || (fx > buttonBox.x2)) return -1;
	if ((fy < buttonBox.y1) || (fy > buttonBox.y2)) return -1;

	int xSlot = int((fx - (buttonBox.x1 + frameBorder)) / xIconStep);
	int ySlot = int(((buttonBox.y2 - frameBorder) - fy) / yIconStep);
	xSlot = std::min(std::max(xSlot, 0), xIcons - 1);
	ySlot = std::min(std::max(ySlot, 0), yIcons - 1);
	const int ii = (activePage * iconsPerPage) + (ySlot * xIcons) + xSlot;

	if ((ii < 0) || (ii >= iconsCount))
		return -1;

	if (fx <= icons[ii].selection.x1) return -1;
	if (fx >= icons[ii].selection.x2) return -1;
	if (fy <= icons[ii].selection.y2) return -1;
	if (fy >= icons[ii].selection.y1) return -1;

	return ii;
}


/******************************************************************************/

enum ModState {
	DontCare,
	Required,
	Forbidden
};

struct ModGroup {
	ModState alt = DontCare;
	ModState ctrl = DontCare;
	ModState meta = DontCare;
	ModState shift = DontCare;
	ModState right = DontCare;
};


static bool ParseCustomCmdMods(std::string& cmd, ModGroup& in, ModGroup& out)
{
	const char* c = cmd.c_str();
	if (*c != '@')
		return false;

	c++;
	bool neg = false;
	while ((*c != 0) && (*c != '@')) {
		const char ch = *c;

		if (ch == '-') neg = true;
		else if (ch == '+') neg = false;

		else if (ch == 'a') { in.alt    = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'c') { in.ctrl   = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'm') { in.meta   = neg ? Forbidden : Required; neg = false; }
		else if (ch == 's') { in.shift  = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'r') { in.right  = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'A') { out.alt   = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'C') { out.ctrl  = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'M') { out.meta  = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'S') { out.shift = neg ? Forbidden : Required; neg = false; }
		else if (ch == 'R') { out.right = neg ? Forbidden : Required; neg = false; }

		c++;
	}

	if (*c == 0)
		return false;

	cmd = cmd.substr((c + 1) - cmd.c_str());

	return true;
}


static bool CheckCustomCmdMods(bool rightMouseButton, ModGroup& inMods)
{
	if ((inMods.alt   ==  Required) && !KeyInput::GetKeyModState(KMOD_ALT  )) return false;
	if ((inMods.alt   == Forbidden) &&  KeyInput::GetKeyModState(KMOD_ALT  )) return false;
	if ((inMods.ctrl  ==  Required) && !KeyInput::GetKeyModState(KMOD_CTRL )) return false;
	if ((inMods.ctrl  == Forbidden) &&  KeyInput::GetKeyModState(KMOD_CTRL )) return false;
	if ((inMods.meta  ==  Required) && !KeyInput::GetKeyModState(KMOD_GUI  )) return false;
	if ((inMods.meta  == Forbidden) &&  KeyInput::GetKeyModState(KMOD_GUI  )) return false;
	if ((inMods.shift ==  Required) && !KeyInput::GetKeyModState(KMOD_SHIFT)) return false;
	if ((inMods.shift == Forbidden) &&  KeyInput::GetKeyModState(KMOD_SHIFT)) return false;

	if ((inMods.right ==  Required) && !rightMouseButton) return false;
	if ((inMods.right == Forbidden) &&  rightMouseButton) return false;

	return true;
}


void CGuiHandler::RunCustomCommands(const std::vector<std::string>& cmds, bool rightMouseButton)
{
	static int depth = 0;

	if (depth > 8)
		return; // recursion protection

	depth++;

	for (std::string copy: cmds) {

		ModGroup inMods;  // must match for the action to execute
		ModGroup outMods; // controls the state of the modifiers  (ex: "group1")

		if (ParseCustomCmdMods(copy, inMods, outMods)) {
			if (CheckCustomCmdMods(rightMouseButton, inMods)) {
				const bool tmpAlt   = !!KeyInput::GetKeyModState(KMOD_ALT);
				const bool tmpCtrl  = !!KeyInput::GetKeyModState(KMOD_CTRL);
				const bool tmpMeta  = !!KeyInput::GetKeyModState(KMOD_GUI);
				const bool tmpShift = !!KeyInput::GetKeyModState(KMOD_SHIFT);

				if (outMods.alt   != DontCare)  { KeyInput::SetKeyModState(KMOD_ALT,   int(outMods.alt   == Required)); }
				if (outMods.ctrl  != DontCare)  { KeyInput::SetKeyModState(KMOD_CTRL,  int(outMods.ctrl  == Required)); }
				if (outMods.meta  != DontCare)  { KeyInput::SetKeyModState(KMOD_GUI,   int(outMods.meta  == Required)); }
				if (outMods.shift != DontCare)  { KeyInput::SetKeyModState(KMOD_SHIFT, int(outMods.shift == Required)); }

				Action action(copy);
				if (!ProcessLocalActions(action))
					game->ProcessAction(action);

				KeyInput::SetKeyModState(KMOD_ALT,   tmpAlt);
				KeyInput::SetKeyModState(KMOD_CTRL,  tmpCtrl);
				KeyInput::SetKeyModState(KMOD_GUI,   tmpMeta);
				KeyInput::SetKeyModState(KMOD_SHIFT, tmpShift);
			}
		}
	}

	depth--;
}


bool CGuiHandler::AboveGui(int x, int y)
{
	if (iconsCount <= 0)
		return false;

	if (!selectThrough) {
		const float fx = MouseX(x);
		const float fy = MouseY(y);

		if ((fx > buttonBox.x1) && (fx < buttonBox.x2) && (fy > buttonBox.y1) && (fy < buttonBox.y2))
			return true;

	}
	return (IconAtPos(x,y) >= 0);
}


unsigned char CGuiHandler::CreateOptions(bool rightMouseButton)
{
	unsigned char options = RIGHT_MOUSE_KEY * rightMouseButton;

	if (GetQueueKeystate()) {
		// allow mouse button 'rocker' movements to force
		// immediate mode (when queuing is the default mode)
		options |= (SHIFT_KEY * !invertQueueKey);
		options |= (SHIFT_KEY * (!mouse->buttons[SDL_BUTTON_LEFT].pressed && !mouse->buttons[SDL_BUTTON_RIGHT].pressed));
	}

	if (KeyInput::GetKeyModState(KMOD_CTRL)) { options |= CONTROL_KEY; }
	if (KeyInput::GetKeyModState(KMOD_ALT) ) { options |=     ALT_KEY; }
	if (KeyInput::GetKeyModState(KMOD_GUI))  { options |=    META_KEY; }

	return options;
}
unsigned char CGuiHandler::CreateOptions(int button)
{
	return CreateOptions(button != SDL_BUTTON_LEFT);
}


float CGuiHandler::GetNumberInput(const SCommandDescription& cd) const
{
	float minV =   0.0f;
	float maxV = 100.0f;

	if (!cd.params.empty())
		minV = atof(cd.params[0].c_str());
	if (cd.params.size() >= 2)
		maxV = atof(cd.params[1].c_str());

	const int minX = (globalRendering->viewSizeX * 1) / 4;
	const int maxX = (globalRendering->viewSizeX * 3) / 4;
	const int effX = Clamp(mouse->lastx, minX, maxX);
	const float factor = float(effX - minX) / float(maxX - minX);

	return (minV + (factor * (maxV - minV)));
}

// CALLINFO:
// DrawMapStuff --> GetDefaultCommand
// CMouseHandler::DrawCursor --> DrawCentroidCursor --> GetDefaultCommand
// LuaUnsyncedRead::GetDefaultCommand --> GetDefaultCommand
int CGuiHandler::GetDefaultCommand(int x, int y, const float3& cameraPos, const float3& mouseDir) const
{
	CInputReceiver* ir = nullptr;

	if (!game->hideInterface && !mouse->offscreen)
		ir = GetReceiverAt(x, y);

	if ((ir != nullptr) && (ir != minimap))
		return -1;

	int cmdID = -1;

	{
		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;

		if ((ir == minimap) && (minimap->FullProxy())) {
			unit = minimap->GetSelectUnit(minimap->GetMapPosition(x, y));
		} else {
			const float viewRange = camera->GetFarPlaneDist() * 1.4f;
			const float dist = TraceRay::GuiTraceRay(cameraPos, mouseDir, viewRange, nullptr, unit, feature, true);
			const float3 hit = cameraPos + mouseDir * dist;

			// make sure the ray hit in the map
			if (unit == nullptr && feature == nullptr && !hit.IsInBounds())
				return -1;
		}

		cmdID = selectedUnitsHandler.GetDefaultCmd(unit, feature);
	}

	// make sure the command is currently available
	const auto pred = [&cmdID](const SCommandDescription& cd) { return (cd.id == cmdID); };
	const auto iter = std::find_if(commands.begin(), commands.end(), pred);

	return ((iter != commands.end())? std::distance(commands.begin(), iter): -1);
}


bool CGuiHandler::ProcessLocalActions(const Action& action)
{
	// do not process these actions if the control panel is not visible
	if (iconsCount <= 0)
		return false;

	// only process the build options while building
	// (conserve the keybinding space where we can)
	if ((size_t(inCommand) < commands.size()) &&
			((commands[inCommand].type == CMDTYPE_ICON_BUILDING) ||
			(commands[inCommand].id == CMD_UNLOAD_UNITS))) {

		if (ProcessBuildActions(action))
			return true;
	}

	if (action.command == "buildiconsfirst") {
		activePage = 0;
		selectedUnitsHandler.SetCommandPage(activePage);
		selectedUnitsHandler.ToggleBuildIconsFirst();
		LayoutIcons(false);
		return true;
	}
	else if (action.command == "firstmenu") {
		activePage = 0;
		selectedUnitsHandler.SetCommandPage(activePage);
		return true;
	}
	else if (action.command == "showcommands") {
		// bonus command for debugging
		LOG("Available Commands:");
		for (size_t i = 0; i < commands.size(); ++i) {
			LOG("  command: " _STPF_ ", id = %i, action = %s", i, commands[i].id, commands[i].action.c_str());
		}
		// show the icon/command linkage
		LOG("Icon Linkage:");
		for (int ii = 0; ii < iconsCount; ++ii) {
			LOG("  icon: %i, commandsID = %i", ii, icons[ii].commandsID);
		}
		LOG("maxPage         = %i", maxPage);
		LOG("activePage      = %i", activePage);
		LOG("iconsSize       = %i", int(icons.size()));
		LOG("iconsCount      = %i", iconsCount);
		LOG("commands.size() = " _STPF_, commands.size());
		return true;
	}
	else if (action.command == "invqueuekey") {
		if (action.extra.empty()) {
			invertQueueKey = !invertQueueKey;
		} else {
			invertQueueKey = !!atoi(action.extra.c_str());
		}
		needShift = false;
		configHandler->Set("InvertQueueKey", invertQueueKey ? 1 : 0);
		return true;
	}

	return false;
}


bool CGuiHandler::ProcessBuildActions(const Action& action)
{
	const std::string& arg = StringToLower(action.extra);
	bool ret = false;

	switch (hashString(action.command.c_str())) {
		case hashString("buildspacing"): {
			switch (hashString(arg.c_str())) {
				case hashString("inc"): {
					buildSpacing += 1;
					ret = true;
				} break;
				case hashString("dec"): {
					buildSpacing -= (buildSpacing > 0);
					ret = true;
				} break;
				default: {
				} break;
			}
		} break;

		case hashString("buildfacing"): {
			static const char* buildFaceDirs[] = {"South", "East", "North", "West"};

			switch (hashString(arg.c_str())) {
				case hashString("inc"): {
					buildFacing = (buildFacing +               1) % NUM_FACINGS; ret = true;
				} break;
				case hashString("dec"): {
					buildFacing = (buildFacing + NUM_FACINGS - 1) % NUM_FACINGS; ret = true;
				} break;

				case hashString("south"): {
					buildFacing = FACING_SOUTH; ret = true;
				} break;
				case hashString("east"): {
					buildFacing = FACING_EAST; ret = true;
				} break;
				case hashString("north"): {
					buildFacing = FACING_NORTH; ret = true;
				} break;
				case hashString("west"): {
					buildFacing = FACING_WEST; ret = true;
				} break;
				default: {
				} break;
			}

			LOG("Buildings set to face %s", buildFaceDirs[buildFacing]);
		} break;

		default: {
		} break;
	}

	return ret;
}


int CGuiHandler::GetIconPosCommand(int slot) const // only called by SetActiveCommand
{
	if (slot < 0)
		return -1;

	const int iconPos = slot + (activePage * iconsPerPage);

	if (iconPos < iconsCount)
		return icons[iconPos].commandsID;

	return -1;
}


bool CGuiHandler::KeyPressed(int key, bool isRepeat)
{
	if (key == SDLK_ESCAPE && activeMousePress) {
		activeMousePress = false;
		inCommand = -1;
		SetShowingMetal(false);
		return true;
	}
	if (key == SDLK_ESCAPE && inCommand >= 0) {
		inCommand = -1;
		SetShowingMetal(false);
		return true;
	}

	const CKeySet ks(key, false);

	// setup actionOffset
	//WTF a bit more documentation???
	int tmpActionOffset = actionOffset;

	if ((inCommand < 0) || (lastKeySet.Key() < 0)) {
		actionOffset = 0;
		tmpActionOffset = 0;
		lastKeySet.Reset();
	}
	else if (!ks.IsPureModifier()) {
		// not a modifier
		if ((ks == lastKeySet) && (ks.Key() >= 0)) {
			tmpActionOffset = ++actionOffset;
		} else {
			tmpActionOffset = 0;
		}
	}

	const CKeyBindings::ActionList& al = keyBindings.GetActionList(ks);
	for (int ali = 0; ali < (int)al.size(); ++ali) {
		const int actionIndex = (ali + tmpActionOffset) % (int)al.size(); //????

		if (SetActiveCommand(al[actionIndex], ks, actionIndex))
			return true;
	}

	return false;
}


bool CGuiHandler::SetActiveCommand(const Action& action,
                                   const CKeySet& ks, int actionIndex)
{
	if (ProcessLocalActions(action))
		return true;

	// See if we have a positional icon command
	int iconCmd = -1;
	if (!action.extra.empty() && (action.command == "iconpos"))
		iconCmd = GetIconPosCommand(ParseIconSlot(action.extra));

	for (size_t a = 0; a < commands.size(); ++a) {
		SCommandDescription& cmdDesc = commands[a];

		if ((static_cast<int>(a) != iconCmd) && (cmdDesc.action != action.command))
			continue; // not a match

		if (cmdDesc.disabled)
			continue; // can not use this command

		const int cmdType = cmdDesc.type;
		const bool isBuildOrStockID = (cmdType == CMDTYPE_ICON && (cmdDesc.id < 0 || cmdDesc.id == CMD_STOCKPILE));
		const bool isModeOrBuilding = (cmdType == CMDTYPE_ICON_MODE || cmdType == CMDTYPE_ICON_BUILDING);

		// set the activePage
		if (!cmdDesc.hidden && (isBuildOrStockID || isModeOrBuilding)) {
			for (int ii = 0; ii < iconsCount; ii++) {
				if (icons[ii].commandsID != static_cast<int>(a))
					continue;

				selectedUnitsHandler.SetCommandPage(activePage = std::min(maxPage, (ii / iconsPerPage)));
			}
		}

		switch (cmdType) {
			case CMDTYPE_ICON: {
				Command c(cmdDesc.id);

				if ((cmdDesc.id < 0) || (cmdDesc.id == CMD_STOCKPILE)) {
					if (action.extra == "+5") {
						c.SetOpts(SHIFT_KEY);
					} else if (action.extra == "+20") {
						c.SetOpts(CONTROL_KEY);
					} else if (action.extra == "+100") {
						c.SetOpts(SHIFT_KEY | CONTROL_KEY);
					} else if (action.extra == "-1") {
						c.SetOpts(RIGHT_MOUSE_KEY);
					} else if (action.extra == "-5") {
						c.SetOpts(RIGHT_MOUSE_KEY | SHIFT_KEY);
					} else if (action.extra == "-20") {
						c.SetOpts(RIGHT_MOUSE_KEY | CONTROL_KEY);
					} else if (action.extra == "-100") {
						c.SetOpts(RIGHT_MOUSE_KEY | SHIFT_KEY | CONTROL_KEY);
					}
				}
				else if (action.extra.find("queued") != std::string::npos) {
					c.SetOpts(c.GetOpts() | SHIFT_KEY);
				}

				GiveCommand(c);
			} break;
			case CMDTYPE_ICON_MODE: {
				int newMode;

				if (!action.extra.empty() && (iconCmd < 0)) {
					newMode = atoi(action.extra.c_str());
				} else {
					newMode = atoi(cmdDesc.params[0].c_str()) + 1;
				}

				if ((newMode < 0) || ((size_t)newMode > (cmdDesc.params.size() - 2)))
					newMode = 0;

				// not really required
				char t[16];
				SNPRINTF(t, sizeof(t), "%d", newMode);
				cmdDesc.params[0] = t;

				GiveCommand(Command(cmdDesc.id, 0, newMode));
				forceLayoutUpdate = true;
			} break;
			case CMDTYPE_NUMBER: {
				if (!action.extra.empty()) {
					const SCommandDescription& cd = cmdDesc;

					float value = atof(action.extra.c_str());
					float minV =   0.0f;
					float maxV = 100.0f;

					if (!cd.params.empty())
						minV = atof(cd.params[0].c_str());
					if (cd.params.size() >= 2)
						maxV = atof(cd.params[1].c_str());

					Command c(cd.id, 0, value = Clamp(value, minV, maxV));

					if (action.extra.find("queued") != std::string::npos)
						c.SetOpts(SHIFT_KEY);

					GiveCommand(c);
					break;
				}
				// fall through
			}
			case CMDTYPE_ICON_MAP:
			case CMDTYPE_ICON_AREA:
			case CMDTYPE_ICON_UNIT:
			case CMDTYPE_ICON_UNIT_OR_MAP:
			case CMDTYPE_ICON_FRONT:
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_OR_RECTANGLE:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA: {
				SetShowingMetal(false);
				actionOffset = actionIndex;
				lastKeySet = ks;
				inCommand = a;
			} break;
			case CMDTYPE_ICON_BUILDING: {
				const UnitDef* ud = unitDefHandler->GetUnitDefByID(-cmdDesc.id);
				SetShowingMetal(ud->extractsMetal > 0);
				actionOffset = actionIndex;
				lastKeySet = ks;
				inCommand = a;
			} break;
			case CMDTYPE_NEXT: {
				activePage += 1;
				activePage  = mix(activePage, 0, activePage > maxPage);

				selectedUnitsHandler.SetCommandPage(activePage);
			} break;
			case CMDTYPE_PREV: {
				activePage -= 1;
				activePage  = mix(activePage, maxPage, activePage < 0);

				selectedUnitsHandler.SetCommandPage(activePage);
			} break;
			case CMDTYPE_CUSTOM: {
				RunCustomCommands(cmdDesc.params, false);
			} break;

			default: {
				lastKeySet.Reset();
				SetShowingMetal(false);
				inCommand = a;
			} break;
		}

		return true; // we used the command
	}

	return false;	// couldn't find a match
}


bool CGuiHandler::KeyReleased(int key)
{
	return false;
}

void CGuiHandler::FinishCommand(int button)
{
	if ((button == SDL_BUTTON_LEFT) && (KeyInput::GetKeyModState(KMOD_SHIFT) || invertQueueKey)) {
		needShift = true;
	} else {
		SetShowingMetal(false);
		inCommand = -1;
	}
}


bool CGuiHandler::IsAbove(int x, int y)
{
	return AboveGui(x, y);
}


std::string CGuiHandler::GetTooltip(int x, int y)
{
	std::string s;

	const int iconPos = IconAtPos(x, y);
	const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;

	if ((iconCmd >= 0) && (iconCmd < (int)commands.size())) {
		if (!commands[iconCmd].tooltip.empty()) {
			s = commands[iconCmd].tooltip;
		} else {
			s = commands[iconCmd].name;
		}

		const CKeyBindings::HotkeyList& hl = keyBindings.GetHotkeys(commands[iconCmd].action);

		if (!hl.empty()) {
			s += "\nHotkeys:";

			for (const std::string& hk: hl) {
				s += " " + hk;
			}
		}
	}

	return s;
}

// CALLINFO:
// luaunsyncedread::getcurrenttooltip --> mousehandler::getcurrenttooltip
// tooltipconsole::draw --> mousehandler::getcurrenttooltip
// mousehandler::getcurrenttooltip --> GetBuildTooltip
// mousehandler::getcurrenttooltip --> CMiniMap::gettooltip --> GetBuildTooltip
std::string CGuiHandler::GetBuildTooltip() const
{
	if ((size_t(inCommand) < commands.size()) && (commands[inCommand].type == CMDTYPE_ICON_BUILDING))
		return commands[inCommand].tooltip;

	return "";
}


Command CGuiHandler::GetOrderPreview()
{
	return GetCommand(mouse->lastx, mouse->lasty, -1, true);
}


inline Command CheckCommand(Command c) {
	// always allow queued commands, since conditions may change s.t. the command becomes valid
	if (selectedUnitsHandler.selectedUnits.empty() || (c.GetOpts() & SHIFT_KEY))
		return c;

	for (const int unitID: selectedUnitsHandler.selectedUnits) {
		const CUnit* u = unitHandler.GetUnit(unitID);
		CCommandAI* cai = u->commandAI;

		if (cai->AllowedCommand(c, false))
			return c;
	}

	return Command(CMD_FAILED);
}

bool ZeroRadiusAllowed(const Command &c) {
	switch (c.GetID()) {
		case CMD_CAPTURE:
		case CMD_GUARD:
		case CMD_LOAD_UNITS:
		case CMD_RECLAIM:
		case CMD_REPAIR:
		case CMD_RESTORE:
		case CMD_RESURRECT:
			return false;
	}

	return true;
}

Command CGuiHandler::GetCommand(int mouseX, int mouseY, int buttonHint, bool preview, const float3& cameraPos, const float3& mouseDir)
{
	const Command defaultRet(CMD_FAILED);

	int tempInCommand = inCommand;
	int button = 0;

	if (buttonHint >= SDL_BUTTON_LEFT) {
		button = buttonHint;
	} else if (inCommand != -1) {
		button = SDL_BUTTON_LEFT;
	} else if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
		button = SDL_BUTTON_RIGHT;
	} else {
		return Command(CMD_STOP);
	}

	if (button == SDL_BUTTON_RIGHT && preview) {
		// right click -> default cmd
		// (in preview we might not have default cmd memory set)
		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
			tempInCommand = defaultCmdMemory;
		} else {
			tempInCommand = GetDefaultCommand(mouseX, mouseY, cameraPos, mouseDir);
		}
	}

	if (size_t(tempInCommand) >= commands.size()) {
		if (!preview)
			inCommand = -1;

		return Command(CMD_STOP);
	}

	switch (commands[tempInCommand].type) {
		case CMDTYPE_NUMBER: {
			return CheckCommand(Command(commands[tempInCommand].id, CreateOptions(button), GetNumberInput(commands[tempInCommand])));
		}

		case CMDTYPE_ICON: {
			Command c(commands[tempInCommand].id, CreateOptions(button));
			if (button == SDL_BUTTON_LEFT && !preview)
				LOG_L(L_WARNING, "CMDTYPE_ICON left button press in incommand test? This should not happen.");

			return CheckCommand(c);
		}

		case CMDTYPE_ICON_MAP: {
			float dist = CGround::LineGroundCol(cameraPos, cameraPos + (mouseDir * camera->GetFarPlaneDist() * 1.4f), false);

			if (dist < 0.0f)
				dist = CGround::LinePlaneCol(cameraPos, mouseDir, camera->GetFarPlaneDist() * 1.4f, cameraPos.y + (mouseDir.y * camera->GetFarPlaneDist() * 1.4f));

			return CheckCommand(Command(commands[tempInCommand].id, CreateOptions(button), cameraPos + (mouseDir * dist)));
		}

		case CMDTYPE_ICON_BUILDING: {
			const UnitDef* unitdef = unitDefHandler->GetUnitDefByID(-commands[inCommand].id);
			const float dist = CGround::LineGroundWaterCol(cameraPos, mouseDir, camera->GetFarPlaneDist() * 1.4f, unitdef->floatOnWater, false);

			if (dist < 0.0f)
				return defaultRet;


			if (unitdef == nullptr)
				return Command(CMD_STOP);

			const BuildInfo bi(unitdef, cameraPos + mouseDir * dist, buildFacing);

			if (GetQueueKeystate() && (button == SDL_BUTTON_LEFT)) {
				const float3 camTracePos = mouse->buttons[SDL_BUTTON_LEFT].camPos;
				const float3 camTraceDir = mouse->buttons[SDL_BUTTON_LEFT].dir;

				const float traceDist = camera->GetFarPlaneDist() * 1.4f;
				const float isectDist = CGround::LineGroundWaterCol(camTracePos, camTraceDir, traceDist, unitdef->floatOnWater, false);

				GetBuildPositions(BuildInfo(unitdef, camTracePos + camTraceDir * isectDist, buildFacing), bi, cameraPos, mouseDir);
			} else {
				GetBuildPositions(bi, bi, cameraPos, mouseDir);
			}

			if (buildInfoQueue.empty())
				return Command(CMD_STOP);

			if (buildInfoQueue.size() == 1) {
				CFeature* feature = nullptr;

				// TODO: maybe also check out-of-range for immobile builder?
				if (!CGameHelper::TestUnitBuildSquare(buildInfoQueue[0], feature, gu->myAllyTeam, false))
					return defaultRet;

			}

			if (!preview) {
				// only issue if more than one entry, i.e. user created some
				// kind of line/area queue (caller handles the last command)
				for (auto beg = buildInfoQueue.cbegin(), end = --buildInfoQueue.cend(); beg != end; ++beg) {
					GiveCommand(beg->CreateCommand(CreateOptions(button)));
				}
			}

			return CheckCommand((buildInfoQueue.back()).CreateCommand(CreateOptions(button)));
		}

		case CMDTYPE_ICON_UNIT: {
			const CUnit* unit = nullptr;
			const CFeature* feature = nullptr;

			TraceRay::GuiTraceRay(cameraPos, mouseDir, camera->GetFarPlaneDist() * 1.4f, nullptr, unit, feature, true);

			if (unit == nullptr)
				return defaultRet;

			Command c(commands[tempInCommand].id, CreateOptions(button));
			c.PushParam(unit->id);
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_UNIT_OR_MAP: {
			Command c(commands[tempInCommand].id, CreateOptions(button));

			const CUnit* unit = nullptr;
			const CFeature* feature = nullptr;

			const float traceDist = camera->GetFarPlaneDist() * 1.4f;
			const float isectDist = TraceRay::GuiTraceRay(cameraPos, mouseDir, traceDist, nullptr, unit, feature, true);

			if (isectDist > (traceDist - 300.0f))
				return defaultRet;

			if (unit != nullptr) {
				// clicked on unit
				c.PushParam(unit->id);
			} else {
				// clicked in map
				c.PushPos(cameraPos + (mouseDir * isectDist));
			}
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_FRONT: {
			const float3 camTracePos = mouse->buttons[button].camPos;
			const float3 camTraceDir = mouse->buttons[button].dir;

			const float traceDist = camera->GetFarPlaneDist() * 1.4f;
			const float innerDist = CGround::LineGroundCol(camTracePos, camTracePos + camTraceDir * traceDist, false);
			      float outerDist = -1.0f;

			if (innerDist < 0.0f)
				return defaultRet;

			float3 innerPos = camTracePos + (camTraceDir * innerDist);
			float3 outerPos;

			Command c(commands[tempInCommand].id, CreateOptions(button), innerPos);

			if (mouse->buttons[button].movement > mouse->dragFrontCommandThreshold) {
				// only create the front if the mouse has moved enough
				if ((outerDist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * traceDist, false)) < 0.0f)
					outerDist = CGround::LinePlaneCol(cameraPos, mouseDir, traceDist, innerPos.y);

				ProcessFrontPositions(innerPos, outerPos = cameraPos + (mouseDir * outerDist));

				const char* paramStr = (commands[tempInCommand].params.empty())? nullptr: commands[tempInCommand].params[0].c_str();

				if (paramStr != nullptr && innerPos.SqDistance2D(outerPos) > Square(std::atof(paramStr)))
					outerPos = innerPos + (outerPos - innerPos).ANormalize() * std::atof(paramStr);

				c.SetPos(0, innerPos);
				c.PushPos(outerPos);
			}

			return CheckCommand(c);
		}

		case CMDTYPE_ICON_UNIT_OR_AREA:
		case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
		case CMDTYPE_ICON_AREA: {
			float maxRadius = 100000.0f;

			if (commands[tempInCommand].params.size() == 1)
				maxRadius = atof(commands[tempInCommand].params[0].c_str());

			Command c(commands[tempInCommand].id, CreateOptions(button));

			if (mouse->buttons[button].movement <= mouse->dragCircleCommandThreshold) {
				const CUnit* unit = nullptr;
				const CFeature* feature = nullptr;
				const float dist2 = TraceRay::GuiTraceRay(cameraPos, mouseDir, camera->GetFarPlaneDist() * 1.4f, nullptr, unit, feature, true);

				if (dist2 > (camera->GetFarPlaneDist() * 1.4f - 300) && (commands[tempInCommand].type != CMDTYPE_ICON_UNIT_FEATURE_OR_AREA))
					return defaultRet;

				if (feature && commands[tempInCommand].type == CMDTYPE_ICON_UNIT_FEATURE_OR_AREA) { // clicked on feature
					c.PushParam(unitHandler.MaxUnits() + feature->id);
				} else if (unit && commands[tempInCommand].type != CMDTYPE_ICON_AREA) { // clicked on unit
					if (c.GetID() == CMD_RESURRECT)
						return defaultRet; // cannot resurrect units!

					c.PushParam(unit->id);
				} else {
					// clicked in map; only attack ground if command explicitly set
					if (explicitCommand < 0 || !ZeroRadiusAllowed(c))
						return defaultRet;

					c.PushPos(cameraPos + (mouseDir * dist2));
					c.PushParam(0); // zero radius

					if (c.GetID() == CMD_UNLOAD_UNITS)
						c.PushParam((float)buildFacing);
				}
			} else {
				// create circular area-command
				// find area center-position by tracing from camera along
				// mouse direction stored when area started being dragged
				const float3 camTracePos = mouse->buttons[button].camPos;
				const float3 camTraceDir = mouse->buttons[button].dir;

				const float traceDist = camera->GetFarPlaneDist() * 1.4f;
				const float innerDist = CGround::LineGroundCol(camTracePos, camTracePos + camTraceDir * traceDist, false);
				      float outerDist = -1.0f;

				if (innerDist < 0.0f)
					return defaultRet;

				const float3 innerPos = camTracePos + camTraceDir * innerDist;

				if ((outerDist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * traceDist, false)) < 0.0f)
					outerDist = CGround::LinePlaneCol(cameraPos, mouseDir, traceDist, innerPos.y);

				c.PushPos(innerPos);
				c.PushParam(std::min(maxRadius, innerPos.distance2D(cameraPos + mouseDir * outerDist)));

				if (c.GetID() == CMD_UNLOAD_UNITS)
					c.PushParam((float)buildFacing);
			}

			return CheckCommand(c);
		}

		case CMDTYPE_ICON_UNIT_OR_RECTANGLE: {
			Command c(commands[tempInCommand].id, CreateOptions(button));

			if (mouse->buttons[button].movement <= mouse->dragBoxCommandThreshold) {
				const CUnit* unit = nullptr;
				const CFeature* feature = nullptr;

				const float traceDist = camera->GetFarPlaneDist() * 1.4f;
				const float outerDist = TraceRay::GuiTraceRay(cameraPos, mouseDir, traceDist, nullptr, unit, feature, true);

				if (outerDist > (traceDist - 300.0f))
					return defaultRet;

				if (unit != nullptr) {
					// clicked on unit
					c.PushParam(unit->id);
				} else {
					// clicked in map; only attack ground if command explicitly set
					if (explicitCommand < 0)
						return defaultRet;

					c.PushPos(cameraPos + (mouseDir * outerDist));
				}
			} else {
				// create rectangular area-command
				const float3 camTracePos = mouse->buttons[button].camPos;
				const float3 camTraceDir = mouse->buttons[button].dir;

				const float traceDist = camera->GetFarPlaneDist() * 1.4f;
				const float innerDist = CGround::LineGroundCol(camTracePos, camTracePos + camTraceDir * traceDist, false);
				      float outerDist = -1.0f;

				if (innerDist < 0.0f)
					return defaultRet;

				const float3 innerPos = camTracePos + camTraceDir * innerDist;

				if ((outerDist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * traceDist, false)) < 0.0f)
					outerDist = CGround::LinePlaneCol(cameraPos, mouseDir, traceDist, innerPos.y);

				c.PushPos(camTracePos + camTraceDir * innerDist);
				c.PushPos(cameraPos + mouseDir * outerDist);
			}

			return CheckCommand(c);
		}
	}

	return Command(CMD_STOP);
}



static bool WouldCancelAnyQueued(const BuildInfo& b)
{
	const Command c = b.CreateCommand();

	for (const int unitID: selectedUnitsHandler.selectedUnits) {
		const CUnit* u = unitHandler.GetUnit(unitID);

		if (u->commandAI->WillCancelQueued(c))
			return true;

	}

	return false;
}

static void FillRowOfBuildPos(const BuildInfo& startInfo, float x, float z, float xstep, float zstep, int n, int facing, bool nocancel, std::vector<BuildInfo>& ret)
{
	for (int i = 0; i < n; ++i) {
		BuildInfo bi(startInfo.def, float3(x, 0.0f, z), (startInfo.buildFacing + facing) % NUM_FACINGS);
		bi.pos = CGameHelper::Pos2BuildPos(bi, false);

		if (!nocancel || !WouldCancelAnyQueued(bi))
			ret.push_back(bi);

		x += xstep;
		z += zstep;
	}
}


size_t CGuiHandler::GetBuildPositions(const BuildInfo& startInfo, const BuildInfo& endInfo, const float3& cameraPos, const float3& mouseDir)
{
	// both builds must have the same unitdef
	assert(startInfo.def == endInfo.def);

	float3 start = CGameHelper::Pos2BuildPos(startInfo, false);
	float3 end = CGameHelper::Pos2BuildPos(endInfo, false);

	BuildInfo other; // the unit around which buildings can be circled

	buildInfoQueue.clear();
	buildInfoQueue.reserve(16);

	if (GetQueueKeystate() && KeyInput::GetKeyModState(KMOD_CTRL)) {
		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;

		TraceRay::GuiTraceRay(cameraPos, mouseDir, camera->GetFarPlaneDist() * 1.4f, nullptr, unit, feature, startInfo.def->floatOnWater);

		if (unit != nullptr) {
			other.def = unit->unitDef;
			other.pos = unit->pos;
			other.buildFacing = unit->buildFacing;
		} else {
			const Command c = CGameHelper::GetBuildCommand(cameraPos, mouseDir);

			if (c.GetID() < 0 && c.GetNumParams() == 4) {
				other.pos = c.GetPos(0);
				other.def = unitDefHandler->GetUnitDefByID(-c.GetID());
				other.buildFacing = int(c.GetParam(3));
			}
		}
	}

	if (other.def && GetQueueKeystate() && KeyInput::GetKeyModState(KMOD_CTRL)) {
		// circle build around building
		const int oxsize = other.GetXSize() * SQUARE_SIZE;
		const int ozsize = other.GetZSize() * SQUARE_SIZE;
		const int xsize = startInfo.GetXSize() * SQUARE_SIZE;
		const int zsize = startInfo.GetZSize() * SQUARE_SIZE;

		start = end = CGameHelper::Pos2BuildPos(other, false);
		start.x -= oxsize / 2;
		start.z -= ozsize / 2;
		end.x += oxsize / 2;
		end.z += ozsize / 2;

		const int nvert = 1 + (oxsize / xsize);
		const int nhori = 1 + (ozsize / xsize);

		FillRowOfBuildPos(startInfo, end.x   + zsize / 2, start.z + xsize / 2,      0,  xsize, nhori, 3, true, buildInfoQueue);
		FillRowOfBuildPos(startInfo, end.x   - xsize / 2, end.z   + zsize / 2, -xsize,      0, nvert, 2, true, buildInfoQueue);
		FillRowOfBuildPos(startInfo, start.x - zsize / 2, end.z   - xsize / 2,      0, -xsize, nhori, 1, true, buildInfoQueue);
		FillRowOfBuildPos(startInfo, start.x + xsize / 2, start.z - zsize / 2,  xsize,      0, nvert, 0, true, buildInfoQueue);
	} else {
		// rectangle or line
		const float3 delta = end - start;

		const float xsize = SQUARE_SIZE * (startInfo.GetXSize() + buildSpacing * 2);
		const float zsize = SQUARE_SIZE * (startInfo.GetZSize() + buildSpacing * 2);

		const int xnum = (int)((math::fabs(delta.x) + xsize * 1.4f)/xsize);
		const int znum = (int)((math::fabs(delta.z) + zsize * 1.4f)/zsize);

		float xstep = (int)((0 < delta.x) ? xsize : -xsize);
		float zstep = (int)((0 < delta.z) ? zsize : -zsize);

		if (KeyInput::GetKeyModState(KMOD_ALT)) {
			// build a (filled or hollow) rectangle
			if (KeyInput::GetKeyModState(KMOD_CTRL)) {
				if ((1 < xnum) && (1 < znum)) {
					// go "down" on the "left" side
					FillRowOfBuildPos(startInfo, start.x                     , start.z + zstep             ,      0,  zstep, znum - 1, 0, false, buildInfoQueue);
					// go "right" on the "bottom" side
					FillRowOfBuildPos(startInfo, start.x + xstep             , start.z + (znum - 1) * zstep,  xstep,      0, xnum - 1, 0, false, buildInfoQueue);
					// go "up" on the "right" side
					FillRowOfBuildPos(startInfo, start.x + (xnum - 1) * xstep, start.z + (znum - 2) * zstep,      0, -zstep, znum - 1, 0, false, buildInfoQueue);
					// go "left" on the "top" side
					FillRowOfBuildPos(startInfo, start.x + (xnum - 2) * xstep, start.z                     , -xstep,      0, xnum - 1, 0, false, buildInfoQueue);
				} else if (1 == xnum) {
					FillRowOfBuildPos(startInfo, start.x, start.z, 0, zstep, znum, 0, false, buildInfoQueue);
				} else if (1 == znum) {
					FillRowOfBuildPos(startInfo, start.x, start.z, xstep, 0, xnum, 0, false, buildInfoQueue);
				}
			} else {
				// filled
				int zn = 0;
				for (float z = start.z; zn < znum; ++zn) {
					if (zn & 1) {
						// every odd line "right" to "left"
						FillRowOfBuildPos(startInfo, start.x + (xnum - 1) * xstep, z, -xstep, 0, xnum, 0, false, buildInfoQueue);
					} else {
						// every even line "left" to "right"
						FillRowOfBuildPos(startInfo, start.x                     , z,  xstep, 0, xnum, 0, false, buildInfoQueue);
					}
					z += zstep;
				}
			}
		} else {
			// build a line
			const bool xDominatesZ = (math::fabs(delta.x) > math::fabs(delta.z));

			if (xDominatesZ) {
				zstep = KeyInput::GetKeyModState(KMOD_CTRL) ? 0 : xstep * delta.z / (delta.x ? delta.x : 1);
			} else {
				xstep = KeyInput::GetKeyModState(KMOD_CTRL) ? 0 : zstep * delta.x / (delta.z ? delta.z : 1);
			}

			FillRowOfBuildPos(startInfo, start.x, start.z, xstep, zstep, xDominatesZ ? xnum : znum, 0, false, buildInfoQueue);
		}
	}

	return (buildInfoQueue.size());
}


void CGuiHandler::ProcessFrontPositions(float3& pos0, const float3& pos1)
{
	// rotate around corner
	if (!frontByEnds)
		return;

	// rotate around center
	pos0 = pos1 + ((pos0 - pos1) * 0.5f);
	pos0.y = CGround::GetHeightReal(pos0.x, pos0.z, false);
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::Draw()
{
	if ((iconsCount <= 0) && (luaUI == nullptr))
		return;

	glAttribStatePtr->PushEnableBit();

	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (iconsCount > 0)
		DrawMenu();

	glAttribStatePtr->PopBits();
}


static std::string FindCornerText(const std::string& corner, const vector<std::string>& params)
{
	const auto pred = [&](const std::string& param) { return (param.find(corner) == 0); };
	const auto iter = std::find_if(params.begin(), params.end(), pred);

	if (iter == params.end())
		return "";

	return ((*iter).substr(corner.length()));
}


bool CGuiHandler::DrawMenuIconCustomTexture(const Box& iconBox, const SCommandDescription& cmdDesc, GL::RenderDataBufferTC* rdBuffer) const
{
	bool blockIconFrame = false;

	if (!(blockIconFrame = DrawMenuIconTexture(iconBox, cmdDesc.iconname, rdBuffer)) || !cmdDesc.onlyTexture)
		DrawMenuIconName(iconBox, cmdDesc.name, false);

	DrawMenuIconNWtext(iconBox, FindCornerText("$nw$", cmdDesc.params));
	DrawMenuIconSWtext(iconBox, FindCornerText("$sw$", cmdDesc.params));
	DrawMenuIconNEtext(iconBox, FindCornerText("$ne$", cmdDesc.params));
	DrawMenuIconSEtext(iconBox, FindCornerText("$se$", cmdDesc.params));
	return blockIconFrame;
}


bool CGuiHandler::DrawMenuUnitBuildIcon(const Box& iconBox, GL::RenderDataBufferTC* rdBuffer, int unitDefID) const
{
	// if cmdDesc.id was >= 0, unitDefID will be <= 0 here and this returns a nullptr
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

	if (ud == nullptr)
		return false;

	glBindTexture(GL_TEXTURE_2D, unitDrawer->GetUnitDefImage(ud));

	rdBuffer->SafeAppend({{iconBox.x1, iconBox.y1, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x2, iconBox.y1, 0.0f}, 1.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x2, iconBox.y2, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});

	rdBuffer->SafeAppend({{iconBox.x2, iconBox.y2, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x1, iconBox.y2, 0.0f}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x1, iconBox.y1, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->Submit(GL_TRIANGLES);

	return true;
}


static inline bool ParseTextures(
	const std::string& texString,
	std::string& tex1,
	std::string& tex2,
	float& xscale,
	float& yscale
) {
	// format:  "&<xscale>x<yscale>&<tex>1&<tex2>"  --  <>'s are not included

	char* endPtr;

	const char* c = texString.c_str() + 1;

	xscale = std::strtod(c, &endPtr);
	if ((endPtr == c) || (endPtr[0] != 'x'))
		return false;
	c = endPtr + 1;

	yscale = std::strtod(c, &endPtr);
	if ((endPtr == c) || (endPtr[0] != '&'))
		return false;

	c = endPtr + 1;

	const char* tex1Start = c;
	while ((c[0] != 0) && (c[0] != '&')) { c++; }

	if (c[0] != '&')
		return false;

	const int tex1Len = c - tex1Start;

	tex1 = ++c; // draw 'tex2' first
	tex2 = std::string(tex1Start, tex1Len);

	return true;
}


static inline bool BindUnitTexByString(const std::string& str)
{
	char* endPtr;
	const char* startPtr = str.c_str() + 1; // skip the '#'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);

	if (endPtr == startPtr)
		return false; // bad unitID spec

	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

	if (ud == nullptr)
		return false;

	glBindTexture(GL_TEXTURE_2D, unitDrawer->GetUnitDefImage(ud));
	return true;
}


static inline bool BindIconTexByString(const std::string& str)
{
	char* endPtr;
	const char* startPtr = str.c_str() + 1; // skip the '^'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);

	if (endPtr == startPtr)
		return false; // bad unitID spec

	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

	if (ud == nullptr)
		return false;

	ud->iconType->BindTexture();
	return true;
}


static inline bool BindLuaTexByString(const std::string& str)
{
	CLuaHandle* luaHandle = nullptr;
	const char scriptType = str[1];
	switch (scriptType) {
		case 'u': { luaHandle = luaUI; break; }
		case 'g': { luaHandle = &luaGaia->unsyncedLuaHandle; break; }
		case 'm': { luaHandle = &luaRules->unsyncedLuaHandle; break; }
		default:  { break; }
	}
	if (luaHandle == nullptr)
		return false;

	if (str[2] != LuaTextures::prefix) // '!'
		return false;

	const LuaTextures& luaTextures = luaHandle->GetTextures();
	const LuaTextures::Texture* texInfo = luaTextures.GetInfo(str.substr(2));

	if (texInfo == nullptr)
		return false;

	if (texInfo->target != GL_TEXTURE_2D)
		return false;

	glBindTexture(GL_TEXTURE_2D, texInfo->id);
	return true;
}


static bool BindTextureString(const std::string& str)
{
	if (str[0] == '#')
		return BindUnitTexByString(str);

	if (str[0] == '^')
		return BindIconTexByString(str);

	if (str[0] == LuaTextures::prefix) // '!'
		return BindLuaTexByString(str);

	return CNamedTextures::Bind(str);
}


bool CGuiHandler::DrawMenuIconTexture(const Box& iconBox, const std::string& texName, GL::RenderDataBufferTC* rdBuffer) const
{
	if (texName.empty())
		return false;

	std::string tex1;
	std::string tex2;
	float xscale = 1.0f;
	float yscale = 1.0f;

	// double texture?
	if (texName[0] == '&') {
		if (!ParseTextures(texName, tex1, tex2, xscale, yscale))
			return false;
	} else {
		tex1 = texName;
	}

	// bind the texture for the full size quad
	if (!BindTextureString(tex1)) {
		if (tex2.empty())
			return false;

		if (!BindTextureString(tex2))
			return false;

		tex2.clear(); // cancel the scaled draw
	}



	// draw the full size quad
	rdBuffer->SafeAppend({{iconBox.x1, iconBox.y1, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x2, iconBox.y1, 0.0f}, 1.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x2, iconBox.y2, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});

	rdBuffer->SafeAppend({{iconBox.x2, iconBox.y2, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x1, iconBox.y2, 0.0f}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{iconBox.x1, iconBox.y1, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->Submit(GL_TRIANGLES);

	if (tex2.empty())
		return true; // success, no second texture to draw

	// bind the texture for the scaled quad
	if (!BindTextureString(tex2))
		return false;

	assert(xscale <= 0.5f); //border >= 50% makes no sense
	assert(yscale <= 0.5f);

	// calculate the scaled quad
	const float x1 = iconBox.x1 + (xIconSize * xscale);
	const float x2 = iconBox.x2 - (xIconSize * xscale);
	const float y1 = iconBox.y1 - (yIconSize * yscale);
	const float y2 = iconBox.y2 + (yIconSize * yscale);

	// draw the scaled quad
	rdBuffer->SafeAppend({{x1, y1, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{x2, y1, 0.0f}, 1.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{x2, y2, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});

	rdBuffer->SafeAppend({{x2, y2, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{x1, y2, 0.0f}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->SafeAppend({{x1, y1, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, textureAlpha}});
	rdBuffer->Submit(GL_TRIANGLES);
	return true;
}


void CGuiHandler::DrawMenuIconFrame(const Box& iconBox, const SColor& color, GL::RenderDataBufferC* rdBuffer, float fudge) const
{
	rdBuffer->SafeAppend({{iconBox.x1 + fudge, iconBox.y1 - fudge, 0.0f}, color});
	rdBuffer->SafeAppend({{iconBox.x2 - fudge, iconBox.y1 - fudge, 0.0f}, color});
	rdBuffer->SafeAppend({{iconBox.x2 - fudge, iconBox.y2 + fudge, 0.0f}, color});

	rdBuffer->SafeAppend({{iconBox.x2 - fudge, iconBox.y2 + fudge, 0.0f}, color});
	rdBuffer->SafeAppend({{iconBox.x1 + fudge, iconBox.y2 + fudge, 0.0f}, color});
	rdBuffer->SafeAppend({{iconBox.x1 + fudge, iconBox.y1 - fudge, 0.0f}, color});
}


void CGuiHandler::DrawMenuIconName(const Box& iconBox, const std::string& text, bool offsetForLEDs) const
{
	if (text.empty())
		return;

	const float yShrink = mix(0.0f, 0.125f * yIconSize, offsetForLEDs);

	const float tWidth  = std::max(0.01f, font->GetSize() * font->GetTextWidth(text) * globalRendering->pixelX);  // FIXME
	const float tHeight = std::max(0.01f, font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY); // FIXME merge in 1 function?
	const float textBorder2 = (2.0f * textBorder);
	const float xScale = (xIconSize - textBorder2          ) / tWidth;
	const float yScale = (yIconSize - textBorder2 - yShrink) / tHeight;
	const float fontScale = std::min(xScale, yScale);

	const float xCenter = 0.5f * (iconBox.x1 + iconBox.x2);
	const float yCenter = 0.5f * (iconBox.y1 + iconBox.y2 + yShrink);

	font->SetTextColor(1.0f, 1.0f, 1.0f, 1.0f);
	font->glPrint(xCenter, yCenter, fontScale, (FONT_SHADOW * dropShadows) | FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, text);
}




void CGuiHandler::DrawMenuIconNWtext(const Box& iconBox, const std::string& text) const
{
	if (text.empty())
		return;

	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = iconBox.x1 + textBorder + 0.002f;
	const float yPos = iconBox.y1 - textBorder - 0.006f;

	font->glPrint(xPos, yPos, fontScale, FONT_TOP | FONT_SCALE | FONT_NORM | FONT_BUFFERED, text);
}

void CGuiHandler::DrawMenuIconSWtext(const Box& iconBox, const std::string& text) const
{
	if (text.empty())
		return;

	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = iconBox.x1 + textBorder + 0.002f;
	const float yPos = iconBox.y2 + textBorder + 0.002f;

	font->glPrint(xPos, yPos, fontScale, FONT_SCALE | FONT_NORM | FONT_BUFFERED, text);
}

void CGuiHandler::DrawMenuIconNEtext(const Box& iconBox, const std::string& text) const
{
	if (text.empty())
		return;

	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = iconBox.x2 - textBorder - 0.002f;
	const float yPos = iconBox.y1 - textBorder - 0.006f;

	font->glPrint(xPos, yPos, fontScale, FONT_TOP | FONT_RIGHT | FONT_SCALE | FONT_NORM | FONT_BUFFERED, text);
}

void CGuiHandler::DrawMenuIconSEtext(const Box& iconBox, const std::string& text) const
{
	if (text.empty())
		return;

	const float textHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float  fontScale = (yIconSize * 0.2f) / textHeight;

	const float xPos = iconBox.x2 - textBorder - 0.002f;
	const float yPos = iconBox.y2 + textBorder + 0.002f;

	font->glPrint(xPos, yPos, fontScale, FONT_RIGHT | FONT_SCALE | FONT_NORM | FONT_BUFFERED, text);
}




void CGuiHandler::DrawMenu()
{
	GL::RenderDataBufferTC* bufferTC = GL::GetRenderBufferTC();
	GL::RenderDataBufferC*  bufferC  = GL::GetRenderBufferC();

	Shader::IProgramObject* shaderTC = bufferTC->GetShader(); // used for button textures
	Shader::IProgramObject* shaderC  = bufferC->GetShader(); // used for everything else

	shaderC->Enable();
	shaderC->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shaderC->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
	shaderC->SetUniform("u_alpha_test_ctrl", 0.0099f, 1.0f, 0.0f, 0.0f); // test > 0.0099


	const float boxAlpha = mix(frameAlpha, guiAlpha, frameAlpha < 0.0f);

	const int mouseIconIdx = IconAtPos(mouse->lastx, mouse->lasty);
	const int   minIconIdx = Clamp(activePage * iconsPerPage, 0, iconsCount); // activePage can be -1
	const int   maxIconIdx = Clamp(minIconIdx + iconsPerPage, 0, iconsCount);

	int prevArrowIconIdx = -1;
	int nextArrowIconIdx = -1;
	int numDisabledIcons =  0;

	menuIconDrawFlags.clear();
	menuIconDrawFlags.resize(maxIconIdx - minIconIdx);

	menuIconIndices.clear();
	menuIconIndices.reserve(maxIconIdx - minIconIdx);

	std::fill(menuIconDrawFlags.begin(), menuIconDrawFlags.end(), 0);

	typedef void(CGuiHandler::*TextDrawFunc)(const Box&, const std::string&) const;
	typedef void(CGuiHandler::*ArrowDrawFunc)(const Box&, const SColor&, GL::RenderDataBufferC*) const;

	constexpr unsigned int polyModes[] = {GL_FILL, GL_LINE};

	const CMouseHandler::ButtonPressEvt& bpl = mouse->buttons[SDL_BUTTON_LEFT ];
	const CMouseHandler::ButtonPressEvt& bpr = mouse->buttons[SDL_BUTTON_RIGHT];

	const SColor bgrndColors[] = {{0.3f, 0.0f, 0.0f, 1.00f}, {0.0f, 0.0f, 0.2f, 1.00f}, {0.2f, 0.0f, 0.0f, 1.00f}};
	const SColor frameColors[] = {{1.0f, 1.0f, 0.0f, 0.75f}, {1.0f, 1.0f, 1.0f, 0.50f}, {1.0f, 0.0f, 0.0f, 0.50f}};
	const SColor arrowColors[] = {{0.7f, 0.7f, 0.7f, 1.00f}, {1.0f, 1.0f, 0.0f, 1.00f}};



	if (boxAlpha > 0.01f) {
		// frame background
		bufferC->SafeAppend({{buttonBox.x1, buttonBox.y1, 0.0f}, {0.2f, 0.2f, 0.2f, boxAlpha}});
		bufferC->SafeAppend({{buttonBox.x1, buttonBox.y2, 0.0f}, {0.2f, 0.2f, 0.2f, boxAlpha}});
		bufferC->SafeAppend({{buttonBox.x2, buttonBox.y2, 0.0f}, {0.2f, 0.2f, 0.2f, boxAlpha}});

		bufferC->SafeAppend({{buttonBox.x2, buttonBox.y2, 0.0f}, {0.2f, 0.2f, 0.2f, boxAlpha}});
		bufferC->SafeAppend({{buttonBox.x2, buttonBox.y1, 0.0f}, {0.2f, 0.2f, 0.2f, boxAlpha}});
		bufferC->SafeAppend({{buttonBox.x1, buttonBox.y1, 0.0f}, {0.2f, 0.2f, 0.2f, boxAlpha}});
		bufferC->Submit(GL_TRIANGLES);
	}

	// filter invalid icons
	for (int iconIdx = minIconIdx; iconIdx < maxIconIdx; iconIdx++) {
		const IconInfo& icon = icons[iconIdx];

		if (icon.commandsID < 0 || icon.commandsID >= commands.size())
			continue;

		menuIconIndices.push_back(iconIdx);
		numDisabledIcons += commands[icon.commandsID].disabled;
	}

	{
		for (bool outline: {true, false}) {
			// LED outlines and filled interiors; latter must come second
			glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, polyModes[outline]);

			for (int iconIdx: menuIconIndices) {
				const IconInfo& icon = icons[iconIdx];
				const SCommandDescription& cmdDesc = commands[icon.commandsID];

				if ((cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM))
					continue;
				if (!useOptionLEDs || cmdDesc.type != CMDTYPE_ICON_MODE)
					continue;

				DrawMenuIconOptionLEDs(icon.visual, commands[icon.commandsID], bufferC, outline);
			}

			bufferC->Submit(outline? GL_LINES: GL_TRIANGLES);
		}
	}
	{
		shaderC->Disable();
		shaderTC->Enable();
		shaderTC->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shaderTC->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		shaderTC->SetUniform("u_alpha_test_ctrl", 0.0099f, 1.0f, 0.0f, 0.0f); // test > 0.0099

		// icon textures (TODO: atlas these)
		for (int iconIdx: menuIconIndices) {
			const IconInfo& icon = icons[iconIdx];
			const SCommandDescription& cmdDesc = commands[icon.commandsID];

			const bool customCommand = (cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM);
			// const bool highlightIcon = ((mouseIconIdx == iconIdx) || (icon.commandsID == inCommand)) && (!customCommand || !cmdDesc.params.empty());

			// flag any icons that should not have decorations drawn on
			// top of them; custom commands decide this for themselves
			menuIconDrawFlags[iconIdx - minIconIdx] += 1;
			menuIconDrawFlags[iconIdx - minIconIdx] += (customCommand?
				DrawMenuIconCustomTexture(icon.visual, commands[icon.commandsID], bufferTC):
				DrawMenuIconTexture(icon.visual, cmdDesc.iconname, bufferTC) || DrawMenuUnitBuildIcon(icon.visual, bufferTC, -cmdDesc.id)
			);
		}

		shaderTC->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
		shaderTC->Disable();
		shaderC->Enable();
	}
	{
		glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE);

		// (default and custom) icon background highlights; use additive blending
		for (int iconIdx: menuIconIndices) {
			const IconInfo& icon = icons[iconIdx];
			const SCommandDescription& cmdDesc = commands[icon.commandsID];

			const bool activeCommand = (icon.commandsID == inCommand);
			const bool customCommand = (cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM);
			const bool highlightIcon = ((mouseIconIdx == iconIdx) || activeCommand) && (!customCommand || !cmdDesc.params.empty());

			if (!highlightIcon)
				continue;

			DrawMenuIconFrame(icon.visual, bgrndColors[(1 + bpl.pressed) * (1 - activeCommand)], bufferC, 0.0f);
		}

		bufferC->Submit(GL_TRIANGLES);
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	{
		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, polyModes[1]);

		// (default and custom) icon frame highlights
		for (int iconIdx: menuIconIndices) {
			const IconInfo& icon = icons[iconIdx];
			const SCommandDescription& cmdDesc = commands[icon.commandsID];

			const bool activeCommand = (icon.commandsID == inCommand);
			const bool customCommand = (cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM);
			const bool highlightIcon = ((mouseIconIdx == iconIdx) || activeCommand) && (!customCommand || !cmdDesc.params.empty());

			if (!highlightIcon)
				continue;

			DrawMenuIconFrame(icon.visual, frameColors[(1 + (bpl.pressed || bpr.pressed)) * (1 - activeCommand)], bufferC, 0.0f);
		}

		// icon frames and labels
		for (int iconIdx: menuIconIndices) {
			const IconInfo& icon = icons[iconIdx];
			const SCommandDescription& cmdDesc = commands[icon.commandsID];

			// custom commands are only (optionally, if untextured) decorated by a frame
			if ((cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM)) {
				if (menuIconDrawFlags[iconIdx - minIconIdx] == 1)
					DrawMenuIconFrame(icon.visual, {1.0f, 1.0f, 1.0f, 0.1f}, bufferC);

				continue;
			}

			// build-count text, always visible
			if (cmdDesc.id < 0) {
				constexpr TextDrawFunc funcs[] = {
					&CGuiHandler::DrawMenuIconSWtext,
					&CGuiHandler::DrawMenuIconNEtext,
					&CGuiHandler::DrawMenuIconNWtext,
					&CGuiHandler::DrawMenuIconSEtext,
				};

				for (size_t i = 1, n = std::min(sizeof(funcs) / sizeof(funcs[0]), cmdDesc.params.size()); i <= n; i++) {
					(this->*funcs[i - 1])(icon.visual, cmdDesc.params[i - 1]);
				}
			}

			if (cmdDesc.onlyTexture)
				continue;

			if ((cmdDesc.type == CMDTYPE_PREV) || (cmdDesc.type == CMDTYPE_NEXT)) {
				prevArrowIconIdx = mix(iconIdx, prevArrowIconIdx, cmdDesc.type != CMDTYPE_PREV);
				nextArrowIconIdx = mix(iconIdx, nextArrowIconIdx, cmdDesc.type != CMDTYPE_NEXT);
			} else if (menuIconDrawFlags[iconIdx - minIconIdx] == 1) {
				// no texture and no arrow, just draw a frame
				DrawMenuIconFrame(icon.visual, {1.0f, 1.0f, 1.0f, 0.1f}, bufferC);
			}

			if (menuIconDrawFlags[iconIdx - minIconIdx] == 2)
				continue;

			// draw the command name (or parameter)
			if (cmdDesc.type == CMDTYPE_ICON_MODE && !cmdDesc.params.empty()) {
				const size_t opt = atoi(cmdDesc.params[0].c_str()) + 1;

				if (opt < cmdDesc.params.size())
					DrawMenuIconName(icon.visual, cmdDesc.params[opt], useOptionLEDs && (cmdDesc.type == CMDTYPE_ICON_MODE));

				continue;
			}

			DrawMenuIconName(icon.visual, cmdDesc.name, useOptionLEDs && (cmdDesc.type == CMDTYPE_ICON_MODE));
		}

		// should be submitted as GL_LINE_LOOP, but this is faster
		bufferC->Submit(GL_TRIANGLES);
		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, polyModes[0]);
	}

	if (numDisabledIcons > 0) {
		glAttribStatePtr->BlendFunc(GL_DST_COLOR, GL_ZERO);

		// darken disabled commands
		for (int iconIdx: menuIconIndices) {
			const IconInfo& icon = icons[iconIdx];
			const SCommandDescription& cmdDesc = commands[icon.commandsID];
			const Box& b = icon.visual;

			if (!cmdDesc.disabled)
				continue;

			bufferC->SafeAppend({{b.x1, b.y1, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
			bufferC->SafeAppend({{b.x2, b.y1, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
			bufferC->SafeAppend({{b.x2, b.y2, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});

			bufferC->SafeAppend({{b.x2, b.y2, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
			bufferC->SafeAppend({{b.x1, b.y2, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
			bufferC->SafeAppend({{b.x1, b.y1, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
		}

		bufferC->Submit(GL_TRIANGLES);
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	{
		// prev- and next-page arrows
		constexpr ArrowDrawFunc funcs[] = {&CGuiHandler::DrawMenuPrevArrow, &CGuiHandler::DrawMenuNextArrow};

		for (int arrowIconIdx: {prevArrowIconIdx, nextArrowIconIdx}) {
			if (arrowIconIdx == -1)
				continue;

			const IconInfo& icon = icons[arrowIconIdx];
			const SCommandDescription& cmdDesc = commands[icon.commandsID];
			const ArrowDrawFunc& drawFunc = funcs[arrowIconIdx == nextArrowIconIdx];

			const bool customCommand = (cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM);
			const bool highlightIcon = ((mouseIconIdx == arrowIconIdx) || (icon.commandsID == inCommand)) && (!customCommand || !cmdDesc.params.empty());

			(this->*drawFunc)(icon.visual, arrowColors[highlightIcon], bufferC);
		}

		bufferC->Submit(GL_TRIANGLES);
	}


	// active page indicator
	if (luaUI == nullptr) {
		if (selectedUnitsHandler.BuildIconsFirst()) {
			font->SetTextColor(cmdColors.build[0], cmdColors.build[1], cmdColors.build[2], cmdColors.build[3]);
		} else {
			font->SetTextColor(0.7f, 0.7f, 0.7f, 1.0f);
		}

		font->glFormat(xBpos, yBpos, 1.2f, FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%i", activePage + 1);
	}

	DrawMenuName(bufferC);

	// LuaUI can handle this
	if (luaUI == nullptr || drawSelectionInfo)
		DrawMenuSelectionInfo(bufferC);

	// submits for us
	DrawMenuNumberInput(bufferC);

	shaderC->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	shaderC->Disable();
	font->DrawBufferedGL4();
}


void CGuiHandler::DrawMenuName(GL::RenderDataBufferC* rdBuffer) const
{
	if (menuName.empty() || (iconsCount == 0))
		return;

	const float fontScale = 1.0f;
	const float xp = 0.5f * (buttonBox.x1 + buttonBox.x2);
	const float yp = buttonBox.y2 + (yIconSize * 0.125f);

	if (!outlineFonts) {
		const float textHeight = fontScale * font->GetTextHeight(menuName) * globalRendering->pixelY;

		rdBuffer->SafeAppend({{buttonBox.x1, buttonBox.y2                                   , 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{buttonBox.x1, buttonBox.y2 + textHeight + (yIconSize * 0.25f), 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{buttonBox.x2, buttonBox.y2 + textHeight + (yIconSize * 0.25f), 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});

		rdBuffer->SafeAppend({{buttonBox.x2, buttonBox.y2 + textHeight + (yIconSize * 0.25f), 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{buttonBox.x2, buttonBox.y2                                   , 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{buttonBox.x1, buttonBox.y2                                   , 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		// done by caller
		// rdBuffer->Submit(GL_TRIANGLES);

		font->glPrint(xp, yp, fontScale, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, menuName);
	} else {
		font->SetColors(); // default
		font->glPrint(xp, yp, fontScale, FONT_CENTER | FONT_OUTLINE | FONT_SCALE | FONT_NORM | FONT_BUFFERED, menuName);
	}
}


void CGuiHandler::DrawMenuSelectionInfo(GL::RenderDataBufferC* rdBuffer) const
{
	if (selectedUnitsHandler.selectedUnits.empty())
		return;

	std::ostringstream buf;

	if (selectedUnitsHandler.GetSelectedGroup() != -1) {
		buf << "Selected units " << selectedUnitsHandler.selectedUnits.size() << " [Group " << selectedUnitsHandler.GetSelectedGroup() << "]";
	} else {
		buf << "Selected units " << selectedUnitsHandler.selectedUnits.size();
	}

	const float fontScale = 1.0f;
	const float fontSize  = fontScale * smallFont->GetSize();

	if (!outlineFonts) {
		float descender;

		const float textHeight = fontSize * smallFont->GetTextHeight(buf.str(), &descender) * globalRendering->pixelY;
		const float textWidth  = fontSize * smallFont->GetTextWidth(buf.str()) * globalRendering->pixelX;
		const float textDescender = fontSize * descender * globalRendering->pixelY; //! descender is always negative

		const float x1 = xSelectionPos - frameBorder;
		const float x2 = xSelectionPos + frameBorder + textWidth;
		const float y1 = ySelectionPos - frameBorder;
		const float y2 = ySelectionPos + frameBorder + textHeight - textDescender;

		rdBuffer->SafeAppend({{x1, y1, 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{x1, y2, 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{x2, y2, 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});

		rdBuffer->SafeAppend({{x2, y2, 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{x2, y1, 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		rdBuffer->SafeAppend({{x1, y1, 0.0f}, {0.2f, 0.2f, 0.2f, guiAlpha}});
		// done by caller
		// rdBuffer->Submit(GL_TRIANGLES);

		smallFont->SetTextColor(1.0f, 1.0f, 1.0f, 0.8f);
		smallFont->glPrint(xSelectionPos, ySelectionPos - textDescender, fontSize, FONT_BASELINE | FONT_NORM | FONT_BUFFERED, buf.str());
	} else {
		smallFont->SetColors(); // default
		smallFont->glPrint(xSelectionPos, ySelectionPos, fontSize, FONT_OUTLINE | FONT_NORM | FONT_BUFFERED, buf.str());
	}
}


void CGuiHandler::DrawMenuNumberInput(GL::RenderDataBufferC* rdBuffer) const
{
	// draw the value for CMDTYPE_NUMBER commands
	if (size_t(inCommand) >= commands.size())
		return;

	const SCommandDescription& cd = commands[inCommand];

	if (cd.type != CMDTYPE_NUMBER)
		return;

	const float value = GetNumberInput(cd);
	const float mouseX = mouse->lastx * globalRendering->pixelX;
	// const float mouseY = 1.0f - (mouse->lasty - 16) * globalRendering->pixelY;
	const float slideX = std::min(std::max(mouseX, 0.25f), 0.75f);


	rdBuffer->SafeAppend({{0.235f, 0.45f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.235f, 0.55f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.250f, 0.55f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.250f, 0.55f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.250f, 0.45f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.235f, 0.45f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});

	rdBuffer->SafeAppend({{0.750f, 0.45f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.750f, 0.55f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.765f, 0.55f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.765f, 0.55f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.765f, 0.45f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.750f, 0.45f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.8f}});

	rdBuffer->SafeAppend({{0.25f, 0.49f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.25f, 0.51f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.75f, 0.51f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.75f, 0.51f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.75f, 0.49f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.8f}});
	rdBuffer->SafeAppend({{0.25f, 0.49f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.8f}});
	// rdBuffer->Submit(GL_TRIANGLES);


	rdBuffer->SafeAppend({{slideX + 0.015f, 0.55f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
	rdBuffer->SafeAppend({{slideX - 0.015f, 0.55f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
	rdBuffer->SafeAppend({{slideX         , 0.50f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
	rdBuffer->SafeAppend({{slideX - 0.015f, 0.45f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
	rdBuffer->SafeAppend({{slideX + 0.015f, 0.45f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
	rdBuffer->SafeAppend({{slideX         , 0.50f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
	rdBuffer->Submit(GL_TRIANGLES);

	font->SetTextColor(1.0f, 1.0f, 1.0f, 0.9f);
	font->glFormat(slideX, 0.56f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%i", (int)value);
}



void CGuiHandler::DrawMenuPrevArrow(const Box& iconBox, const SColor& color, GL::RenderDataBufferC* rdBuffer) const
{
	const float yCenter = 0.5f * (iconBox.y1 + iconBox.y2);
	const float xSize = 0.166f * math::fabs(iconBox.x2 - iconBox.x1);
	const float ySize = 0.125f * math::fabs(iconBox.y2 - iconBox.y1);
	const float xSiz2 = 2.0f * xSize;

	#if 0
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter - ySize, 0.0f}, color}); // v0 (bot)
	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter        , 0.0f}, color}); // v2 (tip)
	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter + ySize, 0.0f}, color}); // v4 (top)
	#else
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter - ySize, 0.0f}, color}); // v0 (bot)
	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter + ySize, 0.0f}, color}); // v4 (top)

	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter + ySize, 0.0f}, color}); // v4 (top)

	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter        , 0.0f}, color}); // v2 (tip)
	rdBuffer->SafeAppend({{iconBox.x1 + xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)
	#endif
}

void CGuiHandler::DrawMenuNextArrow(const Box& iconBox, const SColor& color, GL::RenderDataBufferC* rdBuffer) const
{
	const float yCenter = 0.5f * (iconBox.y1 + iconBox.y2);
	const float xSize = 0.166f * math::fabs(iconBox.x2 - iconBox.x1);
	const float ySize = 0.125f * math::fabs(iconBox.y2 - iconBox.y1);
	const float xSiz2 = 2.0f * xSize;

	#if 0
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter - ySize, 0.0f}, color}); // v0 (bot)
	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter        , 0.0f}, color}); // v2 (tip)
	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter + ySize, 0.0f}, color}); // v4 (top)
	#else
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter - ySize, 0.0f}, color}); // v0 (bot)
	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)

	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter + ySize, 0.0f}, color}); // v4 (top)
	rdBuffer->SafeAppend({{iconBox.x1 + xSize, yCenter - ySize, 0.0f}, color}); // v0 (bot)

	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter - ySize, 0.0f}, color}); // v1 (bot)
	rdBuffer->SafeAppend({{iconBox.x2 - xSize, yCenter        , 0.0f}, color}); // v2 (tip)
	rdBuffer->SafeAppend({{iconBox.x2 - xSiz2, yCenter + ySize, 0.0f}, color}); // v3 (top)
	#endif
}



void CGuiHandler::DrawMenuIconOptionLEDs(const Box& iconBox, const SCommandDescription& cmdDesc, GL::RenderDataBufferC* rdBuffer, bool outline) const
{
	const size_t pCount = cmdDesc.params.size();

	if (pCount < 3)
		return;

	const int option = atoi(cmdDesc.params[0].c_str());

	const float xs = xIconSize / float(1 + ((pCount - 1) * 2));
	const float ys = yIconSize * 0.125f;
	const float x1 = iconBox.x1;
	const float y2 = iconBox.y2;
	const float yp = globalRendering->pixelY;

	const SColor colors[] = {
		{0.25f, 0.25f, 0.25f, 0.50f}, // dark

		{1.0f, 0.0f, 0.0f, 0.75f}, // red
		{0.0f, 1.0f, 0.0f, 0.75f}, // green

		{1.0f, 0.0f, 0.0f, 0.75f}, // red
		{1.0f, 1.0f, 0.0f, 0.75f}, // yellow
		{0.0f, 1.0f, 0.0f, 0.75f}, // green

		{0.75f, 0.75f, 0.75f, 0.75f}, // light
	};

	for (size_t i = 0, c = 0, n = pCount - 1; i < n; i++) {
		switch (pCount) {
			case 3 : { c = 1 +       (option != 0); } break;
			case 4 : { c = 3 + std::min(option, 2); } break;
			default: { c = 6                      ; } break;
		}

		// mask color-index
		c *= (i == option);

		const float startx = x1 + (xs * float(1 + (2 * i)));
		const float starty = y2 + (3.0f * yp) + textBorder;

		if (outline) {
			rdBuffer->SafeAppend({{startx     , starty     , 0.0f}, {1.0f, 1.0f, 1.0f, 0.5f}});
			rdBuffer->SafeAppend({{startx     , starty + ys, 0.0f}, {1.0f, 1.0f, 1.0f, 0.5f}});
			rdBuffer->SafeAppend({{startx + xs, starty + ys, 0.0f}, {1.0f, 1.0f, 1.0f, 0.5f}});
			rdBuffer->SafeAppend({{startx + xs, starty     , 0.0f}, {1.0f, 1.0f, 1.0f, 0.5f}});
		} else {
			rdBuffer->SafeAppend({{startx     , starty     , 0.0f}, colors[c]});
			rdBuffer->SafeAppend({{startx     , starty + ys, 0.0f}, colors[c]});
			rdBuffer->SafeAppend({{startx + xs, starty + ys, 0.0f}, colors[c]});

			rdBuffer->SafeAppend({{startx + xs, starty + ys, 0.0f}, colors[c]});
			rdBuffer->SafeAppend({{startx + xs, starty     , 0.0f}, colors[c]});
			rdBuffer->SafeAppend({{startx     , starty     , 0.0f}, colors[c]});
		}
	}
}


/******************************************************************************/
/******************************************************************************/

static void DrawUnitDefRanges(const CUnit* unit, const UnitDef* unitDef, GL::WideLineAdapterC* wla, const float3& pos)
{
	const auto DrawCircleIf = [&](const float4& center, const float* color) {
		if (center.w <= 0.0f)
			return false;

		glSurfaceCircleW(wla, center, color, 40);
		return true;
	};

	const WeaponDef* shieldWD = unitDef->shieldWeaponDef;
	const WeaponDef*  exploWD = unitDef->selfdExpWeaponDef;

	// draw build range for immobile builders
	if (unitDef->builder)
		DrawCircleIf({pos, unitDef->buildDistance}, cmdColors.rangeBuild);

	// draw shield range for immobile units
	if (shieldWD != nullptr)
		glSurfaceCircleW(wla, {pos, shieldWD->shieldRadius}, cmdColors.rangeShield, 40);

	// draw sensor and jammer ranges
	if (unitDef->onoffable || unitDef->activateWhenBuilt) {
		if (unit != nullptr && unitDef == unit->unitDef) { // test if it's a decoy
			DrawCircleIf({pos, unit->radarRadius    * 1.0f}, cmdColors.rangeRadar      );
			DrawCircleIf({pos, unit->sonarRadius    * 1.0f}, cmdColors.rangeSonar      );
			DrawCircleIf({pos, unit->seismicRadius  * 1.0f}, cmdColors.rangeSeismic    );
			DrawCircleIf({pos, unit->jammerRadius   * 1.0f}, cmdColors.rangeJammer     );
			DrawCircleIf({pos, unit->sonarJamRadius * 1.0f}, cmdColors.rangeSonarJammer);
		} else {
			DrawCircleIf({pos, unitDef->radarRadius    * 1.0f}, cmdColors.rangeRadar      );
			DrawCircleIf({pos, unitDef->sonarRadius    * 1.0f}, cmdColors.rangeSonar      );
			DrawCircleIf({pos, unitDef->seismicRadius  * 1.0f}, cmdColors.rangeSeismic    );
			DrawCircleIf({pos, unitDef->jammerRadius   * 1.0f}, cmdColors.rangeJammer     );
			DrawCircleIf({pos, unitDef->sonarJamRadius * 1.0f}, cmdColors.rangeSonarJammer);
		}
	}

	// draw self-destruct and damage distance
	if (!DrawCircleIf({pos, unitDef->kamikazeDist}, cmdColors.rangeKamikaze))
		return;

	if (exploWD == nullptr)
		return;

	glSurfaceCircleW(wla, {pos, exploWD->damages.damageAreaOfEffect}, cmdColors.rangeSelfDestruct, 40);
}




static void DrawWeaponAngleCone(
	GL::RenderDataBufferC* rdb,
	Shader::IProgramObject* ipo,
	const float3& sourcePos,
	const float2& rangleAngle,
	const float2& headingPitch
) {
	CMatrix44f mat;

	const float  xlen = rangleAngle.x * std::cos(rangleAngle.y);
	const float yzlen = rangleAngle.x * std::sin(rangleAngle.y);

	mat.Translate(sourcePos);
	mat.RotateY(-headingPitch.x);
	mat.RotateZ(-headingPitch.y);
	mat.Scale({xlen, yzlen, yzlen});

	ipo->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix() * mat);

	glDrawCone(rdb, GL_FRONT, 64, {1.0f, 0.0f, 0.0f, 0.25f}); // back-faces (inside) in red
	glDrawCone(rdb, GL_BACK , 64, {0.0f, 1.0f, 0.0f, 0.25f}); // front-faces (outside) in green
}

static inline void DrawUnitWeaponAngleCones(const CUnit* unit, GL::RenderDataBufferC* rdb, Shader::IProgramObject* ipo)
{
	for (const CWeapon* w: unit->weapons) {
		// attack order needs to have been issued or wantedDir is undefined
		if (!w->HaveTarget())
			continue;
		if (w->weaponDef->projectileType == WEAPON_BASE_PROJECTILE)
			continue;

		const float hrads   = math::acos(w->maxForwardAngleDif);
		const float heading = math::atan2(-w->wantedDir.z, w->wantedDir.x);
		const float pitch   = math::asin(w->wantedDir.y);

		// note: cone visualization is invalid for ballistic weapons
		DrawWeaponAngleCone(rdb, ipo, w->weaponMuzzlePos, {w->range, hrads}, {heading, pitch});
	}
}


static void DrawUnitWeaponRangeRingFunc(const CUnit* unit, GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla, Shader::IProgramObject*)
{
	glBallisticCircleW(wla, unit->weapons[0],  40, GL_LINES,  unit->pos, {unit->maxRange, 0.0f, mapInfo->map.gravity}, cmdColors.rangeAttack);
}
static void DrawUnitWeaponAngleConeFunc(const CUnit* unit, GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla, Shader::IProgramObject* ipo)
{
	// never invoked on minimap
	DrawUnitWeaponAngleCones(unit, rdb, ipo);
}

template<typename DrawFunc> static void DrawRangeRingsAndAngleCones(
	const std::vector<BuildInfo>& biQueue,
	const CUnit* pointeeUnit,
	const UnitDef* buildeeDef,
	const DrawFunc unitDrawFunc,
	GL::RenderDataBufferC* rdb,
	GL::WideLineAdapterC* wla,
	Shader::IProgramObject* ipo,
	bool drawSelecteeShapes, // rings for pass 1, cones for pass 2
	bool drawPointeeRing,
	bool drawBuildeeRings,
	bool drawOnMiniMap
) {
	assert((unitDrawFunc == DrawUnitWeaponRangeRingFunc) || (unitDrawFunc == DrawUnitWeaponAngleConeFunc));
	assert(!drawPointeeRing || pointeeUnit != nullptr);

	drawPointeeRing  &= (unitDrawFunc == DrawUnitWeaponRangeRingFunc);
	drawBuildeeRings &= (unitDrawFunc == DrawUnitWeaponRangeRingFunc);

	if (drawPointeeRing)
		unitDrawFunc(pointeeUnit, rdb, wla, ipo);

	if (drawBuildeeRings) {
		// draw (primary) weapon range for queued turrets
		for (const BuildInfo& bi: biQueue) {
			if (!buildeeDef->HasWeapons())
				continue;

			const UnitDefWeapon& udw = buildeeDef->GetWeapon(0);
			const WeaponDef* wd = udw.def;

			glBallisticCircleW(wla, wd,  40, GL_LINES,  bi.pos, {wd->range, wd->heightmod, mapInfo->map.gravity}, cmdColors.rangeAttack);
		}
	}

	if (drawSelecteeShapes) {
		for (const int unitID: selectedUnitsHandler.selectedUnits) {
			const CUnit* unit = unitHandler.GetUnit(unitID);

			// handled above
			if (unit == pointeeUnit)
				continue;

			if (unit->maxRange <= 0.0f)
				continue;
			if (unit->weapons.empty())
				continue;
			// only consider (armed) static structures for the minimap
			if (drawOnMiniMap && !unit->unitDef->IsImmobileUnit())
				continue;

			if (!gu->spectatingFullView && !unit->IsInLosForAllyTeam(gu->myAllyTeam))
				continue;

			unitDrawFunc(unit, rdb, wla, ipo);
		}
	}

	// unlike rings, cones are not batched for simplicity
	// buffer will just be empty here if drawing the latter
	assert(unitDrawFunc != DrawUnitWeaponAngleConeFunc || wla->NumElems() == 0);
	wla->Submit(GL_LINES);
}




void CGuiHandler::DrawMapStuff(bool onMiniMap)
{
	if (!onMiniMap) {
		glAttribStatePtr->EnableDepthTest();
		glAttribStatePtr->DisableDepthMask();
		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}


	float3 tracePos = camera->GetPos();
	float3 traceDir = mouse->dir;

	const bool haveActiveCommand = (size_t(inCommand) < commands.size());
	const bool activeBuildCommand = (haveActiveCommand && (commands[inCommand].type == CMDTYPE_ICON_BUILDING));

	// setup for minimap proxying
	const bool minimapInput = (activeReceiver != this && !mouse->offscreen && GetReceiverAt(mouse->lastx, mouse->lasty) == minimap);
	const bool minimapCoors = (minimap->ProxyMode() || (!game->hideInterface && minimapInput));

	if (minimapCoors) {
		// if drawing on the minimap, start at the world-coordinate
		// position mapped to by mouse->last{x,y} and trace straight
		// down
		tracePos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
		traceDir = -UpVector;

		if (miniMapMarker && minimap->FullProxy() && !onMiniMap && !minimap->GetMinimized())
			DrawMiniMapMarker(tracePos);
	}


	GL::RenderDataBufferC*  buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();
	GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();

	const CMatrix44f& projMat = onMiniMap? minimap->GetProjMat(0): camera->GetProjectionMatrix();
	const CMatrix44f& viewMat = onMiniMap? minimap->GetViewMat(0): camera->GetViewMatrix();
	const int xScale = onMiniMap? minimap->GetSizeX(): globalRendering->viewSizeX;
	const int yScale = onMiniMap? minimap->GetSizeY(): globalRendering->viewSizeY;

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMat);
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMat);
	// no alpha-test if drawing on world, does not seem essential
	// shader->SetUniform("u_alpha_test_ctrl", 0.5f, 0.0f, 0.0f, 1.0f * onMiniMap);
	wla->Setup(buffer, xScale, yScale, 1.0f, projMat * viewMat, onMiniMap);


	if (activeMousePress) {
		int cmdIndex = -1;
		int button = SDL_BUTTON_LEFT;

		if (haveActiveCommand) {
			cmdIndex = inCommand;
		} else {
			if (mouse->buttons[SDL_BUTTON_RIGHT].pressed &&
			    ((activeReceiver == this) || (minimap->ProxyMode()))) {
				cmdIndex = defaultCmdMemory;
				button = SDL_BUTTON_RIGHT;
			}
		}

		if (mouse->buttons[button].pressed && (cmdIndex >= 0) && ((size_t)cmdIndex < commands.size())) {
			const SCommandDescription& cmdDesc = commands[cmdIndex];

			switch (cmdDesc.type) {
				case CMDTYPE_ICON_FRONT: {
					if (mouse->buttons[button].movement > mouse->dragFrontCommandThreshold) {
						float maxSize = 1000000.0f;
						float sizeDiv = 0.0f;

						if (!cmdDesc.params.empty())
							maxSize = atof(cmdDesc.params[0].c_str());
						if (cmdDesc.params.size() > 1)
							sizeDiv = atof(cmdDesc.params[1].c_str());

						DrawFormationFrontOrder(buffer, wla, shader, tracePos, traceDir, button, maxSize, sizeDiv, onMiniMap);
					}
				} break;

				case CMDTYPE_ICON_UNIT_OR_AREA:
				case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
				case CMDTYPE_ICON_AREA: {
					// draw circular area-command
					float maxRadius = 100000.0f;

					if (cmdDesc.params.size() == 1)
						maxRadius = atof(cmdDesc.params[0].c_str());

					if (mouse->buttons[button].movement > mouse->dragCircleCommandThreshold) {
						const float3 camTracePos = mouse->buttons[button].camPos;
						const float3 camTraceDir = mouse->buttons[button].dir;

						const float traceDist = camera->GetFarPlaneDist() * 1.4f;
						const float innerDist = CGround::LineGroundCol(camTracePos, camTracePos + camTraceDir * traceDist, false);
						      float outerDist = -1.0f;

						if (innerDist < 0.0f)
							break;
						if ((outerDist = CGround::LineGroundCol(tracePos, tracePos + traceDir * traceDist, false)) < 0.0f)
							outerDist = CGround::LinePlaneCol(tracePos, traceDir, traceDist, camTracePos.y + camTraceDir.y * innerDist);

						const float3 innerPos = camTracePos + camTraceDir * innerDist;
						const float3 outerPos = tracePos + traceDir * outerDist;

						const float radius = std::min(maxRadius, innerPos.distance2D(outerPos));
						const float* color = nullptr;

						switch (cmdDesc.id) {
							case CMD_ATTACK:
							case CMD_AREA_ATTACK:  { color = cmdColors.attack;    } break;
							case CMD_REPAIR:       { color = cmdColors.repair;    } break;
							case CMD_RECLAIM:      { color = cmdColors.reclaim;   } break;
							case CMD_RESTORE:      { color = cmdColors.restore;   } break;
							case CMD_RESURRECT:    { color = cmdColors.resurrect; } break;
							case CMD_LOAD_UNITS:   { color = cmdColors.load;      } break;
							case CMD_UNLOAD_UNIT:
							case CMD_UNLOAD_UNITS: { color = cmdColors.unload;    } break;
							case CMD_CAPTURE:      { color = cmdColors.capture;   } break;
							default: {
								const CCommandColors::DrawData* dd = cmdColors.GetCustomCmdData(cmdDesc.id);

								if (dd != nullptr && dd->showArea) {
									color = dd->color;
								} else {
									color = cmdColors.customArea;
								}
							}
						}

						if (!onMiniMap) {
							DrawSelectCircle(buffer, wla, shader, {innerPos, radius}, color);
						} else {
							constexpr int divs = 256;

							for (int i = 0; i <= divs; ++i) {
								const float radians = math::TWOPI * (float)i / (float)divs;

								float3 p;
								p.x = innerPos.x + (fastmath::sin(radians) * radius);
								p.z = innerPos.z + (fastmath::cos(radians) * radius);

								buffer->SafeAppend({p, {color[0], color[1], color[2], 0.5f}});
							}

							assert(shader->IsBound());
							// must waste a submit, code below assumes LINES
							buffer->Submit(GL_TRIANGLE_FAN);
						}
					}
				} break;

				case CMDTYPE_ICON_UNIT_OR_RECTANGLE: {
					// draw rectangular area-command
					if (mouse->buttons[button].movement > mouse->dragBoxCommandThreshold) {
						const float3 camTracePos = mouse->buttons[button].camPos;
						const float3 camTraceDir = mouse->buttons[button].dir;

						const float traceDist = camera->GetFarPlaneDist() * 1.4f;
						const float innerDist = CGround::LineGroundCol(camTracePos, camTracePos + camTraceDir * traceDist, false);
						      float outerDist = -1.0f;

						if (innerDist < 0.0f)
							break;
						if ((outerDist = CGround::LineGroundCol(tracePos, tracePos + traceDir * traceDist, false)) < 0.0f)
							outerDist = CGround::LinePlaneCol(tracePos, traceDir, traceDist, camTracePos.y + camTraceDir.y * innerDist);

						const float3 innerPos = camTracePos + camTraceDir * innerDist;
						const float3 outerPos = tracePos + traceDir * outerDist;

						if (!onMiniMap) {
							DrawSelectBox(buffer, wla, shader, innerPos, outerPos);
						} else {
							buffer->SafeAppend({{innerPos.x, 0.0f, innerPos.z}, {1.0f, 0.0f, 0.0f, 0.5f}});
							buffer->SafeAppend({{outerPos.x, 0.0f, innerPos.z}, {1.0f, 0.0f, 0.0f, 0.5f}});
							buffer->SafeAppend({{outerPos.x, 0.0f, outerPos.z}, {1.0f, 0.0f, 0.0f, 0.5f}});

							buffer->SafeAppend({{outerPos.x, 0.0f, outerPos.z}, {1.0f, 0.0f, 0.0f, 0.5f}});
							buffer->SafeAppend({{innerPos.x, 0.0f, outerPos.z}, {1.0f, 0.0f, 0.0f, 0.5f}});
							buffer->SafeAppend({{innerPos.x, 0.0f, innerPos.z}, {1.0f, 0.0f, 0.0f, 0.5f}});

							assert(shader->IsBound());
							// must waste a submit, code below assumes LINES
							buffer->Submit(GL_TRIANGLES);
						}
					}
				} break;
			}
		}
	}

	if (!onMiniMap) {
		glAttribStatePtr->BlendFunc((GLenum) cmdColors.SelectedBlendSrc(), (GLenum) cmdColors.SelectedBlendDst());
		wla->SetWidth(cmdColors.SelectedLineWidth());
	} else {
		wla->SetWidth(1.49f);
	}



	// draw the ranges for the unit that is being pointed at
	const CUnit* pointeeUnit = nullptr;
	const UnitDef* buildeeDef = nullptr;

	const float maxTraceDist = camera->GetFarPlaneDist() * 1.4f;
	      float rayTraceDist = -1.0f;


	if (GetQueueKeystate()) {
		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;

		if (minimapCoors) {
			unit = minimap->GetSelectUnit(tracePos);
		} else {
			// ignore the returned distance, we don't care about it here
			TraceRay::GuiTraceRay(tracePos, traceDir, maxTraceDist, nullptr, unit, feature, false);
		}

		const bool   validUnit = (unit != nullptr);
		const bool visibleUnit = (validUnit && (gu->spectatingFullView || unit->IsInLosForAllyTeam(gu->myAllyTeam)));
		const bool   enemyUnit = (validUnit && unit->allyteam != gu->myAllyTeam && !gu->spectatingFullView);

		if (visibleUnit) {
			pointeeUnit = unit;

			const UnitDef*  unitDef = pointeeUnit->unitDef;
			const UnitDef* decoyDef = unitDef->decoyDef;

			if (enemyUnit && decoyDef != nullptr)
				unitDef = decoyDef;

			DrawUnitDefRanges(unit, unitDef, wla, pointeeUnit->pos);

			// draw decloak distance
			if (pointeeUnit->decloakDistance > 0.0f) {
				if (pointeeUnit->unitDef->decloakSpherical && globalRendering->drawDebug) {
					CMatrix44f mat;
					mat.Translate(pointeeUnit->midPos);
					mat.RotateX(-90.0f * math::DEG_TO_RAD);
					mat.Scale(OnesVector * pointeeUnit->decloakDistance);

					Shader::IProgramObject* cvShader = shaderHandler->GetExtProgramObject("[DebugColVolDrawer]");

					cvShader->Enable();
					cvShader->SetUniformMatrix4fv(0, false, mat);
					cvShader->SetUniformMatrix4fv(1, false, viewMat);
					cvShader->SetUniformMatrix4fv(2, false, projMat);
					cvShader->SetUniform4fv(3, cmdColors.rangeDecloak);

					// spherical
					gleBindMeshBuffers(&COLVOL_MESH_BUFFERS[0]);
					gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);
					gleBindMeshBuffers(nullptr);

					cvShader->Disable();
					shader->Enable();
				} else {
					// cylindrical
					glSurfaceCircleW(wla, {pointeeUnit->pos, pointeeUnit->decloakDistance}, cmdColors.rangeDecloak, 40);
				}
			}

			// draw interceptor range
			if (unitDef->maxCoverage > 0.0f) {
				const CWeapon* w = enemyUnit? pointeeUnit->stockpileWeapon: nullptr; // will be checked if any missiles are ready

				// if this isn't the interceptor, then don't use it
				if (w != nullptr && !w->weaponDef->interceptor)
					w = nullptr;

				// shows as on if enemy, a non-stockpiled weapon, or if the stockpile has a missile
				const float4 colors[] = {cmdColors.rangeInterceptorOff, cmdColors.rangeInterceptorOn};
				const float4& color = colors[!enemyUnit || (w == nullptr) || w->numStockpiled > 0];

				glSurfaceCircleW(wla, {pointeeUnit->pos, unitDef->maxCoverage}, color, 40);
			}
		}
	}


	if (activeBuildCommand) {
		// draw build distance for all immobile builders during build-commands
		for (const auto bi: unitHandler.GetBuilderCAIs()) {
			const CBuilderCAI* builderCAI = bi.second;
			const CUnit* builder = builderCAI->owner;
			const UnitDef* builderDef = builder->unitDef;

			if (builder == pointeeUnit || builder->team != gu->myTeam)
				continue;
			if (!builderDef->builder)
				continue;

			if (!builderDef->canmove || selectedUnitsHandler.IsUnitSelected(builder)) {
				const float radius = builderDef->buildDistance;
				const float* color = cmdColors.rangeBuild;

				if (radius <= 0.0f)
					continue;

				glSurfaceCircleW(wla, {builder->pos, radius}, {color[0], color[1], color[2], color[3] * 0.333f}, 40);
			}
		}

		buildeeDef = unitDefHandler->GetUnitDefByID(-commands[inCommand].id);
	}


	if (buildeeDef != nullptr) {
		assert(activeBuildCommand);

		if ((rayTraceDist = CGround::LineGroundWaterCol(tracePos, traceDir, maxTraceDist, buildeeDef->floatOnWater, false)) > 0.0f) {
			const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];

			const float bpDist = CGround::LineGroundWaterCol(bp.camPos, bp.dir, maxTraceDist, buildeeDef->floatOnWater, false);

			// get the build-position information
			const float3 cPos = tracePos + traceDir * rayTraceDist;
			const float3 bPos = bp.camPos + bp.dir * bpDist;

			if (GetQueueKeystate() && bp.pressed) {
				const BuildInfo cInfo = BuildInfo(buildeeDef, cPos, buildFacing);
				const BuildInfo bInfo = BuildInfo(buildeeDef, bPos, buildFacing);

				buildColors.clear();
				buildColors.reserve(GetBuildPositions(bInfo, cInfo, tracePos, traceDir));
			} else {
				const BuildInfo bi(buildeeDef, cPos, buildFacing);

				buildColors.clear();
				buildColors.reserve(GetBuildPositions(bi, bi, tracePos, traceDir));
			}


			for (const BuildInfo& bi: buildInfoQueue) {
				DrawUnitDefRanges(nullptr, buildeeDef, wla, bi.pos);

				// draw extraction range
				if (buildeeDef->extractRange > 0.0f)
					glSurfaceCircleW(wla, {bi.pos, buildeeDef->extractRange}, cmdColors.rangeExtract, 40);

				// draw interceptor range
				const WeaponDef* wd = buildeeDef->stockpileWeaponDef;

				if (wd == nullptr || !wd->interceptor)
					continue;

				glSurfaceCircleW(wla, {bi.pos, wd->coverageRange}, cmdColors.rangeInterceptorOn, 40);
			}
		}
	}


	if (wla->NumElems() > 0) {
		assert(shader->IsBound());
		wla->Submit(GL_LINES);
	}

	if (rayTraceDist > 0.0f) {
		assert(activeBuildCommand);
		assert(buildeeDef != nullptr);

		if (!buildInfoQueue.empty()) {
			unitDrawer->SetupShowUnitBuildSquares(onMiniMap, true);

			// first pass; grid squares
			for (const BuildInfo& bi: buildInfoQueue) {
				buildCommands.clear();

				if (GetQueueKeystate()) {
					const Command c = bi.CreateCommand();

					for (const int unitID: selectedUnitsHandler.selectedUnits) {
						const CUnit* su = unitHandler.GetUnit(unitID);
						const CCommandAI* cai = su->commandAI;

						for (const Command& cmd: cai->GetOverlapQueued(c)) {
							buildCommands.push_back(cmd);
						}
					}
				}

				if (unitDrawer->ShowUnitBuildSquares(bi, buildCommands, true)) {
					buildColors.emplace_back(0.7f, 1.0f, 1.0f, 0.4f); // yellow
				} else {
					buildColors.emplace_back(1.0f, 0.5f, 0.5f, 0.4f); // red
				}
			}

			unitDrawer->ResetShowUnitBuildSquares(onMiniMap, true);
			unitDrawer->SetupShowUnitBuildSquares(onMiniMap, false);

			// second pass; water-depth indicator lines
			for (const BuildInfo& bi: buildInfoQueue) {
				unitDrawer->ShowUnitBuildSquares(bi, {}, false);
			}

			unitDrawer->ResetShowUnitBuildSquares(onMiniMap, false);
		}

		if (!onMiniMap && !buildInfoQueue.empty()) {
			// render a pending line/area-build queue
			assert((buildInfoQueue.front()).def == (buildInfoQueue.back()).def);
			assert(buildInfoQueue.size() == buildColors.size());

			const auto setupStateFunc = [&](const BuildInfo&, const S3DModel* mdl) {
				unitDrawer->SetupAlphaDrawing(false, true);
				unitDrawer->PushModelRenderState(mdl);
				// TODO: tint model by buildColors[&bi - &buildInfoQueue[0]]
				unitDrawer->SetTeamColour(gu->myTeam, float2(0.25f, 1.0f));
				return (unitDrawer->GetDrawerState(DRAWER_STATE_SEL));
			};
			const auto resetStateFunc = [&](const BuildInfo&, const S3DModel* mdl) {
				unitDrawer->PopModelRenderState(mdl);
				unitDrawer->ResetAlphaDrawing(false);
			};
			const auto nextModelFunc = [&](const BuildInfo& bi, const S3DModel* mdl) -> const S3DModel* {
				return ((mdl == nullptr)? bi.def->LoadModel(): mdl);
			};
			const auto drawModelFunc = [&](const BuildInfo& bi, const S3DModel* mdl, const IUnitDrawerState* uds) {
				unitDrawer->DrawStaticModelRaw(mdl, uds, bi.pos, bi.buildFacing);
			};

			unitDrawer->DrawStaticModelBatch<BuildInfo>(buildInfoQueue, setupStateFunc, resetStateFunc, nextModelFunc, drawModelFunc);
			shader->Enable(); // restore

			glAttribStatePtr->BlendFunc((GLenum)cmdColors.SelectedBlendSrc(), (GLenum)cmdColors.SelectedBlendDst());
		}
	}

	{
		// draw range circles (for immobile units) if attack orders are imminent
		const int defaultCmd = GetDefaultCommand(mouse->lastx, mouse->lasty, tracePos, traceDir);

		const bool  activeAttackCmd  = (haveActiveCommand && commands[inCommand].id == CMD_ATTACK);
		const bool defaultAttackCmd  = (inCommand == -1 && defaultCmd > 0 && commands[defaultCmd].id == CMD_ATTACK);
		const bool  drawPointeeRing  = (pointeeUnit != nullptr && pointeeUnit->maxRange > 0.0f);
		const bool  drawBuildeeRings = (activeBuildCommand && rayTraceDist > 0.0f);
		const bool   drawWeaponCones = (!onMiniMap && gs->cheatEnabled && globalRendering->drawDebug);


		if (activeAttackCmd || defaultAttackCmd || drawPointeeRing || drawBuildeeRings) {
			static constexpr decltype(&glSetupRangeRingDrawState  ) setupDrawStateFuncs[] = {&glSetupRangeRingDrawState, &glSetupWeaponArcDrawState};
			static constexpr decltype(&glResetRangeRingDrawState  ) resetDrawStateFuncs[] = {&glResetRangeRingDrawState, &glResetWeaponArcDrawState};
			static constexpr decltype(&DrawUnitWeaponRangeRingFunc)       unitDrawFuncs[] = {&DrawUnitWeaponRangeRingFunc, &DrawUnitWeaponAngleConeFunc};

			assert(shader->IsBound());

			for (int i = 0; i < (1 + drawWeaponCones); i++) {
				setupDrawStateFuncs[i]();
				DrawRangeRingsAndAngleCones(buildInfoQueue, pointeeUnit, buildeeDef, unitDrawFuncs[i], buffer, wla, shader, activeAttackCmd || defaultAttackCmd, drawPointeeRing, drawBuildeeRings, onMiniMap);
				resetDrawStateFuncs[i]();
			}
		}
	}


	if (!onMiniMap) {
		glAttribStatePtr->EnableDepthMask();
		glAttribStatePtr->DisableBlendMask();
	}

	shader->Disable();
}


void CGuiHandler::DrawMiniMapMarker(const float3& cameraPos)
{
	constexpr float w = 10.0f;
	constexpr float h = 30.0f;

	const static float4 baseColor = {0.1f, 0.333f, 1.0f, 0.75f};
	const static float4 vertColors[] = {
		{float3(baseColor) * 0.4f, baseColor.w},
		{float3(baseColor) * 0.8f, baseColor.w},
		{float3(baseColor) * 0.6f, baseColor.w},
		{float3(baseColor) * 1.0f, baseColor.w},

		{float3(baseColor) * 0.3f, baseColor.w},
		{float3(baseColor) * 0.7f, baseColor.w},
		{float3(baseColor) * 0.5f, baseColor.w},
		{float3(baseColor) * 0.9f, baseColor.w},
	};

	const static VA_TYPE_C markerVerts[] = {
		{{0.0f,   0.0f, 0.0f}, {&baseColor.x}}, // bottom
		{{0.0f,  h + h, 0.0f}, {&baseColor.x}}, // top

		{{  +w,     +h, 0.0f}, {&vertColors[0].x}},
		{{0.0f,     +h,   +w}, {&vertColors[1].x}},
		{{  -w,     +h, 0.0f}, {&vertColors[2].x}},
		{{0.0f,     +h,   -w}, {&vertColors[3].x}},

		{{  +w,     +h, 0.0f}, {&vertColors[4].x}},
		{{0.0f,     +h,   +w}, {&vertColors[5].x}},
		{{  -w,     +h, 0.0f}, {&vertColors[6].x}},
		{{0.0f,     +h,   -w}, {&vertColors[7].x}},
	};
	constexpr static uint32_t markerIndcs[] = {
		0, 2 + 0, 3 + 0,   0, 3 + 0, 4 + 0,   0, 4 + 0, 5 + 0,   0, 5 + 0, 2 + 0,
		1, 2 + 4, 3 + 4,   1, 3 + 4, 4 + 4,   1, 4 + 4, 5 + 4,   1, 5 + 4, 2 + 4,
	};


	static float spinTime = 0.0f;

	spinTime += (globalRendering->lastFrameTime * 0.001f);
	spinTime = math::fmod(spinTime, 60.0f);


	GL::RenderDataBufferC* buffer = GL::GetRenderBufferFC(); // flat shading
	Shader::IProgramObject* shader = buffer->GetShader();

	CMatrix44f markerMat;
	markerMat.Translate(cameraPos.x, CGround::GetHeightAboveWater(cameraPos, false), cameraPos.z);
	markerMat.RotateY(-360.0f * (spinTime * 0.5f) * math::DEG_TO_RAD);


	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix() * markerMat);
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());

	buffer->SafeAppend(markerVerts, sizeof(markerVerts) / sizeof(markerVerts[0]));
	buffer->SafeAppend(markerIndcs, sizeof(markerIndcs) / sizeof(markerIndcs[0]));
	buffer->SubmitIndexed(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void CGuiHandler::DrawCentroidCursor()
{
	int cmd = -1;
	if (size_t(inCommand) < commands.size()) {
		cmd = commands[inCommand].id;
	} else {
		size_t defcmd = 0;

		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed && ((activeReceiver == this) || (minimap->ProxyMode()))) {
			defcmd = defaultCmdMemory;
		} else {
			defcmd = GetDefaultCommand(mouse->lastx, mouse->lasty);
		}

		if (defcmd < commands.size())
			cmd = commands[defcmd].id;
	}

	if ((cmd == CMD_MOVE) || (cmd == CMD_GATHERWAIT)) {
		if (!KeyInput::GetKeyModState(KMOD_CTRL) && !KeyInput::GetKeyModState(KMOD_ALT) && !KeyInput::GetKeyModState(KMOD_GUI)) {
			return;
		}
	} else if ((cmd == CMD_FIGHT) || (cmd == CMD_PATROL)) {
		if (!KeyInput::GetKeyModState(KMOD_CTRL)) {
			return;
		}
	} else {
		return;
	}

	const auto& selUnits = selectedUnitsHandler.selectedUnits;
	if (selUnits.size() < 2)
		return;

	float3 sumPos;

	for (const int unitID: selUnits) {
		sumPos += (unitHandler.GetUnit(unitID))->midPos;
	}

	const float3 winPos = camera->CalcWindowCoordinates(sumPos / selUnits.size());

	if (winPos.z > 1.0f)
		return;

	const CMouseCursor* mc = mouse->FindCursor("Centroid");
	if (mc == nullptr)
		return;

	glAttribStatePtr->DisableDepthTest();
	mc->Draw((int)winPos.x, globalRendering->viewSizeY - (int)winPos.y, 1.0f);
	glAttribStatePtr->EnableDepthTest();
}


void CGuiHandler::DrawFormationFrontOrder(
	GL::RenderDataBufferC* buffer,
	GL::WideLineAdapterC* wla,
	Shader::IProgramObject* shader,
	const float3& cameraPos,
	const float3& mouseDir,
	int button,
	float maxSize,
	float sizeDiv,
	bool onMiniMap
) {
	const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];

	const float buttonDist = CGround::LineGroundCol(bp.camPos, bp.camPos + bp.dir * camera->GetFarPlaneDist() * 1.4f, false);

	if (buttonDist < 0.0f)
		return;

	const float cameraDist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * camera->GetFarPlaneDist() * 1.4f, false);

	if (cameraDist < 0.0f)
		return;

	float3 pos1 = bp.camPos + (  bp.dir * buttonDist);
	float3 pos2 = cameraPos + (mouseDir * cameraDist);

	ProcessFrontPositions(pos1, pos2);


	const float3 zdir = ((pos1 - pos2).cross(UpVector)).ANormalize();
	const float3 xdir = zdir.cross(UpVector);

	if (pos1.SqDistance2D(pos2) > maxSize * maxSize) {
		pos2 = pos1 + xdir * maxSize;
		pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false);
	}


	constexpr float4 color = {0.5f, 1.0f, 0.5f, 0.5f};

	assert(shader->IsBound());

	if (onMiniMap) {
		pos1 += (pos1 - pos2);
		wla->SetWidth(2.0f);
		wla->SafeAppend({pos1, {&color.x}});
		wla->SafeAppend({pos2, {&color.x}});
		wla->Submit(GL_LINES);
		wla->SetWidth(1.0f);
		return;
	}

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	{
		// direction arrow
		{
			buffer->SafeAppend({pos1 + xdir * 25.0f                , {&color.x}});
			buffer->SafeAppend({pos1 - xdir * 25.0f                , {&color.x}});
			buffer->SafeAppend({pos1 - xdir * 25.0f + zdir *  50.0f, {&color.x}});

			buffer->SafeAppend({pos1 - xdir * 25.0f + zdir *  50.0f, {&color.x}});
			buffer->SafeAppend({pos1 + xdir * 25.0f + zdir *  50.0f, {&color.x}});
			buffer->SafeAppend({pos1 + xdir * 25.0f                , {&color.x}});
		}
		{
			buffer->SafeAppend({pos1 + xdir * 40.0f + zdir *  50.0f, {&color.x}});
			buffer->SafeAppend({pos1 - xdir * 40.0f + zdir *  50.0f, {&color.x}});
			buffer->SafeAppend({pos1 +                zdir * 100.0f, {&color.x}});

			buffer->SafeAppend({pos1 +                zdir * 100.0f, {&color.x}});
			buffer->SafeAppend({pos1 +                zdir * 100.0f, {&color.x}});
			buffer->SafeAppend({pos1 + xdir * 40.0f + zdir *  50.0f, {&color.x}});
		}

		glAttribStatePtr->DisableDepthTest();
		buffer->Submit(GL_TRIANGLES);
		glAttribStatePtr->EnableDepthTest();
	}

	pos1 += (pos1 - pos2);

	const float frontLen = (pos1 - pos2).Length2D();

	const int maxSteps = 256;
	const int steps = std::min(maxSteps, std::max(1, int(frontLen / 16.0f)));

	{
		// vertical quad
		const float3 delta = (pos2 - pos1) / (float)steps;

		for (int i = 0; i <= steps; i++) {
			float3 p;

			p.x = pos1.x + (i * delta.x);
			p.z = pos1.z + (i * delta.z);
			p.y = CGround::GetHeightAboveWater(p.x, p.z, false);

			buffer->SafeAppend({p - UpVector * 100.0f, {&color.x}});
			buffer->SafeAppend({p + UpVector * 100.0f, {&color.x}});
		}

		buffer->Submit(GL_TRIANGLE_STRIP);
	}
}








struct BoxData {
	float3 mins;
	float3 maxs;
	SColor color;

	GL::RenderDataBufferC* buffer;
	Shader::IProgramObject* shader;
};

struct CylinderData {
	int divs;
	float2 center; // xc, zc
	float3 params; // yp, yn, radius;
	SColor color;

	GL::RenderDataBufferC* buffer;
	Shader::IProgramObject* shader;
};


static void DrawCylinderShape(const void* data)
{
	const CylinderData* cylData = static_cast<const CylinderData*>(data);

	const float2& center = cylData->center;
	const float3& params = cylData->params;
	const SColor&  color = cylData->color;

	const float step = math::TWOPI / cylData->divs;

	{
		assert(cylData->shader->IsBound());

		// sides
		for (int i = 0; i <= cylData->divs; i++) {
			const float radians = step * (i % cylData->divs);
			const float x = center.x + (params.z * fastmath::sin(radians));
			const float z = center.y + (params.z * fastmath::cos(radians));

			cylData->buffer->SafeAppend({{x, params.x, z}, color});
			cylData->buffer->SafeAppend({{x, params.y, z}, color});
		}

		cylData->buffer->Submit(GL_TRIANGLE_STRIP);
	}
	{
		// top
		for (int i = 0; i < cylData->divs; i++) {
			const float radians = step * i;
			const float x = center.x + (params.z * fastmath::sin(radians));
			const float z = center.y + (params.z * fastmath::cos(radians));

			cylData->buffer->SafeAppend({{x, params.x, z}, color});
		}

		cylData->buffer->Submit(GL_TRIANGLE_FAN);
	}
	{
		// bottom
		for (int i = (cylData->divs - 1); i >= 0; i--) {
			const float radians = step * i;
			const float x = center.x + (params.z * fastmath::sin(radians));
			const float z = center.y + (params.z * fastmath::cos(radians));

			cylData->buffer->SafeAppend({{x, params.y, z}, color});
		}

		cylData->buffer->Submit(GL_TRIANGLE_FAN);
		// leave enabled for DrawSelectCircle center-line
		// cylData->shader->Disable();
	}
}

static void DrawBoxShape(const void* data)
{
	const BoxData* boxData = static_cast<const BoxData*>(data);

	const float3&  mins = boxData->mins;
	const float3&  maxs = boxData->maxs;
	const SColor& color = boxData->color;

	assert(boxData->shader->IsBound());

	{
		// top
		boxData->buffer->SafeAppend({{mins.x, maxs.y, mins.z}, color});
		boxData->buffer->SafeAppend({{mins.x, maxs.y, maxs.z}, color});
		boxData->buffer->SafeAppend({{maxs.x, maxs.y, maxs.z}, color});

		boxData->buffer->SafeAppend({{maxs.x, maxs.y, maxs.z}, color});
		boxData->buffer->SafeAppend({{maxs.x, maxs.y, mins.z}, color});
		boxData->buffer->SafeAppend({{mins.x, maxs.y, mins.z}, color});
	}
	{
		// bottom
		boxData->buffer->SafeAppend({{mins.x, mins.y, mins.z}, color});
		boxData->buffer->SafeAppend({{maxs.x, mins.y, mins.z}, color});
		boxData->buffer->SafeAppend({{maxs.x, mins.y, maxs.z}, color});

		boxData->buffer->SafeAppend({{maxs.x, mins.y, maxs.z}, color});
		boxData->buffer->SafeAppend({{mins.x, mins.y, maxs.z}, color});
		boxData->buffer->SafeAppend({{mins.x, mins.y, mins.z}, color});
	}
	boxData->buffer->Submit(GL_TRIANGLES);

	{
		// sides
		boxData->buffer->SafeAppend({{mins.x, maxs.y, mins.z}, color});
		boxData->buffer->SafeAppend({{mins.x, mins.y, mins.z}, color});

		boxData->buffer->SafeAppend({{mins.x, maxs.y, maxs.z}, color});
		boxData->buffer->SafeAppend({{mins.x, mins.y, maxs.z}, color});

		boxData->buffer->SafeAppend({{maxs.x, maxs.y, maxs.z}, color});
		boxData->buffer->SafeAppend({{maxs.x, mins.y, maxs.z}, color});

		boxData->buffer->SafeAppend({{maxs.x, maxs.y, mins.z}, color});
		boxData->buffer->SafeAppend({{maxs.x, mins.y, mins.z}, color});

		boxData->buffer->SafeAppend({{mins.x, maxs.y, mins.z}, color});
		boxData->buffer->SafeAppend({{mins.x, mins.y, mins.z}, color});
	}

	boxData->buffer->Submit(GL_TRIANGLE_STRIP);
	// leave enabled for DrawSelectBox corner-posts
	// boxData->shader->Disable();
}


void CGuiHandler::DrawSelectCircle(GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla, Shader::IProgramObject* ipo, const float4& pos, const float* color)
{
	CylinderData cylData;
	cylData.center.x = pos.x;
	cylData.center.y = pos.z;
	cylData.params.x = readMap->GetCurrMaxHeight() + 10000.0f;
	cylData.params.y = readMap->GetCurrMinHeight() -   250.0f;
	cylData.params.z = pos.w; // radius
	cylData.divs = 128;
	cylData.color = {color[0], color[1], color[2], 0.25f};
	cylData.buffer = rdb;
	cylData.shader = ipo;

	{
		assert(ipo->IsBound());

		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);

		glDrawVolume(DrawCylinderShape, &cylData);
	}
	{
		assert(ipo->IsBound());

		// draw the center line
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		wla->SetWidth(2.0f);

		const float3 base(pos.x, CGround::GetHeightAboveWater(pos.x, pos.z, false), pos.z);

		wla->SafeAppend({base                    , {color[0], color[1], color[2], 0.9f}});
		wla->SafeAppend({base + UpVector * 128.0f, {color[0], color[1], color[2], 0.9f}});
		wla->Submit(GL_LINES);

		wla->SetWidth(1.0f);
	}
}

void CGuiHandler::DrawSelectBox(GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla, Shader::IProgramObject* ipo, const float3& pos0, const float3& pos1)
{
	constexpr float4 colors[] = {{1.0f, 0.0f, 0.0f, 0.25f}, {0.0f, 0.0f, 0.0f, 0.0f}};

	BoxData boxData;
	boxData.mins = float3(std::min(pos0.x, pos1.x), readMap->GetCurrMinHeight() -   250.0f, std::min(pos0.z, pos1.z));
	boxData.maxs = float3(std::max(pos0.x, pos1.x), readMap->GetCurrMaxHeight() + 10000.0f, std::max(pos0.z, pos1.z));
	boxData.color = &colors[invColorSelect].x;
	boxData.buffer = rdb;
	boxData.shader = ipo;

	assert(ipo->IsBound());
	glAttribStatePtr->EnableBlendMask();

	if (!invColorSelect) {
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDrawVolume(DrawBoxShape, &boxData);
	} else {
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_INVERT);
		glDrawVolume(DrawBoxShape, &boxData);
		glDisable(GL_COLOR_LOGIC_OP);
	}

	{
		// draw corner posts
		const float3 corner0(pos0.x, CGround::GetHeightAboveWater(pos0.x, pos0.z, false), pos0.z);
		const float3 corner1(pos1.x, CGround::GetHeightAboveWater(pos1.x, pos1.z, false), pos1.z);
		const float3 corner2(pos0.x, CGround::GetHeightAboveWater(pos0.x, pos1.z, false), pos1.z);
		const float3 corner3(pos1.x, CGround::GetHeightAboveWater(pos1.x, pos0.z, false), pos0.z);

		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		wla->SetWidth(2.0f);

		assert(ipo->IsBound());

		wla->SafeAppend({corner0                    , {1.0f, 1.0f, 0.0f, 0.9f}});
		wla->SafeAppend({corner0 + UpVector * 128.0f, {1.0f, 1.0f, 0.0f, 0.9f}});
		wla->SafeAppend({corner1                    , {0.0f, 1.0f, 0.0f, 0.9f}});
		wla->SafeAppend({corner1 + UpVector * 128.0f, {0.0f, 1.0f, 0.0f, 0.9f}});
		wla->SafeAppend({corner2                    , {0.0f, 0.0f, 1.0f, 0.9f}});
		wla->SafeAppend({corner2 + UpVector * 128.0f, {0.0f, 0.0f, 1.0f, 0.9f}});
		wla->SafeAppend({corner3                    , {0.0f, 0.0f, 1.0f, 0.9f}});
		wla->SafeAppend({corner3 + UpVector * 128.0f, {0.0f, 0.0f, 1.0f, 0.9f}});
		wla->Submit(GL_LINES);
		// leave enabled for caller (DrawMap)
		// ipo->Disable();

		wla->SetWidth(1.0f);
	}
}

