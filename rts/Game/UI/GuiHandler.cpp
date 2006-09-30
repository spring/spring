#include "StdAfx.h"
// GuiHandler.cpp: implementation of the CGuiHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "GuiHandler.h"
#include <map>
#include <set>
#include "SDL_keysym.h"
#include "SDL_mouse.h"
#include "CommandColors.h"
#include "IconLayoutHandler.h"
#include "InfoConsole.h"
#include "KeyBindings.h"
#include "KeyCodes.h"
#include "MouseHandler.h"
#include "OutlineFont.h"
#include "SimpleParser.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/LogOutput.h"
#include "System/Platform/ConfigHandler.h"
#include "mmgr.h"

extern Uint8 *keys;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CGuiHandler* guihandler;


CGuiHandler::CGuiHandler()
: firstLayout(true),
  inCommand(-1),
  activeMousePress(false),
	forceLayoutUpdate(false),
  defaultCmdMemory(-1),
  needShift(false),
  maxPage(0),
  activePage(0),
  showingMetal(false),
  buildSpacing(0),
  buildFacing(0),
  actionOffset(0),
  layoutHandler(NULL)
{
	icons = new IconInfo[16];
	iconsSize = 16;
	iconsCount = 0;

	LoadConfig("ctrlpanel.txt");

	readmap->mapDefParser.GetDef(autoShowMetal, "1", "MAP\\autoShowMetal");
}


CGuiHandler::~CGuiHandler()
{
	delete icons;
	delete layoutHandler;

	std::map<std::string, unsigned int>::iterator it;
	for (it = textureMap.begin(); it != textureMap.end(); ++it) {
		glDeleteTextures (1, &(it->second));
	}
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

	outlineFont.Enable(false);
}


static bool SafeAtoF(float& var, const string& value)
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

	SimpleParser::Init();

	string deadStr = "";
	string prevStr = "";
	string nextStr = "";
	string fillOrderStr = "";

	while (true) {
		const string line = SimpleParser::GetCleanLine(ifs);
		if (line.empty()) {
			break;   
		}

		vector<string> words = SimpleParser::Tokenize(line, 1);

		const string command = StringToLower(words[0]);

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
			outlineFont.Enable(!!atoi(words[1].c_str()));
		}
	}

	// sane clamps
	xIcons      = max(2,      xIcons);
	yIcons      = max(2,      yIcons);
	xIconSize   = max(0.010f, xIconSize);
	yIconSize   = max(0.010f, yIconSize);
	iconBorder  = max(0.0f,   iconBorder);
	frameBorder = max(0.0f,   frameBorder);

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

	// split the string into slot names
	std::vector<std::string> slotNames = SimpleParser::Tokenize(text, 0);
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


bool CGuiHandler::ReloadConfig()
{
	LoadConfig("ctrlpanel.txt");
	activePage = 0;
	selectedUnits.SetCommandPage(activePage);
	LayoutIcons(false);
	return true;
}


void CGuiHandler::ResizeIconArray(unsigned int size)
{
	int minIconsSize = iconsSize;
	while (minIconsSize < size) {
		minIconsSize *= 2;
	}
	if (iconsSize < minIconsSize) {
		iconsSize = minIconsSize;
		delete icons;
		icons = new IconInfo[iconsSize];
	}
}


void CGuiHandler::AppendPrevAndNext(vector<CommandDescription>& cmds)
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
	if ((inCommand < 0) || (inCommand >= commands.size())) {
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
                                  bool samePage)
{
	for (int a = 0; a < commands.size(); ++a) {
		if (commands[a].id == cmdDesc.id) {
			inCommand = a;
			if (commands[a].type == CMDTYPE_ICON_BUILDING) {
				UnitDef* ud = unitDefHandler->GetUnitByID(-commands[a].id);
				SetShowingMetal(ud->extractsMetal > 0);
			} else {
				SetShowingMetal(false);
			}
			if (samePage) {
				for (int ii = 0; ii < iconsCount; ii++) {
					if (inCommand == icons[ii].commandsID) {
						activePage = min(maxPage, (ii / iconsPerPage));;
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
	// copy the current command state	
	const bool validInCommand = (inCommand >= 0) && (inCommand < commands.size());
	CommandDescription cmdDesc;
	if (validInCommand) {
		cmdDesc = commands[inCommand];
	}
	const bool samePage = validInCommand && (activePage == FindInCommandPage());
	useSelectionPage = useSelectionPage && !samePage;

	// reset some of our state
	inCommand = -1;
	commands.clear();
	forceLayoutUpdate = false;

	// try using the custom layout handler
	if (firstLayout) {
		firstLayout = false;
		layoutHandler = CIconLayoutHandler::GetHandler("ctrlpanel.lua");
	}
	if (layoutHandler != NULL) {
		if (LayoutCustomIcons(useSelectionPage)) {
			if (validInCommand) {
				RevertToCmdDesc(cmdDesc, samePage);
			}
			return; // we're done here
		}
		else {
			delete layoutHandler;
			layoutHandler = NULL;
			LoadConfig("ctrlpanel.txt");
		}
	}

	// get the commands to process
	CSelectedUnits::AvailableCommandsStruct ac;
	ac = selectedUnits.GetAvailableCommands();

	vector<CommandDescription> hidden;
	vector<CommandDescription>::const_iterator cdi;

	// separate the visible/hidden icons	
	for (cdi = ac.commands.begin(); cdi != ac.commands.end(); ++cdi){
		if (cdi->onlyKey) {
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

	maxPage    = max(0, pageCount - 1);
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
		RevertToCmdDesc(cmdDesc, samePage);
	} else if (useSelectionPage) {
		activePage = min(maxPage, ac.commandPage);
	}
	activePage = min(maxPage, activePage);
}


bool CGuiHandler::LayoutCustomIcons(bool useSelectionPage)
{
	if (layoutHandler == NULL) {
		return false;
	}

	// get the commands to process
	CSelectedUnits::AvailableCommandsStruct ac;
	ac = selectedUnits.GetAvailableCommands();
	vector<CommandDescription> cmds = ac.commands;
	if (cmds.size() > 0) {
		AppendPrevAndNext(cmds);
	}

	// call for a custom layout
	int tmpXicons = xIcons;
	int tmpYicons = yIcons;
	vector<int> removeCmds;
	vector<CommandDescription> customCmds;
	vector<int> onlyTextureCmds;
	vector<CIconLayoutHandler::ReStringPair> reTextureCmds;
	vector<CIconLayoutHandler::ReStringPair> reNamedCmds;
	vector<CIconLayoutHandler::ReStringPair> reTooltipCmds;
	vector<CIconLayoutHandler::ReParamsPair> reParamsCmds;
	map<int, int> iconMap;

	if (!layoutHandler->LayoutIcons(tmpXicons, tmpYicons, cmds,
	                                removeCmds, customCmds,
	                                onlyTextureCmds, reTextureCmds,
	                                reNamedCmds, reTooltipCmds, reParamsCmds,
	                                iconMap, menuName)) {
		logOutput.Print("LayoutCustomIcons() lua call failed\n");
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
		if ((index >= 0) || (index < cmds.size())) {
			removeIDs.insert(index);
		} else {
			logOutput.Print("LayoutCustomIcons() skipping bad removeCmd (%i)\n",
			                index);
		}
	}
	// remove unwanted commands  (and mark all as onlyKey)
	vector<CommandDescription> tmpCmds;
	for (i = 0; i < cmds.size(); i++) {
		if (removeIDs.find(i) == removeIDs.end()) {
			cmds[i].onlyKey = true;
			tmpCmds.push_back(cmds[i]);
		}
	}
	cmds = tmpCmds;

	// add the custom commands
	for (i = 0; i < customCmds.size(); i++) {
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
			const map<int, string>& params = reParamsCmds[i].params;
			map<int, string>::const_iterator pit;
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
	vector<int> iconList;
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
			cmds[index].onlyKey = false;

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

	maxPage = max(0, pageCount - 1);
	if (useSelectionPage) {
		activePage = min(maxPage, ac.commandPage);
	} else {
		activePage = min(maxPage, activePage);
	}

	buttonBox.x1 = xPos;
	buttonBox.x2 = xPos + (frameBorder * 2.0f) + (xIcons * xIconStep);
	buttonBox.y1 = yPos;
	buttonBox.y2 = yPos + (frameBorder * 2.0f) + (yIcons * yIconStep);

	return true;
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
				gd->SetMetalTexture(mm->metalMap, mm->extractionMap, mm->metalPal, false);
				showingMetal = true;
			}
		}
	}
}


void CGuiHandler::Update()
{
	if (needShift && !keys[SDLK_LSHIFT]) {
		SetShowingMetal(false);
		inCommand=-1;
		needShift=false;
	}

	const bool commandsChanged = selectedUnits.CommandsChanged();

	bool handlerUpdate = false;
	if (layoutHandler != NULL) {
		if (!layoutHandler->UpdateLayout(handlerUpdate,
		                                 commandsChanged, activePage)) {
			delete layoutHandler;
			layoutHandler = NULL;
			handlerUpdate = false;
			LoadConfig("ctrlpanel.txt");
		}
	}

	if (commandsChanged) {
		defaultCmdMemory = -1;
		SetShowingMetal(false);
		LayoutIcons(true);
		fadein = 100;
	}
	else if (forceLayoutUpdate || handlerUpdate) {
		LayoutIcons(false);
	}

	if (fadein > 0) {
		fadein -= 5;
	}

	SetCursorIcon();
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::SetCursorIcon() const
{
	if (GetReceiverAt(mouse->lastx, mouse->lasty)) {
			mouse->cursorText="";
	} else {
		if (inCommand>=0 && inCommand<commands.size()) {
			mouse->cursorText=commands[guihandler->inCommand].name;
		} else {
			int defcmd;
			if(mouse->buttons[SDL_BUTTON_RIGHT].pressed && mouse->activeReceiver==this)
				defcmd=defaultCmdMemory;
			else
				defcmd=GetDefaultCommand(mouse->lastx,mouse->lasty);
			if(defcmd>=0 && defcmd<commands.size())
				mouse->cursorText=commands[defcmd].name;
			else
				mouse->cursorText="";
		}
	}
}


bool CGuiHandler::MousePress(int x,int y,int button)
{
	if(AboveGui(x,y)){
		activeMousePress=true;
		return true;
	}
	if(inCommand>=0){
		activeMousePress=true;
		return true;
	}
	if(button==SDL_BUTTON_RIGHT){
		activeMousePress=true;
		defaultCmdMemory=GetDefaultCommand(x,y);
		return true;
	}
	return false;
}


void CGuiHandler::MouseRelease(int x,int y,int button)
{
	if (activeMousePress) {
		activeMousePress=false;
	} else {
		return;
	}

	if(needShift && !keys[SDLK_LSHIFT]){
		SetShowingMetal(false);
		inCommand=-1;
		needShift=false;
	}

	const int iconPos = IconAtPos(x, y);
	const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;


//	logOutput << x << " " << y << " " << mouse->lastx << " " << mouse->lasty << "\n";

	if (button == SDL_BUTTON_RIGHT && iconCmd==-1) { // right click -> default cmd
		inCommand=defaultCmdMemory;//GetDefaultCommand(x,y);
		defaultCmdMemory=-1;
	}

	if ((iconCmd >= 0) && (iconCmd < commands.size())) {
		SetShowingMetal(false);
		lastKeySet.Reset();

		switch(commands[iconCmd].type){
			case CMDTYPE_ICON:{
				Command c;
				c.id=commands[iconCmd].id;
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;}
			case CMDTYPE_ICON_MODE:{
				int newMode = atoi(commands[iconCmd].params[0].c_str())+1;
				if (newMode>commands[iconCmd].params.size()-2) {
					newMode = 0;
				}

				// not really required
				char t[10];
				SNPRINTF(t, 10, "%d", newMode);
				commands[iconCmd].params[0]=t;

				Command c;
				c.id=commands[iconCmd].id;
				c.params.push_back(newMode);
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				forceLayoutUpdate = true;
				break;}
			case CMDTYPE_ICON_MAP:
			case CMDTYPE_ICON_AREA:
			case CMDTYPE_ICON_UNIT:
			case CMDTYPE_ICON_UNIT_OR_MAP:
			case CMDTYPE_ICON_FRONT:
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
				inCommand=iconCmd;
				break;

			case CMDTYPE_ICON_BUILDING:{
				UnitDef* ud=unitDefHandler->GetUnitByID(-commands[iconCmd].id);
				SetShowingMetal(ud->extractsMetal > 0);
				inCommand=iconCmd;
				break;}

			case CMDTYPE_COMBO_BOX:
				{
					inCommand=iconCmd;
					CommandDescription& cd=commands[iconCmd];
					list=new CglList(cd.name.c_str(),MenuSelection);
					for(vector<string>::iterator pi=++cd.params.begin();pi!=cd.params.end();++pi)
						list->AddItem(pi->c_str(),"");
					list->place=atoi(cd.params[0].c_str());
					game->showList=list;
					return;
				}
				inCommand=-1;
				break;
			case CMDTYPE_NEXT:
				{
					++activePage;
					if(activePage>maxPage)
						activePage=0;
				}
				selectedUnits.SetCommandPage(activePage);
				inCommand=-1;
				break;

			case CMDTYPE_PREV:
				{
					--activePage;
					if(activePage<0)
						activePage=maxPage;
				}
				selectedUnits.SetCommandPage(activePage);
				inCommand=-1;
				break;

			case CMDTYPE_CUSTOM:
				const bool rmb = (button == SDL_BUTTON_LEFT)? false : true;
				RunCustomCommands(commands[iconCmd].params, rmb);
				break;
		}
		return;
	}

	Command c=GetCommand(x,y,button,false);

	// if cmd_stop is returned it indicates that no good command could be found
	if (c.id != CMD_STOP) {
		selectedUnits.GiveCommand(c);
		lastKeySet.Reset();
	}

	FinishCommand(button);
}


int CGuiHandler::IconAtPos(int x, int y)
{
	const float fx = float(x - gu->screenxPos) / gu->screenx;
	const float fy = float(gu->screeny - y) / gu->screeny;

	if ((fx < buttonBox.x1) || (fx > buttonBox.x2) ||
	    (fy < buttonBox.y1) || (fy > buttonBox.y2)) {
		return -1;
	}

	int xSlot = int((fx - (buttonBox.x1 + frameBorder)) / xIconStep);
	int ySlot = int(((buttonBox.y2 - frameBorder) - fy) / yIconStep);
	xSlot = min(max(xSlot, 0), xIcons - 1);
	ySlot = min(max(ySlot, 0), yIcons - 1);
  const int ii = (activePage * iconsPerPage) + (ySlot * xIcons) + xSlot;
  if ((ii >= 0) && (ii < iconsCount)) {
		if ((fx > icons[ii].selection.x1) && (fx < icons[ii].selection.x2) &&
				(fy > icons[ii].selection.y2) && (fy < icons[ii].selection.y1)) {
			return ii;
		}
	}

	return -1;
}


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


static bool ParseCustomCmdMods(string& cmd, ModGroup& in, ModGroup& out)
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


void CGuiHandler::RunCustomCommands(const vector<string>& cmds, bool rmb)
{
	static int depth = 0;
	if (depth > 8) {
		return; // recursion protection
	}
	depth++;

	for (int p = 0; p < (int)cmds.size(); p++) {
		string copy = cmds[p];
		ModGroup inMods;  // must match for the action to execute
		ModGroup outMods; // controls the state of the modifiers  (ex: "group1")
		if (ParseCustomCmdMods(copy, inMods, outMods)) {
			if (CheckCustomCmdMods(rmb, inMods)) {
				const bool tmpAlt   = keys[SDLK_LALT];
				const bool tmpCtrl  = keys[SDLK_LCTRL];
				const bool tmpMeta  = keys[SDLK_LMETA];
				const bool tmpShift = keys[SDLK_LSHIFT];
				if (outMods.alt   == Required)  { keys[SDLK_LALT]   = 1; }
				if (outMods.alt   == Forbidden) { keys[SDLK_LALT]   = 0; }
				if (outMods.ctrl  == Required)  { keys[SDLK_LCTRL]  = 1; }
				if (outMods.ctrl  == Forbidden) { keys[SDLK_LCTRL]  = 0; }
				if (outMods.meta  == Required)  { keys[SDLK_LMETA]  = 1; }
				if (outMods.meta  == Forbidden) { keys[SDLK_LMETA]  = 0; }
				if (outMods.shift == Required)  { keys[SDLK_LSHIFT] = 1; }
				if (outMods.shift == Forbidden) { keys[SDLK_LSHIFT] = 0; }

				CKeyBindings::Action action(copy);
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
	if (iconsCount <= 0) {
		return false;
	}
	if (!selectThrough) {
		const float fx = float(x - gu->screenxPos) / gu->screenx;
		const float fy = float(gu->screeny - y) / gu->screeny;
		if ((fx > buttonBox.x1) && (fx < buttonBox.x2) &&
				(fy > buttonBox.y1) && (fy < buttonBox.y2)) {
			return true;
		}
	}
	return (IconAtPos(x,y) >= 0);
}


void CGuiHandler::CreateOptions(Command& c,bool rmb)
{
	c.options=0;
	if(rmb)
		c.options|=RIGHT_MOUSE_KEY;
	if(keys[SDLK_LSHIFT])
		c.options|=SHIFT_KEY;
	if(keys[SDLK_LCTRL])
		c.options|=CONTROL_KEY;
	if(keys[SDLK_LALT] || keys[SDLK_LMETA])
		c.options|=ALT_KEY;
	//logOutput << (int)c.options << "\n";
}

int CGuiHandler::GetDefaultCommand(int x,int y) const
{
	CInputReceiver* ir=GetReceiverAt(x,y);
	if(ir!=0)
		return -1;

	CUnit* unit=0;
	CFeature* feature=0;
	float dist=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4f,unit,20,true);
	float dist2=helper->GuiTraceRayFeature(camera->pos,mouse->dir,gu->viewRange*1.4f,feature);

	if(dist>gu->viewRange*1.4f-300 && dist2>gu->viewRange*1.4f-300 && unit==0){
		return -1;
	}

	if(dist>dist2)
		unit=0;
	else
		feature=0;

	// make sure the command is currently available
	int cmd_id = selectedUnits.GetDefaultCmd(unit,feature);
	for (int c = 0; c < (int)commands.size(); c++) {
		if (cmd_id == commands[c].id) {
			return c;
		}
	}
	return -1;
}


bool CGuiHandler::ProcessLocalActions(const CKeyBindings::Action& action)
{
	// do not process these actions if the control panel is not visible
	if (iconsCount <= 0) {
		return false;
	}

	// only process the build options while building
	// (conserve the keybinding space where we can)		
	if ((inCommand >= 0) && (inCommand < commands.size()) &&
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
		if ((iconCmd >= 0) && (iconCmd < commands.size())) {
			game->SetHotBinding(commands[iconCmd].action);
		}
		return true;
	}
	else if (action.command == "hotunbind") {
		const int iconPos = IconAtPos(mouse->lastx, mouse->lasty);
		const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;
		if ((iconCmd >= 0) && (iconCmd < commands.size())) {
			string cmd = "unbindaction " + commands[iconCmd].action;
			keyBindings->Command(cmd);
			logOutput.Print("%s", cmd.c_str());
		}
		return true;
	}
	else if (action.command == "showcommands") {
		// bonus command for debugging
		logOutput.Print("Available Commands:\n");
		for(int i = 0; i < commands.size(); ++i){
			logOutput.Print("  command: %i, id = %i, action = %s\n",
						 i, commands[i].id, commands[i].action.c_str());
		}
		// show the icon/command linkage
		logOutput.Print("Icon Linkage:\n");
		for(int ii = 0; ii < iconsCount; ++ii){
			logOutput.Print("  icon: %i, commandsID = %i\n",
			                ii, icons[ii].commandsID);
		}
		logOutput.Print("maxPage         = %i\n", maxPage);
		logOutput.Print("activePage      = %i\n", activePage);
		logOutput.Print("iconsSize       = %i\n", iconsSize);
		logOutput.Print("iconsCount      = %i\n", iconsCount);
		logOutput.Print("commands.size() = %i\n", commands.size());
		return true;
	}
	else if (action.command == "nextmenu") {
		activePage++;
		if (activePage > maxPage) {
			activePage = 0;
		}
		selectedUnits.SetCommandPage(activePage);
		return true;
	}
	else if (action.command == "prevmenu") {
		activePage--;
		if (activePage < 0) {
			activePage = maxPage;
		}
		selectedUnits.SetCommandPage(activePage);
		return true;
	}
	else if (action.command == "layout") {
		RunLayoutCommand(action.extra);
		return true;
	}

	return false;
}    


void CGuiHandler::RunLayoutCommand(const string& command)
{
	if (command == "reload") {
		if (layoutHandler == NULL) {
			logOutput.Print("Loading \"ctrlpanel.lua\"\n");
			layoutHandler = CIconLayoutHandler::GetHandler("ctrlpanel.lua");
			if (layoutHandler == NULL) {
				logOutput.Print("Loading failed\n");
			}
		} else {
			logOutput.Print("Reloading \"ctrlpanel.lua\"\n");
			delete layoutHandler;
			layoutHandler = CIconLayoutHandler::GetHandler("ctrlpanel.lua");
			if (layoutHandler == NULL) {
				logOutput.Print("Reloading failed\n");
				LoadConfig("ctrlpanel.txt");
			}
		}
	} else {
		if (layoutHandler == NULL) {
			logOutput.Print("Loading \"ctrlpanel.lua\"\n");
			layoutHandler = CIconLayoutHandler::GetHandler("ctrlpanel.lua");
			if (layoutHandler == NULL) {
				logOutput.Print("Loading failed\n");
			}
		}
		layoutHandler->ConfigCommand(command);
	}

	LayoutIcons(false);

	return;
}


bool CGuiHandler::ProcessBuildActions(const CKeyBindings::Action& action)
{
	const string arg = StringToLower(action.extra);
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
    

int CGuiHandler::GetIconPosCommand(int slot) const
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


bool CGuiHandler::KeyPressed(unsigned short key)
{
	if(key==SDLK_ESCAPE && activeMousePress){
		activeMousePress=false;
		inCommand=-1;
		SetShowingMetal(false);
		return true;
	}
	if(key==SDLK_ESCAPE && inCommand>0){
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
		const CKeyBindings::Action& action = al[actionIndex];

		if (ProcessLocalActions(action)) {
			return true;
		}

		// See if we have a positional icon command
		int iconCmd = -1;
		if (!action.extra.empty() && (action.command == "iconpos")) {
			const int iconSlot = ParseIconSlot(action.extra);
			iconCmd = GetIconPosCommand(iconSlot);
		}

		for (int a = 0; a < commands.size(); ++a) {

			if ((a != iconCmd) && (commands[a].action != action.command)) {
				continue; // not a match
			}

			const int cmdType = commands[a].type;

			// set the activePage
			if (!commands[a].onlyKey &&
			    (((cmdType == CMDTYPE_ICON) && 
			       ((commands[a].id < 0) ||
			        (commands[a].id == CMD_STOCKPILE))) ||
			     (cmdType == CMDTYPE_ICON_MODE) ||
			     (cmdType == CMDTYPE_COMBO_BOX) ||
			     (cmdType == CMDTYPE_ICON_BUILDING))) {
				for (int ii = 0; ii < iconsCount; ii++) {
					if (icons[ii].commandsID == a) {
						activePage = min(maxPage, (ii / iconsPerPage));
						selectedUnits.SetCommandPage(activePage);
					}
				}
			}

			switch(cmdType) {
				case CMDTYPE_ICON:{
					Command c;
					c.options = 0;
					c.id = commands[a].id;
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
							c.options = RIGHT_MOUSE_KEY |SHIFT_KEY | CONTROL_KEY;
						}
					}
					else if (c.id == CMD_WAIT) {
						if (action.extra == "queued") {
							c.options = SHIFT_KEY;
						}
					}
					selectedUnits.GiveCommand(c);
					break;
				}
				case CMDTYPE_ICON_MODE: {
					int newMode;

					if (!action.extra.empty() && (iconCmd < 0)) {
						newMode = atoi(action.extra.c_str());
					} else {
						newMode = atoi(commands[a].params[0].c_str()) + 1;
					}

					if ((newMode < 0) || (newMode > (commands[a].params.size() - 2))) {
						newMode = 0;
					}

					// not really required
					char t[10];
					SNPRINTF(t, 10, "%d", newMode);
					commands[a].params[0] = t;

					Command c;
					c.options = 0;
					c.id = commands[a].id;
					c.params.push_back(newMode);
					selectedUnits.GiveCommand(c);
					forceLayoutUpdate = true;
					break;
				}
				case CMDTYPE_ICON_MAP:
				case CMDTYPE_ICON_AREA:
				case CMDTYPE_ICON_UNIT:
				case CMDTYPE_ICON_UNIT_OR_MAP:
				case CMDTYPE_ICON_FRONT:
				case CMDTYPE_ICON_UNIT_OR_AREA:
				case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA: {
					SetShowingMetal(false);
					actionOffset = actionIndex;
					lastKeySet = ks;
					inCommand=a;
					break;
				}
				case CMDTYPE_ICON_BUILDING: {
					UnitDef* ud=unitDefHandler->GetUnitByID(-commands[a].id);
					SetShowingMetal(ud->extractsMetal > 0);
					actionOffset = actionIndex;
					lastKeySet = ks;
					inCommand=a;
					break;
				}
				case CMDTYPE_COMBO_BOX: {
					CommandDescription& cd = commands[a];
					vector<string>::iterator pi;
					// check for an action bound to a specific entry
					if (!action.extra.empty() && (iconCmd < 0)) {
						int p = 0;
						for (pi = ++cd.params.begin(); pi != cd.params.end(); ++pi) {
							if (action.extra == *pi) {
								Command c;
								c.options = 0;
								c.id = cd.id;
								c.params.push_back(p);
								selectedUnits.GiveCommand(c);
								return true;
							}
							p++;
						}
					}
					inCommand = a;
					list = new CglList(cd.name.c_str(), MenuSelection);
					for (pi = ++cd.params.begin(); pi != cd.params.end(); ++pi) {
						list->AddItem(pi->c_str(), "");
					}
					list->place = atoi(cd.params[0].c_str());
					game->showList = list;
					lastKeySet.Reset();
					SetShowingMetal(false);
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
					RunCustomCommands(commands[iconCmd].params, false);
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
	}

	return false;
}


void CGuiHandler::MenuSelection(std::string s)
{
	guihandler->MenuChoice(s);
}


void CGuiHandler::MenuChoice(string s)
{
	game->showList=0;
	delete list;
	if (inCommand>=0 && inCommand<commands.size()) {
		CommandDescription& cd=commands[inCommand];
		switch (cd.type) {
			case CMDTYPE_COMBO_BOX: {
				inCommand=-1;
				vector<string>::iterator pi;
				int a=0;
				for (pi=++cd.params.begin();pi!=cd.params.end();++pi) {
					if (*pi==s) {
						Command c;
						c.id=cd.id;
						c.params.push_back(a);
						CreateOptions(c,0);
						selectedUnits.GiveCommand(c);
						break;
					}
					++a;
				}
				break;
			}
		}
	}
}


void CGuiHandler::FinishCommand(int button)
{
	if(keys[SDLK_LSHIFT] && button==SDL_BUTTON_LEFT){
		needShift=true;
	} else {
		SetShowingMetal(false);
		inCommand=-1;
	}
}


bool CGuiHandler::IsAbove(int x, int y)
{
	return AboveGui(x,y);
}


std::string CGuiHandler::GetTooltip(int x, int y)
{
	string s;
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


std::string CGuiHandler::GetBuildTooltip() const
{
	if ((inCommand >= 0) && (inCommand < commands.size()) &&
	    (commands[inCommand].type == CMDTYPE_ICON_BUILDING)) {
		return commands[inCommand].tooltip;
	}
	return std::string("");
}


Command CGuiHandler::GetOrderPreview(void)
{
	return GetCommand(mouse->lastx,mouse->lasty,-1,true);
}


Command CGuiHandler::GetCommand(int mousex, int mousey, int buttonHint, bool preview)
{
	Command defaultRet;
	defaultRet.id=CMD_STOP;

	int button;
	if(buttonHint>=SDL_BUTTON_LEFT)
		button=buttonHint;
	else if(inCommand!=-1)
		button=SDL_BUTTON_LEFT;
	else if(mouse->buttons[SDL_BUTTON_RIGHT].pressed)
		button=SDL_BUTTON_RIGHT;
	else
		return defaultRet;

	int tempInCommand=inCommand;

	if (button == SDL_BUTTON_RIGHT && preview) { // right click -> default cmd, in preview we might not have default cmd memory set
		if(mouse->buttons[SDL_BUTTON_RIGHT].pressed)
			tempInCommand=defaultCmdMemory;
		else
			tempInCommand=GetDefaultCommand(mousex,mousey);
	}

	if(tempInCommand>=0 && tempInCommand<commands.size()){
		switch(commands[tempInCommand].type){

		case CMDTYPE_ICON:{
			Command c;
			c.id=commands[tempInCommand].id;
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			if(button==SDL_BUTTON_LEFT && !preview)
				logOutput.Print("CMDTYPE_ICON left button press in incommand test? This shouldnt happen");
			return c;}

		case CMDTYPE_ICON_MAP:{
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=camera->pos+mouse->dir*dist;
			Command c;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_BUILDING:{
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
			if(dist<0){
				return defaultRet;
			}
			UnitDef* unitdef = unitDefHandler->GetUnitByID(-commands[inCommand].id);

			if(!unitdef){
				return defaultRet;
			}

			float3 pos=camera->pos+mouse->dir*dist;
			std::vector<BuildInfo> buildPos;
			BuildInfo bi(unitdef, pos, buildFacing);
			if(keys[SDLK_LSHIFT] && button==SDL_BUTTON_LEFT){
				float dist=ground->LineGroundCol(mouse->buttons[SDL_BUTTON_LEFT].camPos,mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4f);
				float3 pos2=mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*dist;
				buildPos=GetBuildPos(BuildInfo(unitdef,pos2,buildFacing),bi);
			} else 
				buildPos=GetBuildPos(bi,bi);

			if(buildPos.empty()){
				return defaultRet;
			}

			int a=0;		//limit the number of max commands possible to send to avoid overflowing the network buffer
			for(std::vector<BuildInfo>::iterator bpi=buildPos.begin();bpi!=--buildPos.end() && a<200;++bpi){
				++a;
				Command c;
				bpi->FillCmd(c);
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				if(!preview)
					selectedUnits.GiveCommand(c);
			}
			Command c;
			buildPos.back().FillCmd(c);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT: {
			CUnit* unit=0;
			Command c;

			c.id=commands[tempInCommand].id;
			float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4f,unit,20,true);
			if (!unit){
				return defaultRet;
			}
			c.params.push_back(unit->id);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT_OR_MAP: {

			Command c;
			c.id=commands[tempInCommand].id;

			CUnit* unit=0;
			float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4f,unit,20,true);
			if(dist2>gu->viewRange*1.4f-300){
				return defaultRet;
			}

			if (unit!=0) {  // clicked on unit
				c.params.push_back(unit->id);
			} else { // clicked in map
				float3 pos=camera->pos+mouse->dir*dist2;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_FRONT:{
			Command c;

			float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*gu->viewRange*1.4f);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);

			if(mouse->buttons[button].movement>30){		//only create the front if the mouse has moved enough
				dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2=camera->pos+mouse->dir*dist;
				if(!commands[tempInCommand].params.empty() && pos.distance2D(pos2)>atoi(commands[tempInCommand].params[0].c_str())){
					float3 dif=pos2-pos;
					dif.Normalize();
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

			if(mouse->buttons[button].movement<4){
				CUnit* unit=0;
				CFeature* feature=0;
				float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4f,unit,20,true);
				float dist3=helper->GuiTraceRayFeature(camera->pos,mouse->dir,gu->viewRange*1.4f,feature);

				if(dist2>gu->viewRange*1.4f-300 && (commands[tempInCommand].type!=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA || dist3>gu->viewRange*1.4f-300)){
					return defaultRet;
				}

				if (feature!=0 && dist3<dist2 && commands[tempInCommand].type==CMDTYPE_ICON_UNIT_FEATURE_OR_AREA) {  // clicked on feature
					c.params.push_back(MAX_UNITS+feature->id);
				} else if (unit!=0 && commands[tempInCommand].type!=CMDTYPE_ICON_AREA) {  // clicked on unit
					c.params.push_back(unit->id);
				} else { // clicked in map
					float3 pos=camera->pos+mouse->dir*dist2;
					c.params.push_back(pos.x);
					c.params.push_back(pos.y);
					c.params.push_back(pos.z);
					c.params.push_back(0);//zero radius
				}
			} else {	//created area
				float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*gu->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
				dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2=camera->pos+mouse->dir*dist;
				c.params.push_back(min(maxRadius,pos.distance2D(pos2)));
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
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


// Assuming both builds have the same unitdef
std::vector<BuildInfo> CGuiHandler::GetBuildPos(const BuildInfo& startInfo, const BuildInfo& endInfo)
{
	std::vector<BuildInfo> ret;

	float3 start=helper->Pos2BuildPos(startInfo);
	float3 end=helper->Pos2BuildPos(endInfo);

	BuildInfo other; // the unit around which buildings can be circled
	if(keys[SDLK_LSHIFT] && keys[SDLK_LCTRL])
	{
		CUnit* unit=0;
		float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4f,unit,20,true);
		if(unit){
			other.def = unit->unitDef;
			other.pos = unit->pos;
			other.buildFacing = unit->buildFacing;
		} else {
			Command c = uh->GetBuildCommand(camera->pos,mouse->dir);
			if(c.id < 0){
				assert(c.params.size()==4);
				other.pos = float3(c.params[0],c.params[1],c.params[2]);
				other.def = unitDefHandler->GetUnitByID(-c.id);
				other.buildFacing = int(c.params[3]);
			}
		}
	}

	if(other.def && keys[SDLK_LSHIFT] && keys[SDLK_LCTRL]){		//circle build around building
		Command c;

		start=end=helper->Pos2BuildPos(other);
		start.x-=(other.GetXSize()/2)*SQUARE_SIZE;
		start.z-=(other.GetYSize()/2)*SQUARE_SIZE;
		end.x+=(other.GetXSize()/2)*SQUARE_SIZE;
		end.z+=(other.GetYSize()/2)*SQUARE_SIZE;

		float3 pos=start;
		int xsize=startInfo.GetXSize();
		int ysize=startInfo.GetYSize();
		for(;pos.x<=end.x;pos.x+=xsize*SQUARE_SIZE){
			BuildInfo bi(startInfo.def,pos,startInfo.buildFacing);
			bi.pos.x+=(xsize/2)*SQUARE_SIZE;
			bi.pos.z-=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui)
			{
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
		pos=end;
		pos.x=start.x;
		for(;pos.z>=start.z;pos.z-=ysize*SQUARE_SIZE){
			BuildInfo bi(startInfo.def,pos,(startInfo.buildFacing+1)%4);
			bi.pos.x-=(xsize/2)*SQUARE_SIZE;
			bi.pos.z-=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui){
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
		pos=end;
		for(;pos.x>=start.x;pos.x-=xsize*SQUARE_SIZE) {
			BuildInfo bi(startInfo.def,pos,(startInfo.buildFacing+2)%4);
			bi.pos.x-=(xsize/2)*SQUARE_SIZE;
			bi.pos.z+=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui)
			{
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
		pos=start;
		pos.x=end.x;
		for(;pos.z<=end.z;pos.z+=ysize*SQUARE_SIZE){
			BuildInfo bi(startInfo.def,pos,(startInfo.buildFacing+3)%4);
			bi.pos.x+=(xsize/2)*SQUARE_SIZE;
			bi.pos.z+=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui)
			{
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
	} else if(keys[SDLK_LALT]){			//build a rectangle
		float xsize=startInfo.GetXSize()*8+buildSpacing*16;
		int xnum=(int)((fabs(end.x-start.x)+xsize*1.4f)/xsize);
		int xstep=(int)xsize;
		if(start.x>end.x)
			xstep*=-1;

		float zsize=startInfo.GetYSize()*8+buildSpacing*16;
		int znum=(int)((fabs(end.z-start.z)+zsize*1.4f)/zsize);
		int zstep=(int)zsize;
		if(start.z>end.z)
			zstep*=-1;

		int zn=0;
		for(float z=start.z;zn<znum;++zn){
			int xn=0;
			for(float x=start.x;xn<xnum;++xn){
				if(!keys[SDLK_LCTRL] || zn==0 || xn==0 || zn==znum-1 || xn==xnum-1){
					BuildInfo bi = startInfo;
					bi.pos = float3(x,0,z);
					bi.pos=helper->Pos2BuildPos(bi);
					ret.push_back(bi);
				}
				x+=xstep;
			}
			z+=zstep;
		}
	} else {			//build a line
		BuildInfo bi=startInfo;
		if(fabs(start.x-end.x)>fabs(start.z-end.z)){
			float step=startInfo.GetXSize()*8+buildSpacing*16;
			float3 dir=end-start;
			if(dir.x==0){
				bi.pos = start;
				ret.push_back(bi);
				return ret;
			}
			dir/=fabs(dir.x);
			if(keys[SDLK_LCTRL])
				dir.z=0;
			for(float3 p=start;fabs(p.x-start.x)<fabs(end.x-start.x)+step*0.4f;p+=dir*step) {
				bi.pos=p;
				ret.push_back(bi);
			}
		} else {
			float step=startInfo.GetYSize()*8+buildSpacing*16;
			float3 dir=end-start;
			if(dir.z==0){
				bi.pos=start;
				ret.push_back(bi);
				return ret;
			}
			dir/=fabs(dir.z);
			if(keys[SDLK_LCTRL])
				dir.x=0;
			for(float3 p=start;fabs(p.z-start.z)<fabs(end.z-start.z)+step*0.4f;p+=dir*step) {
				bi.pos=p;
				ret.push_back(bi);
			}
		}
	}
	return ret;
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::Draw()
{
	DrawButtons();
}


bool CGuiHandler::BindNamedTexture(const std::string& texName)
{
	if (texName.empty()) {
		return false;
	}

	std::map<std::string, unsigned int>::iterator it = textureMap.find(texName);
	if (it != textureMap.end()) {
		const GLuint texID = it->second;
		if (texID == 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		} else {
			glBindTexture(GL_TEXTURE_2D, it->second);
			return true;
		}
	}

	// strip off the qualifiers
	string filename = texName;
	bool nearest = false;
	if (filename.find(":n:") == 0) {
		nearest = true;
		filename = filename.substr(strlen(":n:"));
	}

	// get the image
	CBitmap bitmap;
	if (!bitmap.Load(filename)) {
		textureMap[texName] = 0;
		glBindTexture(GL_TEXTURE_2D, 0);
		return false;
	}

	// make the texture
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	if (nearest) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		if (GLEW_ARB_texture_non_power_of_two) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			             bitmap.xsize, bitmap.ysize, 0,
			             GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
		} else {
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize,
			                  GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize,
											GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
	}

	textureMap[texName] = texID;

	return true;
}


static string FindCornerText(const string& corner, const vector<string>& params)
{
	for (int p = 0; p < (int)params.size(); p++) {
		if (params[p].find(corner) == 0) {
			return params[p].substr(corner.length());
		}
	}
	return string("");
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
		DrawName(icon, cmdDesc.name);
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
	// UnitDefHandler's array size is (numUnits + 1)
	if ((unitDefID <= 0) || (unitDefID > unitDefHandler->numUnits)) {
		return false;
	}
	UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud != NULL) {
		const Box& b = icon.visual;
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, textureAlpha);
		glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitImage(ud));
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


bool CGuiHandler::DrawTexture(const IconInfo& icon, const std::string& texName)
{
	if (texName.empty()) {
		return false;
	}

	// plain texture
	if (texName[0] != '#') {
		if (BindNamedTexture(texName)) {
			const Box& b = icon.visual;
			glEnable(GL_TEXTURE_2D);
			glColor4f(1.0f, 1.0f, 1.0f, textureAlpha);
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

	// unit build picture
	char* endPtr;
	const char* startPtr = texName.c_str() + 1; // skip the '#'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
	if (endPtr == startPtr) {
		return false; // bad unitID spec
	}
	if (endPtr[0] == '\0') {
		return DrawUnitBuildIcon(icon, unitDefID);
	}

	// texture and unit build picture
	if (endPtr[0] != ',') {
		return false;
	}
	startPtr = endPtr + 1; // skip the ','
	const float xscale = (float)strtod(startPtr, &endPtr);
	if (endPtr == startPtr) {
		return false;
	}

	float yscale;
	if (endPtr[0] == ',') {
		endPtr++; // setup for the texture name
		yscale = xscale;
	}
	else if (endPtr[0] != 'x') {
		return false;
	}
	else {
		startPtr = endPtr + 1;
		yscale = (float)strtod(startPtr, &endPtr);
		if ((endPtr == startPtr) || (endPtr[0] != ',')) {
			return false;
		}
		endPtr++; // setup for the texture name
	}

	const Box& iv = icon.visual;
	IconInfo tmpIcon;
	Box& tv = tmpIcon.visual;
	tv.x1 = iv.x1 + (xIconSize * xscale);
	tv.x2 = iv.x2 - (xIconSize * xscale);
	tv.y1 = iv.y1 - (yIconSize * yscale);
	tv.y2 = iv.y2 + (yIconSize * yscale);

	if (!BindNamedTexture(endPtr)) {
		return DrawUnitBuildIcon(icon, unitDefID);
	}
	else {
		const Box& b = icon.visual;
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, textureAlpha);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(b.x1, b.y1);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(b.x2, b.y1);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(b.x2, b.y2);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(b.x1, b.y2);
		glEnd();

		return DrawUnitBuildIcon(tmpIcon, unitDefID);
	}

	return false; // safety
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


void CGuiHandler::DrawName(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;

	const float tWidth  = font->CalcTextWidth(text.c_str());
	const float tHeight = font->CalcTextHeight(text.c_str());
	const float textBorder2 = (2.0f * textBorder);
	float xScale = (xIconSize - textBorder2) / tWidth;
	float yScale = (yIconSize - textBorder2) / tHeight;

	const float yRatio = xScale * gu->aspectRatio;
	if (yRatio < yScale) {
		yScale = yRatio;
	} else {
		const float xRatio = yScale / gu->aspectRatio;
		if (xRatio < xScale) {
			xScale = xRatio;
		}
	}

	const float xCenter = 0.5f * (b.x1 + b.x2);
	const float yCenter = 0.5f * (b.y1 + b.y2);
	const float xStart  = xCenter - (0.5f * xScale * tWidth);
	const float yStart  = yCenter - (0.5f * yScale * (1.25f * tHeight));
	const float dShadow = 0.002f;			

	if (dropShadows) {
		glPushMatrix();
		glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
		glTranslatef(xStart + dShadow, yStart - dShadow, 0.0f);
		glScalef(xScale, yScale, 1.0f);
		font->glPrint("%s",text.c_str());
		glPopMatrix();
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTranslatef(xStart, yStart, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	font->glPrintColor("%s",text.c_str());

	glLoadIdentity();
}


void CGuiHandler::DrawNWtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tHeight = font->CalcTextHeight(text.c_str());
	const float yScale = (yIconSize * 0.2f) / tHeight;
	const float xScale = yScale / gu->aspectRatio;
	const float xPos = b.x1 + textBorder + 0.002f;
	const float yPos = b.y1 - textBorder - (yScale * tHeight) - 0.006f;
	glTranslatef(xPos, yPos, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	font->glPrintColor("%s", text.c_str());
	glLoadIdentity();
}


void CGuiHandler::DrawSWtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tHeight = font->CalcTextHeight(text.c_str());
	const float yScale = (yIconSize * 0.2f) / tHeight;
	const float xScale = yScale / gu->aspectRatio;
	const float xPos = b.x1 + textBorder + 0.002f;
	const float yPos = b.y2 + textBorder + 0.002f;
	glTranslatef(xPos, yPos, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	font->glPrintColor("%s", text.c_str());
	glLoadIdentity();
}


void CGuiHandler::DrawNEtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tWidth = font->CalcTextWidth(text.c_str());
	const float tHeight = font->CalcTextHeight(text.c_str());
	const float yScale = (yIconSize * 0.2f) / tHeight;
	const float xScale = yScale / gu->aspectRatio;
	const float xPos = b.x2 - textBorder - (xScale * tWidth) - 0.002f;
	const float yPos = b.y1 - textBorder - (yScale * tHeight) - 0.006f;
	glTranslatef(xPos, yPos, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	font->glPrintColor("%s", text.c_str());
	glLoadIdentity();
}


void CGuiHandler::DrawSEtext(const IconInfo& icon, const std::string& text)
{
	if (text.empty()) {
		return;
	}
	const Box& b = icon.visual;
	const float tWidth = font->CalcTextWidth(text.c_str());
	const float tHeight = font->CalcTextHeight(text.c_str());
	const float yScale = (yIconSize * 0.2f) / tHeight;
	const float xScale = yScale / gu->aspectRatio;
	const float xPos = b.x2 - textBorder - (xScale * tWidth) - 0.002f;
	const float yPos = b.y2 + textBorder + 0.002f;
	glTranslatef(xPos, yPos, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	font->glPrintColor("%s", text.c_str());
	glLoadIdentity();
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
	glVertex2f(b.x1, b.y1);
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void CGuiHandler::DrawButtons()
{
	Update();

	if (iconsCount <= 0) {
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
	glAlphaFunc(GL_GEQUAL, 0.005f);

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
		glVertex2f(buttonBox.x1 - fx, buttonBox.y1);
		glEnd();
	}

	const int mouseIcon   = IconAtPos(mouse->lastx, mouse->lasty);
	const int buttonStart = min(iconsCount, activePage * iconsPerPage);
	const int buttonEnd   = min(iconsCount, buttonStart + iconsPerPage);

	for (int ii = buttonStart; ii < buttonEnd; ii++) {

		const IconInfo& icon = icons[ii];
		if (icon.commandsID < 0) {
			continue; // inactive icon
		}
		const CommandDescription& cmdDesc = commands[icon.commandsID];
		const bool highlight = (mouseIcon == ii) || (icon.commandsID == inCommand);

		if ((cmdDesc.id == CMD_INTERNAL) && (cmdDesc.type == CMDTYPE_CUSTOM)) {
			DrawCustomButton(icon, highlight);
		}
		else {
			bool usedTexture = false;
			bool onlyTexture = cmdDesc.onlyTexture;

			// specified texture
			if (DrawTexture(icon, cmdDesc.iconname)) {
				usedTexture = true;
			}

			// unit buildpic
			if (!usedTexture) {
				if (cmdDesc.id < 0) {
					UnitDef* ud = unitDefHandler->GetUnitByID(-cmdDesc.id);
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

			// build count text
			if ((cmdDesc.id < 0) && !cmdDesc.params.empty()) {
				DrawSWtext(icon, cmdDesc.params[0]);
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
			}

			// draw the text
			if (!usedTexture || !onlyTexture) {
				// command name (or parameter)
				string toPrint = cmdDesc.name;
				if (cmdDesc.type == CMDTYPE_ICON_MODE) {
					const int opt = atoi(cmdDesc.params[0].c_str()) + 1;
					if (opt < cmdDesc.params.size()) {
						toPrint = cmdDesc.params[opt];
					}
				}
				DrawName(icon, toPrint);
			}

			// draw the mode indicators
			if (useOptionLEDs && (cmdDesc.type == CMDTYPE_ICON_MODE)) {
				DrawOptionLEDs(icon);
			}
		}

		// highlight outline
		if (highlight) {
			if (icon.commandsID == inCommand) {
				glColor4f(1.0f, 1.0f, 0.0f, 0.75f);
			} else if (mouse->buttons[SDL_BUTTON_LEFT].pressed) {
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
	if (layoutHandler == NULL) {
		char buf[64];
		SNPRINTF(buf, 64, "%i", activePage + 1);
		if (selectedUnits.BuildIconsFirst()) {
			glColor4fv(cmdColors.build);
		} else {
			glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
		}
		const float textSize = 1.2f;
		const float yBbot =
			yBpos - (textSize * 0.5f * (font->CalcTextHeight(buf) / 32.0f));
		font->glPrintCentered(xBpos, yBbot, textSize, buf);
	}

	DrawMenuName();

	DrawSelectionInfo();

	glPopAttrib();
}


void CGuiHandler::DrawMenuName()
{
	if (!menuName.empty() && (iconsCount > 0)) {
		const char* text = menuName.c_str();

		const float xScale = 0.0225f;
		const float yScale = xScale * gu->aspectRatio;
		const float textWidth  = xScale * font->CalcTextWidth(text);
		const float textHeight = yScale * font->CalcTextHeight(text);
		const float xp = 0.5f * (buttonBox.x1 + buttonBox.x2 - textWidth);  
		const float yp = buttonBox.y2 + (yIconSize * 0.125f);

		if (!outlineFont.IsEnabled()) {
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f, 0.2f, 0.2f, guiAlpha);
			glRectf(buttonBox.x1,
							buttonBox.y2,
							buttonBox.x2,
							buttonBox.y2 + (textHeight * 1.25f) + (yIconSize * 0.250f));
			glTranslatef(xp, yp, 0.0f);
			glScalef(xScale, yScale, 1.0f);
			font->glPrintColor("%s", text);
		}
		else {
			glTranslatef(xp, yp, 0.0f);
			glScalef(xScale, yScale, 1.0f);
			const float xPixel  = 1.0f / (xScale * (float)gu->screenx);
			const float yPixel  = 1.0f / (yScale * (float)gu->screeny);
			// use (alpha == 0.0) so that we only get the outline
			const float white[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
			outlineFont.print(xPixel, yPixel, white, text);
			font->glPrintColor("%s", text); // draw with color
		}
		glLoadIdentity();
	}

}


void CGuiHandler::DrawSelectionInfo()
{
	if (!selectedUnits.selectedUnits.empty()) {
		char buf[64];

		if(selectedUnits.selectedGroup!=-1){
			SNPRINTF(buf, 64, "Selected units %i  [Group %i]",
			         selectedUnits.selectedUnits.size(),
			         selectedUnits.selectedGroup);
		} else {
			SNPRINTF(buf, 64, "Selected units %i",
			         selectedUnits.selectedUnits.size());
		}

		const float xScale = 0.015f;
		const float yScale = xScale * gu->aspectRatio;
		const float textWidth  = xScale * font->CalcTextWidth(buf);
		const float textHeight = yScale * font->CalcTextHeight(buf);

		if (!outlineFont.IsEnabled()) {
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f, 0.2f, 0.2f, guiAlpha);
			glRectf(xSelectionPos - frameBorder,
							ySelectionPos - frameBorder,
							xSelectionPos + frameBorder + textWidth,
							ySelectionPos + frameBorder + (textHeight * 1.2f));
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			glTranslatef(xSelectionPos, ySelectionPos, 0.0f);
			glScalef(xScale, yScale, 1.0f);
			font->glPrint("%s", buf);
		}
		else {
			glTranslatef(xSelectionPos, ySelectionPos, 0.0f);
			glScalef(xScale, yScale, 1.0f);

			const float xPixel  = 1.0f / (xScale * (float)gu->screenx);
			const float yPixel  = 1.0f / (yScale * (float)gu->screeny);

			const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			outlineFont.print(xPixel, yPixel, white, buf);
		}
		glLoadIdentity();
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
	const float y1 = icon.visual.y1;
	const float x2 = icon.visual.x2;
	const float y2 = icon.visual.y2;
	const float yp = 1.0f / float(gu->screeny);

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
		const float starty = y2 + (3.0f * yp);

		glRectf(startx, starty, startx + xs, starty + ys);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		glRectf(startx, starty, startx + xs, starty + ys);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


/******************************************************************************/
/******************************************************************************/

void CGuiHandler::DrawMapStuff(void)
{
	if(activeMousePress){
		int cc=-1;
		int button=SDL_BUTTON_LEFT;
		if(inCommand!=-1){
			cc=inCommand;
		} else {
			if(mouse->buttons[SDL_BUTTON_RIGHT].pressed && mouse->activeReceiver==this){
				cc=defaultCmdMemory;//GetDefaultCommand(mouse->lastx,mouse->lasty);
				button=SDL_BUTTON_RIGHT;
			}
		}

		if(cc>=0 && cc<commands.size()){
			switch(commands[cc].type){
			case CMDTYPE_ICON_FRONT:
				if(mouse->buttons[button].movement>30){
					if(commands[cc].params.size()>0)
						if(commands[cc].params.size()>1)
							DrawFront(button,atoi(commands[cc].params[0].c_str()),atof(commands[cc].params[1].c_str()));
						else
							DrawFront(button,atoi(commands[cc].params[0].c_str()),0);
					else
						DrawFront(button,1000,0);
				}
				break;
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
			case CMDTYPE_ICON_AREA:{
				float maxRadius=100000;
				if(commands[cc].params.size()==1)
					maxRadius=atof(commands[cc].params[0].c_str());
				if(mouse->buttons[button].movement>4){
					float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*gu->viewRange*1.4f);
					if(dist<0){
						break;
					}
					float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
					dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
					if(dist<0){
						break;
					}
					float3 pos2=camera->pos+mouse->dir*dist;
					DrawArea(pos,min(maxRadius,pos.distance2D(pos2)));
				}
				break;}
			default:
				break;
			}
		}
	}

	glBlendFunc((GLenum)cmdColors.SelectedBlendSrc(),
	            (GLenum)cmdColors.SelectedBlendDst());
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(cmdColors.SelectedLineWidth());

	// draw buildings we are about to build
	if ((inCommand >= 0) && (inCommand < commands.size()) &&
	    (commands[inCommand].type == CMDTYPE_ICON_BUILDING)) {

		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
		if(dist>0){
			UnitDef* unitdef = unitDefHandler->GetUnitByID(-commands[inCommand].id);

			if(unitdef){
				float3 pos=camera->pos+mouse->dir*dist;
				std::vector<BuildInfo> buildPos;
				if(keys[SDLK_LSHIFT] && mouse->buttons[SDL_BUTTON_LEFT].pressed){
					float dist=ground->LineGroundCol(mouse->buttons[SDL_BUTTON_LEFT].camPos,mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4f);
					float3 pos2=mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*dist;
					buildPos=GetBuildPos(BuildInfo(unitdef, pos2,buildFacing), BuildInfo(unitdef,pos, buildFacing));
				} else {
					BuildInfo bi(unitdef, pos, buildFacing);
					buildPos=GetBuildPos(bi,bi);
				}

				for(std::vector<BuildInfo>::iterator bpi=buildPos.begin();bpi!=buildPos.end();++bpi)
				{
					const float3 buildpos = bpi->pos;
					std::vector<Command> cv;
					if(keys[SDLK_LSHIFT]){
						Command c;
						bpi->FillCmd(c);
						std::vector<Command> temp;
						std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
						for(; ui != selectedUnits.selectedUnits.end(); ui++){
							temp = (*ui)->commandAI->GetOverlapQueued(c);
							std::vector<Command>::iterator ti = temp.begin();
							for(; ti != temp.end(); ti++)
								cv.insert(cv.end(),*ti);
						}
					}
					if(uh->ShowUnitBuildSquare(*bpi, cv))
						glColor4f(0.7f,1,1,0.4f);
					else
						glColor4f(1,0.5f,0.5f,0.4f);

					unitDrawer->DrawBuildingSample(bpi->def, gu->myTeam, buildpos, bpi->buildFacing);

					glBlendFunc((GLenum)cmdColors.SelectedBlendSrc(),
											(GLenum)cmdColors.SelectedBlendDst());

					if(unitdef->weapons.size() > 0){	// draw weapon range
						glColor4fv(cmdColors.rangeAttack);
						glBegin(GL_LINE_STRIP);
						for(int a=0;a<=40;++a){
							float3 pos(cos(a*2*PI/40)*unitdef->weapons[0].def->range,0,sin(a*2*PI/40)*unitdef->weapons[0].def->range);
							pos+=buildpos;
							float dh=ground->GetHeight(pos.x,pos.z)-buildpos.y;
							float modRange=unitdef->weapons[0].def->range-dh*unitdef->weapons[0].def->heightmod;
							pos=float3(cos(a*2*PI/40)*(modRange),0,sin(a*2*PI/40)*(modRange));
							pos+=buildpos;
							pos.y=ground->GetHeight(pos.x,pos.z)+8;
							glVertexf3(pos);
						}
						glEnd();
					}
					if(unitdef->extractRange > 0){	// draw extraction range
						glDisable(GL_TEXTURE_2D);
						glColor4fv(cmdColors.rangeExtract);
						glBegin(GL_LINE_STRIP);
						for(int a=0;a<=40;++a){
							float3 pos(cos(a*2*PI/40)*unitdef->extractRange,0,sin(a*2*PI/40)*unitdef->extractRange);
							pos+=buildpos;
							pos.y=ground->GetHeight(pos.x,pos.z)+8;
							glVertexf3(pos);
						}
						glEnd();
					}
					if (unitdef->builder && !unitdef->canmove) { // draw build range
						const float radius = unitdef->buildDistance;
						if (radius > 0.0f) {
							glDisable(GL_TEXTURE_2D);
							glColor4fv(cmdColors.rangeBuild);
							glBegin(GL_LINE_STRIP);
							for(int a = 0; a <= 40; ++a){
								const float radians = a * (2.0f * PI) / 40.0f;
								float3 pos(cos(radians)*radius, 0.0f, sin(radians)*radius);
								pos += buildpos;
								pos.y = ground->GetHeight(pos.x, pos.z) + 8.0f;
								glVertexf3(pos);
							}
							glEnd();
						}
					}
				}
			}
		}
	}

	// draw the ranges for the unit that is being pointed at
	if(keys[SDLK_LSHIFT]){
		CUnit* unit=0;
		float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4f,unit,20,false);
		if(unit && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating)){		//draw weapon range
			if(unit->maxRange>0){
				glColor4fv(cmdColors.rangeAttack);
				glBegin(GL_LINE_STRIP);
				float h=unit->pos.y;
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->maxRange,0,sin(a*2*PI/40)*unit->maxRange);
					pos+=unit->pos;
					float dh=ground->GetHeight(pos.x,pos.z)-h;
					pos=float3(cos(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod),0,sin(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod));
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
			}
			if(unit->unitDef->decloakDistance > 0){			//draw decloak distance
				glColor4fv(cmdColors.rangeDecloak);
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->unitDef->decloakDistance,0,sin(a*2*PI/40)*unit->unitDef->decloakDistance);
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
			}
			if(unit->unitDef->kamikazeDist > 0){			//draw self destruct and damage distance
				glColor4fv(cmdColors.rangeKamikaze);
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->unitDef->kamikazeDist,0,sin(a*2*PI/40)*unit->unitDef->kamikazeDist);
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
				if(!unit->unitDef->selfDExplosion.empty()){
					glColor4fv(cmdColors.rangeSelfDestruct);
					WeaponDef* wd=weaponDefHandler->GetWeapon(unit->unitDef->selfDExplosion);

					glBegin(GL_LINE_STRIP);
					for(int a=0;a<=40;++a){
						float3 pos(cos(a*2*PI/40)*wd->areaOfEffect,0,sin(a*2*PI/40)*wd->areaOfEffect);
						pos+=unit->pos;
						pos.y=ground->GetHeight(pos.x,pos.z)+8;
						glVertexf3(pos);
					}
					glEnd();
				}
			}
			// draw build distance for immobile builders
			if(unit->unitDef->builder && !unit->unitDef->canmove) {
				const float radius = unit->unitDef->buildDistance;
				if (radius > 0.0f) {
					glColor4fv(cmdColors.rangeBuild);
					glBegin(GL_LINE_STRIP);
					for(int a = 0; a <= 40; ++a){
						const float radians = a * (2.0f * PI) / 40.0f;
						float3 pos(cos(radians)*radius, 0.0f, sin(radians)*radius);
						pos += unit->pos;
						pos.y = ground->GetHeight(pos.x, pos.z) + 8.0f;
						glVertexf3(pos);
					}
					glEnd();
				}
			}
		}
	}

	//draw range circles if attack orders are imminent
	int defcmd=GetDefaultCommand(mouse->lastx,mouse->lasty);
	if((inCommand>0 && inCommand<commands.size() && commands[inCommand].id==CMD_ATTACK) ||
	   (inCommand==-1 && defcmd>0 && commands[defcmd].id==CMD_ATTACK)){
		for(std::set<CUnit*>::iterator si=selectedUnits.selectedUnits.begin();si!=selectedUnits.selectedUnits.end();++si){
			CUnit* unit=*si;
			if(unit->maxRange>0 && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating)){
				glColor4fv(cmdColors.rangeAttack);
				glBegin(GL_LINE_STRIP);
				float h=unit->pos.y;
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->maxRange,0,sin(a*2*PI/40)*unit->maxRange);
					pos+=unit->pos;
					float dh=ground->GetHeight(pos.x,pos.z)-h;
					pos=float3(cos(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod),0,sin(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod));
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
			}
		}
	}

	glLineWidth(1.0f);
}


void CGuiHandler::DrawArea(float3 pos, float radius)
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5f,1,0.5f,0.5f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glBegin(GL_TRIANGLE_FAN);
		glVertexf3(pos);
		for(int a=0;a<=40;++a){
			float3 p(cos(a*2*PI/40)*radius,0,sin(a*2*PI/40)*radius);
			p+=pos;
			p.y=ground->GetHeight(p.x,p.z);
			glVertexf3(p);
		}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
}


void CGuiHandler::DrawFront(int button,float maxSize,float sizeDiv)
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5f,1,0.5f,0.5f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	CMouseHandler::ButtonPress& bp=mouse->buttons[button];
	if(bp.movement<5)
		return;
	float dist=ground->LineGroundCol(bp.camPos,bp.camPos+bp.dir*gu->viewRange*1.4f);
	if(dist<0)
		return;
	float3 pos1=bp.camPos+bp.dir*dist;
	dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
	if(dist<0)
		return;
	float3 pos2=camera->pos+mouse->dir*dist;
	float3 forward=(pos1-pos2).cross(UpVector);
	forward.Normalize();
	float3 side=forward.cross(UpVector);
	if(pos1.distance2D(pos2)>maxSize){
		pos2=pos1+side*maxSize;
		pos2.y=ground->GetHeight(pos2.x,pos2.z);
	}

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


	pos1+=pos1-pos2;
	glDisable(GL_FOG);
	glBegin(GL_QUADS);
		glVertexf3(pos1-float3(0,-100,0));
		glVertexf3(pos1+float3(0,-100,0));
		glVertexf3(pos2+float3(0,-100,0));
		glVertexf3(pos2-float3(0,-100,0));
	glEnd();

	if(sizeDiv!=0){
		char c[40];
		SNPRINTF(c, 40, "%d", (int)(pos1.distance2D(pos2)/sizeDiv) );
		mouse->cursorTextRight=c;
	}
	glEnable(GL_FOG);
}


/******************************************************************************/

