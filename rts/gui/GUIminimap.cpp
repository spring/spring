#include "GUIminimap.h"
#include "GUIgame.h"
#include "Bitmap.h"
#include "GlobalStuff.h"
#include "3DOParser.h"
#include "CobInstance.h"
#include "UnitDef.h"
#include "BaseGroundDrawer.h"
#include "UnitHandler.h"
#include "Camera.h"
#include "ProjectileHandler.h"
#include "Projectile.h"
#include "SelectedUnits.h"
#include "Unit.h"
#include "LosHandler.h"
#include "InfoConsole.h"
#include "CommandAI.h"
#include "Team.h"
#include "MouseHandler.h"
#include "CameraController.h"

#include "RadarHandler.h"
#include "Weapon.h"
#include "WeaponDefHandler.h"
#include "SmfReadMap.h"


GUIminimap::GUIminimap():GUIframe(0, 0, 128, 128)
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
	//w*=hw;
	//h/=hw;
	downInMinimap=false;
}

GUIminimap::~GUIminimap()
{
	glDeleteTextures (1, &unitBlip);
}

void GUIminimap::PrivateDraw()
{
	if(parent)
	{
		w=parent->w;
		h=parent->h;
	}

	int xpos=x;
	int ypos=y;
	int width=w;
	int height=h;
	
	GUIframe *p=parent;
	while(p)
	{
		xpos+=p->x;
		ypos+=p->y;
		p=p->parent;
	}
	
	ypos=gu->screeny-ypos-height;
	
	
	glPushAttrib(GL_ALL_ATTRIB_BITS|GL_VIEWPORT_BIT);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	

	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glViewport(xpos,ypos,width,height);
	glLoadIdentity();									// Reset The Current Modelview Matrix
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix

	glColor4f(0.6,0.6,0.6,1);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D, ((CSmfReadMap*)readmap)->shadowTex);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,2);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, readmap->minimapTex);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		
  glActiveTextureARB(GL_TEXTURE0_ARB);

	if((groundDrawer->drawLos || groundDrawer->drawExtraTex)){
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		glBindTexture(GL_TEXTURE_2D, groundDrawer->infoTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	float isx=gs->hmapx/256.0;
	float isy=gs->hmapy/256.0;

	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,1);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,0,isy);
		glVertex2f(0,0);
		glTexCoord2f(0,0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,0,0);
		glVertex2f(0,1);
		glTexCoord2f(1,0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,0);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,isx,0);
		glVertex2f(1,1);
		glTexCoord2f(1,1);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,1);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,isx,isy);
		glVertex2f(1,0);
	glEnd();

	glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE0_ARB);

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
			DrawInMap((*psi)->pos);
		}
	glEnd();

	for(set<CUnit*>::iterator si=selectedUnits.selectedUnits.begin();si!=selectedUnits.selectedUnits.end();++si){
		if((*si)->radarRadius){
			glColor3f(0.3f,1,0.3f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrtf(height)*(*si)->radarRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->radarRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->sonarRadius){
			glColor3f(0.3f,1,0.3f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrtf(height)*(*si)->sonarRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->sonarRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->jammerRadius){
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

	
	
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	glPopAttrib();
}

void GUIminimap::DrawInMap(const float3& pos)
{
	glVertex2f(pos.x/(gs->mapx*SQUARE_SIZE),1-pos.z/(gs->mapy*SQUARE_SIZE));
}

void GUIminimap::GetFrustumSide(float3& side)
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

void GUIminimap::DrawUnit(CUnit* unit,const float size)
{
if(unit->lastDamage>gs->frameNum-90 && gs->frameNum&8)
		return;
	float3 pos=unit->pos;
/*	if(pos.z<0 || pos.z>gs->mapy*SQUARE_SIZE){
		guicontroller->AddText("Errenous position in minimap::drawunit %f %f %f",pos.x,pos.y,pos.z);
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
	else 
		glColor3ubv(gs->teams[unit->team]->color);

	float x=pos.x/(gs->mapx*SQUARE_SIZE);
	float y=1-pos.z/(gs->mapy*SQUARE_SIZE);

	glTexCoord2f(0,0);
	glVertex2f(x-size,y-size);
	glTexCoord2f(0,1);
	glVertex2f(x-size,y+size);
	glTexCoord2f(1,1);
	glVertex2f(x+size,y+size);
	glTexCoord2f(1,0);
	glVertex2f(x+size,y-size);


}

void GUIminimap::SizeChanged()
{
}

bool GUIminimap::MouseUpAction(int x, int y, int button)
{
	if(downInMinimap&&LocalInside(x, y))
	{
		MoveView(x, y, button);
		downInMinimap=false;
		return true;
	}
	downInMinimap=false;
	return false;
}

bool GUIminimap::MouseDownAction(int x, int y, int button)
{
	if(button!=1&&button!=3)
		return false;

	downInMinimap=LocalInside(x, y);

	return downInMinimap;
}

bool GUIminimap::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
{
	if(parent)
	{
		w=parent->w;
		h=parent->h;
	}
	
	if(!LocalInside(x,y))
		return false;

	MoveView(x, y, -1);

	if(button!=1&&button!=3)
		return false;

	if(downInMinimap&&button)
	{
		MoveView(x, y, button);
		return true;	
	}

	return false;
}

void GUIminimap::MoveView(int xpos, int ypos, int button)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+camera->forward*9000);
	float3 dif(0,0,0);
	if(dist>0){
		dif=camera->forward*dist;
	}
	float camHeight=camera->pos.y-ground->GetHeight(camera->pos.x,camera->pos.z);
	float3 clickPos;
	
	float xrel=xpos/(float)w;
	float yrel=ypos/(float)h;
	
	// FIXME: magic number: is this 8 maybe SQUARE_SIZE?
	clickPos.x=xrel*gs->mapx*8;
	clickPos.z=yrel*gs->mapy*8;
	float3 finalPos=clickPos-dif;

	if(guiGameControl->DirectCommand(clickPos, button))
	{
		downInMinimap=false;
		return;
	}
	
	if(button!=1)
		return;

	mouse->currentCamController->SetPos(clickPos);
}

void GUIminimap::SelectCursor()
{
	guiGameControl->SelectCursor();
}
