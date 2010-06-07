/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <map>
#include <set>
#include <list>

#include "mmgr.h"

#include "GuiHandler.h"
#include "SDL_keysym.h"
#include "SDL_mouse.h"
#include "CommandColors.h"
#include "KeyBindings.h"
#include "KeyCodes.h"
#include "LuaUI.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Lua/LuaTextures.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/glFont.h"
#include "Rendering/IconHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/glExtra.h"
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
#include "Sound/AudioChannel.h"
#include "Sound/ISound.h"
#include "EventHandler.h"
#include "FileSystem/SimpleParser.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Util.h"
#include "myMath.h"

extern boost::uint8_t *keys;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CGuiHandler* guihandler = NULL;


CGuiHandler::CGuiHandler():
	inCommand(-1),
	buildFacing(0),
	buildSpacing(0),
	needShift(false),
	showingMetal(false),
	activeMousePress(false),
	forceLayoutUpdate(false),
	maxPage(0),
	activePage(0),
	defaultCmdMemory(-1),
	explicitCommand(-1),
	actionOffset(0),
	drawSelectionInfo(true),
	gatherMode(false)
{
	icons = new IconInfo[16];
	iconsSize = 16;
	iconsCount = 0;

	LoadConfig("ctrlpanel.txt");

	miniMapMarker = !!configHandler->Get("MiniMapMarker", 1);
	invertQueueKey = !!configHandler->Get("InvertQueueKey", 0);

	autoShowMetal = mapInfo->gui.autoShowMetal;

	useStencil = false;
	if (GLEW_NV_depth_clamp && !!configHandler->Get("StencilBufferBits", 1)) {
		GLint stencilBits;
		glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
		useStencil = (stencilBits >= 1);
	}

	failedSound = sound->GetSoundId("FailedCommand", false);
}


CGuiHandler::~CGuiHandler()
{
	CLuaUI::FreeHandler();

	delete[] icons;

	std::map<std::string, unsigned int>::iterator it;
	for (it = textureMap.begin(); it != textureMap.end(); ++it) {
		const GLuint texID = it->second;
		glDeleteTextures (1, &texID);
	}
}


bool CGuiHandler::GetQueueKeystate() const
{
	return (!invertQueueKey && keys[SDLK_LSHIFT]) ||
	       (invertQueueKey && !keys[SDLK_LSHIFT]);
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
	if (endPtr == startPtr) {
		return false;
	}
	var = tmp;
	return true;
}


bool CGuiHandler::LoadConfig(const std::string& filename)
{
	LoadDefaults();

	CFileHandler ifs(filename);
	CSimpleParser parser(ifs);

	std::string deadStr = "";
	std::string prevStr = "";
	std::string nextStr = "";
	std::string fillOrderStr = "";

	while (true) {
		const std::string line = parser.GetCleanLine();
		if (line.empty()) {
			break;
		}

		std::vector<std::string> words = parser.Tokenize(line, 1);

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
	}	else {
		deadIconSlot = -1;
	}

	if (!prevStr.empty() && (prevStr != "auto")) {
		if (prevStr == "none") {
			prevPageSlot = -1;
		} else {
			prevPageSlot = ParseIconSlot(prevStr);
		}
	}	else {
		prevPageSlot = iconsPerPage - 2;
	}

	if (!nextStr.empty() && (nextStr != "auto")) {
		if (nextStr == "none") {
			nextPageSlot = -1;
		} else {
			nextPageSlot = ParseIconSlot(nextStr);
		}
	}	else {
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
		}
		else {
			xBpos = yBpos = -1.0f; // off screen
		}
	}
	else {
		xBpos = yBpos = -1.0f; // off screen
	}

	ParseFillOrder(fillOrderStr);

	return true;
}


void CGuiHandler::ParseFillOrder(const std::string& text)
{
	// setup the default order
	fillOrder.clear();
	for (int i = 0; i < iconsPerPage; i++) {
		fillOrder.push_back(i);
	}

	// split the std::string into slot names
	std::vector<std::string> slotNames = CSimpleParser::Tokenize(text, 0);
	if ((int)slotNames.size() != iconsPerPage) {
		return;
	}

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
	if ((rowLetter < 'a') || (rowLetter > 'z')) {
		return -1;
	}
	const int row = rowLetter - 'a';
	if (row >= yIcons) {
		return -1;
	}

	char* endPtr;
	const char* startPtr = text.c_str() + 1;
	int column = strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (column < 0) || (column >= xIcons)) {
		return -1;
	}

	return (row * xIcons) + column;
}


bool CGuiHandler::ReloadConfig(const std::string& filename)
{
	LoadConfig(filename);
	activePage = 0;
	selectedUnits.SetCommandPage(activePage);
	LayoutIcons(false);
	return true;
}


void CGuiHandler::ResizeIconArray(unsigned int size)
{
	unsigned int minIconsSize = iconsSize;
	while (minIconsSize < size) {
		minIconsSize *= 2;
	}
	if (iconsSize < minIconsSize) {
		iconsSize = minIconsSize;
		delete[] icons;
		icons = new IconInfo[iconsSize];
	}
}


void CGuiHandler::AppendPrevAndNext(std::vector<CommandDescription>& cmds)
{
	CommandDescription cd;

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
	if ((inCommand < 0) || ((size_t)inCommand >= commands.size())) {
		return -1;
	}
	for (int ii = 0; ii < iconsCount; ii++) {
		const IconInfo& icon = icons[ii];
		if (icon.commandsID == inCommand) {
			return (ii / iconsPerPage);
		}
	}
	return -1;
}


void CGuiHandler::RevertToCmdDesc(const CommandDescription& cmdDesc,
                                  bool defaultCommand, bool samePage)
{
	for (size_t a = 0; a < commands.size(); ++a) {
		if (commands[a].id == cmdDesc.id) {
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
						selectedUnits.SetCommandPage(activePage);
					}
				}
			}
			return;
		}
	}
}


void CGuiHandler::LayoutIcons(bool useSelectionPage)
{
	GML_RECMUTEX_LOCK(gui); // LayoutIcons - updates inCommand. Called from CGame::Draw --> RunLayoutCommand

	const bool defCmd =
		(mouse->buttons[SDL_BUTTON_RIGHT].pressed &&
		 (defaultCmdMemory >= 0) && (inCommand < 0) &&
		 ((activeReceiver == this) || (minimap->ProxyMode())));

	const int activeCmd = defCmd ? defaultCmdMemory : inCommand;

	// copy the current command state
	const bool validInCommand = (activeCmd >= 0) && ((size_t)activeCmd < commands.size());
	CommandDescription cmdDesc;
	if (validInCommand) {
		cmdDesc = commands[activeCmd];
	}
	const bool samePage = validInCommand && (activePage == FindInCommandPage());
	useSelectionPage = useSelectionPage && !samePage;

	// reset some of our state
	inCommand = -1;
	defaultCmdMemory = -1;
	commands.clear();
	forceLayoutUpdate = false;

	if ((luaUI != NULL) && luaUI->HasLayoutButtons()) {
		if (LayoutCustomIcons(useSelectionPage)) {
			if (validInCommand) {
				RevertToCmdDesc(cmdDesc, defCmd, samePage);
			}
			return; // we're done here
		}
		else {
			CLuaUI::FreeHandler();
			LoadConfig("ctrlpanel.txt");
		}
	}

	// get the commands to process
	CSelectedUnits::AvailableCommandsStruct ac;
	ac = selectedUnits.GetAvailableCommands();
	ConvertCommands(ac.commands);

	std::vector<CommandDescription> hidden;
	std::vector<CommandDescription>::const_iterator cdi;

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
	if (luaUI == NULL) {
		return false;
	}

	// get the commands to process
	CSelectedUnits::AvailableCommandsStruct ac;
	ac = selectedUnits.GetAvailableCommands();
	std::vector<CommandDescription> cmds = ac.commands;
	if (cmds.size() > 0) {
		ConvertCommands(cmds);
		AppendPrevAndNext(cmds);
	}

	// call for a custom layout
	int tmpXicons = xIcons;
	int tmpYicons = yIcons;
	std::vector<int> removeCmds;
	std::vector<CommandDescription> customCmds;
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
		logOutput.Print("LayoutCustomIcons() bad xIcons or yIcons (%i, %i)\n",
		                tmpXicons, tmpYicons);
		return false;
	}

	unsigned int i;
	const int tmpIconsPerPage = (tmpXicons * tmpYicons);

	// build a set to better find unwanted commands
	set<int> removeIDs;
	for (i = 0; i < removeCmds.size(); i++) {
		const int index = removeCmds[i];
		if ((index >= 0) || ((size_t)index < cmds.size())) {
			removeIDs.insert(index);
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad removeCmd (%i)\n",
			                index);
		}
	}
	// remove unwanted commands  (and mark all as hidden)
	std::vector<CommandDescription> tmpCmds;
	for (i = 0; i < cmds.size(); i++) {
		if (removeIDs.find(i) == removeIDs.end()) {
			cmds[i].hidden = true;
			tmpCmds.push_back(cmds[i]);
		}
	}
	cmds = tmpCmds;

	// add the custom commands
	for (i = 0; i < customCmds.size(); i++) {
		customCmds[i].hidden = true;
		cmds.push_back(customCmds[i]);
	}
	const int cmdCount = (int)cmds.size();

	// set commands to onlyTexture
	for (i = 0; i < onlyTextureCmds.size(); i++) {
		const int index = onlyTextureCmds[i];
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].onlyTexture = true;
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad onlyTexture (%i)\n",
			                index);
		}
	}

	// retexture commands
	for (i = 0; i < reTextureCmds.size(); i++) {
		const int index = reTextureCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].iconname = reTextureCmds[i].texture;
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad reTexture (%i)\n",
			                index);
		}
	}

	// reNamed commands
	for (i = 0; i < reNamedCmds.size(); i++) {
		const int index = reNamedCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].name = reNamedCmds[i].texture;
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad reNamed (%i)\n",
			                index);
		}
	}

	// reTooltip commands
	for (i = 0; i < reTooltipCmds.size(); i++) {
		const int index = reTooltipCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			cmds[index].tooltip = reTooltipCmds[i].texture;
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad reNamed (%i)\n",
			                index);
		}
	}

	// reParams commands
	for (i = 0; i < reParamsCmds.size(); i++) {
		const int index = reParamsCmds[i].cmdIndex;
		if ((index >= 0) && (index < cmdCount)) {
			const map<int, std::string>& params = reParamsCmds[i].params;
			map<int, std::string>::const_iterator pit;
			for (pit = params.begin(); pit != params.end(); ++pit) {
				const int p = pit->first;
				if ((p >= 0) && (p < (int)cmds[index].params.size())) {
					cmds[index].params[p] = pit->second;
				}
			}
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad reParams (%i)\n",
			                index);
		}
	}

	// build the iconList from the map
	std::vector<int> iconList;
	int nextPos = 0;
	map<int, int>::iterator mit;
	for (mit = iconMap.begin(); mit != iconMap.end(); ++mit) {
		const int iconPos = mit->first;
		if (iconPos < nextPos) {
			continue;
		}
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
		}
		else {
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


void CGuiHandler::GiveCommand(const Command& cmd, bool fromUser) const
{
	if (eventHandler.CommandNotify(cmd)) {
		return;
	}

	selectedUnits.GiveCommand(cmd, fromUser);

	if (gatherMode) {
		if ((cmd.id == CMD_MOVE) || (cmd.id == CMD_FIGHT)) {
			Command gatherCmd;
			gatherCmd.id = CMD_GATHERWAIT;
			GiveCommand(gatherCmd, false);
		}
	}
}


void CGuiHandler::ConvertCommands(std::vector<CommandDescription>& cmds)
{
	if (newAttackMode) {
		const int count = (int)cmds.size();
		for (int i = 0; i < count; i++) {
			CommandDescription& cd = cmds[i];
			if ((cd.id == CMD_ATTACK) && (cd.type == CMDTYPE_ICON_UNIT_OR_MAP)) {
				if (attackRect) {
					cd.type = CMDTYPE_ICON_UNIT_OR_RECTANGLE;
				} else {
					cd.type = CMDTYPE_ICON_UNIT_OR_AREA;
				}
			}
		}
	}
}


void CGuiHandler::SetShowingMetal(bool show)
{
	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	if (!show) {
		if (showingMetal) {
			gd->DisableExtraTexture();
			showingMetal = false;
		}
	}
	else {
		if (autoShowMetal) {
			if (gd->drawMode != CBaseGroundDrawer::drawMetal) {
				CMetalMap* mm = readmap->metalMap;
				gd->SetMetalTexture(mm->metalMap, &mm->extractionMap.front(), mm->metalPal, false);
				showingMetal = true;
			}
		}
	}
}


void CGuiHandler::Update()
{
	GML_RECMUTEX_LOCK(gui); // Update - updates inCommand

	SetCursorIcon();

	// Notify LuaUI about groups that have changed
	if (!changedGroups.empty()) {
		set<int>::const_iterator it;
		for (it = changedGroups.begin(); it != changedGroups.end(); ++it) {
			eventHandler.GroupChanged(*it);
		}
		changedGroups.clear();
	}

	if (!invertQueueKey && (needShift && !keys[SDLK_LSHIFT])) {
		SetShowingMetal(false);
		inCommand=-1;
		needShift=false;
	}

	const bool commandsChanged = selectedUnits.CommandsChanged();

	if (commandsChanged) {
		SetShowingMetal(false);
		LayoutIcons(true);
		//fadein = 100;
	}
	else if (forceLayoutUpdate) {
		LayoutIcons(false);
	}

	/*if (fadein > 0) {
		fadein -= 5;
	}*/
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::SetCursorIcon() const
{
	string newCursor = "cursornormal";
	mouse->cursorScale = 1.0f;

	CInputReceiver* ir = NULL;
	if (!game->hideInterface)
		ir = GetReceiverAt(mouse->lastx, mouse->lasty);

	if ((ir != NULL) && (ir != minimap)) {
		mouse->SetCursor(newCursor);
		return;
	}

	if (ir == minimap)
		mouse->cursorScale = minimap->CursorScale();

	const bool useMinimap = (minimap->ProxyMode() || ((activeReceiver != this) && (ir == minimap)));

	if ((inCommand >= 0) && ((size_t)inCommand<commands.size())) {
		const CommandDescription& cmdDesc = commands[inCommand];

		if (!cmdDesc.mouseicon.empty()) {
			newCursor=cmdDesc.mouseicon;
		} else {
			newCursor=cmdDesc.name;
		}

		if (useMinimap && (cmdDesc.id < 0)) {
			BuildInfo bi;
			bi.pos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
			bi.buildFacing = bi.buildFacing;
			bi.def = unitDefHandler->GetUnitDefByID(-cmdDesc.id);
			bi.pos = helper->Pos2BuildPos(bi);
			// if an unit (enemy), is not in LOS, then TestUnitBuildSquare()
			// does not consider it when checking for position blocking
			CFeature* feature;
			if(!uh->TestUnitBuildSquare(bi, feature, gu->myAllyTeam)) {
				newCursor="BuildBad";
			} else {
				newCursor="BuildGood";
			}
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
			const CommandDescription& cmdDesc = commands[defcmd];
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

	mouse->SetCursor(newCursor);
}


bool CGuiHandler::MousePress(int x, int y, int button)
{
	GML_RECMUTEX_LOCK(gui); // MousePress - updates inCommand

	if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT && button != -SDL_BUTTON_RIGHT && button != -SDL_BUTTON_LEFT)
		return false;

	if (button < 0) {
		// proxied click from the minimap
		button = -button;
		activeMousePress=true;
	}
	else if (AboveGui(x,y)) {
		activeMousePress = true;
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

	if (button == SDL_BUTTON_RIGHT) {
		activeMousePress = true;
		defaultCmdMemory = GetDefaultCommand(x, y);
		return true;
	}

	return false;
}


void CGuiHandler::MouseRelease(int x, int y, int button, float3& camerapos, float3& mousedir)
{
	if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT && button != -SDL_BUTTON_RIGHT && button != -SDL_BUTTON_LEFT)
		return;

	int iconCmd = -1;
	explicitCommand = inCommand;

	if (activeMousePress) {
		activeMousePress = false;
	} else {
		return;
	}

	if (!invertQueueKey && needShift && !keys[SDLK_LSHIFT]) {
		SetShowingMetal(false);
		inCommand = -1;
		needShift = false;
	}

	if (button < 0) {
		button = -button; // proxied click from the minimap
	} else {
		// setup iconCmd
		if ((iconCmd < 0) && !game->hideInterface) {
			const int iconPos = IconAtPos(x, y);
			if (iconPos >= 0) {
				iconCmd = icons[iconPos].commandsID;
			}
		}
	}

	if ((button == SDL_BUTTON_RIGHT) && (iconCmd == -1)) {
		// right click -> set the default cmd
		inCommand = defaultCmdMemory;
		defaultCmdMemory = -1;
	}

	if ((iconCmd >= 0) && ((size_t)iconCmd < commands.size())) {
		const bool rmb = (button == SDL_BUTTON_RIGHT);
		SetActiveCommand(iconCmd, rmb);
		return;
	}

	// not over a button, try to execute a command
	Command c = GetCommand(x, y, button, false, camerapos, mousedir);

	if (c.id == CMD_FAILED) { // indicates we should not finish the current command
		Channels::UserInterface.PlaySample(failedSound, 5);
		return;
	}
	// if cmd_stop is returned it indicates that no good command could be found
	if (c.id != CMD_STOP) {
		GiveCommand(c);
		lastKeySet.Reset();
	}

	FinishCommand(button);
}


bool CGuiHandler::SetActiveCommand(int cmdIndex, bool rmb)
{
	GML_RECMUTEX_LOCK(gui); // SetActiveCommand - updates inCommand

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

	CommandDescription& cd = commands[cmdIndex];
	if (cd.disabled) {
		return false;
	}

	lastKeySet.Reset();

	switch (cd.type) {
		case CMDTYPE_ICON: {
			Command c;
			c.id = cd.id;
			if (c.id != CMD_STOP) {
				CreateOptions(c, rmb);
				if (invertQueueKey && ((c.id < 0) || (c.id == CMD_STOCKPILE))) {
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

			Command c;
			c.id = cd.id;
			c.params.push_back(newMode);
			CreateOptions(c, rmb);
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
			selectedUnits.SetCommandPage(activePage);
			break;
		}
		case CMDTYPE_PREV: {
			--activePage;
			if (activePage < 0) {
				activePage=maxPage;
			}
			selectedUnits.SetCommandPage(activePage);
			break;
		}
		case CMDTYPE_CUSTOM: {
			RunCustomCommands(cd.params, rmb);
			break;
		}
		default:
			break;
	}

	return true;
}


bool CGuiHandler::SetActiveCommand(int cmdIndex, int button,
                                   bool lmb, bool rmb,
                                   bool alt, bool ctrl, bool meta, bool shift)
{
	// use the button value instead of rmb
	const bool effectiveRMB = (button == SDL_BUTTON_LEFT) ? false : true;

	// setup the mouse and key states
	const bool  prevLMB   = mouse->buttons[SDL_BUTTON_LEFT].pressed;
	const bool  prevRMB   = mouse->buttons[SDL_BUTTON_RIGHT].pressed;
	const boost::uint8_t prevAlt   = keys[SDLK_LALT];
	const boost::uint8_t prevCtrl  = keys[SDLK_LCTRL];
	const boost::uint8_t prevMeta  = keys[SDLK_LMETA];
	const boost::uint8_t prevShift = keys[SDLK_LSHIFT];

	mouse->buttons[SDL_BUTTON_LEFT].pressed  = lmb;
	mouse->buttons[SDL_BUTTON_RIGHT].pressed = rmb;
	keys[SDLK_LALT]   = alt;
	keys[SDLK_LCTRL]  = ctrl;
	keys[SDLK_LMETA]  = meta;
	keys[SDLK_LSHIFT] = shift;

	const bool retval = SetActiveCommand(cmdIndex, effectiveRMB);

	// revert the mouse and key states
	keys[SDLK_LSHIFT] = prevShift;
	keys[SDLK_LMETA]  = prevMeta;
	keys[SDLK_LCTRL]  = prevCtrl;
	keys[SDLK_LALT]   = prevAlt;
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


static bool CheckCustomCmdMods(bool rmb, ModGroup& inMods)
{
	if (((inMods.alt   == Required)  && !keys[SDLK_LALT])   ||
	    ((inMods.alt   == Forbidden) &&  keys[SDLK_LALT])   ||
	    ((inMods.ctrl  == Required)  && !keys[SDLK_LCTRL])  ||
	    ((inMods.ctrl  == Forbidden) &&  keys[SDLK_LCTRL])  ||
	    ((inMods.meta  == Required)  && !keys[SDLK_LMETA])  ||
	    ((inMods.meta  == Forbidden) &&  keys[SDLK_LMETA])  ||
	    ((inMods.shift == Required)  && !keys[SDLK_LSHIFT]) ||
	    ((inMods.shift == Forbidden) &&  keys[SDLK_LSHIFT]) ||
	    ((inMods.right == Required)  && !rmb)               ||
	    ((inMods.right == Forbidden) &&  rmb)) {
		return false;
	}
	return true;
}


void CGuiHandler::RunCustomCommands(const std::vector<std::string>& cmds, bool rmb)
{
	GML_RECMUTEX_LOCK(gui); // RunCustomCommands - called from LuaUnsyncedCtrl::SendCommands

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
			if (CheckCustomCmdMods(rmb, inMods)) {
				const bool tmpAlt   = !!keys[SDLK_LALT];
				const bool tmpCtrl  = !!keys[SDLK_LCTRL];
				const bool tmpMeta  = !!keys[SDLK_LMETA];
				const bool tmpShift = !!keys[SDLK_LSHIFT];
				if (outMods.alt   == Required)  { keys[SDLK_LALT]   = 1; }
				if (outMods.alt   == Forbidden) { keys[SDLK_LALT]   = 0; }
				if (outMods.ctrl  == Required)  { keys[SDLK_LCTRL]  = 1; }
				if (outMods.ctrl  == Forbidden) { keys[SDLK_LCTRL]  = 0; }
				if (outMods.meta  == Required)  { keys[SDLK_LMETA]  = 1; }
				if (outMods.meta  == Forbidden) { keys[SDLK_LMETA]  = 0; }
				if (outMods.shift == Required)  { keys[SDLK_LSHIFT] = 1; }
				if (outMods.shift == Forbidden) { keys[SDLK_LSHIFT] = 0; }

				Action action(copy);
				if (!ProcessLocalActions(action)) {
					CKeySet ks;
					game->ActionPressed(action, ks, false /*isRepeat*/);
				}

				keys[SDLK_LALT]   = tmpAlt;
				keys[SDLK_LCTRL]  = tmpCtrl;
				keys[SDLK_LMETA]  = tmpMeta;
				keys[SDLK_LSHIFT] = tmpShift;
			}
		}
	}
	depth--;
}


bool CGuiHandler::AboveGui(int x, int y)
{
	GML_RECMUTEX_LOCK(gui); // AboveGui - called from CMouseHandler::GetCurrentTooltip --> IsAbove

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


void CGuiHandler::CreateOptions(Command& c, bool rmb)
{
	c.options = 0;
	if (rmb) {
		c.options |= RIGHT_MOUSE_KEY;
	}
	if (GetQueueKeystate()) {
		// allow mouse button 'rocker' movements to force
		// immediate mode (when queuing is the default mode)
		if (!invertQueueKey ||
		    (!mouse->buttons[SDL_BUTTON_LEFT].pressed &&
		     !mouse->buttons[SDL_BUTTON_RIGHT].pressed)) {
			c.options |= SHIFT_KEY;
		}
	}
	if (keys[SDLK_LCTRL]) {
		c.options |= CONTROL_KEY;
	}
	if (keys[SDLK_LALT]) {// || keys[SDLK_LMETA]) {
		c.options |= ALT_KEY;
	}
	if (keys[SDLK_LMETA]) {
		c.options |= META_KEY;
	}
}


float CGuiHandler::GetNumberInput(const CommandDescription& cd) const
{
	float minV = 0.0f;
	float maxV = 100.0f;
	if (cd.params.size() >= 1) { minV = atof(cd.params[0].c_str()); }
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
int CGuiHandler::GetDefaultCommand(int x, int y, float3& camerapos, float3& mousedir) const
{
	CInputReceiver* ir = NULL;
	if (!game->hideInterface) {
		ir = GetReceiverAt(x, y);
	}

	if ((ir != NULL) && (ir != minimap)) {
		return -1;
	}

	GML_RECMUTEX_LOCK(sel); // GetDefaultCommand - anti deadlock
	GML_RECMUTEX_LOCK(quad); // GetDefaultCommand

	const CUnit* unit = NULL;
	const CFeature* feature = NULL;
	if ((ir == minimap) && (minimap->FullProxy())) {
		unit = minimap->GetSelectUnit(minimap->GetMapPosition(x, y));
	}
	else {
		const float3 camPos = camerapos;
		const float3 camDir = mousedir;
		const float viewRange = globalRendering->viewRange*1.4f;
		const float dist = helper->GuiTraceRay(camPos, camDir, viewRange, unit, true);
		const float dist2 = helper->GuiTraceRayFeature(camPos, camDir, viewRange, feature);
		const float3 hit = camPos + camDir*dist;

		// make sure the ray hit in the map
		if (unit == NULL && (hit.x < 0.f || hit.x > gs->mapx*SQUARE_SIZE
				|| hit.z < 0.f || hit.z > gs->mapy*SQUARE_SIZE))
			return -1;

		if ((dist > viewRange - 300) && (dist2 > viewRange - 300) && (unit == NULL)) {
			return -1;
		}

		if (dist > dist2) {
			unit = NULL;
		} else {
			feature = NULL;
		}
	}

	GML_RECMUTEX_LOCK(gui); // GetDefaultCommand

	// make sure the command is currently available
	const int cmd_id = selectedUnits.GetDefaultCmd(unit, feature);
	for (int c = 0; c < (int)commands.size(); c++) {
		if (cmd_id == commands[c].id) {
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
			(commands[inCommand].type == CMDTYPE_ICON_BUILDING)) {
		if (ProcessBuildActions(action)) {
			return true;
		}
	}

	if (action.command == "buildiconsfirst") {
		activePage = 0;
		selectedUnits.SetCommandPage(activePage);
		selectedUnits.ToggleBuildIconsFirst();
		LayoutIcons(false);
		return true;
	}
	else if (action.command == "firstmenu") {
		activePage = 0;
		selectedUnits.SetCommandPage(activePage);
		return true;
	}
	else if (action.command == "hotbind") {
		const int iconPos = IconAtPos(mouse->lastx, mouse->lasty);
		const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;
		if ((iconCmd >= 0) && ((size_t)iconCmd < commands.size())) {
			game->SetHotBinding(commands[iconCmd].action);
		}
		return true;
	}
	else if (action.command == "hotunbind") {
		const int iconPos = IconAtPos(mouse->lastx, mouse->lasty);
		const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;
		if ((iconCmd >= 0) && ((size_t)iconCmd < commands.size())) {
			std::string cmd = "unbindaction " + commands[iconCmd].action;
			keyBindings->Command(cmd);
			logOutput.Print("%s", cmd.c_str());
		}
		return true;
	}
	else if (action.command == "showcommands") {
		// bonus command for debugging
		logOutput.Print("Available Commands:\n");
		for(size_t i = 0; i < commands.size(); ++i){
			LogObject() << "  command: " << i << ", id = " << commands[i].id << ", action = " << commands[i].action;
		}
		// show the icon/command linkage
		logOutput.Print("Icon Linkage:\n");
		for(int ii = 0; ii < iconsCount; ++ii){
			logOutput.Print("  icon: %i, commandsID = %i\n",
			                ii, icons[ii].commandsID);
		}
		logOutput.Print("maxPage         = %i\n", maxPage);
		logOutput.Print("activePage      = %i\n", activePage);
		logOutput.Print("iconsSize       = %u\n", iconsSize);
		logOutput.Print("iconsCount      = %i\n", iconsCount);
		LogObject() << "commands.size() = " << commands.size();
		return true;
	}
	else if (action.command == "luaui") {
		RunLayoutCommand(action.extra);
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


void CGuiHandler::RunLayoutCommand(const std::string& command)
{
	if (command.find("reload") == 0) {
		if (CLuaHandle::GetActiveHandle() != NULL) {
			// NOTE: causes a SEGV through RunCallIn()
			logOutput.Print("Can not reload from within LuaUI, yet");
			return;
		}
		if (luaUI == NULL) {
			logOutput.Print("Loading: \"%s\"\n", "luaui.lua"); // FIXME
			CLuaUI::LoadHandler();
			if (luaUI == NULL) {
				LoadConfig("ctrlpanel.txt");
				logOutput.Print("Loading failed\n");
			}
		} else {
			logOutput.Print("Reloading: \"%s\"\n", "luaui.lua"); // FIXME
			CLuaUI::FreeHandler();
			CLuaUI::LoadHandler();
			if (luaUI == NULL) {
				LoadConfig("ctrlpanel.txt");
				logOutput.Print("Reloading failed\n");
			}
		}
	}
	else if (command == "disable") {
		if (CLuaHandle::GetActiveHandle() != NULL) {
			// NOTE: might cause a SEGV through RunCallIn()
			logOutput.Print("Can not disable from within LuaUI, yet");
			return;
		}
		if (luaUI != NULL) {
			CLuaUI::FreeHandler();
			LoadConfig("ctrlpanel.txt");
			logOutput.Print("Disabled LuaUI\n");
		}
	}
	else {
		if (luaUI != NULL) {
			luaUI->ConfigCommand(command);
		} else {
			logOutput.Print("LuaUI is not loaded\n");
		}
	}

	LayoutIcons(false);

	return;
}


bool CGuiHandler::ProcessBuildActions(const Action& action)
{
	const std::string arg = StringToLower(action.extra);
	if (action.command == "buildspacing") {
		if (arg == "inc") {
			buildSpacing++;
			return true;
		}
		else if (arg == "dec") {
			if (buildSpacing > 0) {
				buildSpacing--;
			}
			return true;
		}
	}
	else if (action.command == "buildfacing") {
		const char* buildFaceDirs[] = { "South", "East", "North", "West" };
		if (arg == "inc") {
			buildFacing++;
			if (buildFacing > 3) {
				buildFacing = 0;
			}
			logOutput.Print("Buildings set to face %s", buildFaceDirs[buildFacing]);
			return true;
		}
		else if (arg == "dec") {
			buildFacing--;
			if (buildFacing < 0) {
				buildFacing = 3;
			}
			logOutput.Print("Buildings set to face %s", buildFaceDirs[buildFacing]);
			return true;
		}
		else if (arg == "south") {
			buildFacing = 0;
			logOutput.Print("Buildings set to face South");
			return true;
		}
		else if (arg == "east") {
			buildFacing = 1;
			logOutput.Print("Buildings set to face East");
			return true;
		}
		else if (arg == "north") {
			buildFacing = 2;
			logOutput.Print("Buildings set to face North");
			return true;
		}
		else if (arg == "west") {
			buildFacing = 3;
			logOutput.Print("Buildings set to face West");
			return true;
		}
	}
	return false;
}


int CGuiHandler::GetIconPosCommand(int slot) const // only called by SetActiveCommand
{
	if (slot < 0) {
		return -1;
	}
	const int iconPos = slot + (activePage * iconsPerPage);
	if (iconPos < iconsCount) {
		return icons[iconPos].commandsID;
	}
	return -1;
}


bool CGuiHandler::KeyPressed(unsigned short key, bool isRepeat)
{
	GML_RECMUTEX_LOCK(gui); // KeyPressed - updates inCommand

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

	CKeySet ks(key, false);
	const CKeyBindings::ActionList& al = keyBindings->GetActionList(ks);

	// setup actionOffset
	int tmpActionOffset = actionOffset;
	if ((inCommand < 0) || (lastKeySet.Key() < 0)){
		actionOffset = 0;
		tmpActionOffset = 0;
		lastKeySet.Reset();
	}
	else if (!keyCodes->IsModifier(ks.Key()) &&
	         (ks.Key() != keyBindings->GetFakeMetaKey())) {
		// not a modifier
		if ((ks == lastKeySet) && (ks.Key() >= 0)) {
			actionOffset++;
			tmpActionOffset = actionOffset;
		} else {
			tmpActionOffset = 0;
		}
	}

	for (int ali = 0; ali < (int)al.size(); ++ali) {
		const int actionIndex = (ali + tmpActionOffset) % (int)al.size();
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
	GML_RECMUTEX_LOCK(gui); // SetActiveCommand - updates inCommand, called by LuaUnsyncedCtrl

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

		CommandDescription& cmdDesc = commands[a];

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
					selectedUnits.SetCommandPage(activePage);
				}
			}
		}

		switch (cmdType) {
			case CMDTYPE_ICON:{
				Command c;
				c.options = 0;
				c.id = cmdDesc.id;
				if ((c.id < 0) || (c.id == CMD_STOCKPILE)) {
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

				Command c;
				c.options = 0;
				c.id = cmdDesc.id;
				c.params.push_back(newMode);
				GiveCommand(c);
				forceLayoutUpdate = true;
				break;
			}
			case CMDTYPE_NUMBER:{
				if (!action.extra.empty()) {
					const CommandDescription& cd = cmdDesc;
					float value = atof(action.extra.c_str());
					float minV = 0.0f;
					float maxV = 100.0f;
					if (cd.params.size() >= 1) { minV = atof(cd.params[0].c_str()); }
					if (cd.params.size() >= 2) { maxV = atof(cd.params[1].c_str()); }
					value = std::max(std::min(value, maxV), minV);
					Command c;
					c.options = 0;
					if (action.extra.find("queued") != std::string::npos) {
						c.options = SHIFT_KEY;
					}
					c.id = cd.id;
					c.params.push_back(value);
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
				selectedUnits.SetCommandPage(activePage);
				break;
			}
			case CMDTYPE_PREV:{
				--activePage;
				if(activePage<0)
					activePage = maxPage;
				selectedUnits.SetCommandPage(activePage);
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


bool CGuiHandler::KeyReleased(unsigned short key)
{
	return false;
}

void CGuiHandler::FinishCommand(int button)
{
	GML_RECMUTEX_LOCK(gui); // FinishCommand - updates inCommand

	if ((button == SDL_BUTTON_LEFT) && (keys[SDLK_LSHIFT] || invertQueueKey)) {
		needShift=true;
	} else {
		SetShowingMetal(false);
		inCommand=-1;
	}
}


bool CGuiHandler::IsAbove(int x, int y)
{
	return AboveGui(x, y);
}


std::string CGuiHandler::GetTooltip(int x, int y)
{
	GML_RECMUTEX_LOCK(gui); // GetTooltip - called from LuaUnsyncedRead::GetCurrentTooltip --> CMouseHandler::GetCurrentTooltip

	std::string s;

	const int iconPos = IconAtPos(x, y);
	const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;
	if ((iconCmd >= 0) && (iconCmd < (int)commands.size())) {
		if (commands[iconCmd].tooltip != "") {
			s = commands[iconCmd].tooltip;
		}else{
			s = commands[iconCmd].name;
		}

		const CKeyBindings::HotkeyList& hl =
			keyBindings->GetHotkeys(commands[iconCmd].action);
		if(!hl.empty()){
			s+="\nHotkeys:";
			for (int i = 0; i < (int)hl.size(); ++i) {
				s+=" ";
				s+=hl[i];
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
	GML_RECMUTEX_LOCK(gui); // GetBuildTooltip - called from LuaUnsyncedRead::GetCurrentTooltip --> MouseHandler::GetCurrentTooltip

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


Command CGuiHandler::GetCommand(int mousex, int mousey, int buttonHint, bool preview, float3& camerapos, float3& mousedir)
{
	GML_RECMUTEX_LOCK(gui); // GetCommand - updates inCommand

	Command defaultRet;
	defaultRet.id=CMD_STOP;

	int button;
	if (buttonHint >= SDL_BUTTON_LEFT) {
		button = buttonHint;
	} else if (inCommand != -1) {
		button = SDL_BUTTON_LEFT;
	} else if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
		button = SDL_BUTTON_RIGHT;
	} else {
		return defaultRet;
	}

	int tempInCommand = inCommand;

	if (button == SDL_BUTTON_RIGHT && preview) {
		// right click -> default cmd
		// (in preview we might not have default cmd memory set)
		if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
			tempInCommand = defaultCmdMemory;
		} else {
			tempInCommand = GetDefaultCommand(mousex, mousey, camerapos, mousedir);
		}
	}

	if(tempInCommand>=0 && (size_t)tempInCommand<commands.size()){
		switch(commands[tempInCommand].type){

		case CMDTYPE_NUMBER:{
			const float value = GetNumberInput(commands[tempInCommand]);
			Command c;
			c.id = commands[tempInCommand].id;;
			c.params.push_back(value);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON:{
			Command c;
			c.id=commands[tempInCommand].id;
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			if(button==SDL_BUTTON_LEFT && !preview)
				logOutput.Print("CMDTYPE_ICON left button press in incommand test? This shouldnt happen");
			return c;}

		case CMDTYPE_ICON_MAP:{
			float dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=camerapos+mousedir*dist;
			Command c;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_BUILDING:{
			float dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
			if(dist<0){
				return defaultRet;
			}
			const UnitDef* unitdef = unitDefHandler->GetUnitDefByID(-commands[inCommand].id);

			if(!unitdef){
				return defaultRet;
			}

			float3 pos=camerapos+mousedir*dist;
			std::vector<BuildInfo> buildPos;
			BuildInfo bi(unitdef, pos, buildFacing);
			if(GetQueueKeystate() && button==SDL_BUTTON_LEFT){
				float dist=ground->LineGroundCol(mouse->buttons[SDL_BUTTON_LEFT].camPos,mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*globalRendering->viewRange*1.4f);
				float3 pos2=mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*dist;
				buildPos=GetBuildPos(BuildInfo(unitdef,pos2,buildFacing),bi,camerapos,mousedir);
			} else
				buildPos=GetBuildPos(bi,bi,camerapos,mousedir);

			if(buildPos.empty()){
				return defaultRet;
			}

			if(buildPos.size()==1) {
				CFeature* feature; // TODO: Maybe also check out-of-range for immobile builder?
				if (!uh->TestUnitBuildSquare(buildPos[0], feature, gu->myAllyTeam)) {
					Command failedRet;
					failedRet.id = CMD_FAILED;
					return failedRet;
				}
			}

			int a=0; // limit the number of max commands possible to send to avoid overflowing the network buffer
			for(std::vector<BuildInfo>::iterator bpi=buildPos.begin();bpi!=--buildPos.end() && a<200;++bpi){
				++a;
				Command c;
				bpi->FillCmd(c);
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				if(!preview)
					GiveCommand(c);
			}
			Command c;
			buildPos.back().FillCmd(c);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT: {
			const CUnit* unit = NULL;
			Command c;

			c.id=commands[tempInCommand].id;
			helper->GuiTraceRay(camerapos,mousedir,globalRendering->viewRange*1.4f,unit,true);
			if (!unit) {
				return defaultRet;
			}
			c.params.push_back(unit->id);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT_OR_MAP: {

			Command c;
			c.id=commands[tempInCommand].id;

			const CUnit* unit = NULL;
			float dist2 = helper->GuiTraceRay(camerapos,mousedir,globalRendering->viewRange*1.4f,unit,true);
			if(dist2 > (globalRendering->viewRange * 1.4f - 300)) {
				return defaultRet;
			}

			if (unit != NULL) {
				// clicked on unit
				c.params.push_back(unit->id);
			} else {
				// clicked in map
				float3 pos=camerapos+mousedir*dist2;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_FRONT:{
			Command c;

			float dist = ground->LineGroundCol(
				mouse->buttons[button].camPos,
				mouse->buttons[button].camPos + mouse->buttons[button].dir * globalRendering->viewRange * 1.4f);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);

			if(mouse->buttons[button].movement>30){		//only create the front if the mouse has moved enough
				dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2 = camerapos + mousedir * dist;

				ProcessFrontPositions(pos, pos2);

				c.params[0] = pos.x;
				c.params[1] = pos.y;
				c.params[2] = pos.z;

				if (!commands[tempInCommand].params.empty() &&
				    pos.SqDistance2D(pos2) > Square(atof(commands[tempInCommand].params[0].c_str()))) {
					float3 dif=pos2-pos;
					dif.ANormalize();
					pos2=pos+dif*atoi(commands[tempInCommand].params[0].c_str());
				}

				c.params.push_back(pos2.x);
				c.params.push_back(pos2.y);
				c.params.push_back(pos2.z);
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT_OR_AREA:
		case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
		case CMDTYPE_ICON_AREA:{
			float maxRadius=100000;
			if(commands[tempInCommand].params.size()==1)
				maxRadius=atof(commands[tempInCommand].params[0].c_str());

			Command c;
			c.id=commands[tempInCommand].id;

			if (mouse->buttons[button].movement < 4) {

				GML_RECMUTEX_LOCK(unit); // GetCommand
				GML_RECMUTEX_LOCK(feat); // GetCommand

				const CUnit* unit = NULL;
				const CFeature* feature = NULL;
				float dist2 = helper->GuiTraceRay(camerapos,mousedir,globalRendering->viewRange*1.4f,unit,true);
				float dist3 = helper->GuiTraceRayFeature(camerapos,mousedir,globalRendering->viewRange*1.4f,feature);

				if(dist2 > (globalRendering->viewRange * 1.4f - 300) && (commands[tempInCommand].type!=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA || dist3>globalRendering->viewRange*1.4f-300)) {
					return defaultRet;
				}

				if (feature!=0 && dist3<dist2 && commands[tempInCommand].type==CMDTYPE_ICON_UNIT_FEATURE_OR_AREA) {  // clicked on feature
					c.params.push_back(uh->MaxUnits()+feature->id);
				} else if (unit!=0 && commands[tempInCommand].type!=CMDTYPE_ICON_AREA) {  // clicked on unit
					c.params.push_back(unit->id);
				} else { // clicked in map
					if(explicitCommand<0) // only attack ground if explicitly set the command
						return defaultRet;
					float3 pos=camerapos+mousedir*dist2;
					c.params.push_back(pos.x);
					c.params.push_back(pos.y);
					c.params.push_back(pos.z);
					c.params.push_back(0);//zero radius
				}
			} else {	//created area
				float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*globalRendering->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
				dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2=camerapos+mousedir*dist;
				c.params.push_back(std::min(maxRadius,pos.distance2D(pos2)));
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT_OR_RECTANGLE:{
			Command c;
			c.id=commands[tempInCommand].id;

			if (mouse->buttons[button].movement < 16) {
				const CUnit* unit = NULL;

				float dist2 = helper->GuiTraceRay(camerapos, mousedir, globalRendering->viewRange*1.4f, unit, true);

				if (dist2 > (globalRendering->viewRange * 1.4f - 300)) {
					return defaultRet;
				}

				if (unit != NULL) {
					// clicked on unit
					c.params.push_back(unit->id);
				} else {
					// clicked in map
					if (explicitCommand < 0) { // only attack ground if explicitly set the command
						return defaultRet;
					}
					float3 pos = camerapos + (mousedir * dist2);
					c.params.push_back(pos.x);
					c.params.push_back(pos.y);
					c.params.push_back(pos.z);
				}
			} else {
				// created rectangle
				float dist = ground->LineGroundCol(
					mouse->buttons[button].camPos,
					mouse->buttons[button].camPos + mouse->buttons[button].dir * globalRendering->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 startPos = mouse->buttons[button].camPos + mouse->buttons[button].dir * dist;
				dist = ground->LineGroundCol(camerapos, camerapos + mousedir*globalRendering->viewRange * 1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 endPos=camerapos+mousedir*dist;
				c.params.push_back(startPos.x);
				c.params.push_back(startPos.y);
				c.params.push_back(startPos.z);
				c.params.push_back(endPos.x);
				c.params.push_back(endPos.y);
				c.params.push_back(endPos.z);
			}
			CreateOptions(c, (button == SDL_BUTTON_LEFT) ? 0 : 1);
			return c;}

		default:
			return defaultRet;
		}
	} else {
		if(!preview)
			inCommand=-1;
	}
	return defaultRet;
}



static bool WouldCancelAnyQueued(const BuildInfo& b)
{
	GML_RECMUTEX_LOCK(sel); // WouldCancelAnyQueued - called from DrawMapStuff -> GetBuildPos --> FillRowOfBuildPos

	Command c;
	b.FillCmd(c);
	CUnitSet::iterator ui = selectedUnits.selectedUnits.begin();
	for(;ui != selectedUnits.selectedUnits.end(); ++ui){
		if((*ui)->commandAI->WillCancelQueued(c))
			return true;
	}
	return false;
}

static void FillRowOfBuildPos(const BuildInfo& startInfo, float x, float z, float xstep, float zstep, int n, int facing, bool nocancel, std::vector<BuildInfo>& ret)
{
	for(int i=0;i<n;++i){
		BuildInfo bi(startInfo.def,float3(x,0,z),(startInfo.buildFacing+facing)%4);
		bi.pos=helper->Pos2BuildPos(bi);
		if (!nocancel || !WouldCancelAnyQueued(bi)){
			ret.push_back(bi);
		}
		x+=xstep;
		z+=zstep;
	}
}

// Assuming both builds have the same unitdef
std::vector<BuildInfo> CGuiHandler::GetBuildPos(const BuildInfo& startInfo, const BuildInfo& endInfo, float3& camerapos, float3& mousedir)
{
	std::vector<BuildInfo> ret;

	float3 start=helper->Pos2BuildPos(startInfo);
	float3 end=helper->Pos2BuildPos(endInfo);

	BuildInfo other; // the unit around which buildings can be circled
	if(GetQueueKeystate() && keys[SDLK_LCTRL])
	{
		const CUnit* unit = NULL;

		GML_RECMUTEX_LOCK(quad); // GetBuildCommand - accesses activeunits - called from DrawMapStuff -> GetBuildPos

		helper->GuiTraceRay(camerapos,mousedir,globalRendering->viewRange*1.4f,unit,true);
		if (unit) {
			other.def = unit->unitDef;
			other.pos = unit->pos;
			other.buildFacing = unit->buildFacing;
		} else {
			Command c = uh->GetBuildCommand(camerapos,mousedir);
			if(c.id < 0){
				assert(c.params.size()==4);
				other.pos = float3(c.params[0],c.params[1],c.params[2]);
				other.def = unitDefHandler->GetUnitDefByID(-c.id);
				other.buildFacing = int(c.params[3]);
			}
		}
	}

	if(other.def && GetQueueKeystate() && keys[SDLK_LCTRL]){		//circle build around building
		int oxsize=other.GetXSize()*SQUARE_SIZE;
		int ozsize=other.GetZSize()*SQUARE_SIZE;
		int xsize=startInfo.GetXSize()*SQUARE_SIZE;
		int zsize=startInfo.GetZSize()*SQUARE_SIZE;

		start =end=helper->Pos2BuildPos(other);
		start.x-=oxsize/2;
		start.z-=ozsize/2;
		end.x+=oxsize/2;
		end.z+=ozsize/2;

		int nvert=1+oxsize/xsize;
		int nhori=1+ozsize/xsize;

		FillRowOfBuildPos(startInfo, end.x  +zsize/2, start.z+xsize/2,      0, +xsize, nhori, 3, true, ret);
		FillRowOfBuildPos(startInfo, end.x  -xsize/2, end.z  +zsize/2, -xsize,      0, nvert, 2, true, ret);
		FillRowOfBuildPos(startInfo, start.x-zsize/2, end.z  -xsize/2,      0, -xsize, nhori, 1, true, ret);
		FillRowOfBuildPos(startInfo, start.x+xsize/2, start.z-zsize/2, +xsize,      0, nvert, 0, true, ret);

	} else { // rectangle or line
		float3 delta = end - start;

		float xsize=SQUARE_SIZE*(startInfo.GetXSize()+buildSpacing*2);
		int xnum=(int)((fabs(delta.x)+xsize*1.4f)/xsize);
		float xstep=(int)((0<delta.x) ? xsize : -xsize);

		float zsize=SQUARE_SIZE*(startInfo.GetZSize()+buildSpacing*2);
		int znum=(int)((fabs(delta.z)+zsize*1.4f)/zsize);
		float zstep=(int)((0<delta.z) ? zsize : -zsize);

		if(keys[SDLK_LALT]){ // build a rectangle
			if(keys[SDLK_LCTRL]){ // hollow rectangle
				if(1<xnum&&1<znum){
					FillRowOfBuildPos(startInfo, start.x               , start.z+zstep         ,      0,  zstep, znum-1, 0, false, ret); // go "down" on the "left" side
					FillRowOfBuildPos(startInfo, start.x+xstep         , start.z+(znum-1)*zstep,  xstep,      0, xnum-1, 0, false, ret); // go "right" on the "bottom" side
					FillRowOfBuildPos(startInfo, start.x+(xnum-1)*xstep, start.z+(znum-2)*zstep,      0, -zstep, znum-1, 0, false, ret); // go "up" on the "right" side
					FillRowOfBuildPos(startInfo, start.x+(xnum-2)*xstep, start.z               , -xstep,      0, xnum-1, 0, false, ret); // go "left" on the "top" side
				} else if(1==xnum){
					FillRowOfBuildPos(startInfo, start.x, start.z, 0, zstep, znum, 0, false, ret);
				} else if(1==znum){
					FillRowOfBuildPos(startInfo, start.x, start.z, xstep, 0, xnum, 0, false, ret);
				}
			} else { // filled rectangle
				int zn=0;
				for(float z=start.z;zn<znum;++zn){
					if(zn&1){
						FillRowOfBuildPos(startInfo, start.x+(xnum-1)*xstep, z, -xstep, 0, xnum, 0, false, ret); // every odd line "right" to "left"
					} else {
						FillRowOfBuildPos(startInfo, start.x               , z, xstep, 0, xnum, 0, false, ret); // every even line "left" to "right"
					}
					z+=zstep;
				}
			}
		} else { // build a line
			bool x_dominates_z = fabs(delta.x) > fabs(delta.z);
			if (x_dominates_z){
				zstep = keys[SDLK_LCTRL] ? 0 : xstep * delta.z/(delta.x ? delta.x : 1);
			} else {
				xstep = keys[SDLK_LCTRL] ? 0 : zstep * delta.x/(delta.z ? delta.z : 1);
			}
			FillRowOfBuildPos(startInfo, start.x, start.z, xstep, zstep, x_dominates_z ? xnum : znum, 0, false, ret);
		}
	}
	return ret;
}


void CGuiHandler::ProcessFrontPositions(float3& pos0, float3& pos1)
{
	if (!frontByEnds) {
		return; // leave it centered
	}
	pos0 = pos1 + ((pos0 - pos1) * 0.5f);
	pos0.y = ground->GetHeight2(pos0.x, pos0.z);
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::Draw()
{
	if ((iconsCount <= 0) && (luaUI == NULL)) {
		return;
	}

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.01f);

	if (iconsCount > 0) {
		DrawButtons();
	}

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
	const CommandDescription& cmdDesc = commands[icon.commandsID];

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
	// UnitDefHandler's array size is (numUnitDefs + 1)
	if ((unitDefID <= 0) || (unitDefID > unitDefHandler->numUnitDefs)) {
		return false;
	}

	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud != NULL) {
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
                                 std::string& tex1, std::string& tex2,
                                 float& xscale, float& yscale)
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
	// UnitDefHandler's array size is (numUnitDefs + 1)
	if ((unitDefID <= 0) || (unitDefID > unitDefHandler->numUnitDefs)) {
		return false;
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
	// UnitDefHandler's array size is (numUnitDefs + 1)
	if ((unitDefID <= 0) || (unitDefID > unitDefHandler->numUnitDefs)) {
		return false;
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
		case 'g': { luaHandle = luaGaia; break; }
		case 'm': { luaHandle = luaRules; break; }
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

	const float tWidth  = font->GetSize() * font->GetTextWidth(text) * globalRendering->pixelX;  //FIXME
	const float tHeight = font->GetSize() * font->GetTextHeight(text) * globalRendering->pixelY; //FIXME merge in 1 function?
	const float textBorder2 = (2.0f * textBorder);
	float xScale = (xIconSize - textBorder2) / tWidth;
	float yScale = (yIconSize - textBorder2 - yShrink) / tHeight;
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
		const GLfloat fx = 0.0f; //-.2f*(1-fadein/100.0f)+.2f;
			glVertex2f(buttonBox.x1 - fx, buttonBox.y1);
			glVertex2f(buttonBox.x1 - fx, buttonBox.y2);
			glVertex2f(buttonBox.x2 - fx, buttonBox.y2);
			glVertex2f(buttonBox.x2 - fx, buttonBox.y1);
		glEnd();
	}

	const int mouseIcon   = IconAtPos(mouse->lastx, mouse->lasty);
	const int buttonStart = std::min(iconsCount, activePage * iconsPerPage);
	const int buttonEnd   = std::min(iconsCount, buttonStart + iconsPerPage);

	for (int ii = buttonStart; ii < buttonEnd; ii++) {

		const IconInfo& icon = icons[ii];
		if (icon.commandsID < 0) {
			continue; // inactive icon
		}
		const CommandDescription& cmdDesc = commands[icon.commandsID];
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
						&& cmdDesc.params.size() >= 1) {
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
		if (selectedUnits.BuildIconsFirst()) {
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
	GML_RECMUTEX_LOCK(sel); // DrawSelectionInfo - called from Draw --> DrawButtons

	if (!selectedUnits.selectedUnits.empty()) {
		std::ostringstream buf;

		if(selectedUnits.selectedGroup!=-1){
			buf << "Selected units " << selectedUnits.selectedUnits.size() << " [Group " << selectedUnits.selectedGroup << "]";
		} else {
			buf << "Selected units " << selectedUnits.selectedUnits.size();
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
		const CommandDescription& cd = commands[inCommand];
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
	const float xSize = 0.166f * fabs(b.x2 - b.x1);
	const float ySize = 0.125f * fabs(b.y2 - b.y1);
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
	const float xSize = 0.166f * fabs(b.x2 - b.x1);
	const float ySize = 0.125f * fabs(b.y2 - b.y1);
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
	const CommandDescription& cmdDesc = commands[icon.commandsID];

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

static inline void DrawSensorRange(int radius,
                                   const float* color, const float3& pos)
{
	const int sensorScale = radarhandler->radarDiv;
	const int realRadius = ((radius / sensorScale) * sensorScale);
	if (realRadius > 0) {
		glColor4fv(color);
		glSurfaceCircle(pos, (float)realRadius, 40);
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
			glVertex3f(1.0f, sin(rad), cos(rad));
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

	const float xlen = len * cos(hrads);
	const float yzlen = len * sin(hrads);
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
	return; // FIXME: disabled
	if (unit->weapons.empty()) {
		return;
	}
	const CWeapon* w = unit->weapons.front();
	float3 dir;
	if (w->onlyForward) {
		dir = unit->frontdir;
	} else {
		dir = w->wantedDir;
	}

	// copied from Weapon.cpp
	const float3 interPos = unit->pos + (unit->speed * globalRendering->timeOffset);
	float3 pos = interPos +
	             (unit->frontdir * w->relWeaponPos.z) +
	             (unit->updir    * w->relWeaponPos.y) +
	             (unit->rightdir * w->relWeaponPos.x);
	if (pos.y < ground->GetHeight2(pos.x, pos.z)) {
		// hope that we are underground because we are a
		// popup weapon and will come above ground later
		pos = interPos + (UpVector * 10.0f);
	}

	const float hrads   = acos(w->maxAngleDif);
	const float heading = atan2(-dir.z, dir.x);
	const float pitch   = asin(dir.y);
	DrawWeaponCone(pos, w->range, hrads, heading, pitch);
}


void CGuiHandler::DrawMapStuff(int onMinimap)
{
	if (!onMinimap) {
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_ALPHA_TEST);
	}

	float3 camerapos=camera->pos;
	float3 mousedir=mouse->dir;

	// setup for minimap proxying
	const bool minimapCoords =
		(minimap->ProxyMode() ||
		 ((activeReceiver != this) && !game->hideInterface &&
		  (GetReceiverAt(mouse->lastx, mouse->lasty) == minimap)));
	if (minimapCoords) {
		camerapos = minimap->GetMapPosition(mouse->lastx, mouse->lasty);
		mousedir = float3(0.0f, -1.0f, 0.0f);
		if (miniMapMarker && minimap->FullProxy() &&
		    !onMinimap && !minimap->GetMinimized()) {
			DrawMiniMapMarker(camerapos);
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

		if ((cmdIndex >= 0) && ((size_t)cmdIndex < commands.size())) {
			const CommandDescription& cmdDesc = commands[cmdIndex];
			switch (cmdDesc.type) {
				case CMDTYPE_ICON_FRONT: {
					if (mouse->buttons[button].movement > 30) {
						float maxSize = 1000000.0f;
						float sizeDiv = 0.0f;
						if (cmdDesc.params.size() > 0) {
							maxSize = atof(cmdDesc.params[0].c_str());
						}
						if (cmdDesc.params.size() > 1) {
							sizeDiv = atof(cmdDesc.params[1].c_str());
						}
						DrawFront(button, maxSize, sizeDiv, !!onMinimap, camerapos, mousedir);
					}
					break;
				}
				case CMDTYPE_ICON_UNIT_OR_AREA:
				case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
				case CMDTYPE_ICON_AREA: {
					float maxRadius=100000;
					if (cmdDesc.params.size() == 1) {
						maxRadius = atof(cmdDesc.params[0].c_str());
					}
					if (mouse->buttons[button].movement > 4) {
						float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*globalRendering->viewRange*1.4f);
						if(dist<0){
							break;
						}
						float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
						dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
						if (dist < 0) {
							break;
						}
						float3 pos2 = camerapos + mousedir * dist;
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
						}
						else {
							glColor4f(color[0], color[1], color[2], 0.5f);
							glBegin(GL_TRIANGLE_FAN);
							const int divs = 256;
							for(int i = 0; i <= divs; ++i) {
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
						float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*globalRendering->viewRange*1.4f);
						if (dist < 0) {
							break;
						}
						const float3 pos1 = mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
						dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
						if (dist < 0) {
							break;
						}
						const float3 pos2 = camerapos+mousedir*dist;
						if (!onMinimap) {
							DrawSelectBox(pos1, pos2, camerapos);
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

	GML_RECMUTEX_LOCK(unit); // DrawMapStuff

	if (GetQueueKeystate()) {
		const CUnit* unit = NULL;
		if (minimapCoords) {
			unit = minimap->GetSelectUnit(camerapos);
		} else {
			// ignoring the returned distance
			helper->GuiTraceRay(camerapos,mousedir,globalRendering->viewRange*1.4f,unit,false);
		}
		if (unit && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
			pointedAt = unit;
			const UnitDef* unitdef = unit->unitDef;
			const bool enemyUnit = ((unit->allyteam != gu->myAllyTeam) && !gu->spectatingFullView);
			if (enemyUnit && unitdef->decoyDef) {
				unitdef = unitdef->decoyDef;
			}

			// draw weapon range
			if (unitdef->maxWeaponRange > 0) {
				glColor4fv(cmdColors.rangeAttack);
				glBallisticCircle(unit->pos, unitdef->maxWeaponRange,
				                  unit->weapons[0], 40);
			}
			// draw decloak distance
			if (unit->decloakDistance > 0.0f) {
				glColor4fv(cmdColors.rangeDecloak);
#ifndef USE_GML
				if (unit->unitDef->decloakSpherical && globalRendering->drawdebug) {
					glPushMatrix();
					glTranslatef(unit->midPos.x, unit->midPos.y, unit->midPos.z);
					glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
					GLUquadricObj* q = gluNewQuadric();
					gluQuadricDrawStyle(q, GLU_LINE);
					gluSphere(q, unit->decloakDistance, 10, 10);
					gluDeleteQuadric(q);
					glPopMatrix();
				} else
#endif
				{ // cylindrical
					glSurfaceCircle(unit->pos, unit->decloakDistance, 40);
				}
			}
			// draw self destruct and damage distance
			if (unitdef->kamikazeDist > 0) {
				glColor4fv(cmdColors.rangeKamikaze);
				glSurfaceCircle(unit->pos, unitdef->kamikazeDist, 40);
				if (!unitdef->selfDExplosion.empty()) {
					glColor4fv(cmdColors.rangeSelfDestruct);
					const WeaponDef* wd = weaponDefHandler->GetWeapon(unitdef->selfDExplosion);
					glSurfaceCircle(unit->pos, wd->areaOfEffect, 40);
				}
			}
			// draw build distance for immobile builders
			if (unitdef->builder && !unitdef->canmove) {
				const float radius = unitdef->buildDistance;
				if (radius > 0.0f) {
					glColor4fv(cmdColors.rangeBuild);
					glSurfaceCircle(unit->pos, radius, 40);
				}
			}
			// draw shield range for immobile units
			if (unitdef->shieldWeaponDef && !unitdef->canmove) {
				glColor4fv(cmdColors.rangeShield);
				glSurfaceCircle(unit->pos, unitdef->shieldWeaponDef->shieldRadius, 40);
			}
			// draw sensor and jammer ranges
			if (unitdef->onoffable || unitdef->activateWhenBuilt) {
				const float3& p = unit->pos;
				DrawSensorRange(unitdef->radarRadius,    cmdColors.rangeRadar, p);
				DrawSensorRange(unitdef->sonarRadius,    cmdColors.rangeSonar, p);
				DrawSensorRange(unitdef->seismicRadius,  cmdColors.rangeSeismic, p);
				DrawSensorRange(unitdef->jammerRadius,   cmdColors.rangeJammer, p);
				DrawSensorRange(unitdef->sonarJamRadius, cmdColors.rangeSonarJammer, p);
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
			GML_STDMUTEX_LOCK(cai); // DrawMapStuff
			// draw build distance for all immobile builders during build commands
			std::list<CBuilderCAI*>::const_iterator bi;
			for (bi = uh->builderCAIs.begin(); bi != uh->builderCAIs.end(); ++bi) {
				const CUnit* unit = (*bi)->owner;
				if ((unit == pointedAt) || (unit->team != gu->myTeam)) {
					continue;
				}
				const UnitDef* unitdef = unit->unitDef;
				if (unitdef->builder && !unitdef->canmove) {
					const float radius = unitdef->buildDistance;
					if (radius > 0.0f) {
						glDisable(GL_TEXTURE_2D);
						const float* color = cmdColors.rangeBuild;
						glColor4f(color[0], color[1], color[2], color[3] * 0.333f);
						glSurfaceCircle(unit->pos, radius, 40);
					}
				}
			}
		}

		float dist = ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
		if (dist > 0) {
			const UnitDef* unitdef = unitDefHandler->GetUnitDefByID(-commands[inCommand].id);
			if (unitdef) {
				// get the build information
				float3 pos = camerapos+mousedir*dist;
				std::vector<BuildInfo> buildPos;
				const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];
				if (GetQueueKeystate() && bp.pressed) {
					const float dist = ground->LineGroundCol(bp.camPos, bp.camPos + bp.dir * globalRendering->viewRange * 1.4f);
					const float3 pos2 = bp.camPos + bp.dir * dist;
					buildPos = GetBuildPos(BuildInfo(unitdef, pos2, buildFacing),
					                       BuildInfo(unitdef, pos, buildFacing), camerapos, mousedir);
				} else {
					BuildInfo bi(unitdef, pos, buildFacing);
					buildPos = GetBuildPos(bi, bi, camerapos, mousedir);
				}

				for (std::vector<BuildInfo>::iterator bpi = buildPos.begin(); bpi != buildPos.end(); ++bpi) {
					const float3& buildpos = bpi->pos;
					// draw weapon range
					if (unitdef->weapons.size() > 0) {
						glColor4fv(cmdColors.rangeAttack);
						glBallisticCircle(buildpos, unitdef->weapons[0].def->range,
						                  NULL, 40, unitdef->weapons[0].def->heightmod);
					}
					// draw extraction range
					if (unitdef->extractRange > 0) {
						glColor4fv(cmdColors.rangeExtract);

						if (unitdef->extractSquare) {
							glSurfaceSquare(buildpos, unitdef->extractRange, unitdef->extractRange);
						} else {
							glSurfaceCircle(buildpos, unitdef->extractRange, 40);
						}
					}
					// draw build range for immobile builders
					if (unitdef->builder && !unitdef->canmove) {
						const float radius = unitdef->buildDistance;
						if (radius > 0.0f) {
							glColor4fv(cmdColors.rangeBuild);
							glSurfaceCircle(buildpos, radius, 40);
						}
					}
					// draw shield range for immobile units
					if (unitdef->shieldWeaponDef && !unitdef->canmove) {
						glColor4fv(cmdColors.rangeShield);
						glSurfaceCircle(buildpos, unitdef->shieldWeaponDef->shieldRadius, 40);
					}
					// draw interceptor range
					const WeaponDef* wd = unitdef->stockpileWeaponDef;
					if ((wd != NULL) && wd->interceptor) {
						glColor4fv(cmdColors.rangeInterceptorOn);
						glSurfaceCircle(buildpos, wd->coverageRange, 40);
					}
					// draw sensor and jammer ranges
					if (unitdef->onoffable || unitdef->activateWhenBuilt) {
						const float3& p = buildpos;
						DrawSensorRange(unitdef->radarRadius,    cmdColors.rangeRadar, p);
						DrawSensorRange(unitdef->sonarRadius,    cmdColors.rangeSonar, p);
						DrawSensorRange(unitdef->seismicRadius,  cmdColors.rangeSeismic, p);
						DrawSensorRange(unitdef->jammerRadius,   cmdColors.rangeJammer, p);
						DrawSensorRange(unitdef->sonarJamRadius, cmdColors.rangeSonarJammer, p);
					}

					std::vector<Command> cv;
					if (GetQueueKeystate()) {

						GML_RECMUTEX_LOCK(sel); // DrawMapStuff

						Command c;
						bpi->FillCmd(c);
						std::vector<Command> temp;
						CUnitSet::iterator ui = selectedUnits.selectedUnits.begin();
						for (; ui != selectedUnits.selectedUnits.end(); ++ui) {
							temp = (*ui)->commandAI->GetOverlapQueued(c);
							std::vector<Command>::iterator ti = temp.begin();
							for (; ti != temp.end(); ti++) {
								cv.insert(cv.end(),*ti);
							}
						}
					}
					if (unitDrawer->ShowUnitBuildSquare(*bpi, cv)) {
						glColor4f(0.7f,1,1,0.4f);
					} else {
						glColor4f(1,0.5f,0.5f,0.4f);
					}

					if (!onMinimap) {
						unitDrawer->DrawBuildingSample(bpi->def, gu->myTeam, buildpos, bpi->buildFacing);

						glBlendFunc((GLenum)cmdColors.SelectedBlendSrc(),
												(GLenum)cmdColors.SelectedBlendDst());
					}
				}
			}
		}
	}

	// draw range circles if attack orders are imminent
	int defcmd = GetDefaultCommand(mouse->lastx, mouse->lasty, camerapos, mousedir);
	if ((inCommand>=0 && (size_t)inCommand<commands.size() && commands[inCommand].id==CMD_ATTACK) ||
		(inCommand==-1 && defcmd>0 && commands[defcmd].id==CMD_ATTACK)){

		GML_RECMUTEX_LOCK(sel); // DrawMapStuff

		for(CUnitSet::iterator si=selectedUnits.selectedUnits.begin(); si!=selectedUnits.selectedUnits.end(); ++si) {
			CUnit* unit = *si;
			if (unit == pointedAt) {
				continue;
			}
			if (onMinimap && (unit->unitDef->speed > 0.0f)) {
				continue;
			}
			if(unit->maxRange>0 && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)){
				glColor4fv(cmdColors.rangeAttack);
				glBallisticCircle(unit->pos, unit->maxRange,
				                  unit->weapons.front(), 40);
				if (!onMinimap && gs->cheatEnabled) {
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

/*	if (minimapCoords) {
		camera->pos = tmpCamPos;
		mouse->dir = tmpMouseDir;
	}*/
}


void CGuiHandler::DrawMiniMapMarker(float3& camerapos)
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

	const float groundLevel = ground->GetHeight(camerapos.x, camerapos.z);

	static float spinTime = 0.0f;
	spinTime = fmod(spinTime + globalRendering->lastFrameTime, 60.0f);

	glPushMatrix();
	glTranslatef(camerapos.x, groundLevel, camerapos.z);
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
	GML_RECMUTEX_LOCK(sel); // DrawCentroidCursor - called From CMouseHandler::DrawCursor

	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	if (selUnits.size() < 2) {
		return;
	}

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
		if (!keys[SDLK_LCTRL] && !keys[SDLK_LALT] && !keys[SDLK_LMETA]) {
			return;
		}
	} else if ((cmd == CMD_FIGHT) || (cmd == CMD_PATROL)) {
		if (!keys[SDLK_LCTRL]) {
			return;
		}
	} else {
		return;
	}

	float3 pos(0.0f, 0.0f, 0.0f);
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		pos += (*it)->midPos;
	}
	pos /= (float)selUnits.size();
	const float3 winPos = camera->CalcWindowCoordinates(pos);
	if (winPos.z <= 1.0f) {
		std::map<std::string, CMouseCursor*>::const_iterator mcit;
		mcit = mouse->cursorCommandMap.find("Centroid");
		if (mcit != mouse->cursorCommandMap.end()) {
			CMouseCursor* mc = mcit->second;
			if (mc != NULL) {
				glDisable(GL_DEPTH_TEST);
				mc->Draw((int)winPos.x, globalRendering->viewSizeY - (int)winPos.y, 1.0f);
				glEnable(GL_DEPTH_TEST);
			}
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
			p.y=ground->GetHeight(p.x,p.z);
			glVertexf3(p);
		}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
}


void CGuiHandler::DrawFront(int button,float maxSize,float sizeDiv, bool onMinimap, float3& camerapos, float3& mousedir)
{
	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	if(bp.movement<5){
		return;
	}
	float dist=ground->LineGroundCol(bp.camPos,bp.camPos+bp.dir*globalRendering->viewRange*1.4f);
	if(dist<0){
		return;
	}
	float3 pos1=bp.camPos+bp.dir*dist;
	dist=ground->LineGroundCol(camerapos,camerapos+mousedir*globalRendering->viewRange*1.4f);
	if(dist<0){
		return;
	}
	float3 pos2 = camerapos + (mousedir * dist);

	ProcessFrontPositions(pos1, pos2);

	float3 forward=(pos1-pos2).cross(UpVector);
	forward.ANormalize();
	float3 side=forward.cross(UpVector);
	if(pos1.SqDistance2D(pos2)>maxSize*maxSize){
		pos2=pos1+side*maxSize;
		pos2.y=ground->GetHeight(pos2.x,pos2.z);
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
		p.y = ground->GetHeight(p.x, p.z);
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
	const BoxData* boxData = (const BoxData*)data;
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
	const float3 corner0(pos0.x, ground->GetHeight(pos0.x, pos0.z), pos0.z);
	const float3 corner1(pos1.x, ground->GetHeight(pos1.x, pos1.z), pos1.z);
	const float3 corner2(pos0.x, ground->GetHeight(pos0.x, pos1.z), pos1.z);
	const float3 corner3(pos1.x, ground->GetHeight(pos1.x, pos0.z), pos0.z);
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
	boxData.mins = float3(std::min(pos0.x, pos1.x), readmap->currMinHeight - 250.0f,
	                       std::min(pos0.z, pos1.z));
	boxData.maxs = float3(std::max(pos0.x, pos1.x), readmap->currMaxHeight + 10000.0f,
	                      std::max(pos0.z, pos1.z));

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


void CGuiHandler::DrawSelectBox(const float3& pos0, const float3& pos1, float3& camerapos)
{
	if (useStencil) {
		StencilDrawSelectBox(pos0, pos1, invColorSelect);
		return;
	}

	const float3 mins(std::min(pos0.x, pos1.x), readmap->currMinHeight - 250.0f,
	                  std::min(pos0.z, pos1.z));
	const float3 maxs(std::max(pos0.x, pos1.x), readmap->currMaxHeight + 10000.0f,
	                  std::max(pos0.z, pos1.z));

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
	if ((camerapos.x > mins.x) && (camerapos.x < maxs.x) &&
	    (camerapos.y > mins.y) && (camerapos.y < maxs.y) &&
	    (camerapos.z > mins.z) && (camerapos.z < maxs.z)) {
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
	const CylinderData& cyl = *((const CylinderData*)data);
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
	cylData.yp = readmap->currMaxHeight + 10000.0f;
	cylData.yn = readmap->currMinHeight - 250.0f;
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
	const float3 base(pos.x, ground->GetHeight(pos.x, pos.z), pos.z);
	glBegin(GL_LINES);
		glVertexf3(base);
		glVertexf3(base + float3(0.0f, 128.0f, 0.0f));
	glEnd();
	glLineWidth(1.0f);

	glEnable(GL_FOG);
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::SetBuildFacing(int facing)
{
	buildFacing = facing % 4;
	if (buildFacing < 0)
		buildFacing += 4;
}

void CGuiHandler::SetBuildSpacing(int spacing)
{
	buildSpacing = spacing;
	if (buildSpacing < 0)
		buildSpacing = 0;
}

/******************************************************************************/
/******************************************************************************/
