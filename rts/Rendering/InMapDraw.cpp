#include "StdAfx.h"
#include "InMapDraw.h"
#include "Net.h"
#include "Sim/Map/Ground.h"
#include "Game/Camera.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "GL/myGL.h"
#include "GL/VertexArray.h"
#include "glFont.h"
#include "Map/BaseGroundDrawer.h"
#include "Game/Game.h"
#include "Game/UI/InfoConsole.h"
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
			if(dist>31.875)
				dist=31.875;
			tex[y][x][0]=255;
			tex[y][x][1]=255;
			tex[y][x][2]=255;
			tex[y][x][3]=(unsigned char) (255-dist*8);
		}
	}
	for(int y=0;y<64;y++){	//linear falloff
		for(int x=0;x<64;x++){
			float dist=abs(y-32);
			if(dist>31.5)
				dist=31.5;
			tex[y][x+64][0]=255;
			tex[y][x+64][1]=255;
			tex[y][x+64][2]=255;
			tex[y][x+64][3]=(unsigned char) (255-dist*8);
		}
	}
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8,128, 64, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);

	blippSound=sound->GetWaveId("beep6.wav");
}

CInMapDraw::~CInMapDraw(void)
{
	delete[] drawQuads;
	glDeleteTextures (1, &texture);
}

void CInMapDraw::Draw(void)
{
	glDepthMask(0);

	CVertexArray* va=GetVertexArray();
	va->Initialize();

	CVertexArray* lineva=GetVertexArray();
	lineva->Initialize();

	int cx=(int)(camera->pos.x/(SQUARE_SIZE*DRAW_QUAD_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*DRAW_QUAD_SIZE));

	float drawDist=3000;
	int drawSquare=int(drawDist/(SQUARE_SIZE*DRAW_QUAD_SIZE))+1;

	int sy=cy-drawSquare;
	if(sy<0)
		sy=0;
	int ey=cy+drawSquare;
	if(ey>drawQuadsY-1)
		ey=drawQuadsY-1;

	for(int y=sy;y<=ey;y++){
		int sx=cx-drawSquare;
		if(sx<0)
			sx=0;
		int ex=cx+drawSquare;
		if(ex>drawQuadsX-1)
			ex=drawQuadsX-1;
		float xtest,xtest2;
		std::vector<CBaseGroundDrawer::fline>::iterator fli;
		for(fli=groundDrawer->left.begin();fli!=groundDrawer->left.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*DRAW_QUAD_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*DRAW_QUAD_SIZE)+DRAW_QUAD_SIZE)));
			if(xtest>xtest2)
				xtest=xtest2;
			xtest=xtest/DRAW_QUAD_SIZE;
			if(xtest>sx)
				sx=(int)xtest;
		}
		for(fli=groundDrawer->right.begin();fli!=groundDrawer->right.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*DRAW_QUAD_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*DRAW_QUAD_SIZE)+DRAW_QUAD_SIZE)));
			if(xtest<xtest2)
				xtest=xtest2;
			xtest=xtest/DRAW_QUAD_SIZE;
			if(xtest<ex)
				ex=(int)xtest;
		}
		for(int x=sx;x<=ex;x++){/**/
			DrawQuad* dq=&drawQuads[y*drawQuadsX+x];

			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);

			glBindTexture(GL_TEXTURE_2D,texture);

			for(std::list<MapPoint>::iterator pi=dq->points.begin();pi!=dq->points.end();++pi){
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

				va->AddVertexTC(pos1-dir1*size,					  0.25,0,col);
				va->AddVertexTC(pos1+dir1*size,					  0.25,1,col);
				va->AddVertexTC(pos1+dir1*size+dir2*size, 0.00,1,col);
				va->AddVertexTC(pos1-dir1*size+dir2*size, 0.00,0,col);

				va->AddVertexTC(pos1-dir1*size,0.75,0,col);
				va->AddVertexTC(pos1+dir1*size,0.75,1,col);
				va->AddVertexTC(pos2+dir1*size,0.75,1,col);
				va->AddVertexTC(pos2-dir1*size,0.75,0,col);

				va->AddVertexTC(pos2-dir1*size,					  0.25,0,col);
				va->AddVertexTC(pos2+dir1*size,					  0.25,1,col);
				va->AddVertexTC(pos2+dir1*size-dir2*size, 0.00,1,col);
				va->AddVertexTC(pos2-dir1*size-dir2*size, 0.00,0,col);

				if(pi->label.size()>0){
					glPushMatrix();
					glTranslatef3(pi->pos+UpVector*105);
					glScalef(30,30,30);
					glColor4ub(pi->color[0],pi->color[1],pi->color[2],250);
					font->glWorldPrint("%s",pi->label.c_str());
					glPopMatrix();
				}
			}
			for(std::list<MapLine>::iterator li=dq->lines.begin();li!=dq->lines.end();++li){
				lineva->AddVertexC(li->pos-(li->pos - camera->pos).Normalize()*26,li->color);
				lineva->AddVertexC(li->pos2-(li->pos2 - camera->pos).Normalize()*26,li->color);
			}
		}
	}

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
		if(lastLeftClickTime>gu->gameTime-0.3){
			waitingPoint=pos;
			game->userWriting=true;
			wantLabel=true;
			game->userPrompt="Label: ";
			game->ignoreChar='§';		//should do something better here
			SDL_EnableUNICODE(1);
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

	if(mouse->buttons[SDL_BUTTON_LEFT].pressed && lastLineTime<gu->gameTime-0.05){
		AddLine(pos,lastPos);
		lastLineTime=gu->gameTime;
		lastPos=pos;
	}
	if(mouse->buttons[SDL_BUTTON_RIGHT].pressed)
		ErasePos(pos);

}

float3 CInMapDraw::GetMouseMapPos(void)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*9000);
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

		info->AddLine("%s added point: %s",gs->players[msg[2]]->playerName.c_str(),p.label.c_str());
		sound->PlaySound(blippSound);
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
	netbuf[0]=NETMSG_MAPDRAW;
	netbuf[1]=8;
	netbuf[2]=gu->myPlayerNum;
	netbuf[3]=NET_ERASE;
	*(short*)&netbuf[4]=(short int)pos.x;
	*(short*)&netbuf[6]=(short int)pos.z;
	net->SendData(netbuf,netbuf[1]);	
}

void CInMapDraw::CreatePoint(float3 pos, std::string label)
{
	netbuf[0]=NETMSG_MAPDRAW;
	netbuf[1]=label.size()+9;
	netbuf[2]=gu->myPlayerNum;
	netbuf[3]=NET_POINT;
	*(short*)&netbuf[4]=(short int)pos.x;
	*(short*)&netbuf[6]=(short int)pos.z;
	for(int a=0;a<label.size();++a)
		netbuf[a+8]=label[a];
	netbuf[label.size()+8]=0;
	net->SendData(netbuf,netbuf[1]);
}

void CInMapDraw::AddLine(float3 pos, float3 pos2)
{
	netbuf[0]=NETMSG_MAPDRAW;
	netbuf[1]=12;
	netbuf[2]=gu->myPlayerNum;
	netbuf[3]=NET_LINE;
	*(short*)&netbuf[4]=(short int)pos.x;
	*(short*)&netbuf[6]=(short int)pos.z;
	*(short*)&netbuf[8]=(short int)pos2.x;
	*(short*)&netbuf[10]=(short int)pos2.z;
	net->SendData(netbuf,netbuf[1]);	
}
