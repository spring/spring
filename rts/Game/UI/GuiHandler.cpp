/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GuiHandler.h"


#include "CommandColors.h"
#include "KeyBindings.h"
#include "KeyCodes.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/TraceRay.h"
#include "Lua/LuaConfig.h"
#include "Lua/LuaTextures.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUI.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/IconHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/Input/KeyInput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"

#include <map>
#include <set>
#include <list>

#include <SDL_keycode.h>
#include <SDL_mouse.h>

CONFIG(bool, MiniMapMarker).defaultValue(true).headlessValue(false);
CONFIG(bool, InvertQueueKey).defaultValue(false);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CGuiHandler* guihandler = NULL;


CGuiHandler::CGuiHandler():
	inCommand(-1),
	buildFacing(FACING_SOUTH),
	buildSpacing(0),
	needShift(false),
	showingMetal(false),
	activeMousePress(false),
	forceLayoutUpdate(false),
	maxPage(0),
	activePage(0),
	defaultCmdMemory(-1),
	explicitCommand(-1),
	curIconCommand(-1),
	actionOffset(0),
	drawSelectionInfo(true),
	gatherMode(false)
{
	icons.resize(16);
	iconsSize = icons.size();
	iconsCount = 0;

	LoadDefaults();
	LoadDefaultConfig();

	miniMapMarker = configHandler->GetBool("MiniMapMarker");
	invertQueueKey = configHandler->GetBool("InvertQueueKey");

	autoShowMetal = mapInfo->gui.autoShowMetal;

	useStencil = false;
	if (GLEW_NV_depth_clamp) {
		GLint stencilBits;
		glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
		useStencil = (stencilBits >= 1);
	}

	failedSound = sound->GetSoundId("FailedCommand");
}


CGuiHandler::~CGuiHandler()
{
	for (auto& p: textureMap) {
		glDeleteTextures(1, &p.second);
	}
}


bool CGuiHandler::GetQueueKeystate() const
{
	return (!invertQueueKey && KeyInput::GetKeyModState(KMOD_SHIFT)) ||
	       (invertQueueKey && !KeyInput::GetKeyModState(KMOD_SHIFT));
}


void CGuiHandler::LoadDefaults()
{
	xIcons = 2;
	yIcons = 8;

	xPos        = 0.000f;
	yPos        = 0.175f;
	xIconSize   = 0.060f;
	yIconSize   = 0.060f;
	textBorder  = 0.003f;
	iconBorder  = 0.003f;
	frameBorder = 0.003f;

	xSelectionPos = 0.018f;
	ySelectionPos = 0.127f;

	frameAlpha = -1.0f;
	textureAlpha = 0.8f;

	dropShadows = true;
	useOptionLEDs = true;

	selectGaps = true;
	selectThrough = false;

	menuName = "";

	outlineFonts = false;

	attackRect = false;
	newAttackMode = true;
	invColorSelect = true;
	frontByEnds = false;
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

bool CGuiHandler::DisableLuaUI()
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
		LayoutIcons(false);
		return true;
	}

	LOG_L(L_WARNING, "[GUIHandler] disabling LuaUI failed");
	return false;
}



bool CGuiHandler::LoadConfig(const std::string& cfg)
{
	CSimpleParser parser(cfg);

	std::string deadStr = "";
	std::string prevStr = "";
	std::string nextStr = "";
	std::string fillOrderStr = "";

	while (true) {
		const std::string line = parser.GetCleanLine();

		if (line.empty())
			break;

		const std::vector<std::string> &words = parser.Tokenize(line, 1);

		const std::string command = StringToLower(words[0]);

		if ((command == "dropshadows") && (words.size() > 1)) {
			dropShadows = !!atoi(words[1].c_str());
		}
		else if ((command == "useoptionleds") && (words.size() > 1)) {
			useOptionLEDs = !!atoi(words[1].c_str());
		}
		else if ((command == "selectgaps") && (words.size() > 1)) {
			selectGaps = !!atoi(words[1].c_str());
		}
		else if ((command == "selectthrough") && (words.size() > 1)) {
			selectThrough = !!atoi(words[1].c_str());
		}
		else if ((command == "deadiconslot") && (words.size() > 1)) {
			deadStr = StringToLower(words[1]);
		}
		else if ((command == "prevpageslot") && (words.size() > 1)) {
			prevStr = StringToLower(words[1]);
		}
		else if ((command == "nextpageslot") && (words.size() > 1)) {
			nextStr = StringToLower(words[1]);
		}
		else if ((command == "fillorder") && (words.size() > 1)) {
			fillOrderStr = StringToLower(words[1]);
		}
		else if ((command == "xicons") && (words.size() > 1)) {
			xIcons = atoi(words[1].c_str());
		}
		else if ((command == "yicons") && (words.size() > 1)) {
			yIcons = atoi(words[1].c_str());
		}
		else if ((command == "xiconsize") && (words.size() > 1)) {
			SafeAtoF(xIconSize, words[1]);
		}
		else if ((command == "yiconsize") && (words.size() > 1)) {
			SafeAtoF(yIconSize, words[1]);
		}
		else if ((command == "xpos") && (words.size() > 1)) {
			SafeAtoF(xPos, words[1]);
		}
		else if ((command == "ypos") && (words.size() > 1)) {
			SafeAtoF(yPos, words[1]);
		}
		else if ((command == "textborder") && (words.size() > 1)) {
			SafeAtoF(textBorder, words[1]);
		}
		else if ((command == "iconborder") && (words.size() > 1)) {
			SafeAtoF(iconBorder, words[1]);
		}
		else if ((command == "frameborder") && (words.size() > 1)) {
			SafeAtoF(frameBorder, words[1]);
		}
		else if ((command == "xselectionpos") && (words.size() > 1)) {
			SafeAtoF(xSelectionPos, words[1]);
		}
		else if ((command == "yselectionpos") && (words.size() > 1)) {
			SafeAtoF(ySelectionPos, words[1]);
		}
		else if ((command == "framealpha") && (words.size() > 1)) {
			SafeAtoF(frameAlpha, words[1]);
		}
		else if ((command == "texturealpha") && (words.size() > 1)) {
			SafeAtoF(textureAlpha, words[1]);
		}
		else if ((command == "outlinefont") && (words.size() > 1)) {
			outlineFonts = !!atoi(words[1].c_str());
		}
		else if ((command == "attackrect") && (words.size() > 1)) {
			attackRect = !!atoi(words[1].c_str());
		}
		else if ((command == "newattackmode") && (words.size() > 1)) {
			newAttackMode = !!atoi(words[1].c_str());
		}
		else if ((command == "invcolorselect") && (words.size() > 1)) {
			invColorSelect = !!atoi(words[1].c_str());
		}
		else if ((command == "frontbyends") && (words.size() > 1)) {
			frontByEnds = !!atoi(words[1].c_str());
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

	iconsPerPage = (xIcons * yIcons);

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
	fillOrder.clear();
	fillOrder.reserve(iconsPerPage);

	for (int i = 0; i < iconsPerPage; i++)
		fillOrder.push_back(i);

	// split the std::string into slot names
	const std::vector<std::string> &slotNames = CSimpleParser::Tokenize(text, 0);

	if ((int)slotNames.size() != iconsPerPage)
		return;

	std::set<int>    slotSet;
	std::vector<int> slotVec;

	for (int s = 0; s < iconsPerPage; s++) {
		const int slotNumber = ParseIconSlot(slotNames[s]);
		if ((slotNumber < 0) || (slotNumber >= iconsPerPage) ||
				(slotSet.find(slotNumber) != slotSet.end())) {
			return;
		}
		slotSet.insert(slotNumber);
		slotVec.push_back(slotNumber);
	}

	fillOrder = slotVec;
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


void CGuiHandler::ResizeIconArray(unsigned int size)
{
	unsigned int minIconsSize = iconsSize;

	while (minIconsSize < size)
		minIconsSize *= 2;

	if (iconsSize < minIconsSize) {
		iconsSize = minIconsSize;

		icons.clear();
		icons.resize(iconsSize);
	}
}


void CGuiHandler::AppendPrevAndNext(std::vector<SCommandDescription>& cmds)
{
	SCommandDescription cd;

	cd.id=CMD_INTERNAL;
	cd.action="prevmenu";
	cd.type=CMDTYPE_PREV;
	cd.name="";
	cd.tooltip = "Previous menu";
	cmds.push_back(cd);

	cd.id=CMD_INTERNAL;
	cd.action="nextmenu";
	cd.type=CMDTYPE_NEXT;
	cd.name="";
	cd.tooltip = "Next menu";
	cmds.push_back(cd);
}


int CGuiHandler::FindInCommandPage()
{
	if ((inCommand < 0) || ((size_t)inCommand >= commands.size()))
		return -1;

	for (int ii = 0; ii < iconsCount; ii++) {
		const IconInfo& icon = icons[ii];
		if (icon.commandsID == inCommand) {
			return (ii / iconsPerPage);
		}
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

		if (samePage) {
			for (int ii = 0; ii < iconsCount; ii++) {
				if (inCommand == icons[ii].commandsID) {
					activePage = std::min(maxPage, (ii / iconsPerPage));;
					selectedUnitsHandler.SetCommandPage(activePage);
				}
			}
		}
	}
}


void CGuiHandler::LayoutIcons(bool useSelectionPage)
{
	bool defCmd, validInCommand, samePage;
	SCommandDescription cmdDesc;
	{
		defCmd =
			(mouse->buttons[SDL_BUTTON_RIGHT].pressed &&
			(defaultCmdMemory >= 0) && (inCommand < 0) &&
			((activeReceiver == this) || (minimap->ProxyMode())));

		const int activeCmd = defCmd ? defaultCmdMemory : inCommand;

		// copy the current command state
		validInCommand = (activeCmd >= 0) && ((size_t)activeCmd < commands.size());
		if (validInCommand) {
			cmdDesc = commands[activeCmd];
		}
		samePage = validInCommand && (activePage == FindInCommandPage());
		useSelectionPage = useSelectionPage && !samePage;

		// reset some of our state
		inCommand = -1;
		defaultCmdMemory = -1;
		commands.clear();
		forceLayoutUpdate = false;
	}

	if (luaUI && luaUI->WantsEvent("LayoutButtons")) {
		if (LayoutCustomIcons(useSelectionPage)) {
			if (validInCommand)
				RevertToCmdDesc(cmdDesc, defCmd, samePage);

			return; // we're done here
		} else {
			CLuaUI::FreeHandler();
			LoadDefaultConfig();
		}
	}

	// get the commands to process
	CSelectedUnitsHandler::AvailableCommandsStruct ac;
	ac = selectedUnitsHandler.GetAvailableCommands();
	ConvertCommands(ac.commands);

	std::vector<SCommandDescription> hidden;
	std::vector<SCommandDescription>::const_iterator cdi;

	// separate the visible/hidden icons
	for (cdi = ac.commands.begin(); cdi != ac.commands.end(); ++cdi){
		if (cdi->hidden) {
			hidden.push_back(*cdi);
		} else {
			commands.push_back(*cdi);
		}
	}

	// assign extra icons for internal commands
	int extraIcons = 0;
	if (deadIconSlot >= 0) { extraIcons++; }
	if (prevPageSlot >= 0) { extraIcons++; }
	if (nextPageSlot >= 0) { extraIcons++; }

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
		const int mii = ii - tmpSlot + fillOrder[tmpSlot]; // mapped icon index

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
		}
		else {
			// make sure this icon does not get selected
			icon.selection.x1 = icon.selection.x2 = -1.0f;
			icon.selection.y1 = icon.selection.y2 = -1.0f;
		}
	}

	// append the Prev and Next commands  (if required)
	if (multiPage) {
		AppendPrevAndNext(commands);
	}

	// append the hidden commands
	for (cdi = hidden.begin(); cdi != hidden.end(); ++cdi) {
		commands.push_back(*cdi);
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
	if (luaUI == nullptr)
		return false;

	// get the commands to process
	CSelectedUnitsHandler::AvailableCommandsStruct ac;
	ac = selectedUnitsHandler.GetAvailableCommands();
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
	std::map<int, int> iconMap;

	if (!luaUI->LayoutButtons(tmpXicons, tmpYicons, cmds,
	                          removeCmds, customCmds,
	                          onlyTextureCmds, reTextureCmds,
	                          reNamedCmds, reTooltipCmds, reParamsCmds,
	                          iconMap, menuName)) {
		return false;
	}

	if ((tmpXicons < 2) || (tmpYicons < 2)) {
		LOG_L(L_ERROR, "LayoutCustomIcons() bad xIcons or yIcons (%i, %i)", tmpXicons, tmpYicons);
		return false;
	}

	const int tmpIconsPerPage = (tmpXicons * tmpYicons);

	// build a set to better find unwanted commands
	set<int> removeIDs;
	for (unsigned int i = 0; i < removeCmds.size(); i++) {
		const int index = removeCmds[i];
		if ((index >= 0) || ((size_t)index < cmds.size())) {
			removeIDs.insert(index);
		} else {
			LOG_L(L_ERROR, "LayoutCustomIcons() skipping bad removeCmd (%i)",
					index);
		}
	}
	// remove unwanted commands  (and mark all as hidden)
	std::vector<SCommandDescription> tmpCmds;
	for (unsigned int i = 0; i < cmds.size(); i++) {
		if (removeIDs.find(i) == removeIDs.end()) {
			cmds[i].hidden = true;
			tmpCmds.push_back(cmds[i]);
		}
	}
	cmds = tmpCmds;

	// add the custom commands
	for (unsigned int i = 0; i < customCmds.size(); i++) {
		customCmds[i].hidden = true;
		cmds.push_back(customCmds[i]);
	}
	const int cmdCount = (int)cmds.size();

	// set commands to onlyTexture
	for (unsigned int i = 0; i < onlyTextureCmds.size(); i++) {
		const int index = onlyTextureCmds[i];
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].onlyTexture = true;
		} else {
			LOG_L(L_ERROR, "LayoutCustomIcons() skipping bad onlyTexture (%i)",
					index);
		}
	}

	// retexture commands
	for (unsigned int i = 0; i < reTextureCmds.size(); i++) {
		const int index = reTextureCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].iconname = reTextureCmds[i].texture;
		} else {
			LOG_L(L_ERROR, "LayoutCustomIcons() skipping bad reTexture (%i)",
					index);
		}
	}

	// reNamed commands
	for (unsigned int i = 0; i < reNamedCmds.size(); i++) {
		const int index = reNamedCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].name = reNamedCmds[i].texture;
		} else {
			LOG_L(L_ERROR, "LayoutCustomIcons() skipping bad reNamed (%i)",
					index);
		}
	}

	// reTooltip commands
	for (unsigned int i = 0; i < reTooltipCmds.size(); i++) {
		const int index = reTooltipCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].tooltip = reTooltipCmds[i].texture;
		} else {
			LOG_L(L_ERROR, "LayoutCustomIcons() skipping bad reNamed (%i)",
					index);
		}
	}

	// reParams commands
	for (unsigned int i = 0; i < reParamsCmds.size(); i++) {
		const int index = reParamsCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			const map<int, std::string>& params = reParamsCmds[i].params;

			for (auto pit = params.cbegin(); pit != params.cend(); ++pit) {
				const int p = pit->first;

				if ((p >= 0) && (p < (int)cmds[index].params.size())) {
					cmds[index].params[p] = pit->second;
				}
			}
		} else {
			LOG_L(L_ERROR, "LayoutCustomIcons() skipping bad reParams (%i)",
					index);
		}
	}

	// build the iconList from the map
	std::vector<int> iconList;
	int nextPos = 0;

	for (auto mit = iconMap.cbegin(); mit != iconMap.cend(); ++mit) {
		const int iconPos = mit->first;

		if (iconPos < nextPos)
			continue;

		else if (iconPos > nextPos) {
			// fill in the blanks
			for (int p = nextPos; p < iconPos; p++) {
				iconList.push_back(-1);
			}
		}
		iconList.push_back(mit->second); // cmdIndex
		nextPos = iconPos + 1;
	}

	const int iconListCount = (int)iconList.size();
	const int pageCount = ((iconListCount + (tmpIconsPerPage - 1)) / tmpIconsPerPage);
	const int tmpIconsCount = (pageCount * tmpIconsPerPage);

	// resize the icon array if required
	ResizeIconArray(tmpIconsCount);

	// build the iconList
	for (int ii = 0; ii < tmpIconsCount; ii++) {
		IconInfo& icon = icons[ii];

		const int index = (ii < (int)iconList.size()) ? iconList[ii] : -1;

		if ((index >= 0) && (index < cmdCount)) {
			icon.commandsID = index;
			cmds[index].hidden = false;

			const int slot = (ii % tmpIconsPerPage);
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

	maxPage = std::max(0, pageCount - 1);
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


void CGuiHandler::GiveCommand(Command& cmd, bool fromUser)
{
	commandsToGive.push_back(std::pair<const Command, bool>(cmd, fromUser));
}


void CGuiHandler::GiveCommandsNow() {
	std::vector< std::pair<Command, bool> > commandsToGiveTemp;
	{
		commandsToGiveTemp.swap(commandsToGive);
	}

	for(std::vector< std::pair<Command, bool> >::iterator i = commandsToGiveTemp.begin(); i != commandsToGiveTemp.end(); ++i) {
		const Command& cmd = (*i).first;
		if (eventHandler.CommandNotify(cmd)) {
			continue;
		}

		selectedUnitsHandler.GiveCommand(cmd, (*i).second);

		if (gatherMode) {
			if ((cmd.GetID() == CMD_MOVE) || (cmd.GetID() == CMD_FIGHT)) {
				Command gatherCmd(CMD_GATHERWAIT);
				GiveCommand(gatherCmd, false);
			}
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
		if (showingMetal) {
			infoTextureHandler->DisableCurrentMode();
			showingMetal = false;
		}
	} else {
		if (autoShowMetal) {
			if (infoTextureHandler->GetMode() != "metal") {
				infoTextureHandler->SetMode("metal");
				showingMetal = true;
			}
		}
	}
}


void CGuiHandler::Update()
{
	SetCursorIcon();

	{
		if (!invertQueueKey && (needShift && !KeyInput::GetKeyModState(KMOD_SHIFT))) {
			SetShowingMetal(false);
			inCommand=-1;
			needShift=false;
		}
	}

	GiveCommandsNow();

	const bool commandsChanged = selectedUnitsHandler.CommandsChanged();

	if (commandsChanged) {
		SetShowingMetal(false);
		LayoutIcons(true);
	}
	else if (forceLayoutUpdate) {
		LayoutIcons(false);
	}
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::SetCursorIcon() const
{
	string newCursor = "cursornormal";
	float cursorScale = 1.0f;

	CInputReceiver* ir = NULL;
	if (!game->hideInterface)
		ir = GetReceiverAt(mouse->lastx, mouse->lasty);

	if ((ir != NULL) && (ir != minimap)) {
		mouse->ChangeCursor(newCursor, cursorScale);
		return;
	}

	if (ir == minimap)
		cursorScale = minimap->CursorScale();

	const bool useMinimap = (minimap->ProxyMode() || ((activeReceiver != this) && (ir == minimap)));

	if ((inCommand >= 0) && ((size_t)inCommand<commands.size())) {
		const SCommandDescription& cmdDesc = commands[inCommand];

		if (!cmdDesc.mouseicon.empty()) {
			newCursor=cmdDesc.mouseicon;
		} else {
			newCursor=cmdDesc.name;
		}

		if (useMinimap && (cmdDesc.id < 0)) {
			BuildInfo bi;
			bi.pos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			bi.buildFacing = buildFacing;
			bi.def = unitDefHandler->GetUnitDefByID(-cmdDesc.id);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);
			// if an unit (enemy), is not in LOS, then TestUnitBuildSquare()
			// does not consider it when checking for position blocking
			CFeature* feature = NULL;
			if (!CGameHelper::TestUnitBuildSquare(bi, feature, gu->myAllyTeam, false)) {
				newCursor = "BuildBad";
			} else {
				newCursor = "BuildGood";
			}
		}

		if (!TryTarget(cmdDesc)) {
			newCursor = "AttackBad";
		}
	}
	else if (!useMinimap || minimap->FullProxy()) {
		int defcmd;
		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed &&
				((activeReceiver == this) || (minimap->ProxyMode()))) {
			defcmd = defaultCmdMemory;
		} else {
			defcmd = GetDefaultCommand(mouse->lastx, mouse->lasty);
		}
		if ((defcmd >= 0) && ((size_t)defcmd < commands.size())) {
			const SCommandDescription& cmdDesc = commands[defcmd];
			if (!cmdDesc.mouseicon.empty()) {
				newCursor=cmdDesc.mouseicon;
			} else {
				newCursor=cmdDesc.name;
			}
		}
	}

	if (gatherMode &&
	    ((newCursor == "Move") ||
	    (newCursor == "Fight"))) {
		newCursor = "GatherWait";
	}

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

	const float viewRange = globalRendering->viewRange * 1.4f;
	const float dist = TraceRay::GuiTraceRay(camera->GetPos(), mouse->dir, viewRange, NULL, targetUnit, targetFeature, true);
	const float3 groundPos = camera->GetPos() + mouse->dir * dist;

	if (dist <= 0.0f)
		return false;

	for (const CUnit* u: selectedUnitsHandler.selectedUnits) {
		// mobile kamikaze can always move into range
		//FIXME do a range check in case of immobile kamikaze (-> mines)
		if (u->unitDef->canKamikaze && !u->immobile)
			return true;

		for (const CWeapon* w: u->weapons) {
			float3 modGroundPos = groundPos;
			w->AdjustTargetPosToWater(modGroundPos, targetUnit == NULL);
			const SWeaponTarget wtrg = SWeaponTarget(targetUnit, modGroundPos);

			if (u->immobile) {
				// immobile unit
				// check range and weapon target properties
				if (w->TryTarget(wtrg)) {
					return true;
				}
			} else {
				// mobile units can always move into range
				// only check if we got a weapon that can shot the target (i.e. anti-air/anti-sub)
				if (w->TestTarget(w->GetLeadTargetPos(wtrg), wtrg)) {
					return true;
				}
			}
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
				if (iconPos >= 0) {
					curIconCommand = icons[iconPos].commandsID;
				}
			}
			if (button == SDL_BUTTON_RIGHT)
				inCommand = defaultCmdMemory = -1;
			return true;
		}
		else if (minimap && minimap->IsAbove(x, y)) {
			return false; // let the minimap do its job
		}

		if (inCommand >= 0) {
			if (invertQueueKey && (button == SDL_BUTTON_RIGHT) &&
				!mouse->buttons[SDL_BUTTON_LEFT].pressed) { // for rocker gestures
					SetShowingMetal(false);
					inCommand = -1;
					needShift = false;
					return false;
			}
			activeMousePress = true;
			return true;
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
	curIconCommand = -1;

	int iconCmd = -1;
	explicitCommand = inCommand;

	if (activeMousePress) {
		activeMousePress = false;
	} else {
		return;
	}

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
			if (iconPos >= 0) {
				iconCmd = icons[iconPos].commandsID;
				if(iconCmd != lastIconCmd)
					iconCmd = -1; // mouse was pressed on one button and released on another one --> ignore the command
			}
		}
	}

	if ((button == SDL_BUTTON_RIGHT) && (iconCmd == -1)) {
		// right click -> set the default cmd
		inCommand = defaultCmdMemory;
		defaultCmdMemory = -1;
	}

	if ((iconCmd >= 0) && ((size_t)iconCmd < commands.size())) {
		const bool rightMouseButton = (button == SDL_BUTTON_RIGHT);
		SetActiveCommand(iconCmd, rightMouseButton);
		return;
	}

	// not over a button, try to execute a command
	Command c = GetCommand(x, y, button, false, cameraPos, mouseDir);

	if (c.GetID() == CMD_FAILED) { // indicates we should not finish the current command
		Channels::UserInterface->PlaySample(failedSound, 5);
		return;
	}

	// if cmd_stop is returned it indicates that no good command could be found
	if (c.GetID() != CMD_STOP) {
		GiveCommand(c);

		lastKeySet.Reset();
	}

	FinishCommand(button);
}


bool CGuiHandler::SetActiveCommand(int cmdIndex, bool rightMouseButton)
{
	if (cmdIndex >= (int)commands.size()) {
		return false;
	}
	else if (cmdIndex < 0) {
		// cancel the current command
		inCommand = -1;
		defaultCmdMemory = -1;
		needShift = false;
		activeMousePress = false;
		SetShowingMetal(false);
		return true;
	}

	SCommandDescription& cd = commands[cmdIndex];
	if (cd.disabled) {
		return false;
	}

	lastKeySet.Reset();

	switch (cd.type) {
		case CMDTYPE_ICON: {
			Command c(cd.id);
			if (cd.id != CMD_STOP) {
				c.options = CreateOptions(rightMouseButton);
				if (invertQueueKey && ((cd.id < 0) || (cd.id == CMD_STOCKPILE))) {
					c.options = c.options ^ SHIFT_KEY;
				}
			}
			GiveCommand(c);
			break;
		}
		case CMDTYPE_ICON_MODE: {
			int newMode = atoi(cd.params[0].c_str()) + 1;
			if (newMode > (static_cast<int>(cd.params.size())-2)) {
				newMode = 0;
			}

			// not really required
			char t[10];
			SNPRINTF(t, 10, "%d", newMode);
			cd.params[0] = t;

			Command c(cd.id, CreateOptions(rightMouseButton), newMode);
			GiveCommand(c);
			forceLayoutUpdate = true;
			break;
		}
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
			break;
		}
		case CMDTYPE_ICON_BUILDING: {
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(-cd.id);
			inCommand = cmdIndex;
			SetShowingMetal(ud->extractsMetal > 0);
			activeMousePress = false;
			break;
		}
		case CMDTYPE_NEXT: {
			++activePage;
			if (activePage > maxPage) {
				activePage = 0;
			}
			selectedUnitsHandler.SetCommandPage(activePage);
			break;
		}
		case CMDTYPE_PREV: {
			--activePage;
			if (activePage < 0) {
				activePage=maxPage;
			}
			selectedUnitsHandler.SetCommandPage(activePage);
			break;
		}
		case CMDTYPE_CUSTOM: {
			RunCustomCommands(cd.params, rightMouseButton);
			break;
		}
		default:
			break;
	}

	return true;
}


bool CGuiHandler::SetActiveCommand(int cmdIndex, int button,
                                   bool leftMouseButton, bool rightMouseButton,
                                   bool alt, bool ctrl, bool meta, bool shift)
{
	// use the button value instead of rightMouseButton
	const bool effectiveRMB = (button == SDL_BUTTON_LEFT) ? false : true;

	// setup the mouse and key states
	const bool  prevLMB   = mouse->buttons[SDL_BUTTON_LEFT].pressed;
	const bool  prevRMB   = mouse->buttons[SDL_BUTTON_RIGHT].pressed;
	const boost::uint8_t prevAlt   = KeyInput::GetKeyModState(KMOD_ALT);
	const boost::uint8_t prevCtrl  = KeyInput::GetKeyModState(KMOD_CTRL);
	const boost::uint8_t prevMeta  = KeyInput::GetKeyModState(KMOD_GUI);
	const boost::uint8_t prevShift = KeyInput::GetKeyModState(KMOD_SHIFT);

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

	if ((fx < buttonBox.x1) || (fx > buttonBox.x2) ||
	    (fy < buttonBox.y1) || (fy > buttonBox.y2)) {
		return -1;
	}

	int xSlot = int((fx - (buttonBox.x1 + frameBorder)) / xIconStep);
	int ySlot = int(((buttonBox.y2 - frameBorder) - fy) / yIconStep);
	xSlot = std::min(std::max(xSlot, 0), xIcons - 1);
	ySlot = std::min(std::max(ySlot, 0), yIcons - 1);
	const int ii = (activePage * iconsPerPage) + (ySlot * xIcons) + xSlot;

	if ((ii >= 0) && (ii < iconsCount)) {
		if ((fx > icons[ii].selection.x1) && (fx < icons[ii].selection.x2) &&
				(fy > icons[ii].selection.y2) && (fy < icons[ii].selection.y1)) {
			return ii;
		}
	}

	return -1;
}


/******************************************************************************/

enum ModState {
	DontCare, Required, Forbidden
};

struct ModGroup {
	ModGroup()
	: alt(DontCare),
	  ctrl(DontCare),
	  meta(DontCare),
	  shift(DontCare),
	  right(DontCare) {}
	ModState alt, ctrl, meta, shift, right;
};


static bool ParseCustomCmdMods(std::string& cmd, ModGroup& in, ModGroup& out)
{
	const char* c = cmd.c_str();
	if (*c != '@') {
		return false;
	}
	c++;
	bool neg = false;
	while ((*c != 0) && (*c != '@')) {
		char ch = *c;
		if (ch == '-') {
			neg = true;
		}
		else if (ch == '+') {
			neg = false;
		}
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

	if (*c == 0) {
		return false;
	}
	cmd = cmd.substr((c + 1) - cmd.c_str());

	return true;
}


static bool CheckCustomCmdMods(bool rightMouseButton, ModGroup& inMods)
{
	if (((inMods.alt   == Required)  && !KeyInput::GetKeyModState(KMOD_ALT))   ||
	    ((inMods.alt   == Forbidden) &&  KeyInput::GetKeyModState(KMOD_ALT))   ||
	    ((inMods.ctrl  == Required)  && !KeyInput::GetKeyModState(KMOD_CTRL))  ||
	    ((inMods.ctrl  == Forbidden) &&  KeyInput::GetKeyModState(KMOD_CTRL))  ||
	    ((inMods.meta  == Required)  && !KeyInput::GetKeyModState(KMOD_GUI))  ||
	    ((inMods.meta  == Forbidden) &&  KeyInput::GetKeyModState(KMOD_GUI))  ||
	    ((inMods.shift == Required)  && !KeyInput::GetKeyModState(KMOD_SHIFT)) ||
	    ((inMods.shift == Forbidden) &&  KeyInput::GetKeyModState(KMOD_SHIFT)) ||
	    ((inMods.right == Required)  && !rightMouseButton) ||
	    ((inMods.right == Forbidden) &&  rightMouseButton)) {
		return false;
	}
	return true;
}


void CGuiHandler::RunCustomCommands(const std::vector<std::string>& cmds, bool rightMouseButton)
{
	static int depth = 0;
	if (depth > 8) {
		return; // recursion protection
	}
	depth++;

	for (int p = 0; p < (int)cmds.size(); p++) {
		std::string copy = cmds[p];
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
				if (!ProcessLocalActions(action)) {
					game->ProcessAction(action);
				}

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
	if (iconsCount <= 0) {
		return false;
	}
	if (!selectThrough) {
		const float fx = MouseX(x);
		const float fy = MouseY(y);
		if ((fx > buttonBox.x1) && (fx < buttonBox.x2) &&
				(fy > buttonBox.y1) && (fy < buttonBox.y2)) {
			return true;
		}
	}
	return (IconAtPos(x,y) >= 0);
}


unsigned char CGuiHandler::CreateOptions(bool rightMouseButton)
{
	unsigned char options = 0;

	if (rightMouseButton) {
		options |= RIGHT_MOUSE_KEY;
	}
	if (GetQueueKeystate()) {
		// allow mouse button 'rocker' movements to force
		// immediate mode (when queuing is the default mode)
		if (!invertQueueKey ||
		    (!mouse->buttons[SDL_BUTTON_LEFT].pressed &&
		     !mouse->buttons[SDL_BUTTON_RIGHT].pressed)) {
			options |= SHIFT_KEY;
		}
	}
	if (KeyInput::GetKeyModState(KMOD_CTRL)) { options |= CONTROL_KEY; }
	if (KeyInput::GetKeyModState(KMOD_ALT) ) { options |= ALT_KEY;     }
	if (KeyInput::GetKeyModState(KMOD_GUI))  { options |= META_KEY;    }

	return options;
}
unsigned char CGuiHandler::CreateOptions(int button)
{
	return CreateOptions(button != SDL_BUTTON_LEFT);
}


float CGuiHandler::GetNumberInput(const SCommandDescription& cd) const
{
	float minV = 0.0f;
	float maxV = 100.0f;
	if (!cd.params.empty()) { minV = atof(cd.params[0].c_str()); }
	if (cd.params.size() >= 2) { maxV = atof(cd.params[1].c_str()); }
	const int minX = (globalRendering->viewSizeX * 1) / 4;
	const int maxX = (globalRendering->viewSizeX * 3) / 4;
	const int effX = std::max(std::min(mouse->lastx, maxX), minX);
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

	if (!game->hideInterface)
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
			const float viewRange = globalRendering->viewRange * 1.4f;
			const float dist = TraceRay::GuiTraceRay(cameraPos, mouseDir, viewRange, NULL, unit, feature, true);
			const float3 hit = cameraPos + mouseDir * dist;

			// make sure the ray hit in the map
			if (!unit && !feature && !hit.IsInBounds())
				return -1;
		}

		cmdID = selectedUnitsHandler.GetDefaultCmd(unit, feature);
	}

	// make sure the command is currently available
	for (int c = 0; c < (int)commands.size(); c++) {
		if (cmdID == commands[c].id) {
			return c;
		}
	}
	return -1;
}


bool CGuiHandler::ProcessLocalActions(const Action& action)
{
	// do not process these actions if the control panel is not visible
	if (iconsCount <= 0) {
		return false;
	}

	// only process the build options while building
	// (conserve the keybinding space where we can)
	if ((inCommand >= 0) && ((size_t)inCommand < commands.size()) &&
			((commands[inCommand].type == CMDTYPE_ICON_BUILDING) ||
			(commands[inCommand].id == CMD_UNLOAD_UNITS))) {
		if (ProcessBuildActions(action)) {
			return true;
		}
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
		for(size_t i = 0; i < commands.size(); ++i){
			LOG("  command: " _STPF_ ", id = %i, action = %s",
					i, commands[i].id, commands[i].action.c_str());
		}
		// show the icon/command linkage
		LOG("Icon Linkage:");
		for(int ii = 0; ii < iconsCount; ++ii){
			LOG("  icon: %i, commandsID = %i", ii, icons[ii].commandsID);
		}
		LOG("maxPage         = %i", maxPage);
		LOG("activePage      = %i", activePage);
		LOG("iconsSize       = %u", iconsSize);
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

	if (action.command == "buildspacing") {
		if (arg == "inc") {
			buildSpacing++;
			ret = true;
		}
		else if (arg == "dec") {
			if (buildSpacing > 0) {
				buildSpacing--;
			}
			ret = true;
		}
	}
	else if (action.command == "buildfacing") {
		static const char* buildFaceDirs[] = {"South", "East", "North", "West"};

		if (arg == "inc") {
			buildFacing = (buildFacing +               1) % NUM_FACINGS;
			ret = true;
		}
		else if (arg == "dec") {
			buildFacing = (buildFacing + NUM_FACINGS - 1) % NUM_FACINGS;
			ret = true;
		}
		else if (arg == "south") {
			buildFacing = FACING_SOUTH;
			ret = true;
		}
		else if (arg == "east") {
			buildFacing = FACING_EAST;
			ret = true;
		}
		else if (arg == "north") {
			buildFacing = FACING_NORTH;
			ret = true;
		}
		else if (arg == "west") {
			buildFacing = FACING_WEST;
			ret = true;
		}

		LOG("Buildings set to face %s", buildFaceDirs[buildFacing]);
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
		inCommand=-1;
		SetShowingMetal(false);
		return true;
	}

	const CKeySet ks(key, false);

	// setup actionOffset
	//WTF a bit more documentation???
	int tmpActionOffset = actionOffset;
	if ((inCommand < 0) || (lastKeySet.Key() < 0)){
		actionOffset = 0;
		tmpActionOffset = 0;
		lastKeySet.Reset();
	}
	else if (!ks.IsPureModifier()) {
		// not a modifier
		if ((ks == lastKeySet) && (ks.Key() >= 0)) {
			actionOffset++;
			tmpActionOffset = actionOffset;
		} else {
			tmpActionOffset = 0;
		}
	}

	const CKeyBindings::ActionList& al = keyBindings->GetActionList(ks);
	for (int ali = 0; ali < (int)al.size(); ++ali) {
		const int actionIndex = (ali + tmpActionOffset) % (int)al.size(); //????
		const Action& action = al[actionIndex];
		if (SetActiveCommand(action, ks, actionIndex)) {
			return true;
		}
	}

	return false;
}


bool CGuiHandler::SetActiveCommand(const Action& action,
                                   const CKeySet& ks, int actionIndex)
{
	if (ProcessLocalActions(action)) {
		return true;
	}

	// See if we have a positional icon command
	int iconCmd = -1;
	if (!action.extra.empty() && (action.command == "iconpos")) {
		const int iconSlot = ParseIconSlot(action.extra);
		iconCmd = GetIconPosCommand(iconSlot);
	}

	for (size_t a = 0; a < commands.size(); ++a) {

		SCommandDescription& cmdDesc = commands[a];

		if ((static_cast<int>(a) != iconCmd) && (cmdDesc.action != action.command)) {
			continue; // not a match
		}

		if (cmdDesc.disabled) {
			continue; // can not use this command
		}

		const int cmdType = cmdDesc.type;

		// set the activePage
		if (!cmdDesc.hidden &&
				(((cmdType == CMDTYPE_ICON) &&
					 ((cmdDesc.id < 0) ||
						(cmdDesc.id == CMD_STOCKPILE))) ||
				 (cmdType == CMDTYPE_ICON_MODE) ||
				 (cmdType == CMDTYPE_ICON_BUILDING))) {
			for (int ii = 0; ii < iconsCount; ii++) {
				if (icons[ii].commandsID == static_cast<int>(a)) {
					activePage = std::min(maxPage, (ii / iconsPerPage));
					selectedUnitsHandler.SetCommandPage(activePage);
				}
			}
		}

		switch (cmdType) {
			case CMDTYPE_ICON:{
				Command c(cmdDesc.id);
				if ((cmdDesc.id < 0) || (cmdDesc.id == CMD_STOCKPILE)) {
					if (action.extra == "+5") {
						c.options = SHIFT_KEY;
					} else if (action.extra == "+20") {
						c.options = CONTROL_KEY;
					} else if (action.extra == "+100") {
						c.options = SHIFT_KEY | CONTROL_KEY;
					} else if (action.extra == "-1") {
						c.options = RIGHT_MOUSE_KEY;
					} else if (action.extra == "-5") {
						c.options = RIGHT_MOUSE_KEY | SHIFT_KEY;
					} else if (action.extra == "-20") {
						c.options = RIGHT_MOUSE_KEY | CONTROL_KEY;
					} else if (action.extra == "-100") {
						c.options = RIGHT_MOUSE_KEY | SHIFT_KEY | CONTROL_KEY;
					}
				}
				else if (action.extra.find("queued") != std::string::npos) {
					c.options |= SHIFT_KEY;
				}
				GiveCommand(c);
				break;
			}
			case CMDTYPE_ICON_MODE: {
				int newMode;

				if (!action.extra.empty() && (iconCmd < 0)) {
					newMode = atoi(action.extra.c_str());
				} else {
					newMode = atoi(cmdDesc.params[0].c_str()) + 1;
				}

				if ((newMode < 0) || ((size_t)newMode > (cmdDesc.params.size() - 2))) {
					newMode = 0;
				}

				// not really required
				char t[10];
				SNPRINTF(t, 10, "%d", newMode);
				cmdDesc.params[0] = t;

				Command c(cmdDesc.id, 0, newMode);
				GiveCommand(c);
				forceLayoutUpdate = true;
				break;
			}
			case CMDTYPE_NUMBER:{
				if (!action.extra.empty()) {
					const SCommandDescription& cd = cmdDesc;
					float value = atof(action.extra.c_str());
					float minV = 0.0f;
					float maxV = 100.0f;
					if (!cd.params.empty()) { minV = atof(cd.params[0].c_str()); }
					if (cd.params.size() >= 2) { maxV = atof(cd.params[1].c_str()); }
					value = std::max(std::min(value, maxV), minV);
					Command c(cd.id, 0, value);
					if (action.extra.find("queued") != std::string::npos) {
						c.options = SHIFT_KEY;
					}
					GiveCommand(c);
					break;
				}
				else {
					// fall through
				}
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
				break;
			}
			case CMDTYPE_ICON_BUILDING: {
				const UnitDef* ud=unitDefHandler->GetUnitDefByID(-cmdDesc.id);
				SetShowingMetal(ud->extractsMetal > 0);
				actionOffset = actionIndex;
				lastKeySet = ks;
				inCommand = a;
				break;
			}
			case CMDTYPE_NEXT: {
				++activePage;
				if(activePage > maxPage)
					activePage=0;
				selectedUnitsHandler.SetCommandPage(activePage);
				break;
			}
			case CMDTYPE_PREV:{
				--activePage;
				if(activePage<0)
					activePage = maxPage;
				selectedUnitsHandler.SetCommandPage(activePage);
				break;
			}
			case CMDTYPE_CUSTOM: {
				RunCustomCommands(cmdDesc.params, false);
				break;
			}
			default:{
				lastKeySet.Reset();
				SetShowingMetal(false);
				inCommand = a;
			}
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
		if (commands[iconCmd].tooltip != "") {
			s = commands[iconCmd].tooltip;
		}else{
			s = commands[iconCmd].name;
		}

		const CKeyBindings::HotkeyList& hl = keyBindings->GetHotkeys(commands[iconCmd].action);
		if(!hl.empty()){
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
	if ((inCommand >= 0) && ((size_t)inCommand < commands.size()) &&
	    (commands[inCommand].type == CMDTYPE_ICON_BUILDING)) {
		return commands[inCommand].tooltip;
	}
	return std::string("");
}


Command CGuiHandler::GetOrderPreview()
{
	return GetCommand(mouse->lastx, mouse->lasty, -1, true);
}


inline Command CheckCommand(Command c) {
	if (selectedUnitsHandler.selectedUnits.empty() || (c.options & SHIFT_KEY))
		return c; // always allow queued commands, since conditions may change so the command becomes valid
	for (CUnitSet::iterator ui = selectedUnitsHandler.selectedUnits.begin(); ui != selectedUnitsHandler.selectedUnits.end(); ++ui) {
		if((*ui)->commandAI->AllowedCommand(c, false))
			return c;
	}
	Command failedRet(CMD_FAILED);
	return failedRet;
}

bool ZeroRadiusAllowed(const Command &c) {
	switch(c.GetID()) {
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
	Command defaultRet(CMD_FAILED);

	int button;

	if (buttonHint >= SDL_BUTTON_LEFT) {
		button = buttonHint;
	} else if (inCommand != -1) {
		button = SDL_BUTTON_LEFT;
	} else if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
		button = SDL_BUTTON_RIGHT;
	} else {
		return Command(CMD_STOP);
	}

	int tempInCommand = inCommand;

	if (button == SDL_BUTTON_RIGHT && preview) {
		// right click -> default cmd
		// (in preview we might not have default cmd memory set)
		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
			tempInCommand = defaultCmdMemory;
		} else {
			tempInCommand = GetDefaultCommand(mouseX, mouseY, cameraPos, mouseDir);
		}
	}

	if ((size_t)tempInCommand >= commands.size()) {
		if (!preview)
			inCommand = -1;

		return Command(CMD_STOP);
	}

	switch (commands[tempInCommand].type) {
		case CMDTYPE_NUMBER: {
			const float value = GetNumberInput(commands[tempInCommand]);
			Command c(commands[tempInCommand].id, CreateOptions(button), value);
			return CheckCommand(c);
		}

		case CMDTYPE_ICON: {
			Command c(commands[tempInCommand].id, CreateOptions(button));
			if (button == SDL_BUTTON_LEFT && !preview) {
				LOG_L(L_WARNING, "CMDTYPE_ICON left button press in incommand test? This should not happen.");
			}
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_MAP: {
			const float dist = CGround::LineGroundCol(cameraPos, cameraPos + (mouseDir * globalRendering->viewRange * 1.4f), false);

			if (dist < 0.0f)
				return defaultRet;

			const float3 pos = cameraPos + (mouseDir * dist);
			Command c(commands[tempInCommand].id, CreateOptions(button), pos);
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_BUILDING: {
			const float dist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

			if (dist < 0.0f)
				return defaultRet;

			const UnitDef* unitdef = unitDefHandler->GetUnitDefByID(-commands[inCommand].id);

			if (unitdef == nullptr)
				return Command(CMD_STOP);

			std::vector<BuildInfo> buildPos;
			BuildInfo bi(unitdef, cameraPos + mouseDir * dist, buildFacing);

			if (GetQueueKeystate() && (button == SDL_BUTTON_LEFT)) {
				const float dist2 = CGround::LineGroundCol(
					mouse->buttons[SDL_BUTTON_LEFT].camPos,
					mouse->buttons[SDL_BUTTON_LEFT].camPos +
					mouse->buttons[SDL_BUTTON_LEFT].dir * globalRendering->viewRange * 1.4f,
					false
				);
				const float3 pos2 = mouse->buttons[SDL_BUTTON_LEFT].camPos + mouse->buttons[SDL_BUTTON_LEFT].dir * dist2;
				buildPos = GetBuildPos(BuildInfo(unitdef, pos2, buildFacing), bi, cameraPos, mouseDir);
			} else
				buildPos = GetBuildPos(bi, bi, cameraPos, mouseDir);

			if (buildPos.empty())
				return Command(CMD_STOP);

			if (buildPos.size() == 1) {
				CFeature* feature = nullptr;
				// TODO Maybe also check out-of-range for immobile builder?
				if (!CGameHelper::TestUnitBuildSquare(buildPos[0], feature, gu->myAllyTeam, false)) {
					return defaultRet;
				}
			}

			if (!preview) {
				for (auto bpi = buildPos.cbegin(); bpi != --buildPos.cend(); ++bpi) {
					Command c = bpi->CreateCommand(CreateOptions(button));
					GiveCommand(c);
				}
			}
			Command c = buildPos.back().CreateCommand(CreateOptions(button));
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_UNIT: {
			const CUnit* unit = nullptr;
			const CFeature* feature = nullptr;
			Command c(commands[tempInCommand].id, CreateOptions(button));

			TraceRay::GuiTraceRay(cameraPos, mouseDir, globalRendering->viewRange * 1.4f, NULL, unit, feature, true);
			if (!unit) {
				return defaultRet;
			}
			c.PushParam(unit->id);
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_UNIT_OR_MAP: {
			Command c(commands[tempInCommand].id, CreateOptions(button));

			const CUnit* unit = nullptr;
			const CFeature* feature = nullptr;
			const float dist2 = TraceRay::GuiTraceRay(cameraPos, mouseDir, globalRendering->viewRange * 1.4f, NULL, unit, feature, true);

			if (dist2 > (globalRendering->viewRange * 1.4f - 300))
				return defaultRet;

			if (unit != nullptr) {
				// clicked on unit
				c.PushParam(unit->id);
			} else {
				// clicked in map
				c.PushPos(cameraPos + (mouseDir * dist2));
			}
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_FRONT: {
			const float dist = CGround::LineGroundCol(
				mouse->buttons[button].camPos,
				mouse->buttons[button].camPos +
				mouse->buttons[button].dir * globalRendering->viewRange * 1.4f,
				false
			);

			if (dist < 0.0f)
				return defaultRet;

			float3 pos = mouse->buttons[button].camPos + (mouse->buttons[button].dir * dist);

			Command c(commands[tempInCommand].id, CreateOptions(button), pos);

			if (mouse->buttons[button].movement > 30) { // only create the front if the mouse has moved enough
				const float dist2 = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

				if (dist2 < 0.0f)
					return defaultRet;

				float3 pos2 = cameraPos + (mouseDir * dist2);

				ProcessFrontPositions(pos, pos2);
				c.SetPos(0, pos);

				if (!commands[tempInCommand].params.empty() &&
				    pos.SqDistance2D(pos2) > Square(atof(commands[tempInCommand].params[0].c_str()))) {
					const float3 dif = (pos2 - pos).ANormalize();
					pos2 = pos + dif * atoi(commands[tempInCommand].params[0].c_str());
				}

				c.PushPos(pos2);
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

			if (mouse->buttons[button].movement < 4) {
				const CUnit* unit = nullptr;
				const CFeature* feature = nullptr;
				const float dist2 = TraceRay::GuiTraceRay(cameraPos, mouseDir, globalRendering->viewRange * 1.4f, NULL, unit, feature, true);

				if (dist2 > (globalRendering->viewRange * 1.4f - 300) && (commands[tempInCommand].type != CMDTYPE_ICON_UNIT_FEATURE_OR_AREA))
					return defaultRet;

				if (feature && commands[tempInCommand].type == CMDTYPE_ICON_UNIT_FEATURE_OR_AREA) { // clicked on feature
					c.PushParam(unitHandler->MaxUnits() + feature->id);
				} else if (unit && commands[tempInCommand].type != CMDTYPE_ICON_AREA) { // clicked on unit
					if (c.GetID() == CMD_RESURRECT)
						return defaultRet; // cannot resurrect units!
					c.PushParam(unit->id);
				} else { // clicked in map
					if (explicitCommand < 0 || !ZeroRadiusAllowed(c)) // only attack ground if explicitly set the command
						return defaultRet;
					c.PushPos(cameraPos + (mouseDir * dist2));
					c.PushParam(0); // zero radius
					if (c.GetID() == CMD_UNLOAD_UNITS) {
						c.PushParam((float)buildFacing);
					}
				}
			} else { // created area
				const float dist = CGround::LineGroundCol(
					mouse->buttons[button].camPos,
					mouse->buttons[button].camPos +
					mouse->buttons[button].dir * globalRendering->viewRange * 1.4f,
					false
				);

				if (dist < 0.0f)
					return defaultRet;

				const float3 pos = mouse->buttons[button].camPos + mouse->buttons[button].dir * dist;

				c.PushPos(pos);

				const float dist2 = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

				if (dist2 < 0.0f)
					return defaultRet;

				const float3 pos2 = cameraPos + mouseDir * dist2;
				c.PushParam(std::min(maxRadius, pos.distance2D(pos2)));

				if (c.GetID() == CMD_UNLOAD_UNITS) {
					c.PushParam((float)buildFacing);
				}
			}
			return CheckCommand(c);
		}

		case CMDTYPE_ICON_UNIT_OR_RECTANGLE: {
			Command c(commands[tempInCommand].id, CreateOptions(button));

			if (mouse->buttons[button].movement < 16) {
				const CUnit* unit = nullptr;
				const CFeature* feature = nullptr;
				const float dist2 = TraceRay::GuiTraceRay(cameraPos, mouseDir, globalRendering->viewRange * 1.4f, NULL, unit, feature, true);

				if (dist2 > (globalRendering->viewRange * 1.4f - 300))
					return defaultRet;

				if (unit != nullptr) {
					// clicked on unit
					c.PushParam(unit->id);
				} else {
					// clicked in map; only attack ground if explicitly set the command
					if (explicitCommand < 0.0f)
						return defaultRet;

					c.PushPos(cameraPos + (mouseDir * dist2));
				}
			} else {
				// created rectangle
				const float dist = CGround::LineGroundCol(
					mouse->buttons[button].camPos,
					mouse->buttons[button].camPos +
					mouse->buttons[button].dir * globalRendering->viewRange * 1.4f,
					false
				);

				if (dist < 0.0f)
					return defaultRet;

				const float dist2 = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

				if (dist2 < 0.0f)
					return defaultRet;

				const float3 startPos = mouse->buttons[button].camPos + mouse->buttons[button].dir * dist;
				const float3 endPos = cameraPos + (mouseDir * dist2);

				c.PushPos(startPos);
				c.PushPos(endPos);
			}
			return CheckCommand(c);
		}
	}

	return Command(CMD_STOP);
}



static bool WouldCancelAnyQueued(const BuildInfo& b)
{
	Command c = b.CreateCommand();
	CUnitSet::iterator ui = selectedUnitsHandler.selectedUnits.begin();
	for (; ui != selectedUnitsHandler.selectedUnits.end(); ++ui) {
		if ((*ui)->commandAI->WillCancelQueued(c)) {
			return true;
		}
	}
	return false;
}

static void FillRowOfBuildPos(const BuildInfo& startInfo, float x, float z, float xstep, float zstep, int n, int facing, bool nocancel, std::vector<BuildInfo>& ret)
{
	for (int i = 0; i < n; ++i) {
		BuildInfo bi(startInfo.def, float3(x, 0.0f, z), (startInfo.buildFacing + facing) % NUM_FACINGS);
		bi.pos=CGameHelper::Pos2BuildPos(bi, false);
		if (!nocancel || !WouldCancelAnyQueued(bi)) {
			ret.push_back(bi);
		}
		x += xstep;
		z += zstep;
	}
}

// Assuming both builds have the same unitdef
std::vector<BuildInfo> CGuiHandler::GetBuildPos(const BuildInfo& startInfo, const BuildInfo& endInfo, const float3& cameraPos, const float3& mouseDir)
{
	std::vector<BuildInfo> ret;

	float3 start = CGameHelper::Pos2BuildPos(startInfo, false);
	float3 end = CGameHelper::Pos2BuildPos(endInfo, false);

	BuildInfo other; // the unit around which buildings can be circled

	if (GetQueueKeystate() && KeyInput::GetKeyModState(KMOD_CTRL)) {
		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;
		TraceRay::GuiTraceRay(cameraPos, mouseDir, globalRendering->viewRange * 1.4f, NULL, unit, feature, true);

		if (unit != nullptr) {
			other.def = unit->unitDef;
			other.pos = unit->pos;
			other.buildFacing = unit->buildFacing;
		} else {
			Command c = CGameHelper::GetBuildCommand(cameraPos, mouseDir);
			if (c.GetID() < 0 && c.params.size() == 4) {
				other.pos = c.GetPos(0);
				other.def = unitDefHandler->GetUnitDefByID(-c.GetID());
				other.buildFacing = int(c.params[3]);
			}
		}
	}

	if (other.def && GetQueueKeystate() && KeyInput::GetKeyModState(KMOD_CTRL)) {
		// circle build around building
		int oxsize = other.GetXSize() * SQUARE_SIZE;
		int ozsize = other.GetZSize() * SQUARE_SIZE;
		int xsize = startInfo.GetXSize() * SQUARE_SIZE;
		int zsize = startInfo.GetZSize() * SQUARE_SIZE;

		start = end = CGameHelper::Pos2BuildPos(other, false);
		start.x -= oxsize / 2;
		start.z -= ozsize / 2;
		end.x += oxsize / 2;
		end.z += ozsize / 2;

		int nvert = 1 + (oxsize / xsize);
		int nhori = 1 + (ozsize / xsize);

		FillRowOfBuildPos(startInfo, end.x   + zsize / 2, start.z + xsize / 2,      0,  xsize, nhori, 3, true, ret);
		FillRowOfBuildPos(startInfo, end.x   - xsize / 2, end.z   + zsize / 2, -xsize,      0, nvert, 2, true, ret);
		FillRowOfBuildPos(startInfo, start.x - zsize / 2, end.z   - xsize / 2,      0, -xsize, nhori, 1, true, ret);
		FillRowOfBuildPos(startInfo, start.x + xsize / 2, start.z - zsize / 2,  xsize,      0, nvert, 0, true, ret);

	} else { // rectangle or line
		const float3 delta = end - start;

		const float xsize = SQUARE_SIZE * (startInfo.GetXSize() + buildSpacing * 2);
		const int xnum = (int)((math::fabs(delta.x) + xsize * 1.4f)/xsize);
		float xstep = (int)((0 < delta.x) ? xsize : -xsize);

		const float zsize = SQUARE_SIZE * (startInfo.GetZSize() + buildSpacing * 2);
		const int znum = (int)((math::fabs(delta.z) + zsize * 1.4f)/zsize);
		float zstep = (int)((0 < delta.z) ? zsize : -zsize);

		if (KeyInput::GetKeyModState(KMOD_ALT)) { // build a rectangle
			if (KeyInput::GetKeyModState(KMOD_CTRL)) { // hollow rectangle
				if ((1 < xnum) && (1 < znum)) {
					// go "down" on the "left" side
					FillRowOfBuildPos(startInfo, start.x                     , start.z + zstep             ,      0,  zstep, znum - 1, 0, false, ret);
					// go "right" on the "bottom" side
					FillRowOfBuildPos(startInfo, start.x + xstep             , start.z + (znum - 1) * zstep,  xstep,      0, xnum - 1, 0, false, ret);
					// go "up" on the "right" side
					FillRowOfBuildPos(startInfo, start.x + (xnum - 1) * xstep, start.z + (znum - 2) * zstep,      0, -zstep, znum - 1, 0, false, ret);
					// go "left" on the "top" side
					FillRowOfBuildPos(startInfo, start.x + (xnum - 2) * xstep, start.z                     , -xstep,      0, xnum - 1, 0, false, ret);
				} else if (1 == xnum) {
					FillRowOfBuildPos(startInfo, start.x, start.z, 0, zstep, znum, 0, false, ret);
				} else if (1 == znum) {
					FillRowOfBuildPos(startInfo, start.x, start.z, xstep, 0, xnum, 0, false, ret);
				}
			} else { // filled rectangle
				int zn = 0;
				for (float z = start.z; zn < znum; ++zn) {
					if (zn & 1) {
						// every odd line "right" to "left"
						FillRowOfBuildPos(startInfo, start.x + (xnum - 1) * xstep, z, -xstep, 0, xnum, 0, false, ret);
					} else {
						// every even line "left" to "right"
						FillRowOfBuildPos(startInfo, start.x                     , z,  xstep, 0, xnum, 0, false, ret);
					}
					z += zstep;
				}
			}
		} else { // build a line
			const bool xDominatesZ = math::fabs(delta.x) > math::fabs(delta.z);
			if (xDominatesZ) {
				zstep = KeyInput::GetKeyModState(KMOD_CTRL) ? 0 : xstep * delta.z / (delta.x ? delta.x : 1);
			} else {
				xstep = KeyInput::GetKeyModState(KMOD_CTRL) ? 0 : zstep * delta.x / (delta.z ? delta.z : 1);
			}
			FillRowOfBuildPos(startInfo, start.x, start.z, xstep, zstep, xDominatesZ ? xnum : znum, 0, false, ret);
		}
	}
	return ret;
}


void CGuiHandler::ProcessFrontPositions(float3& pos0, const float3& pos1)
{
	if (!frontByEnds) {
		return; // leave it centered
	}
	pos0 = pos1 + ((pos0 - pos1) * 0.5f);
	pos0.y = CGround::GetHeightReal(pos0.x, pos0.z, false);
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::Draw()
{
	if ((iconsCount <= 0) && (luaUI == nullptr))
		return;

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.01f);

	if (iconsCount > 0)
		DrawButtons();

	glPopAttrib();
}


static std::string FindCornerText(const std::string& corner, const vector<std::string>& params)
{
	for (int p = 0; p < (int)params.size(); p++) {
		if (params[p].find(corner) == 0) {
			return params[p].substr(corner.length());
		}
	}
	return std::string("");
}


void CGuiHandler::DrawCustomButton(const IconInfo& icon, bool highlight)
{
	const SCommandDescription& cmdDesc = commands[icon.commandsID];

	const bool usedTexture = DrawTexture(icon, cmdDesc.iconname);

	// highlight overlay before text is applied
	if (highlight) {
		DrawHilightQuad(icon);
	}

	if (!usedTexture || !cmdDesc.onlyTexture) {
		DrawName(icon, cmdDesc.name, false);
	}

	DrawNWtext(icon, FindCornerText("$nw$", cmdDesc.params));
	DrawSWtext(icon, FindCornerText("$sw$", cmdDesc.params));
	DrawNEtext(icon, FindCornerText("$ne$", cmdDesc.params));
	DrawSEtext(icon, FindCornerText("$se$", cmdDesc.params));

	if (!usedTexture) {
		glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
		DrawIconFrame(icon);
	}
}


bool CGuiHandler::DrawUnitBuildIcon(const IconInfo& icon, int unitDefID)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud) {
		const Box& b = icon.visual;
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, textureAlpha);
		glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitDefImage(ud));
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(b.x1, b.y1);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(b.x2, b.y1);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(b.x2, b.y2);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(b.x1, b.y2);
		glEnd();
		return true;
	}

	return false;
}


static inline bool ParseTextures(const std::string& texString,
		std::string& tex1, std::string& tex2, float& xscale, float& yscale)
{
	// format:  "&<xscale>x<yscale>&<tex>1&<tex2>"  --  <>'s are not included

	char* endPtr;

	const char* c = texString.c_str() + 1;
	xscale = strtod(c, &endPtr);
	if ((endPtr == c) || (endPtr[0] != 'x')) { return false; }
	c = endPtr + 1;
	yscale = strtod(c, &endPtr);
	if ((endPtr == c) || (endPtr[0] != '&')) { return false; }
	c = endPtr + 1;
	const char* tex1Start = c;
	while ((c[0] != 0) && (c[0] != '&')) { c++; }
	if (c[0] != '&') { return false; }
	const int tex1Len = c - tex1Start;
	c++;
	tex1 = c; // draw 'tex2' first
	tex2 = std::string(tex1Start, tex1Len);

	return true;
}


static inline bool BindUnitTexByString(const std::string& str)
{
	char* endPtr;
	const char* startPtr = str.c_str() + 1; // skip the '#'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
	if (endPtr == startPtr) {
		return false; // bad unitID spec
	}

	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitDefImage(ud));

	return true;
}


static inline bool BindIconTexByString(const std::string& str)
{
	char* endPtr;
	const char* startPtr = str.c_str() + 1; // skip the '^'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
	if (endPtr == startPtr) {
		return false; // bad unitID spec
	}

	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return false;
	}

	ud->iconType->BindTexture();

	return true;
}


static inline bool BindLuaTexByString(const std::string& str)
{
	CLuaHandle* luaHandle = NULL;
	const char scriptType = str[1];
	switch (scriptType) {
		case 'u': { luaHandle = luaUI; break; }
		case 'g': { luaHandle = &luaGaia->unsyncedLuaHandle; break; }
		case 'm': { luaHandle = &luaRules->unsyncedLuaHandle; break; }
		default:  { break; }
	}
	if (luaHandle == NULL) {
		return false;
	}
	if (str[2] != LuaTextures::prefix) { // '!'
		return false;
	}

	const string luaTexStr = str.substr(2);
	const LuaTextures::Texture* texInfo =
		luaHandle->GetTextures().GetInfo(luaTexStr);
	if (texInfo == NULL) {
		return false;
	}

	if (texInfo->target != GL_TEXTURE_2D) {
		return false;
	}
	glBindTexture(GL_TEXTURE_2D, texInfo->id);

	return true;
}


static bool BindTextureString(const std::string& str)
{
	if (str[0] == '#') {
		return BindUnitTexByString(str);
	} else if (str[0] == '^') {
		return BindIconTexByString(str);
	} else if (str[0] == LuaTextures::prefix) { // '!'
		return BindLuaTexByString(str);
	} else {
		return CNamedTextures::Bind(str);
	}
}


bool CGuiHandler::DrawTexture(const IconInfo& icon, const std::string& texName)
{
	if (texName.empty()) {
		return false;
	}

	std::string tex1;
	std::string tex2;
	float xscale = 1.0f;
	float yscale = 1.0f;

	// double texture?
	if (texName[0] == '&') {
		if (!ParseTextures(texName, tex1, tex2, xscale, yscale)) {
			return false;
		}
	} else {
		tex1 = texName;
	}

	// bind the texture for the full size quad
	if (!BindTextureString(tex1)) {
		if (tex2.empty()) {
			return false;
		} else {
			if (!BindTextureString(tex2)) {
				return false;
			} else {
				tex2.clear(); // cancel the scaled draw
			}
		}
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, textureAlpha);

	// draw the full size quad
	const Box& b = icon.visual;
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(b.x1, b.y1);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(b.x2, b.y1);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(b.x2, b.y2);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(b.x1, b.y2);
	glEnd();

	if (tex2.empty()) {
		return true; // success, no second texture to draw
	}

	// bind the texture for the scaled quad
	if (!BindTextureString(tex2)) {
		return false;
	}

	assert(xscale<=0.5); //border >= 50% makes no sence
	assert(yscale<=0.5);

	// calculate the scaled quad
	const float x1 = b.x1 + (xIconSize * xscale);
	const float x2 = b.x2 - (xIconSize * xscale);
	const float y1 = b.y1 - (yIconSize * yscale);
	const float y2 = b.y2 + (yIconSize * yscale);

	// draw the scaled quad
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(x1, y1);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(x2, y1);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(x2, y2);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(x1, y2);
	glEnd();

	return true;
}


void CGuiHandler::DrawIconFrame(const IconInfo& icon)
{
	const Box& b = icon.visual;
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	const float fudge = 0.001f; // avoids getting creamed if iconBorder == 0.0
	glVertex2f(b.x1 + fudge, b.y1 - fudge);
	glVertex2f(b.x2 - fudge, b.y1 - fudge);
	glVertex2f(b.x2 - fudge, b.y2 + fudge);
	glVertex2f(b.x1 + fudge, b.y2 + fudge);
	glEnd();
}


void CGuiHandler::DrawName(const IconInfo& icon, const std::string& text,
		bool offsetForLEDs)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;

	const float yShrink = offsetForLEDs ? (0.125f * yIconSize) : 0.0f;

	const float tWidth  = std::max(0.01f, font->GetSize() * font->GetTextWidth(text) * globalRendering->pixelX);  // FIXME
	const float tHeight = std::max(0.01f, font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY); // FIXME merge in 1 function?
	const float textBorder2 = (2.0f * textBorder);
	const float xScale = (xIconSize - textBorder2          ) / tWidth;
	const float yScale = (yIconSize - textBorder2 - yShrink) / tHeight;
	const float fontScale = std::min(xScale, yScale);

	const float xCenter = 0.5f * (b.x1 + b.x2);
	const float yCenter = 0.5f * (b.y1 + b.y2 + yShrink);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	font->glPrint(xCenter, yCenter, fontScale, (dropShadows ? FONT_SHADOW : 0) | FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM, text);
}


void CGuiHandler::DrawNWtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = b.x1 + textBorder + 0.002f;
	const float yPos = b.y1 - textBorder - 0.006f;

	font->glPrint(xPos, yPos, fontScale, FONT_TOP | FONT_SCALE | FONT_NORM, text);
}


void CGuiHandler::DrawSWtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = b.x1 + textBorder + 0.002f;
	const float yPos = b.y2 + textBorder + 0.002f;

	font->glPrint(xPos, yPos, fontScale, FONT_SCALE | FONT_NORM, text);
}


void CGuiHandler::DrawNEtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = b.x2 - textBorder - 0.002f;
	const float yPos = b.y1 - textBorder - 0.006f;

	font->glPrint(xPos, yPos, fontScale, FONT_TOP | FONT_RIGHT | FONT_SCALE | FONT_NORM, text);
}


void CGuiHandler::DrawSEtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY;
	const float fontScale = (yIconSize * 0.2f) / tHeight;
	const float xPos = b.x2 - textBorder - 0.002f;
	const float yPos = b.y2 + textBorder + 0.002f;

	font->glPrint(xPos, yPos, fontScale, FONT_RIGHT | FONT_SCALE | FONT_NORM, text);
}


void CGuiHandler::DrawHilightQuad(const IconInfo& icon)
{
	if (icon.commandsID == inCommand) {
		glColor4f(0.3f, 0.0f, 0.0f, 1.0f);
	} else if (mouse->buttons[SDL_BUTTON_LEFT].pressed) {
		glColor4f(0.2f, 0.0f, 0.0f, 1.0f);
	} else {
		glColor4f(0.0f, 0.0f, 0.2f, 1.0f);
	}
	const Box& b = icon.visual;
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE); // additive blending
	glBegin(GL_QUADS);
		glVertex2f(b.x1, b.y1);
		glVertex2f(b.x2, b.y1);
		glVertex2f(b.x2, b.y2);
		glVertex2f(b.x1, b.y2);
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void CGuiHandler::DrawButtons() // Only called by Draw
{
	glLineWidth(1.0f);
	font->Begin();

	// frame box
	const float alpha = (frameAlpha < 0.0f) ? guiAlpha : frameAlpha;
	if (alpha > 0.0f) {
		glColor4f(0.2f, 0.2f, 0.2f, alpha);
		glBegin(GL_QUADS);
			glVertex2f(buttonBox.x1, buttonBox.y1);
			glVertex2f(buttonBox.x1, buttonBox.y2);
			glVertex2f(buttonBox.x2, buttonBox.y2);
			glVertex2f(buttonBox.x2, buttonBox.y1);
		glEnd();
	}

	const int mouseIcon   = IconAtPos(mouse->lastx, mouse->lasty);
	const int buttonStart = std::min(iconsCount, activePage * iconsPerPage);
	const int buttonEnd   = std::min(iconsCount, buttonStart + iconsPerPage);

	for (int ii = buttonStart; ii < buttonEnd; ii++) {

		const IconInfo& icon = icons[ii];

		if (icon.commandsID < 0)
			continue; // inactive icon

		const SCommandDescription& cmdDesc = commands[icon.commandsID];
		const bool customCommand = (cmdDesc.id == CMD_INTERNAL) &&
		                           (cmdDesc.type == CMDTYPE_CUSTOM);
		const bool highlight = ((mouseIcon == ii) ||
		                        (icon.commandsID == inCommand)) &&
		                       (!customCommand || !cmdDesc.params.empty());

		if (customCommand) {
			DrawCustomButton(icon, highlight);
		}
		else {
			bool usedTexture = false;
			bool onlyTexture = cmdDesc.onlyTexture;
			const bool useLEDs = useOptionLEDs && (cmdDesc.type == CMDTYPE_ICON_MODE);

			// specified texture
			if (DrawTexture(icon, cmdDesc.iconname)) {
				usedTexture = true;
			}

			// unit buildpic
			if (!usedTexture) {
				if (cmdDesc.id < 0) {
					const UnitDef* ud = unitDefHandler->GetUnitDefByID(-cmdDesc.id);
					if (ud != NULL) {
						DrawUnitBuildIcon(icon, -cmdDesc.id);
						usedTexture = true;
						onlyTexture = true;
					}
				}
			}

			// highlight background before text is applied
			if (highlight) {
				DrawHilightQuad(icon);
			}

			// build count text (NOTE the weird bracing)
			if (cmdDesc.id < 0) {
				const int psize = (int)cmdDesc.params.size();
				if (psize > 0) { DrawSWtext(icon, cmdDesc.params[0]);
					if (psize > 1) { DrawNEtext(icon, cmdDesc.params[1]);
						if (psize > 2) { DrawNWtext(icon, cmdDesc.params[2]);
							if (psize > 3) { DrawSEtext(icon, cmdDesc.params[3]); } } } }
			}

			// draw arrows, or a frame for text
			if (!usedTexture || !onlyTexture) {
				if ((cmdDesc.type == CMDTYPE_PREV) || (cmdDesc.type == CMDTYPE_NEXT)) {
					// pick the color for the arrow
					if (highlight) {
						glColor4f(1.0f, 1.0f, 0.0f, 1.0f); // selected
					} else {
						glColor4f(0.7f, 0.7f, 0.7f, 1.0f); // normal
					}
					if (cmdDesc.type == CMDTYPE_PREV) {
						DrawPrevArrow(icon);
					} else {
						DrawNextArrow(icon);
					}
				}
				else if (!usedTexture) {
					// no texture, no arrow, ... draw a frame
					glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
					DrawIconFrame(icon);
				}

				// draw the text
				// command name (or parameter)
				std::string toPrint = cmdDesc.name;
				if (cmdDesc.type == CMDTYPE_ICON_MODE
						&& !cmdDesc.params.empty()) {
					const int opt = atoi(cmdDesc.params[0].c_str()) + 1;
					if (opt < 0 || static_cast<size_t>(opt) < cmdDesc.params.size()) {
						toPrint = cmdDesc.params[opt];
					}
				}
				DrawName(icon, toPrint, useLEDs);
			}

			// draw the mode indicators
			if (useLEDs) {
				DrawOptionLEDs(icon);
			}
		}

		// darken disabled commands
		if (cmdDesc.disabled) {
			glDisable(GL_TEXTURE_2D);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
			const Box& vb = icon.visual;
			glRectf(vb.x1, vb.y1, vb.x2, vb.y2);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		// highlight outline
		if (highlight) {
			if (icon.commandsID == inCommand) {
				glColor4f(1.0f, 1.0f, 0.0f, 0.75f);
			} else if (mouse->buttons[SDL_BUTTON_LEFT].pressed ||
			           mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
				glColor4f(1.0f, 0.0f, 0.0f, 0.50f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.50f);
			}
			glLineWidth(1.49f);
			DrawIconFrame(icon);
			glLineWidth(1.0f);
		}
	}

	// active page indicator
	if (luaUI == NULL) {
		if (selectedUnitsHandler.BuildIconsFirst()) {
			glColor4fv(cmdColors.build);
		} else {
			glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
		}
		const float textSize = 1.2f;
		font->glFormat(xBpos, yBpos, textSize, FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM, "%i", activePage + 1);
	}

	DrawMenuName();

	font->End();

	// LuaUI can handle this
	if (luaUI == NULL || drawSelectionInfo)
		DrawSelectionInfo();

	DrawNumberInput();
}


void CGuiHandler::DrawMenuName() // Only called by drawbuttons
{
	if (!menuName.empty() && (iconsCount > 0)) {
		const float fontScale = 1.0f;
		const float xp = 0.5f * (buttonBox.x1 + buttonBox.x2);
		const float yp = buttonBox.y2 + (yIconSize * 0.125f);

		if (!outlineFonts) {
			const float textHeight = fontScale * font->GetTextHeight(menuName) * globalRendering->pixelY;
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f, 0.2f, 0.2f, guiAlpha);
			glRectf(buttonBox.x1,
			        buttonBox.y2,
			        buttonBox.x2,
			        buttonBox.y2 + textHeight + (yIconSize * 0.25f));
			font->glPrint(xp, yp, fontScale, FONT_CENTER | FONT_SCALE | FONT_NORM, menuName);
		}
		else {
			font->SetColors(); // default
			font->glPrint(xp, yp, fontScale, FONT_CENTER | FONT_OUTLINE | FONT_SCALE | FONT_NORM, menuName);
		}
	}

}


void CGuiHandler::DrawSelectionInfo()
{
	if (!selectedUnitsHandler.selectedUnits.empty()) {
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
			const float textWidth  = fontSize * smallFont->GetTextWidth(buf.str()) * globalRendering->pixelX;
			float textHeight = fontSize * smallFont->GetTextHeight(buf.str(), &descender) * globalRendering->pixelY;
			const float textDescender = fontSize * descender * globalRendering->pixelY; //! descender is always negative
			textHeight -= textDescender;

			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f, 0.2f, 0.2f, guiAlpha);
			glRectf(xSelectionPos - frameBorder,
			        ySelectionPos - frameBorder,
			        xSelectionPos + frameBorder + textWidth,
			        ySelectionPos + frameBorder + textHeight);
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			smallFont->glPrint(xSelectionPos, ySelectionPos - textDescender, fontSize, FONT_BASELINE | FONT_NORM, buf.str());
		} else {
			smallFont->SetColors(); // default
			smallFont->glPrint(xSelectionPos, ySelectionPos, fontSize, FONT_OUTLINE | FONT_NORM, buf.str());
		}
	}
}


void CGuiHandler::DrawNumberInput() // Only called by drawbuttons
{
	// draw the value for CMDTYPE_NUMBER commands
	if ((inCommand >= 0) && ((size_t)inCommand < commands.size())) {
		const SCommandDescription& cd = commands[inCommand];
		if (cd.type == CMDTYPE_NUMBER) {
			const float value = GetNumberInput(cd);
			glDisable(GL_TEXTURE_2D);
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			const float mouseX = (float)mouse->lastx / (float)globalRendering->viewSizeX;
			const float slideX = std::min(std::max(mouseX, 0.25f), 0.75f);
			//const float mouseY = 1.0f - (float)(mouse->lasty - 16) / (float)globalRendering->viewSizeY;
			glColor4f(1.0f, 1.0f, 0.0f, 0.8f);
			glRectf(0.235f, 0.45f, 0.25f, 0.55f);
			glRectf(0.75f, 0.45f, 0.765f, 0.55f);
			glColor4f(0.0f, 0.0f, 1.0f, 0.8f);
			glRectf(0.25f, 0.49f, 0.75f, 0.51f);
			glBegin(GL_TRIANGLES);
				glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
				glVertex2f(slideX + 0.015f, 0.55f);
				glVertex2f(slideX - 0.015f, 0.55f);
				glVertex2f(slideX, 0.50f);
				glVertex2f(slideX - 0.015f, 0.45f);
				glVertex2f(slideX + 0.015f, 0.45f);
				glVertex2f(slideX, 0.50f);
			glEnd();
			glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
			font->glFormat(slideX, 0.56f, 2.0f, FONT_CENTER | FONT_SCALE | FONT_NORM, "%i", (int)value);
		}
	}
}


void CGuiHandler::DrawPrevArrow(const IconInfo& icon)
{
	const Box& b = icon.visual;
	const float yCenter = 0.5f * (b.y1 + b.y2);
	const float xSize = 0.166f * math::fabs(b.x2 - b.x1);
	const float ySize = 0.125f * math::fabs(b.y2 - b.y1);
	const float xSiz2 = 2.0f * xSize;
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POLYGON);
		glVertex2f(b.x2 - xSize, yCenter - ySize);
		glVertex2f(b.x1 + xSiz2, yCenter - ySize);
		glVertex2f(b.x1 + xSize, yCenter);
		glVertex2f(b.x1 + xSiz2, yCenter + ySize);
		glVertex2f(b.x2 - xSize, yCenter + ySize);
	glEnd();
}


void CGuiHandler::DrawNextArrow(const IconInfo& icon)
{
	const Box& b = icon.visual;
	const float yCenter = 0.5f * (b.y1 + b.y2);
	const float xSize = 0.166f * math::fabs(b.x2 - b.x1);
	const float ySize = 0.125f * math::fabs(b.y2 - b.y1);
	const float xSiz2 = 2.0f * xSize;
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POLYGON);
		glVertex2f(b.x1 + xSize, yCenter - ySize);
		glVertex2f(b.x2 - xSiz2, yCenter - ySize);
		glVertex2f(b.x2 - xSize, yCenter);
		glVertex2f(b.x2 - xSiz2, yCenter + ySize);
		glVertex2f(b.x1 + xSize, yCenter + ySize);
	glEnd();
}


void CGuiHandler::DrawOptionLEDs(const IconInfo& icon)
{
	const SCommandDescription& cmdDesc = commands[icon.commandsID];

	const int pCount = (int)cmdDesc.params.size() - 1;
	if (pCount < 2) {
		return;
	}
	const int option = atoi(cmdDesc.params[0].c_str());

	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);

	const float xs = xIconSize / float(1 + (pCount * 2));
	const float ys = yIconSize * 0.125f;
	const float x1 = icon.visual.x1;
	const float y2 = icon.visual.y2;
	const float yp = 1.0f / float(globalRendering->viewSizeY);

	for (int x = 0; x < pCount; x++) {
		if (x != option) {
			glColor4f(0.25f, 0.25f, 0.25f, 0.50f); // dark
		} else {
			if (pCount == 2) {
				if (option == 0) {
					glColor4f(1.0f, 0.0f, 0.0f, 0.75f); // red
				} else {
					glColor4f(0.0f, 1.0f, 0.0f, 0.75f); // green
				}
			} else if (pCount == 3) {
				if (option == 0) {
					glColor4f(1.0f, 0.0f, 0.0f, 0.75f); // red
				} else if (option == 1) {
					glColor4f(1.0f, 1.0f, 0.0f, 0.75f); // yellow
				} else {
					glColor4f(0.0f, 1.0f, 0.0f, 0.75f); // green
				}
			} else {
				glColor4f(0.75f, 0.75f, 0.75f, 0.75f); // light
			}
		}

		const float startx = x1 + (xs * float(1 + (2 * x)));
		const float starty = y2 + (3.0f * yp) + textBorder;

		glRectf(startx, starty, startx + xs, starty + ys);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		glRectf(startx, starty, startx + xs, starty + ys);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


/******************************************************************************/
/******************************************************************************/

static inline void DrawSensorRange(int radius, const float* color, const float3& pos)
{
	if (radius > 0) {
		glColor4fv(color);
		glSurfaceCircle(pos, (float)radius, 40);
	}
}


static void DrawUnitDefRanges(const CUnit* unit, const UnitDef* unitdef, const float3 pos)
{
	// draw build range for immobile builders
	if (unitdef->builder) {
		const float radius = unitdef->buildDistance;
		if (radius > 0.0f) {
			glColor4fv(cmdColors.rangeBuild);
			glSurfaceCircle(pos, radius, 40);
		}
	}
	// draw shield range for immobile units
	if (unitdef->shieldWeaponDef) {
		glColor4fv(cmdColors.rangeShield);
		glSurfaceCircle(pos, unitdef->shieldWeaponDef->shieldRadius, 40);
	}
	// draw sensor and jammer ranges
	if (unitdef->onoffable || unitdef->activateWhenBuilt) {
		if (unit != nullptr && unitdef == unit->unitDef) { // test if it's a decoy
			DrawSensorRange(unit->radarRadius   , cmdColors.rangeRadar,       pos);
			DrawSensorRange(unit->sonarRadius   , cmdColors.rangeSonar,       pos);
			DrawSensorRange(unit->seismicRadius , cmdColors.rangeSeismic,     pos);
			DrawSensorRange(unit->jammerRadius  , cmdColors.rangeJammer,      pos);
			DrawSensorRange(unit->sonarJamRadius, cmdColors.rangeSonarJammer, pos);
		} else {
			DrawSensorRange(unitdef->radarRadius   , cmdColors.rangeRadar,       pos);
			DrawSensorRange(unitdef->sonarRadius   , cmdColors.rangeSonar,       pos);
			DrawSensorRange(unitdef->seismicRadius , cmdColors.rangeSeismic,     pos);
			DrawSensorRange(unitdef->jammerRadius  , cmdColors.rangeJammer,      pos);
			DrawSensorRange(unitdef->sonarJamRadius, cmdColors.rangeSonarJammer, pos);
		}
	}
	// draw self destruct and damage distance
	if (unitdef->kamikazeDist > 0) {
		glColor4fv(cmdColors.rangeKamikaze);
		glSurfaceCircle(pos, unitdef->kamikazeDist, 40);

		const WeaponDef* wd = unitdef->selfdExpWeaponDef;
		if (wd != NULL) {
			glColor4fv(cmdColors.rangeSelfDestruct);
			glSurfaceCircle(pos, wd->damages.damageAreaOfEffect, 40);
		}
	}

}



static inline GLuint GetConeList()
{
	static GLuint list = 0; // FIXME: put in the class
	if (list != 0) {
		return list;
	}
	list = glGenLists(1);
	glNewList(list, GL_COMPILE); {
		glBegin(GL_TRIANGLE_FAN);
		const int divs = 64;
		glVertex3f(0.0f, 0.0f, 0.0f);
		for (int i = 0; i <= divs; i++) {
			const float rad = (PI * 2.0) * (float)i / (float)divs;
			glVertex3f(1.0f, std::sin(rad), std::cos(rad));
		}
		glEnd();
	}
	glEndList();
	return list;
}


static void DrawWeaponCone(const float3& pos,
                           float len, float hrads, float heading, float pitch)
{
	glPushMatrix();

	const float xlen = len * std::cos(hrads);
	const float yzlen = len * std::sin(hrads);
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(heading * (180.0 / PI), 0.0f, 1.0f, 0.0f);
	glRotatef(pitch   * (180.0 / PI), 0.0f, 0.0f, 1.0f);
	glScalef(xlen, yzlen, yzlen);

	glEnable(GL_CULL_FACE);

	glCullFace(GL_FRONT);
	glColor4f(1.0f, 0.0f, 0.0f, 0.25f);
	glCallList(GetConeList());

	glCullFace(GL_BACK);
	glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
	glCallList(GetConeList());

	glDisable(GL_CULL_FACE);

	glPopMatrix();
}


static inline void DrawWeaponArc(const CUnit* unit)
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
		DrawWeaponCone(w->weaponMuzzlePos, w->range, hrads, heading, pitch);
	}
}


void CGuiHandler::DrawMapStuff(bool onMinimap)
{
	if (!onMinimap) {
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_ALPHA_TEST);
	}

	float3 cameraPos=camera->GetPos();
	float3 mouseDir=mouse->dir;

	// setup for minimap proxying
	const bool minimapCoords =
		(minimap->ProxyMode() ||
		 ((activeReceiver != this) && !game->hideInterface &&
		  (GetReceiverAt(mouse->lastx, mouse->lasty) == minimap)));
	if (minimapCoords) {
		cameraPos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
		mouseDir = -UpVector;
		if (miniMapMarker && minimap->FullProxy() &&
		    !onMinimap && !minimap->GetMinimized()) {
			DrawMiniMapMarker(cameraPos);
		}
	}


	if (activeMousePress) {
		int cmdIndex = -1;
		int button = SDL_BUTTON_LEFT;
		if ((inCommand >= 0) && ((size_t)inCommand < commands.size())) {
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
					if (mouse->buttons[button].movement > 30) {
						float maxSize = 1000000.0f;
						float sizeDiv = 0.0f;
						if (!cmdDesc.params.empty()) {
							maxSize = atof(cmdDesc.params[0].c_str());
							if (cmdDesc.params.size() > 1) {
								sizeDiv = atof(cmdDesc.params[1].c_str());
							}
						}
						DrawFront(button, maxSize, sizeDiv, onMinimap, cameraPos, mouseDir);
					}
					break;
				}
				case CMDTYPE_ICON_UNIT_OR_AREA:
				case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
				case CMDTYPE_ICON_AREA: {
					float maxRadius = 100000.0f;

					if (cmdDesc.params.size() == 1)
						maxRadius = atof(cmdDesc.params[0].c_str());

					if (mouse->buttons[button].movement > 4) {
						const float dist = CGround::LineGroundCol(
							mouse->buttons[button].camPos,
							mouse->buttons[button].camPos +
							mouse->buttons[button].dir * globalRendering->viewRange * 1.4f,
							false
						);

						if (dist < 0.0f)
							break;

						const float dist2 = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

						if (dist2 < 0.0f)
							break;

						const float3 pos = mouse->buttons[button].camPos + mouse->buttons[button].dir * dist;
						const float3 pos2 = cameraPos + mouseDir * dist2;

						const float* color;

						switch (cmdDesc.id) {
							case CMD_ATTACK:
							case CMD_AREA_ATTACK:  { color = cmdColors.attack;      break; }
							case CMD_REPAIR:       { color = cmdColors.repair;      break; }
							case CMD_RECLAIM:      { color = cmdColors.reclaim;     break; }
							case CMD_RESTORE:      { color = cmdColors.restore;     break; }
							case CMD_RESURRECT:    { color = cmdColors.resurrect;   break; }
							case CMD_LOAD_UNITS:   { color = cmdColors.load;        break; }
							case CMD_UNLOAD_UNIT:
							case CMD_UNLOAD_UNITS: { color = cmdColors.unload;      break; }
							case CMD_CAPTURE:      { color = cmdColors.capture;     break; }
							default: {
								static const float grey[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
								color = grey;
							}
						}

						const float radius = std::min(maxRadius, pos.distance2D(pos2));

						if (!onMinimap) {
							DrawArea(pos, radius, color);
						} else {
							glColor4f(color[0], color[1], color[2], 0.5f);
							glBegin(GL_TRIANGLE_FAN);
							const int divs = 256;
							for (int i = 0; i <= divs; ++i) {
								const float radians = (2 * PI) * (float)i / (float)divs;
								float3 p(pos.x, 0.0f, pos.z);
								p.x += fastmath::sin(radians) * radius;
								p.z += fastmath::cos(radians) * radius;
								glVertexf3(p);
							}
							glEnd();
						}
					}
					break;
				}
				case CMDTYPE_ICON_UNIT_OR_RECTANGLE:{
					if (mouse->buttons[button].movement >= 16) {
						const float dist1 = CGround::LineGroundCol(
							mouse->buttons[button].camPos,
							mouse->buttons[button].camPos +
							mouse->buttons[button].dir * globalRendering->viewRange * 1.4f,
							false
						);

						if (dist1 < 0.0f)
							break;

						const float dist2 = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

						if (dist2 < 0.0f)
							break;

						const float3 pos1 = mouse->buttons[button].camPos+mouse->buttons[button].dir * dist1;
						const float3 pos2 = cameraPos + mouseDir * dist2;

						if (!onMinimap) {
							DrawSelectBox(pos1, pos2, cameraPos);
						} else {
							glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
							glBegin(GL_QUADS);
							glVertex3f(pos1.x, 0.0f, pos1.z);
							glVertex3f(pos2.x, 0.0f, pos1.z);
							glVertex3f(pos2.x, 0.0f, pos2.z);
							glVertex3f(pos1.x, 0.0f, pos2.z);
							glEnd();
						}
					}
					break;
				}
			}
		}
	}

	if (!onMinimap) {
		glBlendFunc((GLenum)cmdColors.SelectedBlendSrc(),
								(GLenum)cmdColors.SelectedBlendDst());
		glLineWidth(cmdColors.SelectedLineWidth());
	} else {
		glLineWidth(1.49f);
	}

	// draw the ranges for the unit that is being pointed at
	const CUnit* pointedAt = NULL;

	if (GetQueueKeystate()) {
		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;

		if (minimapCoords) {
			unit = minimap->GetSelectUnit(cameraPos);
		} else {
			// ignore the returned distance, we don't care about it here
			TraceRay::GuiTraceRay(cameraPos, mouseDir, globalRendering->viewRange * 1.4f, NULL, unit, feature, false);
		}

		if (unit && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
			pointedAt = unit;
			const UnitDef* unitdef = unit->unitDef;
			const bool enemyUnit = ((unit->allyteam != gu->myAllyTeam) && !gu->spectatingFullView);
			if (enemyUnit && unitdef->decoyDef) {
				unitdef = unitdef->decoyDef;
			}

			DrawUnitDefRanges(unit, unitdef, unit->pos);

			// draw weapon range
			if (unitdef->maxWeaponRange > 0) {
				glDisable(GL_DEPTH_TEST);
				glColor4fv(cmdColors.rangeAttack);
				glBallisticCircle(unit->pos, unitdef->maxWeaponRange,
				                  unit->weapons[0], 40);
				glEnable(GL_DEPTH_TEST);
			}
			// draw decloak distance
			if (unit->decloakDistance > 0.0f) {
				glColor4fv(cmdColors.rangeDecloak);
				if (unit->unitDef->decloakSpherical && globalRendering->drawdebug) {
					glPushMatrix();
					glTranslatef(unit->midPos.x, unit->midPos.y, unit->midPos.z);
					glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
					GLUquadricObj* q = gluNewQuadric();
					gluQuadricDrawStyle(q, GLU_LINE);
					gluSphere(q, unit->decloakDistance, 10, 10);
					gluDeleteQuadric(q);
					glPopMatrix();
				} else { // cylindrical
					glSurfaceCircle(unit->pos, unit->decloakDistance, 40);
				}
			}
			// draw interceptor range
			if (unitdef->maxCoverage > 0.0f) {
				const CWeapon* w = NULL; //will be checked if any missiles are ready
				if (!enemyUnit) {
					w = unit->stockpileWeapon;
					if (w != NULL && !w->weaponDef->interceptor) {
						w = NULL; //if this isn't the interceptor, then don't use it
					}
				} //shows as on if enemy, a non-stockpiled weapon, or if the stockpile has a missile
				if (enemyUnit || (w == NULL) || w->numStockpiled) {
					glColor4fv(cmdColors.rangeInterceptorOn);
				} else {
					glColor4fv(cmdColors.rangeInterceptorOff);
				}
				glSurfaceCircle(unit->pos, unitdef->maxCoverage, 40);
			}
		}
	}

	// draw buildings we are about to build
	if ((inCommand >= 0) && ((size_t)inCommand < commands.size()) &&
	    (commands[inCommand].type == CMDTYPE_ICON_BUILDING)) {
		{ // limit the locking scope to avoid deadlock
			// draw build distance for all immobile builders during build commands
			for (const auto bi: unitHandler->GetBuilderCAIs()) {
				const CBuilderCAI* builderCAI = bi.second;
				const CUnit* builder = builderCAI->owner;
				const UnitDef* builderDef = builder->unitDef;

				if (builder == pointedAt || builder->team != gu->myTeam)
					continue;
				if (!builderDef->builder)
					continue;

				if (!builderDef->canmove || selectedUnitsHandler.IsUnitSelected(builder)) {
					const float radius = builderDef->buildDistance;
					if (radius > 0.0f) {
						glDisable(GL_TEXTURE_2D);
						const float* color = cmdColors.rangeBuild;
						glColor4f(color[0], color[1], color[2], color[3] * 0.333f);
						glSurfaceCircle(builder->pos, radius, 40);
					}
				}
			}
		}

		const float dist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

		if (dist > 0.0f) {
			const UnitDef* unitdef = unitDefHandler->GetUnitDefByID(-commands[inCommand].id);

			if (unitdef != nullptr) {
				const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];
				const float bpDist = CGround::LineGroundCol(bp.camPos, bp.camPos + bp.dir * globalRendering->viewRange * 1.4f, false);

				// get the build information
				const float3 cPos = cameraPos + mouseDir * dist;
				const float3 bPos = bp.camPos + bp.dir * bpDist;

				std::vector<BuildInfo> buildInfos;

				if (GetQueueKeystate() && bp.pressed) {
					const BuildInfo cInfo = BuildInfo(unitdef, cPos, buildFacing);
					const BuildInfo bInfo = BuildInfo(unitdef, bPos, buildFacing);

					buildInfos = std::move(GetBuildPos(bInfo, cInfo, cameraPos, mouseDir));
				} else {
					const BuildInfo bi(unitdef, cPos, buildFacing);

					buildInfos = std::move(GetBuildPos(bi, bi, cameraPos, mouseDir));
				}

				for (auto bpi = buildInfos.cbegin(); bpi != buildInfos.cend(); ++bpi) {
					const float3& buildPos = bpi->pos;

					DrawUnitDefRanges(nullptr, unitdef, buildPos);

					// draw weapon range
					if (!unitdef->weapons.empty()) {
						glDisable(GL_DEPTH_TEST);
						glColor4fv(cmdColors.rangeAttack);
						glBallisticCircle(buildPos, unitdef->weapons[0].def->range,
						                  NULL, 40, unitdef->weapons[0].def->heightmod);
						glEnable(GL_DEPTH_TEST);
					}
					// draw extraction range
					if (unitdef->extractRange > 0) {
						glColor4fv(cmdColors.rangeExtract);
						glSurfaceCircle(buildPos, unitdef->extractRange, 40);
					}
					// draw interceptor range
					const WeaponDef* wd = unitdef->stockpileWeaponDef;
					if ((wd != NULL) && wd->interceptor) {
						glColor4fv(cmdColors.rangeInterceptorOn);
						glSurfaceCircle(buildPos, wd->coverageRange, 40);
					}

					std::vector<Command> cv;

					if (GetQueueKeystate()) {
						const Command c = bpi->CreateCommand();
						for (const CUnit* su: selectedUnitsHandler.selectedUnits) {
							for (Command& cmd: su->commandAI->GetOverlapQueued(c)) {
								cv.push_back(cmd);
							}
						}
					}

					if (unitDrawer->ShowUnitBuildSquare(*bpi, cv)) {
						glColor4f(0.7f,1,1,0.4f);
					} else {
						glColor4f(1,0.5f,0.5f,0.4f);
					}

					if (!onMinimap) {
						glPushMatrix();
						glLoadIdentity();
						glTranslatef3(buildPos);
						glRotatef(bpi->buildFacing * 90.0f, 0.0f, 1.0f, 0.0f);

						CUnitDrawer::DrawIndividualDefAlpha(bpi->def, gu->myTeam, false);

						glPopMatrix();
						glBlendFunc((GLenum)cmdColors.SelectedBlendSrc(), (GLenum)cmdColors.SelectedBlendDst());
					}
				}
			}
		}
	}

	// draw range circles if attack orders are imminent
	int defcmd = GetDefaultCommand(mouse->lastx, mouse->lasty, cameraPos, mouseDir);
	if ((inCommand>=0 && (size_t)inCommand<commands.size() && commands[inCommand].id==CMD_ATTACK) ||
		(inCommand==-1 && defcmd>0 && commands[defcmd].id==CMD_ATTACK)
	) {
		for(CUnitSet::iterator si=selectedUnitsHandler.selectedUnits.begin(); si!=selectedUnitsHandler.selectedUnits.end(); ++si) {
			CUnit* unit = *si;
			if (unit == pointedAt) {
				continue;
			}
			if (onMinimap && (unit->unitDef->speed > 0.0f)) {
				continue;
			}
			if(unit->maxRange>0 && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
				glDisable(GL_DEPTH_TEST);
				glColor4fv(cmdColors.rangeAttack);
				glBallisticCircle(unit->pos, unit->maxRange,
				                  unit->weapons.front(), 40);
				glEnable(GL_DEPTH_TEST);
				if (!onMinimap && gs->cheatEnabled && globalRendering->drawdebug) {
					DrawWeaponArc(unit);
				}
			}
		}
	}

	glLineWidth(1.0f);

	if (!onMinimap) {
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}


void CGuiHandler::DrawMiniMapMarker(const float3& cameraPos)
{
	const float w = 10.0f;
	const float h = 30.0f;

	const float bc[4] = { 0.1f, 0.333f, 1.0f, 0.75f }; // base color
	const float d[8] = { 0.4f, 0.8f, 0.6f, 1.0f, 0.3f, 0.7f, 0.5f, 0.9f };
	const float colors[8][4] = {
		{ bc[0] * d[0],  bc[1] * d[0],  bc[2] * d[0],  bc[3] },
		{ bc[0] * d[1],  bc[1] * d[1],  bc[2] * d[1],  bc[3] },
		{ bc[0] * d[2],  bc[1] * d[2],  bc[2] * d[2],  bc[3] },
		{ bc[0] * d[3],  bc[1] * d[3],  bc[2] * d[3],  bc[3] },
		{ bc[0] * d[4],  bc[1] * d[4],  bc[2] * d[4],  bc[3] },
		{ bc[0] * d[5],  bc[1] * d[5],  bc[2] * d[5],  bc[3] },
		{ bc[0] * d[6],  bc[1] * d[6],  bc[2] * d[6],  bc[3] },
		{ bc[0] * d[7],  bc[1] * d[7],  bc[2] * d[7],  bc[3] },
	};

	const float groundLevel = CGround::GetHeightAboveWater(cameraPos.x, cameraPos.z, false);

	static float spinTime = 0.0f;
	spinTime = math::fmod(spinTime + globalRendering->lastFrameTime * 0.001f, 60.0f);

	glPushMatrix();
	glTranslatef(cameraPos.x, groundLevel, cameraPos.z);
	glRotatef(360.0f * (spinTime / 2.0f), 0.0f, 1.0f, 0.0f);

	glEnable(GL_BLEND);
	glShadeModel(GL_FLAT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBegin(GL_TRIANGLE_FAN);
		                       glVertex3f(0.0f, 0.0f, 0.0f);
		                       glVertex3f(  +w,   +h, 0.0f);
		glColor4fv(colors[4]); glVertex3f(0.0f,   +h,   +w);
		glColor4fv(colors[5]); glVertex3f(  -w,   +h, 0.0f);
		glColor4fv(colors[6]); glVertex3f(0.0f,   +h,   -w);
		glColor4fv(colors[7]); glVertex3f(  +w,   +h, 0.0f);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
		                       glVertex3f(0.0f, h * 2.0f, 0.0f);
		                       glVertex3f(  +w,   +h, 0.0f);
		glColor4fv(colors[3]); glVertex3f(0.0f,   +h,   -w);
		glColor4fv(colors[2]); glVertex3f(  -w,   +h, 0.0f);
		glColor4fv(colors[1]); glVertex3f(0.0f,   +h,   +w);
		glColor4fv(colors[0]); glVertex3f(  +w,   +h, 0.0f);
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);
	glPopMatrix();
}


void CGuiHandler::DrawCentroidCursor()
{
	int cmd = -1;
	if ((inCommand >= 0) && ((size_t)inCommand < commands.size())) {
		cmd = commands[inCommand].id;
	} else {
		int defcmd;
		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed &&
				((activeReceiver == this) || (minimap->ProxyMode()))) {
			defcmd = defaultCmdMemory;
		} else {
			defcmd = GetDefaultCommand(mouse->lastx, mouse->lasty);
		}
		if ((defcmd >= 0) && ((size_t)defcmd < commands.size())) {
			cmd = commands[defcmd].id;
		}
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

	const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
	if (selUnits.size() < 2) {
		return;
	}

	float3 pos;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		pos += (*it)->midPos;
	}
	pos /= (float)selUnits.size();
	const float3 winPos = camera->CalcWindowCoordinates(pos);
	if (winPos.z <= 1.0f) {
		const CMouseCursor* mc = mouse->FindCursor("Centroid");
		if (mc != NULL) {
			glDisable(GL_DEPTH_TEST);
			mc->Draw((int)winPos.x, globalRendering->viewSizeY - (int)winPos.y, 1.0f);
			glEnable(GL_DEPTH_TEST);
		}
	}
}


void CGuiHandler::DrawArea(float3 pos, float radius, const float* color)
{
	if (useStencil) {
		DrawSelectCircle(pos, radius, color);
		return;
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(color[0], color[1], color[2], 0.25f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glBegin(GL_TRIANGLE_FAN);
		glVertexf3(pos);
		for(int a=0;a<=40;++a){
			float3 p(fastmath::cos(a*2*PI/40)*radius,0,fastmath::sin(a*2*PI/40)*radius);
			p+=pos;
			p.y=CGround::GetHeightAboveWater(p.x, p.z, false);
			glVertexf3(p);
		}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
}


void CGuiHandler::DrawFront(int button, float maxSize, float sizeDiv, bool onMinimap, const float3& cameraPos, const float3& mouseDir)
{
	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	if (bp.movement < 5)
		return;

	float dist = CGround::LineGroundCol(bp.camPos, bp.camPos + bp.dir * globalRendering->viewRange * 1.4f, false);

	if (dist < 0.0f)
		return;

	dist = CGround::LineGroundCol(cameraPos, cameraPos + mouseDir * globalRendering->viewRange * 1.4f, false);

	if (dist < 0.0f)
		return;

	float3 pos1 = bp.camPos + (  bp.dir * dist);
	float3 pos2 = cameraPos + (mouseDir * dist);

	ProcessFrontPositions(pos1, pos2);

	const float3 forward = ((pos1 - pos2).cross(UpVector)).ANormalize();
	const float3 side = forward.cross(UpVector);

	if (pos1.SqDistance2D(pos2) > maxSize * maxSize) {
		pos2 = pos1 + side * maxSize;
		pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false);
	}

	glColor4f(0.5f, 1.0f, 0.5f, 0.5f);

	if (onMinimap) {
		pos1 += (pos1 - pos2);
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		glVertexf3(pos1);
		glVertexf3(pos2);
		glEnd();
		return;
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
		glVertexf3(pos1+side*25);
		glVertexf3(pos1-side*25);
		glVertexf3(pos1-side*25+forward*50);
		glVertexf3(pos1+side*25+forward*50);

		glVertexf3(pos1+side*40+forward*50);
		glVertexf3(pos1-side*40+forward*50);
		glVertexf3(pos1+forward*100);
		glVertexf3(pos1+forward*100);
	glEnd();
	glEnable(GL_DEPTH_TEST);

	pos1 += (pos1 - pos2);
	const int maxSteps = 256;
	const float frontLen = (pos1 - pos2).Length2D();
	const int steps = std::min(maxSteps, std::max(1, int(frontLen / 16.0f)));

	glDisable(GL_FOG);
	glBegin(GL_QUAD_STRIP);
	const float3 delta = (pos2 - pos1) / (float)steps;
	for (int i = 0; i <= steps; i++) {
		float3 p;
		const float d = (float)i;
		p.x = pos1.x + (d * delta.x);
		p.z = pos1.z + (d * delta.z);
		p.y = CGround::GetHeightAboveWater(p.x, p.z, false);
		p.y -= 100.f; glVertexf3(p);
		p.y += 200.f; glVertexf3(p);
	}
	glEnd();

	glEnable(GL_FOG);
}


/******************************************************************************/
/******************************************************************************/

struct BoxData {
	float3 mins;
	float3 maxs;
};


static void DrawBoxShape(const void* data)
{
	const BoxData* boxData = static_cast<const BoxData*>(data);
	const float3& mins = boxData->mins;
	const float3& maxs = boxData->maxs;
	glBegin(GL_QUADS);
		// the top
		glVertex3f(mins.x, maxs.y, mins.z);
		glVertex3f(mins.x, maxs.y, maxs.z);
		glVertex3f(maxs.x, maxs.y, maxs.z);
		glVertex3f(maxs.x, maxs.y, mins.z);
		// the bottom
		glVertex3f(mins.x, mins.y, mins.z);
		glVertex3f(maxs.x, mins.y, mins.z);
		glVertex3f(maxs.x, mins.y, maxs.z);
		glVertex3f(mins.x, mins.y, maxs.z);
	glEnd();
	glBegin(GL_QUAD_STRIP);
		// the sides
		glVertex3f(mins.x, maxs.y, mins.z);
		glVertex3f(mins.x, mins.y, mins.z);
		glVertex3f(mins.x, maxs.y, maxs.z);
		glVertex3f(mins.x, mins.y, maxs.z);
		glVertex3f(maxs.x, maxs.y, maxs.z);
		glVertex3f(maxs.x, mins.y, maxs.z);
		glVertex3f(maxs.x, maxs.y, mins.z);
		glVertex3f(maxs.x, mins.y, mins.z);
		glVertex3f(mins.x, maxs.y, mins.z);
		glVertex3f(mins.x, mins.y, mins.z);
	glEnd();
}


static void DrawCornerPosts(const float3& pos0, const float3& pos1)
{
	const float3 lineVector(0.0f, 128.0f, 0.0f);
	const float3 corner0(pos0.x, CGround::GetHeightAboveWater(pos0.x, pos0.z, false), pos0.z);
	const float3 corner1(pos1.x, CGround::GetHeightAboveWater(pos1.x, pos1.z, false), pos1.z);
	const float3 corner2(pos0.x, CGround::GetHeightAboveWater(pos0.x, pos1.z, false), pos1.z);
	const float3 corner3(pos1.x, CGround::GetHeightAboveWater(pos1.x, pos0.z, false), pos0.z);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
		glColor4f(1.0f, 1.0f, 0.0f, 0.9f);
		glVertexf3(corner0); glVertexf3(corner0 + lineVector);
		glColor4f(0.0f, 1.0f, 0.0f, 0.9f);
		glVertexf3(corner1); glVertexf3(corner1 + lineVector);
		glColor4f(0.0f, 0.0f, 1.0f, 0.9f);
		glVertexf3(corner2); glVertexf3(corner2 + lineVector);
		glVertexf3(corner3); glVertexf3(corner3 + lineVector);
	glEnd();
	glLineWidth(1.0f);
}


static void StencilDrawSelectBox(const float3& pos0, const float3& pos1,
		bool invColorSelect)
{
	BoxData boxData;
	boxData.mins = float3(std::min(pos0.x, pos1.x), readMap->GetCurrMinHeight() -   250.0f, std::min(pos0.z, pos1.z));
	boxData.maxs = float3(std::max(pos0.x, pos1.x), readMap->GetCurrMaxHeight() + 10000.0f, std::max(pos0.z, pos1.z));

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);

	glEnable(GL_BLEND);

	if (!invColorSelect) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(1.0f, 0.0f, 0.0f, 0.25f);
		glDrawVolume(DrawBoxShape, &boxData);
	} else {
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_INVERT);
		glDrawVolume(DrawBoxShape, &boxData);
		glDisable(GL_COLOR_LOGIC_OP);
	}

	DrawCornerPosts(pos0, pos1);

	glEnable(GL_FOG);
}


static void FullScreenDraw()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glRectf(-1.0f, -1.0f, +1.0f, +1.0f);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


static void DrawMinMaxBox(const float3& mins, const float3& maxs)
{
	glBegin(GL_QUADS);
		// the top
		glVertex3f(mins.x, maxs.y, mins.z);
		glVertex3f(maxs.x, maxs.y, mins.z);
		glVertex3f(maxs.x, maxs.y, maxs.z);
		glVertex3f(mins.x, maxs.y, maxs.z);
	glEnd();
	glBegin(GL_QUAD_STRIP);
		// the sides
		glVertex3f(mins.x, mins.y, mins.z);
		glVertex3f(mins.x, maxs.y, mins.z);
		glVertex3f(mins.x, mins.y, maxs.z);
		glVertex3f(mins.x, maxs.y, maxs.z);
		glVertex3f(maxs.x, mins.y, maxs.z);
		glVertex3f(maxs.x, maxs.y, maxs.z);
		glVertex3f(maxs.x, mins.y, mins.z);
		glVertex3f(maxs.x, maxs.y, mins.z);
		glVertex3f(mins.x, mins.y, mins.z);
		glVertex3f(mins.x, maxs.y, mins.z);
	glEnd();
}


void CGuiHandler::DrawSelectBox(const float3& pos0, const float3& pos1, const float3& cameraPos)
{
	if (useStencil) {
		StencilDrawSelectBox(pos0, pos1, invColorSelect);
		return;
	}

	const float3 mins(std::min(pos0.x, pos1.x), readMap->GetCurrMinHeight() -   250.0f, std::min(pos0.z, pos1.z));
	const float3 maxs(std::max(pos0.x, pos1.x), readMap->GetCurrMaxHeight() + 10000.0f, std::max(pos0.z, pos1.z));

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);

	glDepthMask(GL_FALSE);
	glEnable(GL_CULL_FACE);

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);

	// invert the color for objects within the box
	glCullFace(GL_FRONT); DrawMinMaxBox(mins, maxs);
	glCullFace(GL_BACK);  DrawMinMaxBox(mins, maxs);

	glDisable(GL_CULL_FACE);

	// do a full screen inversion if the camera is within the box
	if ((cameraPos.x > mins.x) && (cameraPos.x < maxs.x) &&
	    (cameraPos.y > mins.y) && (cameraPos.y < maxs.y) &&
	    (cameraPos.z > mins.z) && (cameraPos.z < maxs.z)) {
		glDisable(GL_DEPTH_TEST);
		FullScreenDraw();
		glEnable(GL_DEPTH_TEST);
	}

	glDisable(GL_COLOR_LOGIC_OP);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	DrawCornerPosts(pos0, pos1);

//	glDepthMask(GL_TRUE);
	glEnable(GL_FOG);
}


/******************************************************************************/

struct CylinderData {
	int divs;
	float xc, zc, yp, yn;
	float radius;
};


static void DrawCylinderShape(const void* data)
{
	const CylinderData& cyl = *static_cast<const CylinderData*>(data);
	const float step = fastmath::PI2 / (float)cyl.divs;
	int i;
	glBegin(GL_QUAD_STRIP); // the sides
	for (i = 0; i <= cyl.divs; i++) {
		const float radians = step * float(i % cyl.divs);
		const float x = cyl.xc + (cyl.radius * fastmath::sin(radians));
		const float z = cyl.zc + (cyl.radius * fastmath::cos(radians));
		glVertex3f(x, cyl.yp, z);
		glVertex3f(x, cyl.yn, z);
	}
	glEnd();
	glBegin(GL_TRIANGLE_FAN); // the top
	for (i = 0; i < cyl.divs; i++) {
		const float radians = step * float(i);
		const float x = cyl.xc + (cyl.radius * fastmath::sin(radians));
		const float z = cyl.zc + (cyl.radius * fastmath::cos(radians));
		glVertex3f(x, cyl.yp, z);
	}
	glEnd();
	glBegin(GL_TRIANGLE_FAN); // the bottom
	for (i = (cyl.divs - 1); i >= 0; i--) {
		const float radians = step * float(i);
		const float x = cyl.xc + (cyl.radius * fastmath::sin(radians));
		const float z = cyl.zc + (cyl.radius * fastmath::cos(radians));
		glVertex3f(x, cyl.yn, z);
	}
	glEnd();
}


void CGuiHandler::DrawSelectCircle(const float3& pos, float radius,
                                   const float* color)
{
	CylinderData cylData;
	cylData.xc = pos.x;
	cylData.zc = pos.z;
	cylData.yp = readMap->GetCurrMaxHeight() + 10000.0f;
	cylData.yn = readMap->GetCurrMinHeight() -   250.0f;
	cylData.radius = radius;
	cylData.divs = 128;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(color[0], color[1], color[2], 0.25f);

	glDrawVolume(DrawCylinderShape, &cylData);

	// draw the center line
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(color[0], color[1], color[2], 0.9f);
	glLineWidth(2.0f);
	const float3 base(pos.x, CGround::GetHeightAboveWater(pos.x, pos.z, false), pos.z);
	glBegin(GL_LINES);
		glVertexf3(base);
		glVertexf3(base + float3(0.0f, 128.0f, 0.0f));
	glEnd();
	glLineWidth(1.0f);

	glEnable(GL_FOG);
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::SetBuildFacing(unsigned int facing)
{
	buildFacing = facing % NUM_FACINGS;
}

void CGuiHandler::SetBuildSpacing(int spacing)
{
	buildSpacing = spacing;
	if (buildSpacing < 0)
		buildSpacing = 0;
}

/******************************************************************************/
/******************************************************************************/
