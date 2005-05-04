#include "GUIcenterBuildMenu.h"
#include "GUIbutton.h"
#include "GUIlabel.h"
#include "GUIfont.h"
#include "GUIpane.h"
#include "UnitDefHandler.h"
#include "command.h"
#include "guibuildmenu.h"
#include "infoconsole.h"
#include "mousehandler.h"
#include "selectedunits.h"

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

extern vector<UnitDef*> unitDefs;
extern vector<CommandDescription*> commands;
extern set<GUIbuildMenu*> buildMenus;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUIcenterBuildMenu::GUIcenterBuildMenu(int x, int y, int w, int h, Functor1<UnitDef*> select):GUIbuildMenu(x, y, w, h, select)
{
	rehideMouse=false;
}

GUIcenterBuildMenu::~GUIcenterBuildMenu()
{
}

void GUIcenterBuildMenu::PrivateDraw()
{
	int numx=6;
	if(unitDefs.size()>54)
		numx=12;
	int numy=1;
	while(numx*numy<unitDefs.size()){
		numy+=1;
	}
	x=(0.5*gu->screenx-numx*0.5*buildPicSize);
	y=(0.5*gu->screeny-numy*0.5*buildPicSize);
	w=numx*buildPicSize;
	h=numy*buildPicSize+left->h+10;

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
		glBindTexture(GL_TEXTURE_2D, unitDefs[i]->unitimage);

		int x=(i-menuOffset)%perRow*(buildPicSize+dist);
		int y=(i-menuOffset)/perRow*(buildPicSize+dist);

		if(i==hiliteNum&&isPressed)
			glColor4f(0.8, 0.8, 0.8,0.7);
		else
			glColor4f(1.0, 1.0, 1.0,0.7);

		DrawTexturedQuad(x,	y, buildPicSize, buildPicSize);

		if(!commands[i]->params.empty())
		{
			string queued="+ "+commands[i]->params[0];

			x+=(buildPicSize-guifont->GetWidth(queued))/2.0;
			y+=buildPicSize-guifont->GetHeight();
			guifont->Print(x, y, queued);
		}
	}
	
	glColor3f(1.0, 1.0, 1.0);	
	
	left->x=0;
	left->y=perCol*(buildPicSize+2*dist)+2;
	right->x=(perRow*(buildPicSize+dist))-right->w;
	right->y=perCol*(buildPicSize+2*dist)+2;

	glPopAttrib();
}


bool GUIcenterBuildMenu::MouseDownAction(int x, int y, int button)
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

bool GUIcenterBuildMenu::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
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

extern bool	keys[256];

bool GUIcenterBuildMenu::MouseUpAction(int x, int y, int button)
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

				if(keys[VK_SHIFT])
					c.options|=SHIFT_KEY;
				if(keys[VK_CONTROL])
					c.options|=CONTROL_KEY;
				if(keys[VK_MENU])
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

int GUIcenterBuildMenu::PicForPos(int x, int y)
{
	int perCol=(h-left->h-2*dist-2)/(buildPicSize+dist);
	int perRow=w/(buildPicSize+dist);

	if(x>(buildPicSize+dist)*perRow)
		return -1;

	if(y>(buildPicSize+dist)*perCol)
		return -1;

	int res=y/(buildPicSize+dist)*perRow+x/(buildPicSize+dist)+menuOffset;

	if(res>=unitDefs.size() || res<0)
		return -1;

	return res;
}

void GUIcenterBuildMenu::Move(GUIbutton* button)
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


bool GUIcenterBuildMenu::EventAction(const std::string& event)
{
	if(event=="show"){
		if(mouse->locked){
			mouse->ToggleState(false);
			rehideMouse=true;
		}
	}	else if(event=="hide"){
		if(rehideMouse && !mouse->locked)
			mouse->ToggleState(false);
		rehideMouse=false;
	}
	return GUIframe::EventAction(event);
}

string GUIcenterBuildMenu::Tooltip()
{
	if((hiliteNum>=0)&&(hiliteNum<commands.size()))
		return commands[hiliteNum]->tooltip;
	return GUIframe::Tooltip();
}
