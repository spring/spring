/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keysym.h>
#include <SDL_mouse.h>

#include "StdAfx.h"
#include "mmgr.h"
#include "lib/gml/ThreadSafeContainers.h"

#include "CommandColors.h"
#include "CursorIcons.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "TooltipConsole.h"
#include "Game/Camera/CameraController.h"
#include "Game/Camera/OverheadController.h"
#include "Game/Camera/SmoothController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/UnitTracker.h"
#include "Sim/Misc/TeamHandler.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Util.h"
#include "System/TimeProfiler.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/IEffectChannel.h"

#include <boost/cstdint.hpp>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CMiniMap* minimap = NULL;

CMiniMap::CMiniMap()
: CInputReceiver(BACK),
  fullProxy(false),
  proxyMode(false),
  selecting(false),
  maxspect(false),
  maximized(false),
  minimized(false),
  mouseLook(false),
  mouseMove(false),
  mouseResize(false),
  slaveDrawMode(false),
  showButtons(false),
  useIcons(true)
 {
	lastWindowSizeX = globalRendering->viewSizeX;
	lastWindowSizeY = globalRendering->viewSizeY;

	if (globalRendering->dualScreenMode) {
		width = globalRendering->viewSizeX;
		height = globalRendering->viewSizeY;
		xpos = (globalRendering->viewSizeX - globalRendering->viewPosX);
		ypos = 0;
	}
	else {
		const std::string geodef = "2 2 200 200";
		const std::string geo = configHandler->GetString("MiniMapGeometry", geodef);
		ParseGeometry(geo);
	}

	fullProxy = !!configHandler->Get("MiniMapFullProxy", 1);
	buttonSize = configHandler->Get("MiniMapButtonSize", 16);

	unitBaseSize = configHandler->Get("MiniMapUnitSize", 2.5f);
	unitBaseSize = std::max(0.0f, unitBaseSize);
	unitExponent = configHandler->Get("MiniMapUnitExp", 0.25f);

	cursorScale = configHandler->Get("MiniMapCursorScale", -0.5f);
	useIcons = !!configHandler->Get("MiniMapIcons", 1);
	drawCommands = std::max(0, configHandler->Get("MiniMapDrawCommands", 1));
	drawProjectiles = !!configHandler->Get("MiniMapDrawProjectiles", 1);
	simpleColors = !!configHandler->Get("SimpleMiniMapColors", 0);

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
	ypos = globalRendering->viewSizeY - height - ypos;
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
		height = globalRendering->viewSizeY;
		width = height;
		xpos = (globalRendering->viewSizeX - globalRendering->viewSizeY) / 2;
		ypos = 0;
	}
	else {
		const float mapRatio = (float)gs->mapx / (float)gs->mapy;
		const float viewRatio = globalRendering->aspectRatio;
		if (mapRatio > viewRatio) {
			xpos = 0;
			width = globalRendering->viewSizeX;
			height = int((float)globalRendering->viewSizeX / mapRatio);
			ypos = (globalRendering->viewSizeY - height) / 2;
		} else {
			ypos = 0;
			height = globalRendering->viewSizeY;
			width = int((float)globalRendering->viewSizeY * mapRatio);
			xpos = (globalRendering->viewSizeX - width) / 2;
		}
	}
}


/******************************************************************************/

void CMiniMap::SetSlaveMode(bool newMode)
{
	if (newMode) {
		proxyMode   = false;
		selecting   = false;
		maxspect    = false;
		maximized   = false;
		minimized   = false;
		mouseLook   = false;
		mouseMove   = false;
		mouseResize = false;
	}
	static int oldButtonSize = 16;
	if (newMode != slaveDrawMode) {
		if (newMode) {
			oldButtonSize = buttonSize;
			buttonSize = 0;
		} else {
			buttonSize = oldButtonSize;
		}
	}
	slaveDrawMode = newMode;
	UpdateGeometry();
}


void CMiniMap::ConfigCommand(const std::string& line)
{
	const vector<string> &words = CSimpleParser::Tokenize(line, 1);
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
		unitBaseSize = std::max(0.0f, unitBaseSize);
		UpdateGeometry();
	}
	else if (command == "drawcommands") {
		if (words.size() >= 2) {
			drawCommands = std::max(0, atoi(words[1].c_str()));
		} else {
			drawCommands = (drawCommands > 0) ? 0 : 1;
		}
	}
	else if (command == "drawprojectiles") {
		if (words.size() >= 2) {
			drawProjectiles = !!atoi(words[1].c_str());
		} else {
			drawProjectiles = !drawProjectiles;
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
	if (globalRendering->dualScreenMode) {
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

void CMiniMap::SetGeometry(int px, int py, int sx, int sy)
{
	xpos = px;
	ypos = py;
	width = sx;
	height = sy;
	UpdateGeometry();
}


void CMiniMap::UpdateGeometry()
{
	// keep the same distance to the top
	ypos -= (lastWindowSizeY - globalRendering->viewSizeY);
	if (globalRendering->dualScreenMode) {
		width = globalRendering->viewSizeX;
		height = globalRendering->viewSizeY;
		xpos = (globalRendering->viewSizeX - globalRendering->viewPosX);
		ypos = 0;
	}
	else if (maximized) {
		SetMaximizedGeometry();
	}
	else {
		width = std::max(1, std::min(width, globalRendering->viewSizeX));
		height = std::max(1, std::min(height, globalRendering->viewSizeY));
		ypos = std::max(slaveDrawMode ? 0 : buttonSize, ypos);
		ypos = std::min(globalRendering->viewSizeY - height, ypos);
		xpos = std::max(0, std::min(globalRendering->viewSizeX - width, xpos));
	}
	lastWindowSizeX = globalRendering->viewSizeX;
	lastWindowSizeY = globalRendering->viewSizeY;

	// setup the unit scaling
	const float w = float(width);
	const float h = float(height);
	const float mapx = float(gs->mapx * SQUARE_SIZE);
	const float mapy = float(gs->mapy * SQUARE_SIZE);
	const float ref  = unitBaseSize / pow((200.f * 200.0f), unitExponent);
	const float dpr  = ref * pow((w * h), unitExponent);

	unitSizeX = dpr * (mapx / w);
	unitSizeY = dpr * (mapy / h);
	unitSelectRadius = fastmath::apxsqrt(unitSizeX * unitSizeY);

	// in mouse coordinates
	mapBox.xmin = xpos;
	mapBox.xmax = mapBox.xmin + width - 1;
	mapBox.ymin = globalRendering->viewSizeY - (ypos + height);
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
	float dist = ground->LineGroundCol(pos, pos + (dir * globalRendering->viewRange * 1.4f));
	float3 dif(0,0,0);
	if (dist > 0) {
		dif = dir * dist;
	}
	float3 clickPos;
	clickPos.x = (float(x - xpos)) / width * gs->mapx * 8;
	clickPos.z = (float(y - (globalRendering->viewSizeY - ypos - height))) / height * gs->mapy * 8;
	camHandler->GetCurrentController().SetPos(clickPos);
	unitTracker.Disable();
}


void CMiniMap::SelectUnits(int x, int y) const
{
	GML_RECMUTEX_LOCK(sel); // SelectUnits

	if (!keyInput->IsKeyPressed(SDLK_LSHIFT) && !keyInput->IsKeyPressed(SDLK_LCTRL)) {
		selectedUnits.ClearSelected();
	}

	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];

	if (fullProxy && (bp.movement > 4)) {
		// use a selection box
		const float3 newPos = GetMapPosition(x, y);
		const float3 oldPos = GetMapPosition(bp.x, bp.y);
		const float xmin = std::min(oldPos.x, newPos.x);
		const float xmax = std::max(oldPos.x, newPos.x);
		const float zmin = std::min(oldPos.z, newPos.z);
		const float zmax = std::max(oldPos.z, newPos.z);

		CUnitSet::iterator ui;
		CUnitSet& selection = selectedUnits.selectedUnits;

		CUnit* unit = NULL;
		int addedunits = 0;
		int team, lastTeam;

		if (gu->spectatingFullSelect) {
			team = 0;
			lastTeam = teamHandler->ActiveTeams() - 1;
		} else {
			team = gu->myTeam;
			lastTeam = gu->myTeam;
		}
		for (; team <= lastTeam; team++) {
			CUnitSet& teamUnits = teamHandler->Team(team)->units;
			for (ui = teamUnits.begin(); ui != teamUnits.end(); ++ui) {
				const float3& midPos = (*ui)->midPos;

				if ((midPos.x > xmin) && (midPos.x < xmax) &&
					(midPos.z > zmin) && (midPos.z < zmax)) {

					if (keyInput->IsKeyPressed(SDLK_LCTRL) && (selection.find(*ui) != selection.end())) {
						selectedUnits.RemoveUnit(*ui);
					} else {
						selectedUnits.AddUnit(*ui);
						unit = *ui;
						addedunits++;
					}
				}
			}
		}

		if (addedunits >= 2) {
			Channels::UserInterface.PlaySample(mouse->soundMultiselID);
		}
		else if (addedunits == 1) {
			int soundIdx = unit->unitDef->sounds.select.getRandomIdx();
			if (soundIdx >= 0) {
				Channels::UnitReply.PlaySample(
					unit->unitDef->sounds.select.getID(soundIdx), unit,
					unit->unitDef->sounds.select.getVolume(soundIdx));
			}
		}
	}
	else {
		// Single unit
		const float size = unitSelectRadius;
		const float3 pos = GetMapPosition(x, y);

		CUnit* unit;
		if (gu->spectatingFullSelect) {
			unit = helper->GetClosestUnit(pos, size);
		} else {
			unit = helper->GetClosestFriendlyUnit(pos, size, gu->myAllyTeam);
		}

		CUnitSet::iterator ui;
		CUnitSet& selection = selectedUnits.selectedUnits;

		if (unit && ((unit->team == gu->myTeam) || gu->spectatingFullSelect)) {
			if (bp.lastRelease < (gu->gameTime - mouse->doubleClickTime)) {
				if (keyInput->IsKeyPressed(SDLK_LCTRL) && (selection.find(unit) != selection.end())) {
					selectedUnits.RemoveUnit(unit);
				} else {
					selectedUnits.AddUnit(unit);
				}
			} else {
				//double click
				if (unit->group && !keyInput->IsKeyPressed(SDLK_LCTRL)) {
					//select the current units group if it has one
					selectedUnits.SelectGroup(unit->group->id);
				} else {
					//select all units of same type
					int team, lastTeam;
					if (gu->spectatingFullSelect) {
						team = 0;
						lastTeam = teamHandler->ActiveTeams() - 1;
					} else {
						team = gu->myTeam;
						lastTeam = gu->myTeam;
					}
					for (; team <= lastTeam; team++) {
						CUnitSet& myUnits = teamHandler->Team(team)->units;
						for (ui = myUnits.begin(); ui != myUnits.end(); ++ui) {
							if ((*ui)->unitDef->id == unit->unitDef->id) {
								selectedUnits.AddUnit(*ui);
							}
						}
					}
				}
			}
			bp.lastRelease = gu->gameTime;

			int soundIdx = unit->unitDef->sounds.select.getRandomIdx();
			if (soundIdx >= 0) {
				Channels::UnitReply.PlaySample(
					unit->unitDef->sounds.select.getID(soundIdx), unit,
					unit->unitDef->sounds.select.getVolume(soundIdx));
			}
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
			if (maximized) {
				ToggleMaximized(false);
			} else {
				mouseLook = true;
			}
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
		xpos = std::max(0, xpos);
		if (globalRendering->dualScreenMode) {
			xpos = std::min((2 * globalRendering->viewSizeX) - width, xpos);
		} else {
			xpos = std::min(globalRendering->viewSizeX - width, xpos);
		}
		ypos = std::max(5, std::min(globalRendering->viewSizeY - height, ypos));
		UpdateGeometry();
		return;
	}

	if (mouseResize) {
		ypos   -= dy;
		width  += dx;
		height += dy;
		height = std::min(globalRendering->viewSizeY, height);
		if (globalRendering->dualScreenMode) {
			width = std::min(2 * globalRendering->viewSizeX, width);
		} else {
			width = std::min(globalRendering->viewSizeX, width);
		}
		if (keyInput->IsKeyPressed(SDLK_LSHIFT)) {
			width = (height * gs->mapx) / gs->mapy;
		}
		width = std::max(5, width);
		height = std::max(5, height);
		ypos = std::min(globalRendering->viewSizeY - height, ypos);
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
			ToggleMaximized(!!keyInput->GetKeyState(SDLK_LSHIFT));
			return;
		}

		if (showButtons && minimizeBox.Inside(x, y)) {
			minimized = true;
			return;
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
	                 mapY * float(y - (globalRendering->viewSizeY - ypos - height)) / height);
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

	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	bp.camPos = mapPos;
	bp.dir = float3(0.0f, -1.0f, 0.0f);

	guihandler->MousePress(x, y, -button);
}


void CMiniMap::ProxyMouseRelease(int x, int y, int button)
{
	// is this really needed?
//	CCamera *c = camera;
//	camera = new CCamera(*c);

//	const float3 tmpMouseDir = mouse->dir;

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

	float3 mousedir = float3(0.0f, -1.0f, 0.0f);
	float3 campos = mapPos;
//	float3 camfwd = mousedir; // not used?

	guihandler->MouseRelease(x, y, -button, campos, mousedir);

//	mouse->dir = tmpMouseDir;
//	delete camera;
//	camera = c;
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

	GML_RECMUTEX_LOCK(sel); // GetToolTip - anti deadlock
	GML_RECMUTEX_LOCK(quad); // GetToolTip - called from TooltipConsole::Draw --> MouseHandler::GetCurrentTooltip

	const CUnit* unit = GetSelectUnit(GetMapPosition(x, y));
	if (unit) {
		return CTooltipConsole::MakeUnitString(unit);
	}

	const string selTip = selectedUnits.GetTooltip();
	if (selTip != "") {
		return selTip;
	}

	const float3 pos(float(x-xpos)/width*gs->mapx*SQUARE_SIZE, 500,
	                 float(y-(globalRendering->viewSizeY-ypos-height))/height*gs->mapx*SQUARE_SIZE);
	return CTooltipConsole::MakeGroundString(pos);
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

void CMiniMap::DrawCircle(const float3& pos, float radius)
{
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(radius, 1.0f, radius);

	const float xPixels = radius * float(width) / float(gs->mapx * SQUARE_SIZE);
	const float yPixels = radius * float(height) / float(gs->mapy * SQUARE_SIZE);
	const int lod = (int)(0.25 * log2(1.0f + (xPixels * yPixels)));
	const int lodClamp = std::max(0, std::min(circleListsCount - 1, lod));
	glCallList(circleLists + lodClamp);

	glPopMatrix();
}

void CMiniMap::DrawSquare(const float3& pos, float xsize, float zsize)
{
	float verts[] = {
		pos.x + xsize, 0.0f, pos.z + zsize,
		pos.x - xsize, 0.0f, pos.z + zsize,
		pos.x - xsize, 0.0f, pos.z - zsize,
		pos.x + xsize, 0.0f, pos.z - zsize

	};
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void CMiniMap::DrawSurfaceCircle(const float3& pos, float radius, unsigned int)
{
	minimap->DrawCircle(pos, radius);
}
void CMiniMap::DrawSurfaceSquare(const float3& pos, float xsize, float ysize)
{
	minimap->DrawSquare(pos, xsize, ysize);
}


void CMiniMap::Draw()
{
	if (!slaveDrawMode) {
		DrawForReal(true);
	}
}


void CMiniMap::DrawForReal(bool use_geo)
{
	SCOPED_TIMER("Draw minimap");

	//glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);

	if (minimized) {
		if (!slaveDrawMode) {
			DrawMinimizedButton();
		}
		glPopAttrib();
		glEnable(GL_TEXTURE_2D);
		return;
	}

	// draw the frameborder
	if (!slaveDrawMode && !globalRendering->dualScreenMode && !maximized) {
		glEnable(GL_BLEND);
		DrawFrame();
		glDisable(GL_BLEND);
	}


	bool resetTextureMatrix = false;

	if (use_geo) {
		glPushMatrix();

		// switch to normalized minimap coords
		if (globalRendering->dualScreenMode) {
			glViewport(xpos, ypos, width, height);
			glScalef(width * globalRendering->pixelX, height * globalRendering->pixelY, 1.0f);
		} else {
			glTranslatef(xpos * globalRendering->pixelX, ypos * globalRendering->pixelY, 0.0f);
			glScalef(width * globalRendering->pixelX, height * globalRendering->pixelY, 1.0f);
		}

		/* FIXME: fix mouse handling too and make it fully customizable, so Lua can rotate the minimap to any angle
		CCameraController* camController = &camHandler->GetCurrentController();
		COverheadController* taCam = dynamic_cast<COverheadController*>(camController);
		SmoothController* smCam = dynamic_cast<SmoothController*>(camController);

		if ((taCam && taCam->flipped) || (smCam && smCam->flipped)) {
			glTranslatef(1.0f, 1.0f, 0.0f);
			glScalef(-1.0f, -1.0f, 1.0f);

			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glTranslatef(1.0f, 1.0f, 0.0f);
			glScalef(-1.0f, -1.0f, 1.0f);
			glMatrixMode(GL_MODELVIEW);

			resetTextureMatrix = true;
		}*/
	}

	setSurfaceCircleFunc(DrawSurfaceCircle);
	setSurfaceSquareFunc(DrawSurfaceSquare);
	cursorIcons.Enable(false);

	glColor4f(0.6f, 0.6f, 0.6f, 1.0f);

	// don't mirror the map texture with flipped cameras
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// draw the map
	glDisable(GL_BLEND);
		readmap->DrawMinimap();
	glEnable(GL_BLEND);

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	// clip everything outside of the minimap box
	{
		const double plane0[4] = {0,-1,0,1};
		const double plane1[4] = {0,1,0,0};
		const double plane2[4] = {-1,0,0,1};
		const double plane3[4] = {1,0,0,0};

		glClipPlane(GL_CLIP_PLANE0, plane0); // clip bottom
		glClipPlane(GL_CLIP_PLANE1, plane1); // clip top
		glClipPlane(GL_CLIP_PLANE2, plane2); // clip right
		glClipPlane(GL_CLIP_PLANE3, plane3); // clip left

		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
		glEnable(GL_CLIP_PLANE2);
		glEnable(GL_CLIP_PLANE3);
	}

	// switch to top-down map/world coords (z is twisted with y compared to the real map/world coords)
	glPushMatrix();
	glTranslatef(0.0f, +1.0f, 0.0f);
	glScalef(+1.0f / (gs->mapx * SQUARE_SIZE), -1.0f / (gs->mapy * SQUARE_SIZE), 1.0f);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);

	{
		GML_RECMUTEX_LOCK(unit); // DrawForReal

		const std::set<CUnit*>& units = unitDrawer->GetUnsortedUnits();

		for (std::set<CUnit*>::const_iterator it = units.begin(); it != units.end(); ++it) {
			DrawUnit(*it);
		}

		// highlight the selected unit
		CUnit* unit = GetSelectUnit(GetMapPosition(mouse->lastx, mouse->lasty));
		if (unit != NULL) {
			DrawUnitHighlight(unit);
		}
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glRotatef(-90.0f, +1.0f, 0.0f, 0.0f); // real 'world' coordinates
	glScalef(1.0f, 0.0f, 1.0f); // skip the y-coord (Lua's DrawScreen is perspective and so any z-coord in it influence the x&y, too)

	// draw the projectiles
	if (drawProjectiles) {
		projectileDrawer->DrawProjectilesMiniMap();
	}

	// draw the queued commands
	//
	// NOTE: this needlessly adds to the CursorIcons list, but at least
	//       they are not drawn  (because the input receivers are drawn
	//       after the command queues)

	LuaUnsyncedCtrl::DrawUnitCommandQueues();
	if ((drawCommands > 0) && guihandler->GetQueueKeystate()) {
		selectedUnits.DrawCommands();
	}

	lineDrawer.DrawAll();

	// draw the selection shape, and some ranges
	if (drawCommands > 0) {
		guihandler->DrawMapStuff(!!drawCommands);
	}

	{
		GML_RECMUTEX_LOCK(sel); // DrawForReal

		// draw unit ranges
		const float radarSquare = radarhandler->radarDiv;
		CUnitSet& selUnits = selectedUnits.selectedUnits;
		for(CUnitSet::iterator si = selUnits.begin(); si != selUnits.end(); ++si) {
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
			const CWeapon* w = unit->stockpileWeapon;
			if((w != NULL) && w->weaponDef->interceptor) {
				if (w->numStockpiled) {
					glColor3fv(cmdColors.rangeInterceptorOn);
				} else {
					glColor3fv(cmdColors.rangeInterceptorOff);
				}
				DrawCircle(unit->pos, w->weaponDef->coverageRange);
			}
		}
	}

	glPopMatrix(); // revert to the 2d xform

	// Draw the Camera Frustum
	left.clear();
	GetFrustumSide(cam2->bottom);
	GetFrustumSide(cam2->top);
	GetFrustumSide(cam2->rightside);
	GetFrustumSide(cam2->leftside);

	if (!minimap->maximized) {
		// draw the camera lines
		std::vector<fline>::iterator fli,fli2;
		for(fli=left.begin();fli!=left.end();++fli){
			for(fli2=left.begin();fli2!=left.end();++fli2){
				if(fli==fli2)
					continue;
				if(fli->dir - fli2->dir == 0)
					continue;
				float colz = -(fli->base - fli2->base) / (fli->dir - fli2->dir);
				if (fli2->left * (fli->dir - fli2->dir) > 0) {
					if (colz > fli->minz && colz < 400096)
						fli->minz = colz;
				} else {
					if (colz < fli->maxz && colz > -10000)
						fli->maxz = colz;
				}
			}
		}
		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(left.size()*2, 0, VA_SIZE_2D0);
		for(fli = left.begin(); fli != left.end(); ++fli) {
			if(fli->minz < fli->maxz) {
				va->AddVertex2dQ0(fli->base + (fli->dir * fli->minz), fli->minz);
				va->AddVertex2dQ0(fli->base + (fli->dir * fli->maxz), fli->maxz);
			}
		}
		glLineWidth(2.5f);
		glColor4f(0,0,0,0.5f);
		va->DrawArray2d0(GL_LINES);

		glLineWidth(1.5f);
		glColor4f(1,1,1,0.75f);
		va->DrawArray2d0(GL_LINES);
		glLineWidth(1.0f);
	}

	// selection box
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];
	if (selecting && fullProxy && (bp.movement > 4)) {
		const float3 oldPos = GetMapPosition(bp.x, bp.y);
		const float3 newPos = GetMapPosition(mouse->lastx, mouse->lasty);
		glColor4fv(cmdColors.mouseBox);
		glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		            (GLenum)cmdColors.MouseBoxBlendDst());
		glLineWidth(cmdColors.MouseBoxLineWidth());

		float verts[] = {
			oldPos.x, oldPos.z,
			newPos.x, oldPos.z,
			newPos.x, newPos.z,
			oldPos.x, newPos.z,
		};
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);

		glLineWidth(1.0f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	DrawNotes();

	// reset 1
	if (resetTextureMatrix) {
		glMatrixMode(GL_TEXTURE_MATRIX);
		glPopMatrix();
	}
	glMatrixMode(GL_MODELVIEW);
	if (use_geo) {
		glPopMatrix();
	}

	// reset 2
	glPopMatrix();
	glPopAttrib();
	glEnable(GL_TEXTURE_2D);

	{
		//! prepare ClipPlanes for Lua's DrawInMinimap Modelview matrix

		//! quote from glClipPlane spec:
		//! "When glClipPlane is called, equation is transformed by the inverse of the modelview matrix and stored in the resulting eye coordinates.
		//!  Subsequent changes to the modelview matrix have no effect on the stored plane-equation components."
		//! -> we have to use the same modelview matrix when calling glClipPlane and later draw calls

		//! set the modelview matrix to the same as used in Lua's DrawInMinimap
		glPushMatrix();
		glLoadIdentity();
		glScalef(1.0f / width, 1.0f / height, 1.0f);

		const double plane0[4] = {0,-1,0,height};
		const double plane1[4] = {0,1,0,0};
		const double plane2[4] = {-1,0,0,width};
		const double plane3[4] = {1,0,0,0};

		glClipPlane(GL_CLIP_PLANE0, plane0); // clip bottom
		glClipPlane(GL_CLIP_PLANE1, plane1); // clip top
		glClipPlane(GL_CLIP_PLANE2, plane2); // clip right
		glClipPlane(GL_CLIP_PLANE3, plane3); // clip left

		glPopMatrix();
	}

	//! allow the LUA scripts to draw into the minimap
	eventHandler.DrawInMiniMap();

	if (use_geo && globalRendering->dualScreenMode)
		glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);

	//FIXME: Lua modifies the matrices w/o reseting it! (quite complexe to fix because ClearMatrixStack() makes it impossible to use glPushMatrix)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// disable ClipPlanes
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);

	cursorIcons.Enable(true);
	setSurfaceCircleFunc(NULL);
	setSurfaceSquareFunc(NULL);
}


void CMiniMap::DrawFrame()
{
	// outline
	glLineWidth(1.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(float(xpos - 2 + 0.5f) * globalRendering->pixelX,         float(ypos - 2 + 0.5f) * globalRendering->pixelY);
		glVertex2f(float(xpos - 2 + 0.5f) * globalRendering->pixelX,         float(ypos + height + 2 - 0.5f) * globalRendering->pixelY);
		glVertex2f(float(xpos + width + 2 - 0.5f) * globalRendering->pixelX, float(ypos + height + 2 - 0.5f) * globalRendering->pixelY);
		glVertex2f(float(xpos + width + 2 - 0.5f) * globalRendering->pixelX, float(ypos - 2 + 0.5f) * globalRendering->pixelY);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(float(xpos - 1 + 0.5f) * globalRendering->pixelX,         float(ypos - 1 + 0.5f) * globalRendering->pixelY);
		glVertex2f(float(xpos - 1 + 0.5f) * globalRendering->pixelX,         float(ypos + height + 1 - 0.5f) * globalRendering->pixelY);
		glVertex2f(float(xpos + width + 1 - 0.5f) * globalRendering->pixelX, float(ypos + height + 1 - 0.5f) * globalRendering->pixelY);
		glVertex2f(float(xpos + width + 1 - 0.5f) * globalRendering->pixelX, float(ypos - 1 + 0.5f) * globalRendering->pixelY);
	glEnd();

	DrawButtons();
}


void CMiniMap::DrawMinimizedButton()
{
	const int bs = buttonSize + 0;

	float xmin = (globalRendering->viewPosX + 1) * globalRendering->pixelX;
	float xmax = (globalRendering->viewPosX + 1 + bs) * globalRendering->pixelX;
	float ymin = 1.0f - (1 + bs) * globalRendering->pixelY;
	float ymax = 1.0f - (1) * globalRendering->pixelY;

	if (!buttonsTexture) {
		glDisable(GL_TEXTURE_2D);
		glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
		glRectf(xmin, ymin, xmax, ymax);
	} else {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, buttonsTexture);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		const IntBox& mb = minimizeBox;
		glBegin(GL_QUADS);
			glTexCoord2f(mb.xminTx, mb.yminTx); glVertex2f(xmin, ymin);
			glTexCoord2f(mb.xmaxTx, mb.yminTx); glVertex2f(xmax, ymin);
			glTexCoord2f(mb.xmaxTx, mb.ymaxTx); glVertex2f(xmax, ymax);
			glTexCoord2f(mb.xminTx, mb.ymaxTx); glVertex2f(xmin, ymax);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	// highlight
	if ((mouse->lastx+1 <= buttonSize) && (mouse->lasty+1 <= buttonSize)) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
		glRectf(xmin, ymin, xmax, ymax);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// outline
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(xmin - 0.5f * globalRendering->pixelX, ymax + 0.5f * globalRendering->pixelY);
		glVertex2f(xmin - 0.5f * globalRendering->pixelX, ymin - 0.5f * globalRendering->pixelY);
		glVertex2f(xmax + 0.5f * globalRendering->pixelX, ymin - 0.5f * globalRendering->pixelY);
		glVertex2f(xmax + 0.5f * globalRendering->pixelX, ymax + 0.5f * globalRendering->pixelY);
	glEnd();
}


void CMiniMap::IntBox::DrawBox() const
{
	glRectf(float(xmin)/globalRendering->viewSizeX, 1.0f - float(ymin)/globalRendering->viewSizeY, float(xmax+1)/globalRendering->viewSizeX, 1.0f - float(ymax+1)/globalRendering->viewSizeY);
}


void CMiniMap::IntBox::DrawTextureBox() const
{
	glBegin(GL_QUADS);
		glTexCoord2f(xminTx, yminTx); glVertex2f(float(xmin) * globalRendering->pixelX, 1.0f - float(ymin) * globalRendering->pixelY);
		glTexCoord2f(xmaxTx, yminTx); glVertex2f(float(xmax+1) * globalRendering->pixelX, 1.0f - float(ymin) * globalRendering->pixelY);
		glTexCoord2f(xmaxTx, ymaxTx); glVertex2f(float(xmax+1) * globalRendering->pixelX, 1.0f - float(ymax+1) * globalRendering->pixelY);
		glTexCoord2f(xminTx, ymaxTx); glVertex2f(float(xmin) * globalRendering->pixelX, 1.0f - float(ymax+1) * globalRendering->pixelY);
	glEnd();
}


void CMiniMap::DrawButtons()
{
	const int x = mouse->lastx;
	const int y = mouse->lasty;

	// update the showButtons state
	if (!showButtons) {
		if (mapBox.Inside(x, y) && (buttonSize > 0) && !globalRendering->dualScreenMode) {
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
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(float(buttonBox.xmin - 1 - 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymin + 2 - 0.5f) * globalRendering->pixelY);
		glVertex2f(float(buttonBox.xmin - 1 - 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymax + 2 + 0.5f) * globalRendering->pixelY);
		glVertex2f(float(buttonBox.xmax + 2 + 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymax + 2 + 0.5f) * globalRendering->pixelY);
		glVertex2f(float(buttonBox.xmax + 2 + 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymin + 2 - 0.5f) * globalRendering->pixelY);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(float(buttonBox.xmin - 0 - 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymin + 3 - 0.5f) * globalRendering->pixelY);
		glVertex2f(float(buttonBox.xmin - 0 - 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymax + 1 + 0.5f) * globalRendering->pixelY);
		glVertex2f(float(buttonBox.xmax + 1 + 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymax + 1 + 0.5f) * globalRendering->pixelY);
		glVertex2f(float(buttonBox.xmax + 1 + 0.5f) * globalRendering->pixelX, 1.0f - float(buttonBox.ymin + 3 - 0.5f) * globalRendering->pixelY);
	glEnd();
}


void CMiniMap::GetFrustumSide(float3& side)
{
	fline temp;
	static const float3 up(0,1,0);

	float3 b=up.cross(side); // get vector for collision between frustum and horizontal plane

	if(fabs(b.z) < 0.0001f)  // prevent div by zero
		b.z = 0.00011f;

	temp.dir = b.x / b.z;      // set direction to that
	float3 c = b.cross(side);  // get vector from camera to collision line
	float3 colpoint = cam2->pos - c * (cam2->pos.y/c.y); // a point on the collision line

	temp.base = colpoint.x - colpoint.z * temp.dir; //get intersection between colpoint and z axis
	temp.left = -1;
	if(b.z>0)
		temp.left = 1;
	temp.maxz = gs->mapy * SQUARE_SIZE;
	temp.minz = 0;
	left.push_back(temp);
}


inline const CIconData* CMiniMap::GetUnitIcon(const CUnit* unit, float& scale) const
{
	scale = 1.0f;

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);

	//! show unitdef icon
	if (useIcons) {
		if (
			   (losStatus & LOS_INLOS)
			|| ((losStatus & LOS_INRADAR)&&((losStatus & prevMask) == prevMask))
			|| gu->spectatingFullView)
		{
			const CIconData* iconData = unit->unitDef->iconType.GetIconData();
			if (iconData->GetRadiusAdjust()) {
				scale *= (unit->radius / 30.0f);
			}
			return iconData;
		}
	}

	//! show default icon (unknown unitdef)
	if (losStatus & LOS_INRADAR) {
		return iconHandler->GetDefaultIconData();
	}

	return NULL;
}


void CMiniMap::DrawUnit(const CUnit* unit)
{
	// the simplest test
	if (!unit)
		return;

	// the next simplest test
	if (unit->noMinimap) {
		return;
	}

	// includes the visibility check
	float iconScale;
	const CIconData* iconData = GetUnitIcon(unit, iconScale);
	if (iconData == NULL) {
		return;
	}

	// get the position
	float3 pos;
	if (gu->spectatingFullView) {
		pos = unit->midPos;
	} else {
		pos = helper->GetUnitErrorPos(unit, gu->myAllyTeam);
	}

	// set the color
	if (unit->commandAI->selected) {
		glColor3f(1.0f, 1.0f, 1.0f);
	}
	else {
		if (simpleColors) {
			if (unit->team == gu->myTeam) {
				glColor3ubv(myColor);
			} else if (teamHandler->Ally(gu->myAllyTeam, unit->allyteam)) {
				glColor3ubv(allyColor);
			} else {
				glColor3ubv(enemyColor);
			}
		} else {
			glColor3ubv(teamHandler->Team(unit->team)->color);
		}
	}

	iconScale *= iconData->GetSize();
	const float sizeX = (iconScale * unitSizeX);
	const float sizeY = (iconScale * unitSizeY);

	const float x0 = pos.x - sizeX;
	const float x1 = pos.x + sizeX;
	const float y0 = pos.z - sizeY;
	const float y1 = pos.z + sizeY;

	iconData->Draw(x0, y0, x1, y1);
}


void CMiniMap::DrawUnitHighlight(const CUnit* unit)
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


void CMiniMap::DrawNotes()
{
	if (notes.size()<=0) return;

	const float baseSize = gs->mapx * SQUARE_SIZE;
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	std::list<Notification>::iterator ni = notes.begin();
	while (ni != notes.end()) {
		const float age = gu->gameTime - ni->creationTime;
		if (age > 2) {
			ni = notes.erase(ni);
			continue;
		}
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
			const float sinSize = fastmath::sin(rot) * size;
			const float cosSize = fastmath::cos(rot) * size;

			const unsigned char color[4]    = {
			      (unsigned char)(ni->color[0] * 255),
			      (unsigned char)(ni->color[1] * 255),
			      (unsigned char)(ni->color[2] * 255),
			      (unsigned char)(ni->color[3] * 255)
			};
			va->AddVertexC(float3(ni->pos.x + sinSize, ni->pos.z + cosSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x + cosSize, ni->pos.z - sinSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x + cosSize, ni->pos.z - sinSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x - sinSize, ni->pos.z - cosSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x - sinSize, ni->pos.z - cosSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x - cosSize, ni->pos.z + sinSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x - cosSize, ni->pos.z + sinSize, 0.0f),color);
			va->AddVertexC(float3(ni->pos.x + sinSize, ni->pos.z + cosSize, 0.0f),color);
		}
		++ni;
	}
	va->DrawArrayC(GL_LINES);
}


/******************************************************************************/
