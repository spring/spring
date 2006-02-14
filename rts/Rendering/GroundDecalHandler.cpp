#include "StdAfx.h"
#include "GroundDecalHandler.h"
#include <algorithm>
#include "Rendering/Textures/Bitmap.h"
#include "GL/myGL.h"
#include "GL/VertexArray.h"
#include "Sim/Units/Unit.h"
#include "Sim/Map/Ground.h"
#include "Sim/Units/UnitDef.h"
#include "Game/UI/InfoConsole.h"
#include "Platform/ConfigHandler.h"
#include <cctype>
#include "Game/Camera.h"
#include "mmgr.h"

CGroundDecalHandler* groundDecals=0;
using namespace std;

CGroundDecalHandler::CGroundDecalHandler(void)
{
	decalLevel=configHandler.GetInt("GroundDecals",1);

	if(decalLevel==0)
		return;

	unsigned char* buf=new unsigned char[512*512*4];
	memset(buf,0,512*512*4);

	LoadScar("bitmaps/scars/scar2.bmp",buf,0,0);
	LoadScar("bitmaps/scars/scar3.bmp",buf,256,0);
	LoadScar("bitmaps/scars/scar1.bmp",buf,0,256);
	LoadScar("bitmaps/scars/scar4.bmp",buf,256,256);

	glGenTextures(1, &scarTex);			
	glBindTexture(GL_TEXTURE_2D, scarTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, buf);

//	CBitmap bm(buf,512,512);
//	bm.Save("scar.jpg");
	
	scarFieldX=gs->mapx/32;
	scarFieldY=gs->mapy/32;
	scarField=new std::set<Scar*>[scarFieldX*scarFieldY];
	
	lastTest=0;
	maxOverlap=decalLevel+1;

	delete[] buf;
	/*	
	unsigned char buf[128*128*4];

	for(int y=0;y<128;++y){
		for(int x=0;x<128;++x){
			buf[(y*128+x)*4+0]=180;//60+gu->usRandFloat()*20;
			buf[(y*128+x)*4+1]=0;//40+gu->usRandFloat()*10;
			buf[(y*128+x)*4+2]=255;//20+gu->usRandFloat()*5;
			buf[(y*128+x)*4+3]=0;
		}
	}
	for(int y=0;y<20;++y){
		for(int x=0;x<128;++x){
			buf[(y*128+x)*4+1]=255*min(1.0,max(0.4,sin(x*PI/16)*0.5+0.15))*(y==0?0:1);
			buf[((y+108)*128+x)*4+1]=255*min(1.0,max(0.4,sin(x*PI/16)*0.5+0.15))*(y==19?0:1);
		}
	}
	CBitmap bm(buf,128,128);
	bm.Save("StdTank.bmp");
	tankTex=bm.CreateTexture(true);*/
}

CGroundDecalHandler::~CGroundDecalHandler(void)
{
	for(std::vector<TrackType*>::iterator tti=trackTypes.begin();tti!=trackTypes.end();++tti){
		for(set<UnitTrackStruct*>::iterator ti=(*tti)->tracks.begin();ti!=(*tti)->tracks.end();++ti){
			delete *ti;
		}
		glDeleteTextures (1, &(*tti)->texture);
		delete *tti;
	}
	for(std::list<Scar*>::iterator si=scars.begin();si!=scars.end();++si){
		delete *si;
	}
	if(decalLevel!=0){
		delete[] scarField;

		glDeleteTextures(1,&scarTex);
	}
}

void CGroundDecalHandler::Draw(void)
{
	if(decalLevel==0)
		return;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.01);
	glDepthMask(0);
	glPolygonOffset(-5,-10);
	glEnable(GL_POLYGON_OFFSET_FILL);

	unsigned char color[4];
	color[0]=255;
	color[1]=255;
	color[2]=255;
	color[3]=255;
	unsigned char color2[4];
	color2[0]=255;
	color2[1]=255;
	color2[2]=255;
	color2[3]=255;

	for(std::vector<TrackType*>::iterator tti=trackTypes.begin();tti!=trackTypes.end();++tti){
		if(!(*tti)->tracks.empty()){
			CVertexArray* va=GetVertexArray();
			va->Initialize();
			glBindTexture(GL_TEXTURE_2D,(*tti)->texture);

			for(set<UnitTrackStruct*>::iterator ti=(*tti)->tracks.begin();ti!=(*tti)->tracks.end();){
				UnitTrackStruct* track=*ti;
				while(!track->parts.empty() && track->parts.front().creationTime<gs->frameNum-track->lifeTime){
					track->parts.pop_front();
				}
				if(track->parts.empty()){
					if(track->owner)
						track->owner->myTrack=0;
					delete track;
					(*tti)->tracks.erase(ti++);
					continue;
				}
				if(camera->InView((track->parts.front().pos1+track->parts.back().pos1)*0.5,track->parts.front().pos1.distance(track->parts.back().pos1)+500)){
					list<TrackPart>::iterator ppi=track->parts.begin();
					color2[3]=track->trackAlpha-(gs->frameNum-ppi->creationTime)*(int)track->alphaFalloff;

					for(list<TrackPart>::iterator pi=++track->parts.begin();pi!=track->parts.end();++pi){
						color[3]=track->trackAlpha-(gs->frameNum-ppi->creationTime)*(int)track->alphaFalloff;
						if(pi->connected){
							va->AddVertexTC(ppi->pos1,ppi->texPos,0,color2);
							va->AddVertexTC(ppi->pos2,ppi->texPos,1,color2);
							va->AddVertexTC(pi->pos2,pi->texPos,1,color);
							va->AddVertexTC(pi->pos1,pi->texPos,0,color);
						}
						color2[3]=color[3];
						ppi=pi;
					}
				}
				++ti;
			}
			va->DrawArrayTC(GL_QUADS);
		}
	}

	glBindTexture(GL_TEXTURE_2D,scarTex);
	CVertexArray* va=GetVertexArray();
	va->Initialize();
	glPolygonOffset(-5,-200);

	for(std::list<Scar*>::iterator si=scars.begin();si!=scars.end();){
		Scar* scar=*si;
		if(scar->lifeTime<gs->frameNum){
			RemoveScar(*si,false);
			si=scars.erase(si);
			continue;
		}
		if(camera->InView(scar->pos,scar->radius+16)){
			float3 pos=scar->pos;
			float radius=scar->radius;
			float tx=scar->texOffsetX;
			float ty=scar->texOffsetY;

			if(scar->creationTime+10>gs->frameNum)
				color[3]=(int)(scar->startAlpha*(gs->frameNum-scar->creationTime)*0.1);
			else
				color[3]=(int)(scar->startAlpha-(gs->frameNum-scar->creationTime)*scar->alphaFalloff);

			int sx=(int)max(0.f,(pos.x-radius)/16.0f);
			int ex=(int)min(float(gs->hmapx-1),(pos.x+radius)/16.0f);
			int sz=(int)max(0.f,(pos.z-radius)/16.0f);
			int ez=(int)min(float(gs->hmapy-1),(pos.z+radius)/16.0f);
			for(int x=sx;x<=ex;++x){
				for(int z=sz;z<=ez;++z){
					float px1=x*16;
					float pz1=z*16;
					float px2=px1+16;
					float pz2=pz1+16;

					float tx1=min(0.5f,(pos.x-px1)/(radius*4.0f)+0.25f);
					float tx2=max(0.0f,(pos.x-px2)/(radius*4.0f)+0.25f);
					float tz1=min(0.5f,(pos.z-pz1)/(radius*4.0f)+0.25f);
					float tz2=max(0.0f,(pos.z-pz2)/(radius*4.0f)+0.25f);

					va->AddVertexTC(float3(px1,readmap->heightmap[(z*2)*(gs->mapx+1)+x*2]+0.5,pz1),tx1+tx,tz1+ty,color);
					va->AddVertexTC(float3(px2,readmap->heightmap[(z*2)*(gs->mapx+1)+x*2+2]+0.5,pz1),tx2+tx,tz1+ty,color);
					va->AddVertexTC(float3(px2,readmap->heightmap[(z*2+2)*(gs->mapx+1)+x*2+2]+0.5,pz2),tx2+tx,tz2+ty,color);
					va->AddVertexTC(float3(px1,readmap->heightmap[(z*2+2)*(gs->mapx+1)+x*2]+0.5,pz2),tx1+tx,tz2+ty,color);
				}
			}
		}
		++si;
	}
	va->DrawArrayTC(GL_QUADS);

	glDisable(GL_POLYGON_OFFSET_FILL);
//	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);
	glDisable(GL_BLEND);
}

void CGroundDecalHandler::Update(void)
{
}

void CGroundDecalHandler::UnitMoved(CUnit* unit)
{
	if(decalLevel==0)
		return;

	int zp=(int(unit->pos.z)/SQUARE_SIZE*2);
	int xp=(int(unit->pos.x)/SQUARE_SIZE*2);
	int mp=zp*gs->hmapx+xp;
	if(mp<0)
		mp=0;
	if(mp>=gs->mapSquares/4)
		mp=mp>=gs->mapSquares/4;
	if(!readmap->terrainTypes[readmap->typemap[mp]].receiveTracks)
		return;

	TrackType* type=trackTypes[unit->unitDef->trackType];
	if(!unit->myTrack){
		UnitTrackStruct* ts=new UnitTrackStruct;
		ts->owner=unit;
		ts->lifeTime=(int)(30*decalLevel*unit->unitDef->trackStrength);
		ts->trackAlpha=(int)(unit->unitDef->trackStrength*25);
		ts->alphaFalloff=float(ts->trackAlpha)/float(ts->lifeTime);
		type->tracks.insert(ts);
		unit->myTrack=ts;
	}
	float3 pos=unit->pos+unit->frontdir*unit->unitDef->trackOffset;

	TrackPart tp;
	tp.pos1=pos+unit->rightdir*unit->unitDef->trackWidth*0.5;
	tp.pos1.y=ground->GetHeight2(tp.pos1.x,tp.pos1.z);
	tp.pos2=pos-unit->rightdir*unit->unitDef->trackWidth*0.5;
	tp.pos2.y=ground->GetHeight2(tp.pos2.x,tp.pos2.z);
	tp.creationTime=gs->frameNum;

	if(unit->myTrack->parts.empty()){
		tp.texPos=0;
		tp.connected=false;
	} else {
		tp.texPos=unit->myTrack->parts.back().texPos+tp.pos1.distance(unit->myTrack->parts.back().pos1)/unit->unitDef->trackWidth*unit->unitDef->trackStretch;
		tp.connected=unit->myTrack->parts.back().creationTime==gs->frameNum-8;
	}

	//if the unit is moving in a straight line only place marks at half the rate by replacing old ones
	if(unit->myTrack->parts.size()>1){
		list<TrackPart>::iterator pi=unit->myTrack->parts.end();
		--pi;
		list<TrackPart>::iterator pi2=pi;
		--pi;
		if(((tp.pos1+pi->pos1)*0.5).distance(pi2->pos1)<1){
			unit->myTrack->parts.back()=tp;
			return;
		}
	}

	unit->myTrack->parts.push_back(tp);
}

void CGroundDecalHandler::RemoveUnit(CUnit* unit)
{
	if(decalLevel==0)
		return;

	if(unit->myTrack){
		unit->myTrack->owner=0;
		unit->myTrack=0;
	}
}

int CGroundDecalHandler::GetTrackType(std::string name)
{
	if(decalLevel==0)
		return 0;

	std::transform(name.begin(), name.end(), name.begin(), (int (*)(int))std::tolower);	

	int a=0;
	for(std::vector<TrackType*>::iterator ti=trackTypes.begin();ti!=trackTypes.end();++ti){
		if((*ti)->name==name){
			return a;
		}
		++a;
	}
	TrackType* tt=new TrackType;
	tt->name=name;
	tt->texture=LoadTexture(name);
	trackTypes.push_back(tt);
	return trackTypes.size()-1;
}

unsigned int CGroundDecalHandler::LoadTexture(std::string name)
{
	if(name.find_first_of('.')==string::npos)
		name+=".bmp";
	if(name.find_first_of('\\')==string::npos&&name.find_first_of('/')==string::npos)
		name=string("bitmaps/tracks/")+name;

	CBitmap bm(name);
	for(int y=0;y<bm.ysize;++y){
		for(int x=0;x<bm.xsize;++x){
			bm.mem[(y*bm.xsize+x)*4+3]=bm.mem[(y*bm.xsize+x)*4+1];
			int brighness=bm.mem[(y*bm.xsize+x)*4+0];
			bm.mem[(y*bm.xsize+x)*4+0]=(brighness*90)/255;
			bm.mem[(y*bm.xsize+x)*4+1]=(brighness*60)/255;
			bm.mem[(y*bm.xsize+x)*4+2]=(brighness*30)/255;
		}
	}
	
	return bm.CreateTexture(true);
}

void CGroundDecalHandler::AddExplosion(float3 pos, float damage, float radius)
{
	if(decalLevel==0)
		return;

	float height=pos.y-ground->GetHeight2(pos.x,pos.z);
	if(height>=radius)
		return;

	pos.y-=height;
	radius-=height;

	if(radius<5)
		return;

	if(damage>radius*30)
		damage=radius*30;

	damage*=(radius)/(radius+height);
	if(radius>damage*0.25)
		radius=damage*0.25;

	if(damage>400)
		damage=400+sqrt(damage-399);

	pos.CheckInBounds();

	Scar* s=new Scar;
	s->pos=pos;
	s->radius=radius*1.4;
	s->creationTime=gs->frameNum;
	s->startAlpha=max(50.f,min(255.f,damage));
	float lifeTime=decalLevel*(damage)*3;
	s->alphaFalloff=s->startAlpha/(lifeTime);
	s->lifeTime=(int)(gs->frameNum+lifeTime);
	s->texOffsetX=(gu->usRandInt()&128)?0:0.5f;
	s->texOffsetY=(gu->usRandInt()&128)?0:0.5f;

	s->x1=(int)max(0.f,(pos.x-radius)/16.0f);
	s->x2=(int)min(float(gs->hmapx-1),(pos.x+radius)/16.0f+1);
	s->y1=(int)max(0.f,(pos.z-radius)/16.0f);
	s->y2=(int)min(float(gs->hmapy-1),(pos.z+radius)/16.0f+1);

	s->basesize=(s->x2-s->x1)*(s->y2-s->y1);
	s->overdrawn=0;
	s->lastTest=0;

	TestOverlaps(s);

	int x1=s->x1/16;
	int x2=min(scarFieldX-1,s->x2/16);
	int y1=s->y1/16;
	int y2=min(scarFieldY-1,s->y2/16);

	for(int y=y1;y<=y2;++y){
		for(int x=x1;x<=x2;++x){
			std::set<Scar*>* quad=&scarField[y*scarFieldX+x];
			quad->insert(s);
		}
	}

	scars.push_back(s);
}

void CGroundDecalHandler::LoadScar(std::string file, unsigned char* buf, int xoffset, int yoffset)
{
	CBitmap bm(file);
	for(int y=0;y<bm.ysize;++y){
		for(int x=0;x<bm.xsize;++x){
			buf[((y+yoffset)*512+x+xoffset)*4+3]=bm.mem[(y*bm.xsize+x)*4+1];
			int brighness=bm.mem[(y*bm.xsize+x)*4+0];
			buf[((y+yoffset)*512+x+xoffset)*4+0]=(brighness*90)/255;
			buf[((y+yoffset)*512+x+xoffset)*4+1]=(brighness*60)/255;
			buf[((y+yoffset)*512+x+xoffset)*4+2]=(brighness*30)/255;
		}
	}
}

int CGroundDecalHandler::OverlapSize(Scar* s1, Scar* s2)
{
	if(s1->x1>=s2->x2 || s1->x2<=s2->x1)
		return 0;
	if(s1->y1>=s2->y2 || s1->y2<=s2->y1)
		return 0;

	int xs;
	if(s1->x1<s2->x1)
		xs=s1->x2-s2->x1;
	else
		xs=s2->x2-s1->x1;

	int ys;
	if(s1->y1<s2->y1)
		ys=s1->y2-s2->y1;
	else
		ys=s2->y2-s1->y1;

	return xs*ys;
}

void CGroundDecalHandler::TestOverlaps(Scar* scar)
{
	int x1=scar->x1/16;
	int x2=min(scarFieldX-1,scar->x2/16);
	int y1=scar->y1/16;
	int y2=min(scarFieldY-1,scar->y2/16);

	++lastTest;

	for(int y=y1;y<=y2;++y){
		for(int x=x1;x<=x2;++x){
			std::set<Scar*>* quad=&scarField[y*scarFieldX+x];
			bool redoScan=false;
			do {
				redoScan=false;
				for(std::set<Scar*>::iterator si=quad->begin();si!=quad->end();++si){
					if(lastTest!=(*si)->lastTest && scar->lifeTime>=(*si)->lifeTime){
						Scar* tested=*si;
						tested->lastTest=lastTest;
						int overlap=OverlapSize(scar,tested);
						if(overlap>0 && tested->basesize>0){
							float part=overlap/tested->basesize;
							tested->overdrawn+=part;
							if(tested->overdrawn>maxOverlap){
								RemoveScar(tested,true);
								redoScan=true;
								break;
							}
						}
					}
				}
			} while(redoScan);
		}
	}
}

void CGroundDecalHandler::RemoveScar(Scar* scar,bool removeFromScars)
{
	int x1=scar->x1/16;
	int x2=min(scarFieldX-1,scar->x2/16);
	int y1=scar->y1/16;
	int y2=min(scarFieldY-1,scar->y2/16);

	for(int y=y1;y<=y2;++y){
		for(int x=x1;x<=x2;++x){
			std::set<Scar*>* quad=&scarField[y*scarFieldX+x];
			quad->erase(scar);
		}
	}
	if(removeFromScars)
		scars.remove(scar);
	delete scar;
}
