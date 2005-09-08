#include "GUIgame.h"
#include "GUIcontroller.h"
#include "Camera.h"
#include "Ground.h"
#include "MouseHandler.h"
#include "GameHelper.h"
#include "UnitHandler.h"
#include "UnitDefHandler.h"
#include "SelectedUnits.h"
#include "command.h"
#include "float3.h"
#include "Unit.h"
#include "Feature.h"
#include "TextureHandler.h"
#include "3DOParser.h"
#include "Team.h"
#include "BaseGroundDrawer.h"

// things that I have added
#include "Net.h"
#include "GroupHandler.h"
#include "Bitmap.h"
#include "FileHandler.h"
#include "ShadowHandler.h"
#include "MouseHandler.h"
#include "BaseSky.h"
#include "InMapDraw.h"
#include "Game.h"
#include "Sound.h"
#include "LoadSaveHandler.h"
#ifndef NO_AVI
#include "AVIGenerator.h"
#endif
#include "CameraController.h"
#include "GameController.h"
#include "UnitDrawer.h"
#include "GuiHandler.h"
#include "Unit.h"
#include "UnitDef.h"
#include "Weapon.h"
#include "WeaponDefHandler.h"
#include "FeatureHandler.h"
//#include "mmgr.h"
#include "GUICommandPool.h"
#include "perf.h"
#include "SDL_types.h"
#include "SDL_keysym.h"

extern Uint8 *keys;
extern bool	globalQuit;
extern CGame* game;

extern map<string, string> bindings;

bool mouseHandlerMayDoSelection=true;

bool Selector::BeginSelection(const float3& dir)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+dir*9000);

	isDragging=false;
	
	if(dist<0)
		dist=9000;

	begin=camera->pos+dir*dist;
	end=begin;

	isDragging=true;
	return true;
}

bool Selector::UpdateSelection(const float3& dir)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+dir*9000);

	if(dist<0)
		dist=9000;

	end=camera->pos+dir*dist;
	return isDragging;
}

void Selector::GetSelection(vector<CUnit*>&selected)
{
	selected.clear();

	if(!isDragging) return;

	float3 pos1=begin;
	float3 pos2=end;
	
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
	int addedunits=0;
	for(ui=gs->teams[gu->myTeam]->units.begin();ui!=gs->teams[gu->myTeam]->units.end();++ui)
	{
		float3 vec=(*ui)->midPos-camera->pos;
		if(vec.dot(norm1)<0 && vec.dot(norm2)<0 && vec.dot(norm3)<0 && vec.dot(norm4)<0)
		{
				selected.push_back(*ui);
		}
	}

}

void Selector::EndSelection()
{
	if(!isDragging) return;

	isDragging=false;
	
}

void Selector::DrawSelection()
{
	float3 pos1=begin;
	float3 pos2=end;
	float3 dir1=pos1-camera->pos;
	dir1.Normalize();
	float3 dir2=pos2-camera->pos;
	dir2.Normalize();

	float3 dir1S=camera->right*(dir1.dot(camera->right))/dir1.dot(camera->forward);
	float3 dir1U=camera->up*(dir1.dot(camera->up))/dir1.dot(camera->forward);

	float3 dir2S=camera->right*(dir2.dot(camera->right))/dir2.dot(camera->forward);
	float3 dir2U=camera->up*(dir2.dot(camera->up))/dir2.dot(camera->forward);

	camera->Update(false);

	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5,1,0.5,0.5);

	glBegin(GL_QUADS);
		glVertexf3(camera->pos+dir1U*30+dir1S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir1S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir2S*30+camera->forward*30);
		glVertexf3(camera->pos+dir1U*30+dir2S*30+camera->forward*30);
		
		glVertexf3(camera->pos+dir1U*30+dir1S*30+camera->forward*30);		
		glVertexf3(camera->pos+dir1U*30+dir2S*30+camera->forward*30);
		glVertexf3(camera->pos+dir2U*30+dir2S*30+camera->forward*30);		
		glVertexf3(camera->pos+dir2U*30+dir1S*30+camera->forward*30);		
	glEnd();
}

void Selector::DrawArea()
{
	float radius=begin.distance2D(end);
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5,1,0.5,0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glBegin(GL_TRIANGLE_FAN);
		glVertexf3(begin);
		for(int a=0;a<=40;++a)
		{
			float3 p(cos(a*2*PI/40)*radius,0,sin(a*2*PI/40)*radius);
			p+=begin;		
			p.y=ground->GetHeight(p.x,p.z);
			glVertexf3(p);
		}
	glEnd();
}

void Selector::DrawFront(float maxSize, float sizeDiv)
{
	float3 pos1=begin;
	float3 pos2=end;

	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5,1,0.5,0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	float3 forward=(pos1-pos2).cross(UpVector);
	forward.Normalize();
	float3 side=forward.cross(UpVector);
	if(pos1.distance2D(pos2)>maxSize){
		pos2=pos1+side*maxSize;
		pos2.y=ground->GetHeight(pos2.x,pos2.z);
	}

	glDisable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
		glVertexf3(pos1+side*25);
		glVertexf3(pos1-side*25);
		glVertexf3(pos1-side*25+forward*50);
		glVertexf3(pos1+side*25+forward*50);

		glVertexf3(pos1+side*40+forward*50);
		glVertexf3(pos1-side*40+forward*50);
		glVertexf3(pos1+forward*100);
		glVertexf3(pos1+forward*100);
	glEnd();
	glEnable(GL_DEPTH_TEST);


	pos1+=pos1-pos2;
	glDisable(GL_FOG);
	glBegin(GL_QUADS);
		glVertexf3(pos1-float3(0,-100,0));
		glVertexf3(pos1+float3(0,-100,0));
		glVertexf3(pos2+float3(0,-100,0));
		glVertexf3(pos2-float3(0,-100,0));
	glEnd();

	if(sizeDiv!=0){
		char c[40];
#ifdef _WIN32
 		itoa(pos1.distance2D(pos2)/sizeDiv,c,10);
#else
 		snprintf(c,39,"%d",pos1.distance2D(pos2)/sizeDiv);
#endif
		mouse->cursorTextRight=c;
	}
	glEnable(GL_FOG);
}

const int dragSensitivity=10;

extern map<int, string> commandNames;
extern map<string, CommandDescription> commandDesc;

#define NOTHING 0
#define DOWN 1
#define DRAGGING 2

GUIgame *guiGameControl;

GUIgame::GUIgame(GUIcontroller *c):GUIpane(0, 0, gu->screenx, gu->screeny), controller(c)
{
	// initialize the commands map
	InitCommands();

	canDrag=false;
	canResize=false;

	commandButton=0;
	mouseLook=false;
	unit=0;
	feature=0;
	position=ZeroVector;
	showingMetalMap=false;

	for(int i=0; i<numButtons; i++)
	{
		buttonAction[i]=NOTHING;	
	}
	
	guiGameControl=this;
}

GUIgame::GUIgame(int x, int y, int w, int h, GUIcontroller *c):GUIpane(x, y, w, h), controller(c)
{
	// initialize the commands map
	InitCommands();

	canDrag=true;
	canResize=true;

	commandButton=0;
	mouseLook=false;

	for(int i=0; i<numButtons; i++)
	{
		buttonAction[i]=NOTHING;	
	}
}

GUIgame::~GUIgame()
{

}

void GUIgame::StopCommand()
{
	currentCommand=NULL;
	if(showingMetalMap)
	{
		groundDrawer->SetExtraTexture(0,0,false);
		showingMetalMap=false;
	}
}

void GUIgame::StartCommand(CommandDescription& cmd)
{
	if(cmd.type==CMDTYPE_ICON)
	{
		Command c;
		InitCommand(c);
		c.id=cmd.id;
		selectedUnits.GiveCommand(c);
	}
	else
	{
		currentCommand=&cmd;

		if(currentCommand->type==CMDTYPE_ICON_BUILDING)
		{
			UnitDef* ud=unitDefHandler->GetUnitByID(-currentCommand->id);
			if(ud->extractsMetal>0 && !groundDrawer->drawMetalMap)
			{
					groundDrawer->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
					showingMetalMap=true;
			}
		}
	}
}

void GUIgame::PrivateDraw()
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	camera->Update(false);

	if(currentCommand)
	{
		if(currentCommand->id<0)
		{
			// draw possible unit placement
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
			if(dist>0)
			{
				UnitDef *unitdef = unitDefHandler->GetUnitByID(-currentCommand->id);

				float3 pos=camera->pos+mouse->dir*dist;
				pos.x=floor(pos.x/(float)SQUARE_SIZE+0.5)*(float)SQUARE_SIZE;
				pos.z=floor(pos.z/(float)SQUARE_SIZE+0.5)*(float)SQUARE_SIZE;

				if(unitdef->floater)
					pos.y = max(ground->GetHeight2(pos.x,pos.z),-unitdef->waterline);
				else
					pos.y=ground->GetHeight2(pos.x,pos.z);
				if(unitdef)
				{


					glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
					glEnable(GL_TEXTURE_2D);
					texturehandler->SetTexture();
					glDepthMask(0);
					S3DOModel* model=unit3doparser->Load3DO((unitdef->model.modelpath).c_str() ,1, gu->myTeam);
					
					if(selector[commandButton].IsDragging())
					{
						// draw unit placement
						std::vector<float3> positions=GetBuildPos(selector[commandButton].Start(), selector[commandButton].End(),unitdef);

						int size=positions.size();
						for(int i=0; i<size; i++)
						{
							if(uh->ShowUnitBuildSquare(positions[i],unitdef))
								glColor4f(0.7,1,1,0.4);
							else
								glColor4f(1,0.5,0.5,0.4);

							glPushMatrix();
							glTranslatef3(positions[i]);
							model->rootobject->DrawStatic();
							glDisable(GL_TEXTURE_2D);
							glDepthMask(1);
							glPopMatrix();
							if(unitdef->weapons.size()>0){	// Draw firerange
							glDisable(GL_TEXTURE_2D);
							glColor4f(1,0.3,0.3,0.7);
							glBegin(GL_LINE_STRIP);
							for(int a=0;a<=40;++a){
								float3 wpos(cos(a*2*PI/40)*unitdef->weapons[0].def->range,0,sin(a*2*PI/40)*unitdef->weapons[0].def->range);
								wpos+=pos;
								float dh=ground->GetHeight(wpos.x,wpos.z)-pos.y;
								float modRange=unitdef->weapons[0].def->range-dh*unitdef->weapons[0].def->heightmod;
								wpos=float3(cos(a*2*PI/40)*(modRange),0,sin(a*2*PI/40)*(modRange));
								wpos+=pos;
								wpos.y=ground->GetHeight(wpos.x,wpos.z)+8;
								glVertexf3(wpos);
								}
							glEnd();
							}	

						}
					}
					else
					{
						if(uh->ShowUnitBuildSquare(pos,unitdef))
							glColor4f(0.7,1,1,0.4);
						else
							glColor4f(1,0.5,0.5,0.4);

						// draw single unit
						glPushMatrix();
						glTranslatef3(pos);
						model->rootobject->DrawStatic();
						glDisable(GL_TEXTURE_2D);
						glDepthMask(1);
						glPopMatrix();						
						if(unitdef->weapons.size()>0){	// Draw firerange
						glDisable(GL_TEXTURE_2D);
						glColor4f(1,0.3,0.3,0.7);
						glBegin(GL_LINE_STRIP);
						for(int a=0;a<=40;++a){
							float3 wpos(cos(a*2*PI/40)*unitdef->weapons[0].def->range,0,sin(a*2*PI/40)*unitdef->weapons[0].def->range);
							wpos+=pos;
							float dh=ground->GetHeight(wpos.x,wpos.z)-pos.y;
							float modRange=unitdef->weapons[0].def->range-dh*unitdef->weapons[0].def->heightmod;
							wpos=float3(cos(a*2*PI/40)*(modRange),0,sin(a*2*PI/40)*(modRange));
							wpos+=pos;
							wpos.y=ground->GetHeight(wpos.x,wpos.z)+8;
							glVertexf3(wpos);
							}
						glEnd();
						}
						if(unitdef->extractRange>0){	//draw range
						glDisable(GL_TEXTURE_2D);
						glColor4f(1,0.3,0.3,0.7);
						glBegin(GL_LINE_STRIP);
						for(int a=0;a<=40;++a){
							float3 wpos(cos(a*2*PI/40)*unitdef->extractRange,0,sin(a*2*PI/40)*unitdef->extractRange);
							wpos+=pos;
							wpos.y=ground->GetHeight(wpos.x,wpos.z)+8;
							glVertexf3(wpos);
						}
						glEnd();
					}
					}
				}
			}
		}
		else
		{
			switch(currentCommand->type)
			{
				case CMDTYPE_ICON_FRONT:
					if(selector[commandButton].IsDragging())
					{
						float maxSize=1000, sizeDiv=0;
						if(!currentCommand->params.empty())
						{
							maxSize=atof(currentCommand->params[0].c_str());
							if(currentCommand->params.size()>1)
								sizeDiv=atof(currentCommand->params[1].c_str());
						}

						selector[commandButton].DrawFront(maxSize, sizeDiv);
					}
					break;
	//			case CMDTYPE_ICON_UNIT:
	//					if(selector[commandButton].IsDragging())
	//					{
	//						selector[commandButton].DrawSelection();
	//					}
	//					break;
				default:
					if(selector[commandButton].IsDragging())
					{
						selector[commandButton].DrawArea();
					}
					break;
			}
		}
		if (currentCommand->id == CMD_ATTACK) {
			for(std::set<CUnit*>::iterator si=selectedUnits.selectedUnits.begin();si!=selectedUnits.selectedUnits.end();++si){
				CUnit* unit=*si;
				if(unit->maxRange>0 && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating)){
					glDisable(GL_TEXTURE_2D);
					glColor4f(1,0.3,0.3,0.7);
					glBegin(GL_LINE_STRIP);
					float h=unit->pos.y;
					for(int a=0;a<=40;++a){
						float3 pos(cos(a*2*PI/40)*unit->maxRange,0,sin(a*2*PI/40)*unit->maxRange);
						pos+=unit->pos;
						float dh=ground->GetHeight(pos.x,pos.z)-h;
						pos=float3(cos(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod),0,sin(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod));
						pos+=unit->pos;
						pos.y=ground->GetHeight(pos.x,pos.z)+8;
						glVertexf3(pos);
					}
					glEnd();
				}
			}
		}
	}
	if(keys[SDLK_LSHIFT]){
		CUnit* unit=0;
		float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,false);
		if(unit && ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating)){		//draw weapon range
			if(unit->maxRange>0){
				glDisable(GL_TEXTURE_2D);
				glColor4f(1,0.3,0.3,0.7);
				glBegin(GL_LINE_STRIP);
				float h=unit->pos.y;
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->maxRange,0,sin(a*2*PI/40)*unit->maxRange);
					pos+=unit->pos;
					float dh=ground->GetHeight(pos.x,pos.z)-h;
					pos=float3(cos(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod),0,sin(a*2*PI/40)*(unit->maxRange-dh*unit->weapons.front()->heightMod));
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
			}
			if(unit->unitDef->decloakDistance>0){			//draw decloak distance
				glDisable(GL_TEXTURE_2D);
				glColor4f(0.3,0.3,1,0.7);
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->unitDef->decloakDistance,0,sin(a*2*PI/40)*unit->unitDef->decloakDistance);
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
			}
			if(unit->unitDef->kamikazeDist>0){			//draw self destruct distance
				glDisable(GL_TEXTURE_2D);
				glColor4f(0.8,0.8,0.1,0.7);
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=40;++a){
					float3 pos(cos(a*2*PI/40)*unit->unitDef->kamikazeDist,0,sin(a*2*PI/40)*unit->unitDef->kamikazeDist);
					pos+=unit->pos;
					pos.y=ground->GetHeight(pos.x,pos.z)+8;
					glVertexf3(pos);
				}
				glEnd();
			}
		}
	}


	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}


bool GUIgame::MouseDownAction(int x, int y, int button)
{
	const float3& dir=mouse->dir;

	down[button].x=x;
	down[button].y=y;
	down[button].dir=dir;

	buttonAction[button]=DOWN;
	
	commandButton=button;
	
	return false;
}

bool GUIgame::MouseUpAction(int x, int y, int button)
{
	//commandButton=0;

	if(!buttonAction[button])
		return false;

	if(buttonAction[button]!=DRAGGING)
	{
		mouseHandlerMayDoSelection=true;
		buttonAction[button]=NOTHING;
		// the event was not a drag, so it must be a click. send event
		char buf[20];
		sprintf(buf, "mouse%i_click", button);
		string command=GUIcontroller::Modifiers()+buf;
		return MouseEvent(bindings[command], x, y, button);
	}
	else
	{
		buttonAction[button]=NOTHING;
		// the event is a drag.
		if(selector[button].IsDragging())
		{
			string command=selector[button].DragCommand();

			selector[button].UpdateSelection(mouse->dir);
			mouseHandlerMayDoSelection=true;

			if(currentCommand)
			{
				// todo: implement areas
				if(1)
				{
					// FinishCommand(units);

					// if the command was the "replace selection" kind (i.e. without shift),
					// the selected command is finished.

					if(command=="drag_command_add"||command=="drag_command_replace"||command=="drag_defaultcommand")
					{
						DispatchCommand(GetCommand());
					}


					if(command=="drag_command_replace"||command=="drag_defaultcommand")
						StopCommand();
					// else it was "add_selection", so the currentCommand stays.

					selector[button].EndSelection();
					
					return true;
				}
			}

			// either there is no command active, or it doesn't take several units (or an area or whatever)
			StopCommand();

			if(command=="replace_selection")
				selectedUnits.ClearSelected();

			vector<CUnit*> units;
			
			selector[button].GetSelection(units);
			selector[button].EndSelection();

			vector<CUnit*>::iterator i=units.begin();
			vector<CUnit*>::iterator e=units.end();
			for(;i!=e; i++)
			{
				if((*i)->team==gu->myTeam)
					selectedUnits.AddUnit(*i);
			}


			return true;
		}
	}

	return false;
}

bool GUIgame::DirectCommand(const float3& p, int button)
{
	unit=0;
	feature=0;
	
	float3 over=float3(p.x, 8000, p.z);
	float3 dir=float3(0, -1, 0);
	
	float dist=ground->LineGroundCol(over, over+dir*9000);
	
	position=over+dir*dist;
	
	helper->GuiTraceRay(over,dir,9000,unit,20,true);
	helper->GuiTraceRayFeature(over,dir,9000,feature);
	
	buttonAction[button]=NOTHING;
	// the event was not a drag, so it must be a click. send event
	char buf[20];
	sprintf(buf, "mouse%i_click", button);
	string command=GUIcontroller::Modifiers()+buf;
	return MouseEvent(bindings[command], x, y, button);	
}

bool GUIgame::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
{
	unit=0;
	feature=0;
	position=ZeroVector;

	float3 dir=mouse->dir;

	float dist=ground->LineGroundCol(camera->pos,camera->pos+dir*9000);
	
	if(dist<9000)
		position=camera->pos+dir*dist;

	float dist2=helper->GuiTraceRay(camera->pos,dir,9000,unit,20,true);
	float dist3=helper->GuiTraceRayFeature(camera->pos,dir,9000,feature);

	for(int button=0; button<numButtons; button++)
	{
		if(buttonAction[button]!=NOTHING)
		{
			char buf[20];
			sprintf(buf, "mouse%i_drag", button);

			string command=bindings[GUIcontroller::Modifiers()+buf];

			if(buttonAction[button]==DOWN)
			{
				// if we could drag and the mouse has moved more than dragSensitivity pixels,
				// we start the actual dragging process.
				if(abs(x-down[button].x)>dragSensitivity||abs(y-down[button].y)>dragSensitivity)
				{
					buttonAction[button]=DRAGGING;

					if(command=="replace_selection")
					{
						selector[button].BeginSelection(down[button].dir);
						selector[button].SetDragCommand(command);
						return true;
					}
					else if(command=="add_selection")
					{
						selector[button].BeginSelection(down[button].dir);
						selector[button].SetDragCommand(command);
						return true;
					}
					else if(command=="drag_command_add"||command=="drag_command_replace")
					{
						if(IsDragCommand(currentCommand))
						{
							mouseHandlerMayDoSelection=false;
							selector[button].BeginSelection(down[button].dir);
							selector[button].SetDragCommand(command);
							return true;
						}
					}
					else if(command=="drag_defaultcommand")
					{
						EventAction("default_command");
						if(IsDragCommand(currentCommand))
						{
							mouseHandlerMayDoSelection=false;
							selector[button].BeginSelection(down[button].dir);
							selector[button].SetDragCommand(command);
							return true;
						}
					}
				}
			}
			if(buttonAction[button]==DRAGGING)
			{
				// update dragging coordinates
				return selector[button].UpdateSelection(mouse->dir);
			}
		}
	}

	return false;
}

bool GUIgame::EventAction(const string& command)
{
	// there needs to be a better solution to this.  right now, camera speed manipulation is command based which can be awkward
	// for instance: press shift, then a button, then release both at the same time, or shift first.  shift__release
	// if never fired, and camMove[6] or 7 is not changed back to false, so this needs to be done
	game->camMove[6] = game->camMove[7] = false;

	int id = FindCommand(command);

	if ( id == -1 )
	{
		return false;
	}
	else if(id==COMMAND_stop_command)
	{
		StopCommand();
	}
	else if(id==COMMAND_deselect_all)		// deselect units
	{
		selectedUnits.ClearSelected();
	}
	else if(id==COMMAND_cancel_or_deselect)	// if there's a command, deselect it
	{										// else clear the current selection
		if(currentCommand)
			StopCommand();
		else if ( selectedUnits.selectedUnits.size() != 0 )
			selectedUnits.ClearSelected();		
		else
		{
			guicontroller->AddText("Use shift-esc to exit");
		}
	}
	else if(id==COMMAND_default_command) // if there's no command, either
	{									// add unit or take default command
		if(!currentCommand)
		{
			//vector<CommandDescription> commands=selectedUnits.GetAvailableCommands();
			int cmd_id = selectedUnits.GetDefaultCmd(unit,feature);	
			string& name=commandNames[cmd_id];

			StartCommand(commandDesc["selection_"+name]);
			temporaryCommand=true;
		}
	}
	else if(id==COMMAND_dispatch_command)	// dispatch the currently selected command
	{
		if(!currentCommand)
			return false;

		DispatchCommand(GetCommand());
		return true;
		
	}
	else if(id==COMMAND_maybe_add_selection)	// add unit, but only if no command current
	{
		if(unit&&!currentCommand)
		{
			if(unit->team==gu->myTeam)
			{
				selectedUnits.AddUnit(unit);
				StopCommand();
			}
		}
	}
	else if(id==COMMAND_maybe_replace_selection)	// replace unit, but only if no command current
	{
		if(unit&&!currentCommand)
		{
			if(unit->team==gu->myTeam)
			{
				selectedUnits.ClearSelected();
				selectedUnits.AddUnit(unit);
				StopCommand();
			}
		}
	}

	if(id==COMMAND_showhealthbars){
		unitDrawer->showHealthBars=!unitDrawer->showHealthBars;
	}

	else if(id==COMMAND_group0)	
	{
		grouphandler->GroupCommand(0);
	}
	else if(id==COMMAND_group1)	
	{
		grouphandler->GroupCommand(1);
	}
	else if(id==COMMAND_group2)	
	{
		grouphandler->GroupCommand(2);
	}
	else if(id==COMMAND_group3)	
	{
		grouphandler->GroupCommand(3);
	}
	else if(id==COMMAND_group4)	
	{
		grouphandler->GroupCommand(4);
	}
	else if(id==COMMAND_group5)	
	{
		grouphandler->GroupCommand(5);
	}
	else if(id==COMMAND_group6)	
	{
		grouphandler->GroupCommand(6);
	}
	else if(id==COMMAND_group7)	
	{
		grouphandler->GroupCommand(7);
	}
	else if(id==COMMAND_group8)	
	{
		grouphandler->GroupCommand(8);
	}
	else if(id==COMMAND_group9)	
	{
		grouphandler->GroupCommand(9);
	}
	else if (id==COMMAND_quit){
			globalQuit=true;
	}
	else if (id==COMMAND_togglelos){
		groundDrawer->ToggleLosTexture();
	}
	else if(id==COMMAND_mousestate){
		mouse->ToggleState(keys[SDLK_LSHIFT] || keys[SDLK_LCTRL]);
	}

	else if (id==COMMAND_updatefov)
		groundDrawer->updateFov=!groundDrawer->updateFov;

	else if (id==COMMAND_drawtrees)
		treeDrawer->drawTrees=!treeDrawer->drawTrees;

	else if (id==COMMAND_dynamicsky)
		sky->dynamicSky=!sky->dynamicSky;

//	else if (s=="hideinterface")
//		hideInterface=!hideInterface;

	else if (id==COMMAND_increaseviewradius){
		groundDrawer->viewRadius+=2;
	//	(*guicontroller) << "ViewRadius is now " << groundDrawer->viewRadius << "\n";
	}

	else if (id==COMMAND_decreaseviewradius){
		groundDrawer->viewRadius-=2;
	//	(*guicontroller) << "ViewRadius is now " << groundDrawer->viewRadius << "\n";
	}

	else if (id==COMMAND_moretrees){
		groundDrawer->baseTreeDistance+=0.2f;
	//	(*guicontroller) << "Base tree distance " << groundDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}

	else if (id==COMMAND_lesstrees){
		groundDrawer->baseTreeDistance-=0.2f;
	//	(*guicontroller) << "Base tree distance " << groundDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}

	else if (id==COMMAND_moreclouds){
		sky->cloudDensity*=0.95f;
	//	(*guicontroller) << "Cloud density " << 1/sky->cloudDensity << "\n";
	}

	else if (id==COMMAND_lessclouds){
		sky->cloudDensity*=1.05f;
	//	(*guicontroller) << "Cloud density " << 1/sky->cloudDensity << "\n";
	}

	else if (id==COMMAND_screenshot){
		int x=gu->screenx;
		if(gu->screenx%4)
			gu->screenx+=4-gu->screenx%4;
		unsigned char* buf=new unsigned char[gu->screenx*gu->screeny*4];
		glReadPixels(0,0,gu->screenx,gu->screeny,GL_RGBA,GL_UNSIGNED_BYTE,buf);
		CBitmap b(buf,gu->screenx,gu->screeny);
		b.ReverseYAxis();
		string name;
		for(int a=0;a<99;++a){
			char t[50];
			snprintf(t,10,"%d",a);
			name=string("screen")+t+".jpg";
			CFileHandler ifs(name);
			if(!ifs.FileExists())
				break;
		}
		b.Save(name);
		delete[] buf;
		gu->screenx=x;
	}

	else if(id==COMMAND_speedup)
	{
		float speed=gs->userSpeedFactor;
		if(speed<1){
			speed/=0.8;
			if(speed>0.99)
				speed=1;
		}else 
			speed+=0.5;
		if(!net->playbackDemo){
			netbuf[0]=NETMSG_USER_SPEED;
			*((float*)&netbuf[1])=speed;
			net->SendData(netbuf,5);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;	
			guicontroller->AddText("Speed %f \n",gs->speedFactor);			
			}

	}
	else if (id==COMMAND_slowdown){
		float speed=gs->userSpeedFactor;
		if(speed<=1){
			speed*=0.8;
			if(speed<0.1)
				speed=0.1;
		}else 
			speed-=0.5;
		if(!net->playbackDemo){
			netbuf[0]=NETMSG_USER_SPEED;
			*((float*)&netbuf[1])=speed;
			net->SendData(netbuf,5);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			guicontroller->AddText("Speed %f \n",gs->speedFactor);
		}
	}
	else 
	#ifdef DIRECT_CONTROL_ALLOWED
	if(id==COMMAND_controlunit){
		Command c;
		c.id=CMD_STOP;
		c.options=0;
		selectedUnits.GiveCommand(c,false);		//force it to update selection and clear order que

		netbuf[0]=NETMSG_DIRECT_CONTROL;
		netbuf[1]=gu->myPlayerNum;
		net->SendData(netbuf,2);
	} else
	#endif
	if (id==COMMAND_showshadowmap){
		shadowHandler->showShadowMap=!shadowHandler->showShadowMap;
	}

	else if (id==COMMAND_showstandard){
		groundDrawer->SetExtraTexture(0,0,false);
	}

	else if (id==COMMAND_showelevation){	
		groundDrawer->SetHeightTexture();
	}
	else if (id==COMMAND_lastmsgpos){
		mouse->currentCamController->SetPos(guicontroller->lastMsgPos);
		mouse->inStateTransit=true;
		mouse->transitSpeed=0.5;
	}
	else if (id==COMMAND_showmetalmap){
		groundDrawer->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);		
	}
	else if (id==COMMAND_showpathmap && gs->cheatEnabled){
		groundDrawer->SetPathMapTexture();
	}

	else if(id==COMMAND_drawinmapon){
		inMapDrawer->keyPressed=true;
	}
	
	else if(id==COMMAND_drawinmapoff){
		inMapDrawer->keyPressed=false;
	}

	else if (id==COMMAND_pausegame){      
		netbuf[0]=NETMSG_PAUSE;
		netbuf[1]=!gs->paused;
		netbuf[2]=gu->myPlayerNum;
		net->SendData(netbuf,3);
		perfCounter(&(game->lastframe));
	}
	else if (id==COMMAND_singlestep){	
		game->bOneStep=true;
	}
//	else if (id==COMMAND_chat"){
//		activeController->userWriting=true;
//		userPrompt="Say: ";
//		game->chatting=true;
//		if(k!=SDLK_RETURN)
//			activeController->ignoreNextChar=true;
//	}
	else if (id==COMMAND_debug)
		gu->drawdebug=!gu->drawdebug;

	else if (id==COMMAND_track)	
		game->trackingUnit=!game->trackingUnit;

	else if (id==COMMAND_nosound)
		sound->noSound=!sound->noSound;
	
	else if(id==COMMAND_savegame){
		CLoadSaveHandler ls;
		ls.SaveGame("Test.ssf");
	}

#ifndef NO_AVI
	else if (id==COMMAND_createvideo){
		if(game->creatingVideo){
			game->creatingVideo=false;
			game->aviGenerator->ReleaseEngine();
			delete game->aviGenerator;
			game->aviGenerator=0;
			guicontroller->AddText("Finished avi");
		} else {
			game->creatingVideo=true;
			string name;
			for(int a=0;a<99;++a){
				char t[50];
				itoa(a,t,10);
				name=string("video")+t+".avi";
				CFileHandler ifs(name);
				if(!ifs.FileExists())
					break;
			}
			int x=gu->screenx;
			x-=gu->screenx%4;

			int y=gu->screeny;
			y-=gu->screeny%4;

			BITMAPINFOHEADER bih;
			bih.biSize=sizeof(BITMAPINFOHEADER);
			bih.biWidth=x;
			bih.biHeight=y;
			bih.biPlanes=1;
			bih.biBitCount=24;
			bih.biCompression=BI_RGB;
			bih.biSizeImage=0;
			bih.biXPelsPerMeter=1000;
			bih.biYPelsPerMeter=1000;
			bih.biClrUsed=0;
			bih.biClrImportant=0;

			game->aviGenerator=new CAVIGenerator();
			game->aviGenerator->SetFileName(name.c_str());
			game->aviGenerator->SetRate(30);
			game->aviGenerator->SetBitmapHeader(&bih);
			Sint32 hr=game->aviGenerator->InitEngine();
			if(hr!=AVIERR_OK){  
				game->creatingVideo=false;
			} else {
				guicontroller->AddText("Recording avi to %s size %i %i",name.c_str(),x,y);
			}
		}
	}
#endif 
	else if (id==COMMAND_start_moveforward)
		game->camMove[0]=true;

	else if (id==COMMAND_start_moveback)
		game->camMove[1]=true;

	else if (id==COMMAND_start_moveleft)
		game->camMove[2]=true;

	else if (id==COMMAND_start_moveright)
		game->camMove[3]=true;

	else if (id==COMMAND_start_moveup)
		game->camMove[4]=true;

	else if (id==COMMAND_start_movedown)
		game->camMove[5]=true;

	else if (id==COMMAND_start_movefast)
	{
		game->camMove[6]=true;
		game->camMove[7]=false;
	}

	else if (id==COMMAND_start_moveslow)
	{
		game->camMove[7]=true;
		game->camMove[6]=false;
	}

	else if (id==COMMAND_end_moveforward)
		game->camMove[0]=false;

	else if (id==COMMAND_end_moveback)
		game->camMove[1]=false;

	else if (id==COMMAND_end_moveleft)
		game->camMove[2]=false;

	else if (id==COMMAND_end_moveright)
		game->camMove[3]=false;

	else if (id==COMMAND_end_moveup)
		game->camMove[4]=false;

	else if (id==COMMAND_end_movedown)
		game->camMove[5]=false;

	else if (id==COMMAND_end_movefast || id==COMMAND_end_moveslow)
	{
		game->camMove[6]=false;
		game->camMove[7]=false;
	}

	else if ( id==COMMAND_trajectory )
	{
	}

	return false;
}


bool GUIgame::MouseEvent(const string& event1, int x, int y, int button)
{
	temporaryCommand=false;
	
	bool res=GUIcontroller::Event(event1);

	if(temporaryCommand)
		StopCommand();
	return res;
}




void GUIgame::SelectCursor()
{
	if(LocalInside(x, y))
	{

		if(currentCommand)	// show command cursor
		{
			mouse->cursorText=currentCommand->name;
		}
		else				// show default command cursor or add cursor
		{

			if(unit)
				if(unit->team==gu->myTeam)	// could add this unit
				{
					mouse->cursorText="";	// TODO: where is the selection cursor?
				//	return;
				}

			int cmd_id = selectedUnits.GetDefaultCmd(unit,feature);

			string name="selection_" + commandNames[cmd_id];
			if(commandDesc.find(name)!=commandDesc.end())
			{
				mouse->cursorText=commandDesc[name].name;
			}
		}
	}
}

string GUIgame::Tooltip()
{
	char tmp[500];
	if(unit && (unit->team==gu->myTeam))
	{
		tooltip=unit->tooltip;

		sprintf(tmp,"\nHealth %.0f/%.0f \nExperience %.2f Cost %.0f Range %.0f \n\xff\xd3\xdb\xffMetal: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f\xff\xd3\xdb\xff Energy: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f",
			unit->health,unit->maxHealth,unit->experience,unit->metalCost+unit->energyCost/60,unit->maxRange, unit->metalMake, unit->metalUse, unit->energyMake, unit->energyUse);
		tooltip+=tmp;
	} else
		if ((feature) && (feature->def)) 
	{
		tooltip="";
		if (feature->def->metal > 0){ 
			sprintf(tmp , "Metal : %.0f \n" , feature->def->metal);
			tooltip+=tmp;
		} 
		if (feature->def->energy > 0){
			sprintf(tmp , "Energy : %.0f \n" , feature->def->energy);
			tooltip+=tmp;
		}
		sprintf(tmp , "Health : %.0f \n" , feature->health);
		tooltip+=tmp;
		if (feature->reclaimLeft < 1){
			sprintf(tmp , "Reclaim Left : %.2f \n" , feature->reclaimLeft);
			tooltip+=tmp;
		}		
	}
	else
		tooltip="";
		
	return tooltip;
}

void GUIgame::InitCommand(Command& c)
{
	c.options=0;
	if(keys[SDLK_LSHIFT])
		c.options|=SHIFT_KEY;
	if(keys[SDLK_LCTRL])
		c.options|=CONTROL_KEY;
	if(keys[SDLK_LALT])
		c.options|=ALT_KEY;
}

void GUIgame::DispatchCommand(Command c)
{
	if(c.id<0&&(c.params.size()>3))
	{
		// this is a multi-position build command. split it up in several single-unit build commands
		Command temp=c;
		
		InitCommand(temp);

		temp.params.push_back(c.params[0]);
		temp.params.push_back(c.params[1]);
		temp.params.push_back(c.params[2]);

		// the first command is given like usual
		selectedUnits.GiveCommand(temp);
	
		// the following commands are enqueued (like with shift key pressed)
		temp.options|=SHIFT_KEY;
	
		int size=c.params.size()/3;

		for(int i=1; i<size; i++)
		{
			temp.params.clear();
			
			temp.params.push_back(c.params[i*3+0]);
			temp.params.push_back(c.params[i*3+1]);
			temp.params.push_back(c.params[i*3+2]);

			selectedUnits.GiveCommand(temp);
		}				
		return;
	}

	selectedUnits.GiveCommand(c);
}



Command GUIgame::GetCommand()
{
	Command ret;
	Command defaultRet;


	int button=commandButton;

	defaultRet.id=CMD_STOP;

	CommandDescription *desc=currentCommand;

	if(!currentCommand)
	{
		int cmd_id = selectedUnits.GetDefaultCmd(unit,feature);

		string name="selection_" + commandNames[cmd_id];
		if(commandDesc.find(name)!=commandDesc.end())
			desc=&(commandDesc[name]);
	}

	if(!desc)
		return defaultRet;

	InitCommand(ret);

	ret.id=desc->id;

	switch(desc->type)
	{
		case CMDTYPE_ICON_MAP:
		{
			if(position==ZeroVector)
				return defaultRet;
			ret.params.push_back(position.x);
			ret.params.push_back(position.y);
			ret.params.push_back(position.z);
			return ret;
		}

		case CMDTYPE_ICON_BUILDING:
		{
		//	button=1;
			if(selector[button].IsDragging())
			{
				UnitDef *unitdef = unitDefHandler->GetUnitByID(-desc->id);

				std::vector<float3> positions=GetBuildPos(selector[button].Start(), selector[button].End(), unitdef);

				int size=positions.size();
				for(int i=0; i<size; i++)
				{
					//MakeBuildPos(positions[i],unitdef);
					ret.params.push_back(positions[i].x);
					ret.params.push_back(positions[i].y);
					ret.params.push_back(positions[i].z);
				}
			}
			else
			{
				if(position==ZeroVector)
					return defaultRet;
				ret.params.push_back(position.x);
				ret.params.push_back(position.y);
				ret.params.push_back(position.z);
			}
		//	button=commandButton;
			return ret;
		}
		
		case CMDTYPE_ICON_UNIT: 
		{
	//		if(selector[button].IsDragging())
	//		{
	//			vector<CUnit*> units;
	//			selector[button].GetSelection(units);
	//			
	//			int size=units.size();
	//			for(int i=0; i<size; i++)
	//				ret.params.push_back(units[i]->id);
	//		}
	//		else
			{
				if(!unit)
					return defaultRet;

				ret.params.push_back(unit->id);
			}
			return ret;
			
		}

		case CMDTYPE_ICON_UNIT_OR_MAP: 
		{
			if(unit)	// clicked on an unit
			{
				ret.params.push_back(unit->id);
			}
			else
			{			// clicked in map
				if(position==ZeroVector)
					return defaultRet;
				ret.params.push_back(position.x);
				ret.params.push_back(position.y);
				ret.params.push_back(position.z);
			}
			return ret;
		}

		case CMDTYPE_ICON_FRONT:
		{
			if(!selector[button].IsDragging())
				return defaultRet;

			float3 pos=selector[button].Start();

			ret.params.push_back(pos.x);
			ret.params.push_back(pos.y);
			ret.params.push_back(pos.z);

			float3 pos2=selector[button].End();
			if(!desc->params.empty() && pos.distance2D(pos2)>atoi(desc->params[0].c_str()))
			{
				float3 dif=pos2-pos;
				dif.Normalize();
				pos2=pos+dif*atoi(desc->params[0].c_str());
			}

			ret.params.push_back(pos2.x);
			ret.params.push_back(pos2.y);
			ret.params.push_back(pos2.z);

			return ret;
		}

		case CMDTYPE_ICON_UNIT_OR_AREA:
		case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
		case CMDTYPE_ICON_AREA:
		{
			float maxRadius=100000;

			// read maximum radius from description
			if(desc->params.size()==1)
				maxRadius=atof(desc->params[0].c_str());


			if(!selector[button].IsDragging())
			{
				if (feature && desc->type==CMDTYPE_ICON_UNIT_FEATURE_OR_AREA)
				{
					ret.params.push_back(MAX_UNITS+feature->id);
				}
				else if (unit && desc->type!=CMDTYPE_ICON_AREA)
				{
					ret.params.push_back(unit->id);
				}
				else 
				{
					if(position==ZeroVector)
						return defaultRet;
					ret.params.push_back(position.x);
					ret.params.push_back(position.y);
					ret.params.push_back(position.z);
					ret.params.push_back(0);//zero radius
				}
			}
			else 
			{		
				float3 pos1=selector[button].Start();

				ret.params.push_back(pos1.x);
				ret.params.push_back(pos1.y);
				ret.params.push_back(pos1.z);
				
				float3 pos2=selector[button].End();
				ret.params.push_back(min(maxRadius,pos1.distance2D(pos2)));
			}
			return ret;
		}

		default:
			;
	}

	return defaultRet;
}


std::vector<float3> GUIgame::GetBuildPos(float3 start, float3 end,UnitDef* unitdef)
{
	std::vector<float3> ret;

	start=helper->Pos2BuildPos(start,unitdef);
	end=helper->Pos2BuildPos(end,unitdef);

	
	CUnit* unit=0;
	float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,true);

	if(unit && keys[SDLK_LSHIFT] && keys[SDLK_LCTRL]){		//circle build around building
		UnitDef* unitdef2=unit->unitDef;
		float3 pos2=unit->pos;
		pos2=helper->Pos2BuildPos(pos2,unitdef2);
		start=pos2;
		end=pos2;
		start.x-=(unitdef2->xsize/2)*SQUARE_SIZE;
		start.z-=(unitdef2->ysize/2)*SQUARE_SIZE;
		end.x+=(unitdef2->xsize/2)*SQUARE_SIZE;
		end.z+=(unitdef2->ysize/2)*SQUARE_SIZE;
		
		float3 pos=start;
		for(;pos.x<=end.x;pos.x+=unitdef->xsize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x+=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z-=(unitdef->ysize/2)*SQUARE_SIZE;
			p2=helper->Pos2BuildPos(p2,unitdef);
			ret.push_back(p2);
		}
		pos=start;
		pos.x=end.x;
		for(;pos.z<=end.z;pos.z+=unitdef->ysize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x+=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z+=(unitdef->ysize/2)*SQUARE_SIZE;
			p2=helper->Pos2BuildPos(p2,unitdef);
			ret.push_back(p2);
		}
		pos=end;
		for(;pos.x>=start.x;pos.x-=unitdef->xsize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x-=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z+=(unitdef->ysize/2)*SQUARE_SIZE;
			p2=helper->Pos2BuildPos(p2,unitdef);
			ret.push_back(p2);
		}
		pos=end;
		pos.x=start.x;
		for(;pos.z>=start.z;pos.z-=unitdef->ysize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x-=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z-=(unitdef->ysize/2)*SQUARE_SIZE;
			p2=helper->Pos2BuildPos(p2,unitdef);
			ret.push_back(p2);
		}
	} else if(keys[SDLK_LALT]){			//build a rectangle
		float xsize=unitdef->xsize*8;
		int xnum=(int)((fabs(end.x-start.x)+xsize*1.4)/xsize);
		int xstep=(int)xsize;
		if(start.x>end.x)
			xstep*=-1;

		float zsize=unitdef->ysize*8;
		int znum=(int)((fabs(end.z-start.z)+zsize*1.4)/zsize);
		int zstep=(int)zsize;
		if(start.z>end.z)
			zstep*=-1;

		int zn=0;
		for(float z=start.z;zn<znum;++zn){
			int xn=0;
			for(float x=start.x;xn<xnum;++xn){
				if(!keys[SDLK_LCTRL] || zn==0 || xn==0 || zn==znum-1 || xn==xnum-1){
					float3 pos(x,0,z);
					pos=helper->Pos2BuildPos(pos,unitdef);
					ret.push_back(pos);
				}
				x+=xstep;
			}
			z+=zstep;
		}
	} else {			//build a line
		if(fabs(start.x-end.x)>fabs(start.z-end.z)){
			float step=unitdef->xsize*8;
			float3 dir=end-start;
			if(dir.x==0){
				ret.push_back(start);
				return ret;
			}
			dir/=fabs(dir.x);
			if(keys[SDLK_LCTRL])
				dir.z=0;
			for(float3 p=start;fabs(p.x-start.x)<fabs(end.x-start.x)+step*0.4;p+=dir*step)
				ret.push_back(p);
		} else {
			float step=unitdef->ysize*8;
			float3 dir=end-start;
			if(dir.z==0){
				ret.push_back(start);
				return ret;
			}
			dir/=fabs(dir.z);
			if(keys[SDLK_LCTRL])
				dir.x=0;
			for(float3 p=start;fabs(p.z-start.z)<fabs(end.z-start.z)+step*0.4;p+=dir*step)
				ret.push_back(p);
		}
	}
	return ret;
}

void GUIgame::MakeBuildPos(float3& pos,UnitDef* unitdef)
{
	pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	pos.y=uh->GetBuildHeight(pos,unitdef);
	if(unitdef->floater)
		pos.y = max(pos.y,0.f);
}

bool GUIgame::IsDragCommand(CommandDescription *cmd)
{
	if(cmd)
	{
		if(cmd->type==CMDTYPE_ICON_BUILDING ||
			cmd->type==CMDTYPE_ICON_UNIT_OR_AREA ||
			cmd->type==CMDTYPE_ICON_UNIT_FEATURE_OR_AREA ||
			cmd->type==CMDTYPE_ICON_AREA ||
			cmd->type==CMDTYPE_ICON_FRONT 			)
			return true;
	}
	return false;
}



