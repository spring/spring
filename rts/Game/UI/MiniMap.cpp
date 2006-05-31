// MiniMap.cpp: implementation of the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "StdAfx.h"
#include "MiniMap.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Game/Camera.h"
#include "InfoConsole.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Game/Team.h"
#include "Sim/Misc/LosHandler.h"
#include "Map/Ground.h"
#include "MouseHandler.h"
#include "Game/CameraController.h"
#include "Game/SelectedUnits.h"
#include "GuiHandler.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Map/MapDamage.h"
#include "Map/MetalMap.h"
#include "Platform/ConfigHandler.h"
#include "SDL_types.h"
#include "SDL_keysym.h"
#include "SDL_mouse.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMiniMap* minimap;
extern Uint8 *keys;

CMiniMap::CMiniMap()
: xpos(10),
	ypos(gu->screeny-210),
	height(200),
	width(200),
	mouseMove(false),
	mouseResize(false),
	mouseLook(false),
	maximized(false),
	minimized(false)
{
	unsigned char tex[16][16][4];
	for(int y=0;y<16;++y){
		float dy=y-7.5;
		for(int x=0;x<16;++x){
			float dx=x-7.5;
			float dist=sqrt(dx*dx+dy*dy);
			if(dist<6 || dist>9){
				tex[y][x][0]=255;
				tex[y][x][1]=255;
				tex[y][x][2]=255;
			} else {
				tex[y][x][0]=0;
				tex[y][x][1]=0;
				tex[y][x][2]=0;
			}
			if(dist<7){
				tex[y][x][3]=255;
			} else {
				tex[y][x][3]=0;
			}
		}
	}
	glGenTextures(1, &unitBlip);
	glBindTexture(GL_TEXTURE_2D, unitBlip);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,16, 16, GL_RGBA, GL_UNSIGNED_BYTE, tex);

	float hw=sqrt(float(gs->mapx)/gs->mapy);
	width = (int) (width*hw);
	height = (int) (height/hw);
	ypos=gu->screeny-height-10;

	simpleColors=!!configHandler.GetInt("SimpleMiniMapColors",0);
	myColor[0]=(unsigned char)(0.2*255);
	myColor[1]=(unsigned char)(0.9*255);
	myColor[2]=(unsigned char)(0.2*255);
	myColor[3]=(unsigned char)(1*255);
	allyColor[0]=(unsigned char)(0.3*255);
	allyColor[1]=(unsigned char)(0.3*255);
	allyColor[2]=(unsigned char)(0.9*255);
	allyColor[3]=(unsigned char)(1*255);
	enemyColor[0]=(unsigned char)(0.9*255);
	enemyColor[1]=(unsigned char)(0.2*255);
	enemyColor[2]=(unsigned char)(0.2*255);
	enemyColor[3]=(unsigned char)(1*255);
}

CMiniMap::~CMiniMap()
{
	glDeleteTextures (1, &unitBlip);
}

void CMiniMap::Draw()
{
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	if(minimized){
		glColor4f(1,1,1,0.5);
		glDisable(GL_TEXTURE_2D);
		glViewport(0,gu->screeny-10,10,10);
		glBegin(GL_QUADS);
		glVertex2f(0,0);
		glVertex2f(1,0);
		glVertex2f(1,1);
		glVertex2f(0,1);
		glEnd();
		glViewport(0,0,gu->screenx,gu->screeny);
		return;
	}
	glViewport(xpos,ypos,width,height);
	glLoadIdentity();									// Reset The Current Modelview Matrix
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix

	glColor4f(0.6,0.6,0.6,1);

	readmap->DrawMinimap ();

	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, unitBlip);
	glBegin(GL_QUADS);
	float size=0.2f/sqrt((float)width+height);
	list<CUnit*>::iterator ui;
	for(ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();ui++){
		DrawUnit(*ui,size);
	}
	glEnd();

	glDisable(GL_TEXTURE_2D);

	left.clear();
	
	//Add restraints for camera sides
	GetFrustumSide(cam2->bottom);
	GetFrustumSide(cam2->top);
	GetFrustumSide(cam2->rightside);
	GetFrustumSide(cam2->leftside);

	std::vector<fline>::iterator fli,fli2;
  for(fli=left.begin();fli!=left.end();fli++){
	  for(fli2=left.begin();fli2!=left.end();fli2++){
			if(fli==fli2)
				continue;
			float colz=0;
			if(fli->dir-fli2->dir==0)
				continue;
			colz=-(fli->base-fli2->base)/(fli->dir-fli2->dir);
			if(fli2->left*(fli->dir-fli2->dir)>0){
				if(colz>fli->minz && colz<400096)
					fli->minz=colz;
			} else {
				if(colz<fli->maxz && colz>-10000)
					fli->maxz=colz;
			}
		}
	}
	glColor4f(1,1,1,0.5);
  glBegin(GL_LINES);
  for(fli=left.begin();fli!=left.end();fli++){
		if(fli->minz<fli->maxz){
			DrawInMap(float3(fli->base+fli->dir*fli->minz,0,fli->minz));
			DrawInMap(float3(fli->base+fli->dir*fli->maxz,0,fli->maxz));
		}
	}
	glEnd();

	glColor4f(1,1,1,0.0002f*(width+height));
	glBegin(GL_POINTS);
		Projectile_List::iterator psi;
		for(psi=ph->ps.begin();psi != ph->ps.end();++psi){
			CProjectile* p=*psi;
			if((p->owner && p->owner->allyteam==gu->myAllyTeam) || loshandler->InLos(p,gu->myAllyTeam)){
				DrawInMap((*psi)->pos);
			}
		}
	glEnd();

	for(set<CUnit*>::iterator si=selectedUnits.selectedUnits.begin();si!=selectedUnits.selectedUnits.end();++si){
		if((*si)->radarRadius && !(*si)->beingBuilt && (*si)->activated){
			glColor3f(0.3f,1,0.3f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrtf(height)*(*si)->radarRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->radarRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->sonarRadius && !(*si)->beingBuilt && (*si)->activated){
			glColor3f(0.3f,1,0.3f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrtf(height)*(*si)->sonarRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->sonarRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->jammerRadius && !(*si)->beingBuilt && (*si)->activated){
			glColor3f(1.0f,0.2f,0.2f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrtf(height)*(*si)->jammerRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->jammerRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->stockpileWeapon && (*si)->stockpileWeapon->weaponDef->interceptor){		//change if someone someday create a non stockpiled interceptor
			CWeapon* w=(*si)->stockpileWeapon;
			if(w->numStockpiled)
				glColor4f(1.0f,1.0f,1.0f,1.0f);
			else
				glColor4f(0.0f,0.0f,0.0f,0.6f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrtf(height)*w->weaponDef->coverageRange/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*w->weaponDef->coverageRange);
			}
			glEnd();
		}
	}

	DrawNotes();

	glColor4f(1,1,1,0.5);
	glViewport(xpos,ypos,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(xpos+width-10,ypos,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(xpos+width-10,ypos+height-10,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(xpos,ypos+height-10,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(0,0,gu->screenx,gu->screeny);
}



void CMiniMap::DrawInMap(const float3& pos)
{
	glVertex2f(pos.x/(gs->mapx*SQUARE_SIZE),1-pos.z/(gs->mapy*SQUARE_SIZE));
}

void CMiniMap::GetFrustumSide(float3& side)
{
	fline temp;
	float3 up(0,1,0);
	
	float3 b=up.cross(side);		//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001)
		b.z=0.00011f;
	if(fabs(b.z)>0.0001){
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		float3 colpoint;				//a point on the collision line
		
		if(side.y>0)								
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);
		else
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);
		
		
		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		temp.left=-1;
		if(b.z>0)
			temp.left=1;
		temp.maxz=gs->mapy*SQUARE_SIZE;
		temp.minz=0;
		left.push_back(temp);			
	}	

}

void CMiniMap::DrawUnit(CUnit* unit,float size)
{
	if(unit->lastDamage>gs->frameNum-90 && gs->frameNum&8)
		return;
	float3 pos=unit->pos;
/*	if(pos.z<0 || pos.z>gs->mapy*SQUARE_SIZE){
		info->AddLine("Errenous position in minimap::drawunit %f %f %f",pos.x,pos.y,pos.z);
		return;
	}*/
	if(unit->allyteam==gu->myAllyTeam || (unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating){
	}else if((unit->losStatus[gu->myAllyTeam] & LOS_INRADAR)){
		pos+=unit->posErrorVector*radarhandler->radarErrorSize[gu->myAllyTeam];
	}else{
		return;
	}
	if(unit->commandAI->selected)
		glColor3f(1,1,1);
	else {
		if(simpleColors){
			if(unit->team==gu->myTeam){
				glColor3ubv(myColor);
			} else if (gs->Ally(gu->myAllyTeam,unit->allyteam)){
				glColor3ubv(allyColor);
			} else {
				glColor3ubv(enemyColor);
			}
		} else {
			glColor3ubv(gs->Team(unit->team)->color);
		}
	}

	float x=pos.x/(gs->mapx*SQUARE_SIZE);
	float y=1-pos.z/(gs->mapy*SQUARE_SIZE);

	float aspectRatio=float(width)/float(height);

	glTexCoord2f(0,0);
	glVertex2f(x-size,y-size*aspectRatio);
	glTexCoord2f(0,1);
	glVertex2f(x-size,y+size*aspectRatio);
	glTexCoord2f(1,1);
	glVertex2f(x+size,y+size*aspectRatio);
	glTexCoord2f(1,0);
	glVertex2f(x+size,y-size*aspectRatio);
}


bool CMiniMap::MousePress(int x, int y, int button)
{
	if(minimized && x<10 && y<10){
		minimized=false;
		return true;
	}
	if(minimized)
		return false;

	if((x>xpos) && (x<xpos+width) && (y>gu->screeny-ypos-height) && (y<gu->screeny-ypos)){
		if(button==SDL_BUTTON_LEFT){
			if(x>xpos+width-10 && y>gu->screeny-ypos-10)
				mouseResize=true;
			else if (x<xpos+10 && y<gu->screeny-ypos-height+10)
				mouseMove=true;
			return true;
		} else if(button==SDL_BUTTON_RIGHT){
			MoveView(x,y);
			mouseLook=true;
			return true;
		}
	}
	return false;
}

void CMiniMap::MouseMove(int x, int y, int dx, int dy, int button)
{
	if(mouseMove){
		xpos+=dx;
		ypos-=dy;
		return;
	} else  if(mouseResize){
		ypos-=dy;
		height+=dy;
		width+=dx;
	 if(keys[SDLK_LSHIFT])
		{
			width = height * gs->mapx/gs->mapy;
		}

		return;
	} else if(mouseLook && (x>xpos) && (x<xpos+width) && (y>gu->screeny-ypos-height) && (y<gu->screeny-ypos)){
		MoveView(x,y);
		return;
	}
}

void CMiniMap::MouseRelease(int x, int y, int button)
{
	if(mouseMove || mouseResize || mouseLook){
		mouseMove=false;
		mouseResize=false;
		mouseLook=false;
		return;
	}
	if(button==SDL_BUTTON_LEFT){
		if(x>xpos+width-10 && y<gu->screeny-ypos-height+10){
			if(maximized){
				maximized=false;
				xpos=oldxpos;
				ypos=oldypos;
				height=oldheight;
				width=oldwidth;
			} else {
				maximized=true;
				oldxpos=xpos;
				oldypos=ypos;
				oldheight=height;
				oldwidth=width;
				height=gu->screeny;
				width=height;
				xpos=(gu->screenx-gu->screeny)/2;
				ypos=0;
			}
			return;
		}
		if(x<xpos+10 && y>gu->screeny-ypos-10){
			minimized=true;
			return;
		}
		float3 pos(float(x-xpos)/width*gs->mapx*SQUARE_SIZE,500,float(y-(gu->screeny-ypos-height))/height*gs->mapx*SQUARE_SIZE);
//		info->AddLine("x %f y %f",pos.x,pos.z);
		if(guihandler->inCommand!=-1){
			FakeMousePress(pos,0);
			return;
		}
		float size=0.2f/sqrt((float)width+height)*gs->mapx*SQUARE_SIZE;
//		info->AddLine("r %f",size);
		CUnit* unit=helper->GetClosestFriendlyUnit(pos,size,gu->myAllyTeam);
		if(unit){
		if(!keys[SDLK_LSHIFT])
		        selectedUnits.ClearSelected();
			selectedUnits.AddUnit(unit);
			return;
		}
		FakeMousePress(pos,1);
		return;
	}
}

void CMiniMap::MoveView(int x, int y)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+camera->forward*gu->viewRange*1.4);
	float3 dif(0,0,0);
	if(dist>0){
		dif=camera->forward*dist;
	}
	float camHeight=camera->pos.y-ground->GetHeight(camera->pos.x,camera->pos.z);
	float3 clickPos;
	clickPos.x=(float(x-xpos))/width*gs->mapx*8;
	clickPos.z=(float(y-(gu->screeny-ypos-height)))/height*gs->mapy*8;
	mouse->currentCamController->SetPos(clickPos);
}

void CMiniMap::FakeMousePress(float3 pos, int button)
{
	CCamera c=*camera;
	camera->pos=pos;
	camera->forward=float3(0,-1,0);
	mouse->buttons[button].x=gu->screenx/2;
	mouse->buttons[button].y=gu->screeny/2;
	mouse->buttons[button].camPos=camera->pos;
	mouse->buttons[button].dir=camera->forward;
	guihandler->MouseRelease(gu->screenx/2,gu->screeny/2,button);
	*camera=c;
}

bool CMiniMap::IsAbove(int x, int y)
{
	if(minimized && x<10 && y<10){
		return true;
	}
	if(minimized)
		return false;

	if((x>xpos) && (x<xpos+width) && (y>gu->screeny-ypos-height) && (y<gu->screeny-ypos)){
		return true;
	}
	return false;
}

std::string CMiniMap::GetTooltip(int x, int y)
{
	if(minimized)
		return "Unminimize map";

	if(x<xpos+10 && y>gu->screeny-ypos-10)
		return "Minimize map";

	if(x>xpos+width-10 && y<gu->screeny-ypos-height+10)
		return "Maximize map";

	if(x>xpos+width-10 && y>gu->screeny-ypos-10)
		return "Resize map";

	if (x<xpos+10 && y<gu->screeny-ypos-height+10)
		return "Move map";

	float3 pos(float(x-xpos)/width*gs->mapx*SQUARE_SIZE,500,float(y-(gu->screeny-ypos-height))/height*gs->mapx*SQUARE_SIZE);

	char tmp[500];

	CReadMap::TerrainType* tt=&readmap->terrainTypes[readmap->typemap[min(gs->hmapx*gs->hmapy-1,max(0,((int)pos.z/16)*gs->hmapx+((int)pos.x/16)))]];
	string ttype=tt->name;
	sprintf(tmp,"Pos %.0f %.0f Elevation %.0f\nTerrain type: %s\nSpeeds T/K/H/S %.2f %.2f %.2f %.2f\nHardness %.0f Metal %.1f",pos.x,pos.z,ground->GetHeight2(pos.x,pos.z),ttype.c_str(),tt->tankSpeed,tt->kbotSpeed,tt->hoverSpeed,tt->shipSpeed,tt->hardness*mapDamage->mapHardness,readmap->metalMap->getMetalAmount((int)(pos.x/16),(int)(pos.z/16)));
	return tmp;
}

void CMiniMap::AddNotification(float3 pos, float3 color, float alpha)
{
	Notification n;
	n.pos=pos;
	n.color=color;
	n.alpha=alpha;
	n.creationTime=gu->gameTime;

	notes.push_back(n);
}

void CMiniMap::DrawNotes(void)
{
	float baseSize=gs->mapx*SQUARE_SIZE;
	glBegin(GL_LINES);
	for(list<Notification>::iterator ni=notes.begin();ni!=notes.end();){
		float age=gu->gameTime-ni->creationTime;
		if(age>2){
			ni=notes.erase(ni);
			continue;
		}
		glColor4f(ni->color.x,ni->color.y,ni->color.z,ni->alpha);
		for(int a=0;a<3;++a){
			float modage=age+a*0.1;
			float rot=modage*3;
			float size=baseSize-modage*baseSize*0.9;
			if(size<0){
				if(size<-baseSize*0.4)
					continue;
				else if(size>-baseSize*0.2)
					size=modage*baseSize*0.9-baseSize;
				else
					size=baseSize*1.4-modage*baseSize*0.9;
			}
			DrawInMap(ni->pos+float3(sin(rot),0,cos(rot))*size);	
			DrawInMap(ni->pos+float3(cos(rot),0,-sin(rot))*size);	

			DrawInMap(ni->pos+float3(cos(rot),0,-sin(rot))*size);	
			DrawInMap(ni->pos+float3(-sin(rot),0,-cos(rot))*size);	

			DrawInMap(ni->pos+float3(-sin(rot),0,-cos(rot))*size);	
			DrawInMap(ni->pos+float3(-cos(rot),0,sin(rot))*size);	

			DrawInMap(ni->pos+float3(-cos(rot),0,sin(rot))*size);	
			DrawInMap(ni->pos+float3(sin(rot),0,cos(rot))*size);	
		}
		++ni;
	}
	glEnd();
}
