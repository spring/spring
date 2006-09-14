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
#include "Game/UI/GUI/GUIframe.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
//#include "Rendering/UnitModels/Unit3DLoader.h"
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
#include "System/Platform/ConfigHandler.h"
#include "mmgr.h"

extern Uint8 *keys;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CGuiHandler* guihandler;


CGuiHandler::CGuiHandler()
: inCommand(-1),
  activeMousePress(false),
  defaultCmdMemory(-1),
  needShift(false),
  maxPage(0),
  activePage(0),
  showingMetal(false),
  buildSpacing(0),
  buildFacing(0),
  actionOffset(0)
{
	icons = new IconInfo[16];
	iconsSize = 16;
	iconsCount = 0;
	
	LoadConfig("ctrlpanel.txt");
	
	LoadCMDBitmap(CMD_STOCKPILE, "bitmaps/armsilo1.bmp");
	readmap->mapDefParser.GetDef(autoShowMetal, "1", "MAP\\autoShowMetal");
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


void CGuiHandler::LoadDefaults()
{
	xIcons = 2;
	yIcons = 8;

	xPos        = 0.000f;
	yPos        = 0.175f;
	xIconSize   = 0.060f;
	yIconSize   = 0.060f;
	iconBorder  = 0.003f;
	frameBorder = 0.003f;
	
	xSelectionPos = 0.018f;
	ySelectionPos = 0.127f;

	noSelectGaps = false;

	outlineFont.Enable(false);
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
		
		if ((command == "noselectgaps") && (words.size() > 1)) {
			noSelectGaps = !!atoi(words[1].c_str());
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
	if ((inCommand >= 0) && (inCommand < commands.size())) {
		const CommandDescription cmdDesc = commands[inCommand];
		LayoutIcons();
		ResetInCommand(cmdDesc);
	} else {
		LayoutIcons();
	}
	return true;
}


//Ladda bitmap -> textur
bool CGuiHandler::LoadCMDBitmap (int id, char* filename)
{
	GuiIconData icondata;
	glGenTextures(1, &icondata.texture);
	CBitmap TextureImage;
	if (!TextureImage.Load(filename))
		throw content_error(std::string("Could not load command bitmap from file ") + filename);

	// create mipmapped texture
	glBindTexture(GL_TEXTURE_2D, icondata.texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,TextureImage.xsize, TextureImage.ysize, GL_RGBA, GL_UNSIGNED_BYTE, TextureImage.mem);

	icondata.has_bitmap = true;
	icondata.width = xIconSize;
	icondata.height = yIconSize;

	iconMap[id] = icondata;

	return true;
}


CGuiHandler::~CGuiHandler()
{
	delete icons;
	glDeleteTextures (1, &iconMap[0].texture);
}


// Ikonpositioner
void CGuiHandler::LayoutIcons()
{
	// reset some of our state
	commands.clear();
	defaultCmdMemory = -1;
	SetShowingMetal(false);
	
	// get the commands to process
	CSelectedUnits::AvailableCommandsStruct ac;
	ac = selectedUnits.GetAvailableCommands();

	vector<CommandDescription> hidden;
	vector<CommandDescription>::const_iterator cdi;
	
	// separate the visible/hidden icons	
	for(cdi = ac.commands.begin(); cdi != ac.commands.end(); ++cdi){
		if(cdi->onlyKey){
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
	activePage = min(maxPage, ac.commandPage);
	iconsCount = pageCount * iconsPerPage;
	
	// resize the icon array if required
	int minIconsSize = iconsSize;
	while (minIconsSize < iconsCount) {
		minIconsSize *= 2;
	}
	if (iconsSize < minIconsSize) {
		iconsSize = minIconsSize;
		delete icons;
		icons = new IconInfo[iconsSize];
	}

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

			const float noGap = noSelectGaps ? iconBorder : 0.0f;
			icon.selection.x1 = icon.visual.x1 - noGap;
			icon.selection.x2 = icon.visual.x2 + noGap;
			icon.selection.y1 = icon.visual.y1 + noGap;
			icon.selection.y2 = icon.visual.y2 - noGap;
		}
	}

	// append the Prev and Next commands  (if required)
	if (multiPage) {
		CommandDescription c;

		c.id = CMD_INTERNAL;
		c.action = "prevmenu";
		c.type = CMDTYPE_PREV;
		c.name = "";
		c.tooltip = "Previous menu";
		commands.push_back(c);

		c.id = CMD_INTERNAL;
		c.action = "nextmenu";
		c.type = CMDTYPE_NEXT;
		c.name = "";
		c.tooltip = "Next menu";
		commands.push_back(c);
	}

	// append the hidden commands
	for (cdi = hidden.begin(); cdi != hidden.end(); ++cdi) {
		commands.push_back(*cdi);
	}

//	logOutput << "LayoutIcons called" << "\n";
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

	if (selectedUnits.CommandsChanged()) {
		LayoutIcons();
		fadein = 100;
	} else if (fadein > 0) {
		fadein -= 5;
	}
	
	SetCursorIcon();
}


void CGuiHandler::Draw()
{
	DrawButtons();
}


void CGuiHandler::DrawButtons()
{
	Update();
	
	glDisable(GL_DEPTH_TEST);

	const int mouseIcon = IconAtPos(mouse->lastx, mouse->lasty);

	// Rita "container"ruta
	if (iconsCount > 0) {

		glDisable(GL_TEXTURE_2D);
		glColor4f(0.2f,0.2f,0.2f,GUI_TRANS);
		glBegin(GL_QUADS);

		GLfloat fx = 0;//-.2f*(1-fadein/100.0f)+.2f;
		glVertex2f(buttonBox.x1-fx, buttonBox.y1);
		glVertex2f(buttonBox.x1-fx, buttonBox.y2);
		glVertex2f(buttonBox.x2-fx, buttonBox.y2);
		glVertex2f(buttonBox.x2-fx, buttonBox.y1);
		glVertex2f(buttonBox.x1-fx, buttonBox.y1);

		glEnd();
	}

	// F� varje knapp (rita den)
	const int buttonStart = min(iconsCount, activePage * iconsPerPage);
	const int buttonEnd   = min(iconsCount, buttonStart + iconsPerPage);

	for (unsigned int ii = buttonStart; ii < buttonEnd; ii++) {

		const IconInfo& icon = icons[ii];
		if (icon.commandsID < 0) {
			continue; // inactive icon
		}
		const CommandDescription& cmdDesc = commands[icon.commandsID];

		const GLfloat x1 = icon.visual.x1;
		const GLfloat y1 = icon.visual.y1;
		const GLfloat x2 = icon.visual.x2;
		const GLfloat y2 = icon.visual.y2;

		glDisable(GL_TEXTURE_2D);

		if ((mouseIcon == ii) || (icon.commandsID == inCommand)) {
			glBegin(GL_QUADS);

			if (icon.commandsID == inCommand) {
				glColor4f(0.5f,0,0,0.8f);
			} else if (mouse->buttons[SDL_BUTTON_LEFT].pressed) {
				glColor4f(0.5f,0,0,0.2f);
			} else {
				glColor4f(0,0,0.5f,0.2f);
			}

			glVertex2f(x1,y1);
			glVertex2f(x2,y1);
			glVertex2f(x2,y2);
			glVertex2f(x1,y2);
			glVertex2f(x1,y1);
			glEnd();
		}

		// "markerad" (l�g m�kt f�t �er)
		glEnable(GL_TEXTURE_2D);
		glColor4f(1,1,1,0.8f);

		UnitDef* ud = unitDefHandler->GetUnitByName(cmdDesc.name);

		if (ud != NULL) {
			// unitikon
			glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitImage(ud));
			glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2f(x1,y1);
			glTexCoord2f(1,0);
			glVertex2f(x2,y1);
			glTexCoord2f(1,1);
			glVertex2f(x2,y2);
			glTexCoord2f(0,1);
			glVertex2f(x1,y2);
			glEnd();

			if (!cmdDesc.params.empty()){			//skriv texten i f�sta param ovanp�
				const string& toPrint = cmdDesc.params[0];

				const float tHeight = font->CalcTextHeight(toPrint.c_str());
				const float yScale = (yIconSize * 0.2f) / tHeight;
				const float xScale = yScale / gu->aspectRatio;

				glTranslatef(x1 + 0.004f, y2 + 0.004f, 0.0f);
				glScalef(xScale, yScale, 1.0f);
				font->glPrint("%s",toPrint.c_str());
			}
		}
		else {
			if (iconMap.find(cmdDesc.id) != iconMap.end() && iconMap[cmdDesc.id].has_bitmap) {
				// Bitmapikon
				glBindTexture(GL_TEXTURE_2D, iconMap[cmdDesc.id].texture);
				glBegin(GL_QUADS);
				glTexCoord2f(0,0);
				glVertex2f(x1,y1);
				glTexCoord2f(1,0);
				glVertex2f(x2,y1);
				glTexCoord2f(1,1);
				glVertex2f(x2,y2);
				glTexCoord2f(0,1);
				glVertex2f(x1,y2);
				glEnd();
			} else {
				glPushAttrib(GL_ENABLE_BIT);
				glDisable(GL_LIGHTING);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				GLfloat xSize = fabs(x2-x1);
				GLfloat yCenter = (y1+y2)/2.f;
				GLfloat ySize = fabs(y2-y1);

				if (cmdDesc.type == CMDTYPE_PREV) {
					glBegin(GL_POLYGON);
					if ((mouseIcon == ii) || (icon.commandsID == inCommand)) {
						glColor4f(1.f,1.f,0.f,1.f); // selected
					} else {
						glColor4f(0.7f,0.7f,0.7f,1.f); // normal
					}
					glVertex2f(x2-xSize/6,yCenter-ySize/8);
					glVertex2f(x1+2*xSize/6,yCenter-ySize/8);
					glVertex2f(x1+xSize/6,yCenter);
					glVertex2f(x1+2*xSize/6,yCenter+ySize/8);
					glVertex2f(x2-xSize/6,yCenter+ySize/8);
					glEnd();
				}
				else if (cmdDesc.type == CMDTYPE_NEXT) {
					glBegin(GL_POLYGON);
					if ((mouseIcon == ii) || (icon.commandsID == inCommand)) {
						glColor4f(1.f,1.f,0.f,1.f); // selected
					} else {
						glColor4f(0.7f,0.7f,0.7f,1.f); // normal
					}
					glVertex2f(x1+xSize/6,yCenter-ySize/8);
					glVertex2f(x2-2*xSize/6,yCenter-ySize/8);
					glVertex2f(x2-xSize/6,yCenter);
					glVertex2f(x2-2*xSize/6,yCenter+ySize/8);
					glVertex2f(x1+xSize/6,yCenter+ySize/8);
					glEnd();
				}
				else {
					glBegin(GL_LINE_LOOP);
					glColor4f(1.f,1.f,1.f,0.1f);
					glVertex2f(x1,y1);
					glVertex2f(x2,y1);
					glVertex2f(x2,y2);
					glVertex2f(x1,y2);
					glEnd();
				}
				glPopAttrib();
			}

			// skriv text
			string toPrint = cmdDesc.name;
			
			if (cmdDesc.type==CMDTYPE_ICON_MODE) {
				int opt = atoi(cmdDesc.params[0].c_str()) + 1;
				if (opt < cmdDesc.params.size()) {
					toPrint = cmdDesc.params[opt];
				}
			}

			const float tWidth  = font->CalcTextWidth(toPrint.c_str());
			const float tHeight = font->CalcTextHeight(toPrint.c_str());
			float xScale = (xIconSize - 0.006f) / tWidth;
			float yScale = (yIconSize - 0.006f) / tHeight;

			const float yClamp = xScale * gu->aspectRatio;
			yScale = min(yScale, yClamp);

			const float xCenter = 0.5f * (x1 + x2);
			const float yCenter = 0.5f * (y1 + y2);
			const float xStart  = xCenter - (0.5f * tWidth  * xScale);
			const float yStart  = yCenter - (0.5f * tHeight * yScale);
			const float dShadow = 0.002f;			

			glPushMatrix();
			glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
			glTranslatef(xStart + dShadow, yStart - dShadow, 0.0f);
			glScalef(xScale, yScale, 1.0f);
			font->glPrint("%s",toPrint.c_str());
			glPopMatrix();
			
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glTranslatef(xStart, yStart, 0.0f);
			glScalef(xScale, yScale, 1.0f);
			font->glPrint("%s",toPrint.c_str());
		}

		glLoadIdentity();
		
		if ((mouseIcon == ii) || (icon.commandsID == inCommand)) {
			
			if (icon.commandsID == inCommand) {
				glColor4f(1.0f, 1.0f, 0.0f, 0.75f);
			} else if (mouse->buttons[SDL_BUTTON_LEFT].pressed) {
				glColor4f(1.0f, 0.0f, 0.0f, 0.50f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.50f);
			}

			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLineWidth(1.49f);

			glBegin(GL_LINE_LOOP);
			glVertex2f(x1,y1);
			glVertex2f(x2,y1);
			glVertex2f(x2,y2);
			glVertex2f(x1,y2);
			glEnd();

			glLineWidth(1.0f);

			glPopAttrib();
		}
	}

	if (iconsCount > 0) {
		char buf[64];

		SNPRINTF(buf, 64, "%i", activePage + 1);
		if (selectedUnits.BuildIconsFirst()) {
			glColor4fv(cmdColors.build);
		} else {
			glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
		}
		const float textSize = 1.2f;
		const float yBbot = yBpos - (textSize * 0.5f * (font->CalcTextHeight(buf) / 32.0f));
		font->glPrintCentered(xBpos, yBbot, textSize, buf);

		if (selectedUnits.selectedGroup != -1) {
			SNPRINTF(buf, 64, "Selected units %i  [Group %i]",
			         selectedUnits.selectedUnits.size(),
			         selectedUnits.selectedGroup);
		} else {
			SNPRINTF(buf, 64, "Selected units %i",
			         selectedUnits.selectedUnits.size());
		}

		const float textScale = 0.6;
		const float textWidth  = textScale * (font->CalcTextWidth(buf) / 48.0f);
		const float textHeight = textScale * (font->CalcTextHeight(buf) / 24.0f);

		if (!outlineFont.IsEnabled()) {
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f, 0.2f, 0.2f, GUI_TRANS);
			glRectf(xSelectionPos - frameBorder,
							ySelectionPos - frameBorder,
							xSelectionPos + frameBorder + textWidth,
							ySelectionPos + frameBorder + textHeight);
			glColor4f(1,1,1,0.8f);
			font->glPrintAt(xSelectionPos, ySelectionPos, textScale, "%s", buf);
		}
		else {
			glTranslatef(xSelectionPos, ySelectionPos, 0.0f);

			const float xScale = textScale * 0.0225f;
			const float yScale = textScale * 0.0300f;
			glScalef(xScale, yScale, 1.0f);
			
			const float xPixel  = 1.0f / (xScale * (float)gu->screenx);
			const float yPixel  = 1.0f / (yScale * (float)gu->screeny);

			const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			outlineFont.print(xPixel, yPixel, white, buf);
		}
	}
}


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
		activeMousePress = false;
	} else {
		return;
	}

	if (needShift && !keys[SDLK_LSHIFT]) {
		SetShowingMetal(false);
		inCommand = -1;
		needShift = false;
	}

	const int iconPos = IconAtPos(x, y);
	const int iconCmd = (iconPos >= 0) ? icons[iconPos].commandsID : -1;
	

//	logOutput << x << " " << y << " " << mouse->lastx << " " << mouse->lasty << "\n";

	if (button == SDL_BUTTON_RIGHT && iconCmd==-1) { // right click -> default cmd
		inCommand = defaultCmdMemory; //GetDefaultCommand(x,y);
		defaultCmdMemory = -1;
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
				int newMode=atoi(commands[iconCmd].params[0].c_str())+1;
				if(newMode>commands[iconCmd].params.size()-2)
					newMode=0;

				char t[10];
				SNPRINTF(t, 10, "%d", newMode);
				commands[iconCmd].params[0]=t;

				Command c;
				c.id=commands[iconCmd].id;
				c.params.push_back(newMode);
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				selectedUnits.GiveCommand(c);
				inCommand=-1;
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
	const float fx = float(x) / gu->screenx;
	const float fy = float(gu->screeny-y) / gu->screeny;

	if ((fx < buttonBox.x1) || (fx > buttonBox.x2) ||
	    (fy < buttonBox.y1) || (fy > buttonBox.y2)) {
		return -1;
	}

	// FIXME -- icons are now positional ... use it
	const int buttonStart = min(iconsCount, activePage * iconsPerPage);
	const int buttonEnd   = min(iconsCount, buttonStart + iconsPerPage);
	for (int ii = buttonStart; ii < buttonEnd; ii++) {
		if ((fx > icons[ii].selection.x1) && (fx < icons[ii].selection.x2) &&
		    (fy > icons[ii].selection.y2) && (fy < icons[ii].selection.y1)) {
			return ii;
		}
	}

	return -1;
}


bool CGuiHandler::AboveGui(int x, int y)
{
	if (iconsCount <= 0) {
		return false;
	}

	const float fx = float(x) / gu->screenx;
	const float fy = float(gu->screeny-y) / gu->screeny;
	if ((fx > buttonBox.x1) && (fx < buttonBox.x2) &&
	    (fy > buttonBox.y1) && (fy < buttonBox.y2)) {
		return true;
	}
	
	return IconAtPos(x,y) != -1;
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
	if(keys[SDLK_LALT])
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

	if(dist>gu->viewRange*1.4f-100 && dist2>gu->viewRange*1.4f-100 && unit==0){
		return -1;
	}

	if(dist>dist2)
		unit=0;
	else
		feature=0;

	// make sure the command is currently available
	int cmd_id = selectedUnits.GetDefaultCmd(unit, feature);
	for (int c = 0; c < (int)commands.size(); c++) {
		if (cmd_id == commands[c].id) {
			return c;
		}
	}
	return -1;
}

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
	if(inCommand>=0 && inCommand<commands.size() && commands[guihandler->inCommand].type==CMDTYPE_ICON_BUILDING){
		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
		if(dist>0){
			string s=commands[guihandler->inCommand].name;
			UnitDef *unitdef = unitDefHandler->GetUnitByName(s);

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


void CGuiHandler::ResetInCommand(const CommandDescription& cmdDesc)
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
			for (int ii = 0; ii < iconsCount; ii++) {
				if (inCommand == icons[ii].commandsID) {
					activePage = min(maxPage, (ii / iconsPerPage));;
					selectedUnits.SetCommandPage(activePage);
				}
			}
		}
	}
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
		if ((inCommand >= 0) && (inCommand < commands.size())) {
			const CommandDescription cmdDesc = commands[inCommand];
			LayoutIcons();
			ResetInCommand(cmdDesc);
		}
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
		printf("Available Commands:\n");
		for(int i = 0; i < commands.size(); ++i){
			printf("  button: %i, id = %i, action = %s\n",
						 i, commands[i].id, commands[i].action.c_str());
		}
		return true;
	}

	return false;
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
			    (((cmdType == CMDTYPE_ICON) && (commands[a].id < 0)) ||
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
			
			switch (cmdType) {
				case CMDTYPE_ICON:{
					Command c;
					c.id = commands[a].id;
					if (c.id < 0) {
						c.options = 0;
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
					} else {
						CreateOptions(c, false); //(button==SDL_BUTTON_LEFT?0:1));
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

					char t[10];
					SNPRINTF(t, 10, "%d", newMode);
					commands[a].params[0] = t;

					Command c;
					c.id = commands[a].id;
					c.params.push_back(newMode);
					CreateOptions(c, false); //(button==SDL_BUTTON_LEFT?0:1));
					selectedUnits.GiveCommand(c);
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
								c.id = cd.id;
								c.params.push_back(p);
								CreateOptions(c, false);
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
	if(inCommand>=0 && inCommand<commands.size())
	{
		CommandDescription& cd=commands[inCommand];
		switch(cd.type)
		{
		case CMDTYPE_COMBO_BOX:
			{
				inCommand=-1;
				vector<string>::iterator pi;
				int a=0;
				for(pi=++cd.params.begin();pi!=cd.params.end();++pi)
				{
					if(*pi==s)
					{
						Command c;
						c.id=cd.id;
						c.params.push_back(a);
						CreateOptions(c,0);
						selectedUnits.GiveCommand(c);
						break;
					}
					++a;
				}
			}
				break;
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
			string s=commands[guihandler->inCommand].name;
			UnitDef *unitdef = unitDefHandler->GetUnitByName(s);

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
			if(dist2>gu->viewRange*1.4f-100){
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

				if(dist2>gu->viewRange*1.4f-100 && (commands[tempInCommand].type!=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA || dist3>8900)){
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
