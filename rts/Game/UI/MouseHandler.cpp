#include "StdAfx.h"
// MouseHandler.cpp: implementation of the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Map/Ground.h"
#include "Game/Game.h"
#include "Game/Camera.h"
#include "GuiHandler.h"
#include "CommandColors.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/Team.h"
#include "InfoConsole.h"
#include "MiniMap.h"
#include "InputReceiver.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/glFont.h"
#include "Sim/Units/UnitHandler.h"
#include "MouseCursor.h"
#include "MouseInput.h"
#include "Sound.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Misc/FeatureDef.h"
#include "ExternalAI/Group.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/InMapDraw.h"
#include "Game/CameraController.h"
#include "Map/MapDamage.h"
#include "mmgr.h"
#include "SDL_types.h"
#include "SDL_mouse.h"
#include "SDL_keysym.h"
#include "SDL_events.h"

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
	transitSpeed(0),
	inStateTransit(false)
{
	lastx=300;
	lasty=200;
	hide=false;

	for(int a=1;a<=NUM_BUTTONS;a++){
		buttons[a].pressed=false;
		buttons[a].lastRelease=-20;
		buttons[a].movement=0;
	}

	cursors[""] = new CMouseCursor("cursornormal", CMouseCursor::TopLeft);
	cursors["Move"] = new CMouseCursor("cursormove", CMouseCursor::Center);
	cursors["Guard"] = new CMouseCursor("cursordefend", CMouseCursor::Center);
	cursors["Attack"] = new CMouseCursor("cursorattack", CMouseCursor::Center);
	cursors["DGun"] = new CMouseCursor("cursorattack", CMouseCursor::Center);
	cursors["Patrol"] = new CMouseCursor("cursorpatrol", CMouseCursor::Center);
	cursors["Fight"] = new CMouseCursor("cursorattack", CMouseCursor::Center);
	cursors["Repair"] = new CMouseCursor("cursorrepair", CMouseCursor::Center);
	cursors["Load units"] = new CMouseCursor("cursorpickup", CMouseCursor::Center);
	cursors["Unload units"] = new CMouseCursor("cursorunload", CMouseCursor::Center);	
	cursors["Reclaim"] = new CMouseCursor("cursorreclamate", CMouseCursor::Center);
	cursors["Resurrect"] = new CMouseCursor("cursorrevive", CMouseCursor::Center);
	cursors["Capture"] = new CMouseCursor("cursorcapture", CMouseCursor::Center);
	cursors["Wait"] = new CMouseCursor("cursorwait", CMouseCursor::Center);

	SDL_ShowCursor(SDL_DISABLE);

	soundMultiselID = sound->GetWaveId("button9.wav");

	invertMouse=!!configHandler.GetInt("InvertMouse",1);
  doubleClickTime = (float)configHandler.GetInt("DoubleClickTime", 200) / 1000.0f;

	camControllers.push_back(new CFPSController);				//fps camera must always be the first one in the list
	camControllers.push_back(new COverheadController);
	camControllers.push_back(new CTWController);
	camControllers.push_back(new CRotOverheadController);

	overviewController=new COverviewController();
	int mode=configHandler.GetInt("CamMode",1);
	currentCamController=camControllers[mode];
	currentCamControllerNum=mode;
}

CMouseHandler::~CMouseHandler()
{
	SDL_ShowCursor(SDL_ENABLE);
	for (map<string, CMouseCursor *>::iterator i = cursors.begin(); i != cursors.end(); ++i) {
		delete i->second;
	}

	while(!camControllers.empty()){
		delete camControllers.back();
		camControllers.pop_back();
	}
}

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

	if(buttons[SDL_BUTTON_MIDDLE].pressed){
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

	buttons[button].pressed=true;
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

	if(button==SDL_BUTTON_MIDDLE){
		return;
	}

#ifndef NEW_GUI
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri)->MousePress(x,y,button)){
			activeReceiver=*ri;
			return;
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
		inMapDrawer->MouseRelease(x,y,button);
		return;
	}

	if(button==SDL_BUTTON_MIDDLE){
		if(buttons[SDL_BUTTON_MIDDLE].time>gu->gameTime-0.3f)
			ToggleState(keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]);
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

	if(button==SDL_BUTTON_LEFT && mouseHandlerMayDoSelection){
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
				} else {												//double click
					if(unit->group){									//select the current units group if it has one
						selectedUnits.SelectGroup(unit->group->id);
					} else {														//select all units of same type on screen
						set<CUnit*>::iterator ui;
						for(ui=gs->Team(gu->myTeam)->units.begin();ui!=gs->Team(gu->myTeam)->units.end();++ui){
							if((*ui)->aihint==unit->aihint && camera->InView((*ui)->midPos)){
								selectedUnits.AddUnit(*ui);
							}
						}
					}
				}
				buttons[button].lastRelease=gu->gameTime;

				if(unit->unitDef->sounds.select.id)
					sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit, unit->unitDef->sounds.select.volume);
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
	if(buttons[SDL_BUTTON_LEFT].pressed && buttons[SDL_BUTTON_LEFT].movement>4 &&
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
		lastx = gu->screenx/2;  
		lasty = gu->screeny/2;  
	    SDL_ShowCursor(SDL_DISABLE);
		mouseInput->SetPos (int2(lastx,lasty));
		hide=true;
	}
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
	inStateTransit = true;
	transitSpeed = 1;

	CCameraController* oldc = currentCamController;
	currentCamController = camControllers[currentCamControllerNum];
	currentCamController->SetPos(oldc->SwitchFrom());
	currentCamController->SwitchTo();
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
		inStateTransit=true;
		transitSpeed=1;

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

void CMouseHandler::UpdateCam()
{
	camera->up.x=0;
	camera->up.y=1;
	camera->up.z=0;
	camera->fov=45;

	float3 wantedCamPos=currentCamController->GetPos();
	float3 wantedCamDir=currentCamController->GetDir();


	if(inStateTransit){
		transitSpeed=transitSpeed*pow(0.00003f,(float)gu->lastFrameTime)/*-gu->lastFrameTime*/;
		camera->pos+=(wantedCamPos-camera->pos)*(1-pow(transitSpeed,(float)gu->lastFrameTime));
		camera->forward+=(wantedCamDir-camera->forward)*(1-pow(transitSpeed,(float)gu->lastFrameTime));
		camera->forward.Normalize();
		if(camera->pos.distance(wantedCamPos)<0.01f && camera->forward.distance(wantedCamDir)<0.001f)
			inStateTransit=false;
	} else {
		camera->pos=wantedCamPos;
		camera->forward=wantedCamDir;
	}
	dir=(hide ? camera->forward : camera->CalcPixelDir(lastx,lasty));
}

void CMouseHandler::UpdateCursors()
{
	map<string, CMouseCursor *>::iterator i;
	for (i = cursors.begin(); i != cursors.end(); ++i) {
		i->second->Update();
	}
}

void CMouseHandler::DrawCursor(void)
{
	if (hide)
		return;

	CMouseCursor *mc;
	map<string, CMouseCursor *>::iterator i;
	if ((i = cursors.find(cursorText)) != cursors.end()) {
		mc = i->second;
	}
	else {
		//logOutput.Print("Unknown cursor: %s", cursorText.c_str());		
		mc = cursors[""];
	}

	mc->Draw(lastx, lasty);
}

std::string CMouseHandler::GetCurrentTooltip(void)
{
	std::string s;
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri)->IsAbove(lastx,lasty)){
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

	int dx = lastx-gu->screenx/2;
	int dy = lasty-gu->screeny/2;
	lastx = gu->screenx/2; lasty = gu->screeny/2;

	float3 move;
	move.x=dx;
	move.y=dy;
	move.z=invertMouse? -1 : 1;
	currentCamController->MouseMove(move);
	
	mouseInput->SetPos (int2(lastx,lasty));
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
	inStateTransit=true;
	transitSpeed=1;
}


