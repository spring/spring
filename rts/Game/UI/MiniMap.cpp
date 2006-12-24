// MiniMap.cpp: implementation of the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "StdAfx.h"
#include "MiniMap.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_types.h>
#include "CommandColors.h"
#include "GuiHandler.h"
#include "InfoConsole.h"
#include "MouseHandler.h"
#include "SimpleParser.h"
#include "ExternalAI/Group.h"
#include "Game/CameraController.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/IconHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/GL/glExtra.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Sound.h"
#include "mmgr.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CMiniMap* minimap = NULL;

extern Uint8* keys;


CMiniMap::CMiniMap()
: fullProxy(false),
  proxyMode(false),
  selecting(false),
  mouseMove(false),
  mouseResize(false),
  mouseLook(false),
  maxspect(false),
  maximized(false),
  minimized(false),
  showButtons(false),
  useIcons(true)
 {
	lastWindowSizeX = gu->viewSizeX;
	lastWindowSizeY = gu->viewSizeY;

	if (gu->dualScreenMode) {
		width = gu->viewSizeX;
		height = gu->viewSizeY;
		xpos = (gu->viewSizeX - gu->viewPosX);
		ypos = 0;
	}
	else {
		const std::string geodef = "2 2 200 200";
		const std::string geo = configHandler.GetString("MiniMapGeometry", geodef);
		ParseGeometry(geo);
	}

	fullProxy = !!configHandler.GetInt("MiniMapFullProxy", 1);
	
	buttonSize = configHandler.GetInt("MiniMapButtonSize", 16);
	
	unitBaseSize =
		atof(configHandler.GetString("MiniMapUnitSize", "2.5").c_str());
	unitBaseSize = max(0.0f, unitBaseSize);

	unitExponent =
		atof(configHandler.GetString("MiniMapUnitExp", "0.25").c_str());

	cursorScale =
		atof(configHandler.GetString("MiniMapCursorScale", "-0.5").c_str());

	useIcons = !!configHandler.GetInt("MiniMapIcons", 1);
	
	drawCommands = max(0, configHandler.GetInt("MiniMapDrawCommands", 1));
	
	simpleColors = !!configHandler.GetInt("SimpleMiniMapColors", 0);

	myColor[0]    = (unsigned char)(0.2f * 255);
	myColor[1]    = (unsigned char)(0.9f * 255);
	myColor[2]    = (unsigned char)(0.2f * 255);
	myColor[3]    = (unsigned char)(1.0f * 255);
	allyColor[0]  = (unsigned char)(0.3f * 255);
	allyColor[1]  = (unsigned char)(0.3f * 255);
	allyColor[2]  = (unsigned char)(0.9f * 255);
	allyColor[3]  = (unsigned char)(1.0f * 255);
	enemyColor[0] = (unsigned char)(0.9f * 255);
	enemyColor[1] = (unsigned char)(0.2f * 255);
	enemyColor[2] = (unsigned char)(0.2f * 255);
	enemyColor[3] = (unsigned char)(1.0f * 255);

	UpdateGeometry();
	
	circleLists = glGenLists(circleListsCount);
	for (int cl = 0; cl < circleListsCount; cl++) {
		glNewList(circleLists + cl, GL_COMPILE);
		glBegin(GL_LINE_LOOP);
		const int divs = (1 << (cl + 3));
		for (int d = 0; d < divs; d++) {
			const float rads = float(2.0 * PI) * float(d) / float(divs);
			glVertex3f(sin(rads), 0.0f, cos(rads));
		}
		glEnd();
		glEndList();
	}

	// setup the buttons' texture and texture coordinates
	buttonsTexture = 0;
	CBitmap bitmap;
	bool unfiltered = false;
	if (bitmap.Load("bitmaps/minimapbuttons.png")) {
		if ((bitmap.ysize == buttonSize) && (bitmap.xsize == (buttonSize * 4))) {
			unfiltered = true;
		}
		glGenTextures(1, &buttonsTexture);
		glBindTexture(GL_TEXTURE_2D, buttonsTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
								 bitmap.xsize, bitmap.ysize, 0,
								 GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		if (unfiltered) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	const float xshift = unfiltered ? 0.0f : (0.5f / bitmap.xsize);
	const float yshift = unfiltered ? 0.0f : (0.5f / bitmap.ysize);
	    moveBox.xminTx = 0.50f + xshift;
	    moveBox.xmaxTx = 0.75f - xshift;
	  resizeBox.xminTx = 0.75f + xshift;
	  resizeBox.xmaxTx = 1.00f - xshift;
	minimizeBox.xminTx = 0.00f + xshift;
	minimizeBox.xmaxTx = 0.25f - xshift;
	maximizeBox.xminTx = 0.25f + xshift;
	maximizeBox.xmaxTx = 0.50f - xshift;
	    moveBox.yminTx = 1.00f - yshift;
	  resizeBox.yminTx = 1.00f - yshift;
	minimizeBox.yminTx = 1.00f - yshift;
	maximizeBox.yminTx = 1.00f - yshift;
	    moveBox.ymaxTx = 0.00f + yshift;
	  resizeBox.ymaxTx = 0.00f + yshift;
	minimizeBox.ymaxTx = 0.00f + yshift;
	maximizeBox.ymaxTx = 0.00f + yshift;
}


CMiniMap::~CMiniMap()
{
	glDeleteLists(circleLists, circleListsCount);
}


void CMiniMap::ParseGeometry(const string& geostr)
{
	const std::string geodef = "2 2 200 200";
	const int scanned = sscanf(geostr.c_str(), "%i %i %i %i",
	                           &xpos, &ypos, &width, &height);
	const bool userGeo = ((scanned != 4) || (geostr != geodef));

	if (!userGeo) {
		xpos = 2;
		ypos = 2;
		width = -200;
		height = -200;
	} else {
		if (width <= 0) {
			width = -200;
		}
		if (height <= 0) {
			height = -200;
		}
	}

	if ((width <= 0) && (height <= 0)) {
		const float hw = sqrt(float(gs->mapx) / float(gs->mapy));
		width = (int)(-width * hw);
		height = (int)(-height / hw);
	}
	else if (width <= 0) {
		width = (int)(float(height) * float(gs->mapx) / float(gs->mapy));
	}
	else if (height <= 0) {
		height = (int)(float(width) * float(gs->mapy) / float(gs->mapx));
	}

	// convert to GL coords with a top-left corner affinity
	ypos = gu->viewSizeY - height - ypos;
}


void CMiniMap::ToggleMaximized(bool _maxspect)
{
	if (maximized) {
		xpos = oldxpos;
		ypos = oldypos;
		width = oldwidth;
		height = oldheight;
	}
	else {
		oldxpos = xpos;
		oldypos = ypos;
		oldwidth = width;
		oldheight = height;
		maxspect = _maxspect;
		SetMaximizedGeometry();
	}
	maximized = !maximized;
	UpdateGeometry();
}


void CMiniMap::SetMaximizedGeometry()
{
	if (!maxspect) {
		height = gu->viewSizeY;
		width = height;
		xpos = (gu->viewSizeX - gu->viewSizeY) / 2;
		ypos = 0;
	}
	else {
		const float mapRatio = (float)gs->mapx / (float)gs->mapy;
		const float viewRatio = (float)gu->viewSizeX / (float)gu->viewSizeY;
		if (mapRatio > viewRatio) {
			xpos = 0;
			width = gu->viewSizeX;
			height = int((float)gu->viewSizeX / mapRatio);
			ypos = (gu->viewSizeY - height) / 2;
		} else {
			ypos = 0;
			height = gu->viewSizeY;
			width = int((float)gu->viewSizeY * mapRatio);
			xpos = (gu->viewSizeX - width) / 2;
		}
	}
}


/******************************************************************************/

void CMiniMap::ConfigCommand(const std::string& line)
{
  const vector<string> words = SimpleParser::Tokenize(line, 1);
  if (words.empty()) {
  	return;
	}
	const string command = StringToLower(words[0]);

	if (command == "fullproxy") {
		if (words.size() >= 2) {
			fullProxy = !!atoi(words[1].c_str());
		} else {
			fullProxy = !fullProxy;
		}
	}
	else if (command == "icons") {
		if (words.size() >= 2) {
			useIcons = !!atoi(words[1].c_str());
		} else {
			useIcons = !useIcons;
		}
	}
	else if (command == "unitexp") {
		if (words.size() >= 2) {
			unitExponent = atof(words[1].c_str());
		}
		UpdateGeometry();
	}
	else if (command == "unitsize") {
		if (words.size() >= 2) {
			unitBaseSize = atof(words[1].c_str());
		}
		unitBaseSize = max(0.0f, unitBaseSize);
		UpdateGeometry();
	}
	else if (command == "drawcommands") {
		if (words.size() >= 2) {
			drawCommands = max(0, atoi(words[1].c_str()));
			
		} else {
			drawCommands = (drawCommands > 0) ? 0 : 1;
		}
	}
	else if (command == "simplecolors") {
		if (words.size() >= 2) {
			simpleColors = !!atoi(words[1].c_str());
		} else {
			simpleColors = !simpleColors;
		}
	}

	// the following commands can not be used in dualscreen mode
	if (gu->dualScreenMode) {
		return;
	}

	if ((command == "geo") || (command == "geometry")) {
		if (words.size() < 2) {
			return;
		}
		ParseGeometry(words[1]);
		UpdateGeometry();
	}
	else if ((command == "min") || (command == "minimize")) {
		if (words.size() >= 2) {
			minimized = !!atoi(words[1].c_str());
		} else {
			minimized = !minimized;
		}
	}
	else if ((command == "max") ||
	         (command == "maximize") || (command == "maxspect")) {
		bool newMax = maximized;
		if (words.size() >= 2) {
			newMax = !!atoi(words[1].c_str());
		} else {
			newMax = !newMax;
		}
		if (newMax != maximized) {
			ToggleMaximized(command == "maxspect");
		}
	}
}


/******************************************************************************/
    
bool CMiniMap::MousePress(int x, int y, int button)
{
	if (minimized) {
		if ((x < buttonSize) && (y < buttonSize)) {
			minimized = false;
			return true;
		} else {
			return false;
		}
	}

	const bool inMap = mapBox.Inside(x, y);
	const bool inButtons = buttonBox.Inside(x, y);
	
	if (!inMap && !inButtons) {
		return false;
	}
	
	if (button == SDL_BUTTON_LEFT) {
		if (inMap && (guihandler->inCommand >= 0)) {
			proxyMode = true;
			ProxyMousePress(x, y, button);
			return true;
		}
		if (showButtons && inButtons) {
			if (moveBox.Inside(x, y)) {
				mouseMove = true;
				return true;
			}
			else if (resizeBox.Inside(x, y)) {
				mouseResize = true;
				return true;
			}
			else if (minimizeBox.Inside(x, y) ||
			         maximizeBox.Inside(x, y)) {
				return true;
			}
		}
		if (inMap && !mouse->buttons[SDL_BUTTON_LEFT].chorded) {
			selecting = true;
			return true;
		}
	}
	else if (inMap) {
		if ((fullProxy && (button == SDL_BUTTON_MIDDLE)) ||
				(!fullProxy && (button == SDL_BUTTON_RIGHT))) {
			MoveView(x, y);
			mouseLook = true;
			return true;
		}
		else if (fullProxy && (button == SDL_BUTTON_RIGHT)) {
			proxyMode = true;
			ProxyMousePress(x, y, button);
			return true;
		}
	}
	
	return false;
}


void CMiniMap::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (mouseMove) {
		xpos += dx;
		ypos -= dy;
		xpos = max(0, xpos);
		if (gu->dualScreenMode) {
			xpos = min((2 * gu->viewSizeX) - width, xpos);
		} else {
			xpos = min(gu->viewSizeX - width, xpos);
		}
		ypos = max(5, min(gu->viewSizeY - height, ypos));
		UpdateGeometry();
		return;
	}

	if (mouseResize) {
		ypos   -= dy;
		width  += dx;
		height += dy;
		height = min(gu->viewSizeY, height);
		if (gu->dualScreenMode) {
			width = min(2 * gu->viewSizeX, width);
		} else {
			width = min(gu->viewSizeX, width);
		}
		if (keys[SDLK_LSHIFT]) {
			width = (height * gs->mapx) / gs->mapy;
		}
		width = max(5, width);
		height = max(5, height);
		ypos = min(gu->viewSizeY - height, ypos);
		UpdateGeometry();
		return;
	}
	
	if (mouseLook && mapBox.Inside(x, y)) {
			MoveView(x,y);
		return;
	}
}


void CMiniMap::MouseRelease(int x, int y, int button)
{
	if (mouseMove || mouseResize || mouseLook) {
		mouseMove = false;
		mouseResize = false;
		mouseLook = false;
		proxyMode = false;
		return;
	}
	
	if (proxyMode) {
		ProxyMouseRelease(x, y, button);
		proxyMode = false;
		return;
	}
	
	if (selecting) {
		SelectUnits(x, y);
		selecting = false;
		return;
	}

	if (button == SDL_BUTTON_LEFT) {
		if (showButtons && maximizeBox.Inside(x, y)) {
			ToggleMaximized(keys[SDLK_LSHIFT]);
			return;
		}

		if (showButtons && minimizeBox.Inside(x, y)) {
			minimized = true;
			return;
		}
	}
}


void CMiniMap::UpdateGeometry()
{
	// keep the same distance to the top
	ypos -= (lastWindowSizeY - gu->viewSizeY);
	if (gu->dualScreenMode) {
		width = gu->viewSizeX;
		height = gu->viewSizeY;
		xpos = (gu->viewSizeX - gu->viewPosX);
		ypos = 0;
	}
	else if (maximized) {
		SetMaximizedGeometry();
	}
	else {
		width = max(1, min(width, gu->viewSizeX));
		height = max(1, min(height, gu->viewSizeY));
		ypos = max(buttonSize, ypos);
		ypos = min(gu->viewSizeY - height, ypos);
		xpos = max(0, min(gu->viewSizeX - width, xpos));
	}
	lastWindowSizeX = gu->viewSizeX;
	lastWindowSizeY = gu->viewSizeY;

	// setup the unit scaling	
	const float w = float(width);
	const float h = float(height);
	const float mapx = float(gs->mapx * SQUARE_SIZE);
	const float mapy = float(gs->mapy * SQUARE_SIZE);
	const float ref  = unitBaseSize / powf((200.f * 200.0f), unitExponent);
	const float dpr  = ref * powf((w * h), unitExponent);

	unitSizeX = dpr * (mapx / w);
	unitSizeY = dpr * (mapy / h);
	unitSelectRadius = sqrtf(unitSizeX * unitSizeY);

	// in mouse coordinates
	mapBox.xmin = xpos;
	mapBox.xmax = mapBox.xmin + width - 1;
	mapBox.ymin = gu->viewSizeY - (ypos + height);
	mapBox.ymax = mapBox.ymin + height - 1;

	if (!maximized) {
		// right to left
		resizeBox.xmax   = mapBox.xmax;
		resizeBox.xmin   = resizeBox.xmax - (buttonSize - 1);
		moveBox.xmax     = resizeBox.xmax   - buttonSize;
		moveBox.xmin     = resizeBox.xmin   - buttonSize;
		maximizeBox.xmax = moveBox.xmax     - buttonSize;
		maximizeBox.xmin = moveBox.xmin     - buttonSize;
		minimizeBox.xmax = maximizeBox.xmax - buttonSize;
		minimizeBox.xmin = maximizeBox.xmin - buttonSize;
		const int ymin = (mapBox.ymax + 1) + 3; // 3 for the white|black|white
		const int ymax = ymin + (buttonSize - 1);
		resizeBox.ymin = moveBox.ymin = maximizeBox.ymin = minimizeBox.ymin = ymin;
		resizeBox.ymax = moveBox.ymax = maximizeBox.ymax = minimizeBox.ymax = ymax;
		buttonBox.xmin = minimizeBox.xmin;
		buttonBox.xmax = mapBox.xmax;
		buttonBox.ymin = ymin - 3;
		buttonBox.ymax = ymax;
	} else {
		// left to right
		minimizeBox.xmin = mapBox.xmin;
		minimizeBox.xmax = minimizeBox.xmin + (buttonSize - 1);
		maximizeBox.xmin = minimizeBox.xmin + buttonSize;
		maximizeBox.xmax = minimizeBox.xmax + buttonSize;
		// dead buttons
		resizeBox.xmin = resizeBox.ymin = moveBox.xmin = moveBox.ymin = 0;
		resizeBox.xmax = resizeBox.ymax = moveBox.xmax = moveBox.ymax = -1;
		const int ymin = mapBox.ymin;
		const int ymax = ymin + (buttonSize - 1);
		maximizeBox.ymin = minimizeBox.ymin = ymin;
		maximizeBox.ymax = minimizeBox.ymax = ymax;
		buttonBox.xmin = minimizeBox.xmin;
		buttonBox.xmax = maximizeBox.xmax;
		buttonBox.ymin = ymin - 3;
		buttonBox.ymax = ymax;
	}
}


/******************************************************************************/

void CMiniMap::MoveView(int x, int y)
{
	const float3& pos = camera->pos;
	const float3& dir = camera->forward;
	float dist = ground->LineGroundCol(pos, pos + (dir * gu->viewRange * 1.4f));
	float3 dif(0,0,0);
	if (dist > 0) {
		dif = dir * dist;
	}
	float camHeight = pos.y - ground->GetHeight(pos.x, pos.z);
	float3 clickPos;
	clickPos.x = (float(x - xpos)) / width * gs->mapx * 8;
	clickPos.z = (float(y - (gu->viewSizeY - ypos - height))) / height * gs->mapy * 8;
	mouse->currentCamController->SetPos(clickPos);
}


void CMiniMap::SelectUnits(int x, int y) const
{
	if (!keys[SDLK_LSHIFT] && !keys[SDLK_LCTRL]) {
		selectedUnits.ClearSelected();
	}

	CMouseHandler::ButtonPress& bp = mouse->buttons[SDL_BUTTON_LEFT];

	if (fullProxy && (bp.movement > 4)) {
		// use a selection box
		const float3 newPos = GetMapPosition(x, y);
		const float3 oldPos = GetMapPosition(bp.x, bp.y);
		const float xmin = min(oldPos.x, newPos.x);
		const float xmax = max(oldPos.x, newPos.x);
		const float zmin = min(oldPos.z, newPos.z);
		const float zmax = max(oldPos.z, newPos.z);
		
		set<CUnit*>::iterator ui;
		set<CUnit*>& selection = selectedUnits.selectedUnits;
		set<CUnit*>& myUnits = gs->Team(gu->myTeam)->units;

		CUnit* unit;
		int addedunits = 0;
		for (ui = myUnits.begin(); ui != myUnits.end(); ++ui) {
			const float3& midPos = (*ui)->midPos;
			if ((midPos.x > xmin) && (midPos.x < xmax) &&
			    (midPos.z > zmin) && (midPos.z < zmax)) {
				if (keys[SDLK_LCTRL] && (selection.find(*ui) != selection.end())) {
					selectedUnits.RemoveUnit(*ui);
				} else {
					selectedUnits.AddUnit(*ui);
					unit = *ui;
					addedunits++;
				}
			}
		}

		if (addedunits >= 2) {
			sound->PlaySample(mouse->soundMultiselID);
		}
		else if (addedunits == 1) {
			if (unit->unitDef->sounds.select.id) {
				sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit,
														 unit->unitDef->sounds.select.volume);
			}
		}
	}
	else {
		// Single unit
		const float size = unitSelectRadius;
		const float3 pos = GetMapPosition(x, y);
		CUnit* unit = helper->GetClosestFriendlyUnit(pos, size, gu->myAllyTeam);

		set<CUnit*>::iterator ui;
		set<CUnit*>& selection = selectedUnits.selectedUnits;
		set<CUnit*>& myUnits = gs->Team(gu->myTeam)->units;
		
		if (unit && (unit->team == gu->myTeam)){
			if (bp.lastRelease < (gu->gameTime - mouse->doubleClickTime)) {
				if (keys[SDLK_LCTRL] && (selection.find(unit) != selection.end())) {
					selectedUnits.RemoveUnit(unit);
				} else {
					selectedUnits.AddUnit(unit);
				}
			} else {
				//double click
				if (unit->group && !keys[SDLK_LCTRL]) {
					//select the current units group if it has one
					selectedUnits.SelectGroup(unit->group->id);
				} else {
					//select all units of same type
					for (ui = myUnits.begin(); ui != myUnits.end(); ++ui) {
						if ((*ui)->aihint == unit->aihint) {
							selectedUnits.AddUnit(*ui);
						}
					}
				}
			}
			bp.lastRelease = gu->gameTime;

			if (unit->unitDef->sounds.select.id) {
				sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit,
														 unit->unitDef->sounds.select.volume);
			}
		}
	}
}


/******************************************************************************/

CUnit* CMiniMap::GetSelectUnit(const float3& pos) const
{
	CUnit* unit = helper->GetClosestUnit(pos, unitSelectRadius);
	if (unit != NULL) {
		const int losMask = (LOS_INLOS | LOS_INRADAR);
		if ((unit->losStatus[gu->myAllyTeam] & losMask) || gu->spectatingFullView) {
			return unit;
		} else {
			return NULL;
		}
	}
	return unit;
}


float3 CMiniMap::GetMapPosition(int x, int y) const
{
	const float mapHeight = readmap->maxheight + 1000.0f;
	const float mapX = gs->mapx * SQUARE_SIZE;
	const float mapY = gs->mapy * SQUARE_SIZE;
	const float3 pos(mapX * float(x - xpos) / width, mapHeight,
	                 mapY * float(y - (gu->viewSizeY - ypos - height)) / height);
	return pos;
}


void CMiniMap::ProxyMousePress(int x, int y, int button)
{
	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);
	if (unit) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = helper->GetUnitErrorPos(unit, gu->myAllyTeam);
			mapPos.y = readmap->maxheight + 1000.0f;
		}
	}

	CMouseHandler::ButtonPress& bp = mouse->buttons[button];
	bp.camPos = mapPos;
	bp.dir = float3(0.0f, -1.0f, 0.0f);
	
	guihandler->MousePress(x, y, -button);
}


void CMiniMap::ProxyMouseRelease(int x, int y, int button)
{
	CCamera c = *camera;
	const float3 tmpMouseDir = mouse->dir;

	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);
	if (unit) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = helper->GetUnitErrorPos(unit, gu->myAllyTeam);
			mapPos.y = readmap->maxheight + 1000.0f;
		}
	}

	mouse->dir = float3(0.0f, -1.0f, 0.0f);
	camera->pos = mapPos;
	camera->forward = mouse->dir;

	guihandler->MouseRelease(x, y, -button);
	
	mouse->dir = tmpMouseDir;
	*camera = c;
}


/******************************************************************************/

bool CMiniMap::IsAbove(int x, int y)
{
	if (minimized) {
		if ((x < buttonSize) && (y < buttonSize)) {
			return true;
		} else {
			return false;
		}
	}
	else if (mapBox.Inside(x, y)) {
		return true;
	}
	else if (showButtons && buttonBox.Inside(x, y)) {
		return true;
	}
	return false;
}


std::string CMiniMap::GetTooltip(int x, int y)
{
	if (minimized) {
		return "Unminimize map";
	}

	if (buttonBox.Inside(x, y)) {
		if (resizeBox.Inside(x, y)) {
			return "Resize map\n(SHIFT to maintain aspect ratio)";
		}
		if (moveBox.Inside(x, y)) {
			return "Move map";
		}
		if (maximizeBox.Inside(x, y)) {
			if (!maximized) {
				return "Maximize map\n(SHIFT to maintain aspect ratio)";
			} else {
				return "Unmaximize map";
			}
		}
		if (minimizeBox.Inside(x, y)) {
			return "Minimize map";
		}
	}

	const string buildTip = guihandler->GetBuildTooltip();
	if (!buildTip.empty()) {
		return buildTip;
	}
	
	CUnit* unit = GetSelectUnit(GetMapPosition(x, y));
	
	if (unit) {
		// don't show the tooltip if it's a radar dot
		if(!gu->spectatingFullView && (gs->AllyTeam(unit->team) != gu->myAllyTeam) &&
		   !loshandler->InLos(unit,gu->myAllyTeam)){
			return "Enemy unit";
		}
		// show the player name instead of unit name if it has FBI tag showPlayerName
		string s;
		if(unit->unitDef->showPlayerName){
			s=gs->players[gs->Team(unit->team)->leader]->playerName.c_str();
		} else {
			s=unit->tooltip;
		}
		// don't show the unit health and other info if it has
		// FBI tag hideDamage and isn't on our ally team
		if(!(!gu->spectatingFullView && unit->unitDef->hideDamage &&
		     (gs->AllyTeam(unit->team) != gu->myAllyTeam))){
			char tmp[512];

			sprintf(tmp,"\nHealth %.0f/%.0f",unit->health,unit->maxHealth);
			s+=tmp;

			if(unit->unitDef->maxFuel>0){
				sprintf(tmp," Fuel %.0f/%.0f",unit->currentFuel,unit->unitDef->maxFuel);
				s+=tmp;
			}

			sprintf(tmp,"\nExperience %.2f Cost %.0f Range %.0f \n\xff\xd3\xdb\xffMetal: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f\xff\xd3\xdb\xff Energy: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f",
				unit->experience,unit->metalCost+unit->energyCost/60,unit->maxRange, unit->metalMake, unit->metalUse, unit->energyMake, unit->energyUse);
			s+=tmp;
		}

		if (gs->cheatEnabled) {
			char buf[32];
			SNPRINTF(buf, 32, "\xff\xc0\xc0\xff  [TechLevel %i]", unit->unitDef->techLevel);
			s += buf;
		}
		return s;
	}
	
	const string selTip = selectedUnits.GetTooltip();
	if (selTip != "") {
		return selTip;
	}
				
	float3 pos(float(x-xpos)/width*gs->mapx*SQUARE_SIZE,500,float(y-(gu->viewSizeY-ypos-height))/height*gs->mapx*SQUARE_SIZE);


	CReadMap::TerrainType* tt = &readmap->terrainTypes[readmap->typemap[min(gs->hmapx*gs->hmapy-1,max(0,((int)pos.z/16)*gs->hmapx+((int)pos.x/16)))]];
	string ttype = tt->name;
	char tmp[512];
	sprintf(tmp, "Pos %.0f %.0f Elevation %.0f\n"
	             "Terrain type: %s\n"
	             "Speeds T/K/H/S %.2f %.2f %.2f %.2f\n"
	             "Hardness %.0f Metal %.1f",
	             pos.x, pos.z, ground->GetHeight2(pos.x,pos.z), ttype.c_str(),
	             tt->tankSpeed, tt->kbotSpeed, tt->hoverSpeed, tt->shipSpeed,
	             tt->hardness * mapDamage->mapHardness,
	             readmap->metalMap->getMetalAmount((int)(pos.x/16), (int)(pos.z/16)));
	return tmp;
}


void CMiniMap::AddNotification(float3 pos, float3 color, float alpha)
{
	Notification n;
	n.pos = pos;
	n.color[0] = color.x;
	n.color[1] = color.y;
	n.color[2] = color.z;
	n.color[3] = alpha;
	n.creationTime = gu->gameTime;

	notes.push_back(n);
}


/******************************************************************************/

inline void DrawInMap2D(float x, float z)
{
	glVertex2f(x, z);
}


void CMiniMap::DrawCircle(const float3& pos, float radius)
{
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(radius, 1.0f, radius);
	
	const float xPixels = radius * float(width) / float(gs->mapx * SQUARE_SIZE);
	const float yPixels = radius * float(height) / float(gs->mapy * SQUARE_SIZE);
	const int lod = (int)(0.25 * log2(1.0f + (xPixels * yPixels)));
	const int lodClamp = max(0, min(circleListsCount - 1, lod));
	glCallList(circleLists + lodClamp);

	glPopMatrix();
}


void CMiniMap::DrawSurfaceCircle(const float3& pos, float radius, unsigned int)
{
	minimap->DrawCircle(pos, radius);
}


void CMiniMap::Draw()
{
	setSurfaceCircleFunc(DrawSurfaceCircle);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (minimized) {
		const int bs = buttonSize + 0;
		glViewport(gu->viewPosX + 1, gu->viewSizeY - bs - 1, bs, bs);
		if (!buttonsTexture) {
			glDisable(GL_TEXTURE_2D);
			glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
			glRectf(0.0f, 0.0f, 1.0f, 1.0f);
		} else {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, buttonsTexture);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			const IntBox& mb = minimizeBox;
			glBegin(GL_QUADS);
				glTexCoord2f(mb.xminTx, mb.yminTx); glVertex2f(0.0f, 0.0f);
				glTexCoord2f(mb.xmaxTx, mb.yminTx); glVertex2f(1.0f, 0.0f);
				glTexCoord2f(mb.xmaxTx, mb.ymaxTx); glVertex2f(1.0f, 1.0f);
				glTexCoord2f(mb.xminTx, mb.ymaxTx); glVertex2f(0.0f, 1.0f);
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}
		// highlight
		if ((mouse->lastx < buttonSize) && (mouse->lasty < buttonSize)) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
			glRectf(0.0f, 0.0f, 1.0f, 1.0f);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		// outline
		glViewport(gu->viewPosX, gu->viewSizeY - bs - 2, bs + 2, bs + 2);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(1.51f);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glRectf(0.0f, 0.0f, 1.0f, 1.0f);
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
		return;
	}

	glViewport(xpos, ypos, width, height);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0e6, +1.0e6);
	glMatrixMode(GL_MODELVIEW);

	glColor4f(0.6f, 0.6f, 0.6f, 1.0f);

	// draw the map
	glDisable(GL_BLEND);
	readmap->DrawMinimap();
	glEnable(GL_BLEND);

	// move some of the transform to the GPU
	glTranslatef(0.0f, +1.0f, 0.0f);
	glScalef(+1.0f / (gs->mapx * SQUARE_SIZE), -1.0f / (gs->mapy * SQUARE_SIZE), 1.0);
	
	// draw the units
	list<CUnit*>::iterator ui;
	for(ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();ui++){
		DrawUnit(*ui);
	}
	
	// highlight the selected unit
	CUnit* unit = GetSelectUnit(GetMapPosition(mouse->lastx, mouse->lasty));
	if (unit != NULL) {
		DrawUnitHighlight(unit);
	}

	glDisable(GL_TEXTURE_2D);

	left.clear();

	// add restraints for camera sides
	GetFrustumSide(cam2->bottom);
	GetFrustumSide(cam2->top);
	GetFrustumSide(cam2->rightside);
	GetFrustumSide(cam2->leftside);

	// draw the camera lines
	std::vector<fline>::iterator fli,fli2;
	for(fli=left.begin();fli!=left.end();fli++){
	  for(fli2=left.begin();fli2!=left.end();fli2++){
			if(fli==fli2)
				continue;
			float colz=0;
			if(fli->dir-fli2->dir==0)
				continue;
			colz=-(fli->base-fli2->base)/(fli->dir-fli2->dir);
			if(fli2->left*(fli->dir-fli2->dir)>0){
				if(colz>fli->minz && colz<400096)
					fli->minz=colz;
			} else {
				if(colz<fli->maxz && colz>-10000)
					fli->maxz=colz;
			}
		}
	}
	glColor4f(1,1,1,0.5f);
	glBegin(GL_LINES);
	for(fli = left.begin(); fli != left.end(); fli++) {
		if(fli->minz < fli->maxz) {
			DrawInMap2D(fli->base + (fli->dir * fli->minz), fli->minz);
			DrawInMap2D(fli->base + (fli->dir * fli->maxz), fli->maxz);
		}
	}
	glEnd();

	// draw the projectiles
	glRotatef(-90.0f, +1.0f, 0.0f, 0.0f); // real 'world' coordinates

	glColor4f(1,1,1,0.0002f*(width+height));
	glBegin(GL_POINTS);
	Projectile_List::iterator psi;
	for(psi = ph->ps.begin(); psi != ph->ps.end(); ++psi) {
		CProjectile* p=*psi;
		if((p->owner && (p->owner->allyteam == gu->myAllyTeam)) ||
		   loshandler->InLos(p, gu->myAllyTeam)) {
			glVertexf3((*psi)->pos);
		}
	}
	glEnd();

	// draw the queued commands
	if ((drawCommands > 0) && guihandler->GetQueueKeystate()) {
	  // NOTE: this needlessly adds to the CursorIcons list, but at least
	  //       they are not drawn  (because the input receivers are drawn
	  //       after the command queues)
	  selectedUnits.DrawCommands();
	  glDisable(GL_DEPTH_TEST);
	  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	// draw the selection shape, and some ranges
	if (drawCommands > 0) {
		guihandler->DrawMapStuff(drawCommands);
	}
	
	// draw unit ranges
	const float radarSquare = (SQUARE_SIZE * RADAR_SIZE);
	set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	for(set<CUnit*>::iterator si = selUnits.begin(); si != selUnits.end(); ++si) {
		CUnit* unit = *si;
		if (unit->radarRadius && !unit->beingBuilt && unit->activated) {
			glColor3fv(cmdColors.rangeRadar);
			DrawCircle(unit->pos, (unit->radarRadius * radarSquare));
		}
		if (unit->sonarRadius && !unit->beingBuilt && unit->activated) {
			glColor3fv(cmdColors.rangeSonar);
			DrawCircle(unit->pos, (unit->sonarRadius * radarSquare));
		}
		if (unit->jammerRadius && !unit->beingBuilt && unit->activated) {
			glColor3fv(cmdColors.rangeJammer);
			DrawCircle(unit->pos, (unit->jammerRadius * radarSquare));
		}
		// change if someone someday create a non stockpiled interceptor
		if(unit->stockpileWeapon && unit->stockpileWeapon->weaponDef->interceptor) {
			const CWeapon* w = unit->stockpileWeapon;
			if (w->numStockpiled) {
				glColor4f(1.0f,1.0f,1.0f,1.0f);
			} else {
				glColor4f(0.0f,0.0f,0.0f,0.6f);
			}
			DrawCircle(unit->pos, w->weaponDef->coverageRange);
		}
	}

	glRotatef(+90.0f, +1.0f, 0.0f, 0.0f); // revert to the 2d xform

	// selection box
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	CMouseHandler::ButtonPress& bp = mouse->buttons[SDL_BUTTON_LEFT];
	if (selecting && fullProxy && (bp.movement > 4)) {
		const float3 oldPos = GetMapPosition(bp.x, bp.y);
		const float3 newPos = GetMapPosition(mouse->lastx, mouse->lasty);
		glColor4fv(cmdColors.mouseBox);
		glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		            (GLenum)cmdColors.MouseBoxBlendDst());
		glLineWidth(cmdColors.MouseBoxLineWidth());
		glBegin(GL_LINE_LOOP);
			DrawInMap2D(oldPos.x, oldPos.z);
			DrawInMap2D(newPos.x, oldPos.z);
			DrawInMap2D(newPos.x, newPos.z);
			DrawInMap2D(oldPos.x, newPos.z);
		glEnd();
		glLineWidth(1.0f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	DrawNotes();

	glLoadIdentity(); // reset the modelview

	DrawButtons();

	// outline
	glLineWidth(1.51f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glViewport(xpos - 1, ypos - 1, width + 2, height + 2);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glViewport(xpos - 2, ypos - 2, width + 4, height + 4);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.0f);
	
	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
	
	setSurfaceCircleFunc(NULL);
}


void CMiniMap::IntBox::DrawBox() const
{
	glViewport(xmin, gu->viewSizeY - ymax - 1,
	           (xmax - xmin) + 1, (ymax - ymin) + 1);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
}


void CMiniMap::IntBox::DrawTextureBox() const
{
	glViewport(xmin, gu->viewSizeY - ymax - 1,
	           (xmax - xmin) + 1, (ymax - ymin) + 1);
	glBegin(GL_QUADS);
		glTexCoord2f(xminTx, yminTx); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(xmaxTx, yminTx); glVertex2f(1.0f, 0.0f);
		glTexCoord2f(xmaxTx, ymaxTx); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(xminTx, ymaxTx); glVertex2f(0.0f, 1.0f);
	glEnd();
}


void CMiniMap::DrawButtons()
{
	const int x = mouse->lastx;
	const int y = mouse->lasty;
	
	// update the showButtons state
	if (!showButtons) {
		if (mapBox.Inside(x, y) && (buttonSize > 0) && !gu->dualScreenMode) {
			showButtons = true;
		} else {
			return;
		}
	}	else if (!mouseMove && !mouseResize &&
	           !mapBox.Inside(x, y) && !buttonBox.Inside(x, y)) {
		showButtons = false;
		return;
	}

	if (buttonsTexture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, buttonsTexture);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		resizeBox.DrawTextureBox();
		moveBox.DrawTextureBox();
		maximizeBox.DrawTextureBox();
		minimizeBox.DrawTextureBox();
		glDisable(GL_TEXTURE_2D);
	} else {
		glColor4f(0.1f, 0.1f, 0.8f, 0.8f); resizeBox.DrawBox();   // blue
		glColor4f(0.0f, 0.8f, 0.0f, 0.8f); moveBox.DrawBox();     // green
		glColor4f(0.8f, 0.8f, 0.0f, 0.8f); maximizeBox.DrawBox(); // yellow
		glColor4f(0.8f, 0.0f, 0.0f, 0.8f); minimizeBox.DrawBox(); // red
	}

	// highlight
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
	if (mouseResize || (!mouseMove && resizeBox.Inside(x, y))) {
		if (!buttonsTexture) { glColor4f(0.3f, 0.4f, 1.0f, 0.9f); }
		resizeBox.DrawBox();
	}
	else if (mouseMove || (!mouseResize && moveBox.Inside(x, y))) {
		if (!buttonsTexture) { glColor4f(1.0f, 1.0f, 1.0f, 0.3f); }
		moveBox.DrawBox();
	}
	else if (!mouseMove && !mouseResize) {
		if (minimizeBox.Inside(x, y)) {
			if (!buttonsTexture) { glColor4f(1.0f, 0.2f, 0.2f, 0.6f); }
			minimizeBox.DrawBox();
		} else if (maximizeBox.Inside(x, y)) {
			if (!buttonsTexture) { glColor4f(1.0f, 1.0f, 1.0f, 0.3f); }
			maximizeBox.DrawBox();
		}
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// outline the button box
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.51f);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(buttonBox.xmin - 2, (gu->viewSizeY - buttonBox.ymax - 1) - 2,
						 (buttonBox.xmax - buttonBox.xmin + 1) + 4,
						 (buttonBox.ymax - buttonBox.ymin + 1) + 4 - 3);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glViewport(buttonBox.xmin - 1, (gu->viewSizeY - buttonBox.ymax - 1) - 1,
						 (buttonBox.xmax - buttonBox.xmin + 1) + 2,
						 (buttonBox.ymax - buttonBox.ymin + 1) + 2 - 3);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void CMiniMap::GetFrustumSide(float3& side)
{
	fline temp;
	float3 up(0,1,0);

	float3 b=up.cross(side);  // get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001f)
		b.z=0.00011f;
	if(fabs(b.z)>0.0001f){
		temp.dir=b.x/b.z;       // set direction to that
		float3 c=b.cross(side); // get vector from camera to collision line
		float3 colpoint;        // a point on the collision line

		if(side.y>0)
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);
		else
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);


		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		temp.left=-1;
		if(b.z>0)
			temp.left=1;
		temp.maxz=gs->mapy*SQUARE_SIZE;
		temp.minz=0;
		left.push_back(temp);
	}

}


inline CIcon* CMiniMap::GetUnitIcon(CUnit* unit, float& scale) const
{
	scale = 1.0f;
	
	if (useIcons) {
		if ((unit->allyteam == gu->myAllyTeam) ||
				(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) ||
				((unit->losStatus[gu->myAllyTeam] & LOS_PREVLOS) &&
				 (unit->losStatus[gu->myAllyTeam] & LOS_CONTRADAR)) ||
				gu->spectatingFullView) {
			CIcon* icon = iconHandler->GetIcon(unit->unitDef->iconType);
			if (icon->radiusAdjust) {
				scale *= (unit->radius / 30.0f);
			}
			return icon;
		}
	}

	if (unit->losStatus[gu->myAllyTeam] & LOS_INRADAR) {
		return iconHandler->GetIcon("default");
	}
	
	return NULL;
}


void CMiniMap::DrawUnit(CUnit* unit)
{
	if ((unit->lastDamage > (gs->frameNum - 90)) && (gs->frameNum & 8)) {
		return;
	}

	float3 pos;
	if (gu->spectatingFullView) {
		pos = unit->midPos;
	} else {
		pos = helper->GetUnitErrorPos(unit, gu->myAllyTeam);
	}

	float iconScale;
	CIcon* icon = GetUnitIcon(unit, iconScale);
	if (icon == NULL) {
		return;
	}

	if (unit->commandAI->selected) {
		glColor3f(1.0f, 1.0f, 1.0f);
	} else {
		if (simpleColors) {
			if(unit->team==gu->myTeam) {
				glColor3ubv(myColor);
			} else if (gs->Ally(gu->myAllyTeam,unit->allyteam)){
				glColor3ubv(allyColor);
			} else {
				glColor3ubv(enemyColor);
			}
		} else {
			glColor3ubv(gs->Team(unit->team)->color);
		}
	}

	iconScale *= icon->size;
	const float sizeX = (iconScale * unitSizeX);
	const float sizeY = (iconScale * unitSizeY);
	
	glBindTexture(GL_TEXTURE_2D, icon->texture);
	glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(pos.x + sizeX, pos.z + sizeY);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(pos.x + sizeX, pos.z - sizeY);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(pos.x - sizeX, pos.z - sizeY);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(pos.x - sizeX, pos.z + sizeY);
	glEnd();
}


void CMiniMap::DrawUnitHighlight(CUnit* unit)
{
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_COPY_INVERTED);
	
	const float origX = unitSizeX;
	const float origY = unitSizeY;
	unitSizeX *= 2.0f;
	unitSizeY *= 2.0f;

	DrawUnit(unit);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_COLOR_LOGIC_OP);

	unitSizeX = origX;
	unitSizeY = origY;
	
	DrawUnit(unit);

	return;
}


void CMiniMap::DrawNotes(void)
{
	const float baseSize = gs->mapx * SQUARE_SIZE;
	glBegin(GL_LINES);
	list<Notification>::iterator ni = notes.begin();
	while (ni != notes.end()) {
		const float age = gu->gameTime - ni->creationTime;
		if (age > 2) {
			ni = notes.erase(ni);
			continue;
		}
		glColor4fv(ni->color);
		for (int a = 0; a < 3; ++a) {
			const float modage = age + a * 0.1f;
			const float rot = modage * 3;
			float size = baseSize - modage * baseSize * 0.9f;
			if (size < 0){
				if (size < -baseSize * 0.4f) {
					continue;
				} else if (size > -baseSize * 0.2f) {
					size = modage * baseSize * 0.9f - baseSize;
				} else {
					size = baseSize * 1.4f - modage * baseSize * 0.9f;
				}
			}
			const float sinSize = sin(rot) * size;
			const float cosSize = cos(rot) * size;

			DrawInMap2D(ni->pos.x + sinSize, ni->pos.z + cosSize);
			DrawInMap2D(ni->pos.x + cosSize, ni->pos.z - sinSize);
			DrawInMap2D(ni->pos.x + cosSize, ni->pos.z - sinSize);
			DrawInMap2D(ni->pos.x - sinSize, ni->pos.z - cosSize);
			DrawInMap2D(ni->pos.x - sinSize, ni->pos.z - cosSize);
			DrawInMap2D(ni->pos.x - cosSize, ni->pos.z + sinSize);
			DrawInMap2D(ni->pos.x - cosSize, ni->pos.z + sinSize);
			DrawInMap2D(ni->pos.x + sinSize, ni->pos.z + cosSize);
		}
		++ni;
	}
	glEnd();
}


/******************************************************************************/
