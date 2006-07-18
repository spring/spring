#include "StdAfx.h"
#include "GUIbuildMenu.h"
#include "GUIbutton.h"
#include "GUIlabel.h"
#include "GUIfont.h"
#include "GUIpane.h"
#include "GUItab.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Game/command.h"
#include "Game/SelectedUnits.h"
#include "SDL_types.h"
#include "SDL_keysym.h"

#include <set>
using namespace std;

const int dist=0;

static void DrawTexturedQuad(int x, int y, int w, int h)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2f(x, y);
		glTexCoord2f(0,1);
		glVertex2f(x, y+h);
		glTexCoord2f(1,1);
		glVertex2f(x+w,	y+h);
		glTexCoord2f(1,0);
		glVertex2f(x+w,	y);
	glEnd();
}

//this is just extremly ugly --
// TODO: enable registration of 'GUIbuildMenus' with the GUIcontroller, so that the GUIcontroller
// can call back 'SetBuildMenuOptions' when needed.  This way, it doesn't have to be static,
// and unitDefs can be a class variable rather than global.
vector<UnitDef*> unitDefs;
vector<CommandDescription*> commands;
set<GUIbuildMenu*> buildMenus;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUIbuildMenu::GUIbuildMenu(int x, int y, int w, int h, Functor1<UnitDef*> select):GUIframe(x,y,w,h), clicked(select)
{
	emptyTex=Texture("bitmaps/emptyBuildPane.bmp");

	isPressed=false;
	menuOffset=0;
	
	left = new GUIbutton(20,20,"<<<", makeFunctor((Functor1<GUIbutton*>*) 0, *this, &GUIbuildMenu::Move));
	right = new GUIbutton(20,20,">>>", makeFunctor((Functor1<GUIbutton*>*) 0, *this, &GUIbuildMenu::Move));
	AddChild(left);
	AddChild(right);
	
	buildPicSize=64;
	
	buildMenus.insert(this);
}

GUIbuildMenu::~GUIbuildMenu()
{
	buildMenus.erase(this);
}

void GUIbuildMenu::PrivateDraw()
{
	GUIpane *pane=dynamic_cast<GUIpane*>(parent);
	if(pane)
	{
		if(h!=pane->h)
			SizeChanged();

		w=pane->w;
		h=pane->h;
	}

	glPushAttrib(GL_CURRENT_BIT);

	//glBindTexture(GL_TEXTURE_2D, 0);
	//glColor4f(0, 0, 0, 0.2);
	//DrawTexturedQuad(0,	0, w, h);

	int perCol=(h-left->h-2*dist-2)/(buildPicSize+dist);
	int perRow=w/(buildPicSize+dist);

	int i;
	for(i=menuOffset; i<unitDefs.size()&&(i-menuOffset)<perCol*perRow; i++)
	{
		glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitImage(unitDefs[i]));

		int x=(i-menuOffset)%perRow*(buildPicSize+dist);
		int y=(i-menuOffset)/perRow*(buildPicSize+dist);

		if(i==hiliteNum&&isPressed)
			glColor3f(0.8, 0.8, 0.8);
		else
			glColor3f(1.0, 1.0, 1.0);

		DrawTexturedQuad(x,	y, buildPicSize, buildPicSize);

		if(!commands[i]->params.empty())
		{
			string queued="+ "+commands[i]->params[0];

			x+=(int)((buildPicSize-guifont->GetWidth(queued))/2.0);
			y+=(int)(buildPicSize-guifont->GetHeight());
			guifont->Print(x, y, queued);
		}
	}
	
	glColor3f(1.0, 1.0, 1.0);
	// fill remaining squares with empty texture
	// disabled: as long as the empty texture is as ugly
	// as it is now...
	/*for(; (i-menuOffset)<perCol*perRow; i++)
	{
		glBindTexture(GL_TEXTURE_2D, emptyTex);

		int x=(i-menuOffset)/perCol*(buildPicSize+dist);
		int y=(i-menuOffset)%perCol*(buildPicSize+dist);
		
		DrawTexturedQuad(x,	y, buildPicSize, buildPicSize);

	}*/
	
	
	left->x=0;
	left->y=perCol*(buildPicSize+2*dist)+2;
	right->x=(perRow*(buildPicSize+dist))-right->w;
	right->y=perCol*(buildPicSize+2*dist)+2;
	
	glPopAttrib();
}


bool GUIbuildMenu::MouseDownAction(int x, int y, int button)
{
	isPressed=false;
	
	if(button!=3&&button!=1)
		return false;

	if(LocalInside(x, y))
	{
		hiliteNum=PicForPos(x, y);
		if(hiliteNum<unitDefs.size())
			isPressed=true;
	}

	return isPressed;
}

bool GUIbuildMenu::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
{
	if(LocalInside(x, y))
	{
		hiliteNum=PicForPos(x, y);
	}
	else
	{
		hiliteNum=-1;
	}

	return isPressed;
}

extern Uint8 *keys;

bool GUIbuildMenu::MouseUpAction(int x, int y, int button)
{
	if(isPressed)
	{
		if(LocalInside(x, y)&&hiliteNum>=0&&hiliteNum<commands.size())
		{
			if(commands[hiliteNum]->type==CMDTYPE_ICON_BUILDING)
				clicked(unitDefs[hiliteNum]);
			else
			{
				Command c;
				c.id=commands[hiliteNum]->id;
				c.options=0;

				if(keys[SDLK_LSHIFT])
					c.options|=SHIFT_KEY;
				if(keys[SDLK_LCTRL])
					c.options|=CONTROL_KEY;
				if(keys[SDLK_LALT])
					c.options|=ALT_KEY;
				if(button!=1)
					c.options|=RIGHT_MOUSE_KEY;

				selectedUnits.GiveCommand(c);
			}
		}

		isPressed=false;
		hiliteNum=-1;
		return true;
	}
	return false;
}


void GUIbuildMenu::SetBuildMenuOptions(const vector<CommandDescription*>& newOpts)
{
	unitDefs.clear();
	commands.clear();
	commands.assign(newOpts.begin(), newOpts.end());

	for(vector<CommandDescription*>::iterator i=commands.begin(); i!=commands.end(); i++)
	{
		UnitDef *ud = unitDefHandler->GetUnitByName((*i)->name);
		unitDefs.push_back(ud);
	}

	for(set<GUIbuildMenu*>::iterator i=buildMenus.begin(); i!=buildMenus.end(); i++)
		(*i)->menuOffset=0;
}

int GUIbuildMenu::PicForPos(int x, int y)
{
	int perCol=(h-left->h-2*dist-2)/(buildPicSize+dist);
	int perRow=w/(buildPicSize+dist);

	if(x>(buildPicSize+dist)*perRow)
		return -1;

	if(y>(buildPicSize+dist)*perCol)
		return -1;

	int res=y/(buildPicSize+dist)*perRow+x/(buildPicSize+dist)+menuOffset;
//	int res=x/(buildPicSize+dist)*perCol+y/(buildPicSize+dist)+menuOffset;

	if(res>=unitDefs.size())
		return -1;

	return res;
}

void GUIbuildMenu::Move(GUIbutton* button)
{
	const int perCol=(h-left->h)/(buildPicSize+dist);
	const int perRow=w/(buildPicSize+dist);
	const int inc=perCol*perRow;
	
	if(button==left)
	{
		menuOffset-=inc;
		menuOffset=max(menuOffset, 0);
	}
	else
	{
		if(menuOffset+inc>unitDefs.size()-1)
			return;
		menuOffset+=inc;
	}
}


bool GUIbuildMenu::EventAction(const std::string& event)
{
	if(event.compare(0, 6, "switch_"))
	{
		const int num=atoi(event.substr(7, std::string::npos).c_str());
		const int perCol=(h-left->h)/(buildPicSize+dist);
		const int perRow=w/(buildPicSize+dist);
		const int inc=perCol*perRow;
		const int pos=inc*num;
		
		if(pos<unitDefs.size()-1)
			menuOffset=pos;

		return true;
	}

	return GUIframe::EventAction(event);
}

string GUIbuildMenu::Tooltip()
{
	if((hiliteNum>=0)&&(hiliteNum<commands.size()))
		return commands[hiliteNum]->tooltip;
	return GUIframe::Tooltip();
}
