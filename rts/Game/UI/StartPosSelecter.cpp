/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "StartPosSelecter.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/Team.h"
#include "NetProtocol.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Rendering/InMapDraw.h"


CStartPosSelecter* CStartPosSelecter::selector = NULL;


CStartPosSelecter::CStartPosSelecter(void) : CInputReceiver(BACK)
{
	showReady = true;
	startPosSet = false;
	selector = this;
	readyBox.x1 = 0.71f;
	readyBox.y1 = 0.72f;
	readyBox.x2 = 0.81f;
	readyBox.y2 = 0.76f;
}


CStartPosSelecter::~CStartPosSelecter(void)
{
	selector = NULL;
}


bool CStartPosSelecter::Ready()
{
	if (gs->frameNum > 0) {
		delete this;
		return true;
	}

	if (!startPosSet) // Player doesn't set startpos yet, so don't let him ready up
		return false;

	net->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, gu->myTeam, 1, startPos.x, startPos.y, startPos.z));

	delete this;
	return true;
}


bool CStartPosSelecter::MousePress(int x, int y, int button)
{
	float mx = MouseX(x);
	float my = MouseY(y);
	if ((showReady && InBox(mx, my, readyBox)) || gs->frameNum > 0) {
		return !Ready();
	}

	float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*globalRendering->viewRange*1.4f);
	if(dist<0)
		return true;

	startPosSet = true;
	inMapDrawer->SendErase(startPos);
	startPos = camera->pos + mouse->dir * dist;

	if(startPos.z<gameSetup->allyStartingData[gu->myAllyTeam].startRectTop *gs->mapy*8)
		startPos.z=gameSetup->allyStartingData[gu->myAllyTeam].startRectTop*gs->mapy*8;

	if(startPos.z>gameSetup->allyStartingData[gu->myAllyTeam].startRectBottom*gs->mapy*8)
		startPos.z=gameSetup->allyStartingData[gu->myAllyTeam].startRectBottom*gs->mapy*8;

	if(startPos.x<gameSetup->allyStartingData[gu->myAllyTeam].startRectLeft*gs->mapx*8)
		startPos.x=gameSetup->allyStartingData[gu->myAllyTeam].startRectLeft*gs->mapx*8;

	if(startPos.x>gameSetup->allyStartingData[gu->myAllyTeam].startRectRight*gs->mapx*8)
		startPos.x=gameSetup->allyStartingData[gu->myAllyTeam].startRectRight*gs->mapx*8;

	net->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, gu->myTeam, 0, startPos.x, startPos.y, startPos.z));

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

	glColor4f(0.2f,0.8f,0.2f,0.5f);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
	float by= gameSetup->allyStartingData[gu->myAllyTeam].startRectTop *gs->mapy*8;
	float bx= gameSetup->allyStartingData[gu->myAllyTeam].startRectLeft *gs->mapx*8;

	float dy = (gameSetup->allyStartingData[gu->myAllyTeam].startRectBottom - gameSetup->allyStartingData[gu->myAllyTeam].startRectTop) *gs->mapy*8/10;
	float dx = (gameSetup->allyStartingData[gu->myAllyTeam].startRectRight - gameSetup->allyStartingData[gu->myAllyTeam].startRectLeft) *gs->mapx*8/10;

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

	float mx=float(mouse->lastx)/globalRendering->viewSizeX;
	float my=(globalRendering->viewSizeY-float(mouse->lasty))/globalRendering->viewSizeY;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	if (!showReady) {
		return;
	}

	if (InBox(mx, my, readyBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
	} else {
		glColor4f(0.7f, 0.7f, 0.2f, guiAlpha);
	}
	DrawBox(readyBox);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (InBox(mx, my, readyBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
	} else {
		glColor4f(0.7f, 0.7f, 0.2f, guiAlpha);
	}
	DrawBox(readyBox);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// fit text into box
	const float unitWidth  = font->GetSize() * font->GetTextWidth("Ready") * globalRendering->pixelX;
	const float unitHeight = font->GetSize() * font->GetLineHeight() * globalRendering->pixelY;

	const float ySize = (readyBox.y2 - readyBox.y1);
	const float xSize = (readyBox.x2 - readyBox.x1);

	const float fontScale = 0.9f * std::min(xSize/unitWidth, ySize/unitHeight);
	const float yPos = 0.5f * (readyBox.y1 + readyBox.y2);
	const float xPos = 0.5f * (readyBox.x1 + readyBox.x2);

	font->Begin(); 
	font->SetColors(); // default
	font->glPrint(xPos, yPos, fontScale, FONT_OUTLINE | FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM, "Ready");
	font->End(); 
}
