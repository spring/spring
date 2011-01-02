/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include <algorithm>
#include <boost/cstdint.hpp>

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
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/PlayerHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/UI/UnitTracker.h"
#include "Lua/LuaInputReceiver.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"
#include "System/Input/MouseInput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/IEffectChannel.h"

// can't be up there since those contain conflicting definitions
#include <SDL_mouse.h>
#include <SDL_events.h>
#include <SDL_keysym.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMouseHandler* mouse = NULL;

static CInputReceiver*& activeReceiver = CInputReceiver::GetActiveReceiverRef();


CMouseHandler::CMouseHandler()
	: hardwareCursor(false)
	, lastx(300)
	, lasty(200)
	, hide(true)
	, hwHide(true)
	, locked(false)
	, invertMouse(false)
	, doubleClickTime(0.0f)
	, scrollWheelSpeed(0.0f)
	, dragScrollThreshold(0.0f)
	, crossSize(0.0f)
	, crossAlpha(0.0f)
	, crossMoveScale(0.0f)
	, activeButton(-1)
	, dir(ZeroVector)
	, soundMultiselID(0)
	, cursorScale(1.0f)
	, cursorText("")
	, currentCursor(NULL)
	, wasLocked(false)
	, scrollx(0.0f)
	, scrolly(0.0f)
{
	for (int a = 1; a <= NUM_BUTTONS; a++) {
		buttons[a].pressed = false;
		buttons[a].lastRelease = -20;
		buttons[a].movement = 0;
	}

	LoadCursors();

#ifndef __APPLE__
	hardwareCursor = !!configHandler->Get("HardwareCursor", 0);
#endif

	soundMultiselID = sound->GetSoundId("MultiSelect", false);

	invertMouse = !!configHandler->Get("InvertMouse",0);
	doubleClickTime = configHandler->Get("DoubleClickTime", 200.0f) / 1000.0f;

	scrollWheelSpeed = configHandler->Get("ScrollWheelSpeed", 25.0f);
	scrollWheelSpeed = Clamp(scrollWheelSpeed,-255.f,255.f);

	crossSize      = configHandler->Get("CrossSize", 12.0f);
	crossAlpha     = configHandler->Get("CrossAlpha", 0.5f);
	crossMoveScale = configHandler->Get("CrossMoveScale", 1.0f) * 0.005f;

	dragScrollThreshold = configHandler->Get("MouseDragScrollThreshold", 0.3f);

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
	AssignMouseCursor("DGun",         "cursordgun",       mCenter,  false);
	AssignMouseCursor("DGun",         "cursorattack",     mCenter,  false); // backup
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
	lastx = x;
	lasty = y;

	const int screenCenterX = globalRendering->viewSizeX / 2 + globalRendering->viewPosX;
	const int screenCenterY = globalRendering->viewSizeY / 2 + globalRendering->viewPosY;

	scrollx += lastx - screenCenterX;
	scrolly += lasty - screenCenterY;

	dir = hide ? camera->forward : camera->CalcPixelDir(x,y);

	if (hide) {
		camHandler->GetCurrentController().MouseMove(float3(dx, dy, invertMouse ? -1.0f : 1.0f));
		return;
	}

	buttons[SDL_BUTTON_LEFT].movement  += (int)sqrt(float(dx*dx + dy*dy));
	buttons[SDL_BUTTON_RIGHT].movement += (int)sqrt(float(dx*dx + dy*dy));

	if (!game->gameOver) {
		playerHandler->Player(gu->myPlayerNum)->currentStats.mousePixels += (int)sqrt(float(dx*dx + dy*dy));
	}

	if (activeReceiver){
		activeReceiver->MouseMove(x, y, dx, dy, activeButton);
	}

	if (inMapDrawer && inMapDrawer->keyPressed){
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

	dir = hide? camera->forward: camera->CalcPixelDir(x, y);

	if (!game->gameOver)
		playerHandler->Player(gu->myPlayerNum)->currentStats.mouseClicks++;

 	buttons[button].chorded = buttons[SDL_BUTTON_LEFT].pressed ||
 	                          buttons[SDL_BUTTON_RIGHT].pressed;
	buttons[button].pressed = true;
	buttons[button].time = gu->gameTime;
	buttons[button].x = x;
	buttons[button].y = y;
	buttons[button].camPos = camera->pos;
	buttons[button].dir = dir;
	buttons[button].movement = 0;

	activeButton = button;

	if (activeReceiver && activeReceiver->MousePress(x, y, button))
		return;

	if(inMapDrawer &&  inMapDrawer->keyPressed){
		inMapDrawer->MousePress(x, y, button);
		return;
	}

	// limited receivers for MMB
	if (button == SDL_BUTTON_MIDDLE){
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


void CMouseHandler::MouseRelease(int x, int y, int button)
{
	GML_RECMUTEX_LOCK(sel); // MouseRelease

	if (button > NUM_BUTTONS)
		return;

	camHandler->GetCurrentController().MouseRelease(x, y, button);

	dir = hide ? camera->forward: camera->CalcPixelDir(x, y);
	buttons[button].pressed = false;

	if (inMapDrawer && inMapDrawer->keyPressed){
		inMapDrawer->MouseRelease(x, y, button);
		return;
	}

	if (activeReceiver) {
		activeReceiver->MouseRelease(x, y, button);
		if(!buttons[SDL_BUTTON_LEFT].pressed && !buttons[SDL_BUTTON_MIDDLE].pressed && !buttons[SDL_BUTTON_RIGHT].pressed)
			activeReceiver = NULL;
		return;
	}

	//! Switch camera mode on a middle click that wasn't a middle mouse drag scroll.
	//! the latter is determined by the time the mouse was held down:
	//! switch (dragScrollThreshold)
	//!   <= means a camera mode switch
	//!    > means a drag scroll
	if (button == SDL_BUTTON_MIDDLE) {
		if (buttons[SDL_BUTTON_MIDDLE].time > (gu->gameTime - dragScrollThreshold))
			ToggleState();
		return;
	}

	if (gu->directControl) {
		return;
	}

	if ((button == SDL_BUTTON_LEFT) && !buttons[button].chorded) {
		if (!keyInput->IsKeyPressed(SDLK_LSHIFT) && !keyInput->IsKeyPressed(SDLK_LCTRL)) {
			selectedUnits.ClearSelected();
		}

		if (buttons[SDL_BUTTON_LEFT].movement > 4) {
			// select box
			float dist=ground->LineGroundCol(buttons[SDL_BUTTON_LEFT].camPos,buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*globalRendering->viewRange*1.4f);
			if(dist<0)
				dist=globalRendering->viewRange*1.4f;
			float3 pos1=buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*dist;

			dist=ground->LineGroundCol(camera->pos,camera->pos+dir*globalRendering->viewRange*1.4f);
			if(dist<0)
				dist=globalRendering->viewRange*1.4f;
			float3 pos2=camera->pos+dir*dist;

			float3 dir1 = pos1 - camera->pos;
			dir1.ANormalize();
			float3 dir2 = pos2 - camera->pos;
			dir2.ANormalize();

			float rl1 = dir1.dot(camera->right) / dir1.dot(camera->forward);
			float ul1 = dir1.dot(camera->up) / dir1.dot(camera->forward);

			float rl2 = dir2.dot(camera->right) / dir2.dot(camera->forward);
			float ul2 = dir2.dot(camera->up) / dir2.dot(camera->forward);

			if (rl1<rl2)
				std::swap(rl1, rl2);
			if (ul1<ul2)
				std::swap(ul1, ul2);

			float3 norm1,norm2,norm3,norm4;

			if (ul1>0)
				norm1=-camera->forward+camera->up/fabs(ul1);
			else if(ul1<0)
				norm1=camera->forward+camera->up/fabs(ul1);
			else
				norm1=camera->up;

			if (ul2>0)
				norm2=camera->forward-camera->up/fabs(ul2);
			else if(ul2<0)
				norm2=-camera->forward-camera->up/fabs(ul2);
			else
				norm2=-camera->up;

			if(rl1>0)
				norm3=-camera->forward+camera->right/fabs(rl1);
			else if(rl1<0)
				norm3=camera->forward+camera->right/fabs(rl1);
			else
				norm3=camera->right;

			if(rl2>0)
				norm4=camera->forward-camera->right/fabs(rl2);
			else if(rl2<0)
				norm4=-camera->forward-camera->right/fabs(rl2);
			else
				norm4=-camera->right;

			CUnitSet::iterator ui;
			CUnit* unit = NULL;
			int addedunits=0;
			int team, lastTeam;
			if (gu->spectatingFullSelect) {
				team = 0;
				lastTeam = teamHandler->ActiveTeams() - 1;
			} else {
				team = gu->myTeam;
				lastTeam = gu->myTeam;
			}
			for (; team <= lastTeam; team++) {
				for(ui=teamHandler->Team(team)->units.begin();ui!=teamHandler->Team(team)->units.end();++ui){
					float3 vec=(*ui)->midPos-camera->pos;
					if (vec.dot(norm1) < 0.0f && vec.dot(norm2) < 0.0f && vec.dot(norm3) < 0.0f && vec.dot(norm4) < 0.0f) {
						if (keyInput->IsKeyPressed(SDLK_LCTRL) && selectedUnits.selectedUnits.find(*ui) != selectedUnits.selectedUnits.end()) {
							selectedUnits.RemoveUnit(*ui);
						} else {
							selectedUnits.AddUnit(*ui);
							unit = *ui;
							addedunits++;
						}
					}
				}
			}
			if (addedunits == 1) {
				int soundIdx = unit->unitDef->sounds.select.getRandomIdx();
				if (soundIdx >= 0) {
					Channels::UnitReply.PlaySample(
						unit->unitDef->sounds.select.getID(soundIdx), unit,
						unit->unitDef->sounds.select.getVolume(soundIdx));
				}
			}
			else if(addedunits) //more than one unit selected
				Channels::UserInterface.PlaySample(soundMultiselID);
		} else {
			const CUnit* unit;
			helper->GuiTraceRay(camera->pos,dir,globalRendering->viewRange*1.4f,unit,false);
			if (unit && ((unit->team == gu->myTeam) || gu->spectatingFullSelect)) {
				if (buttons[button].lastRelease < (gu->gameTime - doubleClickTime)) {
					CUnit* unitM = uh->units[unit->id];
					if (keyInput->IsKeyPressed(SDLK_LCTRL) && selectedUnits.selectedUnits.find((CUnit*)unit) != selectedUnits.selectedUnits.end()) {
						selectedUnits.RemoveUnit(unitM);
					} else {
						selectedUnits.AddUnit(unitM);
					}
				} else {
					//double click
					if (unit->group && !keyInput->IsKeyPressed(SDLK_LCTRL)) {
						//select the current unit's group if it has one
						selectedUnits.SelectGroup(unit->group->id);
					} else {
						//select all units of same type (on screen, unless CTRL is pressed)
						int team, lastTeam;
						if (gu->spectatingFullSelect) {
							team = 0;
							lastTeam = teamHandler->ActiveTeams() - 1;
						} else {
							team = gu->myTeam;
							lastTeam = gu->myTeam;
						}
						for (; team <= lastTeam; team++) {
							CUnitSet::iterator ui;
							CUnitSet& teamUnits = teamHandler->Team(team)->units;
							for (ui = teamUnits.begin(); ui != teamUnits.end(); ++ui) {
								if ((*ui)->unitDef->id == unit->unitDef->id) {
									if (camera->InView((*ui)->midPos) || keyInput->IsKeyPressed(SDLK_LCTRL)) {
										selectedUnits.AddUnit(*ui);
									}
								}
							}
						}
					}
				}
				buttons[button].lastRelease=gu->gameTime;

				int soundIdx = unit->unitDef->sounds.select.getRandomIdx();
				if (soundIdx >= 0) {
					Channels::UnitReply.PlaySample(
						unit->unitDef->sounds.select.getID(soundIdx), unit,
						unit->unitDef->sounds.select.getVolume(soundIdx));
				}
			}
		}
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

	if (gu->directControl) {
		return;
	}
	if (buttons[SDL_BUTTON_LEFT].pressed && !buttons[SDL_BUTTON_LEFT].chorded &&
	   (buttons[SDL_BUTTON_LEFT].movement > 4) &&
	   (!inMapDrawer || !inMapDrawer->keyPressed)) {

		float dist=ground->LineGroundCol(buttons[SDL_BUTTON_LEFT].camPos,
		                                 buttons[SDL_BUTTON_LEFT].camPos
		                                 + buttons[SDL_BUTTON_LEFT].dir*globalRendering->viewRange*1.4f);
		if(dist<0)
			dist=globalRendering->viewRange*1.4f;
		float3 pos1=buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*dist;

		dist=ground->LineGroundCol(camera->pos,camera->pos+dir*globalRendering->viewRange*1.4f);
		if(dist<0)
			dist=globalRendering->viewRange*1.4f;
		float3 pos2=camera->pos+dir*dist;

		float3 dir1=pos1-camera->pos;
		dir1.Normalize();
		float3 dir2=pos2-camera->pos;
		dir2.Normalize();

		float3 dir1S = camera->right * dir1.dot(camera->right) / dir1.dot(camera->forward);
		float3 dir1U = camera->up * dir1.dot(camera->up) / dir1.dot(camera->forward);

		float3 dir2S = camera->right * dir2.dot(camera->right) / dir2.dot(camera->forward);
		float3 dir2U = camera->up * dir2.dot(camera->up) / dir2.dot(camera->forward);

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
			camera->pos+dir1U*30+dir1S*30+camera->forward*30,
			camera->pos+dir2U*30+dir1S*30+camera->forward*30,
			camera->pos+dir2U*30+dir2S*30+camera->forward*30,
			camera->pos+dir1U*30+dir2S*30+camera->forward*30,
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);

		glLineWidth(1.0f);

		glPopAttrib();
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


// CALLINFO:
// LuaUnsyncedRead::GetCurrentTooltip
// CTooltipConsole::Draw --> CMouseHandler::GetCurrentTooltip
std::string CMouseHandler::GetCurrentTooltip(void)
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

	GML_RECMUTEX_LOCK(sel); // GetCurrentTooltip - anti deadlock
	GML_RECMUTEX_LOCK(quad); // GetCurrentTooltip - called from ToolTipConsole::Draw --> MouseHandler::GetCurrentTooltip

	const float range = (globalRendering->viewRange * 1.4f);
	const CUnit* unit = NULL;
	float udist = helper->GuiTraceRay(camera->pos, dir, range, unit, true);
	const CFeature* feature = NULL;
	float fdist = helper->GuiTraceRayFeature(camera->pos, dir, range, feature);

	if ((udist > (range - 300.0f)) &&
	    (fdist > (range - 300.0f)) && (unit == NULL)) {
		return "";
	}

	if (udist > fdist) {
		unit = NULL;
	} else {
		feature = NULL;
	}

	if (unit) {
		return CTooltipConsole::MakeUnitString(unit);
	}

	if (feature) {
		return CTooltipConsole::MakeFeatureString(feature);
	}

	const string selTip = selectedUnits.GetTooltip();
	if (selTip != "") {
		return selTip;
	}

	if (udist < (range - 100.0f)) {
		const float3 pos = camera->pos + (dir * udist);
		return CTooltipConsole::MakeGroundString(pos);
	}

	return "";
}


void CMouseHandler::EmptyMsgQueUpdate()
{
	if (!hide) {
		return;
	}

	scrollx *= 0.5f;
	scrolly *= 0.5f;

	lastx = globalRendering->viewSizeX / 2 + globalRendering->viewPosX;
	lasty = globalRendering->viewSizeY / 2 + globalRendering->viewPosY;

	if (globalRendering->active) {
		mouseInput->SetPos(int2(lastx, lasty));
	}
}


void CMouseHandler::ShowMouse()
{
	if(hide){
		// I don't use SDL_ShowCursor here 'cos it would cause a flicker (with hwCursor)
		// instead update state and cursor at the same time
		if (hardwareCursor){
			hwHide=true; //call SDL_ShowCursor(SDL_ENABLE) later!
		}else{
			SDL_ShowCursor(SDL_DISABLE);
		}
		cursorText=""; //force hardware cursor rebinding (else we have standard b&w cursor)
		hide=false;
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


void CMouseHandler::ToggleState()
{
	if (locked) {
		locked = false;
		ShowMouse();
	} else {
		locked = true;
		HideMouse();
	}
}


void CMouseHandler::UpdateHwCursor()
{
	if (hardwareCursor){
		hwHide=true; //call SDL_ShowCursor(SDL_ENABLE) later!
	}else{
		mouseInput->SetWMMouseCursor(NULL);
		SDL_ShowCursor(SDL_DISABLE);
	}
	cursorText = "";
}


/******************************************************************************/

void CMouseHandler::SetCursor(const std::string& cmdName)
{
	if (cursorText.compare(cmdName) == 0) {
		return;
	}

	cursorText = cmdName;
	map<string, CMouseCursor*>::iterator it = cursorCommandMap.find(cmdName);
	if (it != cursorCommandMap.end()) {
		currentCursor = it->second;
	} else {
		currentCursor = cursorFileMap["cursornormal"];
	}

	if (hardwareCursor && !hide && currentCursor) {
		if (currentCursor->hwValid) {
			if (hwHide) {
				SDL_ShowCursor(SDL_ENABLE);
				hwHide = false;
			}
			currentCursor->BindHwCursor();
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
	if (guihandler)
		guihandler->DrawCentroidCursor();

	if (locked) {
		if (crossSize > 0.0f) {
			const float xscale = (crossSize / globalRendering->viewSizeX);
			const float yscale = (crossSize / globalRendering->viewSizeY);

			glPushMatrix();
			glTranslatef(0.5f - globalRendering->pixelX * 0.5f, 0.5f - globalRendering->pixelY * 0.5f, 0.f);
			glScalef(xscale, yscale, 1.f);

			if (gu->directControl) {
				DrawFPSCursor();
			} else {
				DrawScrollCursor();
			}

			glPopMatrix();
		}

		glEnable(GL_TEXTURE_2D);
		return;
	}

	if (hide || (cursorText == "none"))
		return;

	if (!currentCursor || (hardwareCursor && currentCursor->hwValid)) {
		return;
	}

	// draw the 'software' cursor
	if (cursorScale >= 0.0f) {
		currentCursor->Draw(lastx, lasty, cursorScale);
	}
	else {
		//! hovered minimap, show default cursor and draw `special` cursor scaled-down bottom right of the default one
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
	cmdIt = cursorCommandMap.find(cmdName);
	const bool haveCmd  = (cmdIt  != cursorCommandMap.end());

	std::map<std::string, CMouseCursor*>::iterator fileIt;
	fileIt = cursorFileMap.find(fileName);
	const bool haveFile = (fileIt != cursorFileMap.end());

	if (haveCmd && !overwrite) {
		return false; // already assigned
	}

	if (haveFile) {
		cursorCommandMap[cmdName] = fileIt->second;
		return true;
	}

	CMouseCursor* oldCursor = haveCmd ? cmdIt->second : NULL;

	CMouseCursor* cursor = CMouseCursor::New(fileName, hotSpot);
	if (cursor == NULL) {
		return false; // invalid cursor
	}
	cursorFileMap[fileName] = cursor;

	// assign the new cursor
	cursorCommandMap[cmdName] = cursor;

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

	delete oldCursor;

	if (currentCursor == oldCursor) {
		currentCursor = newCursor;
	}

	return true;
}


void CMouseHandler::SafeDeleteCursor(CMouseCursor* cursor)
{
	std::map<std::string, CMouseCursor*>::iterator it;

	for (it = cursorCommandMap.begin(); it != cursorCommandMap.end(); ++it) {
		if (it->second == cursor) {
			return; // being used
		}
	}

	for (it = cursorFileMap.begin(); it != cursorFileMap.end(); ++it) {
		if (it->second == cursor) {
			cursorFileMap.erase(it);
			delete cursor;
			return;
		}
	}

	if (currentCursor == cursor) {
		currentCursor = NULL;
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
