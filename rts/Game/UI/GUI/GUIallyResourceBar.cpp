#include "GUIallyResourceBar.h"
#include "Game/Team.h"
#include "GUIfont.h"
#include "Game/Player.h"

static void Quad(float x, float y, float w, float h)
{
	glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x, y+h);
		glVertex2f(x+w, y+h);
		glVertex2f(x+w, y);
	glEnd();
}

GUIallyResourceBar::GUIallyResourceBar(int x, int y, int w, int h):GUIpane(x, y, w, h)
{
	SetFrame(false);
	SetResizeable(true);
	downInBar=false;
	minW=220;
	minH=h;
}

GUIallyResourceBar::~GUIallyResourceBar()
{
}

void GUIallyResourceBar::PrivateDraw()
{
	int numAllies=0;
	for(int a=0;a<gs->activeTeams;++a){
		if(gs->Ally(gs->AllyTeam(a),gu->myAllyTeam) && a!=gu->myTeam)
			numAllies++;
	}
	h=numAllies*32;
	w=200;

	glPushAttrib(GL_CURRENT_BIT);

	glColor4f(0.0, 0.0, 0.0, 0.5);
	glBindTexture(GL_TEXTURE_2D, 0);

	Quad(0, 0, w, h);
	
	int yoff=0;
	for(int a=0;a<gs->activeTeams;++a){
		if(gs->Ally(gs->AllyTeam(a),gu->myAllyTeam) && a!=gu->myTeam){
			glColor3f(1, 1, 1);
			guifont->Print(2, yoff+10, gs->players[gs->Team(a)->leader]->playerName.c_str());
			guifont->Print(65, yoff+2, "%.0f",gs->Team(a)->metal);
			guifont->Print(65, yoff+18, "%.0f",gs->Team(a)->energy);
			
			glBindTexture(GL_TEXTURE_2D, 0);

			glColor4f(0.1f, 0.1f, 0.3f, 0.8f);
			Quad(120,yoff+5,75,6);

			glColor4f(0.8f, 0.8f, 1.0f, 0.8f);
			Quad(120,yoff+5,75*gs->Team(a)->metal/gs->Team(a)->metalStorage,6);

			glColor4f(0.2f, 0.2f, 0.1f, 0.8f);
			Quad(120,yoff+21,75,6);

			glColor4f(1.0f, 1.0f, 0.5f, 0.8f);
			Quad(120,yoff+21,75*gs->Team(a)->energy/gs->Team(a)->energyStorage,6);

			yoff+=32;
		}
	}
	glPopAttrib();
}

void GUIallyResourceBar::SizeChanged()
{
	h=23;
}

