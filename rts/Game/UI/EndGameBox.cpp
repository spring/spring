#include "StdAfx.h"
#include "EndGameBox.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Rendering/glFont.h"
#include "Net.h"
#include "Game/Game.h"
#include "Game/SelectedUnits.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/GL/VertexArray.h"
#include "LogOutput.h"
#include "Game/UI/GUI/GUIframe.h"

extern bool globalQuit;

static string FloatToSmallString(float num,float mul=1){
	char c[50];

	if(num==0)
		return "0";
	if(fabs(num)<10*mul){
		sprintf(c,"%.1f",num);
	} else if(fabs(num)<10000*mul){
		sprintf(c,"%.0f",num);
	} else if(fabs(num)<10000000*mul){
		sprintf(c,"%.0fk",num/1000);
	} else {
		sprintf(c,"%.0fM",num/1000000);
	}
	return c;
};

CEndGameBox::CEndGameBox(void)
{
	box.x1 = 0.14f;
	box.y1 = 0.1f;
	box.x2 = 0.86f;
	box.y2 = 0.8f;

	exitBox.x1=0.31;
	exitBox.y1=0.02;
	exitBox.x2=0.41;
	exitBox.y2=0.06;

	playerBox.x1=0.05;
	playerBox.y1=0.62;
	playerBox.x2=0.15;
	playerBox.y2=0.65;

	sumBox.x1=0.16;
	sumBox.y1=0.62;
	sumBox.x2=0.26;
	sumBox.y2=0.65;

	difBox.x1=0.27;
	difBox.y1=0.62;
	difBox.x2=0.38;
	difBox.y2=0.65;

	dispMode=0;
	stat1=1;
	stat2=2;

	CBitmap bm;
	if (!bm.Load("bitmaps/graphPaper.bmp"))
		throw content_error("Could not load bitmaps/graphPaper.bmp");
	graphTex=bm.CreateTexture();
}

CEndGameBox::~CEndGameBox(void)
{
	glDeleteTextures(1,&graphTex);
}

bool CEndGameBox::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box)){
		moveBox=true;
		if(InBox(mx,my,box+exitBox))
			moveBox=false;
		if(InBox(mx,my,box+playerBox))
			moveBox=false;
		if(InBox(mx,my,box+sumBox))
			moveBox=false;
		if(InBox(mx,my,box+difBox))
			moveBox=false;
		if(dispMode>0 && mx>box.x1+0.01 && mx<box.x1+0.12 && my<box.y1+0.57 && my>box.y1+0.571-stats.size()*0.02)
			moveBox=false;
		return true;
	}
	return false;
}

void CEndGameBox::MouseMove(int x, int y, int dx,int dy, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(moveBox){
		box.x1+=float(dx)/gu->screenx;
		box.x2+=float(dx)/gu->screenx;
		box.y1-=float(dy)/gu->screeny;
		box.y2-=float(dy)/gu->screeny;
	}
}

void CEndGameBox::MouseRelease(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box+exitBox)){
		delete this;
		globalQuit=true;
		return;
	}

	if(InBox(mx,my,box+playerBox)){
		dispMode=0;
	}
	if(InBox(mx,my,box+sumBox)){
		dispMode=1;
	}
	if(InBox(mx,my,box+difBox)){
		dispMode=2;
	}

	if(dispMode>0){
		if(mx>box.x1+0.01 && mx<box.x1+0.12 && my<box.y1+0.57 && my>box.y1+0.571-stats.size()*0.02){
			int sel=(int)floor(-(my-box.y1-0.57f)*50);

			if(button==1)
				stat1=sel;
			else
				stat2=sel;
		}
	}

}

bool CEndGameBox::IsAbove(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box))
		return true;
	return false;
}

void CEndGameBox::Draw()
{
	float mx=float(mouse->lastx)/gu->screenx;
	float my=(gu->screeny-float(mouse->lasty))/gu->screeny;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f,0.2f,0.2f,GUI_TRANS);
	DrawBox(box);

	glColor4f(0.2f,0.2f,0.7f,GUI_TRANS);
	if(dispMode==0){
		DrawBox(box+playerBox);
	} else if(dispMode==1){
		DrawBox(box+sumBox);
	} else {
		DrawBox(box+difBox);
	}

	if(InBox(mx,my,box+exitBox)){
		glColor4f(0.7f,0.2f,0.2f,GUI_TRANS);
		DrawBox(box+exitBox);
	}
	if(InBox(mx,my,box+playerBox)){
		glColor4f(0.7f,0.2f,0.2f,GUI_TRANS);
		DrawBox(box+playerBox);
	}
	if(InBox(mx,my,box+sumBox)){
		glColor4f(0.7f,0.2f,0.2f,GUI_TRANS);
		DrawBox(box+sumBox);
	}
	if(InBox(mx,my,box+difBox)){
		glColor4f(0.7f,0.2f,0.2f,GUI_TRANS);
		DrawBox(box+difBox);
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8);
	font->glPrintAt(box.x1+exitBox.x1+0.025,box.y1+exitBox.y1+0.005,1,"Exit");
	font->glPrintAt(box.x1+playerBox.x1+0.015,box.y1+playerBox.y1+0.005,0.7,"Player stats");
	font->glPrintAt(box.x1+sumBox.x1+0.015,box.y1+sumBox.y1+0.005,0.7,"Team stats");
	font->glPrintAt(box.x1+difBox.x1+0.015,box.y1+difBox.y1+0.005,0.7,"Team delta stats");

	if(gs->Team(gu->myTeam)->isDead){
		font->glPrintAt(box.x1+0.25,box.y1+0.65,1,"You lost the game");
	} else {
		font->glPrintAt(box.x1+0.25,box.y1+0.65,1,"You won the game");
	}

	if(dispMode==0){
		float xpos=0.01;
	
		string headers[]={"Name","MC/m","MP/m","KP/m","Cmds/m","ACS"};

		for(int a=0;a<6;++a){
			font->glPrintAt(box.x1+xpos,box.y1+0.55,0.8,headers[a].c_str());
			xpos+=0.1;
		}

		float ypos=0.5;
		for(int a=0;a<gs->activePlayers;++a){
			if(gs->players[a]->currentStats->mousePixels==0)
				continue;
			char values[6][100];

			sprintf(values[0],"%s",	gs->players[a]->playerName.c_str());
			sprintf(values[1],"%i",(int)(gs->players[a]->currentStats->mouseClicks*60/game->totalGameTime));
			sprintf(values[2],"%i",(int)(gs->players[a]->currentStats->mousePixels*60/game->totalGameTime));
			sprintf(values[3],"%i",(int)(gs->players[a]->currentStats->keyPresses*60/game->totalGameTime));
			sprintf(values[4],"%i",(int)(gs->players[a]->currentStats->numCommands*60/game->totalGameTime));
			sprintf(values[5],"%i",(int)
				( gs->players[a]->currentStats->numCommands != 0 ) ? 
				( gs->players[a]->currentStats->unitCommands/gs->players[a]->currentStats->numCommands) :
				( 0 ));
			
			float xpos=0.01;
			for(int a=0;a<6;++a){
				font->glPrintAt(box.x1+xpos,box.y1+ypos,0.8,values[a]);
				xpos+=0.1;
			}
				
			ypos-=0.02;
		}		
	} else {
		if(stats.empty())
			FillTeamStats();

		glBindTexture(GL_TEXTURE_2D, graphTex);
		CVertexArray* va=GetVertexArray();
		va->Initialize();

		va->AddVertexT(float3(box.x1+0.15, box.y1+0.08, 0),0,0);
		va->AddVertexT(float3(box.x1+0.69, box.y1+0.08, 0),4,0);
		va->AddVertexT(float3(box.x1+0.69, box.y1+0.62, 0),4,4);
		va->AddVertexT(float3(box.x1+0.15, box.y1+0.62, 0),0,4);

		va->DrawArrayT(GL_QUADS);

		if(mx>box.x1+0.01 && mx<box.x1+0.12 && my<box.y1+0.57 && my>box.y1+0.571-stats.size()*0.02){

			int sel=(int)floor(-(my-box.y1-0.57f)*50);

			glColor4f(0.7f,0.2f,0.2f,GUI_TRANS);
			glDisable(GL_TEXTURE_2D);
			CVertexArray* va=GetVertexArray();
			va->Initialize();

			va->AddVertex0(float3(box.x1+0.01, box.y1+0.55-sel*0.02 , 0));
			va->AddVertex0(float3(box.x1+0.01, box.y1+0.55-sel*0.02+0.02 , 0));
			va->AddVertex0(float3(box.x1+0.12, box.y1+0.55-sel*0.02+0.02 , 0));
			va->AddVertex0(float3(box.x1+0.12, box.y1+0.55-sel*0.02 , 0));

			va->DrawArray0(GL_QUADS);
			glEnable(GL_TEXTURE_2D);		
			glColor4f(1,1,1,0.8);
		}
		float ypos=0.55;
		for(int a=0;a<stats.size();++a){
			font->glPrintAt(box.x1+0.01,box.y1+ypos,0.8,stats[a].name.c_str());
			ypos-=0.02;
		}
		float maxy=1;
		if(dispMode==1)
			maxy=std::max(stats[stat1].max,stats[stat2].max);
		else
			maxy=std::max(stats[stat1].maxdif,stats[stat2].maxdif)/16;

		int numPoints=stats[0].values[0].size();
		float scalex=0.54f/max(1.0f,numPoints-1.0f);
		float scaley=0.54/maxy;

		for(int a=0;a<5;++a){
			font->glPrintAt(box.x1+0.12,box.y1+0.07+a*0.135,0.8,"%s",FloatToSmallString(maxy*0.25*a).c_str());
			font->glPrintAt(box.x1+0.135+a*0.135,box.y1+0.057,0.8,"%i:%2i",int(a*0.25*numPoints*16/60),int(a*0.25*(numPoints-1)*16)%60);
		}

		font->glPrintAt(box.x1+0.55,box.y1+0.65,0.8,"%s",stats[stat1].name.c_str());
		font->glPrintAt(box.x1+0.55,box.y1+0.63,0.8,"%s",stats[stat2].name.c_str());

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
				glVertex3f(box.x1+0.50,box.y1+0.66,0);
				glVertex3f(box.x1+0.55,box.y1+0.66,0);
		glEnd();

		glLineStipple(3,0x5555);
		glEnable(GL_LINE_STIPPLE);
		glBegin(GL_LINES);
				glVertex3f(box.x1+0.50,box.y1+0.64,0);
				glVertex3f(box.x1+0.55,box.y1+0.64,0);
		glEnd();
		glDisable(GL_LINE_STIPPLE);

		for(int team=0; team<gs->activeTeams; team++){
			glColor4ubv(gs->Team(team)->color);

			glBegin(GL_LINE_STRIP);
			for(int a=0;a<numPoints;++a){
				float value=0;
				if(dispMode==1)
					value=stats[stat1].values[team][a];
				else if(a>0)
					value=(stats[stat1].values[team][a]-stats[stat1].values[team][a-1])/16;

				glVertex3f(box.x1+0.15+a*scalex,box.y1+0.08+value*scaley,0);
			}
			glEnd();

			glLineStipple(3,0x5555);
			glEnable(GL_LINE_STIPPLE);

			glBegin(GL_LINE_STRIP);
			for(int a=0;a<numPoints;++a){
				float value=0;
				if(dispMode==1)
					value=stats[stat2].values[team][a];
				else if(a>0)
					value=(stats[stat2].values[team][a]-stats[stat2].values[team][a-1])/16;

				glVertex3f(box.x1+0.15+a*scalex,box.y1+0.08+value*scaley,0);
			}
			glEnd();

			glDisable(GL_LINE_STIPPLE);
		}
	}
}

std::string CEndGameBox::GetTooltip(int x,int y)
{
	float mx=float(x)/gu->screenx;

	if(dispMode==0){
		if(mx>box.x1+0.02 && mx<box.x1+0.1*6){
			string tips[]={"Player Name","Mouse clicks per minute","Mouse movement in pixels per minute","Keyboard presses per minute","Unit commands per minute","Average command size (units affected per command)"};

			return tips[int((mx-box.x1-0.01)*10)];
		}
	}

	return "No tooltip defined";
}

void CEndGameBox::FillTeamStats()
{
	stats.push_back(Stat(""));
	stats.push_back(Stat("Metal used"));
	stats.push_back(Stat("Energy used"));
	stats.push_back(Stat("Metal produced"));
	stats.push_back(Stat("Energy produced"));
	
	stats.push_back(Stat("Metal excess"));
	stats.push_back(Stat("Energy excess"));
	
	stats.push_back(Stat("Metal received"));
	stats.push_back(Stat("Energy received"));
	
	stats.push_back(Stat("Metal sent"));
	stats.push_back(Stat("Energy sent"));
	
	stats.push_back(Stat("Metal stored"));
	stats.push_back(Stat("Energy stored"));

	stats.push_back(Stat("Active Units"));
	stats.push_back(Stat("Units killed"));

	stats.push_back(Stat("Units produced"));
	stats.push_back(Stat("Units died"));
	
	stats.push_back(Stat("Units received"));
	stats.push_back(Stat("Units sent"));
	stats.push_back(Stat("Units captured"));
	stats.push_back(Stat("Units stolen"));
	
	stats.push_back(Stat("Damage Dealt"));
	stats.push_back(Stat("Damage Received"));
	
	for(int team=0; team<gs->activeTeams; team++){
		for(std::list<CTeam::Statistics>::iterator si=gs->Team(team)->statHistory.begin(); si!=gs->Team(team)->statHistory.end(); si++){
			stats[0].AddStat(team,0);				

			stats[1].AddStat(team, si->metalUsed);
			stats[2].AddStat(team, si->energyUsed);
			stats[3].AddStat(team, si->metalProduced);
			stats[4].AddStat(team, si->energyProduced);
			
			stats[5].AddStat(team, si->metalExcess);
			stats[6].AddStat(team, si->energyExcess);
			
			stats[7].AddStat(team, si->metalReceived);
			stats[8].AddStat(team, si->energyReceived);
			
			stats[9].AddStat(team, si->metalSent);
			stats[10].AddStat(team, si->energySent);

			stats[11].AddStat(team, si->metalProduced+si->metalReceived - (si->metalUsed+si->metalSent+si->metalExcess) );
			stats[12].AddStat(team, si->energyProduced+si->energyReceived - (si->energyUsed+si->energySent+si->energyExcess) );
			
			stats[13].AddStat(team, si->unitsProduced+si->unitsReceived+si->unitsCaptured - (si->unitsDied+si->unitsSent+si->unitsOutCaptured) );
			stats[14].AddStat(team, si->unitsKilled);

			stats[15].AddStat(team, si->unitsProduced);
			stats[16].AddStat(team, si->unitsDied);
			
			stats[17].AddStat(team, si->unitsReceived);
			stats[18].AddStat(team, si->unitsSent);
			stats[19].AddStat(team, si->unitsCaptured);
			stats[20].AddStat(team, si->unitsOutCaptured);

			stats[21].AddStat(team, si->damageDealt);
			stats[22].AddStat(team, si->damageReceived);
		}
	}	
}
