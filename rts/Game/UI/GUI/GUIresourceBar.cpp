#include "GUIresourceBar.h"
#include "Game/Team.h"
#include "GUIfont.h"
#include "Net.h"

static void Quad(float x, float y, float w, float h)
{
	glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x, y+h);
		glVertex2f(x+w, y+h);
		glVertex2f(x+w, y);
	glEnd();
}

GUIresourceBar::GUIresourceBar(int x, int y, int w, int h):GUIpane(x, y, w, h)
{
	SetFrame(false);
	SetResizeable(true);
	downInBar=false;
	minW=220;
	minH=h;
}

GUIresourceBar::~GUIresourceBar()
{
}

void GUIresourceBar::PrivateDraw()
{
	glPushAttrib(GL_CURRENT_BIT);

	glColor4f(0.0, 0.0, 0.0, 0.5);
	glBindTexture(GL_TEXTURE_2D, 0);

	Quad(0, 0, w, h);
	
	const float offset=5;
	float middle=(h-guifont->GetHeight())/2.0;
	float mWidth=guifont->GetWidth("Metal")+5;
	float eWidth=guifont->GetWidth("Energy")+5;
	glColor3f(1, 1, 1);
	guifont->Print(offset, middle, "Metal");
	guifont->Print((w+offset)/2.0, middle, "Energy");
	
	char buf[500];
	
	glColor3f(0, 1, 0);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->oldMetalIncome);
	guifont->Print(offset+mWidth, middle-5, 0.8, buf);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->oldEnergyIncome);
	guifont->Print((w+offset)/2.0+eWidth, middle-5, 0.8, buf);

	glColor3f(1, 0.2, 0.2);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->oldMetalExpense);
	guifont->Print(offset+mWidth, middle+5, 0.8, buf);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->oldEnergyExpense);
	guifont->Print((w+offset)/2.0+eWidth, middle+5, 0.8, buf);

	glColor3f(0, 1, 0);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->metal);
	guifont->Print(offset+mWidth + 30, middle, 0.8, buf);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->energy);
	guifont->Print((w+offset)/2.0+eWidth + 30, middle, 0.8, buf);

	glColor3f(1, 0.2, 0.2);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->metalStorage);
	guifont->Print(w/2-90, middle, 0.8, buf);
	sprintf(buf, "%.1f", gs->Team(gu->myTeam)->energyStorage);
	guifont->Print(w-90, middle, 0.8, buf);

	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f(1, 1, 1, 0.2);
	float barHeight=guifont->GetHeight()/2.0;
	middle=(h-barHeight)/2.0;

	float barStart=mWidth+offset+95;
	float barLength=w/2.0-barStart-100;
	
	float metal=gs->Team(gu->myTeam)->metal;
	float maxMetal=gs->Team(gu->myTeam)->metalStorage;
	float metalShared=gs->Team(gu->myTeam)->metalShare;
	float energy=gs->Team(gu->myTeam)->energy;
	float maxEnergy=gs->Team(gu->myTeam)->energyStorage;
	float energyShared=gs->Team(gu->myTeam)->energyShare;

	Quad(barStart, middle, barLength, barHeight);

	glColor4f(0, 0.5, 1, 0.8);
	
	Quad(barStart, middle, barLength*metal/maxMetal, barHeight);

	glColor4f(0, 1, 0, 0.8);
	Quad(barStart, middle+barHeight, barLength*metalShared/maxMetal, 2);

	glColor4f(1, 1, 1, 0.2);
	barStart=w/2.0+eWidth+offset+95;
	barLength=w-barStart-100;
	
	Quad(barStart, middle, barLength, barHeight);
	
	glColor4f(1, 1, 0, 0.8);
	
	Quad(barStart, middle, barLength*energy/maxEnergy, barHeight);
	
	glColor4f(0, 1, 0, 0.8);
	Quad(barStart, middle+barHeight, barLength*energyShared/maxEnergy, 2);
	
	glPopAttrib();
}

void GUIresourceBar::SizeChanged()
{
	h=23;
}



bool GUIresourceBar::MouseMoveAction(int x1, int y1, int xrel, int yrel, int buttons)
{
	if(downInBar)
	{
		const float offset=5;
		float mWidth=guifont->GetWidth("Metal")+5;
		float eWidth=guifont->GetWidth("Energy")+5;

		float barStart=mWidth+offset+95;
		float barEnd=w/2.0-100;

		if(x1>barStart-20&&x1<barEnd+20)
		{
			float rel=(x1-barStart)/(barEnd-barStart);
			rel = max(rel, 0.0f);
			rel = min(rel, 1.0f);
			float metalShare=gs->Team(gu->myTeam)->metalStorage*rel;
			netbuf[0]=NETMSG_SETSHARE;
			netbuf[1]=gu->myTeam;
			*(float*)&netbuf[2]=metalShare;
			*(float*)&netbuf[6]=gs->Team(gu->myTeam)->energyShare;
			net->SendData(netbuf,10);
		}
		
		barStart=w/2.0+eWidth+offset+95;
		barEnd=w-100;
		if(x1>barStart-20&&x1<barEnd+20)
		{
			float rel=(x1-barStart)/(barEnd-barStart);
			rel = max(rel, 0.0f);
			rel = min(rel, 1.0f);
			float energyShare=gs->Team(gu->myTeam)->energyStorage*rel;
			netbuf[0]=NETMSG_SETSHARE;
			netbuf[1]=gu->myTeam;
			*(float*)&netbuf[2]=gs->Team(gu->myTeam)->metalShare;
			*(float*)&netbuf[6]=energyShare;
			net->SendData(netbuf,10);
			
		}
		return true;
	}
	return GUIpane::MouseMoveAction(x1, y1, xrel, yrel, buttons);
}

bool GUIresourceBar::MouseDownAction(int x1, int y1, int button)
{
	if(LocalInside(x1, y1)&&button==1)
	{
		downInBar=true;
		MouseMoveAction(x1, y1, 0, 0, button);
		return true;
	}
	return GUIpane::MouseDownAction(x1, y1, button);
}

bool GUIresourceBar::MouseUpAction(int x1, int y1, int button)
{
	if(downInBar)
	{
		downInBar=false;
		return true;
	}
	return GUIpane::MouseUpAction(x1, y1, button);
}
