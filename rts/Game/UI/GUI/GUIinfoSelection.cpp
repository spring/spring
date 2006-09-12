#include "StdAfx.h"
#include "GUIinfoSelection.h"
#include "GUIcontroller.h"
#include "GUIfont.h"
#include "Net.h"
#include "Game/SelectedUnits.h"
#include "ExternalAI/GroupHandler.h"
#include "ExternalAI/Group.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Units/UnitDefHandler.h"

extern CSelectedUnits selectedUnits;
extern CGroupHandler* grouphandler;
extern CUnitDefHandler* unitDefHandler;
extern vector<UnitDef*> unitDefs;


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

static void Quad(float x, float y, float w, float h)
{
	glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x, y+h);
		glVertex2f(x+w, y+h);
		glVertex2f(x+w, y);
	glEnd();
}

GUIinfoSelection::GUIinfoSelection(int x, int y, int w, int h):GUIpane(x, y, w, h)
{
	SetFrame(false);
	SetResizeable(true);
	minW=220;
	minH=h;
}

GUIinfoSelection::~GUIinfoSelection()
{
}

void GUIinfoSelection::PrivateDraw()
{


	glPushAttrib(GL_CURRENT_BIT);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBindTexture(GL_TEXTURE_2D, 0);

	Quad(0, 0, w, h);

	glColor3f(0, 1, 0);

	map<UnitDef*,GroupUnitsInfo> lstUnits;
	
	char buf[500];

	int i = 0;
	set<CUnit*>::iterator ui;
	if(selectedUnits.selectedGroup!=-1){
		for(ui=grouphandler->groups[selectedUnits.selectedGroup]->units.begin();ui!=grouphandler->groups[selectedUnits.selectedGroup]->units.end();++ui){
			i++;
			lstUnits[(*ui)->unitDef].number = lstUnits[(*ui)->unitDef].number+1;
			lstUnits[(*ui)->unitDef].health = (int)(lstUnits[(*ui)->unitDef].health+(*ui)->health);
			lstUnits[(*ui)->unitDef].maxHealth = (int)(lstUnits[(*ui)->unitDef].maxHealth+(*ui)->maxHealth);
			lstUnits[(*ui)->unitDef].id = (*ui)->unitDef->id;
			if ((*ui)->stockpileWeapon) {
				lstUnits[(*ui)->unitDef].numStockpiled = lstUnits[(*ui)->unitDef].numStockpiled + (*ui)->stockpileWeapon->numStockpiled;
				lstUnits[(*ui)->unitDef].numStockpileQued = lstUnits[(*ui)->unitDef].numStockpileQued + (*ui)->stockpileWeapon->numStockpileQued;
			}
		}
	} else {
		for(ui=selectedUnits.selectedUnits.begin();ui!=selectedUnits.selectedUnits.end();++ui){
			i++;
			lstUnits[(*ui)->unitDef].number = lstUnits[(*ui)->unitDef].number+1;
			lstUnits[(*ui)->unitDef].health = (int)(lstUnits[(*ui)->unitDef].health+(*ui)->health);
			lstUnits[(*ui)->unitDef].maxHealth = (int)(lstUnits[(*ui)->unitDef].maxHealth+(*ui)->maxHealth);
			lstUnits[(*ui)->unitDef].id = (*ui)->unitDef->id;
			if ((*ui)->stockpileWeapon) {
				lstUnits[(*ui)->unitDef].numStockpiled = lstUnits[(*ui)->unitDef].numStockpiled + (*ui)->stockpileWeapon->numStockpiled;
				lstUnits[(*ui)->unitDef].numStockpileQued = lstUnits[(*ui)->unitDef].numStockpileQued + (*ui)->stockpileWeapon->numStockpileQued;
			}
		}
	}
	i=0;
	map<UnitDef*,GroupUnitsInfo>::iterator lsti;
	for(lsti=lstUnits.begin();lsti!=lstUnits.end();lsti++){
		i++;
		float hpp = (float)(lsti->second.health)/(lsti->second.maxHealth);		
		if(hpp>0.5f)
			glColor3f(1.0f-((hpp-0.5f)*2.0f),1.0f,0.0f);
		else
			glColor3f(1.0f,hpp*2.0f,0.0f);
		sprintf(buf,"%d x",lsti->second.number);
		guifont->Print(13,buildPicSize*i,1,buf);
		glBindTexture(GL_TEXTURE_2D,unitDefHandler->GetUnitImage(lsti->first));
		int x=48;
		int y=buildPicSize*(i-1)+buildPicSize/2;
		glColor3f(1.0f, 1.0f, 1.0f);
		DrawTexturedQuad(x,	y, buildPicSize, buildPicSize);	
		if (lsti->second.numStockpiled != 0 || lsti->second.numStockpileQued != 0) {
			glColor3f(1.0f,1.0f,0.0f);
			sprintf(buf,"(%d/%d)",lsti->second.numStockpiled,lsti->second.numStockpileQued);
			guifont->Print(50,buildPicSize*i,1,buf);
		}
	}
	
	h=(i+(i!=0))* buildPicSize;
	glPopAttrib();
}

void GUIinfoSelection::SizeChanged()
{
	//h=23;
}



bool GUIinfoSelection::MouseMoveAction(int x1, int y1, int xrel, int yrel, int buttons)
{	
	return GUIpane::MouseMoveAction(x1, y1, xrel, yrel, buttons);
}

bool GUIinfoSelection::MouseDownAction(int x1, int y1, int button)
{
	return GUIpane::MouseDownAction(x1, y1, button);
}

bool GUIinfoSelection::MouseUpAction(int x1, int y1, int button)
{
	if(LocalInside(x1, y1)){	
        
	}
	
    return GUIpane::MouseUpAction(x1, y1, button);
}
