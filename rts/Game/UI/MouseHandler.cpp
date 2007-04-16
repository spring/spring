#include "StdAfx.h"
// MouseHandler.cpp: implementation of the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <SDL_types.h>
#include <SDL_mouse.h>
#include <SDL_keysym.h>
#include <SDL_events.h>
#include "MouseHandler.h"
#include "CommandColors.h"
#include "InfoConsole.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseCursor.h"
#include "MouseInput.h"
#include "TooltipConsole.h"
#include "ExternalAI/Group.h"
#include "Game/CameraController.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Platform/ConfigHandler.h"
#include "Platform/errorhandler.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/FeatureDef.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTracker.h"
#include "Sound.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern bool	fullscreen;
extern Uint8 *keys;

bool mouseHandlerMayDoSelection=true;

CMouseHandler* mouse=0;


CMouseHandler::CMouseHandler()
: locked(false),
	activeReceiver(0),
	cameraTime(0.0f),
	cameraTimeLeft(0.0f),
	preControlCamNum(0),
	preOverviewCamNum(0)
{
	lastx=300;
	lasty=200;
	hide=false;

	for(int a=1;a<=NUM_BUTTONS;a++){
		buttons[a].pressed=false;
		buttons[a].lastRelease=-20;
		buttons[a].movement=0;
	}

	cursorScale = 1.0f;

	LoadCursors();

	SDL_ShowCursor(SDL_DISABLE);

	soundMultiselID = sound->GetWaveId("sounds/button9.wav");

	invertMouse=!!configHandler.GetInt("InvertMouse",1);
	doubleClickTime = (float)configHandler.GetInt("DoubleClickTime", 200) / 1000.0f;

	//fps camera must always be the first one in the list
	std::vector<CCameraController*>& camCtrls = camControllers;
	camCtrls.push_back(SAFE_NEW CFPSController         (camCtrls.size())); // 0
	camCtrls.push_back(SAFE_NEW COverheadController    (camCtrls.size())); // 1
	camCtrls.push_back(SAFE_NEW CTWController          (camCtrls.size())); // 2
	camCtrls.push_back(SAFE_NEW CRotOverheadController (camCtrls.size())); // 3
	camCtrls.push_back(SAFE_NEW CFreeController        (camCtrls.size())); // 4
	camCtrls.push_back(SAFE_NEW COverviewController    (camCtrls.size())); // 5

	int mode = configHandler.GetInt("CamMode", 1);
	mode = max(0, min(mode, (int)camControllers.size() - 1));
	currentCamControllerNum = mode;
	currentCamController = camControllers[currentCamControllerNum];

	const double z = 0.0; // casting problems...
	cameraTimeFactor =
		max(z, atof(configHandler.GetString("CamTimeFactor", "1.0").c_str()));
	cameraTimeExponent =
		max(z, atof(configHandler.GetString("CamTimeExponent", "4.0").c_str()));
}

CMouseHandler::~CMouseHandler()
{
	SDL_ShowCursor(SDL_ENABLE);
	map<string, CMouseCursor*>::iterator ci;
	for (ci = cursorFileMap.begin(); ci != cursorFileMap.end(); ++ci) {
		delete ci->second;
	}

	while(!camControllers.empty()){
		delete camControllers.back();
		camControllers.pop_back();
	}
}


void CMouseHandler::LoadCursors()
{
	const CMouseCursor::HotSpot mCenter  = CMouseCursor::Center;
	const CMouseCursor::HotSpot mTopLeft = CMouseCursor::TopLeft;

	AddMouseCursor("",             "cursornormal",     mTopLeft, false);
	AddMouseCursor("Area attack",  "cursorareaattack", mCenter,  false);
	AddMouseCursor("Area attack",  "cursorattack",     mCenter,  false); // backup
	AddMouseCursor("Attack",       "cursorattack",     mCenter,  false);
	AddMouseCursor("BuildBad",     "cursorbuildbad",   mCenter,  false);
	AddMouseCursor("BuildGood",    "cursorbuildgood",  mCenter,  false);
	AddMouseCursor("Capture",      "cursorcapture",    mCenter,  false);
	AddMouseCursor("Centroid",     "cursorcentroid",   mCenter,  false);
	AddMouseCursor("DeathWait",    "cursordwatch",     mCenter,  false);
	AddMouseCursor("DeathWait",    "cursorwait",       mCenter,  false); // backup
	AddMouseCursor("DGun",         "cursordgun",       mCenter,  false);
	AddMouseCursor("DGun",         "cursorattack",     mCenter,  false); // backup
	AddMouseCursor("Fight",        "cursorfight",      mCenter,  false);
	AddMouseCursor("Fight",        "cursorattack",     mCenter,  false); // backup
	AddMouseCursor("GatherWait",   "cursorgather",     mCenter,  false);
	AddMouseCursor("GatherWait",   "cursorwait",       mCenter,  false); // backup
	AddMouseCursor("Guard",        "cursordefend",     mCenter,  false);
	AddMouseCursor("Load units",   "cursorpickup",     mCenter,  false);
	AddMouseCursor("Move",         "cursormove",       mCenter,  false);
	AddMouseCursor("Patrol",       "cursorpatrol",     mCenter,  false);
	AddMouseCursor("Reclaim",      "cursorreclamate",  mCenter,  false);
	AddMouseCursor("Repair",       "cursorrepair",     mCenter,  false);
	AddMouseCursor("Resurrect",    "cursorrevive",     mCenter,  false);
	AddMouseCursor("Resurrect",    "cursorrepair",     mCenter,  false); // backup
	AddMouseCursor("Restore",      "cursorrestore",    mCenter,  false);
	AddMouseCursor("Restore",      "cursorrepair",     mCenter,  false); // backup
	AddMouseCursor("SelfD",        "cursorselfd",      mCenter,  false);
	AddMouseCursor("SquadWait",    "cursornumber",     mCenter,  false);
	AddMouseCursor("SquadWait",    "cursorwait",       mCenter,  false); // backup
	AddMouseCursor("TimeWait",     "cursortime",       mCenter,  false);
	AddMouseCursor("TimeWait",     "cursorwait",       mCenter,  false); // backup
	AddMouseCursor("Unload units", "cursorunload",     mCenter,  false);
	AddMouseCursor("Wait",         "cursorwait",       mCenter,  false);

	// the default cursor must exist
	if (cursorCommandMap.find("") == cursorCommandMap.end()) {
		handleerror(0, "Missing default cursor", "cursornormal", 0);
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
		gs->players[gu->myPlayerNum]->currentStats->mousePixels+=abs(dx)+abs(dy);
	}

	if(activeReceiver){
		activeReceiver->MouseMove(x, y, dx, dy, activeButton);
	}

	if(inMapDrawer && inMapDrawer->keyPressed){
		inMapDrawer->MouseMove(x, y, dx, dy, activeButton);
	}

	if (buttons[SDL_BUTTON_MIDDLE].pressed && (activeReceiver == NULL)) {
		const float cameraSpeed = 1.0f;
		currentCamController->MouseMove(float3(dx, dy, invertMouse ? -1.0f : 1.0f));
		unitTracker.Disable();
		return;
	}
}


void CMouseHandler::MousePress(int x, int y, int button)
{
	if (button > NUM_BUTTONS) return;

	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);

	if(!game->gameOver)
		gs->players[gu->myPlayerNum]->currentStats->mouseClicks++;

	if(button==4){
		if (guihandler->buildSpacing > 0)
			guihandler->buildSpacing--;
		return;
	}
	if(button==5){
		guihandler->buildSpacing++;
		return;
	}

 	buttons[button].chorded = buttons[SDL_BUTTON_LEFT].pressed ||
 	                          buttons[SDL_BUTTON_RIGHT].pressed;
	buttons[button].pressed = true;
	buttons[button].time=gu->gameTime;
	buttons[button].x=x;
	buttons[button].y=y;
	buttons[button].camPos=camera->pos;
	buttons[button].dir=hide ? camera->forward : camera->CalcPixelDir(x,y);
	buttons[button].movement=0;

	activeButton=button;

	if(inMapDrawer &&  inMapDrawer->keyPressed){
		inMapDrawer->MousePress(x,y,button);
		return;
	}

	// limited receivers for MMB
	if (button == SDL_BUTTON_MIDDLE){
		if (!locked) {
			if ((minimap != NULL) && minimap->FullProxy()) {
				if (minimap->MousePress(x, y, button)) {
					activeReceiver = minimap;
					return;
				}
			}
			if (guihandler != NULL) {
				if (guihandler->MousePress(x, y, button)) {
					activeReceiver = guihandler;
					return;
				}
			}
		}
		return;
	}

	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	if (!game->hideInterface) {
		for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
			if((*ri) && (*ri)->MousePress(x,y,button)){
				activeReceiver=*ri;
				return;
			}
		}
	} else if (guihandler) {
		if (guihandler->MousePress(x,y,button)) {
			activeReceiver = guihandler; // for default (rmb) commands
		}
	}
}


void CMouseHandler::MouseRelease(int x, int y, int button)
{
	if (button > NUM_BUTTONS) return;

	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);


	buttons[button].pressed=false;

	if(inMapDrawer && inMapDrawer->keyPressed){
		inMapDrawer->MouseRelease(x, y, button);
		return;
	}

	if(activeReceiver){
		activeReceiver->MouseRelease(x,y,button);
		activeReceiver=0;
		return;
	}

	// Switch camera mode on a middle click that wasn't a middle mouse drag scroll.
	// (the latter is determined by the time the mouse was held down:
	//  <= 0.3 s means a camera mode switch, > 0.3 s means a drag scroll)
	if (button == SDL_BUTTON_MIDDLE) {
		if (buttons[SDL_BUTTON_MIDDLE].time > (gu->gameTime - 0.3f)) {
			ToggleState(keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]);
		}
		return;
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
		return;
	}
#endif

	if(button==SDL_BUTTON_LEFT && !buttons[button].chorded && mouseHandlerMayDoSelection){
		if(!keys[SDLK_LSHIFT] && !keys[SDLK_LCTRL])
			selectedUnits.ClearSelected();

		if(buttons[SDL_BUTTON_LEFT].movement>4){		//select box
			float dist=ground->LineGroundCol(buttons[SDL_BUTTON_LEFT].camPos,buttons[SDL_BUTTON_LEFT].camPos+buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4f);
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

			set<CUnit*>::iterator ui;
			CUnit* unit;
			int addedunits=0;
			int team, lastTeam;
			if (gu->spectatingFullSelect) {
				team = 0;
				lastTeam = gs->activeTeams - 1;
			} else {
				team = gu->myTeam;
				lastTeam = gu->myTeam;
			}
			for (; team <= lastTeam; team++) {
				for(ui=gs->Team(team)->units.begin();ui!=gs->Team(team)->units.end();++ui){
					float3 vec=(*ui)->midPos-camera->pos;
					if(vec.dot(norm1)<0 && vec.dot(norm2)<0 && vec.dot(norm3)<0 && vec.dot(norm4)<0){
						if(keys[SDLK_LCTRL] && selectedUnits.selectedUnits.find(*ui)!=selectedUnits.selectedUnits.end()){
							selectedUnits.RemoveUnit(*ui);
						} else {
							selectedUnits.AddUnit(*ui);
							unit = *ui;
							addedunits++;
						}
					}
				}
			}
			if(addedunits==1)
			{
				if(unit->unitDef->sounds.select.id)
					sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit, unit->unitDef->sounds.select.volume);
			}
			else if(addedunits) //more than one unit selected
				sound->PlaySample(soundMultiselID);
		} else {
			CUnit* unit;
			float dist=helper->GuiTraceRay(camera->pos,dir,gu->viewRange*1.4f,unit,20,false);
			if(unit && ((unit->team == gu->myTeam) || gu->spectatingFullSelect)){
				if(buttons[button].lastRelease < (gu->gameTime - doubleClickTime)){
					if(keys[SDLK_LCTRL] && selectedUnits.selectedUnits.find(unit)!=selectedUnits.selectedUnits.end()){
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
							lastTeam = gs->activeTeams - 1;
						} else {
							team = gu->myTeam;
							lastTeam = gu->myTeam;
						}
						for (; team <= lastTeam; team++) {
							set<CUnit*>::iterator ui;
							set<CUnit*>& teamUnits = gs->Team(team)->units;
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

				if(unit->unitDef->sounds.select.id)
					sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit,
					                     unit->unitDef->sounds.select.volume);
			}
		}
	}
}


void CMouseHandler::Draw()
{
	dir = hide ? camera->forward : camera->CalcPixelDir(lastx, lasty);
	if (activeReceiver) {
		return;
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if (gu->directControl) {
		return;
	}
#endif
	if(buttons[SDL_BUTTON_LEFT].pressed && !buttons[SDL_BUTTON_LEFT].chorded &&
	   (buttons[SDL_BUTTON_LEFT].movement > 4) &&
	   mouseHandlerMayDoSelection && (!inMapDrawer || !inMapDrawer->keyPressed)){

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

void CMouseHandler::ShowMouse()
{
	if(hide){
		SDL_ShowCursor(SDL_DISABLE);
		hide=false;
	}
}


void CMouseHandler::HideMouse()
{
	if (!hide) {
		lastx = gu->viewSizeX/2;
		lasty = gu->viewSizeY/2;
    SDL_ShowCursor(SDL_DISABLE);
		mouseInput->SetPos(int2(lastx, lasty));
		hide = true;
	}
}


void CMouseHandler::WarpMouse(int x, int y)
{
	if (!locked) {
		lastx = x;
		lasty = y;
		mouseInput->SetPos(int2(x, y));
	}
}


std::string CMouseHandler::GetCurrentTooltip(void)
{
	std::string s;
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		if ((*ri) && (*ri)->IsAbove(lastx, lasty)) {
			s = (*ri)->GetTooltip(lastx, lasty);
			if (s != "") {
				return s;
			}
		}
	}

	const string buildTip = guihandler->GetBuildTooltip();
	if (!buildTip.empty()) {
		return buildTip;
	}

	const float range = (gu->viewRange * 1.4f);
	CUnit* unit = NULL;
	CFeature* feature = NULL;
	float udist = helper->GuiTraceRay(camera->pos, dir, range, unit, 20, true);
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

	int dx = lastx - gu->viewSizeX / 2;
	int dy = lasty - gu->viewSizeY / 2;
	lastx = gu->viewSizeX / 2;
	lasty = gu->viewSizeY / 2;

	float3 move;
	move.x = dx;
	move.y = dy;
	move.z = invertMouse? -1.0f : 1.0f;
	currentCamController->MouseMove(move);

	if (gu->active) {
		mouseInput->SetPos(int2(lastx, lasty));
	}
}


/******************************************************************************/

void CMouseHandler::UpdateCam()
{
	camera->up.x = 0.0f;
	camera->up.y = 1.0f;
	camera->up.z = 0.0f;

	const float  wantedCamFOV = currentCamController->GetFOV();
	const float3 wantedCamPos = currentCamController->GetPos();
	const float3 wantedCamDir = currentCamController->GetDir();

	if (cameraTimeLeft <= 0.0f) {
		camera->pos = wantedCamPos;
		camera->forward = wantedCamDir;
		camera->fov = wantedCamFOV;
	}
	else {
		const float currTime = cameraTimeLeft;
		cameraTimeLeft = max(0.0f, (cameraTimeLeft - gu->lastFrameTime));
		const float nextTime = cameraTimeLeft;
		const float exp = cameraTimeExponent;
		const float ratio = 1.0f - (float)pow((nextTime / currTime), exp);

		const float  deltaFOV = wantedCamFOV - camera->fov;
		const float3 deltaPos = wantedCamPos - camera->pos;
		const float3 deltaDir = wantedCamDir - camera->forward;
		camera->fov     += deltaFOV * ratio;
		camera->pos     += deltaPos * ratio;
		camera->forward += deltaDir * ratio;
		camera->forward.Normalize();
	}

	dir = (hide ? camera->forward : camera->CalcPixelDir(lastx, lasty));
}


void CMouseHandler::SetCameraMode(int mode)
{
	if ((mode < 0)
	 || (mode >= camControllers.size())
	 || (mode == currentCamControllerNum)) {
		return;
	}

	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = currentCamController;

	currentCamControllerNum = mode;
	currentCamController = camControllers[currentCamControllerNum];
	currentCamController->SetPos(oldCamCtrl->SwitchFrom());
	currentCamController->SwitchTo();
}


void CMouseHandler::CameraTransition(float time)
{
	time = max(time, 0.0f) * cameraTimeFactor;
	cameraTime = time;
	cameraTimeLeft = time;
}


void CMouseHandler::ToggleState(bool shift)
{
	if(!shift){
		if(locked){
			locked=false;
			ShowMouse();
		} else {
			locked=true;
			HideMouse();
		}
	} else {
		CameraTransition(1.0f);

		CCameraController* oldCamCtrl = currentCamController;
		currentCamControllerNum++;
		if (currentCamControllerNum >= camControllers.size()) {
			currentCamControllerNum = 0;
		}

		int a = 0;
		const int maxTries = camControllers.size() - 1;
		while ((a < maxTries) &&
		       !camControllers[currentCamControllerNum]->enabled) {
			currentCamControllerNum++;
			if (currentCamControllerNum >= camControllers.size()) {
				currentCamControllerNum = 0;
			}
			a++;
		}

		currentCamController = camControllers[currentCamControllerNum];
		currentCamController->SetPos(oldCamCtrl->SwitchFrom());
		currentCamController->SwitchTo();
	}
}


void CMouseHandler::ToggleOverviewCamera(void)
{
	const int ovCamNum = (int)camControllers.size() - 1;
	CCameraController* ovCamCtrl = camControllers[ovCamNum];
	if (currentCamController == ovCamCtrl) {
		const float3 pos = ovCamCtrl->SwitchFrom();
		currentCamControllerNum = preOverviewCamNum;
		currentCamController = camControllers[currentCamControllerNum];
		currentCamController->SwitchTo(false);
		currentCamController->SetPos(pos);
	} else {
		preOverviewCamNum = currentCamControllerNum;
		currentCamControllerNum = ovCamNum;
		currentCamController = camControllers[currentCamControllerNum];
		currentCamController->SwitchTo(false);
	}
	CameraTransition(1.0f);
}


/******************************************************************************/

void CMouseHandler::SaveView(const std::string& name)
{
	if (name.empty()) {
		return;
	}
	ViewData vd;
	vd.mode = currentCamControllerNum;
	vd.state = currentCamController->GetState();
	views[name] = vd;
	logOutput.Print("Saved view: " + name);
	return;
}


bool CMouseHandler::LoadView(const std::string& name)
{
	if (name.empty()) {
		return false;
	}

	std::map<std::string, ViewData>::const_iterator it = views.find(name);
	if (it == views.end()) {
		return false;
	}
	const ViewData& saved = it->second;

	ViewData current;
	current.mode = currentCamControllerNum;
	current.state = currentCamController->GetState();

	for (it = views.begin(); it != views.end(); ++it) {
		if (it->second == current) {
			break;
		}
	}
	if (it == views.end()) {
		tmpView = current;
	}

	ViewData effective;
	if (saved == current) {
		effective = tmpView;
	} else {
		effective = saved;
	}

	return LoadViewData(effective);
}


bool CMouseHandler::LoadViewData(const ViewData& vd)
{
	if (vd.state.size() <= 0) {
		return false;
	}

	int currentMode = currentCamControllerNum;

	if ((vd.mode == -1) ||
			((vd.mode >= 0) && (vd.mode < camControllers.size()))) {
		const float3 dummy = currentCamController->SwitchFrom();
		currentCamControllerNum = vd.mode;
		currentCamController = camControllers[currentCamControllerNum];
		const bool showMode = (currentMode != vd.mode);
		currentCamController->SwitchTo(showMode);
		CameraTransition(1.0f);
	}

	return currentCamController->SetState(vd.state);
}


/******************************************************************************/

void CMouseHandler::UpdateCursors()
{
	map<string, CMouseCursor *>::iterator it;
	for (it = cursorFileMap.begin(); it != cursorFileMap.end(); ++it) {
		if (it->second != NULL) {
			it->second->Update();
		}
	}
}


void CMouseHandler::DrawCursor(void)
{
	if (guihandler) {
		guihandler->DrawCentroidCursor();
	}

	if (hide || (cursorText == "none")) {
		return;
	}

	CMouseCursor* mc;
	map<string, CMouseCursor*>::iterator it = cursorCommandMap.find(cursorText);
	if (it != cursorCommandMap.end()) {
		mc = it->second;
	} else {
		mc = cursorFileMap["cursornormal"];
	}

	if (mc == NULL) {
		return;
	}

	if (cursorScale >= 0.0f) {
		mc->Draw(lastx, lasty, cursorScale);
	}
	else {
		CMouseCursor* nc = cursorFileMap["cursornormal"];
		if (nc == NULL) {
			mc->Draw(lastx, lasty, -cursorScale);
		}
		else {
			nc->Draw(lastx, lasty, 1.0f);
			if (mc != nc) {
				mc->Draw(lastx + nc->GetMaxSizeX(),
								 lasty + nc->GetMaxSizeY(), -cursorScale);
			}
		}
	}
}


bool CMouseHandler::AddMouseCursor(const std::string& name,
	                                 const std::string& filename,
	                                 CMouseCursor::HotSpot hotSpot,
																	 bool overwrite)
{
	if (!overwrite && (cursorCommandMap.find(name) != cursorCommandMap.end())) {
		return true; // already assigned
	}

	CMouseCursor* cursor = NULL;
	std::map<std::string, CMouseCursor*>::iterator fileIt;
	fileIt = cursorFileMap.find(filename);
	if (fileIt != cursorFileMap.end()) {
		cursor = fileIt->second;
	} else {
		cursor = CMouseCursor::New(filename, hotSpot);
		if (cursor == NULL) {
			return false; // invalid cursor
		}
		cursorFileMap[filename] = cursor;
	}

	// assign the new cursor
	cursorCommandMap[name] = cursor;

	return true;
}


/******************************************************************************/
