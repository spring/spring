#include "StdAfx.h"
#include "InMapDraw.h"
#include "Net.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "GL/myGL.h"
#include "GL/VertexArray.h"
#include "glFont.h"
#include "Map/BaseGroundDrawer.h"
#include "Game/Game.h"
#include "LogOutput.h"
#include "Sound.h"
#include "Game/UI/MiniMap.h"
#include "SDL_mouse.h"
#include "SDL_keyboard.h"

CInMapDraw* inMapDrawer;

#define DRAW_QUAD_SIZE 32

CInMapDraw::CInMapDraw(void)
{
	keyPressed=false;
	wantLabel=false;
	lastLineTime=0;
	lastLeftClickTime=0;
	lastPos=float3(1,1,1);

	drawQuadsX=gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY=gs->mapy/DRAW_QUAD_SIZE;
	numQuads=drawQuadsX*drawQuadsY;
	drawQuads=new DrawQuad[numQuads];

	unsigned char tex[64][128][4];
	for(int y=0;y<64;y++){
		for(int x=0;x<128;x++){
			tex[y][x][0]=0;
			tex[y][x][1]=0;
			tex[y][x][2]=0;
			tex[y][x][3]=0;
		}
	}
	for(int y=0;y<64;y++){	//circular thingy
		for(int x=0;x<64;x++){
			float dist=sqrt((float)(x-32)*(x-32)+(y-32)*(y-32));
			if (dist > 31.875f) {
				// do nothing - leave transparent
			} else if (dist > 24.5f) {
				// black outline
				tex[y][x][3]=255;
			} else {
				tex[y][x][0]=255;
				tex[y][x][1]=255;
				tex[y][x][2]=255;
				tex[y][x][3]=200;
			}
		}
	}
	for(int y=0;y<64;y++){	//linear falloff
		for(int x=0;x<64;x++){
			float dist=abs(y-32);
			if(dist > 24.5f) {
				// black outline
				tex[y][x+64][3]=255;
			} else {
				tex[y][x+64][0]=255;
				tex[y][x+64][1]=255;
				tex[y][x+64][2]=255;
				tex[y][x+64][3]=200;
			}
		}
	}
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8,128, 64, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);

	blippSound=sound->GetWaveId("sounds/beep6.wav");
}

CInMapDraw::~CInMapDraw(void)
{
	delete[] drawQuads;
	glDeleteTextures (1, &texture);
}

struct InMapDraw_QuadDrawer : public CReadMap::IQuadDrawer
{
	CVertexArray *va, *lineva;
	CInMapDraw *imd;
	unsigned int texture;

	void DrawQuad (int x,int y);
};

void InMapDraw_QuadDrawer::DrawQuad (int x,int y)
{
	int drawQuadsX = imd->drawQuadsX;
	CInMapDraw::DrawQuad* dq=&imd->drawQuads[y*drawQuadsX+x];

	for(std::list<CInMapDraw::MapPoint>::iterator pi=dq->points.begin();pi!=dq->points.end();++pi){
		float3 pos=pi->pos;

		float3 dif(pos-camera->pos);
		float camDist=dif.Length();
		dif/=camDist;
		float3 dir1(dif.cross(UpVector));
		dir1.Normalize();
		float3 dir2(dif.cross(dir1));

		unsigned char col[4];
		col[0]=pi->color[0];
		col[1]=pi->color[1];
		col[2]=pi->color[2];
		col[3]=200;//intensity*255;

		float size=6;

		float3 pos1=pos;
		float3 pos2=pos1;
		pos2.y+=100;

		va->AddVertexTC(pos1-dir1*size,					  0.25f,0,col);
		va->AddVertexTC(pos1+dir1*size,					  0.25f,1,col);
		va->AddVertexTC(pos1+dir1*size+dir2*size, 0.00f,1,col);
		va->AddVertexTC(pos1-dir1*size+dir2*size, 0.00f,0,col);

		va->AddVertexTC(pos1-dir1*size,0.75f,0,col);
		va->AddVertexTC(pos1+dir1*size,0.75f,1,col);
		va->AddVertexTC(pos2+dir1*size,0.75f,1,col);
		va->AddVertexTC(pos2-dir1*size,0.75f,0,col);

		va->AddVertexTC(pos2-dir1*size,					  0.25f,0,col);
		va->AddVertexTC(pos2+dir1*size,					  0.25f,1,col);
		va->AddVertexTC(pos2+dir1*size-dir2*size, 0.00f,1,col);
		va->AddVertexTC(pos2-dir1*size-dir2*size, 0.00f,0,col);

		if(pi->label.size()>0){
			glPushMatrix();
			glTranslatef3(pi->pos+UpVector*105);
			glScalef(30,30,30);
			glColor4ub(pi->color[0],pi->color[1],pi->color[2],250);
			font->glWorldPrint("%s",pi->label.c_str());
			glPopMatrix();
			glBindTexture(GL_TEXTURE_2D, texture);
		}
	}
	for(std::list<CInMapDraw::MapLine>::iterator li=dq->lines.begin();li!=dq->lines.end();++li){
		lineva->AddVertexC(li->pos-(li->pos - camera->pos).Normalize()*26,li->color);
		lineva->AddVertexC(li->pos2-(li->pos2 - camera->pos).Normalize()*26,li->color);
	}
}

void CInMapDraw::Draw(void)
{
	glDepthMask(0);

	CVertexArray* va=GetVertexArray();
	va->Initialize();

	CVertexArray* lineva=GetVertexArray();
	lineva->Initialize();

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D,texture);

	InMapDraw_QuadDrawer drawer;
	drawer.imd = this;
	drawer.lineva = lineva;
	drawer.va = va;
	drawer.texture = texture;

	readmap->GridVisibility (camera, DRAW_QUAD_SIZE, 3000.0f, &drawer);

	glDisable(GL_TEXTURE_2D);
	glLineWidth(3);
	lineva->DrawArrayC(GL_LINES);
	glLineWidth(1);
	glEnable(GL_TEXTURE_2D);
	va->DrawArrayTC(GL_QUADS);
	glDepthMask(1);
}

void CInMapDraw::MousePress(int x, int y, int button)
{
	float3 pos=GetMouseMapPos();
	if(pos.x<0)
		return;

	switch(button){
	case SDL_BUTTON_LEFT:
		if(lastLeftClickTime>gu->gameTime-0.3f){
			PromptLabel(pos);
		}
		lastLeftClickTime=gu->gameTime;
		break;
	case SDL_BUTTON_RIGHT:
		ErasePos(pos);
		break;
	case SDL_BUTTON_MIDDLE:{
		CreatePoint(pos,"");
		break;}
	}
	lastPos=pos;
}

void CInMapDraw::MouseRelease(int x,int y,int button)
{

}

void CInMapDraw::MouseMove(int x, int y, int dx,int dy, int button)
{
	float3 pos=GetMouseMapPos();
	if(pos.x<0)
		return;

	if(mouse->buttons[SDL_BUTTON_LEFT].pressed && lastLineTime<gu->gameTime-0.05f){
		AddLine(pos,lastPos);
		lastLineTime=gu->gameTime;
		lastPos=pos;
	}
	if(mouse->buttons[SDL_BUTTON_RIGHT].pressed)
		ErasePos(pos);

}

float3 CInMapDraw::GetMouseMapPos(void)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*gu->viewRange*1.4f);
	if(dist<0){
		return float3(-1,1,-1);
	}
	float3 pos=camera->pos+mouse->dir*dist;
	pos.CheckInBounds();
	return pos;
}

void CInMapDraw::GotNetMsg(unsigned char* msg)
{
	int team=gs->players[msg[2]]->team;
	int allyteam=gs->AllyTeam(team);
	if((!gs->Ally(gu->myAllyTeam,allyteam) || !gs->Ally(allyteam,gu->myAllyTeam) || gs->players[msg[2]]->spectator) && !gu->spectating)
		return;

	switch(msg[3]){
	case NET_POINT:{
		float3 pos(*(short*)&msg[4],0,*(short*)&msg[6]);
		pos.CheckInBounds();
		pos.y=ground->GetHeight(pos.x,pos.z)+2;
		MapPoint p;
		p.pos=pos;
		p.color=gs->Team(team)->color;
		p.label=(char*)&msg[8];

		int quad=int(pos.z/DRAW_QUAD_SIZE/SQUARE_SIZE)*drawQuadsX+int(pos.x/DRAW_QUAD_SIZE/SQUARE_SIZE);
		drawQuads[quad].points.push_back(p);

		logOutput.Print("%s added point: %s",gs->players[msg[2]]->playerName.c_str(),p.label.c_str());
		logOutput.SetLastMsgPos(pos);
		sound->PlaySample(blippSound);
		minimap->AddNotification(pos,float3(1,1,1),1);	//todo: make compatible with new gui
		break;}
	case NET_ERASE:{
		float3 pos(*(short*)&msg[4],0,*(short*)&msg[6]);
		pos.CheckInBounds();
		for(int y=(int)max(0.f,(pos.z-100)/DRAW_QUAD_SIZE/SQUARE_SIZE);y<=min(float(drawQuadsY-1),(pos.z+100)/DRAW_QUAD_SIZE/SQUARE_SIZE);++y){
			for(int x=(int)max(0.f,(pos.x-100)/DRAW_QUAD_SIZE/SQUARE_SIZE);x<=min(float(drawQuadsX-1),(pos.x+100)/DRAW_QUAD_SIZE/SQUARE_SIZE);++x){
				DrawQuad* dq=&drawQuads[y*drawQuadsX+x];
				for(std::list<MapPoint>::iterator pi=dq->points.begin();pi!=dq->points.end();){
					if(pi->pos.distance2D(pos)<100)
						pi=dq->points.erase(pi);
					else
						++pi;
				}
				for(std::list<MapLine>::iterator li=dq->lines.begin();li!=dq->lines.end();){
					if(li->pos.distance2D(pos)<100)
						li=dq->lines.erase(li);
					else
						++li;
				}
			}
		}
		break;}
	case NET_LINE:{
		float3 pos(*(short*)&msg[4],0,*(short*)&msg[6]);
		pos.CheckInBounds();
		float3 pos2(*(short*)&msg[8],0,*(short*)&msg[10]);
		pos2.CheckInBounds();
		pos.y=ground->GetHeight(pos.x,pos.z)+2;
		pos2.y=ground->GetHeight(pos2.x,pos2.z)+2;
		MapLine l;
		l.pos=pos;
		l.pos2=pos2;
		l.color=gs->Team(team)->color;

		int quad=int(pos.z/DRAW_QUAD_SIZE/SQUARE_SIZE)*drawQuadsX+int(pos.x/DRAW_QUAD_SIZE/SQUARE_SIZE);
		drawQuads[quad].lines.push_back(l);
		break;}
	}
}

void CInMapDraw::ErasePos(float3 pos)
{
	net->SendData<unsigned char, unsigned char, unsigned char, short, short>(
			NETMSG_MAPDRAW, 8 /*message size*/, gu->myPlayerNum, NET_ERASE, (short)pos.x, (short)pos.z);
}

void CInMapDraw::CreatePoint(float3 pos, std::string label)
{
	net->SendSTLData<unsigned char, unsigned char, short, short, std::string>(
			NETMSG_MAPDRAW, gu->myPlayerNum, NET_POINT, (short)pos.x, (short)pos.z, label);
}

void CInMapDraw::AddLine(float3 pos, float3 pos2)
{
	net->SendData<unsigned char, unsigned char, unsigned char, short, short, short, short>(
			NETMSG_MAPDRAW, 12 /*message size*/, gu->myPlayerNum, NET_LINE,
			(short)pos.x, (short)pos.z, (short)pos2.x, (short)pos2.z);
}


void CInMapDraw::PromptLabel (float3 pos)
{
	waitingPoint=pos;
	game->userWriting=true;
	wantLabel=true;
	game->userPrompt="Label: ";
	game->ignoreChar='\xA7';		//should do something better here
}

