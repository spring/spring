#include "stdafx.h"
// GuiHandler.cpp: implementation of the CGuiHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "GuiHandler.h"
#include "selectedunits.h"
#include "mygl.h"
#include "mousehandler.h"
#include "glFont.h"
#include "ground.h"
#include "camera.h"
#include "infoconsole.h"
#include <map>
#include "Unit.h"
#include "gamehelper.h"
#include "game.h"
#include "gllist.h"
#include "weapon.h"
#include "loshandler.h"
#include "bitmap.h"
#include "unitloader.h"
//#include "unit3dloader.h"
#include "unithandler.h"
#include "texturehandler.h"
#include ".\guihandler.h"
#include "3doparser.h"
#include "unitdefhandler.h"
#include "feature.h"
#include "basegrounddrawer.h"
#include "weapondefhandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGuiHandler* guihandler;
extern bool keys[256];
unsigned int total_active_icons;

//unsigned int icon_texture[256];
//bool has_bitmap[256] = {false};

GLfloat guiposition_x[3] = {0.02f, 0.11f, 0.20f};
GLfloat guiposition_y[6] = {0.70f, 0.64f, 0.58f, 0.52f, 0.46f, 0.40f};
unsigned int maxxpos=1;
int fadein;

struct container_box {
	GLfloat x1;
	GLfloat y1;
	GLfloat x2;
	GLfloat y2;
};

struct guiicon_data {
	unsigned int x;
	unsigned int y;
	GLfloat width;
	GLfloat height;
	bool has_bitmap;
	unsigned int texture;
	char* c_str;
};

struct active_icon {
	GLfloat x1;
	GLfloat y1;
	GLfloat x2;
	GLfloat y2;
	int id;
};
	
static map<int,guiicon_data> iconmap;

//guiicon_data icon_data[2560];
active_icon curricon[400];
container_box box[20];

container_box buttonBox;

static void MenuSelection(std::string s)
{
	guihandler->MenuChoice(s);
}

CGuiHandler::CGuiHandler()
: inCommand(-1),
	activeMousePress(false),
	defaultCmdMemory(CMD_STOP),
	needShift(false),
	activePage(0),
	maxPages(0),
	showingMetal(false)
{
//	LoadCMDBitmap(CMD_STOP, "bitmaps\\ocean.bmp");
	LoadCMDBitmap(CMD_STOCKPILE, "bitmaps\\armsilo1.bmp");
	

}

//Ladda bitmap -> textur
bool CGuiHandler::LoadCMDBitmap (int id, char* filename) 
{
	guiicon_data icondata;
	glGenTextures(1, &icondata.texture);
	CBitmap TextureImage(filename);				

	// create mipmapped texture
	glBindTexture(GL_TEXTURE_2D, icondata.texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,TextureImage.xsize, TextureImage.ysize, GL_RGBA, GL_UNSIGNED_BYTE, TextureImage.mem);

	icondata.has_bitmap = true;
	icondata.width = .06f;
	icondata.height = .06f;
	
	iconmap[id] = icondata;
	return true;
}

CGuiHandler::~CGuiHandler()
{
	glDeleteTextures (1, &iconmap[0].texture);
}

// Ikonpositioner
void CGuiHandler::LayoutIcons() 
{
	unsigned int nr=0;
	unsigned int xpos=0, ypos=0;
	GLfloat texticon_width = .06f;
	GLfloat texticon_height = .06f;
	GLfloat height,width;

	buttonBox.x1 = 0.01f;
	buttonBox.y1 = 0.125f;
	buttonBox.x2 = 0.18f;
	buttonBox.y2 = 0.71f;

	if(showingMetal){
		showingMetal=false;
		groundDrawer->SetExtraTexture(0,0,false);
	}

	CSelectedUnits::AvailableCommandsStruct ac=selectedUnits.GetAvailableCommands();;
	vector<CommandDescription> uncommands=ac.commands;
	activePage=ac.commandPage;

	commands.clear();
	
	GLfloat x0 = buttonBox.x1 + .01f;
	GLfloat y0 = buttonBox.y2 - .01f;
	
	GLfloat xstep = .09f;
	GLfloat ystep = -.07f;
	
	vector<CommandDescription> hidden;

	vector<CommandDescription>::iterator cdi;
	for(cdi=uncommands.begin();cdi!=uncommands.end();++cdi){
		if(cdi->onlyKey){
			hidden.push_back(*cdi);
			continue;
		}

		if (iconmap.find(cdi->id) != iconmap.end() && iconmap[cdi->id].has_bitmap) {
			width = iconmap[cdi->id].width;
			height = iconmap[cdi->id].height;
		} else {
			width = texticon_width;
			height = texticon_height;
		}
		
		curricon[nr].x1 = x0 + xpos*xstep;
		curricon[nr].y1 = y0 + ypos*ystep;
		curricon[nr].x2 = curricon[nr].x1 + width;
		curricon[nr].y2 = curricon[nr].y1 - height;	

		curricon[nr].id = cdi->id;

		commands.push_back(*cdi);

		nr++;
		if (xpos < maxxpos) xpos++;
		else {ypos++;xpos=0;}

		if(nr>0 && (nr+2)%(NUMICOPAGE)==0)
		{
			curricon[nr].x1 = x0 + xpos*xstep;
			curricon[nr].y1 = y0 + ypos*ystep;
			curricon[nr].x2 = curricon[nr].x1 + width;
			curricon[nr].y2 = curricon[nr].y1 - height;	
			nr++;
			if (xpos < maxxpos) xpos++;
			else {ypos++;xpos=0;}
			curricon[nr].x1 = x0 + xpos*xstep;
			curricon[nr].y1 = y0 + ypos*ystep;
			curricon[nr].x2 = curricon[nr].x1 + width;
			curricon[nr].y2 = curricon[nr].y1 - height;	
			nr++;
			if (xpos < maxxpos) xpos++;
			else {ypos++;xpos=0;}

//			(*info) << "arf" << "\n";
			CommandDescription c;
			c.id=CMD_INTERNAL;
			c.type=CMDTYPE_PREV;
			c.name="<--";
			c.tooltip = "Previous menu";
			commands.push_back(c);

			c.id=CMD_INTERNAL;
			c.type=CMDTYPE_NEXT;
			c.name="-->";
			c.tooltip = "Next menu";
			commands.push_back(c);

			xpos=ypos=0;
		}
	}

	if(nr>NUMICOPAGE && (nr)%(NUMICOPAGE)!=0)
	{
			curricon[nr].x1 = x0 + 0*xstep;
			curricon[nr].y1 = y0 + 7*ystep;
			curricon[nr].x2 = curricon[nr].x1 + width;
			curricon[nr].y2 = curricon[nr].y1 - height;	
			nr++;
			if (xpos < maxxpos) xpos++;
			else {ypos++;xpos=0;}
			curricon[nr].x1 = x0 + 1*xstep;
			curricon[nr].y1 = y0 + 7*ystep;
			curricon[nr].x2 = curricon[nr].x1 + width;
			curricon[nr].y2 = curricon[nr].y1 - height;	
			nr++;
			if (xpos < maxxpos) xpos++;
			else {ypos++;xpos=0;}

//		(*info) << "arf" << "\n";
		CommandDescription c;
		c.id=CMD_INTERNAL;
		c.type=CMDTYPE_PREV;
		c.name="<--";
		c.tooltip = "Previous menue";
		commands.push_back(c);

		c.id=CMD_INTERNAL;
		c.type=CMDTYPE_NEXT;
		c.name="-->";
		c.tooltip = "Next menue";
		commands.push_back(c);
	}

	total_active_icons = nr;
	maxPages=max(0,(total_active_icons-1)/NUMICOPAGE);
	activePage=min(maxPages,activePage);

	for(cdi=hidden.begin();cdi!=hidden.end();++cdi){
		commands.push_back(*cdi);
	}

//	(*info) << "LayoutIcons called" << "\n";
}


void CGuiHandler::DrawButtons()
{
	if(needShift && !keys[VK_SHIFT]){
		if(showingMetal){
			showingMetal=false;
			groundDrawer->SetExtraTexture(0,0,false);
		}
		inCommand=-1;
		needShift=false;
	}

	if (selectedUnits.CommandsChanged()) {LayoutIcons();fadein=100;}
	else if (fadein>0) fadein-=5;

	int mouseIcon=IconAtPos(mouse->lastx,mouse->lasty);

	GLfloat x1,x2,y1,y2;
	GLfloat scalef;


	// Rita "container"ruta
	if (total_active_icons > 0) {
		
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.2f,0.2f,0.2f,0.4f);
		glBegin(GL_QUADS);

		GLfloat fx = 0;//-.2f*(1-fadein/100.0)+.2f;
		glVertex2f(buttonBox.x1-fx, buttonBox.y1);
		glVertex2f(buttonBox.x1-fx, buttonBox.y2);
		glVertex2f(buttonBox.x2-fx, buttonBox.y2);
		glVertex2f(buttonBox.x2-fx, buttonBox.y1);
		glVertex2f(buttonBox.x1-fx, buttonBox.y1);

		glEnd();
	}
	
	// För varje knapp (rita den)
	int buttonStart=max(0,min(activePage*NUMICOPAGE,total_active_icons));
	int buttonEnd=max(0,min((activePage+1)*NUMICOPAGE,total_active_icons));

	for(unsigned int nr=buttonStart;nr<buttonEnd;nr++) {

		if(commands[nr].onlyKey)
			continue;
		x1 = curricon[nr].x1;
		y1 = curricon[nr].y1;
		x2 = curricon[nr].x2;
		y2 = curricon[nr].y2;
		
		glDisable(GL_TEXTURE_2D);
		
		if(mouseIcon==nr || nr ==inCommand){
			glBegin(GL_QUADS);

			if(mouse->buttons[0].pressed)
				glColor4f(0.5,0,0,0.2f);
			else if (nr == inCommand)
				glColor4f(0.5,0,0,0.8f);
			else
				glColor4f(0,0,0.5,0.2f);

			glVertex2f(x1,y1);
			glVertex2f(x2,y1);
			glVertex2f(x2,y2);
			glVertex2f(x1,y2);
			glVertex2f(x1,y1);
			glEnd();
		} 
		//else
		//glBegin(GL_LINE_STRIP);

		// "markerad" (lägg mörkt fält över)
		glEnable(GL_TEXTURE_2D);
		glColor4f(1,1,1,0.8f);

		if(UnitDef *ud = unitDefHandler->GetUnitByName(commands[nr].name))
		{
			// unitikon
			glColor4f(1,1,1,0.8f);
			glBindTexture(GL_TEXTURE_2D, ud->unitimage);
			glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2f(x1,y1);
			glTexCoord2f(1,0); 
			glVertex2f(x2,y1);
			glTexCoord2f(1,1);
			glVertex2f(x2,y2);
			glTexCoord2f(0,1);
			glVertex2f(x1,y2);
			glEnd();
			if(!commands[nr].params.empty()){			//skriv texten i första param ovanpå
				string toPrint=commands[nr].params[0];

				if (toPrint.length() > 6) 
					scalef = toPrint.length()/4.0;
				else 
					scalef = 6.0/4.0;
				
				glTranslatef(x1+.004f,y1-.039f,0.0f);
				glScalef(0.02f/scalef,0.03f/scalef,0.1f);
				font->glPrint("%s",toPrint.c_str()); 
			}
		}
		else {
			if (iconmap.find(curricon[nr].id) != iconmap.end() && iconmap[curricon[nr].id].has_bitmap) {
				// Bitmapikon
				glBindTexture(GL_TEXTURE_2D, iconmap[curricon[nr].id].texture);
				glBegin(GL_QUADS);
				glTexCoord2f(0,0);
				glVertex2f(x1,y1);
				glTexCoord2f(1,0); 
				glVertex2f(x2,y1);
				glTexCoord2f(1,1);
				glVertex2f(x2,y2);
				glTexCoord2f(0,1);
				glVertex2f(x1,y2);
				glEnd();
			}
			// skriv text
			string toPrint=commands[nr].name;
			if(commands[nr].type==CMDTYPE_ICON_MODE){
				toPrint=commands[nr].params[atoi(commands[nr].params[0].c_str())+1];
			}

			if (toPrint.length() > 6) 
				scalef = toPrint.length()/4.0;
			else 
				scalef = 6.0/4.0;
			
		  glTranslatef(x1+.004f,y1-.039f,0.0f);
			glScalef(0.02f/scalef,0.03f/scalef,0.1f);
			font->glPrint("%s",toPrint.c_str()); 
		}

		glLoadIdentity();
	}
	if (total_active_icons > 0) {
		glColor4f(1,1,1,0.6f);
		if(selectedUnits.selectedGroup!=-1){
			font->glPrintAt(0.02,0.13,0.6,"Group %i selected ",selectedUnits.selectedGroup); 
		} else {
			font->glPrintAt(0.02,0.13,0.6,"Selected units %i",selectedUnits.selectedUnits.size()); 
		}
	}
	if(GetReceiverAt(mouse->lastx,mouse->lasty)){
			mouse->cursorText="";
	} else {
		if(inCommand>=0 && inCommand<commands.size()){
			mouse->cursorText=commands[guihandler->inCommand].name;
		} else {
			int defcmd;
			if(mouse->buttons[1].pressed && mouse->activeReceiver==this)
				defcmd=defaultCmdMemory;
			else
				defcmd=GetDefaultCommand(mouse->lastx,mouse->lasty);
			if(defcmd>=0 && defcmd<commands.size())
				mouse->cursorText=commands[defcmd].name;
			else
				mouse->cursorText="";
		}
	}
}

void CGuiHandler::Draw()
{
	DrawButtons();
}

bool CGuiHandler::MousePress(int x,int y,int button)
{
	if(AboveGui(x,y)){
		activeMousePress=true;
		return true;
	}
	if(inCommand>=0){
		activeMousePress=true;
		return true;
	}
	if(button==1){
		activeMousePress=true;
		defaultCmdMemory=GetDefaultCommand(x,y);

		return true;
	}
	return false;
}

void CGuiHandler::MouseRelease(int x,int y,int button)
{
	if(activeMousePress)
		activeMousePress=false;
	else 
		return;

	if(needShift && !keys[VK_SHIFT]){
		if(showingMetal){
			showingMetal=false;
			groundDrawer->SetExtraTexture(0,0,false);
		}
		inCommand=-1;
		needShift=false;
	}

	int icon=IconAtPos(x,y);

//	(*info) << x << " " << y << " " << mouse->lastx << " " << mouse->lasty << "\n";

	if (button == 1 && icon==-1) { // right click -> default cmd
		inCommand=defaultCmdMemory;//GetDefaultCommand(x,y);
	} 

	if(icon>=0 && icon<commands.size()){
		if(showingMetal){
			showingMetal=false;
			groundDrawer->SetExtraTexture(0,0,false);
		}
		switch(commands[icon].type){
			case CMDTYPE_ICON:{
				Command c;
				c.id=commands[icon].id;
				CreateOptions(c,!!button);
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;}
			case CMDTYPE_ICON_MODE:{
				int newMode=atoi(commands[icon].params[0].c_str())+1;
				if(newMode>commands[icon].params.size()-2)
					newMode=0;

				char t[10];
				itoa(newMode,t,10);
				commands[icon].params[0]=t;

				Command c;
				c.id=commands[icon].id;
				c.params.push_back(newMode);
				CreateOptions(c,!!button);
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;}
			case CMDTYPE_ICON_MAP:
			case CMDTYPE_ICON_AREA:
			case CMDTYPE_ICON_UNIT:
			case CMDTYPE_ICON_UNIT_OR_MAP:
			case CMDTYPE_ICON_FRONT:
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
				inCommand=icon;
				break;

			case CMDTYPE_ICON_BUILDING:{
				UnitDef* ud=unitDefHandler->GetUnitByID(-commands[icon].id);
				if(ud->extractsMetal>0){
					groundDrawer->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
					showingMetal=true;
				}
				inCommand=icon;
				break;}

			case CMDTYPE_COMBO_BOX:
				{
					inCommand=icon;
					CommandDescription& cd=commands[icon];
					list=new CglList(cd.name.c_str(),MenuSelection);
					for(vector<string>::iterator pi=++cd.params.begin();pi!=cd.params.end();++pi)
						list->AddItem(pi->c_str(),"");
					list->place=atoi(cd.params[0].c_str());
					game->showList=list;
					return;
				}
				inCommand=-1;
				break;
			case CMDTYPE_NEXT:
				{
					++activePage;
					if(activePage>maxPages)
						activePage=0;
				}
				selectedUnits.SetCommandPage(activePage);
				inCommand=-1;
				break;

			case CMDTYPE_PREV:
				{
					--activePage;
					if(activePage<0)
						activePage=maxPages;
				}
				selectedUnits.SetCommandPage(activePage);
				inCommand=-1;
				break;
		}
		return;
	}

	Command c=GetCommand(x,y,button,false);

	selectedUnits.GiveCommand(c);
	FinishCommand(button);
}

int CGuiHandler::IconAtPos(int x, int y)
{
	float fx=float(x)/gu->screenx;
	float fy=float(gu->screeny-y)/gu->screeny;

	if(fx>buttonBox.x2 || fx<buttonBox.x1 || fy>buttonBox.y2 || fy<buttonBox.y1)
		return -1;

	int buttonStart=max(0,min(activePage*NUMICOPAGE,total_active_icons));
	int buttonEnd=max(0,min((activePage+1)*NUMICOPAGE,total_active_icons));
	for (int a=buttonStart;a<buttonEnd;a++) {
		if (fx>curricon[a].x1 && fy<curricon[a].y1 && fx<curricon[a].x2 && fy>curricon[a].y2) 
			return a;
	}

	return -1;
}

bool CGuiHandler::AboveGui(int x, int y)
{
	if (total_active_icons <= 0)
		return false;

	float fx=float(x)/gu->screenx;
	float fy=float(gu->screeny-y)/gu->screeny;
	if(fx>buttonBox.x1 && fx<buttonBox.x2 && fy>buttonBox.y1 && fy<buttonBox.y2)
		return true;
	return IconAtPos(x,y)!=-1;
}

void CGuiHandler::CreateOptions(Command& c,bool rmb)
{
	c.options=0;
	if(rmb)
		c.options|=RIGHT_MOUSE_KEY;
	if(keys[VK_SHIFT])
		c.options|=SHIFT_KEY;
	if(keys[VK_CONTROL])
		c.options|=CONTROL_KEY;
	if(keys[VK_MENU])
		c.options|=ALT_KEY;
	//(*info) << (int)c.options << "\n";
}

int CGuiHandler::GetDefaultCommand(int x,int y)
{
	CInputReceiver* ir=GetReceiverAt(x,y);
	if(ir!=0)
		return -1;

	CUnit* unit=0;
	CFeature* feature=0;
	float dist=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,true);
	float dist2=helper->GuiTraceRayFeature(camera->pos,mouse->dir,9000,feature);

	if(dist>8900 && dist2>8900 && unit==0){
		return -1;
	}

	if(dist>dist2)
		unit=0;
	else
		feature=0;

	int cmd_id = selectedUnits.GetDefaultCmd(unit,feature);
	for (int c=0;c<total_active_icons;c++) {
		if ( cmd_id == curricon[c].id ) return c;
	}
	return -1;
}

void CGuiHandler::DrawMapStuff(void)
{
	if(activeMousePress){
		int cc=-1;
		int button=0;
		if(inCommand!=-1){
			cc=inCommand;
		} else {
			if(mouse->buttons[1].pressed && mouse->activeReceiver==this){
				cc=defaultCmdMemory;//GetDefaultCommand(mouse->lastx,mouse->lasty);
				button=1;
			}
		}

		if(cc>=0 && cc<commands.size()){
			switch(commands[cc].type){
			case CMDTYPE_ICON_FRONT:
				if(commands[cc].params.size()>0)
					if(commands[cc].params.size()>1)
						DrawFront(button,atoi(commands[cc].params[0].c_str()),atof(commands[cc].params[1].c_str()));
					else
						DrawFront(button,atoi(commands[cc].params[0].c_str()),0);
				else
					DrawFront(button,1000,0);
				break;
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
			case CMDTYPE_ICON_AREA:{
				float maxRadius=100000;
				if(commands[cc].params.size()==1)
					maxRadius=atof(commands[cc].params[0].c_str());
				if(mouse->buttons[button].movement>4){
					float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*9000);
					if(dist<0){
						break;
					}
					float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
					dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
					if(dist<0){
						break;
					}
					float3 pos2=camera->pos+mouse->dir*dist;
					DrawArea(pos,min(maxRadius,pos.distance2D(pos2)));
				}
				break;}
			default:
				break;
			}
		}
	}
	if(inCommand>=0 && inCommand<commands.size() && commands[guihandler->inCommand].type==CMDTYPE_ICON_BUILDING){
		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
		if(dist>0){
			string s=commands[guihandler->inCommand].name;
			UnitDef *unitdef = unitDefHandler->GetUnitByName(s);

			if(unitdef){
				float3 pos=camera->pos+mouse->dir*dist;
				std::vector<float3> buildPos;
				if(keys[VK_SHIFT] && mouse->buttons[0].pressed){
					float dist=ground->LineGroundCol(mouse->buttons[0].camPos,mouse->buttons[0].camPos+mouse->buttons[0].dir*9000);
					float3 pos2=mouse->buttons[0].camPos+mouse->buttons[0].dir*dist;
					buildPos=GetBuildPos(pos2,pos,unitdef);
				} else {
					buildPos=GetBuildPos(pos,pos,unitdef);
				}

				for(std::vector<float3>::iterator bpi=buildPos.begin();bpi!=buildPos.end();++bpi){
					float3 pos=*bpi;
					if(uh->ShowUnitBuildSquare(pos,unitdef))
						glColor4f(0.7,1,1,0.4);
					else
						glColor4f(1,0.5,0.5,0.4);
					glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
					glEnable(GL_TEXTURE_2D);
					texturehandler->SetTexture();
					glDepthMask(0);
					S3DOModel* model=unit3doparser->Load3DO((unitdef->model.modelpath).c_str() ,1, gu->myTeam);
					glPushMatrix();
					glTranslatef3(pos);
					//glCallList(model->displist);
					model->rootobject->DrawStatic();
					glDisable(GL_TEXTURE_2D);
					glDepthMask(1);
					glPopMatrix();
					if(unitdef->weapons.size()>0){
						glDisable(GL_TEXTURE_2D);
						glColor4f(1,0.3,0.3,0.7);
						glBegin(GL_LINE_STRIP);
						for(int a=0;a<=40;++a){
							float3 wpos(cos(a*2*PI/40)*unitdef->weapons[0]->range,0,sin(a*2*PI/40)*unitdef->weapons[0]->range);
							wpos+=pos;
							float dh=ground->GetHeight(wpos.x,wpos.z)-pos.y;
							float modRange=unitdef->weapons[0]->range-dh*unitdef->weapons[0]->heightmod;
							wpos=float3(cos(a*2*PI/40)*(modRange),0,sin(a*2*PI/40)*(modRange));
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

	CUnit* unit=0;
	float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,false);
	if(unit && unit->maxRange>0 && keys[VK_SHIFT] && (loshandler->InLos(unit,gu->myAllyTeam) || gu->spectating)){
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

void CGuiHandler::DrawFront(int button,float maxSize,float sizeDiv)
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5,1,0.5,0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	CMouseHandler::ButtonPress& bp=mouse->buttons[button];
	if(bp.movement<5)
		return;
	float dist=ground->LineGroundCol(bp.camPos,bp.camPos+bp.dir*9000);
	if(dist<0)
		return;
	float3 pos1=bp.camPos+bp.dir*dist;
	dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
	if(dist<0)
		return;
	float3 pos2=camera->pos+mouse->dir*dist;
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
		itoa(pos1.distance2D(pos2)/sizeDiv,c,10);
		mouse->cursorTextRight=c;
	}
	glEnable(GL_FOG);
}

bool CGuiHandler::KeyPressed(unsigned char key)
{
	if(key==VK_ESCAPE && activeMousePress){
		activeMousePress=false;
		inCommand=-1;
		if(showingMetal){
			showingMetal=false;
			groundDrawer->SetExtraTexture(0,0,false);
		}
		return true;
	}
	if(key==VK_ESCAPE && inCommand>0){
		inCommand=-1;
		if(showingMetal){
			showingMetal=false;
			groundDrawer->SetExtraTexture(0,0,false);
		}
		return true;
	}
	unsigned char keyOptions=0;
	if(keys[VK_SHIFT])
		keyOptions|=SHIFT_KEY;
	if(keys[VK_CONTROL])
		keyOptions|=CONTROL_KEY;
	if(keys[VK_MENU])
		keyOptions|=ALT_KEY;

	int a;
	for(a=0;a<commands.size();++a){
		if(commands[a].key==key && (keyOptions==commands[a].switchKeys || keyOptions==(commands[a].switchKeys|SHIFT_KEY))){
			switch(commands[a].type){
			case CMDTYPE_ICON:{
				Command c;
				c.id=commands[a].id;
				CreateOptions(c,0);
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;}
			case CMDTYPE_ICON_MODE:{
				int newMode=atoi(commands[a].params[0].c_str())+1;
				if(newMode>commands[a].params.size()-2)
					newMode=0;

				char t[10];
				itoa(newMode,t,10);
				commands[a].params[0]=t;

				Command c;
				c.id=commands[a].id;
				c.params.push_back(newMode);
				CreateOptions(c,0);
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;}
			case CMDTYPE_COMBO_BOX:{
				inCommand=a;
				CommandDescription& cd=commands[a];
				list=new CglList(cd.name.c_str(),MenuSelection);
				vector<string>::iterator pi;
				for(pi=++cd.params.begin();pi!=cd.params.end();++pi)
					list->AddItem(pi->c_str(),"");
				list->place=atoi(cd.params[0].c_str());
				game->showList=list;
				break;}
			default:
				inCommand=a;
			}
			return true;
		}
	}
	return false;
}

void CGuiHandler::MenuChoice(string s)
{
	game->showList=0;
	delete list;
	if(inCommand>=0 && inCommand<commands.size())
	{
		CommandDescription& cd=commands[inCommand];
		switch(cd.type)
		{
		case CMDTYPE_COMBO_BOX:
			{
				inCommand=-1;
				vector<string>::iterator pi;
				int a=0;
				for(pi=++cd.params.begin();pi!=cd.params.end();++pi)
				{
					if(*pi==s)
					{
						Command c;
						c.id=cd.id;
						c.params.push_back(a);
						CreateOptions(c,0);
						selectedUnits.GiveCommand(c);
						break;
					}
					++a;
				}
			}
				break;
		}
	}
}

void CGuiHandler::FinishCommand(int button)
{
	if(keys[VK_SHIFT] && button==0){
		needShift=true;
	} else {
		if(showingMetal){
			showingMetal=false;
			groundDrawer->SetExtraTexture(0,0,false);
		}
		inCommand=-1;
	}
}

bool CGuiHandler::IsAbove(int x, int y)
{
	return AboveGui(x,y);
}

std::string CGuiHandler::GetTooltip(int x, int y)
{
	string s;
	int icon=IconAtPos(x,y);
	if(icon>=0){
		if(commands[icon].tooltip!="")
			s=commands[icon].tooltip;
		else
			s=commands[icon].name;
		if(commands[icon].key){
			s+="\nHotkey ";
			if(commands[icon].switchKeys & SHIFT_KEY)
				s+="Shift+";
			if(commands[icon].switchKeys & CONTROL_KEY)
				s+="Ctrl+";
			if(commands[icon].switchKeys & ALT_KEY)
				s+="Alt+";
			s+=commands[icon].key;
		}
	}
	return s;
}

void CGuiHandler::DrawArea(float3 pos, float radius)
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5,1,0.5,0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glBegin(GL_TRIANGLE_FAN);
		glVertexf3(pos);
		for(int a=0;a<=40;++a){
			float3 p(cos(a*2*PI/40)*radius,0,sin(a*2*PI/40)*radius);
			p+=pos;		
			p.y=ground->GetHeight(p.x,p.z);
			glVertexf3(p);
		}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
}

Command CGuiHandler::GetOrderPreview(void)
{
	return GetCommand(mouse->lastx,mouse->lasty,-1,true);
}

Command CGuiHandler::GetCommand(int mousex, int mousey, int buttonHint, bool preview)
{
	Command defaultRet;
	defaultRet.id=CMD_STOP;

	int button;
	if(buttonHint>=0)
		button=buttonHint;
	else if(inCommand!=-1)
		button=0;
	else if(mouse->buttons[1].pressed)
		button=1;
	else
		return defaultRet;

	int tempInCommand=inCommand;

	if (button == 1 && preview) { // right click -> default cmd, in preview we might not have default cmd memory set
		if(mouse->buttons[1].pressed)
			tempInCommand=defaultCmdMemory;
		else
			tempInCommand=GetDefaultCommand(mousex,mousey);
	} 

	if(tempInCommand>=0 && tempInCommand<commands.size()){
		switch(commands[tempInCommand].type){

		case CMDTYPE_ICON:{
			Command c;
			c.id=commands[tempInCommand].id;
			CreateOptions(c,!!button);
			if(!button && !preview)
				info->AddLine("CMDTYPE_ICON left button press in incommand test? This shouldnt happen");
			return c;}

		case CMDTYPE_ICON_MAP:{
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=camera->pos+mouse->dir*dist;
			Command c;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
			CreateOptions(c,!!button);
			return c;}

		case CMDTYPE_ICON_BUILDING:{
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
			if(dist<0){
				return defaultRet;
			}
			string s=commands[guihandler->inCommand].name;
			UnitDef *unitdef = unitDefHandler->GetUnitByName(s);

			if(!unitdef){
				return defaultRet;
			}
			float3 pos=camera->pos+mouse->dir*dist;
			std::vector<float3> buildPos;
			if(keys[VK_SHIFT] && button==0){
				float dist=ground->LineGroundCol(mouse->buttons[0].camPos,mouse->buttons[0].camPos+mouse->buttons[0].dir*9000);
				float3 pos2=mouse->buttons[0].camPos+mouse->buttons[0].dir*dist;
				buildPos=GetBuildPos(pos2,pos,unitdef);
			} else {
				buildPos=GetBuildPos(pos,pos,unitdef);
			}

			for(std::vector<float3>::iterator bpi=buildPos.begin();bpi!=--buildPos.end();++bpi){
				float3 pos=*bpi;
				Command c;
				c.id=commands[tempInCommand].id;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
				CreateOptions(c,!!button);
				if(!preview)
					selectedUnits.GiveCommand(c);
			}
			pos=*(--buildPos.end());
			Command c;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
			CreateOptions(c,!!button);
			return c;}

		case CMDTYPE_ICON_UNIT: {
			CUnit* unit=0;
			Command c;

			c.id=commands[tempInCommand].id;
			float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,true);
			if (!unit){
				return defaultRet;
			}
			c.params.push_back(unit->id);
			CreateOptions(c,!!button);
			return c;}
		
		case CMDTYPE_ICON_UNIT_OR_MAP: {
			
			Command c;
			c.id=commands[tempInCommand].id;

			CUnit* unit=0;
			float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,true);
			if(dist2>8900){
				return defaultRet;
			}

			if (unit!=0) {  // clicked on unit
				c.params.push_back(unit->id);
			} else { // clicked in map
				float3 pos=camera->pos+mouse->dir*dist2;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
			}
			CreateOptions(c,!!button);
			return c;}

		case CMDTYPE_ICON_FRONT:{
			Command c;

			float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*9000);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);

			dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
			if(dist<0){
				return defaultRet;
			}
			float3 pos2=camera->pos+mouse->dir*dist;
			if(!commands[tempInCommand].params.empty() && pos.distance2D(pos2)>atoi(commands[tempInCommand].params[0].c_str())){
				float3 dif=pos2-pos;
				dif.Normalize();
				pos2=pos+dif*atoi(commands[tempInCommand].params[0].c_str());
			}

			c.id=commands[tempInCommand].id;
			c.params.push_back(pos2.x);
			c.params.push_back(pos2.y);
			c.params.push_back(pos2.z);
			CreateOptions(c,!!button);
			return c;}

		case CMDTYPE_ICON_UNIT_OR_AREA:
		case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
		case CMDTYPE_ICON_AREA:{
			float maxRadius=100000;
			if(commands[tempInCommand].params.size()==1)
				maxRadius=atof(commands[tempInCommand].params[0].c_str());

			Command c;
			c.id=commands[tempInCommand].id;

			if(mouse->buttons[button].movement<4){
				CUnit* unit=0;
				CFeature* feature=0;
				float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,true);
				float dist3=helper->GuiTraceRayFeature(camera->pos,mouse->dir,9000,feature);

				if(dist2>8900 && (commands[tempInCommand].type!=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA || dist3>8900)){
					return defaultRet;
				}

				if (feature!=0 && dist3<dist2 && commands[tempInCommand].type==CMDTYPE_ICON_UNIT_FEATURE_OR_AREA) {  // clicked on feature
					c.params.push_back(MAX_UNITS+feature->id);
				} else if (unit!=0 && commands[tempInCommand].type!=CMDTYPE_ICON_AREA) {  // clicked on unit
					c.params.push_back(unit->id);
				} else { // clicked in map
					float3 pos=camera->pos+mouse->dir*dist2;
					c.params.push_back(pos.x);
					c.params.push_back(pos.y);
					c.params.push_back(pos.z);
					c.params.push_back(0);//zero radius
				}
			} else {	//created area
				float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*9000);
				if(dist<0){
					return defaultRet;
				}
				float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
				dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2=camera->pos+mouse->dir*dist;
				c.params.push_back(min(maxRadius,pos.distance2D(pos2)));
			}
			CreateOptions(c,!!button);
			return c;}

		default:
			return defaultRet;
		}
	} else {
		if(!preview)
			inCommand=-1;
	}
	return defaultRet;
}

std::vector<float3> CGuiHandler::GetBuildPos(float3 start, float3 end,UnitDef* unitdef)
{
	std::vector<float3> ret;

	MakeBuildPos(start,unitdef);
	MakeBuildPos(end,unitdef);

	
	CUnit* unit=0;
	float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,9000,unit,20,true);

	if(unit && keys[VK_SHIFT] && keys[VK_CONTROL]){		//circle build around building
		UnitDef* unitdef2=unit->unitDef;
		float3 pos2=unit->pos;
		MakeBuildPos(pos2,unitdef2);
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
			MakeBuildPos(p2,unitdef);
			ret.push_back(p2);
		}
		pos=start;
		pos.x=end.x;
		for(;pos.z<=end.z;pos.z+=unitdef->ysize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x+=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z+=(unitdef->ysize/2)*SQUARE_SIZE;
			MakeBuildPos(p2,unitdef);
			ret.push_back(p2);
		}
		pos=end;
		for(;pos.x>=start.x;pos.x-=unitdef->xsize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x-=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z+=(unitdef->ysize/2)*SQUARE_SIZE;
			MakeBuildPos(p2,unitdef);
			ret.push_back(p2);
		}
		pos=end;
		pos.x=start.x;
		for(;pos.z>=start.z;pos.z-=unitdef->ysize*SQUARE_SIZE){
			float3 p2=pos;
			p2.x-=(unitdef->xsize/2)*SQUARE_SIZE;
			p2.z-=(unitdef->ysize/2)*SQUARE_SIZE;
			MakeBuildPos(p2,unitdef);
			ret.push_back(p2);
		}
	} else if(keys[VK_MENU]){			//build a rectangle
		float xsize=unitdef->xsize*8;
		int xnum=(fabs(end.x-start.x)+xsize*1.4)/xsize;
		int xstep=xsize;
		if(start.x>end.x)
			xstep*=-1;

		float zsize=unitdef->ysize*8;
		int znum=(fabs(end.z-start.z)+zsize*1.4)/zsize;
		int zstep=zsize;
		if(start.z>end.z)
			zstep*=-1;

		int zn=0;
		for(float z=start.z;zn<znum;++zn){
			int xn=0;
			for(float x=start.x;xn<xnum;++xn){
				if(!keys[VK_CONTROL] || zn==0 || xn==0 || zn==znum-1 || xn==xnum-1){
					float3 pos(x,0,z);
					MakeBuildPos(pos,unitdef);
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
			if(keys[VK_CONTROL])
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
			if(keys[VK_CONTROL])
				dir.x=0;
			for(float3 p=start;fabs(p.z-start.z)<fabs(end.z-start.z)+step*0.4;p+=dir*step)
				ret.push_back(p);
		}
	}
	return ret;
}

void CGuiHandler::MakeBuildPos(float3& pos,UnitDef* unitdef)
{
	pos.x=floor(pos.x/SQUARE_SIZE+0.5)*SQUARE_SIZE;
	pos.z=floor(pos.z/SQUARE_SIZE+0.5)*SQUARE_SIZE;
	pos.y=uh->GetBuildHeight(pos,unitdef);
	if(unitdef->floater)
		pos.y = max(pos.y,0);
}
