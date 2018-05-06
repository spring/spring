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
#include "Game/SelectedUnitsHandler.h"
#include "Game/TraceRay.h"
#include "Game/Camera/CameraController.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/UnitTracker.h"
#include "Lua/LuaInputReceiver.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Game/UI/Groups/Group.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"

#include <algorithm>

// can't be up there since those contain conflicting definitions
#include <SDL_mouse.h>
#include <SDL_events.h>
#include <SDL_keycode.h>


CONFIG(bool, HardwareCursor).defaultValue(false).description("Sets hardware mouse cursor rendering. If you have a low framerate, your mouse cursor will seem \"laggy\". Setting hardware cursor will render the mouse cursor separately from spring and the mouse will behave normally. Note, not all GPU drivers support it in fullscreen mode!");
CONFIG(bool, InvertMouse).defaultValue(false);
CONFIG(float, DoubleClickTime).defaultValue(200.0f).description("Double click time in milliseconds.");

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

CMouseHandler* mouse = nullptr;

static CInputReceiver*& activeReceiver = CInputReceiver::GetActiveReceiverRef();


CMouseHandler::CMouseHandler()
{
	const int2 mousepos = IMouseInput::GetInstance()->GetPos();

	lastx = mousepos.x;
	lasty = mousepos.y;

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

	configHandler->NotifyOnChange(this, {"MouseDragScrollThreshold", "ScrollWheelSpeed"});
}

CMouseHandler::~CMouseHandler()
{
	if (hwHide)
		SDL_ShowCursor(SDL_ENABLE);

	configHandler->RemoveObserver(this);
}


void CMouseHandler::InitStatic()
{
	assert(mouse == nullptr);
	mouse = new CMouseHandler();
}

void CMouseHandler::KillStatic()
{
	spring::SafeDelete(mouse);
}


void CMouseHandler::ReloadCursors()
{
	const CMouseCursor::HotSpot mCenter  = CMouseCursor::Center;
	const CMouseCursor::HotSpot mTopLeft = CMouseCursor::TopLeft;

	activeCursorIdx = -1;

	loadedCursors.clear();
	loadedCursors.reserve(32);
	// null-cursor; always lives at index 0
	loadedCursors.emplace_back();

	cursorCommandMap.clear();
	cursorCommandMap.reserve(32);
	cursorFileMap.clear();
	cursorFileMap.reserve(32);

	cursorCommandMap["none"] = loadedCursors.size() - 1;
	cursorFileMap["null"] = loadedCursors.size() - 1;

	AssignMouseCursor("",             "cursornormal",     mTopLeft, false);

	AssignMouseCursor("Area attack",  "cursorareaattack", mCenter,  false);
	AssignMouseCursor("Area attack",  "cursorattack",     mCenter,  false); // backup

	AssignMouseCursor("Attack",       "cursorattack",     mCenter,  false);
	AssignMouseCursor("AttackBad",    "cursorattackbad",  mCenter,  false);
	AssignMouseCursor("AttackBad",    "cursorattack",     mCenter,  false); // backup
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
	const auto defCursorIt = cursorCommandMap.find("");

	if (defCursorIt == cursorCommandMap.end()) {
		throw content_error(
			"Unable to load default cursor. Check that you have the required\n"
			"content packages installed in your Spring \"base/\" directory.\n");
	}

	activeCursorIdx = defCursorIt->second;
}


/******************************************************************************/

void CMouseHandler::MouseMove(int x, int y, int dx, int dy)
{
	// FIXME: don't update if locked?
	lastx = std::abs(x);
	lasty = std::abs(y);

	// MouseInput passes negative coordinates when cursor leaves the window
	offscreen = (x < 0 && y < 0);

	const int2 screenCenter = globalRendering->GetScreenCenter();

	scrollx += (lastx - screenCenter.x);
	scrolly += (lasty - screenCenter.y);

	dir = hide ? camera->GetDir() : camera->CalcPixelDir(x, y);

	if (locked && camHandler != nullptr) {
		camHandler->GetCurrentController().MouseMove(float3(dx, dy, invertMouse ? -1.0f : 1.0f));
		return;
	}

	const int movedPixels = (int)fastmath::sqrt_sse(float(dx*dx + dy*dy));
	buttons[SDL_BUTTON_LEFT].movement  += movedPixels;
	buttons[SDL_BUTTON_RIGHT].movement += movedPixels;

	if (game != nullptr && !game->IsGameOver())
		playerHandler.Player(gu->myPlayerNum)->currentStats.mousePixels += movedPixels;

	if (activeReceiver != nullptr)
		activeReceiver->MouseMove(x, y, dx, dy, activeButtonIdx);

	if (inMapDrawer != nullptr && inMapDrawer->IsDrawMode())
		inMapDrawer->MouseMove(x, y, dx, dy, activeButtonIdx);

	if (buttons[SDL_BUTTON_MIDDLE].pressed && (activeReceiver == NULL) && camHandler != nullptr) {
		camHandler->GetCurrentController().MouseMove(float3(dx, dy, invertMouse ? -1.0f : 1.0f));
		unitTracker.Disable();
		return;
	}
}


void CMouseHandler::MousePress(int x, int y, int button)
{
	if (button > NUM_BUTTONS)
		return;

	dir = hide ? camera->GetDir() : camera->CalcPixelDir(x, y);

	if (game != nullptr && !game->IsGameOver())
		playerHandler.Player(gu->myPlayerNum)->currentStats.mouseClicks++;

	ButtonPressEvt& bp = buttons[button];
	bp.chorded  = (buttons[SDL_BUTTON_LEFT].pressed || buttons[SDL_BUTTON_RIGHT].pressed);
	bp.pressed  = true;
	bp.time     = gu->gameTime;
	bp.x        = x;
	bp.y        = y;
	bp.camPos   = camera->GetPos();
	bp.dir      = dir;
	bp.movement = 0;

	activeButtonIdx = button;

	if (activeReceiver && activeReceiver->MousePress(x, y, button))
		return;

	if (inMapDrawer && inMapDrawer->IsDrawMode()) {
		inMapDrawer->MousePress(x, y, button);
		return;
	}

	// limited receivers for MMB
	if (button == SDL_BUTTON_MIDDLE) {
		if (!locked) {
			if (luaInputReceiver->MousePress(x, y, button)) {
				activeReceiver = luaInputReceiver;
				return;
			}
			if ((minimap != nullptr) && minimap->FullProxy()) {
				if (minimap->MousePress(x, y, button)) {
					activeReceiver = minimap;
					return;
				}
			}
		}
		return;
	}

	if (luaInputReceiver->MousePress(x, y, button)) {
		if (activeReceiver == nullptr)
			activeReceiver = luaInputReceiver;
		return;
	}

	if (game != nullptr && !game->hideInterface) {
		for (CInputReceiver* recv: CInputReceiver::GetReceivers()) {
			if (recv != nullptr && recv->MousePress(x, y, button)) {
				if (activeReceiver == nullptr)
					activeReceiver = recv;

				return;
			}
		}
	} else {
		if (guihandler != nullptr && guihandler->MousePress(x, y, button)) {
			if (activeReceiver == nullptr)
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
	float dist = CGround::LineGroundCol(pos1, pos1 + dir1 * globalRendering->viewRange * 1.4f, false);
	if(dist < 0) dist = globalRendering->viewRange * 1.4f;
	float3 gpos1 = pos1 + dir1 * dist;

	dist = CGround::LineGroundCol(pos2, pos2 + dir2 * globalRendering->viewRange * 1.4f, false);
	if(dist < 0) dist = globalRendering->viewRange * 1.4f;
	float3 gpos2 = pos2 + dir2 * dist;

	const float3 cdir1 = (gpos1 - camera->GetPos()).ANormalize();
	const float3 cdir2 = (gpos2 - camera->GetPos()).ANormalize();

	// prevent DivByZero
	float cdir1_fw = cdir1.dot(camera->GetDir()); if (cdir1_fw == 0.0f) cdir1_fw = 0.0001f;
	float cdir2_fw = cdir2.dot(camera->GetDir()); if (cdir2_fw == 0.0f) cdir2_fw = 0.0001f;

	// one corner of the rectangle
	topright.x = cdir1.dot(camera->GetRight()) / cdir1_fw;
	topright.y = cdir1.dot(camera->GetUp())    / cdir1_fw;

	// opposite corner
	btmleft.x = cdir2.dot(camera->GetRight()) / cdir2_fw;
	btmleft.y = cdir2.dot(camera->GetUp())    / cdir2_fw;

	// sort coeff so topright really is the topright corner
	if (topright.x < btmleft.x) std::swap(topright.x, btmleft.x);
	if (topright.y < btmleft.y) std::swap(topright.y, btmleft.y);
}


void CMouseHandler::MouseRelease(int x, int y, int button)
{
	const CUnit *_lastClicked = lastClicked;
	lastClicked = nullptr;

	if (button > NUM_BUTTONS)
		return;

	dir = hide ? camera->GetDir() : camera->CalcPixelDir(x, y);

	buttons[button].pressed = false;

	if (inMapDrawer && inMapDrawer->IsDrawMode()){
		inMapDrawer->MouseRelease(x, y, button);
		return;
	}

	if (activeReceiver != nullptr) {
		activeReceiver->MouseRelease(x, y, button);

		if (!buttons[SDL_BUTTON_LEFT].pressed && !buttons[SDL_BUTTON_MIDDLE].pressed && !buttons[SDL_BUTTON_RIGHT].pressed)
			activeReceiver = nullptr;

		return;
	}

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

	if (gu->fpsMode)
		return;
	// outside game, neither guiHandler nor quadField exist and TraceRay would crash
	if (guihandler == nullptr)
		return;

	if ((button == SDL_BUTTON_LEFT) && !buttons[button].chorded) {
		ButtonPressEvt& bp = buttons[SDL_BUTTON_LEFT];

		if (!KeyInput::GetKeyModState(KMOD_SHIFT) && !KeyInput::GetKeyModState(KMOD_CTRL))
			selectedUnitsHandler.ClearSelected();

		if (bp.movement > 4) {
			// select box
			float2 topright, btmleft;
			GetSelectionBoxCoeff(bp.camPos, bp.dir, camera->GetPos(), dir, topright, btmleft);

			// GetSelectionBoxCoeff returns us the corner pos, but we want to do a inview frustum check.
			// To do so we need the frustum planes (= plane normal + plane offset).
			float3 norm1 = camera->GetUp();
			float3 norm2 = -camera->GetUp();
			float3 norm3 = camera->GetRight();
			float3 norm4 = -camera->GetRight();

			#define signf(x) ((x>0.0f) ? 1.0f : -1)
			if (topright.y != 0) norm1 = (camera->GetDir() * signf(-topright.y)) + (camera->GetUp() / math::fabs(topright.y));
			if (btmleft.y  != 0) norm2 = (camera->GetDir() * signf(  btmleft.y)) - (camera->GetUp() / math::fabs(btmleft.y));
			if (topright.x != 0) norm3 = (camera->GetDir() * signf(-topright.x)) + (camera->GetRight() / math::fabs(topright.x));
			if (btmleft.x  != 0) norm4 = (camera->GetDir() * signf(  btmleft.x)) - (camera->GetRight() / math::fabs(btmleft.x));

			const float4 plane1(norm1, -(norm1.dot(camera->GetPos())));
			const float4 plane2(norm2, -(norm2.dot(camera->GetPos())));
			const float4 plane3(norm3, -(norm3.dot(camera->GetPos())));
			const float4 plane4(norm4, -(norm4.dot(camera->GetPos())));

			selectedUnitsHandler.HandleUnitBoxSelection(plane1, plane2, plane3, plane4);
		} else {
			const CUnit* unit = nullptr;
			const CFeature* feature = nullptr;

			TraceRay::GuiTraceRay(camera->GetPos(), dir, globalRendering->viewRange * 1.4f, NULL, unit, feature, false);
			lastClicked = unit;

			const bool selectType = (bp.lastRelease >= (gu->gameTime - doubleClickTime) && unit == _lastClicked);

			selectedUnitsHandler.HandleSingleUnitClickSelection(const_cast<CUnit*>(unit), true, selectType);
		}

		bp.lastRelease = gu->gameTime;
	}
}


void CMouseHandler::MouseWheel(float delta)
{
	if (eventHandler.MouseWheel(delta > 0.0f, delta))
		return;

	delta *= scrollWheelSpeed;

	if (camHandler != nullptr)
		camHandler->GetCurrentController().MouseWheelMove(delta);
}


void CMouseHandler::DrawSelectionBox()
{
	dir = hide ? camera->GetDir() : camera->CalcPixelDir(lastx, lasty);

	if (activeReceiver)
		return;

	if (gu->fpsMode)
		return;

	ButtonPressEvt& bp = buttons[SDL_BUTTON_LEFT];

	if (bp.pressed && !bp.chorded && (bp.movement > 4) &&
	   (!inMapDrawer || !inMapDrawer->IsDrawMode()))
	{
		float2 topright, btmleft;
		GetSelectionBoxCoeff(bp.camPos, bp.dir, camera->GetPos(), dir, topright, btmleft);

		float3 dir1S = camera->GetRight() * topright.x;
		float3 dir1U = camera->GetUp()    * topright.y;
		float3 dir2S = camera->GetRight() * btmleft.x;
		float3 dir2U = camera->GetUp()    * btmleft.y;

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
			camera->GetPos() + (dir1U + dir1S + camera->GetDir()) * 30,
			camera->GetPos() + (dir2U + dir1S + camera->GetDir()) * 30,
			camera->GetPos() + (dir2U + dir2S + camera->GetDir()) * 30,
			camera->GetPos() + (dir1U + dir2S + camera->GetDir()) * 30,
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
	if (!offscreen) {
		std::string s;

		if (luaInputReceiver->IsAbove(lastx, lasty)) {
			s = std::move(luaInputReceiver->GetTooltip(lastx, lasty));

			if (!s.empty())
				return s;
		}

		for (CInputReceiver* recv: CInputReceiver::GetReceivers()) {
			if (recv == nullptr)
				continue;
			if (!recv->IsAbove(lastx, lasty))
				continue;

			s = std::move(recv->GetTooltip(lastx, lasty));

			if (!s.empty())
				return s;
		}
	}

	// outside game, neither guiHandler nor quadField exist and TraceRay would crash
	if (guihandler == nullptr)
		return "";

	const std::string& buildTip = guihandler->GetBuildTooltip();

	if (!buildTip.empty())
		return buildTip;

	const float range = globalRendering->viewRange * 1.4f;
	float dist = 0.0f;

	const CUnit* unit = nullptr;
	const CFeature* feature = nullptr;

	{
		dist = TraceRay::GuiTraceRay(camera->GetPos(), dir, range, nullptr, unit, feature, true, false, true);

		if (unit    != nullptr) return CTooltipConsole::MakeUnitString(unit);
		if (feature != nullptr) return CTooltipConsole::MakeFeatureString(feature);
	}

	const string selTip = std::move(selectedUnitsHandler.GetTooltip());

	if (!selTip.empty())
		return selTip;

	if (dist <= range)
		return CTooltipConsole::MakeGroundString(camera->GetPos() + (dir * dist));

	return "";
}


void CMouseHandler::Update()
{
	SetCursor(newCursor);

	if (!hide)
		return;

	const int2 screenCenter = globalRendering->GetScreenCenter();

	// Update MiddleClickScrolling
	scrollx *= 0.5f;
	scrolly *= 0.5f;
	lastx = screenCenter.x;
	lasty = screenCenter.y;

	if (!globalRendering->active)
		return;

	mouseInput->SetPos(screenCenter);
}


void CMouseHandler::WarpMouse(int x, int y)
{
	if (locked)
		return;

	lastx = x + globalRendering->viewPosX;
	lasty = y + globalRendering->viewPosY;

	mouseInput->SetPos(int2(lastx, lasty));
}


void CMouseHandler::ShowMouse()
{
	if (!hide)
		return;

	hide = false;
	cursorText = "none"; // force hardware cursor rebinding (else we have standard b&w cursor)

	// don't use SDL_ShowCursor here, it would cause a flicker with hwCursor
	// (by switching between default cursor and later the real one, e.g. `attack`)
	// instead update state and cursor at the same time
	if (hardwareCursor) {
		hwHide = true;
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}


void CMouseHandler::HideMouse()
{
	if (hide)
		return;

	hwHide = true;
	SDL_ShowCursor(SDL_DISABLE);
	mouseInput->SetWMMouseCursor(nullptr);

	const int2 screenCenter = globalRendering->GetScreenCenter();

	scrollx = 0.0f;
	scrolly = 0.0f;
	lastx = screenCenter.x;
	lasty = screenCenter.y;

	mouseInput->SetPos(screenCenter);
	hide = true;
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
		mouseInput->SetWMMouseCursor(nullptr);
		SDL_ShowCursor(SDL_DISABLE);
	}
	cursorText = "none";
}


/******************************************************************************/

void CMouseHandler::ChangeCursor(const std::string& cmdName, const float scale)
{
	newCursor = cmdName;
	cursorScale = scale;
}


void CMouseHandler::SetCursor(const std::string& cmdName, const bool forceRebind)
{
	if ((cursorText == cmdName) && !forceRebind)
		return;

	cursorText = cmdName;
	const auto it = cursorCommandMap.find(cmdName);

	if (it != cursorCommandMap.end()) {
		activeCursorIdx = it->second;
	} else {
		activeCursorIdx = cursorCommandMap[""];
	}

	if (!hardwareCursor || hide)
		return;

	if (loadedCursors[activeCursorIdx].IsHWValid()) {
		hwHide = false;
		loadedCursors[activeCursorIdx].BindHwCursor(); // calls SDL_ShowCursor(SDL_ENABLE);
	} else {
		hwHide = true;
		SDL_ShowCursor(SDL_DISABLE);
		mouseInput->SetWMMouseCursor(nullptr);
	}
}


void CMouseHandler::UpdateCursors()
{
	// we update all cursors (for the command queue icons)
	for (auto it = cursorFileMap.begin(); it != cursorFileMap.end(); ++it) {
		loadedCursors[it->second].Update();
	}
}

void CMouseHandler::DrawScrollCursor()
{
	const float scaleL = math::fabs(std::min(0.0f,scrollx)) * crossMoveScale + 1.0f;
	const float scaleT = math::fabs(std::min(0.0f,scrolly)) * crossMoveScale + 1.0f;
	const float scaleR = math::fabs(std::max(0.0f,scrollx)) * crossMoveScale + 1.0f;
	const float scaleB = math::fabs(std::max(0.0f,scrolly)) * crossMoveScale + 1.0f;

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

	WorkaroundATIPointSizeBug();
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

	const float wingHalf = math::PI / 9.0f;
	const int stepNumHalf = 2;
	const float step = wingHalf / stepNumHalf;

	glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

		for (float angle = 0.0f; angle < math::TWOPI; angle += math::TWOPI / 3.f) {
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
	assert(activeCursorIdx != -1);

	if (guihandler != nullptr)
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

	if (hardwareCursor && loadedCursors[activeCursorIdx].IsHWValid())
		return;

	// draw the 'software' cursor
	if (cursorScale >= 0.0f) {
		loadedCursors[activeCursorIdx].Draw(lastx, lasty, cursorScale);
		return;
	}

	// hovered minimap, show default cursor and draw `special` cursor scaled-down bottom right of the default one
	const size_t normalCursorIndex = cursorFileMap["cursornormal"];

	if (normalCursorIndex == 0) {
		loadedCursors[activeCursorIdx].Draw(lastx, lasty, -cursorScale);
		return;
	}

	CMouseCursor& normalCursor = loadedCursors[normalCursorIndex];
	normalCursor.Draw(lastx, lasty, 1.0f);

	if (activeCursorIdx == normalCursorIndex)
		return;

	loadedCursors[activeCursorIdx].Draw(lastx + normalCursor.GetMaxSizeX(), lasty + normalCursor.GetMaxSizeY(), -cursorScale);
}


bool CMouseHandler::AssignMouseCursor(
	const std::string& cmdName,
	const std::string& fileName,
	CMouseCursor::HotSpot hotSpot,
	bool overwrite
) {
	const auto cmdIt = cursorCommandMap.find(cmdName);
	const auto fileIt = cursorFileMap.find(fileName);

	const bool haveCmd = (cmdIt != cursorCommandMap.end());
	const bool haveFile = (fileIt != cursorFileMap.end());

	if (haveCmd && !overwrite)
		return false; // already assigned

	if (haveFile) {
		// cursor is already loaded, reuse it
		cursorCommandMap[cmdName] = fileIt->second;
		return true;
	}

	CMouseCursor newCursor = std::move(CMouseCursor::New(fileName, hotSpot));

	if (!newCursor.IsValid())
		return false;

	const int commandCursorIdx = haveCmd? cmdIt->second: -1;

	// assign the new cursor and remap indices
	loadedCursors.emplace_back();
	loadedCursors.back() = std::move(newCursor);

	cursorFileMap[fileName] = loadedCursors.size() - 1;
	cursorCommandMap[cmdName] = loadedCursors.size() - 1;

	// switch to the null-cursor if our active one is being (re)assigned
	if (activeCursorIdx == commandCursorIdx)
		SetCursor("none", true);

	return true;
}

bool CMouseHandler::ReplaceMouseCursor(
	const string& oldName,
	const string& newName,
	CMouseCursor::HotSpot hotSpot
) {
	const auto fileIt = cursorFileMap.find(oldName);

	if (fileIt == cursorFileMap.end())
		return false;

	CMouseCursor newCursor = std::move(CMouseCursor::New(newName, hotSpot));

	if (!newCursor.IsValid())
		return false; // leave the old one

	loadedCursors[fileIt->second] = std::move(newCursor);

	// update current cursor if necessary
	if (activeCursorIdx == fileIt->second)
		SetCursor(cursorText, true);

	return true;
}


/******************************************************************************/

void CMouseHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	dragScrollThreshold = configHandler->GetFloat("MouseDragScrollThreshold");
	scrollWheelSpeed = configHandler->GetFloat("ScrollWheelSpeed");
}
