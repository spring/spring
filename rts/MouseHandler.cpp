#include "StdAfx.h"
// MouseHandler.cpp: implementation of the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "MouseHandler.h"
#include <winsock2.h>
#include <windows.h>		// Header File For Windows
#include "myGL.h"
#include "Ground.h"
#include "Game.h"
#include "Camera.h"
#include "GuiHandler.h"
#include "GameHelper.h"
#include "SelectedUnits.h"
#include "Unit.h"
#include "Team.h"
#include "InfoConsole.h"
#include "MiniMap.h"
#include "InputReceiver.h"
#include "Bitmap.h"
#include "glFont.h"
#include "UnitHandler.h"
#include "MouseCursor.h"
#include "Sound.h"
#include "UnitDef.h"
#include "Group.h"
#include "RegHandler.h"
#include "InMapDraw.h"
#include "CameraController.h"
//#include "mmgr.h"

#include "NewGuiDefine.h"

#ifdef NEW_GUI
#include "GUIcontroller.h"
#include "MouseHandler.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern bool	fullscreen;
extern bool keys[256];

extern bool mouseHandlerMayDoSelection;

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

	for(int a=0;a<5;a++){
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
	cursors["Repair"] = new CMouseCursor("cursorrepair", CMouseCursor::Center);
	cursors["Load units"] = new CMouseCursor("cursorpickup", CMouseCursor::Center);
	cursors["Unload units"] = new CMouseCursor("cursorunload", CMouseCursor::Center);	
	cursors["Reclaim"] = new CMouseCursor("cursorreclamate", CMouseCursor::Center);

	ShowCursor(FALSE);

	soundMultiselID = sound->GetWaveId("button9.wav");

	invertMouse=!!regHandler.GetInt("InvertMouse",1);

	camControllers.push_back(new CFPSController);				//fps camera must always be the first one in the list
	camControllers.push_back(new COverheadController);
	camControllers.push_back(new CTWController);

	currentCamController=camControllers[0];
	currentCamControllerNum=0;
}

CMouseHandler::~CMouseHandler()
{
	ShowCursor(TRUE);
	for (map<string, CMouseCursor *>::iterator i = cursors.begin(); i != cursors.end(); ++i) {
		delete i->second;
	}

	while(!camControllers.empty()){
		delete camControllers.back();
		camControllers.pop_back();
	}
}


static bool internalMouseMove=true;
static bool mouseMovedFromCenter=false;

void CMouseHandler::MouseMove(int x, int y)
{
	if(hide){
		if(!internalMouseMove){
			mouseMovedFromCenter=true;
		} else {
			internalMouseMove=false;
			lastx = x;  
			lasty = y;  
			return;
		}
	}
	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);

	int dx=x-lastx;
	int dy=y-lasty;
	lastx = x;  
	lasty = y;  
	buttons[0].movement+=abs(dx)+abs(dy);
	buttons[1].movement+=abs(dx)+abs(dy);

	gs->players[gu->myPlayerNum]->currentStats->mousePixels+=abs(dx)+abs(dy);

#ifdef NEW_GUI
	if(dx||dy)
		if(GUIcontroller::MouseMove(x, y, dx, dy, activeButton))
		{
			return;
		}
#else
	if(activeReceiver){
		activeReceiver->MouseMove(x,y,dx,dy,activeButton);
	}
#endif

	if(inMapDrawer->keyPressed){
		inMapDrawer->MouseMove(x,y,dx,dy,activeButton);
	}

	if(buttons[2].pressed){
		float cameraSpeed=1;
		if(keys[VK_SHIFT])
	        {
			cameraSpeed*=0.1f;
		}
		if(keys[VK_CONTROL])
		{
			cameraSpeed*=10;
		}
		currentCamController->MouseMove(float3(dx,dy,invertMouse?-1:1));

		return;
	} 

	if(hide){
		float3 move;
		move.x=dx;
		move.y=dy;
		move.z=invertMouse? -1 : 1;
		currentCamController->MouseMove(move);
		return;	
	}
}

void CMouseHandler::MousePress(int x, int y, int button)
{
	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);
	
	gs->players[gu->myPlayerNum]->currentStats->mouseClicks++;

#ifdef NEW_GUI
	activeButton=button;
	if(GUIcontroller::MouseDown(x, y, button))
		return;
#endif
	
	if(button==4){
/*		CUnit* u;
		float dist=helper->GuiTraceRay(camera->pos,hide ? camera->forward : camera->CalcPixelDir(x,y),9000,u,20,false);
		if(dist<8900 && gs->cheatEnabled){
			float3 pos=camera->pos+(hide ? camera->forward : camera->CalcPixelDir(x,y))*dist;
			helper->Explosion(pos,DamageArray()*600,64,0);
		}/**/			//make network compatible before enabling
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

	if(inMapDrawer->keyPressed){
		inMapDrawer->MousePress(x,y,button);
		return;
	}

	if(button==2){
		return;
	}

#ifndef NEW_GUI
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers->begin();ri!=inputReceivers->end();++ri){
		if((*ri)->MousePress(x,y,button)){
			activeReceiver=*ri;
			return;
		}
	}
#endif

}

void CMouseHandler::MouseRelease(int x, int y, int button)
{
	dir=hide ? camera->forward : camera->CalcPixelDir(x,y);
	

	buttons[button].pressed=false;

#ifdef NEW_GUI
	if(GUIcontroller::MouseUp(x, y, button))
		return;
#endif
	if(inMapDrawer->keyPressed){
		inMapDrawer->MouseRelease(x,y,button);
		return;
	}

	if(button==2){
		if(buttons[2].time>gu->gameTime-0.3)
			ToggleState(keyShift()|keyCtrl());
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

	if(button==0 && mouseHandlerMayDoSelection){
		if(!keyShift() && !keyCtrl())
			selectedUnits.ClearSelected();

		if(buttons[0].movement>4){		//select box
			float dist=ground->LineGroundCol(buttons[0].camPos,buttons[0].camPos+buttons[0].dir*9000);
			if(dist<0)
				dist=9000;
			float3 pos1=buttons[0].camPos+buttons[0].dir*dist;

			dist=ground->LineGroundCol(camera->pos,camera->pos+dir*9000);
			if(dist<0)
				dist=9000;
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
			for(ui=gs->teams[gu->myTeam]->units.begin();ui!=gs->teams[gu->myTeam]->units.end();++ui){
				float3 vec=(*ui)->midPos-camera->pos;
				if(vec.dot(norm1)<0 && vec.dot(norm2)<0 && vec.dot(norm3)<0 && vec.dot(norm4)<0){
					if(keyCtrl() && selectedUnits.selectedUnits.find(*ui)!=selectedUnits.selectedUnits.end()){
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
					sound->PlaySound(unit->unitDef->sounds.select.id, unit->pos, unit->unitDef->sounds.select.volume);
			}
			else if(addedunits) //more than one unit selected
				sound->PlaySound(soundMultiselID);
		} else {
			CUnit* unit;
			float dist=helper->GuiTraceRay(camera->pos,dir,9000,unit,20,false);
			if(unit && unit->team==gu->myTeam){
				if(buttons[button].lastRelease<gu->gameTime-0.2){
					if(keyCtrl() && selectedUnits.selectedUnits.find(unit)!=selectedUnits.selectedUnits.end()){
						selectedUnits.RemoveUnit(unit);
					} else {
						selectedUnits.AddUnit(unit);
					}
				} else {												//double click
					if(unit->group){									//select the current units group if it has one
						selectedUnits.SelectGroup(unit->group->id);
					} else {														//select all units of same type on screen
						set<CUnit*>::iterator ui;
						for(ui=gs->teams[gu->myTeam]->units.begin();ui!=gs->teams[gu->myTeam]->units.end();++ui){
							if((*ui)->aihint==unit->aihint && camera->InView((*ui)->midPos)){
								selectedUnits.AddUnit(*ui);
							}
						}
					}
				}
				buttons[button].lastRelease=gu->gameTime;

				if(unit->unitDef->sounds.select.id)
					sound->PlaySound(unit->unitDef->sounds.select.id, unit->pos, unit->unitDef->sounds.select.volume);
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

	if(buttons[0].pressed && buttons[0].movement>4 && mouseHandlerMayDoSelection && !inMapDrawer->keyPressed){
		glDisable(GL_TEXTURE_2D);
		glColor4f(1,1,1,0.5);

		float dist=ground->LineGroundCol(buttons[0].camPos,buttons[0].camPos+buttons[0].dir*9000);
		if(dist<0)
			dist=9000;
		float3 pos1=buttons[0].camPos+buttons[0].dir*dist;

		dist=ground->LineGroundCol(camera->pos,camera->pos+dir*9000);
		if(dist<0)
			dist=9000;
		float3 pos2=camera->pos+dir*dist;

		float3 dir1=pos1-camera->pos;
		dir1.Normalize();
		float3 dir2=pos2-camera->pos;
		dir2.Normalize();

		float3 dir1S=camera->right*(dir1.dot(camera->right))/dir1.dot(camera->forward);
		float3 dir1U=camera->up*(dir1.dot(camera->up))/dir1.dot(camera->forward);

		float3 dir2S=camera->right*(dir2.dot(camera->right))/dir2.dot(camera->forward);
		float3 dir2U=camera->up*(dir2.dot(camera->up))/dir2.dot(camera->forward);

		glDisable(GL_FOG);
		glDisable(GL_DEPTH_TEST);
		glBegin(GL_LINE_STRIP);
		glVertexf3(camera->pos+dir1U*30+dir1S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir1S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir2S*30+camera->forward*30);
		glVertexf3(camera->pos+dir1U*30+dir2S*30+camera->forward*30);
		glVertexf3(camera->pos+dir1U*30+dir1S*30+camera->forward*30);
		glEnd();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_FOG);
	}
}

void CMouseHandler::ShowMouse()
{
	if(hide){
		ShowCursor(FALSE);
		hide=false;
	}
}

void CMouseHandler::HideMouse()
{
	if(!hide){
		lastx = gu->screenx/2;  
		lasty = gu->screeny/2;  
#ifdef USE_GLUT
		glutSetCursor(GLUT_CURSOR_NONE);
#warning add a way to set cursor pos
#else
	        ShowCursor(FALSE);
		hide=true;
		RECT cr;
		if(GetWindowRect(hWnd,&cr)==0)
			MessageBox(0,"mouse error","",0);
		SetCursorPos(gu->screenx/2+cr.left+4*!fullscreen,gu->screeny/2+cr.top+23*!fullscreen);
#endif //GLUT
	}
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
		transitSpeed=transitSpeed*pow(0.00003,(double)gu->lastFrameTime)/*-gu->lastFrameTime*/;
		camera->pos+=(wantedCamPos-camera->pos)*(1-pow(transitSpeed,(double)gu->lastFrameTime));
		camera->forward+=(wantedCamDir-camera->forward)*(1-pow(transitSpeed,(double)gu->lastFrameTime));
		camera->forward.Normalize();
		if(camera->pos.distance(wantedCamPos)<0.01 && camera->forward.distance(wantedCamDir)<0.001)
			inStateTransit=false;
	} else {
		camera->pos=wantedCamPos;
		camera->forward=wantedCamDir;
	}
	dir=(hide ? camera->forward : camera->CalcPixelDir(lastx,lasty));
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
		//info->AddLine("Unknown cursor: %s", cursorText.c_str());		
		mc = cursors[""];
	}

	mc->Draw(lastx, lasty);
}

std::string CMouseHandler::GetCurrentTooltip(void)
{
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers->begin();ri!=inputReceivers->end();++ri){
		if((*ri)->IsAbove(lastx,lasty)){
			std::string s=(*ri)->GetTooltip(lastx,lasty);
			if(s!="")
				return s;
		}
	}
	std::string s=selectedUnits.GetTooltip();
	if(s!="")
		return s;

	CUnit* unit=0;
	float dist=helper->GuiTraceRay(camera->pos,dir,9000,unit,20,false);

	if(unit){
		s=unit->tooltip;
		char tmp[500];
		sprintf(tmp,"\nHealth %.0f/%.0f \nExperience %.2f Cost %.0f Range %.0f \n\xff\xd3\xdb\xffMetal: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f\xff\xd3\xdb\xff Energy: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f",
			unit->health,unit->maxHealth,unit->experience,unit->metalCost+unit->energyCost/60,unit->maxRange, unit->metalMake, unit->metalUse, unit->energyMake, unit->energyUse);
		s+=tmp;		
		return s;
	} else {
		if(dist<8900){
			float3 pos=camera->pos+dir*dist;
			char tmp[500];
			sprintf(tmp,"Pos %.0f %.0f Elevation %.0f",pos.x,pos.z,pos.y);
			return tmp;
		} else {
			return "";
		}
	}
}

void CMouseHandler::EmptyMsgQueUpdate(void)
{
#ifndef USE_GLUT
	if(hide && mouseMovedFromCenter){
		RECT cr;
		if(GetWindowRect(hWnd,&cr)==0)
			MessageBox(0,"mouse error","",0);
		SetCursorPos(gu->screenx/2+cr.left+4*!fullscreen,gu->screeny/2+cr.top+23*!fullscreen);
		internalMouseMove=true;				//this only works if the msg que is empty of mouse moves, so someone should figure out something better
		mouseMovedFromCenter=false;
	}
#else
#warning add a way to set cursor pos
#endif
}
