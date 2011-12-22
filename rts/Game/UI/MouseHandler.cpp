/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MouseHandler.h"

#include "CommandColors.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseCursor.h"
#include "TooltipConsole.h"
#include "Game/CameraHandler.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/InMapDraw.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/SelectedUnits.h"
#include "Game/TraceRay.h"
#include "Game/Camera/CameraController.h"
#include "Game/UI/UnitTracker.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaUI.h" // FIXME: for GML
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/mmgr.h"
#include "System/Util.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"

#include <algorithm>
#include <boost/cstdint.hpp>

// can't be up there since those contain conflicting definitions
#include <SDL_mouse.h>
#include <SDL_events.h>
#include <SDL_keysym.h>


CONFIG(bool, HardwareCursor).defaultValue(false);
CONFIG(bool, InvertMouse).defaultValue(false);
CONFIG(float, DoubleClickTime).defaultValue(200.0f);

CONFIG(float, ScrollWheelSpeed)
	.defaultValue(25.0f)
	.minimumValue(-255.f)
	.maximumValue(255.f);

CONFIG(float, CrossSize).defaultValue(12.0f);
CONFIG(float, CrossAlpha).defaultValue(0.5f);
CONFIG(float, CrossMoveScale).defaultValue(1.0f);
CONFIG(float, MouseDragScrollThreshold).defaultValue(0.3f);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMouseHandler* mouse = NULL;

static CInputReceiver*& activeReceiver = CInputReceiver::GetActiveReceiverRef();


CMouseHandler::CMouseHandler()
	: lastx(-1)
	, lasty(-1)
	, locked(false)
	, doubleClickTime(0.0f)
	, scrollWheelSpeed(0.0f)
	, activeButton(-1)
	, dir(ZeroVector)
	, wasLocked(false)
	, crossSize(0.0f)
	, crossAlpha(0.0f)
	, crossMoveScale(0.0f)
	, cursorText("")
	, currentCursor(NULL)
	, cursorScale(1.0f)
	, hide(true)
	, hwHide(true)
	, hardwareCursor(false)
	, invertMouse(false)
	, dragScrollThreshold(0.0f)
	, scrollx(0.0f)
	, scrolly(0.0f)
{
	const int2 mousepos = IMouseInput::GetInstance()->GetPos();
	lastx = mousepos.x;
	lasty = mousepos.y;

	for (int a = 1; a <= NUM_BUTTONS; a++) {
		buttons[a].pressed = false;
		buttons[a].lastRelease = -20;
		buttons[a].movement = 0;
	}

	LoadCursors();

	currentCursor = cursorCommandMap[""];

#ifndef __APPLE__
	hardwareCursor = configHandler->GetBool("HardwareCursor");
#endif

	invertMouse = configHandler->GetBool("InvertMouse");
	doubleClickTime = configHandler->GetFloat("DoubleClickTime") / 1000.0f;

	scrollWheelSpeed = configHandler->GetFloat("ScrollWheelSpeed");

	crossSize      = configHandler->GetFloat("CrossSize");
	crossAlpha     = configHandler->GetFloat("CrossAlpha");
	crossMoveScale = configHandler->GetFloat("CrossMoveScale") * 0.005f;

	dragScrollThreshold = configHandler->GetFloat("MouseDragScrollThreshold");

	configHandler->NotifyOnChange(this);
}

CMouseHandler::~CMouseHandler()
{
	if (hwHide)
		SDL_ShowCursor(SDL_ENABLE);

	std::map<std::string, CMouseCursor*>::iterator ci;
	for (ci = cursorFileMap.begin(); ci != cursorFileMap.end(); ++ci) {
		delete ci->second;
	}
}


void CMouseHandler::LoadCursors()
{
	const CMouseCursor::HotSpot mCenter  = CMouseCursor::Center;
	const CMouseCursor::HotSpot mTopLeft = CMouseCursor::TopLeft;

	CMouseCursor* nullCursor = CMouseCursor::GetNullCursor();
	cursorCommandMap["none"] = nullCursor;
	// Note: we intentionally don't add it there cause GetNullCursor() returns
	//  a pointer to a static var, so it gets automatically deleted
	//cursorFileMap["null"] = nullCursor;

	AssignMouseCursor("",             "cursornormal",     mTopLeft, false);

	AssignMouseCursor("Area attack",  "cursorareaattack", mCenter,  false);
	AssignMouseCursor("Area attack",  "cursorattack",     mCenter,  false); // backup

	AssignMouseCursor("Attack",       "cursorattack",     mCenter,  false);
	AssignMouseCursor("BuildBad",     "cursorbuildbad",   mCenter,  false);
	AssignMouseCursor("BuildGood",    "cursorbuildgood",  mCenter,  false);
	AssignMouseCursor("Capture",      "cursorcapture",    mCenter,  false);
	AssignMouseCursor("Centroid",     "cursorcentroid",   mCenter,  false);

	AssignMouseCursor("DeathWait",    "cursordwatch",     mCenter,  false);
	AssignMouseCursor("DeathWait",    "cursorwait",       mCenter,  false); // backup

	AssignMouseCursor("ManualFire",   "cursormanfire",    mCenter,  false);
	AssignMouseCursor("ManualFire",   "cursordgun",       mCenter,  false); // backup (backward compability)
	AssignMouseCursor("ManualFire",   "cursorattack",     mCenter,  false); // backup

	AssignMouseCursor("Fight",        "cursorfight",      mCenter,  false);
	AssignMouseCursor("Fight",        "cursorattack",     mCenter,  false); // backup

	AssignMouseCursor("GatherWait",   "cursorgather",     mCenter,  false);
	AssignMouseCursor("GatherWait",   "cursorwait",       mCenter,  false); // backup

	AssignMouseCursor("Guard",        "cursordefend",     mCenter,  false);
	AssignMouseCursor("Load units",   "cursorpickup",     mCenter,  false);
	AssignMouseCursor("Move",         "cursormove",       mCenter,  false);
	AssignMouseCursor("Patrol",       "cursorpatrol",     mCenter,  false);
	AssignMouseCursor("Reclaim",      "cursorreclamate",  mCenter,  false);
	AssignMouseCursor("Repair",       "cursorrepair",     mCenter,  false);

	AssignMouseCursor("Resurrect",    "cursorrevive",     mCenter,  false);
	AssignMouseCursor("Resurrect",    "cursorrepair",     mCenter,  false); // backup

	AssignMouseCursor("Restore",      "cursorrestore",    mCenter,  false);
	AssignMouseCursor("Restore",      "cursorrepair",     mCenter,  false); // backup

	AssignMouseCursor("SelfD",        "cursorselfd",      mCenter,  false);

	AssignMouseCursor("SquadWait",    "cursornumber",     mCenter,  false);
	AssignMouseCursor("SquadWait",    "cursorwait",       mCenter,  false); // backup

	AssignMouseCursor("TimeWait",     "cursortime",       mCenter,  false);
	AssignMouseCursor("TimeWait",     "cursorwait",       mCenter,  false); // backup

	AssignMouseCursor("Unload units", "cursorunload",     mCenter,  false);
	AssignMouseCursor("Wait",         "cursorwait",       mCenter,  false);

	// the default cursor must exist
	if (cursorCommandMap.find("") == cursorCommandMap.end()) {
		throw content_error(
			"Unable to load default cursor. Check that you have the required\n"
			"content packages installed in your Spring \"base/\" directory.\n");
	}
}


/******************************************************************************/

void CMouseHandler::MouseMove(int x, int y, int dx, int dy)
{
	//FIXME don't update with lock?
	lastx = x;
	lasty = y;

	const int screenCenterX = globalRendering->viewSizeX / 2 + globalRendering->viewPosX;
	const int screenCenterY = globalRendering->viewSizeY / 2 + globalRendering->viewPosY;

	scrollx += lastx - screenCenterX;
	scrolly += lasty - screenCenterY;

	dir = hide ? camera->forward : camera->CalcPixelDir(x,y);

	if (locked) {
		camHandler->GetCurrentController().MouseMove(float3(dx, dy, invertMouse ? -1.0f : 1.0f));
		return;
	}

	const int movedPixels = (int)sqrt(float(dx*dx + dy*dy));
	buttons[SDL_BUTTON_LEFT].movement  += movedPixels;
	buttons[SDL_BUTTON_RIGHT].movement += movedPixels;

	if (!game->gameOver) {
		playerHandler->Player(gu->myPlayerNum)->currentStats.mousePixels += movedPixels;
	}

	if (activeReceiver) {
		activeReceiver->MouseMove(x, y, dx, dy, activeButton);
	}

	if (inMapDrawer && inMapDrawer->IsDrawMode()) {
		inMapDrawer->MouseMove(x, y, dx, dy, activeButton);
	}

	if (buttons[SDL_BUTTON_MIDDLE].pressed && (activeReceiver == NULL)) {
		camHandler->GetCurrentController().MouseMove(float3(dx, dy, invertMouse ? -1.0f : 1.0f));
		unitTracker.Disable();
		return;
	}
}


void CMouseHandler::MousePress(int x, int y, int button)
{
	if (button > NUM_BUTTONS)
		return;

	camHandler->GetCurrentController().MousePress(x, y, button);

	dir = hide ? camera->forward : camera->CalcPixelDir(x, y);

	if (!game->gameOver)
		playerHandler->Player(gu->myPlayerNum)->currentStats.mouseClicks++;

	ButtonPressEvt& bp = buttons[button];
	bp.chorded  = (buttons[SDL_BUTTON_LEFT].pressed || buttons[SDL_BUTTON_RIGHT].pressed);
	bp.pressed  = true;
	bp.time     = gu->gameTime;
	bp.x        = x;
	bp.y        = y;
	bp.camPos   = camera->pos;
	bp.dir      = dir;
	bp.movement = 0;

	activeButton = button;

	if (activeReceiver && activeReceiver->MousePress(x, y, button))
		return;

	if (inMapDrawer && inMapDrawer->IsDrawMode()) {
		inMapDrawer->MousePress(x, y, button);
		return;
	}

	// limited receivers for MMB
	if (button == SDL_BUTTON_MIDDLE) {
		if (!locked) {
			if (luaInputReceiver != NULL) {
				if (luaInputReceiver->MousePress(x, y, button)) {
					activeReceiver = luaInputReceiver;
					return;
				}
			}
			if ((minimap != NULL) && minimap->FullProxy()) {
				if (minimap->MousePress(x, y, button)) {
					activeReceiver = minimap;
					return;
				}
			}
		}
		return;
	}

	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	if (!game->hideInterface) {
		for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
			CInputReceiver* recv=*ri;
			if (recv && recv->MousePress(x, y, button))
			{
				if (!activeReceiver)
					activeReceiver = recv;
				return;
			}
		}
	} else {
		if (luaInputReceiver && luaInputReceiver->MousePress(x, y, button)) {
			if (!activeReceiver)
				activeReceiver = luaInputReceiver;
			return;
		}
		if (guihandler && guihandler->MousePress(x,y,button)) {
			if (!activeReceiver)
				activeReceiver = guihandler; // for default (rmb) commands
			return;
		}
	}
}


/**
 * GetSelectionBoxCoeff
 *  returns the topright & bottomleft corner positions of the SelectionBox in (cam->right, cam->up)-space
 */
void CMouseHandler::GetSelectionBoxCoeff(const float3& pos1, const float3& dir1, const float3& pos2, const float3& dir2, float2& topright, float2& btmleft)
{
	float dist = ground->LineGroundCol(pos1, pos1 + dir1 * globalRendering->viewRange * 1.4f, false);
	if(dist < 0) dist = globalRendering->viewRange * 1.4f;
	float3 gpos1 = pos1 + dir1 * dist;

	dist = ground->LineGroundCol(pos2, pos2 + dir2 * globalRendering->viewRange * 1.4f, false);
	if(dist < 0) dist = globalRendering->viewRange * 1.4f;
	float3 gpos2 = pos2 + dir2 * dist;

	const float3 cdir1 = (gpos1 - camera->pos).ANormalize();
	const float3 cdir2 = (gpos2 - camera->pos).ANormalize();

	// prevent DivByZero
	float cdir1_fw = cdir1.dot(camera->forward); if (cdir1_fw == 0.0f) cdir1_fw = 0.0001f;
	float cdir2_fw = cdir2.dot(camera->forward); if (cdir2_fw == 0.0f) cdir2_fw = 0.0001f;

	// one corner of the rectangle
	topright.x = cdir1.dot(camera->right) / cdir1_fw;
	topright.y = cdir1.dot(camera->up)    / cdir1_fw;

	// opposite corner
	btmleft.x = cdir2.dot(camera->right) / cdir2_fw;
	btmleft.y = cdir2.dot(camera->up)    / cdir2_fw;

	// sort coeff so topright really is the topright corner
	if (topright.x < btmleft.x) std::swap(topright.x, btmleft.x);
	if (topright.y < btmleft.y) std::swap(topright.y, btmleft.y);
}


void CMouseHandler::MouseRelease(int x, int y, int button)
{
	if (button > NUM_BUTTONS)
		return;

	camHandler->GetCurrentController().MouseRelease(x, y, button);

	dir = hide ? camera->forward : camera->CalcPixelDir(x, y);
	buttons[button].pressed = false;

	if (inMapDrawer && inMapDrawer->IsDrawMode()){
		inMapDrawer->MouseRelease(x, y, button);
		return;
	}

	if (activeReceiver) {
		activeReceiver->MouseRelease(x, y, button);
		if(!buttons[SDL_BUTTON_LEFT].pressed && !buttons[SDL_BUTTON_MIDDLE].pressed && !buttons[SDL_BUTTON_RIGHT].pressed)
			activeReceiver = NULL;
		return;
	}

	GML_RECMUTEX_LOCK(sel); //FIXME redundant? (selectedUnits already has mutexes)

	// Switch camera mode on a middle click that wasn't a middle mouse drag scroll.
	// the latter is determined by the time the mouse was held down:
	// switch (dragScrollThreshold)
	//   <= means a camera mode switch
	//    > means a drag scroll
	if (button == SDL_BUTTON_MIDDLE) {
		if (buttons[SDL_BUTTON_MIDDLE].time > (gu->gameTime - dragScrollThreshold))
			ToggleMiddleClickScroll();
		return;
	}

	if (gu->fpsMode) {
		return;
	}

	if ((button == SDL_BUTTON_LEFT) && !buttons[button].chorded) {
		ButtonPressEvt& bp = buttons[SDL_BUTTON_LEFT];

		if (!keyInput->IsKeyPressed(SDLK_LSHIFT) && !keyInput->IsKeyPressed(SDLK_LCTRL)) {
			selectedUnits.ClearSelected();
		}

		if (bp.movement > 4) {
			// select box
			float2 topright, btmleft;
			GetSelectionBoxCoeff(bp.camPos, bp.dir, camera->pos, dir, topright, btmleft);

			// GetSelectionBoxCoeff returns us the corner pos, but we want to do a inview frustum check.
			// To do so we need the frustum planes (= plane normal + plane offset).
			float3 norm1 = camera->up;
			float3 norm2 = -camera->up;
			float3 norm3 = camera->right;
			float3 norm4 = -camera->right;

			#define signf(x) ((x>0.0f) ? 1.0f : -1)
			if (topright.y != 0) norm1 = (camera->forward * signf(-topright.y)) + (camera->up / fabs(topright.y));
			if (btmleft.y  != 0) norm2 = (camera->forward * signf(  btmleft.y)) - (camera->up / fabs(btmleft.y));
			if (topright.x != 0) norm3 = (camera->forward * signf(-topright.x)) + (camera->right / fabs(topright.x));
			if (btmleft.x  != 0) norm4 = (camera->forward * signf(  btmleft.x)) - (camera->right / fabs(btmleft.x));

			const float4 plane1(norm1, -(norm1.dot(camera->pos)));
			const float4 plane2(norm2, -(norm2.dot(camera->pos)));
			const float4 plane3(norm3, -(norm3.dot(camera->pos)));
			const float4 plane4(norm4, -(norm4.dot(camera->pos)));

			selectedUnits.HandleUnitBoxSelection(plane1, plane2, plane3, plane4);
		} else {
			CUnit* unit;
			CFeature* feature;
			TraceRay::GuiTraceRay(camera->pos, dir, globalRendering->viewRange * 1.4f, false, NULL, unit, feature);

			selectedUnits.HandleSingleUnitClickSelection(unit, true);
		}

		bp.lastRelease = gu->gameTime;
	}
}


void CMouseHandler::MouseWheel(float delta)
{
	if (eventHandler.MouseWheel(delta>0.0f, delta)) {
		return;
	}
	delta *= scrollWheelSpeed;
	camHandler->GetCurrentController().MouseWheelMove(delta);
}


void CMouseHandler::DrawSelectionBox()
{
	dir = hide ? camera->forward : camera->CalcPixelDir(lastx, lasty);
	if (activeReceiver) {
		return;
	}

	if (gu->fpsMode) {
		return;
	}

	ButtonPressEvt& bp = buttons[SDL_BUTTON_LEFT];

	if (bp.pressed && !bp.chorded && (bp.movement > 4) &&
	   (!inMapDrawer || !inMapDrawer->IsDrawMode()))
	{
		float2 topright, btmleft;
		GetSelectionBoxCoeff(bp.camPos, bp.dir, camera->pos, dir, topright, btmleft);

		float3 dir1S = camera->right * topright.x;
		float3 dir1U = camera->up    * topright.y;
		float3 dir2S = camera->right * btmleft.x;
		float3 dir2U = camera->up    * btmleft.y;

		glColor4fv(cmdColors.mouseBox);

		glPushAttrib(GL_ENABLE_BIT);

		glDisable(GL_FOG);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		            (GLenum)cmdColors.MouseBoxBlendDst());

		glLineWidth(cmdColors.MouseBoxLineWidth());

		float3 verts[] = {
			camera->pos + (dir1U + dir1S + camera->forward) * 30,
			camera->pos + (dir2U + dir1S + camera->forward) * 30,
			camera->pos + (dir2U + dir2S + camera->forward) * 30,
			camera->pos + (dir1U + dir2S + camera->forward) * 30,
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);

		glLineWidth(1.0f);

		glPopAttrib();
	}
}


// CALLINFO:
// LuaUnsyncedRead::GetCurrentTooltip
// CTooltipConsole::Draw --> CMouseHandler::GetCurrentTooltip
std::string CMouseHandler::GetCurrentTooltip()
{
	std::string s;
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv=*ri;
		if (recv && recv->IsAbove(lastx, lasty)) {
			s = recv->GetTooltip(lastx, lasty);
			if (s != "") {
				return s;
			}
		}
	}

	const string buildTip = guihandler->GetBuildTooltip();
	if (!buildTip.empty()) {
		return buildTip;
	}

	const float range = (globalRendering->viewRange * 1.4f);

	CUnit* unit;
	CFeature* feature;
	float dist;
	{
		GML_THRMUTEX_LOCK(unit, GML_DRAW); // GetCurrentTooltip
		GML_THRMUTEX_LOCK(feat, GML_DRAW); // GetCurrentTooltip

		dist = TraceRay::GuiTraceRay(camera->pos, dir, range, true, NULL, unit, feature);

		if (unit)    return CTooltipConsole::MakeUnitString(unit);
		if (feature) return CTooltipConsole::MakeFeatureString(feature);
	}

	const string selTip = selectedUnits.GetTooltip();
	if (selTip != "") {
		return selTip;
	}

	if (dist <= range) {
		const float3 pos = camera->pos + (dir * dist);
		return CTooltipConsole::MakeGroundString(pos);
	}

	return "";
}


void CMouseHandler::Update()
{
	SetCursor(newCursor);

	if (!hide) {
		return;
	}

	// Update MiddleClickScrolling
	scrollx *= 0.5f;
	scrolly *= 0.5f;
	lastx = globalRendering->viewSizeX / 2 + globalRendering->viewPosX;
	lasty = globalRendering->viewSizeY / 2 + globalRendering->viewPosY;
	if (globalRendering->active) {
		mouseInput->SetPos(int2(lastx, lasty));
	}
}


void CMouseHandler::WarpMouse(int x, int y)
{
	if (!locked) {
		lastx = x + globalRendering->viewPosX;
		lasty = y + globalRendering->viewPosY;
		mouseInput->SetPos(int2(lastx, lasty));
	}
}


void CMouseHandler::ShowMouse()
{
	if (hide) {
		hide = false;
		cursorText = "none"; // force hardware cursor rebinding (else we have standard b&w cursor)

		// I don't use SDL_ShowCursor here 'cos it would cause a flicker with hwCursor
		// (flicker caused by switching between default cursor and later the really one e.g. `attack`)
		// instead update state and cursor at the same time
		if (hardwareCursor) {
			hwHide = true;
		} else {
			SDL_ShowCursor(SDL_DISABLE);
		}
	}
}


void CMouseHandler::HideMouse()
{
	if (!hide) {
		hwHide = true;
		SDL_ShowCursor(SDL_DISABLE);
		mouseInput->SetWMMouseCursor(NULL);
		scrollx = 0.f;
		scrolly = 0.f;
		lastx = globalRendering->viewSizeX / 2 + globalRendering->viewPosX;
		lasty = globalRendering->viewSizeY / 2 + globalRendering->viewPosY;
		mouseInput->SetPos(int2(lastx, lasty));
		hide = true;
	}
}


void CMouseHandler::ToggleMiddleClickScroll()
{
	if (locked) {
		locked = false;
		ShowMouse();
	} else {
		locked = true;
		HideMouse();
	}
}


void CMouseHandler::ToggleHwCursor(const bool& enable)
{
	hardwareCursor = enable;
	if (hardwareCursor) {
		hwHide = true;
	} else {
		mouseInput->SetWMMouseCursor(NULL);
		SDL_ShowCursor(SDL_DISABLE);
	}
	cursorText = "none";
}


/******************************************************************************/

void CMouseHandler::ChangeCursor(const std::string& cmdName, const float& scale)
{
	newCursor = cmdName;
	cursorScale = scale;
}


void CMouseHandler::SetCursor(const std::string& cmdName, const bool& forceRebind)
{
	if ((cursorText == cmdName) && !forceRebind) {
		return;
	}

	cursorText = cmdName;
	map<string, CMouseCursor*>::iterator it = cursorCommandMap.find(cmdName);
	if (it != cursorCommandMap.end()) {
		currentCursor = it->second;
	} else {
		currentCursor = cursorCommandMap[""];
	}

	if (hardwareCursor && !hide) {
		if (currentCursor->hwValid) {
			hwHide = false;
			currentCursor->BindHwCursor(); // calls SDL_ShowCursor(SDL_ENABLE);
		} else {
			hwHide = true;
			SDL_ShowCursor(SDL_DISABLE);
			mouseInput->SetWMMouseCursor(NULL);
		}
	}
}


void CMouseHandler::UpdateCursors()
{
	// we update all cursors (for the command queue icons)
	map<string, CMouseCursor *>::iterator it;
	for (it = cursorFileMap.begin(); it != cursorFileMap.end(); ++it) {
		if (it->second != NULL) {
			it->second->Update();
		}
	}
}

void CMouseHandler::DrawScrollCursor()
{
	const float scaleL = fabs(std::min(0.0f,scrollx)) * crossMoveScale + 1.0f;
	const float scaleT = fabs(std::min(0.0f,scrolly)) * crossMoveScale + 1.0f;
	const float scaleR = fabs(std::max(0.0f,scrollx)) * crossMoveScale + 1.0f;
	const float scaleB = fabs(std::max(0.0f,scrolly)) * crossMoveScale + 1.0f;

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 1.0f, 1.0f, crossAlpha);
			glVertex2f(   0.f * scaleT,  1.00f * scaleT);
			glVertex2f( 0.33f * scaleT,  0.66f * scaleT);
			glVertex2f(-0.33f * scaleT,  0.66f * scaleT);

			glVertex2f(   0.f * scaleB, -1.00f * scaleB);
			glVertex2f( 0.33f * scaleB, -0.66f * scaleB);
			glVertex2f(-0.33f * scaleB, -0.66f * scaleB);

			glVertex2f(-1.00f * scaleL,    0.f * scaleL);
			glVertex2f(-0.66f * scaleL,  0.33f * scaleL);
			glVertex2f(-0.66f * scaleL, -0.33f * scaleL);

			glVertex2f( 1.00f * scaleR,    0.f * scaleR);
			glVertex2f( 0.66f * scaleR,  0.33f * scaleR);
			glVertex2f( 0.66f * scaleR, -0.33f * scaleR);

		glColor4f(1.0f, 1.0f, 1.0f, crossAlpha);
			glVertex2f(-0.33f * scaleT,  0.66f * scaleT);
			glVertex2f( 0.33f * scaleT,  0.66f * scaleT);
		glColor4f(0.2f, 0.2f, 0.2f, 0.f);
			glVertex2f(   0.f,    0.f);

		glColor4f(1.0f, 1.0f, 1.0f, crossAlpha);
			glVertex2f(-0.33f * scaleB, -0.66f * scaleB);
			glVertex2f( 0.33f * scaleB, -0.66f * scaleB);
		glColor4f(0.2f, 0.2f, 0.2f, 0.f);
			glVertex2f(   0.f,    0.f);

		glColor4f(1.0f, 1.0f, 1.0f, crossAlpha);
			glVertex2f(-0.66f * scaleL,  0.33f * scaleL);
			glVertex2f(-0.66f * scaleL, -0.33f * scaleL);
		glColor4f(0.2f, 0.2f, 0.2f, 0.f);
			glVertex2f(   0.f,    0.f);

		glColor4f(1.0f, 1.0f, 1.0f, crossAlpha);
			glVertex2f( 0.66f * scaleR, -0.33f * scaleR);
			glVertex2f( 0.66f * scaleR,  0.33f * scaleR);
		glColor4f(0.2f, 0.2f, 0.2f, 0.f);
			glVertex2f(   0.f,    0.f);
	glEnd();

	glEnable(GL_POINT_SMOOTH);

	glPointSize(crossSize * 0.6f);
	glBegin(GL_POINTS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.2f * crossAlpha);
		glVertex2f(0.f, 0.f);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPointSize(1.0f);
}


void CMouseHandler::DrawFPSCursor()
{
	glDisable(GL_TEXTURE_2D);

	const float wingHalf = fastmath::PI / 9.0f;
	const int stepNumHalf = 2;
	const float step = wingHalf / stepNumHalf;

	glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

		for (float angle = 0.0f; angle < fastmath::PI2; angle += fastmath::PI2 / 3.f) {
			for (int i = -stepNumHalf; i < stepNumHalf; i++) {
				glVertex2f(0.1f * fastmath::sin(angle),                0.1f * fastmath::cos(angle));
				glVertex2f(0.8f * fastmath::sin(angle +     i * step), 0.8f * fastmath::cos(angle +     i * step));
				glVertex2f(0.8f * fastmath::sin(angle + (i+1) * step), 0.8f * fastmath::cos(angle + (i+1) * step));
			}
		}
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


void CMouseHandler::DrawCursor()
{
	assert(currentCursor);
	
	if (guihandler)
		guihandler->DrawCentroidCursor();

	if (locked) {
		if (crossSize > 0.0f) {
			const float xscale = (crossSize / globalRendering->viewSizeX);
			const float yscale = (crossSize / globalRendering->viewSizeY);

			glPushMatrix();
			glTranslatef(0.5f - globalRendering->pixelX * 0.5f, 0.5f - globalRendering->pixelY * 0.5f, 0.f);
			glScalef(xscale, yscale, 1.f);

			if (gu->fpsMode) {
				DrawFPSCursor();
			} else {
				DrawScrollCursor();
			}

			glPopMatrix();
		}

		glEnable(GL_TEXTURE_2D);
		return;
	}

	if (hide)
		return;

	if (hardwareCursor && currentCursor->hwValid)
		return;

	// draw the 'software' cursor
	if (cursorScale >= 0.0f) {
		currentCursor->Draw(lastx, lasty, cursorScale);
	}
	else {
		// hovered minimap, show default cursor and draw `special` cursor scaled-down bottom right of the default one
		CMouseCursor* nc = cursorFileMap["cursornormal"];
		if (nc == NULL) {
			currentCursor->Draw(lastx, lasty, -cursorScale);
		}
		else {
			nc->Draw(lastx, lasty, 1.0f);
			if (currentCursor != nc) {
				currentCursor->Draw(lastx + nc->GetMaxSizeX(),
				                    lasty + nc->GetMaxSizeY(), -cursorScale);
			}
		}
	}
}


bool CMouseHandler::AssignMouseCursor(const std::string& cmdName,
                                      const std::string& fileName,
                                      CMouseCursor::HotSpot hotSpot,
                                      bool overwrite)
{
	std::map<std::string, CMouseCursor*>::iterator cmdIt;
	std::map<std::string, CMouseCursor*>::iterator fileIt;

	cmdIt = cursorCommandMap.find(cmdName);
	fileIt = cursorFileMap.find(fileName);

	const bool haveCmd = (cmdIt != cursorCommandMap.end());
	const bool haveFile = (fileIt != cursorFileMap.end());

	if (haveCmd && !overwrite) {
		return false; // already assigned
	}

	if (haveFile) {
		// cursor is already loaded, reuse it
		cursorCommandMap[cmdName] = fileIt->second;
		return true;
	}

	CMouseCursor* oldCursor = haveCmd ? cmdIt->second : NULL;
	CMouseCursor* newCursor = CMouseCursor::New(fileName, hotSpot);

	if (newCursor == NULL) {
		return false; // invalid cursor
	}

	cursorFileMap[fileName] = newCursor;

	// assign the new cursor
	cursorCommandMap[cmdName] = newCursor;

	SafeDeleteCursor(oldCursor);
	return true;
}


bool CMouseHandler::ReplaceMouseCursor(const string& oldName,
                                       const string& newName,
                                       CMouseCursor::HotSpot hotSpot)
{
	std::map<std::string, CMouseCursor*>::iterator fileIt;
	fileIt = cursorFileMap.find(oldName);
	if (fileIt == cursorFileMap.end()) {
		return false;
	}

	CMouseCursor* newCursor = CMouseCursor::New(newName, hotSpot);
	if (newCursor == NULL) {
		return false; // leave the old one
	}

	CMouseCursor* oldCursor = fileIt->second;

	std::map<std::string, CMouseCursor*>& cmdMap = cursorCommandMap;
	std::map<std::string, CMouseCursor*>::iterator cmdIt;
	for (cmdIt = cmdMap.begin(); cmdIt != cmdMap.end(); ++cmdIt) {
		if (cmdIt->second == oldCursor) {
			cmdIt->second = newCursor;
		}
	}

	fileIt->second = newCursor;

	if (currentCursor == oldCursor) {
		SetCursor(cursorText, true);
	}

	delete oldCursor;
	return true;
}


void CMouseHandler::SafeDeleteCursor(CMouseCursor* cursor)
{
	std::map<std::string, CMouseCursor*>::iterator it;

	for (it = cursorCommandMap.begin(); it != cursorCommandMap.end(); ++it) {
		if (it->second == cursor) {
			return; // being used, can't delete
		}
	}

	for (it = cursorFileMap.begin(); it != cursorFileMap.end(); ++it) {
		if (it->second == cursor) {
			it = set_erase(cursorFileMap, it);
		}
	}

	if (currentCursor == cursor) {
		SetCursor("none", true);
	}

	delete cursor;
}


/******************************************************************************/

void CMouseHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	if (key == "MouseDragScrollThreshold") {
		dragScrollThreshold = atof(value.c_str());
	}
}
