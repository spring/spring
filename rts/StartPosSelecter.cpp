#include "StdAfx.h"
#include "StartPosSelecter.h"
#include "MouseHandler.h"
#include "myGL.h"
#include "glFont.h"
#include "GameSetup.h"
#include "Team.h"
#include "Net.h"
#include "Ground.h"
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
	
	if(pos.z<gameSetup->startRectTop[gu->myAllyTeam]*gs->mapy*8)
		pos.z=gameSetup->startRectTop[gu->myAllyTeam]*gs->mapy*8;

	if(pos.z>gameSetup->startRectBottom[gu->myAllyTeam]*gs->mapy*8)
		pos.z=gameSetup->startRectBottom[gu->myAllyTeam]*gs->mapy*8;

	if(pos.x<gameSetup->startRectLeft[gu->myAllyTeam]*gs->mapx*8)
		pos.x=gameSetup->startRectLeft[gu->myAllyTeam]*gs->mapx*8;

	if(pos.x>gameSetup->startRectRight[gu->myAllyTeam]*gs->mapx*8)
		pos.x=gameSetup->startRectRight[gu->myAllyTeam]*gs->mapx*8;

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

	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);

	camera->Update(true);

	glColor4f(0.2,0.8,0.2,0.5);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
	float by=gameSetup->startRectTop[gu->myAllyTeam]*gs->mapy*8;
	float bx=gameSetup->startRectLeft[gu->myAllyTeam]*gs->mapx*8;

	float dy=(gameSetup->startRectBottom[gu->myAllyTeam]-gameSetup->startRectTop[gu->myAllyTeam])*gs->mapy*8/10;
	float dx=(gameSetup->startRectRight[gu->myAllyTeam]-gameSetup->startRectLeft[gu->myAllyTeam])*gs->mapx*8/10;

	for(int a=0;a<10;++a){	//draw start rect restrictions
		float3 pos1(bx+a*dx,0,by);
		pos1.y=ground->GetHeight(pos1.x,pos1.z);
		float3 pos2(bx+(a+1)*dx,0,by);
		pos2.y=ground->GetHeight(pos2.x,pos2.z);

		glVertexf3(pos1);
		glVertexf3(pos2);
		glVertexf3(pos2+UpVector*100);
		glVertexf3(pos1+UpVector*100);

		pos1=float3(bx+a*dx,0,by+dy*10);
		pos1.y=ground->GetHeight(pos1.x,pos1.z);
		pos2=float3(bx+(a+1)*dx,0,by+dy*10);
		pos2.y=ground->GetHeight(pos2.x,pos2.z);

		glVertexf3(pos1);
		glVertexf3(pos2);
		glVertexf3(pos2+UpVector*100);
		glVertexf3(pos1+UpVector*100);

		pos1=float3(bx,0,by+dy*a);
		pos1.y=ground->GetHeight(pos1.x,pos1.z);
		pos2=float3(bx,0,by+dy*(a+1));
		pos2.y=ground->GetHeight(pos2.x,pos2.z);

		glVertexf3(pos1);
		glVertexf3(pos2);
		glVertexf3(pos2+UpVector*100);
		glVertexf3(pos1+UpVector*100);

		pos1=float3(bx+dx*10,0,by+dy*a);
		pos1.y=ground->GetHeight(pos1.x,pos1.z);
		pos2=float3(bx+dx*10,0,by+dy*(a+1));
		pos2.y=ground->GetHeight(pos2.x,pos2.z);

		glVertexf3(pos1);
		glVertexf3(pos2);
		glVertexf3(pos2+UpVector*100);
		glVertexf3(pos1+UpVector*100);
	}
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDisable(GL_DEPTH_TEST);

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
