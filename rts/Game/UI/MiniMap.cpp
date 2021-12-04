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
#include "Game/UI/UnitTracker.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/SimpleParser.h"

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

CONFIG(int, MiniMapRefreshRate).defaultValue(0).minimumValue(0).description("Refresh rate of asynchronous MiniMap texture updates. Value of \"0\" autoselects between 10-60FPS.");



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CMiniMap* minimap = nullptr;

CMiniMap::CMiniMap(): CInputReceiver(BACK)
{
	lastWindowSizeX = globalRendering->viewSizeX;
	lastWindowSizeY = globalRendering->viewSizeY;

	if (globalRendering->dualScreenMode) {
		curDim.x = globalRendering->viewSizeX;
		curDim.y = globalRendering->viewSizeY;
		curPos.x = (globalRendering->viewSizeX - globalRendering->viewPosX);
		curPos.y = 0;
	} else {
		ParseGeometry(configHandler->GetString("MiniMapGeometry"));
	}

	{
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
	}

	UpdateGeometry();
	LoadBuffer();


	// setup the buttons' texture and texture coordinates
	CBitmap bitmap;
	bool unfiltered = false;

	if (bitmap.Load("bitmaps/minimapbuttons.png")) {
		glGenTextures(1, &buttonsTextureID);
		glBindTexture(GL_TEXTURE_2D, buttonsTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bitmap.xsize, bitmap.ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetRawMem());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		if ((unfiltered = (bitmap.ysize == buttonSize && bitmap.xsize == (buttonSize * 4)))) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		resizeBox.color = {1.0f, 1.0f, 1.0f, 1.0f};
		moveBox.color = {1.0f, 1.0f, 1.0f, 1.0f};
		maximizeBox.color = {1.0f, 1.0f, 1.0f, 1.0f};
		minimizeBox.color = {1.0f, 1.0f, 1.0f, 1.0f};
	} else {
		bitmap.AllocDummy({255, 255, 255, 255});
		buttonsTextureID = bitmap.CreateTexture();

		resizeBox.color = {0.1f, 0.1f, 0.8f, 0.8f}; // blue
		moveBox.color = {0.0f, 0.8f, 0.0f, 0.8f}; // green
		maximizeBox.color = {0.8f, 0.8f, 0.0f, 0.8f}; // yellow
		minimizeBox.color = {0.8f, 0.0f, 0.0f, 0.8f}; // red
	}

	{
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
}


CMiniMap::~CMiniMap()
{
	glDeleteTextures(1, &buttonsTextureID);
	glDeleteTextures(1, &minimapTextureID);

	miniMap.Kill();
}


void CMiniMap::LoadBuffer()
{
	const std::string& vsText = Shader::GetShaderSource("GLSL/MiniMapVertProg.glsl");
	const std::string& fsText = Shader::GetShaderSource("GLSL/MiniMapFragProg.glsl");

	const float isx = mapDims.mapx / float(mapDims.pwr2mapx);
	const float isy = mapDims.mapy / float(mapDims.pwr2mapy);

	typedef Shader::ShaderInput MiniMapAttrType;

	const std::array<MiniMapVertType, 6> verts = {{
		{{0.0f, 0.0f}, {0.0f, 1.0f}}, // tl
		{{0.0f, 1.0f}, {0.0f, 0.0f}}, // bl
		{{1.0f, 1.0f}, {1.0f, 0.0f}}, // br

		{{1.0f, 1.0f}, {1.0f, 0.0f}}, // br
		{{1.0f, 0.0f}, {1.0f, 1.0f}}, // tr
		{{0.0f, 0.0f}, {0.0f, 1.0f}}, // tl
	}};
	const std::array<MiniMapAttrType, 2> attrs = {{
		{0,  2, GL_FLOAT,  (sizeof(float) * 4),  "a_vertex_pos", VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 4),  "a_tex_coords", VA_TYPE_OFFSET(float, 2)},
	}};

	miniMap.Init(false);
	miniMap.TUpload<MiniMapVertType, uint32_t, MiniMapAttrType>(verts.size(), 0, attrs.size(),  verts.data(), nullptr, attrs.data()); // no indices


	Shader::GLSLShaderObject shaderObjs[2] = {{GL_VERTEX_SHADER, vsText, ""}, {GL_FRAGMENT_SHADER, fsText, ""}};
	Shader::IProgramObject* shaderProg = miniMap.CreateShader((sizeof(shaderObjs) / sizeof(shaderObjs[0])), 0, &shaderObjs[0], nullptr);

	shaderProg->Enable();
	shaderProg->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shaderProg->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);
	shaderProg->SetUniform("u_shading_tex", 0);
	shaderProg->SetUniform("u_minimap_tex", 1);
	shaderProg->SetUniform("u_infomap_tex", 2);
	shaderProg->SetUniform("u_texcoor_mul", isx, isy);
	shaderProg->SetUniform("u_infotex_mul", 0.0f);
	shaderProg->SetUniform("u_gamma_expon", 1.0f);
	shaderProg->Disable();
}

void CMiniMap::ParseGeometry(const string& geostr)
{
	const char* geoStr = geostr.c_str();
	const char* geoDef = "2 2 200 200";

	if ((sscanf(geoStr, "%i %i %i %i", &curPos.x, &curPos.y, &curDim.x, &curDim.y) == 4) && (strcmp(geoStr, geoDef) == 0)) {
		// default geometry
		curDim.x = -200;
		curDim.y = -200;
	} else {
		if (curDim.x <= 0) { curDim.x = -200; }
		if (curDim.y <= 0) { curDim.y = -200; }
	}

	if ((curDim.x <= 0) && (curDim.y <= 0)) {
		const float hw = math::sqrt(mapDims.mapx * 1.0f / mapDims.mapy);
		curDim.x = (-curDim.x * hw);
		curDim.y = (-curDim.y / hw);
	}
	else if (curDim.x <= 0) {
		curDim.x = curDim.y * (mapDims.mapx * 1.0f / mapDims.mapy);
	}
	else if (curDim.y <= 0) {
		curDim.y = curDim.x * (mapDims.mapy * 1.0f / mapDims.mapx);
	}

	// convert to GL coords (MM origin in bottom-left corner)
	curPos.y = globalRendering->viewSizeY - curDim.y - curPos.y;
}


void CMiniMap::ToggleMaximized(bool _maxspect)
{
	if ((maximized = !maximized)) {
		// stash current geometry
		oldPos = curPos;
		oldDim = curDim;
		maxspect = _maxspect;
	} else {
		// restore previous geometry
		curPos = oldPos;
		curDim = oldDim;
	}

	// needed for SetMaximizedGeometry
	UpdateGeometry();
}


void CMiniMap::SetMaximizedGeometry()
{
	if (!maxspect) {
		curDim.y = globalRendering->viewSizeY;
		curDim.x = curDim.y;
		curPos.x = (globalRendering->viewSizeX - globalRendering->viewSizeY) / 2;
		curPos.y = 0;
		return;
	}

	const float  mapRatio = (float)mapDims.mapx / (float)mapDims.mapy;
	const float viewRatio = globalRendering->aspectRatio;

	if (mapRatio > viewRatio) {
		curPos.x = 0;
		curDim.x = globalRendering->viewSizeX;
		curDim.y = globalRendering->viewSizeX / mapRatio;
		curPos.y = (globalRendering->viewSizeY - curDim.y) / 2;
	} else {
		curPos.y = 0;
		curDim.y = globalRendering->viewSizeY;
		curDim.x = globalRendering->viewSizeY * mapRatio;
		curPos.x = (globalRendering->viewSizeX - curDim.x) / 2;
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
	const std::vector<std::string>& words = CSimpleParser::Tokenize(line, 1);
	if (words.empty())
		return;

	switch (hashStringLower(words[0].c_str())) {
		case hashString("fullproxy"): {
			fullProxy = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !fullProxy;
		} break;
		case hashString("icons"): {
			useIcons = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !useIcons;
		} break;
		case hashString("unitexp"): {
			if (words.size() >= 2)
				unitExponent = atof(words[1].c_str());

			UpdateGeometry();
		} break;
		case hashString("unitsize"): {
			if (words.size() >= 2)
				unitBaseSize = atof(words[1].c_str());

			unitBaseSize = std::max(0.0f, unitBaseSize);
			UpdateGeometry();
		} break;
		case hashString("drawcommands"): {
			if (words.size() >= 2) {
				drawCommands = std::max(0, atoi(words[1].c_str()));
			} else {
				drawCommands = (drawCommands > 0) ? 0 : 1;
			}
		} break;
		case hashString("drawprojectiles"): {
			drawProjectiles = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !drawProjectiles;
		} break;
		case hashString("simplecolors"): {
			simpleColors = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !simpleColors;
		} break;

		// the following commands can not be used in dualscreen mode
		case hashString("geo"):
		case hashString("geometry"): {
			if (globalRendering->dualScreenMode)
				return;
			if (words.size() < 2)
				return;

			ParseGeometry(words[1]);
			UpdateGeometry();
		} break;

		case hashString("min"):
		case hashString("minimize"): {
			if (globalRendering->dualScreenMode)
				return;

			minimized = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !minimized;
		} break;

		case hashString("max"):
		case hashString("maximize"):
		case hashString("maxspect"): {
			if (globalRendering->dualScreenMode)
				return;

			const bool   isMaximized = maximized;
			const bool wantMaximized = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !isMaximized;

			if (isMaximized != wantMaximized)
				ToggleMaximized(StrCaseStr(words[0].c_str(), "maxspect") == 0);
		} break;

		case hashString("mouseevents"): {
			mouseEvents = (words.size() >= 2) ? !!atoi(words[1].c_str()) : !mouseEvents;
		} break;

		default: {
		} break;
	}
}

/******************************************************************************/

void CMiniMap::SetGeometry(int px, int py, int sx, int sy)
{
	curPos = {px, py};
	curDim = {sx, sy};

	UpdateGeometry();
}


void CMiniMap::UpdateGeometry()
{
	// keep the same distance to the top
	curPos.y -= (lastWindowSizeY - globalRendering->viewSizeY);

	if (globalRendering->dualScreenMode) {
		curDim.x = globalRendering->viewSizeX;
		curDim.y = globalRendering->viewSizeY;
		curPos.x = (globalRendering->viewSizeX - globalRendering->viewPosX);
		curPos.y = 0;
	}
	else if (maximized) {
		SetMaximizedGeometry();
	}
	else {
		curDim.x = Clamp(curDim.x, 1, globalRendering->viewSizeX);
		curDim.y = Clamp(curDim.y, 1, globalRendering->viewSizeY);

		curPos.y = std::max(slaveDrawMode ? 0 : buttonSize, curPos.y);
		curPos.y = std::min(globalRendering->viewSizeY - curDim.y, curPos.y);
		curPos.x = Clamp(curPos.x, 0, globalRendering->viewSizeX - curDim.x);
	}


	{
		// Draw{WorldStuff} transform
		viewMats[0].LoadIdentity();
		viewMats[0].Translate(UpVector);
		viewMats[0].Scale({+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f});
		viewMats[0].RotateX(90.0f * math::DEG_TO_RAD); // rotate to match real 'world' coordinates
		viewMats[0].Scale(XZVector + UpVector * 0.0001f); // (invertibly) flatten; LuaOpenGL::DrawScreen uses persp-proj so z-values influence x&y

		viewMats[1].LoadIdentity();
		viewMats[1].Translate(UpVector);
		// heightmap (squares) to minimap
		viewMats[1].Scale({1.0f / mapDims.mapx, -1.0f / mapDims.mapy, 1.0f});
		// worldmap (elmos) to minimap
		// viewMats[1].Scale({1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f});

		viewMats[2].LoadIdentity();
		viewMats[2].Scale({1.0f / curDim.x, 1.0f / curDim.y, 1.0f});

		projMats[0] = CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f);
		projMats[1] = CMatrix44f::ClipOrthoProj(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, globalRendering->supportClipSpaceControl * 1.0f);
		projMats[2] = projMats[1];
	}


	lastWindowSizeX = globalRendering->viewSizeX;
	lastWindowSizeY = globalRendering->viewSizeY;

	// setup the unit scaling
	const float w = float(curDim.x);
	const float h = float(curDim.y);
	const float mapx = float(mapDims.mapx * SQUARE_SIZE);
	const float mapy = float(mapDims.mapy * SQUARE_SIZE);
	const float ref  = unitBaseSize / math::pow((200.0f * 200.0f), unitExponent);
	const float dpr  = ref * math::pow((w * h), unitExponent);

	unitSizeX = dpr * (mapx / w);
	unitSizeY = dpr * (mapy / h);
	unitSelectRadius = fastmath::apxsqrt(unitSizeX * unitSizeY);

	// in mouse coordinates
	mapBox.xmin = curPos.x;
	mapBox.xmax = mapBox.xmin + curDim.x - 1;
	mapBox.ymin = globalRendering->viewSizeY - (curPos.y + curDim.y);
	mapBox.ymax = mapBox.ymin + curDim.y - 1;

	// FIXME:
	//   also need to make sure we can leave maximized-mode when !maxspect (in
	//   which case the buttons should be drawn on top of map, not outside it)
	if (!maximized || maxspect) {
		// work right (resizeBox) to left (minimizeBox)
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
		// work left to right
		minimizeBox.xmin = mapBox.xmin;
		minimizeBox.xmax = minimizeBox.xmin + (buttonSize - 1);

		maximizeBox.xmin = minimizeBox.xmin + buttonSize;
		maximizeBox.xmax = minimizeBox.xmax + buttonSize;

		// dead buttons
		resizeBox.xmin = resizeBox.ymin = moveBox.xmin = moveBox.ymin =  0;
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
	camHandler->CameraTransition(0.0f);
	camHandler->GetCurrentController().SetPos(GetMapPosition(x, y) * XZVector);
	unitTracker.Disable();
}


void CMiniMap::SelectUnits(int x, int y)
{
	const CUnit* _lastClicked = lastClicked;
	lastClicked = nullptr;

	if (!KeyInput::GetKeyModState(KMOD_SHIFT) && !KeyInput::GetKeyModState(KMOD_CTRL))
		selectedUnitsHandler.ClearSelected();

	CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];

	if (fullProxy && (bp.movement > mouse->dragSelectionThreshold)) {
		// use a selection box
		const float3 newMapPos = GetMapPosition(x, y);
		const float3 oldMapPos = GetMapPosition(bp.x, bp.y);

		const float xmin = std::min(oldMapPos.x, newMapPos.x);
		const float xmax = std::max(oldMapPos.x, newMapPos.x);
		const float zmin = std::min(oldMapPos.z, newMapPos.z);
		const float zmax = std::max(oldMapPos.z, newMapPos.z);

		const float4  planeRight(-RgtVector,  xmin);
		const float4   planeLeft( RgtVector, -xmax);
		const float4    planeTop( FwdVector, -zmax);
		const float4 planeBottom(-FwdVector,  zmin);

		selectedUnitsHandler.HandleUnitBoxSelection(planeRight, planeLeft, planeTop, planeBottom);
	} else {
		// Single unit
		const float3 pos = GetMapPosition(x, y);

		CUnit* unit;
		if (gu->spectatingFullSelect) {
			unit = CGameHelper::GetClosestUnit(pos, unitSelectRadius);
		} else {
			unit = CGameHelper::GetClosestFriendlyUnit(nullptr, pos, unitSelectRadius, gu->myAllyTeam);
		}

		selectedUnitsHandler.HandleSingleUnitClickSelection(lastClicked = unit, false, bp.lastRelease >= (gu->gameTime - mouse->doubleClickTime) && unit == _lastClicked);
	}

	bp.lastRelease = gu->gameTime;
}

/******************************************************************************/

bool CMiniMap::MousePress(int x, int y, int button)
{
	if (!mouseEvents)
		return false;

	if (minimized) {
		if ((x < buttonSize) && (y < buttonSize)) {
			minimized = false;
			return true;
		}

		return false;
	}

	const bool inMap = mapBox.Inside(x, y);
	const bool inButtons = buttonBox.Inside(x, y);

	if (!inMap && !inButtons)
		return false;

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
			if (resizeBox.Inside(x, y)) {
				mouseResize = true;
				return true;
			}

			if (minimizeBox.Inside(x, y) || maximizeBox.Inside(x, y))
				return true;
		}
		if (inMap && !mouse->buttons[SDL_BUTTON_LEFT].chorded) {
			selecting = true;
			return true;
		}
	}
	else if (inMap) {
		if ((fullProxy && (button == SDL_BUTTON_MIDDLE)) || (!fullProxy && (button == SDL_BUTTON_RIGHT))) {
			MoveView(x, y);

			if (maximized) {
				ToggleMaximized(false);
			} else {
				mouseLook = true;
			}

			return true;
		}
		if (fullProxy && (button == SDL_BUTTON_RIGHT)) {
			proxyMode = true;
			ProxyMousePress(x, y, button);
			return true;
		}
	}

	return false;
}


void CMiniMap::MouseMove(int x, int y, int dx, int dy, int button)
{
	// if Press is not handled, should never get Move
	assert(mouseEvents);

	if (mouseMove) {
		curPos.x += dx;
		curPos.y -= dy;
		curPos.x  = std::max(0, std::min(globalRendering->viewSizeX * (1 + globalRendering->dualScreenMode) - curDim.x, curPos.x));
		curPos.y  = std::max(5, std::min(globalRendering->viewSizeY                                         - curDim.y, curPos.y));

		UpdateGeometry();
		return;
	}

	if (mouseResize) {
		curPos.y -= dy;
		curDim.x += dx;
		curDim.y += dy;
		curDim.x  = std::min(globalRendering->viewSizeX * (1 + globalRendering->dualScreenMode), curDim.x);
		curDim.y  = std::min(globalRendering->viewSizeY                                        , curDim.y);

		if (KeyInput::GetKeyModState(KMOD_SHIFT))
			curDim.x = (curDim.y * mapDims.mapx) / mapDims.mapy;

		curDim.x = std::max(5, curDim.x);
		curDim.y = std::max(5, curDim.y);
		curPos.y = std::min(globalRendering->viewSizeY - curDim.y, curPos.y);

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
	// if Press is not handled, should never get Release
	assert(mouseEvents);

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

	if (unit == nullptr)
		return unit;

	if ((unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) || gu->spectatingFullView)
		return unit;

	return nullptr;
}


float3 CMiniMap::GetMapPosition(int x, int y) const
{
	const float mapX = float3::maxxpos + 1.0f;
	const float mapZ = float3::maxzpos + 1.0f;

	// mouse coordinate-origin is top-left of screen while
	// the minimap origin is in its bottom-left corner, so
	//   (x =     0, y =     0) maps to world-coors (   0, h,    0)
	//   (x = dim.x, y =     0) maps to world-coors (mapX, h,    0)
	//   (x = dim.x, y = dim.y) maps to world-coors (mapX, h, mapZ)
	//   (x =     0, y = dim.y) maps to world-coors (   0, h, mapZ)
	const float sx = Clamp(float(x -                               tmpPos.x            ) / curDim.x, 0.0f, 1.0f);
	const float sz = Clamp(float(y - (globalRendering->viewSizeY - tmpPos.y - curDim.y)) / curDim.y, 0.0f, 1.0f);

	return {mapX * sx, readMap->GetCurrMaxHeight(), mapZ * sz};
}


void CMiniMap::ProxyMousePress(int x, int y, int button)
{
	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);

	if (unit != nullptr) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = unit->GetObjDrawErrorPos(gu->myAllyTeam);
			mapPos.y = readMap->GetCurrMaxHeight();
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

	if (unit != nullptr) {
		if (gu->spectatingFullView) {
			mapPos = unit->midPos;
		} else {
			mapPos = unit->GetObjDrawErrorPos(gu->myAllyTeam);
			mapPos.y = readMap->GetCurrMaxHeight();
		}
	}

	guihandler->MouseRelease(x, y, -button, mapPos, -UpVector);
}


/******************************************************************************/

bool CMiniMap::IsAbove(int x, int y)
{
	if (minimized)
		return ((x < buttonSize) && (y < buttonSize));

	if (mapBox.Inside(x, y))
		return true;

	if (showButtons && buttonBox.Inside(x, y))
		return true;

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
			if (!maximized)
				return "Maximize map\n(SHIFT to maintain aspect ratio)";

			return "Unmaximize map";
		}

		if (minimizeBox.Inside(x, y))
			return "Minimize map";
	}

	const std::string buildTip = std::move(guihandler->GetBuildTooltip());
	if (!buildTip.empty())
		return buildTip;

	const float3 wpos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(wpos);
	if (unit != nullptr)
		return (std::move(CTooltipConsole::MakeUnitString(unit)));

	const std::string selTip = std::move(selectedUnitsHandler.GetTooltip());
	if (!selTip.empty())
		return selTip;

	return (std::move(CTooltipConsole::MakeGroundString({wpos.x, CGround::GetHeightReal(wpos.x, wpos.z, false), wpos.z})));
}


void CMiniMap::AddNotification(float3 pos, float3 color, float alpha)
{
	Notification n;
	n.pos = pos;
	n.color = {color, alpha};
	n.creationTime = gu->gameTime;

	notes.push_back(n);
}


/******************************************************************************/

void CMiniMap::DrawCircle(GL::RenderDataBufferC* buffer, const float4& pos, const float4& color) const
{
	const float xzScale = pos.w;
	const float xPixels = xzScale * float(curDim.x) / float(mapDims.mapx * SQUARE_SIZE);
	const float yPixels = xzScale * float(curDim.y) / float(mapDims.mapy * SQUARE_SIZE);

	const int lod = (int)(0.25 * math::log2(1.0f + (xPixels * yPixels)));
	const int divs = 1 << (Clamp(lod, 0, 6 - 1) + 3);

	for (int d = 0; d < divs; d++) {
		const float step0 = (d    ) * 1.0f / divs;
		const float step1 = (d + 1) * 1.0f / divs;
		const float rads0 = math::TWOPI * step0;
		const float rads1 = math::TWOPI * step1;

		const float3 vtx0 = {std::sin(rads0), 0.0f, std::cos(rads0)};
		const float3 vtx1 = {std::sin(rads1), 0.0f, std::cos(rads1)};

		buffer->SafeAppend({pos + vtx0 * xzScale, SColor(&color.x)});
		buffer->SafeAppend({pos + vtx1 * xzScale, SColor(&color.x)});
	}
}

void CMiniMap::DrawSurfaceCircleFunc(GL::RenderDataBufferC* buffer, const float4& pos, const float4& color, unsigned int)
{
	minimap->DrawCircle(buffer, pos, color);
}


void CMiniMap::DrawCircleW(GL::WideLineAdapterC* wla, const float4& pos, const float4& color) const
{
	const float xzScale = pos.w;
	const float xPixels = xzScale * float(curDim.x) / float(mapDims.mapx * SQUARE_SIZE);
	const float yPixels = xzScale * float(curDim.y) / float(mapDims.mapy * SQUARE_SIZE);

	const int lod = (int)(0.25 * math::log2(1.0f + (xPixels * yPixels)));
	const int divs = 1 << (Clamp(lod, 0, 6 - 1) + 3);

	for (int d = 0; d < divs; d++) {
		const float step0 = (d    ) * 1.0f / divs;
		const float step1 = (d + 1) * 1.0f / divs;
		const float rads0 = math::TWOPI * step0;
		const float rads1 = math::TWOPI * step1;

		const float3 vtx0 = {std::sin(rads0), 0.0f, std::cos(rads0)};
		const float3 vtx1 = {std::sin(rads1), 0.0f, std::cos(rads1)};

		wla->SafeAppend({pos + vtx0 * xzScale, SColor(&color.x)});
		wla->SafeAppend({pos + vtx1 * xzScale, SColor(&color.x)});
	}
}

void CMiniMap::DrawSurfaceCircleWFunc(GL::WideLineAdapterC* wla, const float4& pos, const float4& color, unsigned int)
{
	minimap->DrawCircleW(wla, pos, color);
}




CMatrix44f CMiniMap::CalcNormalizedCoorMatrix(bool dualScreen, bool luaCall) const
{
	CMatrix44f mat;

	#if 0
	// "Lua{OpenGL::ResetMiniMapMatrices} uses 0..minimapSize{X,Y}" no longer applies
	if (luaCall)
		mat.Scale({globalRendering->viewSizeX * 1.0f, globalRendering->viewSizeY * 1.0f, 1.0f});
	#endif
	if (!dualScreen)
		mat.Translate(curPos.x * globalRendering->pixelX, curPos.y * globalRendering->pixelY, 0.0f);

	mat.Scale({curDim.x * globalRendering->pixelX, curDim.y * globalRendering->pixelY, 1.0f});
	return mat;
}

/******************************************************************************/

void CMiniMap::Update()
{
	// need this because UpdateTextureCache sets curPos={0,0}
	// (while calling DrawForReal, which can reach GetMapPos)
	tmpPos = curPos;

	if (minimized || curDim.x == 0 || curDim.y == 0)
		return;

	static spring_time nextDrawScreenTime = spring_gettime();

	if (spring_gettime() <= nextDrawScreenTime)
		return;

	float refreshRate = minimapRefreshRate;

	if (minimapRefreshRate == 0) {
		const float viewArea = globalRendering->viewSizeX * globalRendering->viewSizeY;
		const float mmapArea = (curDim.x * curDim.y) / viewArea;
		refreshRate = (mmapArea >= 0.45f) ? 60 : (mmapArea > 0.15f) ? 25 : 15;
	}
	nextDrawScreenTime = spring_gettime() + spring_msecs(1000.0f / refreshRate);

	fbo.Bind();
	ResizeTextureCache();
	fbo.Bind();
	UpdateTextureCache();

	// gets done in CGame
	// fbo.Unbind();
}


void CMiniMap::ResizeTextureCache()
{
	#ifdef HEADLESS
	return;
	#endif

	if (minimapTexSize == curDim)
		return;

	minimapTexSize = curDim;

	if ((multisampledFBO = (fbo.GetMaxSamples() > 1))) {
		fbo.Detach(GL_COLOR_ATTACHMENT0); // delete old RBO
		fbo.CreateRenderBufferMultisample(GL_COLOR_ATTACHMENT0, GL_RGBA8, minimapTexSize.x, minimapTexSize.y, 4);
		//fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT16, minimapTexSize.x, minimapTexSize.y);

		if (!(multisampledFBO = fbo.CheckStatus("MINIMAP")))
			fbo.Detach(GL_COLOR_ATTACHMENT0);
	}

	glDeleteTextures(1, &minimapTextureID);
	glGenTextures(1, &minimapTextureID);

	glBindTexture(GL_TEXTURE_2D, minimapTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, minimapTexSize.x, minimapTexSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	if (multisampledFBO) {
		// resolve FBO with attached final texture target
		fboResolve.Bind();
		fboResolve.AttachTexture(minimapTextureID);

		if (fboResolve.CheckStatus("MINIMAP-RESOLVE"))
			return;

		throw opengl_error("[MiniMap::ResizeTextureCache] bad multisample framebuffer");
	}

	// directly render to texture without multisampling (fallback solution)
	fbo.Bind();
	fbo.AttachTexture(minimapTextureID);

	if (fbo.CheckStatus("MINIMAP-RESOLVE"))
		return;

	throw opengl_error("[MiniMap::ResizeTextureCache] bad fallback framebuffer");
}


void CMiniMap::UpdateTextureCache()
{
	{
		curPos = {0, 0};

		// draw minimap into FBO
		glAttribStatePtr->ViewPort(0, 0, minimapTexSize.x, minimapTexSize.y);
		glAttribStatePtr->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);

		if (!minimized)
			DrawForReal();

		curPos = tmpPos;
	}

	// resolve multisampled FBO if there is one
	if (!multisampledFBO)
		return;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo.fboId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboResolve.fboId);
	glBlitFramebuffer(
		0, 0, minimapTexSize.x, minimapTexSize.y,
		0, 0, minimapTexSize.x, minimapTexSize.y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST
	);
}


/******************************************************************************/

void CMiniMap::Draw()
{
	if (slaveDrawMode)
		return;

	{
		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAttribStatePtr->PushDepthBufferBit();
		glAttribStatePtr->DisableDepthTest();
		glAttribStatePtr->DepthFunc(GL_LEQUAL);
		glAttribStatePtr->DisableDepthMask();

		if (minimized) {
			DrawMinimizedButtonQuad(GL::GetRenderBufferTC());
			DrawMinimizedButtonLoop(GL::GetRenderBufferC());
			glAttribStatePtr->PopBits();
			return;
		}

		// draw the frame border
		if (!globalRendering->dualScreenMode) {
			DrawFrame(GL::GetRenderBufferC());
			DrawButtons(GL::GetRenderBufferTC(), GL::GetRenderBufferC());
		}

		glAttribStatePtr->PopBits();
	}

	{
		// draw minimap itself
		RenderCachedTextureNormalized(false);
	}
}



// MiniMap::UpdateTextureCache -> DrawForReal()
void CMiniMap::DrawForReal()
{
	// reroute LuaOpenGL::DrawGroundCircle if it is called inside minimap
	SetDrawSurfaceCircleFuncs(DrawSurfaceCircleFunc, DrawSurfaceCircleWFunc);
	cursorIcons.Enable(false);



	glAttribStatePtr->PushDepthBufferBit();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->DepthFunc(GL_LEQUAL);
	glAttribStatePtr->DisableDepthMask();

	{
		// clip everything outside of the minimap box
		// no real need for this: most objects are on the map
		// most of the time, texture scissor handles the rest
		// SetClipPlanes(false);
		DrawBackground();
	}

	{
		// allow Lua scripts to overdraw the background image
		// minimap matrices are accessible via LuaOpenGLUtils
		// SetClipPlanes(true);
		eventHandler.DrawInMiniMapBackground();
		// SetClipPlanes(false);
	}

	DrawUnitIcons();
	DrawWorldStuff();

	glAttribStatePtr->PopBits();

	{
		// allow Lua scripts to draw into the minimap
		// SetClipPlanes(true);
		eventHandler.DrawInMiniMap();
	}

	cursorIcons.Enable(true);
	SetDrawSurfaceCircleFuncs(nullptr, nullptr);
}


/******************************************************************************/
/******************************************************************************/

void CMiniMap::DrawMinimizedButtonQuad(GL::RenderDataBufferTC* buffer)
{
	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	const float xmin = (globalRendering->viewPosX + 1             ) * px;
	const float xmax = (globalRendering->viewPosX + 1 + buttonSize) * px;
	const float ymin = 1.0f - (1 + buttonSize) * py;
	const float ymax = 1.0f - (1             ) * py;

	glBindTexture(GL_TEXTURE_2D, buttonsTextureID);

	Shader::IProgramObject* shader = buffer->GetShader();
	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);
	buffer->SafeAppend({{xmin, ymin, 0.0f}, minimizeBox.xminTx, minimizeBox.yminTx, {1.0f, 1.0f, 1.0f, 1.0f}}); // tl
	buffer->SafeAppend({{xmax, ymin, 0.0f}, minimizeBox.xmaxTx, minimizeBox.yminTx, {1.0f, 1.0f, 1.0f, 1.0f}}); // tr
	buffer->SafeAppend({{xmax, ymax, 0.0f}, minimizeBox.xmaxTx, minimizeBox.ymaxTx, {1.0f, 1.0f, 1.0f, 1.0f}}); // br

	buffer->SafeAppend({{xmax, ymax, 0.0f}, minimizeBox.xmaxTx, minimizeBox.ymaxTx, {1.0f, 1.0f, 1.0f, 1.0f}}); // br
	buffer->SafeAppend({{xmin, ymax, 0.0f}, minimizeBox.xminTx, minimizeBox.ymaxTx, {1.0f, 1.0f, 1.0f, 1.0f}}); // bl
	buffer->SafeAppend({{xmin, ymin, 0.0f}, minimizeBox.xminTx, minimizeBox.yminTx, {1.0f, 1.0f, 1.0f, 1.0f}}); // tl
	buffer->Submit(GL_TRIANGLES);
	shader->Disable();
}

void CMiniMap::DrawMinimizedButtonLoop(GL::RenderDataBufferC* buffer)
{
	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	const float xmin = (globalRendering->viewPosX + 1             ) * px;
	const float xmax = (globalRendering->viewPosX + 1 + buttonSize) * px;
	const float ymin = 1.0f - (1 + buttonSize) * py;
	const float ymax = 1.0f - (1             ) * py;

	Shader::IProgramObject* shader = buffer->GetShader();
	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);

	// highlight
	if (((mouse->lastx + 1) <= buttonSize) && ((mouse->lasty + 1) <= buttonSize)) {
		// glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);
		buffer->SafeAppend({{xmin, ymin, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}}); // tl
		buffer->SafeAppend({{xmax, ymin, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}}); // tr
		buffer->SafeAppend({{xmax, ymax, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}}); // br

		buffer->SafeAppend({{xmax, ymax, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}}); // br
		buffer->SafeAppend({{xmin, ymax, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}}); // bl
		buffer->SafeAppend({{xmin, ymin, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}}); // tl
		buffer->Submit(GL_TRIANGLES);
		// glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// outline
	buffer->SafeAppend({{xmin - 0.5f * px, ymax + 0.5f * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->SafeAppend({{xmin - 0.5f * px, ymin - 0.5f * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->SafeAppend({{xmax + 0.5f * px, ymin - 0.5f * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->SafeAppend({{xmax + 0.5f * px, ymax + 0.5f * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->Submit(GL_LINE_LOOP);
	shader->Disable();
}



void CMiniMap::IntBox::DrawBox(GL::RenderDataBufferC* buffer) const
{
	const float vsx = globalRendering->viewSizeX;
	const float vsy = globalRendering->viewSizeY;

	const float tlx =         (xmin      / vsx);
	const float tly = 1.0f -  (ymin      / vsy);
	const float brx =        ((xmax + 1) / vsx);
	const float bry = 1.0f - ((ymax + 1) / vsy);

	buffer->SafeAppend({{tlx, tly, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}});
	buffer->SafeAppend({{brx, tly, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}});
	buffer->SafeAppend({{brx, bry, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}});

	buffer->SafeAppend({{brx, bry, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}});
	buffer->SafeAppend({{tlx, bry, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}});
	buffer->SafeAppend({{tlx, tly, 0.0f}, {1.0f, 1.0f, 1.0f, 0.4f}});
}

void CMiniMap::IntBox::DrawTextureBox(GL::RenderDataBufferTC* buffer) const
{
	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	buffer->SafeAppend({{(xmin    ) * px, 1.0f - (ymin    ) * py, 0.0f}, xminTx, yminTx, color}); // tl
	buffer->SafeAppend({{(xmax + 1) * px, 1.0f - (ymin    ) * py, 0.0f}, xmaxTx, yminTx, color}); // tr
	buffer->SafeAppend({{(xmax + 1) * px, 1.0f - (ymax + 1) * py, 0.0f}, xmaxTx, ymaxTx, color}); // br

	buffer->SafeAppend({{(xmax + 1) * px, 1.0f - (ymax + 1) * py, 0.0f}, xmaxTx, ymaxTx, color}); // br
	buffer->SafeAppend({{(xmin    ) * px, 1.0f - (ymax + 1) * py, 0.0f}, xminTx, ymaxTx, color}); // bl
	buffer->SafeAppend({{(xmin    ) * px, 1.0f - (ymin    ) * py, 0.0f}, xminTx, yminTx, color}); // tl
}


void CMiniMap::DrawFrame(GL::RenderDataBufferC* buffer)
{
	const float px = globalRendering->pixelX;
	const float py = globalRendering->pixelY;

	Shader::IProgramObject* shader = buffer->GetShader();
	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);

	buffer->SafeAppend({{(curPos.x            - 2 + 0.5f) * px, (curPos.y            - 2 + 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->SafeAppend({{(curPos.x            - 2 + 0.5f) * px, (curPos.y + curDim.y + 2 - 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->SafeAppend({{(curPos.x + curDim.x + 2 - 0.5f) * px, (curPos.y + curDim.y + 2 - 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->SafeAppend({{(curPos.x + curDim.x + 2 - 0.5f) * px, (curPos.y            - 2 + 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
	buffer->Submit(GL_LINE_LOOP);

	buffer->SafeAppend({{(curPos.x            - 1 + 0.5f) * px, (curPos.y            - 1 + 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
	buffer->SafeAppend({{(curPos.x            - 1 + 0.5f) * px, (curPos.y + curDim.y + 1 - 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
	buffer->SafeAppend({{(curPos.x + curDim.x + 1 - 0.5f) * px, (curPos.y + curDim.y + 1 - 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
	buffer->SafeAppend({{(curPos.x + curDim.x + 1 - 0.5f) * px, (curPos.y            - 1 + 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
	buffer->Submit(GL_LINE_LOOP);

	shader->Disable();
}

bool CMiniMap::DrawButtonQuads(GL::RenderDataBufferTC* buffer)
{
	const int x = mouse->lastx;
	const int y = mouse->lasty;

	// update the showButtons state
	if (!showButtons) {
		if (!(showButtons = (mapBox.Inside(x, y) && (buttonSize > 0) && !globalRendering->dualScreenMode)))
			return false;

	} else if (!mouseMove && !mouseResize && !mapBox.Inside(x, y) && !buttonBox.Inside(x, y))
		return (showButtons = false);

	glBindTexture(GL_TEXTURE_2D, buttonsTextureID);

	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);

	resizeBox.DrawTextureBox(buffer);
	moveBox.DrawTextureBox(buffer);
	maximizeBox.DrawTextureBox(buffer);
	minimizeBox.DrawTextureBox(buffer);

	buffer->Submit(GL_TRIANGLES);
	shader->Disable();

	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

bool CMiniMap::DrawButtonLoops(GL::RenderDataBufferC* buffer)
{
	Shader::IProgramObject* shader = buffer->GetShader();

	// matrices are already set in DrawFrame
	shader->Enable();

	{
		const int x = mouse->lastx;
		const int y = mouse->lasty;

		// highlight
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		if (mouseResize || (!mouseMove && resizeBox.Inside(x, y))) {
			resizeBox.DrawBox(buffer);
		} else if (mouseMove || (!mouseResize && moveBox.Inside(x, y))) {
			moveBox.DrawBox(buffer);
		} else if (!mouseMove && !mouseResize) {
			if (minimizeBox.Inside(x, y)) {
				minimizeBox.DrawBox(buffer);
			} else if (maximizeBox.Inside(x, y)) {
				maximizeBox.DrawBox(buffer);
			}
		}

		buffer->Submit(GL_TRIANGLES);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	{
		const float px = globalRendering->pixelX;
		const float py = globalRendering->pixelY;

		// outline the button box
		buffer->SafeAppend({{(buttonBox.xmin - 1 - 0.5f) * px, 1.0f - (buttonBox.ymin + 2 - 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
		buffer->SafeAppend({{(buttonBox.xmin - 1 - 0.5f) * px, 1.0f - (buttonBox.ymax + 2 + 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
		buffer->SafeAppend({{(buttonBox.xmax + 2 + 0.5f) * px, 1.0f - (buttonBox.ymax + 2 + 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
		buffer->SafeAppend({{(buttonBox.xmax + 2 + 0.5f) * px, 1.0f - (buttonBox.ymin + 2 - 0.5f) * py, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}});
		buffer->Submit(GL_LINE_LOOP);

		buffer->SafeAppend({{(buttonBox.xmin - 0 - 0.5f) * px, 1.0f - (buttonBox.ymin + 3 - 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
		buffer->SafeAppend({{(buttonBox.xmin - 0 - 0.5f) * px, 1.0f - (buttonBox.ymax + 1 + 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
		buffer->SafeAppend({{(buttonBox.xmax + 1 + 0.5f) * px, 1.0f - (buttonBox.ymax + 1 + 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
		buffer->SafeAppend({{(buttonBox.xmax + 1 + 0.5f) * px, 1.0f - (buttonBox.ymin + 3 - 0.5f) * py, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
		buffer->Submit(GL_LINE_LOOP);
	}

	shader->Disable();
	return true;
}




void CMiniMap::RenderCachedTextureRaw() {
	const CMatrix44f& viewMat = GL::GetMatrix(GL_MODELVIEW );
	const CMatrix44f& projMat = GL::GetMatrix(GL_PROJECTION);

	RenderCachedTextureImpl(viewMat, projMat);
}

void CMiniMap::RenderCachedTextureNormalized(bool luaCall)
{
	const CMatrix44f& viewMat = CalcNormalizedCoorMatrix(globalRendering->dualScreenMode, luaCall);
	const CMatrix44f& projMat = projMats[1];

	RenderCachedTextureImpl(viewMat, projMat);
}

void CMiniMap::RenderCachedTextureImpl(const CMatrix44f& viewMat, const CMatrix44f& projMat)
{
	// switch to top-down map/world coords (NB: z and y are swapped wrt real map/world coords)
	CMatrix44f miniMat = viewMat;

	miniMat.Translate(UpVector);
	miniMat.Scale({1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f});

	if (globalRendering->dualScreenMode)
		glAttribStatePtr->ViewPort(curPos.x, curPos.y, curDim.x, curDim.y);

	glAttribStatePtr->PushBits(GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT);
	glAttribStatePtr->DisableBlendMask();

	{
		GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
		Shader::IProgramObject* shader = buffer->GetShader();

		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMat);
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMat);

		glBindTexture(GL_TEXTURE_2D, minimapTextureID);

		buffer->SafeAppend({{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // tl
		buffer->SafeAppend({{1.0f, 0.0f, 0.0f}, 1.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // tr
		buffer->SafeAppend({{1.0f, 1.0f, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // br

		buffer->SafeAppend({{1.0f, 1.0f, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // br
		buffer->SafeAppend({{0.0f, 1.0f, 0.0f}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // bl
		buffer->SafeAppend({{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // tl
		buffer->Submit(GL_TRIANGLES);

		shader->Disable();
	}

	// clipping-planes are more work to set up
	glAttribStatePtr->EnableScissorTest();
	glAttribStatePtr->Scissor(curPos.x, curPos.y, curDim.x, curDim.y);

	{
		GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
		Shader::IProgramObject* shader = buffer->GetShader();
		GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();

		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, miniMat);
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMat);
		wla->Setup(buffer, globalRendering->viewSizeX, globalRendering->viewSizeY, 1.0f, projMat * miniMat);

		RenderCameraFrustumLinesAndSelectionBox(wla);
		RenderMarkerNotificationRectangles(buffer);

		shader->Disable();
	}

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->PopBits();

	if (globalRendering->dualScreenMode)
		glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
}

void CMiniMap::RenderCameraFrustumLinesAndSelectionBox(GL::WideLineAdapterC* wla)
{
	if (!maximized) {
		// draw the camera frustum lines
		// CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);
		CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

		cam->CalcFrustumLines(readMap->GetCurrAvgHeight(), readMap->GetCurrAvgHeight(), 1.0f, true);
		cam->ClipFrustumLines(-100.0f, mapDims.mapy * SQUARE_SIZE + 100.0f, true);

		const CCamera::FrustumLine* negLines = cam->GetNegFrustumLines();

		const SColor lineColors[] = {{0.0f, 0.0f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f, 0.75f}};
		constexpr float lineWidths[] = {2.5f, 1.5f};

		for (unsigned int j = 0; j < 2; j++) {
			wla->SetWidth(lineWidths[j]);

			for (int idx = 0; idx < /*negLines[*/4/*].sign*/; idx++) {
				const CCamera::FrustumLine& fl = negLines[idx];

				if (fl.minz >= fl.maxz)
					continue;

				wla->SafeAppend({{(fl.dir * fl.minz) + fl.base, fl.minz, 0.0f}, lineColors[j]});
				wla->SafeAppend({{(fl.dir * fl.maxz) + fl.base, fl.maxz, 0.0f}, lineColors[j]});
			}

			wla->Submit(GL_LINES);
		}
	}

	if (selecting && fullProxy) {
		// selection box
		const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];

		if (bp.movement <= mouse->dragSelectionThreshold)
			return;

		const float3 oldMapPos = GetMapPosition(bp.x, bp.y);
		const float3 newMapPos = GetMapPosition(mouse->lastx, mouse->lasty);

		// glAttribStatePtr->BlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(), (GLenum)cmdColors.MouseBoxBlendDst());
		wla->SetWidth(cmdColors.MouseBoxLineWidth());

		wla->SafeAppend({{oldMapPos.x, oldMapPos.z, 0.0f}, cmdColors.mouseBox});
		wla->SafeAppend({{newMapPos.x, oldMapPos.z, 0.0f}, cmdColors.mouseBox});
		wla->SafeAppend({{newMapPos.x, newMapPos.z, 0.0f}, cmdColors.mouseBox});
		wla->SafeAppend({{oldMapPos.x, newMapPos.z, 0.0f}, cmdColors.mouseBox});
		wla->Submit(GL_LINE_LOOP);
		// glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void CMiniMap::RenderMarkerNotificationRectangles(GL::RenderDataBufferC* buffer)
{
	if (notes.empty())
		return;

	const float baseSize = mapDims.mapx * SQUARE_SIZE;

	for (auto ni = notes.begin(); ni != notes.end(); ) {
		const float age = gu->gameTime - ni->creationTime;

		if (age > 2.0f) {
			ni = notes.erase(ni);
			continue;
		}

		SColor color(&ni->color.x);

		for (int a = 0; a < 3; ++a) {
			const float modage = age + a * 0.1f;
			const float rot = modage * 3;
			float size = baseSize - modage * baseSize * 0.9f;

			if (size < 0) {
				if (size < (-baseSize * 0.4f))
					continue;

				if (size > (-baseSize * 0.2f)) {
					size = modage * baseSize * 0.9f - baseSize;
				} else {
					size = baseSize * 1.4f - modage * baseSize * 0.9f;
				}
			}
			color.a = (255 * ni->color[3]) / (3 - a);

			const float sinSize = fastmath::sin(rot) * size;
			const float cosSize = fastmath::cos(rot) * size;

			buffer->SafeAppend({{ni->pos.x + sinSize, ni->pos.z + cosSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x + cosSize, ni->pos.z - sinSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x + cosSize, ni->pos.z - sinSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x - sinSize, ni->pos.z - cosSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x - sinSize, ni->pos.z - cosSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x - cosSize, ni->pos.z + sinSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x - cosSize, ni->pos.z + sinSize, 0.0f}, {color}});
			buffer->SafeAppend({{ni->pos.x + sinSize, ni->pos.z + cosSize, 0.0f}, {color}});
		}

		++ni;
	}

	buffer->Submit(GL_LINES);
}




void CMiniMap::DrawBackground()
{
	GL::RenderDataBuffer* buffer = &miniMap;
	Shader::IProgramObject* shader = &buffer->GetShader();

	// draw the map
	glAttribStatePtr->DisableBlendMask();

	{
		readMap->BindMiniMapTextures();
		shader->Enable();
		shader->SetUniform("u_infotex_mul", infoTextureHandler->IsEnabled() * 1.0f);
		shader->SetUniform("u_gamma_expon", globalRendering->gammaExponent);
		buffer->Submit(GL_TRIANGLES, 0, buffer->GetNumElems<MiniMapVertType>());
		shader->Disable();
	}

	glAttribStatePtr->EnableBlendMask();
}


void CMiniMap::DrawUnitIcons() const
{
	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	// switch to top-down map/world coords (z and y are swapped wrt real map/world coords)
	// viewMats[1] contains the wrong scale factor, viewMats[0] has an additional rotation
	CMatrix44f viewMat;
	viewMat.Translate(UpVector);
	viewMat.Scale({+1.0f / (mapDims.mapx * SQUARE_SIZE), -1.0f / (mapDims.mapy * SQUARE_SIZE), 1.0f});

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMat);
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);
	shader->SetUniform("u_alpha_test_ctrl", 0.0f, 1.0f, 0.0f, 0.0f); // test > 0.0
	shader->SetUniform("u_tex0", 0); // icon texture binding

	unitDrawer->DrawUnitMiniMapIcons(buffer);

	shader->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	shader->Disable();
}


void CMiniMap::DrawUnitRanges() const
{
	// draw unit ranges
	const auto& selUnits = selectedUnitsHandler.selectedUnits;
	const float4 colors[] = {cmdColors.rangeInterceptorOff, cmdColors.rangeInterceptorOn};

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, projMats[0]);
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, viewMats[0]);

	for (const int unitID: selUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);

		// LOS Ranges
		if (unit->radarRadius > 0.0f && !unit->beingBuilt && unit->activated)
			DrawCircle(buffer, {unit->pos, unit->radarRadius * 1.0f}, cmdColors.rangeRadar);

		if (unit->sonarRadius > 0.0f && !unit->beingBuilt && unit->activated)
			DrawCircle(buffer, {unit->pos, unit->sonarRadius * 1.0f}, cmdColors.rangeSonar);

		if (unit->jammerRadius > 0.0f && !unit->beingBuilt && unit->activated)
			DrawCircle(buffer, {unit->pos, unit->jammerRadius * 1.0f}, cmdColors.rangeJammer);

		// Interceptor Ranges
		for (const CWeapon* w: unit->weapons) {
			const WeaponDef& wd = *w->weaponDef;

			if ((w->range <= 300.0f) || !wd.interceptor)
				continue;

			DrawCircle(buffer, {unit->pos, wd.coverageRange}, colors[w->numStockpiled || !wd.stockpile]);
		}
	}

	buffer->Submit(GL_LINES);
	shader->Disable();
}


void CMiniMap::DrawWorldStuff() const
{
	// draw the projectiles
	if (drawProjectiles)
		projectileDrawer->DrawProjectilesMiniMap();

	{
		// draw the queued commands
		commandDrawer->DrawLuaQueuedUnitSetCommands(true);

		// NOTE: this needlessly adds to the CursorIcons list, but at least
		//       they are not drawn  (because the input receivers are drawn
		//       after the command queues)
		if ((drawCommands > 0) && guihandler->GetQueueKeystate())
			selectedUnitsHandler.DrawCommands(true);

		// draw lines batched by the above calls
		lineDrawer.DrawAll(true);
	}

	// draw the selection shape, and some ranges
	if (drawCommands > 0)
		guihandler->DrawMapStuff(true);

	DrawUnitRanges();
}

