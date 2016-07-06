/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include "CommandColors.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "TooltipConsole.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Game/UI/UnitTracker.h"
#include "Sim/Misc/TeamHandler.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Util.h"
#include "System/TimeProfiler.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Sound/ISoundChannels.h"

#include <boost/cstdint.hpp>

CONFIG(std::string, MiniMapGeometry).defaultValue("2 2 200 200");
CONFIG(bool, MiniMapFullProxy).defaultValue(true);
CONFIG(int, MiniMapButtonSize).defaultValue(16);

CONFIG(float, MiniMapUnitSize)
	.defaultValue(2.5f)
	.minimumValue(0.0f);

CONFIG(float, MiniMapUnitExp).defaultValue(0.25f);
CONFIG(float, MiniMapCursorScale).defaultValue(-0.5f);
CONFIG(bool, MiniMapIcons).defaultValue(true).headlessValue(false);

CONFIG(int, MiniMapDrawCommands).defaultValue(1).headlessValue(0).minimumValue(0);

CONFIG(bool, MiniMapDrawProjectiles).defaultValue(true).headlessValue(false);
CONFIG(bool, SimpleMiniMapColors).defaultValue(false);

CONFIG(bool, MiniMapRenderToTexture).defaultValue(true).safemodeValue(false).description("Asynchronous render MiniMap to a texture independent of screen FPS.");
CONFIG(int, MiniMapRefreshRate).defaultValue(0).minimumValue(0).description("The refresh rate of the async MiniMap texture. Needs MiniMapRenderToTexture to be true. Value of \"0\" autoselects between 10-60FPS.");



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CMiniMap* minimap = NULL;

CMiniMap::CMiniMap()
	: CInputReceiver(BACK)
	, fullProxy(false)
	, proxyMode(false)
	, selecting(false)
	, maxspect(false)
	, maximized(false)
	, minimized(false)
	, mouseLook(false)
	, mouseMove(false)
	, mouseResize(false)
	, slaveDrawMode(false)
	, showButtons(false)
	, useIcons(true)
	, myColor(0.2f, 0.9f, 0.2f, 1.0f)
	, allyColor(0.3f, 0.3f, 0.9f, 1.0f)
	, enemyColor(0.9f, 0.2f, 0.2f, 1.0f)
	, renderToTexture(true)
	, multisampledFBO(false)
	, minimapTex(0)
	, lastClicked(nullptr)
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
		const std::string geo = configHandler->GetString("MiniMapGeometry");
		ParseGeometry(geo);
	}

	fullProxy = configHandler->GetBool("MiniMapFullProxy");
	buttonSize = configHandler->GetInt("MiniMapButtonSize");

	unitBaseSize = configHandler->GetFloat("MiniMapUnitSize");
	unitExponent = configHandler->GetFloat("MiniMapUnitExp");

	cursorScale = configHandler->GetFloat("MiniMapCursorScale");
	useIcons = configHandler->GetBool("MiniMapIcons");
	drawCommands = configHandler->GetInt("MiniMapDrawCommands");
	drawProjectiles = configHandler->GetBool("MiniMapDrawProjectiles");
	simpleColors = configHandler->GetBool("SimpleMiniMapColors");
	minimapRefreshRate = configHandler->GetInt("MiniMapRefreshRate");
	renderToTexture = configHandler->GetBool("MiniMapRenderToTexture") && FBO::IsSupported();

	UpdateGeometry();

	circleLists = glGenLists(circleListsCount);
	for (int cl = 0; cl < circleListsCount; cl++) {
		glNewList(circleLists + cl, GL_COMPILE);
		glBegin(GL_LINE_LOOP);
		const int divs = (1 << (cl + 3));
		for (int d = 0; d < divs; d++) {
			const float rads = float(2.0 * PI) * float(d) / float(divs);
			glVertex3f(std::sin(rads), 0.0f, std::cos(rads));
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
								 GL_RGBA, GL_UNSIGNED_BYTE, &bitmap.mem[0]);
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
	glDeleteTextures(1, &buttonsTexture);
	glDeleteTextures(1, &minimapTex);
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
		const float hw = math::sqrt(float(mapDims.mapx) / float(mapDims.mapy));
		width = (int)(-width * hw);
		height = (int)(-height / hw);
	}
	else if (width <= 0) {
		width = (int)(float(height) * float(mapDims.mapx) / float(mapDims.mapy));
	}
	else if (height <= 0) {
		height = (int)(float(width) * float(mapDims.mapy) / float(mapDims.mapx));
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
		const float mapRatio = (float)mapDims.mapx / (float)mapDims.mapy;
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
	if (newMode != slaveDrawMode) {
		static int oldButtonSize = 16;
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
		fullProxy = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !fullProxy;
	}
	else if (command == "icons") {
		useIcons = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !useIcons;
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
		drawProjectiles = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !drawProjectiles;
	}
	else if (command == "simplecolors") {
		simpleColors = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !simpleColors;
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
		minimized = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !minimized;
	}
	else if ((command == "max") ||
	         (command == "maximize") || (command == "maxspect")) {
		const bool newMax = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !maximized;
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
	const float mapx = float(mapDims.mapx * SQUARE_SIZE);
	const float mapy = float(mapDims.mapy * SQUARE_SIZE);
	const float ref  = unitBaseSize / math::pow((200.f * 200.0f), unitExponent);
	const float dpr  = ref * math::pow((w * h), unitExponent);

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
	float3 clickPos;
	clickPos.x = ((float(x -                               xpos          )) /  width) * (mapDims.mapx * SQUARE_SIZE);
	clickPos.z = ((float(y - (globalRendering->viewSizeY - ypos - height))) / height) * (mapDims.mapy * SQUARE_SIZE);
	camHandler->CameraTransition(0.0f);
	camHandler->GetCurrentController().SetPos(clickPos);
	unitTracker.Disable();
}


void CMiniMap::SelectUnits(int x, int y)
{
	const CUnit *_lastClicked = lastClicked;
	lastClicked = nullptr;
	if (!KeyInput::GetKeyModState(KMOD_SHIFT) && !KeyInput::GetKeyModState(KMOD_CTRL)) {
		selectedUnitsHandler.ClearSelected();
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

		const float4  planeRight(-RgtVector,  xmin);
		const float4   planeLeft( RgtVector, -xmax);
		const float4    planeTop( FwdVector, -zmax);
		const float4 planeBottom(-FwdVector,  zmin);

		selectedUnitsHandler.HandleUnitBoxSelection(planeRight, planeLeft, planeTop, planeBottom);
	}
	else {
		// Single unit
		const float size = unitSelectRadius;
		const float3 pos = GetMapPosition(x, y);

		CUnit* unit;
		if (gu->spectatingFullSelect) {
			unit = CGameHelper::GetClosestUnit(pos, size);
		} else {
			unit = CGameHelper::GetClosestFriendlyUnit(NULL, pos, size, gu->myAllyTeam);
		}
		lastClicked = unit;
		const bool selectType = bp.lastRelease >= (gu->gameTime - mouse->doubleClickTime) && unit == _lastClicked;

		selectedUnitsHandler.HandleSingleUnitClickSelection(unit, false, selectType);
	}

	bp.lastRelease = gu->gameTime;
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
		if (KeyInput::GetKeyModState(KMOD_SHIFT)) {
			width = (height * mapDims.mapx) / mapDims.mapy;
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
			ToggleMaximized(!!KeyInput::GetKeyModState(KMOD_SHIFT));
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
	CUnit* unit = CGameHelper::GetClosestUnit(pos, unitSelectRadius);
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
	const float mapHeight = readMap->GetInitMaxHeight() + 1000.0f;
	const float mapX = mapDims.mapx * SQUARE_SIZE;
	const float mapY = mapDims.mapy * SQUARE_SIZE;
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
			mapPos = unit->GetObjDrawErrorPos(gu->myAllyTeam);
			mapPos.y = readMap->GetInitMaxHeight() + 1000.0f;
		}
	}

	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	bp.camPos = mapPos;
	bp.dir = -UpVector;

	guihandler->MousePress(x, y, -button);
}


void CMiniMap::ProxyMouseRelease(int x, int y, int button)
{
	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);
	if (unit) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = unit->GetObjDrawErrorPos(gu->myAllyTeam);
			mapPos.y = readMap->GetInitMaxHeight() + 1000.0f;
		}
	}

	float3 mousedir = -UpVector;
	float3 campos = mapPos;

	guihandler->MouseRelease(x, y, -button, campos, mousedir);
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
	if (minimized)
		return "Unminimize map";

	if (buttonBox.Inside(x, y)) {
		if (resizeBox.Inside(x, y))
			return "Resize map\n(SHIFT to maintain aspect ratio)";

		if (moveBox.Inside(x, y))
			return "Move map";

		if (maximizeBox.Inside(x, y)) {
			if (!maximized) {
				return "Maximize map\n(SHIFT to maintain aspect ratio)";
			} else {
				return "Unmaximize map";
			}
		}

		if (minimizeBox.Inside(x, y))
			return "Minimize map";
	}

	const string buildTip = std::move(guihandler->GetBuildTooltip());
	if (!buildTip.empty())
		return buildTip;

	const CUnit* unit = GetSelectUnit(GetMapPosition(x, y));
	if (unit != nullptr)
		return CTooltipConsole::MakeUnitString(unit);

	const string selTip = std::move(selectedUnitsHandler.GetTooltip());
	if (selTip != "")
		return selTip;

	float3 wpos;
	wpos.x = float(x                               - xpos          ) / width  * mapDims.mapx * SQUARE_SIZE;
	wpos.z = float(y - (globalRendering->viewSizeY - ypos - height)) / height * mapDims.mapx * SQUARE_SIZE;
	wpos.y = CGround::GetHeightReal(wpos.x, wpos.z, false);

	return CTooltipConsole::MakeGroundString(wpos);
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

void CMiniMap::DrawCircle(const float3& pos, float radius) const
{
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(radius, 1.0f, radius);

	const float xPixels = radius * float(width) / float(mapDims.mapx * SQUARE_SIZE);
	const float yPixels = radius * float(height) / float(mapDims.mapy * SQUARE_SIZE);
	const int lod = (int)(0.25 * math::log2(1.0f + (xPixels * yPixels)));
	const int lodClamp = std::max(0, std::min(circleListsCount - 1, lod));
	glCallList(circleLists + lodClamp);

	glPopMatrix();
}

void CMiniMap::DrawSquare(const float3& pos, float xsize, float zsize) const
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


void CMiniMap::ApplyConstraintsMatrix() const
{
	if (!renderToTexture) {
		glTranslatef(xpos * globalRendering->pixelX, ypos * globalRendering->pixelY, 0.0f);
		glScalef(width * globalRendering->pixelX, height * globalRendering->pixelY, 1.0f);
	}
}


/******************************************************************************/

void CMiniMap::Update()
{
	SCOPED_GMARKER("CMiniMap::Update");

	if (minimized || width == 0 || height == 0)
		return;

	if (renderToTexture) {
		static spring_time nextDrawScreen = spring_gettime();
		if (spring_gettime() > nextDrawScreen) {
			float refreshRate = minimapRefreshRate;
			if (minimapRefreshRate == 0) {
				const float screenArea = (width * height) / (globalRendering->viewSizeX * globalRendering->viewSizeY);
				refreshRate = (screenArea >= 0.45f) ? 60 : (screenArea > 0.15f) ? 25 : 15;
			}
			nextDrawScreen = spring_gettime() + spring_msecs(1000.0f / refreshRate);

			fbo.Bind();
			if (minimapTexSize != int2(width, height))
				ResizeTextureCache();

			fbo.Bind();
			UpdateTextureCache();

			// no need, gets done in CGame
			//fbo.Unbind();
		}
	}
}


void CMiniMap::ResizeTextureCache()
{
	minimapTexSize = int2(width, height);
	multisampledFBO = (fbo.GetMaxSamples() > 1);

	if (multisampledFBO) {
		// multisampled FBO we are render to
		fbo.Detach(GL_COLOR_ATTACHMENT0_EXT); // delete old RBO
		fbo.CreateRenderBufferMultisample(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, minimapTexSize.x, minimapTexSize.y, 4);
		//fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, minimapTexSize.x, minimapTexSize.y);

		if (!fbo.CheckStatus("MINIMAP")) {
			fbo.Detach(GL_COLOR_ATTACHMENT0_EXT);
			multisampledFBO = false;
		}
	}

	glDeleteTextures(1, &minimapTex);
	glGenTextures(1, &minimapTex);

	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, minimapTexSize.x, minimapTexSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	if (multisampledFBO) {
		// resolve FBO with attached final texture target
		fboResolve.Bind();
		fboResolve.AttachTexture(minimapTex);

		if (!fboResolve.CheckStatus("MINIMAP-RESOLVE")) {
			renderToTexture = false;
			return;
		}
	} else {
		// directly render to texture without multisampling (fallback solution)
		fbo.Bind();
		fbo.AttachTexture(minimapTex);

		if (!fbo.CheckStatus("MINIMAP-RESOLVE")) {
			renderToTexture = false;
			return;
		}
	}
}


void CMiniMap::UpdateTextureCache()
{
	// draws minimap into FBO
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	const int2 oldPos(xpos, ypos);
	xpos = 0.0f;
	ypos = 0.0f;

		glViewport(0, 0, minimapTexSize.x, minimapTexSize.y);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		DrawForReal(false, true);

	xpos = oldPos.x;
	ypos = oldPos.y;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// resolve multisampled FBO if there is one
	if (multisampledFBO) {
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER, fbo.fboId);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, fboResolve.fboId);
		glBlitFramebufferEXT(
			0, 0, minimapTexSize.x, minimapTexSize.y,
			0, 0, minimapTexSize.x, minimapTexSize.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
}


/******************************************************************************/

void CMiniMap::Draw()
{
	if (slaveDrawMode)
		return;

	// Draw Border
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPushAttrib(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_MODELVIEW);

		if (minimized) {
			DrawMinimizedButton();
			glPopAttrib();
			glEnable(GL_TEXTURE_2D);
			return;
		}

		// draw the frameborder
		if (!globalRendering->dualScreenMode && !maximized) {
			DrawFrame();
		}

		glPopAttrib();
	}

	// Draw Minimap itself
	DrawForReal(true);
}


void CMiniMap::DrawForReal(bool use_geo, bool updateTex)
{
	SCOPED_TIMER("MiniMap::DrawForReal");

	if (minimized)
		return;

	glActiveTexture(GL_TEXTURE0);

	if ((!updateTex) && RenderCachedTexture(use_geo))
		return;

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);

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
	}

	setSurfaceCircleFunc(DrawSurfaceCircle);
	setSurfaceSquareFunc(DrawSurfaceSquare);
	cursorIcons.Enable(false);

	// clip everything outside of the minimap box
	SetClipPlanes(false);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);

	DrawBackground();

	// allow the LUA scripts to overdraw the background image
	SetClipPlanes(true);
	eventHandler.DrawInMiniMapBackground();
	SetClipPlanes(false);

	DrawUnitIcons();
	DrawWorldStuff();

	if (use_geo) {
		glPopMatrix();
	}
	glPopAttrib();
	glEnable(GL_TEXTURE_2D);

	// allow the LUA scripts to draw into the minimap
	SetClipPlanes(true);
	eventHandler.DrawInMiniMap();

	if (!updateTex) {
		glPushMatrix();
			glTranslatef(xpos * globalRendering->pixelX, ypos * globalRendering->pixelY, 0.0f);
			glScalef(width * globalRendering->pixelX, height * globalRendering->pixelY, 1.0f);
			DrawCameraFrustumAndMouseSelection();
		glPopMatrix();
	}

	// Finish
	// Reset of GL state
	if (use_geo && globalRendering->dualScreenMode)
		glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);

	// disable ClipPlanes
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);

	cursorIcons.Enable(true);
	setSurfaceCircleFunc(NULL);
	setSurfaceSquareFunc(NULL);
}


/******************************************************************************/
/******************************************************************************/

void CMiniMap::DrawCameraFrustumAndMouseSelection()
{
	glDisable(GL_TEXTURE_2D);

	// clip everything outside of the minimap box
	SetClipPlanes(false);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);

	// switch to top-down map/world coords (z is twisted with y compared to the real map/world coords)
	glPushMatrix();
	glTranslatef(0.0f, +1.0f, 0.0f);
	glScalef(+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);

	if (!minimap->maximized) {
		// draw the camera frustum lines
		// CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_SHADOW);
		CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_PLAYER);

		cam->GetFrustumSides(0.0f, 0.0f, 1.0f, true);
		cam->ClipFrustumLines(true, -100.0f, mapDims.mapy * SQUARE_SIZE + 100.0f);

		const std::vector<CCamera::FrustumLine>& negSides = cam->GetNegFrustumSides();

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(negSides.size() * 2, 0, VA_SIZE_2D0);

		for (auto& fl: negSides) {
			if (fl.minz < fl.maxz) {
				va->AddVertexQ2d0((fl.dir * fl.minz) + fl.base, fl.minz);
				va->AddVertexQ2d0((fl.dir * fl.maxz) + fl.base, fl.maxz);
			}
		}

		glLineWidth(2.5f);
		glColor4f(0, 0, 0, 0.5f);
		va->DrawArray2d0(GL_LINES);

		glLineWidth(1.5f);
		glColor4f(1, 1, 1, 0.75f);
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
		//glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		//            (GLenum)cmdColors.MouseBoxBlendDst());
		glLineWidth(cmdColors.MouseBoxLineWidth());

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(4, 0, VA_SIZE_2D0);
			va->AddVertexQ2d0(oldPos.x, oldPos.z);
			va->AddVertexQ2d0(newPos.x, oldPos.z);
			va->AddVertexQ2d0(newPos.x, newPos.z);
			va->AddVertexQ2d0(oldPos.x, newPos.z);
		va->DrawArray2d0(GL_LINE_LOOP);

		glLineWidth(1.0f);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	DrawNotes();

	// disable ClipPlanes
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);

	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
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
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
		glRectf(xmin, ymin, xmax, ymax);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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


void CMiniMap::DrawNotes()
{
	if (notes.empty()) {
		return;
	}

	const float baseSize = mapDims.mapx * SQUARE_SIZE;
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	std::deque<Notification>::iterator ni = notes.begin();
	while (ni != notes.end()) {
		const float age = gu->gameTime - ni->creationTime;
		if (age > 2) {
			ni = notes.erase(ni);
			continue;
		}

		SColor color(ni->color[0], ni->color[1], ni->color[2], ni->color[3]);
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
			color.a = (255 * ni->color[3]) / (3 - a);
			const float sinSize = fastmath::sin(rot) * size;
			const float cosSize = fastmath::cos(rot) * size;
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



bool CMiniMap::RenderCachedTexture(bool use_geo)
{
	if (!renderToTexture) {
		return false;
	}

	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	if (use_geo) {
		glPushMatrix();
		glTranslatef(xpos * globalRendering->pixelX, ypos * globalRendering->pixelY, 0.0f);
		glScalef(width * globalRendering->pixelX, height * globalRendering->pixelY, 1.0f);
	}

	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0, 0.0); glVertex2f(1.0f, 0.0f);
		glTexCoord2f(1.0, 1.0); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0); glVertex2f(0.0f, 1.0f);
	glEnd();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawCameraFrustumAndMouseSelection();

	if (use_geo)
		glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	glPopAttrib();
	return true;
}


void CMiniMap::DrawBackground() const
{
	glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
	//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// don't mirror the map texture with flipped cameras
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

		// draw the map
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
			readMap->DrawMinimap();
		glEnable(GL_BLEND);

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


void CMiniMap::DrawUnitIcons() const
{
	// switch to top-down map/world coords (z is twisted with y compared to the real map/world coords)
	glPushMatrix();
	glTranslatef(0.0f, +1.0f, 0.0f);
	glScalef(+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);

	unitDrawer->DrawUnitMiniMapIcons();

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}


void CMiniMap::DrawUnitRanges() const
{
	// draw unit ranges
	CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
	for(const CUnit* unit: selUnits) {
		// LOS Ranges
		if (unit->radarRadius && !unit->beingBuilt && unit->activated) {
			glColor3fv(cmdColors.rangeRadar);
			DrawCircle(unit->pos, unit->radarRadius);
		}
		if (unit->sonarRadius && !unit->beingBuilt && unit->activated) {
			glColor3fv(cmdColors.rangeSonar);
			DrawCircle(unit->pos, unit->sonarRadius);
		}
		if (unit->jammerRadius && !unit->beingBuilt && unit->activated) {
			glColor3fv(cmdColors.rangeJammer);
			DrawCircle(unit->pos, unit->jammerRadius);
		}

		// Interceptor Ranges
		for (const CWeapon* w: unit->weapons) {
			auto& wd = *w->weaponDef;
			if ((w->range > 300) && wd.interceptor) {
				if (w->numStockpiled || !wd.stockpile) {
					glColor3fv(cmdColors.rangeInterceptorOn);
				} else {
					glColor3fv(cmdColors.rangeInterceptorOff);
				}
				DrawCircle(unit->pos, wd.coverageRange);
			}
		}
	}
}


void CMiniMap::DrawWorldStuff() const
{
	glPushMatrix();
	glTranslatef(0.0f, +1.0f, 0.0f);
	glScalef(+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f);
	glRotatef(-90.0f, +1.0f, 0.0f, 0.0f); // real 'world' coordinates
	glScalef(1.0f, 0.0f, 1.0f); // skip the y-coord (Lua's DrawScreen is perspective and so any z-coord in it influence the x&y, too)

	// draw the projectiles
	if (drawProjectiles) {
		glPointSize(1.0f);
		WorkaroundATIPointSizeBug();
		projectileDrawer->DrawProjectilesMiniMap();
	}

	{
		// draw the queued commands
		commandDrawer->DrawLuaQueuedUnitSetCommands();

		// NOTE: this needlessly adds to the CursorIcons list, but at least
		//       they are not drawn  (because the input receivers are drawn
		//       after the command queues)
		if ((drawCommands > 0) && guihandler->GetQueueKeystate()) {
			selectedUnitsHandler.DrawCommands();
		}
	}


	glLineWidth(2.5f);
	lineDrawer.DrawAll();
	glLineWidth(1.0f);

	// draw the selection shape, and some ranges
	if (drawCommands > 0) {
		guihandler->DrawMapStuff(!!drawCommands);
	}

	DrawUnitRanges();

	glPopMatrix();
}


void CMiniMap::SetClipPlanes(const bool lua) const
{
	if (lua) {
		// prepare ClipPlanes for Lua's DrawInMinimap Modelview matrix

		// quote from glClipPlane spec:
		// "When glClipPlane is called, equation is transformed by the inverse of the modelview matrix and stored in the resulting eye coordinates.
		//  Subsequent changes to the modelview matrix have no effect on the stored plane-equation components."
		// -> we have to use the same modelview matrix when calling glClipPlane and later draw calls

		// set the modelview matrix to the same as used in Lua's DrawInMinimap
		glPushMatrix();
		glLoadIdentity();
		glScalef(1.0f / width, 1.0f / height, 1.0f);

		const double plane0[4] = {0, -1, 0, double(height)};
		const double plane1[4] = {0, 1, 0, 0};
		const double plane2[4] = {-1, 0, 0, double(width)};
		const double plane3[4] = {1, 0, 0, 0};

		glClipPlane(GL_CLIP_PLANE0, plane0); // clip bottom
		glClipPlane(GL_CLIP_PLANE1, plane1); // clip top
		glClipPlane(GL_CLIP_PLANE2, plane2); // clip right
		glClipPlane(GL_CLIP_PLANE3, plane3); // clip left

		glPopMatrix();
	} else {
		// clip everything outside of the minimap box
		const double plane0[4] = {0,-1,0,1};
		const double plane1[4] = {0,1,0,0};
		const double plane2[4] = {-1,0,0,1};
		const double plane3[4] = {1,0,0,0};

		glClipPlane(GL_CLIP_PLANE0, plane0); // clip bottom
		glClipPlane(GL_CLIP_PLANE1, plane1); // clip top
		glClipPlane(GL_CLIP_PLANE2, plane2); // clip right
		glClipPlane(GL_CLIP_PLANE3, plane3); // clip left
	}
}



/******************************************************************************/
