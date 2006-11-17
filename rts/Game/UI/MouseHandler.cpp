#include "StdAfx.h"
// MouseHandler.cpp: implementation of the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "MouseHandler.h"
#include "CommandColors.h"
#include "InfoConsole.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseCursor.h"
#include "MouseInput.h"
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
#include "Sound.h"
#include "mmgr.h"
#include <SDL_types.h>
#include <SDL_mouse.h>
#include <SDL_keysym.h>
#include <SDL_events.h>

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
	cameraTimeLeft(0.0f)
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

	cursorFileMap["cursornormal"] = // must always be loaded
		CMouseCursor::New("cursornormal", CMouseCursor::TopLeft);

	LoadCursorFile("cursornormal",     CMouseCursor::TopLeft);
	LoadCursorFile("cursorareaattack", CMouseCursor::Center);
	LoadCursorFile("cursorattack",     CMouseCursor::Center);
	LoadCursorFile("cursorbuildbad",   CMouseCursor::Center);
	LoadCursorFile("cursorbuildgood",  CMouseCursor::Center);
	LoadCursorFile("cursorcapture",    CMouseCursor::Center);
	LoadCursorFile("cursorcentroid",   CMouseCursor::Center);
	LoadCursorFile("cursordefend",     CMouseCursor::Center);
	LoadCursorFile("cursordgun",       CMouseCursor::Center);
	LoadCursorFile("cursordwatch",     CMouseCursor::Center);
	LoadCursorFile("cursorfight",      CMouseCursor::Center);
	LoadCursorFile("cursorgather",     CMouseCursor::Center);
	LoadCursorFile("cursormove",       CMouseCursor::Center);
	LoadCursorFile("cursornumber",     CMouseCursor::Center);
	LoadCursorFile("cursorpatrol",     CMouseCursor::Center);
	LoadCursorFile("cursorpickup",     CMouseCursor::Center);
	LoadCursorFile("cursorreclamate",  CMouseCursor::Center);
	LoadCursorFile("cursorrepair",     CMouseCursor::Center);
	LoadCursorFile("cursorrestore",    CMouseCursor::Center);
	LoadCursorFile("cursorrevive",     CMouseCursor::Center);
	LoadCursorFile("cursorselfd",      CMouseCursor::Center);
	LoadCursorFile("cursortime",       CMouseCursor::Center);
	LoadCursorFile("cursorunload",     CMouseCursor::Center);	
	LoadCursorFile("cursorwait",       CMouseCursor::Center);

	AttachCursorCommand("",             "cursornormal");
	AttachCursorCommand("Area attack",  "cursorareaattack", "cursorattack");
	AttachCursorCommand("Attack",       "cursorattack");
	AttachCursorCommand("BuildBad",     "cursorbuildbad");
	AttachCursorCommand("BuildGood",    "cursorbuildgood");
	AttachCursorCommand("Capture",      "cursorcapture");
	AttachCursorCommand("Centroid",     "cursorcentroid");
	AttachCursorCommand("DeathWait",    "cursordwatch",     "cursorwait");
	AttachCursorCommand("DGun",         "cursordgun",       "cursorattack");
	AttachCursorCommand("Fight",        "cursorfight",      "cursorattack");
	AttachCursorCommand("GatherWait",   "cursorgather",     "cursorwait");
	AttachCursorCommand("Guard",        "cursordefend");
	AttachCursorCommand("Load units",   "cursorpickup");
	AttachCursorCommand("Move",         "cursormove");
	AttachCursorCommand("Patrol",       "cursorpatrol");
	AttachCursorCommand("Reclaim",      "cursorreclamate");
	AttachCursorCommand("Repair",       "cursorrepair");
	AttachCursorCommand("Resurrect",    "cursorrevive",     "cursorrepair");
	AttachCursorCommand("Restore",      "cursorrestore",    "cursorrepair");
	AttachCursorCommand("SelfD",        "cursorselfd");
	AttachCursorCommand("SquadWait",    "cursornumber",     "cursorwait");
	AttachCursorCommand("TimeWait",     "cursortime",       "cursorwait");
	AttachCursorCommand("Unload units", "cursorunload");
	AttachCursorCommand("Wait",         "cursorwait");

	SDL_ShowCursor(SDL_DISABLE);

	soundMultiselID = sound->GetWaveId("sounds/button9.wav");

	invertMouse=!!configHandler.GetInt("InvertMouse",1);
  doubleClickTime = (float)configHandler.GetInt("DoubleClickTime", 200) / 1000.0f;

	//fps camera must always be the first one in the list
	camControllers.push_back(new CFPSController);
	camControllers.push_back(new COverheadController);
	camControllers.push_back(new CTWController);
	camControllers.push_back(new CRotOverheadController);

	overviewController=new COverviewController();
	int mode=configHandler.GetInt("CamMode",1);
	currentCamController=camControllers[mode];
	currentCamControllerNum=mode;

	cameraTimeFactor   = atof(configHandler.GetString("CamTimeFactor", "1.0").c_str());
	cameraTimeFactor   = max(0.0f, cameraTimeFactor);
	cameraTimeExponent = atof(configHandler.GetString("CamTimeExponent", "4.0").c_str());
	cameraTimeExponent = max(0.1f, cameraTimeExponent);
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


/******************************************************************************/

void CMouseHandler::MouseMove(int x, int y)
{
	if(hide) {
		lastx = x;
		lasty = y;
		return;
	}

	int dx=x-lastx;
	int dy=y-lasty;
	lastx = x;  
	lasty = y;  

	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);

	buttons[SDL_BUTTON_LEFT].movement+=abs(dx)+abs(dy);
	buttons[SDL_BUTTON_RIGHT].movement+=abs(dx)+abs(dy);

	if(!game->gameOver)
		gs->players[gu->myPlayerNum]->currentStats->mousePixels+=abs(dx)+abs(dy);

	if(activeReceiver){
		activeReceiver->MouseMove(x,y,dx,dy,activeButton);
	}

#ifndef NEW_GUI
	if(inMapDrawer && inMapDrawer->keyPressed){
		inMapDrawer->MouseMove(x,y,dx,dy,activeButton);
	}
#endif

	if (buttons[SDL_BUTTON_MIDDLE].pressed &&
	    !((minimap != NULL) && (activeReceiver == minimap))) {
		float cameraSpeed = 1.0f;
		currentCamController->MouseMove(float3(dx * cameraSpeed, dy * cameraSpeed, invertMouse ? -1 : 1));
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
			guihandler->buildSpacing --;
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

	if (button==SDL_BUTTON_MIDDLE){
		if ((minimap != NULL) && minimap->FullProxy() && !locked) {
			if (minimap->MousePress(x, y, button)) {
				activeReceiver = minimap;
			}
		}
		return;
	}

#ifndef NEW_GUI
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
			activeReceiver = guihandler;
		}
	}
#endif

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

	if (button == SDL_BUTTON_MIDDLE) {
		if ((minimap != NULL) && (activeReceiver == minimap)) {
			minimap->MouseRelease(x, y, button);
			activeReceiver = 0;
			return;
		}
		if (buttons[SDL_BUTTON_MIDDLE].time>gu->gameTime-0.3f) {
			ToggleState(keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]);
		}
		return;		
	}

#ifndef NEW_GUI
	if(activeReceiver){
		activeReceiver->MouseRelease(x,y,button);
		activeReceiver=0;
		return;
	}
#endif
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
			for(ui=gs->Team(gu->myTeam)->units.begin();ui!=gs->Team(gu->myTeam)->units.end();++ui){
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
			if(unit && unit->team==gu->myTeam){
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
						set<CUnit*>::iterator ui;
						set<CUnit*>& myTeamUnits = gs->Team(gu->myTeam)->units;
						for (ui = myTeamUnits.begin(); ui != myTeamUnits.end(); ++ui) {
							if (((*ui)->aihint == unit->aihint) &&
							    (camera->InView((*ui)->midPos) || keys[SDLK_LCTRL])) {
								selectedUnits.AddUnit(*ui);
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
	dir=hide ? camera->forward : camera->CalcPixelDir(lastx,lasty);
	if(activeReceiver)
		return;

#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
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
	if(!hide){
		lastx = gu->viewSizeX/2;  
		lasty = gu->viewSizeY/2;  
	    SDL_ShowCursor(SDL_DISABLE);
		mouseInput->SetPos (int2(lastx,lasty));
		hide=true;
	}
}


std::string CMouseHandler::GetCurrentTooltip(void)
{
	std::string s;
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri) && (*ri)->IsAbove(lastx,lasty)){
			s=(*ri)->GetTooltip(lastx,lasty);
			if(s!="")
				return s;
		}
	}

#ifndef NEW_GUI
	s=guihandler->GetBuildTooltip();
	if(s!="")
		return s;
#endif

	/*
	NOTE:
	The code below (up untill "if(unit)...") is exactly the same as
	some lines in CGuiHandler::GetDefaultCommand().
	Perhaps it would be possible to integrate the two, because now a
	racetray for units and features might be performed twice per frame.
	*/
	CUnit* unit=0;
	CFeature* feature=0;
	float dist=helper->GuiTraceRay(camera->pos,dir,gu->viewRange*1.4f,unit,20,true);
	float dist2=helper->GuiTraceRayFeature(camera->pos,dir,gu->viewRange*1.4f,feature);

	if(dist>gu->viewRange*1.4f-300 && dist2>gu->viewRange*1.4f-300 && unit==0){
		return "";
	}

	if(dist>dist2)
		unit=0;
	else
		feature=0;

	if(unit){
		// don't show the tooltip if it's a radar dot
		if(!gu->spectating && gs->AllyTeam(unit->team) != gu->myAllyTeam && !loshandler->InLos(unit,gu->myAllyTeam)){
			return "Enemy unit";
		}
		// show the player name instead of unit name if it has FBI tag showPlayerName
		if(unit->unitDef->showPlayerName){
			s=gs->players[gs->Team(unit->team)->leader]->playerName.c_str();
		} else {
			s=unit->tooltip;
		}
		// don't show the unit health and other info if it has FBI tag hideDamage and isn't on our ally team
		if(!(!gu->spectating && unit->unitDef->hideDamage && gs->AllyTeam(unit->team) != gu->myAllyTeam)){
			char tmp[500];

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
	
	if(feature){
		if(feature->def->description==""){
			s="Feature";
		} else {
			s=feature->def->description;
		}

		float remainingMetal = feature->RemainingMetal();
		float remainingEnergy = feature->RemainingEnergy();

		std::string metalColor = remainingMetal > 0 ? "\xff\x50\xff\x50" : "\xff\xff\x50\x01";
		std::string energyColor = remainingEnergy > 0 ? "\xff\x50\xff\x50" : "\xff\xff\x50\x01";
		
		char tmp[500];
		sprintf(tmp,"\n\xff\xd3\xdb\xffMetal: %s%.0f \xff\xd3\xdb\xff Energy: %s%.0f",
			metalColor.c_str(), remainingMetal,
			energyColor.c_str(), remainingEnergy);
		s+=tmp;

		return s;
	}

	s=selectedUnits.GetTooltip();
	if(s!="")
		return s;

	if(dist<gu->viewRange*1.4f-100){
		float3 pos=camera->pos+dir*dist;
		char tmp[500];
		CReadMap::TerrainType* tt=&readmap->terrainTypes[readmap->typemap[min(gs->hmapx*gs->hmapy-1,max(0,((int)pos.z/16)*gs->hmapx+((int)pos.x/16)))]];
		string ttype=tt->name;
		sprintf(tmp,"Pos %.0f %.0f Elevation %.0f\nTerrain type: %s\nSpeeds T/K/H/S %.2f %.2f %.2f %.2f\nHardness %.0f Metal %.1f",pos.x,pos.z,pos.y,ttype.c_str(),tt->tankSpeed,tt->kbotSpeed,tt->hoverSpeed,tt->shipSpeed,tt->hardness*mapDamage->mapHardness,readmap->metalMap->getMetalAmount((int)(pos.x/16),(int)(pos.z/16)));
		return tmp;
	}

	return "";
}


void CMouseHandler::EmptyMsgQueUpdate(void)
{
	if (!hide)
		return;

	int dx = lastx-gu->viewSizeX/2;
	int dy = lasty-gu->viewSizeY/2;
	lastx = gu->viewSizeX/2; lasty = gu->viewSizeY/2;

	float3 move;
	move.x=dx;
	move.y=dy;
	move.z=invertMouse? -1 : 1;
	currentCamController->MouseMove(move);
	
	mouseInput->SetPos (int2(lastx,lasty));
}


/******************************************************************************/

void CMouseHandler::UpdateCam()
{
	camera->up.x = 0.0f;
	camera->up.y = 1.0f;
	camera->up.z = 0.0f;
	camera->fov = 45.0f;

	const float3 wantedCamPos = currentCamController->GetPos();
	const float3 wantedCamDir = currentCamController->GetDir();

	if (cameraTimeLeft <= 0.0f) {
		camera->pos = wantedCamPos;
		camera->forward = wantedCamDir;
	}
	else {
		const float currTime = cameraTimeLeft;
		cameraTimeLeft = max(0.0f, (cameraTimeLeft - gu->lastFrameTime));
		const float nextTime = cameraTimeLeft;
		const float exp = cameraTimeExponent;
		const float ratio = 1.0f - (float)pow((nextTime / currTime), exp);

		const float3 deltaPos = wantedCamPos - camera->pos;
		const float3 deltaDir = wantedCamDir - camera->forward;
		camera->pos     += deltaPos * ratio;
		camera->forward += deltaDir * ratio;
		camera->forward.Normalize();
	}
	
	dir = (hide ? camera->forward : camera->CalcPixelDir(lastx,lasty));
}


void CMouseHandler::SetCameraMode(int mode)
{
	if ((mode < 0)
	 || (mode >= camControllers.size())
	 || (mode == currentCamControllerNum)
	 || (!camControllers[currentCamControllerNum]->enabled)) {
		return;
	}

	currentCamControllerNum = mode;
	CameraTransition(1.0f);

	CCameraController* oldc = currentCamController;
	currentCamController = camControllers[currentCamControllerNum];
	currentCamController->SetPos(oldc->SwitchFrom());
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

		CCameraController* oldc=currentCamController;
		currentCamControllerNum++;
		if(currentCamControllerNum>=camControllers.size())
			currentCamControllerNum=0;
		int a=0;
		while(a<4 && !camControllers[currentCamControllerNum]->enabled){
			currentCamControllerNum++;
			if(currentCamControllerNum>=camControllers.size())
				currentCamControllerNum=0;
		}
		currentCamController=camControllers[currentCamControllerNum];
		currentCamController->SetPos(oldc->SwitchFrom());
		currentCamController->SwitchTo();
	}
}


void CMouseHandler::ToggleOverviewCamera(void)
{
	if(currentCamController==overviewController){
		float3 pos=overviewController->SwitchFrom();
		currentCamController=camControllers[currentCamControllerNum];
		currentCamController->SwitchTo(false);
		currentCamController->SetPos(pos);
	} else {
		currentCamController=overviewController;
		overviewController->SwitchTo(false);
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
	if (currentCamController == overviewController) {
		vd.mode = -1;
	} else {
		vd.mode = currentCamControllerNum;
	}
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
	current.state = currentCamController->GetState();
	if (currentCamController == overviewController) {
		current.mode = -1;
	} else {
		current.mode = currentCamControllerNum;
	}
	
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
	
	if (effective.state.size() <= 0) {
		return false;
	}
	
	if ((effective.mode == -1) ||
			((effective.mode >= 0) && (effective.mode < camControllers.size()) &&
			 camControllers[effective.mode]->enabled)) {
		const float3 dummy = currentCamController->SwitchFrom();
		if (effective.mode >= 0) {
			currentCamControllerNum = effective.mode;
			currentCamController = camControllers[effective.mode];
		} else {
			currentCamController = overviewController;
		}
		const bool showMode = (current.mode != effective.mode);
		currentCamController->SwitchTo(showMode);
		CameraTransition(1.0f);
	}
	
	return currentCamController->SetState(effective.state);;
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


void CMouseHandler::LoadCursorFile(const std::string& filename,
                                   CMouseCursor::HotSpot hotspot)
{
	if (cursorFileMap.find(filename) == cursorFileMap.end()) {
		cursorFileMap[filename] = CMouseCursor::New(filename, hotspot);
	}
}


void CMouseHandler::AttachCursorCommand(const std::string& commandName,
                                        const std::string& filename)
{
	std::map<std::string, CMouseCursor*>::iterator it;
	it = cursorFileMap.find(filename);
	if ((it != cursorFileMap.end()) && (it->second != NULL)) {
		cursorCommandMap[commandName] = it->second;
		return;
	}
	logOutput.Print("No cursor available for command: %s",
	                commandName.c_str());
}


void CMouseHandler::AttachCursorCommand(const std::string& commandName,
                                        const std::string& filename1,
                                        const std::string& filename2)
{
	std::map<std::string, CMouseCursor*>::iterator it;
	it = cursorFileMap.find(filename1);
	if ((it != cursorFileMap.end()) && (it->second != NULL)) {
		cursorCommandMap[commandName] = it->second;
		return;
	}
	it = cursorFileMap.find(filename2);
	if ((it != cursorFileMap.end()) && (it->second != NULL)) {
		cursorCommandMap[commandName] = it->second;
		return;
	}
	logOutput.Print("No cursor available for command: %s",
	                commandName.c_str());
}


/******************************************************************************/
