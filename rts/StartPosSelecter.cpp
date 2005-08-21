#include "StdAfx.h"
#include "StartPosSelecter.h"
#include "MouseHandler.h"
#include "myGL.h"
#include "glFont.h"
#include "GameSetup.h"
#include "Team.h"
#include "Net.h"
#include "Ground.h"
#include "MouseHandler.h"
#include "Camera.h"
#include "InMapDraw.h"

CStartPosSelecter::CStartPosSelecter(void)
{
	readyBox.x1=0.71;
	readyBox.y1=0.72;
	readyBox.x2=0.81;
	readyBox.y2=0.76;
}

CStartPosSelecter::~CStartPosSelecter(void)
{
}

bool CStartPosSelecter::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,readyBox) && gs->teams[gu->myTeam]->startPos.y!=-500){
		gameSetup->readyTeams[gu->myTeam]=true;
		netbuf[0]=NETMSG_STARTPOS;
		netbuf[1]=gu->myTeam;
		netbuf[2]=1;
		*(float*)&netbuf[3]=gs->teams[gu->myTeam]->startPos.x;
		*(float*)&netbuf[7]=gs->teams[gu->myTeam]->startPos.y;
		*(float*)&netbuf[11]=gs->teams[gu->myTeam]->startPos.z;
		net->SendData(netbuf,15);
		delete this;
		return false;
	}
	float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
	if(dist<0)
		return true;
	float3 pos=camera->pos+mouse->dir*dist;

	inMapDrawer->ErasePos(gs->teams[gu->myTeam]->startPos);

	netbuf[0]=NETMSG_STARTPOS;
	netbuf[1]=gu->myTeam;
	netbuf[2]=0;
	*(float*)&netbuf[3]=pos.x;
	*(float*)&netbuf[7]=pos.y;
	*(float*)&netbuf[11]=pos.z;
	net->SendData(netbuf,15);

	char t[500];
	sprintf(t,"Start %i",gu->myTeam);
	inMapDrawer->CreatePoint(pos,t);

	return true;
}

void CStartPosSelecter::Draw()
{
	if(gu->spectating){
		delete this;
		return;
	}

	float mx=float(mouse->lastx)/gu->screenx;
	float my=(gu->screeny-float(mouse->lasty))/gu->screeny;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	if(InBox(mx,my,readyBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(readyBox);
	}
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8);
	font->glPrintAt(readyBox.x1+0.025,readyBox.y1+0.005,1,"Ready");
}
