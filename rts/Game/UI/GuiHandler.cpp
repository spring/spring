#include "StdAfx.h"
// GuiHandler.cpp: implementation of the CGuiHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "GuiHandler.h"
#include "KeyBindings.h"
#include "Game/SelectedUnits.h"
#include "Rendering/GL/myGL.h"
#include "MouseHandler.h"
#include "Rendering/glFont.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "InfoConsole.h"
#include <map>
#include "Sim/Units/Unit.h"
#include "Game/GameHelper.h"
#include "Game/Game.h"
#include "Rendering/GL/glList.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Misc/LosHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Units/UnitLoader.h"
//#include "Rendering/UnitModels/Unit3DLoader.h"
#include "Sim/Units/UnitHandler.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Misc/Feature.h"
#include "Map/BaseGroundDrawer.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "SDL_keysym.h"
#include "SDL_mouse.h"
#include "mmgr.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Game/UI/GUI/GUIframe.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGuiHandler* guihandler;
extern Uint8 *keys;
unsigned int total_active_icons;

//unsigned int icon_texture[256];
//bool has_bitmap[256] = {false};

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
	defaultCmdMemory(0),
	needShift(false),
	activePage(0),
	maxPages(0),
	showingMetal(false),
	buildSpacing(0),
	buildFacing(0)
{
//	LoadCMDBitmap(CMD_STOP, "bitmaps\\ocean.bmp");
	LoadCMDBitmap(CMD_STOCKPILE, "bitmaps/armsilo1.bmp");
	readmap->mapDefParser.GetDef(autoShowMetal,"1","MAP\\autoShowMetal");
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
	defaultCmdMemory=0;
	unsigned int nr=0;
	unsigned int xpos=0, ypos=0;
	GLfloat texticon_width = .06f;
	GLfloat texticon_height = .06f;
	GLfloat height,width;

	//buttonBox.x1 = 0.01f;
	//buttonBox.y1 = 0.125f;
	//buttonBox.x2 = 0.18f;
	//buttonBox.y2 = 0.71f;

	//GLfloat xstep = .09f;
	//GLfloat ystep = -.07f;

	GLfloat fMargin = 0.005f;
	GLfloat xstep = .065f;
	GLfloat ystep = -.065f;

	buttonBox.x1 = fMargin;
	buttonBox.x2 = fMargin*2+xstep*2;
	buttonBox.y2 = 0.71f;
	buttonBox.y1 = buttonBox.y2+
		ystep*NUMICOPAGE/2;// number of buttons

	if(showingMetal){
		showingMetal=false;
		readmap->GetGroundDrawer()->DisableExtraTexture();
	}


	CSelectedUnits::AvailableCommandsStruct ac=selectedUnits.GetAvailableCommands();;
	vector<CommandDescription> uncommands=ac.commands;
	activePage=ac.commandPage;

	commands.clear();

	GLfloat x0 = buttonBox.x1 + fMargin;
	GLfloat y0 = buttonBox.y2 - fMargin;

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
			c.action="prevmenu";
			c.type=CMDTYPE_PREV;
			c.name="";
			c.tooltip = "Previous menu";
			commands.push_back(c);

			c.id=CMD_INTERNAL;
			c.action="nextmenu";
			c.type=CMDTYPE_NEXT;
			c.name="";
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
		c.action="prevmenu";
		c.type=CMDTYPE_PREV;
		c.name="";
		c.tooltip = "Previous menu";
		commands.push_back(c);

		c.id=CMD_INTERNAL;
		c.action="nextmenu";
		c.type=CMDTYPE_NEXT;
		c.name="";
		c.tooltip = "Next menu";
		commands.push_back(c);
	}

	total_active_icons = nr;
	maxPages=max(0,(int)((total_active_icons-1)/NUMICOPAGE));
	activePage=min(maxPages,activePage);

	for(cdi=hidden.begin();cdi!=hidden.end();++cdi){
		commands.push_back(*cdi);
	}

//	(*info) << "LayoutIcons called" << "\n";
}


void CGuiHandler::DrawButtons()
{
	glDisable(GL_DEPTH_TEST);
	if(needShift && !keys[SDLK_LSHIFT]){
		if(showingMetal){
			showingMetal=false;
			readmap->GetGroundDrawer()->DisableExtraTexture();
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
		glColor4f(0.2f,0.2f,0.2f,GUI_TRANS);
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
	int buttonStart=max(0,(int)(min(activePage*NUMICOPAGE,(int)total_active_icons)));
	int buttonEnd=max(0,int(min((activePage+1)*NUMICOPAGE,(int)total_active_icons)));

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

			if(mouse->buttons[SDL_BUTTON_LEFT].pressed)
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
			glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitImage(ud));
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
			else
			{
				glPushAttrib(GL_ENABLE_BIT);
				glDisable(GL_LIGHTING);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				GLfloat xSize = fabs(x2-x1);
				GLfloat yCenter = (y1+y2)/2.f;
				GLfloat ySize = fabs(y2-y1);

				if (commands[nr].type == CMDTYPE_PREV)
				{
					glBegin(GL_POLYGON);
					if(mouseIcon==nr || nr ==inCommand)
					{	// selected
						glColor4f(1.f,1.f,0.f,1.f);
					}
					else
					{	// normal
						glColor4f(0.7f,0.7f,0.7f,1.f);
					}
					glVertex2f(x2-xSize/6,yCenter-ySize/8);
					glVertex2f(x1+2*xSize/6,yCenter-ySize/8);
					glVertex2f(x1+xSize/6,yCenter);
					glVertex2f(x1+2*xSize/6,yCenter+ySize/8);
					glVertex2f(x2-xSize/6,yCenter+ySize/8);
					glEnd();
				}
				else if (commands[nr].type == CMDTYPE_NEXT)
				{
					glBegin(GL_POLYGON);
					if(mouseIcon==nr || nr ==inCommand)
					{	// selected
						glColor4f(1.f,1.f,0.f,1.f);
					}
					else
					{	// normal
						glColor4f(0.7f,0.7f,0.7f,1.f);
					}
					glVertex2f(x1+xSize/6,yCenter-ySize/8);
					glVertex2f(x2-2*xSize/6,yCenter-ySize/8);
					glVertex2f(x2-xSize/6,yCenter);
					glVertex2f(x2-2*xSize/6,yCenter+ySize/8);
					glVertex2f(x1+xSize/6,yCenter+ySize/8);
					glEnd();
				}
				else
				{
					glBegin(GL_LINE_LOOP);
					glColor4f(1.f,1.f,1.f,0.1f);
					glVertex2f(x1,y1);
					glVertex2f(x2,y1);
					glVertex2f(x2,y2);
					glVertex2f(x1,y2);
					glEnd();
				}
				glPopAttrib();
			}

			// skriv text
			string toPrint=commands[nr].name;
			if(commands[nr].type==CMDTYPE_ICON_MODE && (atoi(commands[nr].params[0].c_str())+1)<commands[nr].params.size()){
				toPrint=commands[nr].params[atoi(commands[nr].params[0].c_str())+1];
			}


			float fDecX = 0.f;
			float fTxtLen = font->CalcTextWidth(toPrint.c_str());
			scalef = 1.f;
			float fIconWidth = 2.5f;
			float fIconHeight = 2.0;
			if (fTxtLen>fIconWidth)
			{
				scalef = fTxtLen/fIconWidth;
			}
			else
			{
				fDecX = (fIconWidth-fTxtLen)/2;
				fDecX = fDecX / fIconWidth * (x2-x1);// convert to screen coords
			}

			glPushMatrix();
			float dshadow = 0.003f/scalef;
			glColor4f(0.0f,0.0f,0.f,0.8f);
			glTranslatef(dshadow+fDecX+x1+.004f,-dshadow+y1-.039f,0.0f);
			glScalef(0.02f/scalef,0.03f/scalef,0.1f);
			font->glPrint("%s",toPrint.c_str());
			glPopMatrix();

			glColor4f(1.f,1.f,1.f,1.f);
			glTranslatef(fDecX+x1+.004f,y1-.039f,0.0f);
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
			if(mouse->buttons[SDL_BUTTON_RIGHT].pressed && mouse->activeReceiver==this)
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
	if(button==SDL_BUTTON_RIGHT){
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

	if(needShift && !keys[SDLK_LSHIFT]){
		if(showingMetal){
			showingMetal=false;
			readmap->GetGroundDrawer()->DisableExtraTexture();
		}
		inCommand=-1;
		needShift=false;
	}

	int icon=IconAtPos(x,y);

//	(*info) << x << " " << y << " " << mouse->lastx << " " << mouse->lasty << "\n";

	if (button == SDL_BUTTON_RIGHT && icon==-1) { // right click -> default cmd
		inCommand=defaultCmdMemory;//GetDefaultCommand(x,y);
		defaultCmdMemory=0;
	}

	if(icon>=0 && icon<commands.size()){
		if(showingMetal){
			showingMetal=false;
			readmap->GetGroundDrawer()->DisableExtraTexture();
		}
		switch(commands[icon].type){
			case CMDTYPE_ICON:{
				Command c;
				c.id=commands[icon].id;
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;}
			case CMDTYPE_ICON_MODE:{
				int newMode=atoi(commands[icon].params[0].c_str())+1;
				if(newMode>commands[icon].params.size()-2)
					newMode=0;

				char t[10];
				SNPRINTF(t, 10, "%d", newMode);
				commands[icon].params[0]=t;

				Command c;
				c.id=commands[icon].id;
				c.params.push_back(newMode);
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
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
				CBaseGroundDrawer *gd = readmap->GetGroundDrawer ();
				if(ud->extractsMetal>0 && gd->drawMode != CBaseGroundDrawer::drawMetal && autoShowMetal){
					readmap->GetGroundDrawer()->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
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

	if(c.id!=CMD_STOP)	//if cmd_stop is returned it indicates that no good command could be found
	selectedUnits.GiveCommand(c);
	FinishCommand(button);
}

int CGuiHandler::IconAtPos(int x, int y)
{
	float fx=float(x)/gu->screenx;
	float fy=float(gu->screeny-y)/gu->screeny;

	if(fx>buttonBox.x2 || fx<buttonBox.x1 || fy>buttonBox.y2 || fy<buttonBox.y1)
		return -1;

	int buttonStart=max(0,min(activePage*NUMICOPAGE,(int)total_active_icons));
	int buttonEnd=max(0,min((activePage+1)*NUMICOPAGE,(int)total_active_icons));
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
	if(keys[SDLK_LSHIFT])
		c.options|=SHIFT_KEY;
	if(keys[SDLK_LCTRL])
		c.options|=CONTROL_KEY;
	if(keys[SDLK_LALT])
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
	float dist=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4,unit,20,true);
	float dist2=helper->GuiTraceRayFeature(camera->pos,mouse->dir,gu->viewRange*1.4,feature);

	if(dist>gu->viewRange*1.4-100 && dist2>gu->viewRange*1.4-100 && unit==0){
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
		int button=SDL_BUTTON_LEFT;
		if(inCommand!=-1){
			cc=inCommand;
		} else {
			if(mouse->buttons[SDL_BUTTON_RIGHT].pressed && mouse->activeReceiver==this){
				cc=defaultCmdMemory;//GetDefaultCommand(mouse->lastx,mouse->lasty);
				button=SDL_BUTTON_RIGHT;
			}
		}

		if(cc>=0 && cc<commands.size()){
			switch(commands[cc].type){
			case CMDTYPE_ICON_FRONT:
				if(mouse->buttons[button].movement>30){
					if(commands[cc].params.size()>0)
						if(commands[cc].params.size()>1)
							DrawFront(button,atoi(commands[cc].params[0].c_str()),atof(commands[cc].params[1].c_str()));
						else
							DrawFront(button,atoi(commands[cc].params[0].c_str()),0);
					else
						DrawFront(button,1000,0);
				}
				break;
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA:
			case CMDTYPE_ICON_AREA:{
				float maxRadius=100000;
				if(commands[cc].params.size()==1)
					maxRadius=atof(commands[cc].params[0].c_str());
				if(mouse->buttons[button].movement>4){
					float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*gu->viewRange*1.4);
					if(dist<0){
						break;
					}
					float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
					dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
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
	//draw buildings we are about to build
	if(inCommand>=0 && inCommand<commands.size() && commands[guihandler->inCommand].type==CMDTYPE_ICON_BUILDING){
		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
		if(dist>0){
			string s=commands[guihandler->inCommand].name;
			UnitDef *unitdef = unitDefHandler->GetUnitByName(s);

			if(unitdef){
				float3 pos=camera->pos+mouse->dir*dist;
				std::vector<BuildInfo> buildPos;
				if(keys[SDLK_LSHIFT] && mouse->buttons[SDL_BUTTON_LEFT].pressed){
					float dist=ground->LineGroundCol(mouse->buttons[SDL_BUTTON_LEFT].camPos,mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4);
					float3 pos2=mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*dist;
					buildPos=GetBuildPos(BuildInfo(unitdef, pos2,buildFacing), BuildInfo(unitdef,pos, buildFacing));
				} else {
					BuildInfo bi(unitdef, pos, buildFacing);
					buildPos=GetBuildPos(bi,bi);
				}

				for(std::vector<BuildInfo>::iterator bpi=buildPos.begin();bpi!=buildPos.end();++bpi)
				{
					float3 pos=bpi->pos;
					std::vector<Command> cv;
					if(keys[SDLK_LSHIFT]){
						Command c;
						bpi->FillCmd(c);
						std::vector<Command> temp;
						std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
						for(; ui != selectedUnits.selectedUnits.end(); ui++){
							temp = (*ui)->commandAI->GetOverlapQueued(c);
							std::vector<Command>::iterator ti = temp.begin();
							for(; ti != temp.end(); ti++)
								cv.insert(cv.end(),*ti);
						}
					}
					if(uh->ShowUnitBuildSquare(*bpi, cv))
						glColor4f(0.7,1,1,0.4);
					else
						glColor4f(1,0.5,0.5,0.4);

					unitDrawer->DrawBuildingSample(bpi->def, gu->myTeam, pos, bpi->buildFacing);
					if(unitdef->weapons.size()>0){	//draw range
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

	if(keys[SDLK_LSHIFT]){
		CUnit* unit=0;
		float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4,unit,20,false);
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
			if(unit->unitDef->kamikazeDist>0){			//draw self destruct and damage distance
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
				if(!unit->unitDef->selfDExplosion.empty()){
					glColor4f(0.8,0.1,0.1,0.7);
					WeaponDef* wd=weaponDefHandler->GetWeapon(unit->unitDef->selfDExplosion);

					glBegin(GL_LINE_STRIP);
					for(int a=0;a<=40;++a){
						float3 pos(cos(a*2*PI/40)*wd->areaOfEffect,0,sin(a*2*PI/40)*wd->areaOfEffect);
						pos+=unit->pos;
						pos.y=ground->GetHeight(pos.x,pos.z)+8;
						glVertexf3(pos);
					}
					glEnd();
				}
			}
		}
	}
	//draw range circles if attack orders are imminent
	int defcmd=GetDefaultCommand(mouse->lastx,mouse->lasty);
	if((inCommand>0 && inCommand<commands.size() && commands[inCommand].id==CMD_ATTACK) || (inCommand==-1 && defcmd>0 && commands[defcmd].id==CMD_ATTACK)){
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

void CGuiHandler::DrawFront(int button,float maxSize,float sizeDiv)
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5,1,0.5,0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	CMouseHandler::ButtonPress& bp=mouse->buttons[button];
	if(bp.movement<5)
		return;
	float dist=ground->LineGroundCol(bp.camPos,bp.camPos+bp.dir*gu->viewRange*1.4);
	if(dist<0)
		return;
	float3 pos1=bp.camPos+bp.dir*dist;
	dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
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
		SNPRINTF(c, 40, "%d", (int)(pos1.distance2D(pos2)/sizeDiv) );
		mouse->cursorTextRight=c;
	}
	glEnable(GL_FOG);
}

bool CGuiHandler::KeyPressed(unsigned short key)
{
	if(key==SDLK_ESCAPE && activeMousePress){
		activeMousePress=false;
		inCommand=-1;
		if(showingMetal){
			showingMetal=false;
			readmap->GetGroundDrawer()->DisableExtraTexture();
		}
		return true;
	}
	if(key==SDLK_ESCAPE && inCommand>0){
		inCommand=-1;
		if(showingMetal){
			showingMetal=false;
			readmap->GetGroundDrawer()->DisableExtraTexture();
		}
		return true;
	}

	CKeySet ks(key, false);
	const string action = keyBindings->GetAction(ks, "command");
		
	int a;
	if (action == "showcommands") {
		for(a = 0; a < commands.size(); ++a){
			printf("  button: %i, id = %i, action = %s\n",
						 a, commands[a].id, commands[a].action.c_str());
		}
		return true;
	}

	for(a = 0; a < commands.size(); ++a){
		if(commands[a].action != action) {
			continue;
		}
		switch(commands[a].type){
			case CMDTYPE_ICON:{
				Command c;
				c.id=commands[a].id;
				CreateOptions(c, false); //(button==SDL_BUTTON_LEFT?0:1));
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;
			}
			case CMDTYPE_ICON_MODE:{
				int newMode=atoi(commands[a].params[0].c_str())+1;
				if(newMode>commands[a].params.size()-2)
					newMode=0;

				char t[10];
				SNPRINTF(t, 10, "%d", newMode);
				commands[a].params[0]=t;

				Command c;
				c.id=commands[a].id;
				c.params.push_back(newMode);
				CreateOptions(c, false); //(button==SDL_BUTTON_LEFT?0:1));
				selectedUnits.GiveCommand(c);
				inCommand=-1;
				break;
			}
			case CMDTYPE_ICON_MAP:
			case CMDTYPE_ICON_AREA:
			case CMDTYPE_ICON_UNIT:
			case CMDTYPE_ICON_UNIT_OR_MAP:
			case CMDTYPE_ICON_FRONT:
			case CMDTYPE_ICON_UNIT_OR_AREA:
			case CMDTYPE_ICON_UNIT_FEATURE_OR_AREA: {
				inCommand=a;
				break;
			}
			case CMDTYPE_ICON_BUILDING:{
				UnitDef* ud=unitDefHandler->GetUnitByID(-commands[a].id);
				CBaseGroundDrawer *gd = readmap->GetGroundDrawer ();
				if(ud->extractsMetal>0 && gd->drawMode != CBaseGroundDrawer::drawMetal && autoShowMetal){
					readmap->GetGroundDrawer()->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
					showingMetal=true;
				}
				inCommand=a;
				break;
			}
			case CMDTYPE_COMBO_BOX:{
				inCommand=a;
				CommandDescription& cd=commands[a];
				list=new CglList(cd.name.c_str(),MenuSelection);
				for(vector<string>::iterator pi=++cd.params.begin();pi!=cd.params.end();++pi)
					list->AddItem(pi->c_str(),"");
				list->place=atoi(cd.params[0].c_str());
				game->showList=list;
				//						return;
				inCommand=-1;
				break;
			}
			case CMDTYPE_NEXT:{
				++activePage;
				if(activePage>maxPages)
					activePage=0;
				selectedUnits.SetCommandPage(activePage);
				inCommand=-1;
				break;
			}
			case CMDTYPE_PREV:{
				--activePage;
				if(activePage<0)
					activePage=maxPages;
				selectedUnits.SetCommandPage(activePage);
				inCommand=-1;
				break;
			}
			default:{
				inCommand=a;
			}
		}
		return true; // we used the command
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
	if(keys[SDLK_LSHIFT] && button==SDL_BUTTON_LEFT){
		needShift=true;
	} else {
		if(showingMetal){
			showingMetal=false;
			readmap->GetGroundDrawer()->DisableExtraTexture();
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
		if(!commands[icon].hotkey.empty()){
			s+="\nHotkey " + commands[icon].hotkey;
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
	if(buttonHint>=SDL_BUTTON_LEFT)
		button=buttonHint;
	else if(inCommand!=-1)
		button=SDL_BUTTON_LEFT;
	else if(mouse->buttons[SDL_BUTTON_RIGHT].pressed)
		button=SDL_BUTTON_RIGHT;
	else
		return defaultRet;

	int tempInCommand=inCommand;

	if (button == SDL_BUTTON_RIGHT && preview) { // right click -> default cmd, in preview we might not have default cmd memory set
		if(mouse->buttons[SDL_BUTTON_RIGHT].pressed)
			tempInCommand=defaultCmdMemory;
		else
			tempInCommand=GetDefaultCommand(mousex,mousey);
	}

	if(tempInCommand>=0 && tempInCommand<commands.size()){
		switch(commands[tempInCommand].type){

		case CMDTYPE_ICON:{
			Command c;
			c.id=commands[tempInCommand].id;
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			if(button==SDL_BUTTON_LEFT && !preview)
				info->AddLine("CMDTYPE_ICON left button press in incommand test? This shouldnt happen");
			return c;}

		case CMDTYPE_ICON_MAP:{
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=camera->pos+mouse->dir*dist;
			Command c;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_BUILDING:{
			float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
			if(dist<0){
				return defaultRet;
			}
			string s=commands[guihandler->inCommand].name;
			UnitDef *unitdef = unitDefHandler->GetUnitByName(s);

			if(!unitdef){
				return defaultRet;
			}

			float3 pos=camera->pos+mouse->dir*dist;
			std::vector<BuildInfo> buildPos;
			BuildInfo bi(unitdef, pos, buildFacing);
			if(keys[SDLK_LSHIFT] && button==SDL_BUTTON_LEFT){
				float dist=ground->LineGroundCol(mouse->buttons[SDL_BUTTON_LEFT].camPos,mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*gu->viewRange*1.4);
				float3 pos2=mouse->buttons[SDL_BUTTON_LEFT].camPos+mouse->buttons[SDL_BUTTON_LEFT].dir*dist;
				buildPos=GetBuildPos(BuildInfo(unitdef,pos2,buildFacing),bi);
			} else 
				buildPos=GetBuildPos(bi,bi);

			if(buildPos.empty()){
				return defaultRet;
			}

			int a=0;		//limit the number of max commands possible to send to avoid overflowing the network buffer
			for(std::vector<BuildInfo>::iterator bpi=buildPos.begin();bpi!=--buildPos.end() && a<200;++bpi){
				++a;
				Command c;
				bpi->FillCmd(c);
				CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
				if(!preview)
					selectedUnits.GiveCommand(c);
			}
			Command c;
			buildPos.back().FillCmd(c);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT: {
			CUnit* unit=0;
			Command c;

			c.id=commands[tempInCommand].id;
			float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4,unit,20,true);
			if (!unit){
				return defaultRet;
			}
			c.params.push_back(unit->id);
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_UNIT_OR_MAP: {

			Command c;
			c.id=commands[tempInCommand].id;

			CUnit* unit=0;
			float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4,unit,20,true);
			if(dist2>gu->viewRange*1.4-100){
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
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
			return c;}

		case CMDTYPE_ICON_FRONT:{
			Command c;

			float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*gu->viewRange*1.4);
			if(dist<0){
				return defaultRet;
			}
			float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
			c.id=commands[tempInCommand].id;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);

			if(mouse->buttons[button].movement>30){		//only create the front if the mouse has moved enough
				dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2=camera->pos+mouse->dir*dist;
				if(!commands[tempInCommand].params.empty() && pos.distance2D(pos2)>atoi(commands[tempInCommand].params[0].c_str())){
					float3 dif=pos2-pos;
					dif.Normalize();
					pos2=pos+dif*atoi(commands[tempInCommand].params[0].c_str());
				}

				c.params.push_back(pos2.x);
				c.params.push_back(pos2.y);
				c.params.push_back(pos2.z);
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
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
				float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4,unit,20,true);
				float dist3=helper->GuiTraceRayFeature(camera->pos,mouse->dir,gu->viewRange*1.4,feature);

				if(dist2>gu->viewRange*1.4-100 && (commands[tempInCommand].type!=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA || dist3>8900)){
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
				float dist=ground->LineGroundCol(mouse->buttons[button].camPos,mouse->buttons[button].camPos+mouse->buttons[button].dir*gu->viewRange*1.4);
				if(dist<0){
					return defaultRet;
				}
				float3 pos=mouse->buttons[button].camPos+mouse->buttons[button].dir*dist;
				c.params.push_back(pos.x);
				c.params.push_back(pos.y);
				c.params.push_back(pos.z);
				dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4);
				if(dist<0){
					return defaultRet;
				}
				float3 pos2=camera->pos+mouse->dir*dist;
				c.params.push_back(min(maxRadius,pos.distance2D(pos2)));
			}
			CreateOptions(c,(button==SDL_BUTTON_LEFT?0:1));
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

// Assuming both builds have the same unitdef
std::vector<BuildInfo> CGuiHandler::GetBuildPos(const BuildInfo& startInfo, const BuildInfo& endInfo)
{
	std::vector<BuildInfo> ret;

	float3 start=helper->Pos2BuildPos(startInfo);
	float3 end=helper->Pos2BuildPos(endInfo);

	BuildInfo other; // the unit around which buildings can be circled
	if(keys[SDLK_LSHIFT] && keys[SDLK_LCTRL])
	{
		CUnit* unit=0;
		float dist2=helper->GuiTraceRay(camera->pos,mouse->dir,gu->viewRange*1.4,unit,20,true);
		if(unit){
			other.def = unit->unitDef;
			other.pos = unit->pos;
			other.buildFacing = unit->buildFacing;
		} else {
			Command c = uh->GetBuildCommand(camera->pos,mouse->dir);
			if(c.id < 0){
				assert(c.params.size()==4);
				other.pos = float3(c.params[0],c.params[1],c.params[2]);
				other.def = unitDefHandler->GetUnitByID(-c.id);
				other.buildFacing = int(c.params[3]);
			}
		}
	}

	if(other.def && keys[SDLK_LSHIFT] && keys[SDLK_LCTRL]){		//circle build around building
		Command c;

		start=end=helper->Pos2BuildPos(other);
		start.x-=(other.GetXSize()/2)*SQUARE_SIZE;
		start.z-=(other.GetYSize()/2)*SQUARE_SIZE;
		end.x+=(other.GetXSize()/2)*SQUARE_SIZE;
		end.z+=(other.GetYSize()/2)*SQUARE_SIZE;

		float3 pos=start;
		int xsize=startInfo.GetXSize();
		int ysize=startInfo.GetYSize();
		for(;pos.x<=end.x;pos.x+=xsize*SQUARE_SIZE){
			BuildInfo bi(startInfo.def,pos,startInfo.buildFacing);
			bi.pos.x+=(xsize/2)*SQUARE_SIZE;
			bi.pos.z-=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui)
			{
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
		pos=end;
		pos.x=start.x;
		for(;pos.z>=start.z;pos.z-=ysize*SQUARE_SIZE){
			BuildInfo bi(startInfo.def,pos,(startInfo.buildFacing+1)%4);
			bi.pos.x-=(xsize/2)*SQUARE_SIZE;
			bi.pos.z-=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui){
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
		pos=end;
		for(;pos.x>=start.x;pos.x-=xsize*SQUARE_SIZE) {
			BuildInfo bi(startInfo.def,pos,(startInfo.buildFacing+2)%4);
			bi.pos.x-=(xsize/2)*SQUARE_SIZE;
			bi.pos.z+=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui)
			{
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
		pos=start;
		pos.x=end.x;
		for(;pos.z<=end.z;pos.z+=ysize*SQUARE_SIZE){
			BuildInfo bi(startInfo.def,pos,(startInfo.buildFacing+3)%4);
			bi.pos.x+=(xsize/2)*SQUARE_SIZE;
			bi.pos.z+=(ysize/2)*SQUARE_SIZE;
			bi.pos=helper->Pos2BuildPos(bi);
			bi.FillCmd(c);
			bool cancel = false;
			std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
			for(;ui != selectedUnits.selectedUnits.end() && !cancel; ++ui)
			{
				if((*ui)->commandAI->WillCancelQueued(c))
					cancel = true;
			}
			if(!cancel)
				ret.push_back(bi);
		}
	} else if(keys[SDLK_LALT]){			//build a rectangle
		float xsize=startInfo.GetXSize()*8+buildSpacing*16;
		int xnum=(int)((fabs(end.x-start.x)+xsize*1.4)/xsize);
		int xstep=(int)xsize;
		if(start.x>end.x)
			xstep*=-1;

		float zsize=startInfo.GetYSize()*8+buildSpacing*16;
		int znum=(int)((fabs(end.z-start.z)+zsize*1.4)/zsize);
		int zstep=(int)zsize;
		if(start.z>end.z)
			zstep*=-1;

		int zn=0;
		for(float z=start.z;zn<znum;++zn){
			int xn=0;
			for(float x=start.x;xn<xnum;++xn){
				if(!keys[SDLK_LCTRL] || zn==0 || xn==0 || zn==znum-1 || xn==xnum-1){
					BuildInfo bi = startInfo;
					bi.pos = float3(x,0,z);
					bi.pos=helper->Pos2BuildPos(bi);
					ret.push_back(bi);
				}
				x+=xstep;
			}
			z+=zstep;
		}
	} else {			//build a line
		BuildInfo bi=startInfo;
		if(fabs(start.x-end.x)>fabs(start.z-end.z)){
			float step=startInfo.GetXSize()*8+buildSpacing*16;
			float3 dir=end-start;
			if(dir.x==0){
				bi.pos = start;
				ret.push_back(bi);
				return ret;
			}
			dir/=fabs(dir.x);
			if(keys[SDLK_LCTRL])
				dir.z=0;
			for(float3 p=start;fabs(p.x-start.x)<fabs(end.x-start.x)+step*0.4;p+=dir*step) {
				bi.pos=p;
				ret.push_back(bi);
			}
		} else {
			float step=startInfo.GetYSize()*8+buildSpacing*16;
			float3 dir=end-start;
			if(dir.z==0){
				bi.pos=start;
				ret.push_back(bi);
				return ret;
			}
			dir/=fabs(dir.z);
			if(keys[SDLK_LCTRL])
				dir.x=0;
			for(float3 p=start;fabs(p.z-start.z)<fabs(end.z-start.z)+step*0.4;p+=dir*step) {
				bi.pos=p;
				ret.push_back(bi);
			}
		}
	}
	return ret;
}
