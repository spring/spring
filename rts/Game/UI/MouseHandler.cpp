#include "StdAfx.h"
// MouseHandler.cpp: implementation of the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "mmgr.h"

#include "MouseHandler.h"
#include "Game/CameraHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "CommandColors.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseCursor.h"
#include "MouseInput.h"
#include "TooltipConsole.h"
#include "Sim/Units/Groups/Group.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/PlayerHandler.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Lua/LuaInputReceiver.h"
#include "ConfigHandler.h"
#include "Platform/errorhandler.h"
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
#include "Sim/Units/UnitTracker.h"
#include "EventHandler.h"
#include "Sound/Sound.h"
#include "Sound/AudioChannel.h"
#include <boost/cstdint.hpp>

// can't be up there since those contain conflicting definitions
#include <SDL_mouse.h>
#include <SDL_events.h>
#include <SDL_keysym.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern bool	fullscreen;
extern boost::uint8_t *keys;


CMouseHandler* mouse = NULL;

static CInputReceiver*& activeReceiver = CInputReceiver::GetActiveReceiverRef();


CMouseHandler::CMouseHandler()
: locked(false)
{
	lastx = 300;
	lasty = 200;
	hide = true;
	hwHide = true;
	currentCursor = NULL;

	for (int a = 1; a <= NUM_BUTTONS; a++) {
		buttons[a].pressed = false;
		buttons[a].lastRelease = -20;
		buttons[a].movement = 0;
	}

	cursorScale = 1.0f;

	LoadCursors();

	// hide the cursor until we are ingame (not in loading screen etc.)
	SDL_ShowCursor(SDL_DISABLE);

#ifdef __APPLE__
	hardwareCursor = false;
#else
	hardwareCursor = !!configHandler->Get("HardwareCursor", 0);
#endif

	soundMultiselID = sound->GetSoundId("MultiSelect", false);

	invertMouse = !!configHandler->Get("InvertMouse",0);
	doubleClickTime = configHandler->Get("DoubleClickTime", 200.0f) / 1000.0f;

	scrollWheelSpeed = configHandler->Get("ScrollWheelSpeed", 25.0f);
	scrollWheelSpeed = std::max(-255.0f, std::min(255.0f, scrollWheelSpeed));
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
		handleerror(0,
			"Unable to load default cursor. Check that you have the OTA\n"
			"content package (\"otacontent.sdz\") installed in your Spring\n"
			"\"base/\" directory.",
			"Missing Dependency: \"cursornormal\"", 0);
	}
}


/******************************************************************************/

void CMouseHandler::MouseMove(int x, int y)
{
	if(hide) {
		lastx = x;
		lasty = y;
		return;
	}

	const int dx = x - lastx;
	const int dy = y - lasty;
	lastx = x;
	lasty = y;

	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);

	buttons[SDL_BUTTON_LEFT].movement += abs(dx) + abs(dy);
	buttons[SDL_BUTTON_RIGHT].movement += abs(dx) + abs(dy);

	if (!game->gameOver) {
		playerHandler->Player(gu->myPlayerNum)->currentStats.mousePixels+=abs(dx)+abs(dy);
	}

	if(activeReceiver){
		activeReceiver->MouseMove(x, y, dx, dy, activeButton);
	}

	if(inMapDrawer && inMapDrawer->keyPressed){
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

	if (button == 4) {
		if (guihandler->buildSpacing > 0)
			guihandler->buildSpacing--;
		return;
	}
	if (button == 5) {
		guihandler->buildSpacing++;
		return;
	}

 	buttons[button].chorded = buttons[SDL_BUTTON_LEFT].pressed ||
 	                          buttons[SDL_BUTTON_RIGHT].pressed;
	buttons[button].pressed = true;
	buttons[button].time = gu->gameTime;
	buttons[button].x = x;
	buttons[button].y = y;
	buttons[button].camPos = camera->pos;
	buttons[button].dir = hide ? camera->forward : camera->CalcPixelDir(x,y);
	buttons[button].movement = 0;

	activeButton = button;

	if (activeReceiver) {
		activeReceiver->MousePress(x, y, button);
		return;
	}

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
			if (recv && recv->MousePress(x, y, button)) {
				activeReceiver = recv;
				return;
			}
		}
	} else {
		if (luaInputReceiver && luaInputReceiver->MousePress(x, y, button)) {
			activeReceiver = luaInputReceiver;
			return;
		}
		if (guihandler && guihandler->MousePress(x,y,button)) {
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
		int x, y;
		const boost::uint8_t buttons = SDL_GetMouseState(&x, &y);
		const boost::uint8_t mask = (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK);
		if ((buttons & mask) == 0) {
			activeReceiver = NULL;
		}
		return;
	}

	// Switch camera mode on a middle click that wasn't a middle mouse drag scroll.
	// (the latter is determined by the time the mouse was held down:
	//  <= 0.3 s means a camera mode switch, > 0.3 s means a drag scroll)
	if (button == SDL_BUTTON_MIDDLE) {
		if (buttons[SDL_BUTTON_MIDDLE].time > (gu->gameTime - 0.3f)) {
			if (keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]) {
				camHandler->ToggleState();
			} else {
				ToggleState();
			}
		}
		return;
	}

	if (gu->directControl) {
		return;
	}

	if ((button == SDL_BUTTON_LEFT) && !buttons[button].chorded) {
		if (!keys[SDLK_LSHIFT] && !keys[SDLK_LCTRL]) {
			selectedUnits.ClearSelected();
		}

		if (buttons[SDL_BUTTON_LEFT].movement > 4) {
			// select box
			float dist=ground->LineGroundCol(buttons[SDL_BUTTON_LEFT].camPos,buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4f);
			if(dist<0)
				dist=gu->viewRange*1.4f;
			float3 pos1=buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*dist;

			dist=ground->LineGroundCol(camera->pos,camera->pos+dir*gu->viewRange*1.4f);
			if(dist<0)
				dist=gu->viewRange*1.4f;
			float3 pos2=camera->pos+dir*dist;

			float3 dir1=pos1-camera->pos;
			dir1.ANormalize();
			float3 dir2=pos2-camera->pos;
			dir2.ANormalize();

			float rl1=(dir1.dot(camera->right))/dir1.dot(camera->forward);
			float ul1=(dir1.dot(camera->up))/dir1.dot(camera->forward);

			float rl2=(dir2.dot(camera->right))/dir2.dot(camera->forward);
			float ul2=(dir2.dot(camera->up))/dir2.dot(camera->forward);

			if(rl1<rl2){
				float t=rl1;
				rl1=rl2;
				rl2=t;
			}
			if(ul1<ul2){
				float t=ul1;
				ul1=ul2;
				ul2=t;
			}

			float3 norm1,norm2,norm3,norm4;

			if(ul1>0)
				norm1=-camera->forward+camera->up/fabs(ul1);
			else if(ul1<0)
				norm1=camera->forward+camera->up/fabs(ul1);
			else
				norm1=camera->up;

			if(ul2>0)
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
					if(vec.dot(norm1)<0 && vec.dot(norm2)<0 && vec.dot(norm3)<0 && vec.dot(norm4)<0){
						if (keys[SDLK_LCTRL] && selectedUnits.selectedUnits.find(*ui) != selectedUnits.selectedUnits.end()) {
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
			CUnit* unit;
			helper->GuiTraceRay(camera->pos,dir,gu->viewRange*1.4f,unit,false);
			if(unit && ((unit->team == gu->myTeam) || gu->spectatingFullSelect)){
				if(buttons[button].lastRelease < (gu->gameTime - doubleClickTime)){
					if (keys[SDLK_LCTRL] && selectedUnits.selectedUnits.find(unit) != selectedUnits.selectedUnits.end()) {
						selectedUnits.RemoveUnit(unit);
					} else {
						selectedUnits.AddUnit(unit);
					}
				}
				else {
					//double click
					if (unit->group && !keys[SDLK_LCTRL]) {
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
								if (((*ui)->aihint == unit->aihint) &&
										(camera->InView((*ui)->midPos) || keys[SDLK_LCTRL])) {
									selectedUnits.AddUnit(*ui);
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


void CMouseHandler::Draw()
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
		                                 + buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4f);
		if(dist<0)
			dist=gu->viewRange*1.4f;
		float3 pos1=buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*dist;

		dist=ground->LineGroundCol(camera->pos,camera->pos+dir*gu->viewRange*1.4f);
		if(dist<0)
			dist=gu->viewRange*1.4f;
		float3 pos2=camera->pos+dir*dist;

		float3 dir1=pos1-camera->pos;
		dir1.Normalize();
		float3 dir2=pos2-camera->pos;
		dir2.Normalize();

		float3 dir1S=camera->right*(dir1.dot(camera->right))/dir1.dot(camera->forward);
		float3 dir1U=camera->up*(dir1.dot(camera->up))/dir1.dot(camera->forward);

		float3 dir2S=camera->right*(dir2.dot(camera->right))/dir2.dot(camera->forward);
		float3 dir2U=camera->up*(dir2.dot(camera->up))/dir2.dot(camera->forward);

		glColor4fv(cmdColors.mouseBox);

		glPushAttrib(GL_ENABLE_BIT);

		glDisable(GL_FOG);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		            (GLenum)cmdColors.MouseBoxBlendDst());

		glLineWidth(cmdColors.MouseBoxLineWidth());
		glBegin(GL_LINE_LOOP);
		glVertexf3(camera->pos+dir1U*30+dir1S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir1S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir2S*30+camera->forward*30);
		glVertexf3(camera->pos+dir1U*30+dir2S*30+camera->forward*30);
		glEnd();
		glLineWidth(1.0f);

		glPopAttrib();
	}
}


void CMouseHandler::WarpMouse(int x, int y)
{
	if (!locked) {
		lastx = x + gu->viewPosX;
		lasty = y + gu->viewPosY;
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

	const float range = (gu->viewRange * 1.4f);
	CUnit* unit = NULL;
	float udist = helper->GuiTraceRay(camera->pos, dir, range, unit, true);
	CFeature* feature = NULL;
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


void CMouseHandler::EmptyMsgQueUpdate(void)
{
	if (!hide) {
		return;
	}

	int dx = lastx - (gu->viewSizeX / 2 + gu->viewPosX);
	int dy = lasty - (gu->viewSizeY / 2 + gu->viewPosY);
	lastx = gu->viewSizeX / 2 + gu->viewPosX;
	lasty = gu->viewSizeY / 2 + gu->viewPosY;

	float3 move;
	move.x = dx;
	move.y = dy;
	move.z = invertMouse? -1.0f : 1.0f;
	camHandler->GetCurrentController().MouseMove(move);

	if (gu->active) {
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
		lastx = gu->viewSizeX / 2 + gu->viewPosX;
		lasty = gu->viewSizeY / 2 + gu->viewPosY;
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


void CMouseHandler::DrawCursor(void)
{
	if (guihandler)
		guihandler->DrawCentroidCursor();

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
