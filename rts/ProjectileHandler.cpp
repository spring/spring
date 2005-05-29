#include "StdAfx.h"
// ProjectileHandler.cpp: implementation of the CProjectileHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "ProjectileHandler.h"
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include <windows.h>		// Header File For Windows
#include "myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include "Projectile.h"
#include "Camera.h"
#include "VertexArray.h"
#include "QuadField.h"
#include "Unit.h"
#include "TimeProfiler.h"
#include "Bitmap.h"
#include "GroundFlash.h"
#include "LosHandler.h"
#include "Ground.h"
#include "TextureHandler.h"
#include "Feature.h"
#include "RegHandler.h"
#include "ShadowHandler.h"
#include "UnitHandler.h"
#include "3DOParser.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProjectileHandler* ph;
using namespace std;
extern GLfloat FogBlack[]; 
extern GLfloat FogLand[]; 

CProjectileHandler::CProjectileHandler()
{
	PrintLoadMsg("Creating projectile texture");

	maxParticles=regHandler.GetInt("MaxParticles",4000);

	currentParticles=0;
	particleSaturation=0;

	unsigned char tex[512][512][4];
	for(int y=0;y<512;y++)
		for(int x=0;x<512;x++){
			tex[y][x][0]=0;
			tex[y][x][1]=0;
			tex[y][x][2]=0;
			tex[y][x][3]=0;
		}

	for(int y=0;y<64;y++){	//circular thingy
		for(int x=0;x<64;x++){
			float dist=sqrt((float)(x-32)*(x-32)+(y-32)*(y-32));
			if(dist>31.875)
				dist=31.875;
			tex[y][x][0]=255-(unsigned char) (dist*8);
			tex[y][x][1]=255-(unsigned char) (dist*8);
			tex[y][x][2]=255-(unsigned char) (dist*8);
			tex[y][x][3]=255-(unsigned char) (dist*8);
		}
	}

	for(int y=0;y<64;y++){	//linear falloff for lasers
		for(int x=0;x<64;x++){
			float dist=abs(y-32);
			if(dist>31.5)
				dist=31.5;
			tex[y][x+320][0]=255-(unsigned char) (dist*8);
			tex[y][x+320][1]=255-(unsigned char) (dist*8);
			tex[y][x+320][2]=255-(unsigned char) (dist*8);
			tex[y][x+320][3]=255;
		}
	}
	for(int y=0;y<64;y++){	//laser endings
		for(int x=0;x<64;x++){
			float dist=sqrt((float)(x-32)*(x-32)+(y-32)*(y-32));
			if(dist>31.875)
				dist=31.875;
			tex[y][x+448][0]=255-(unsigned char) (dist*8);
			tex[y][x+448][1]=255-(unsigned char) (dist*8);
			tex[y][x+448][2]=255-(unsigned char) (dist*8);
			if(tex[y][x+448][0]!=0)
				tex[y][x+448][3]=255;
		}
	}

	LoadSmoke(tex,64,0,"bitmaps\\smoke\\smoke0000.bmp","bitmaps\\smoke\\smoke_alpha0000.bmp");
	LoadSmoke(tex,96,0,"bitmaps\\smoke\\smoke0001.bmp","bitmaps\\smoke\\smoke_alpha0001.bmp");
	LoadSmoke(tex,128,0,"bitmaps\\smoke\\smoke0002.bmp","bitmaps\\smoke\\smoke_alpha0002.bmp");
	LoadSmoke(tex,160,0,"bitmaps\\smoke\\smoke0003.bmp","bitmaps\\smoke\\smoke_alpha0003.bmp");
	LoadSmoke(tex,192,0,"bitmaps\\smoke\\smoke0004.bmp","bitmaps\\smoke\\smoke_alpha0004.bmp");
	LoadSmoke(tex,224,0,"bitmaps\\smoke\\smoke0005.bmp","bitmaps\\smoke\\smoke_alpha0005.bmp");

	LoadSmoke(tex,64,32,"bitmaps\\smoke\\smoke0006.bmp","bitmaps\\smoke\\smoke_alpha0006.bmp");
	LoadSmoke(tex,96,32,"bitmaps\\smoke\\smoke0007.bmp","bitmaps\\smoke\\smoke_alpha0007.bmp");
	LoadSmoke(tex,128,32,"bitmaps\\smoke\\smoke0008.bmp","bitmaps\\smoke\\smoke_alpha0008.bmp");
	LoadSmoke(tex,160,32,"bitmaps\\smoke\\smoke0009.bmp","bitmaps\\smoke\\smoke_alpha0009.bmp");
	LoadSmoke(tex,192,32,"bitmaps\\smoke\\smoke0010.bmp","bitmaps\\smoke\\smoke_alpha0010.bmp");
	LoadSmoke(tex,224,32,"bitmaps\\smoke\\smoke0011.bmp","bitmaps\\smoke\\smoke_alpha0011.bmp");

	for(int y=0;y<64;y++){		//fix smoke
		for(int x=64;x<256;x++){
			int a=(255-tex[y][x][3])/3;
			tex[y][x][0]-=a;
			tex[y][x][1]-=a;
			tex[y][x][2]-=a;
		}
	}

	for(int y=64;y<96;y++){	//smoke trail
		for(int x=0;x<256;x++){
			tex[y][x][0]=128;
			tex[y][x][1]=128;
			tex[y][x][2]=128;
			tex[y][x][3]=0;
		}
	}

	for(int a=0;a<16;++a){//smoke trail
		int xnum=(rand()%6)*32;
		int	ynum=(rand()&1)*32;
		for(int by=0;by<32;by++){
			int y=by+64;
			for(int bx=0;bx<32;bx++){
				int x=bx+a*8;
				if(x>127)
					x-=128;
				if(tex[by+ynum][bx+xnum+64][3]==0)
					continue;
				int totalAlpha=tex[y][x+16][3]+tex[by+ynum][bx+xnum+64][3];
				float alpha=(tex[by+ynum][bx+xnum+64][3]/255.0)/(totalAlpha/255.0);
				tex[y][x+16][0]=(unsigned char) (tex[y][x+16][0]*(1-alpha)+tex[by+ynum][bx+xnum+64][0]*alpha);
				tex[y][x+16][1]=(unsigned char) (tex[y][x+16][1]*(1-alpha)+tex[by+ynum][bx+xnum+64][1]*alpha);
				tex[y][x+16][2]=(unsigned char) (tex[y][x+16][2]*(1-alpha)+tex[by+ynum][bx+xnum+64][2]*alpha);
				tex[y][x+16][3]=min(255,totalAlpha);
			}
		}
	}
	for(int y=0;y<32;y++){//smoke trail
		for(int x=0;x<16;x++){
			for(int c=0;c<4;++c){
				tex[y+64][x][c]=tex[y+64][112+x][c];
				tex[y+64][144+x][c]=tex[y+64][16+x][c];			
			}
		}
	}
	for(int y=0;y<32;y++){//smoke trail
		float dist=1-fabs(float(y-16))/16.0;
		float amod=sqrt(dist);
		for(int x=0;x<160;x++){
			tex[y+64][x][3]=(unsigned char) (tex[y+64][x][3]*amod);
		}
	}
	for(int y2=0;y2<2;++y2){		//make alpha fall of a bit radially for the smoke
		int yoffs=y2*32;
		for(int x2=0;x2<6;++x2){
			int xoffs=64+x2*32;
			for(int y=0;y<32;y++){
				for(int x=0;x<32;x++){
					float xd=(x-16)/16.0;
					float yd=(y-16)/16.0;
					float dist=xd*xd+yd*yd;
					tex[yoffs+y][xoffs+x][3]*=(unsigned char)max(0.0,(1-dist*0.7));
				}
			}
		}
	}
	ConvertTex(tex,64,0,256,64,1);
	ConvertTex(tex,0,64,256,128,1);

	CBitmap explo("bitmaps\\explo.bmp");
	for(int y=0;y<128;y++){
		for(int x=0;x<128;x++){
			tex[y+128][x+128][0]=explo.mem[(y*128+x)*4];
			tex[y+128][x+128][1]=explo.mem[(y*128+x)*4+1];
			tex[y+128][x+128][2]=explo.mem[(y*128+x)*4+2];
			if(explo.mem[(y*128+x)*4]!=0)
				tex[y+128][x+128][3]=255;
		}
	}
	for(int y=0;y<128;y++){	//explo med fadeande alpha
		for(int x=0;x<128;x++){
			tex[y+128][x][0]=explo.mem[(y*128+x)*4];
			tex[y+128][x][1]=explo.mem[(y*128+x)*4+1];
			tex[y+128][x][2]=explo.mem[(y*128+x)*4+2];
			tex[y+128][x][3]=(explo.mem[(y*128+x)*4]+explo.mem[(y*128+x)*4+1]+explo.mem[(y*128+x)*4+2])/3;
		}
	}

	CBitmap flare("bitmaps\\flare.bmp");
	for(int y=0;y<128;y++){
		for(int x=0;x<256;x++){
			tex[y+64][x+256][0]=flare.mem[(y*256+x)*4+0];
			tex[y+64][x+256][1]=flare.mem[(y*256+x)*4+1];
			tex[y+64][x+256][2]=flare.mem[(y*256+x)*4+2];
			tex[y+64][x+256][3]=255;
			if(!tex[y+64][x+256][0] && !tex[y+64][x+256][1] && !tex[y+64][x+256][2])
				tex[y+64][x+256][3]=0;
		}
	}

	float dotAlpha[256][256];
	for(int y=0;y<256;y++){//random dots
		for(int x=0;x<256;x++){
			tex[y+256][x][0]=205+(unsigned char) (gs->randFloat()*50);
			tex[y+256][x][1]=205+(unsigned char) (gs->randFloat()*50);
			tex[y+256][x][2]=205+(unsigned char) (gs->randFloat()*50);
			tex[y+256][x][3]=0;
			dotAlpha[y][x]=gs->randFloat()*30;
		}
	}
	for(int a=0;a<3;++a){//random dots
		float mag=(60<<a);
		float size=(2<<a);
		for(int y=(int)size;y<255-(int)size;y+=(int)size){
			for(int x=(int)size;x<255-(int)size;x+=(int)size){
				float p=gs->randFloat()*mag;
				for(int y2=y-(int)size;y2<=y+(int)size;y2++){
					float ym=float(size-abs(y2-y))/size;
					for(int x2=x-(int)size;x2<=x+(int)size;x2++){
						float xm=float(size-abs(x2-x))/size;
						dotAlpha[y2][x2]+=p*xm*ym;
					}
				}
			}
		}
	}

	for(int a=0;a<20;++a){//random dots
		int bx=(int) (gs->randFloat()*228);
		int by=(int) (gs->randFloat()*228)+256;
		for(int y=0;y<16;y++){
			for(int x=0;x<16;x++){
				float dist=sqrt((float)(x-8)*(x-8)+(y-8)*(y-8));
				if(dist>8)
					continue;
				int alpha=60-(int)(dist*35+dotAlpha[by-256+y][bx+x]);
				if(tex[by+y][bx+x][3]<alpha){
					tex[by+y][bx+x][3]=max(0,min(255,alpha));
				}
			}
		}
	}
	ConvertTex(tex,0,256,256,512,1);

	for(int y=256;y<256+128;++y){//wake
		for(int x=256;x<256+128;++x){
			tex[y][x][0]=220;
			tex[y][x][1]=230;
			tex[y][x][2]=255;
		}
	}
	for(int a=0;a<40;++a){//wake
		float3 r(0,0,0);
		do{
			r.x=(gs->randFloat()-0.5)*2;
			r.y=(gs->randFloat()-0.5)*2;
		} while(r.Length()>1);
		int bx=(int)(r.x*52)+256+64-12;
		int by=(int)(r.y*52)+256+64-12;
		for(int y=0;y<24;y++){
			for(int x=0;x<24;x++){
				float dist=sqrt((float)(x-12)*(x-12)+(y-12)*(y-12));
				if(dist>12)
					continue;
				float alpha=255-dist*20;
				float alpha2=tex[by+y][bx+x][3];
				alpha=1-((1-alpha/255)*(1-alpha2/255));
				tex[by+y][bx+x][3]=(unsigned char) max((float)0,min((float)255,alpha*255));
			}
		}
	}
	ConvertTex(tex,256,256,256+128,256+128,1);

	glGenTextures(1, CProjectile::textures);
	glBindTexture(GL_TEXTURE_2D, CProjectile::textures[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);

//	CBitmap b(tex[0][0],512,512);
//	b.Save("proj.bmp");

	unsigned char tex2[512*256];

	for(int y=0;y<256;y++){
		for(int x=0;x<256;x++){
			float dist=sqrt((float)(x-127)*(x-127)+(y-127)*(y-127))/128.0;
			float alpha;
			if(dist<0.5){
				alpha=0.0;
			}else {
				alpha=1-4.0*fabs(dist-0.75);
				if(alpha<0)
					alpha=0;
			}
			tex2[y*256+x]=(unsigned char)(alpha*255);
			alpha=1-dist;
			if(alpha<0)
				alpha=0;
			tex2[(y+256)*256+x]=(unsigned char)(alpha*255);
		}
	}

	glGenTextures(1,&CGroundFlash::texture);
	glBindTexture(GL_TEXTURE_2D, CGroundFlash::texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 1,256, 512, GL_LUMINANCE, GL_UNSIGNED_BYTE, tex2);


	distlistsize=100;
	distlist=new projdist[100];

	if(shadowHandler->drawShadows){
		projectileShadowVP=LoadVertexProgram("projectileshadow.vp");
	}

}

CProjectileHandler::~CProjectileHandler()
{
	glDeleteTextures (1, CProjectile::textures);
	glDeleteTextures (1, &CGroundFlash::texture);
	Projectile_List::iterator psi;
	for(psi=ps.begin();psi!=ps.end();++psi)
		delete *psi;
	std::set<CGroundFlash*>::iterator gfi;
	for(gfi=groundFlashes.begin();gfi!=groundFlashes.end();++gfi)
		delete *gfi;
	delete[] distlist;

	if(shadowHandler->drawShadows){
		glDeleteProgramsARB( 1, &projectileShadowVP );
	}
	for(std::list<FlyingPiece*>::iterator pi=flyingPieces.begin();pi!=flyingPieces.end();++pi){
		delete *pi;
	}
	ph=0;
}

void CProjectileHandler::Update()
{
START_TIME_PROFILE
	while(!toBeDeletedFlashes.empty()){
		CGroundFlash* del=toBeDeletedFlashes.top();
		toBeDeletedFlashes.pop();
		std::set<CGroundFlash*>::iterator gfi;
		if((gfi=groundFlashes.find(del))!=groundFlashes.end()){
			delete del;
			groundFlashes.erase(gfi);
		}
	}	

	Projectile_List::iterator psi=ps.begin();
	while(psi!= ps.end()){
		if((*psi)->deleteMe){
			Projectile_List::iterator prev=psi++;
			delete *prev;
			ps.erase(prev);
		} else {
			(*psi)->Update();
			++psi;
		}
	}
	std::set<CGroundFlash*>::iterator gfi;
	for(gfi=groundFlashes.begin();gfi!=groundFlashes.end();++gfi){
		(*gfi)->Update();
	}

	for(std::list<FlyingPiece*>::iterator pi=flyingPieces.begin();pi!=flyingPieces.end();){
		(*pi)->pos+=(*pi)->speed;
		(*pi)->speed*=0.996;
		(*pi)->speed.y+=gs->gravity;
		(*pi)->rot+=(*pi)->rotSpeed;
		if((*pi)->pos.y<ground->GetApproximateHeight((*pi)->pos.x,(*pi)->pos.z)-10){
			delete *pi;
			pi=flyingPieces.erase(pi);
		} else {
			++pi;
		}
	}
END_TIME_PROFILE("Projectile handler");
}

int CompareProjDist( const void *arg1, const void *arg2 ){
	if(((CProjectileHandler::projdist*)arg1)->dist > ((CProjectileHandler::projdist*)arg2)->dist)
	   return -1;
   return 1;
}

void CProjectileHandler::Draw(bool drawReflection)
{
	while(distlistsize<(int)ps.size()){
		distlistsize*=2;
		delete[] distlist;
		distlist=new projdist[distlistsize];
	}

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(1);
	uh->SetupForUnitDrawing();

	CVertexArray* va=GetVertexArray();
	va->Initialize();
	for(std::list<FlyingPiece*>::iterator pi=flyingPieces.begin();pi!=flyingPieces.end();++pi){
		CMatrix44f m;
		m.Rotate((*pi)->rot,(*pi)->rotAxis);
		float3 interPos=(*pi)->pos+(*pi)->speed*gu->timeOffset;
		CTextureHandler::UnitTexture* tex=(*pi)->prim->texture;

		SVertex* v=&(*pi)->object->vertices[(*pi)->prim->vertices[0]];
		float3 tp=m.Mul(v->pos);
		float3 tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xstart,tex->ystart,tn);

		v=&(*pi)->object->vertices[(*pi)->prim->vertices[1]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xend,tex->ystart,tn);

		v=&(*pi)->object->vertices[(*pi)->prim->vertices[2]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xend,tex->yend,tn);

		v=&(*pi)->object->vertices[(*pi)->prim->vertices[3]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xstart,tex->yend,tn);
	}
	int drawnPieces=va->drawIndex/32;
	va->DrawArrayTN(GL_QUADS);

	Projectile_List::iterator psi;
	int a=0;
	for(psi=ps.begin();psi != ps.end();++psi){
		if(cam2->InView((*psi)->pos,(*psi)->drawRadius) && (loshandler->InLos(*psi,gu->myAllyTeam) || gu->spectating || ((*psi)->owner && gs->allies[(*psi)->owner->allyteam][gu->myAllyTeam]))){
			if(drawReflection){
				if((*psi)->pos.y<-3)
					continue;
				float dif=(*psi)->pos.y-camera->pos.y;
				float3 zeroPos=camera->pos*((*psi)->pos.y/dif) + (*psi)->pos*(-camera->pos.y/dif);
				if(ground->GetApproximateHeight(zeroPos.x,zeroPos.z)>3)
					continue;
			}
			if((*psi)->isUnitPart)
				(*psi)->DrawUnitPart();
			distlist[a].proj=*psi;
			distlist[a].dist=(*psi)->pos.dot(camera->forward);
			a++;
		}
	}
	uh->CleanUpUnitDrawing();

	unsigned int sortSize=a;
	qsort(distlist, sortSize,8,CompareProjDist);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, CProjectile::textures[0]);
	glEnable(GL_BLEND);
	glDepthMask(0);
	glColor4f(1,1,1,0.2f);
	glAlphaFunc(GL_GREATER,0.0);
	glEnable(GL_ALPHA_TEST);
//	glFogfv(GL_FOG_COLOR,FogLand);
	glDisable(GL_FOG);

	currentParticles=0;
	CProjectile::inArray=false;
	CProjectile::va=GetVertexArray();
	CProjectile::va->Initialize();
	for(int a=0;a<sortSize;a++){
		distlist[a].proj->Draw();
	}
	if(CProjectile::inArray)
		CProjectile::DrawArray();
	glDisable(GL_TEXTURE_2D);
	glDepthMask(1);
	currentParticles=(int)(ps.size()*0.8+currentParticles*0.2);
	currentParticles+=(int)(0.2*drawnPieces+0.3*flyingPieces.size());
	particleSaturation=(float)currentParticles/(float)maxParticles;
//	glFogfv(GL_FOG_COLOR,FogLand);
}

void CProjectileHandler::DrawShadowPass(void)
{
	while(distlistsize<(int)ps.size()){
		distlistsize*=2;
		delete[] distlist;
		distlist=new projdist[distlistsize];
	}
	Projectile_List::iterator psi;
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, projectileShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glDisable(GL_TEXTURE_2D);
	int a=0;
	for(psi=ps.begin();psi != ps.end();++psi){
		if((loshandler->InLos(*psi,gu->myAllyTeam) || gu->spectating || ((*psi)->owner && gs->allies[(*psi)->owner->allyteam][gu->myAllyTeam]))){
			if((*psi)->isUnitPart)
				(*psi)->DrawUnitPart();
			if((*psi)->castShadow){
				distlist[a].proj=*psi;
				a++;
			}
		}
	}
	int sortSize=a;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, CProjectile::textures[0]);
	glColor4f(1,1,1,1);
	glAlphaFunc(GL_GREATER,0.3);
	glEnable(GL_ALPHA_TEST);
	glShadeModel(GL_SMOOTH);

	CProjectile::inArray=false;
	CProjectile::va=GetVertexArray();
	CProjectile::va->Initialize();
	for(int a=0;a<sortSize;a++){
		distlist[a].proj->Draw();
	}
	if(CProjectile::inArray)
		CProjectile::DrawArray();

	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable( GL_VERTEX_PROGRAM_ARB );
}

void CProjectileHandler::AddProjectile(CProjectile* p)
{
	ps.push_back(p);
//	toBeAddedProjectile.push(p);
}

void CProjectileHandler::LoadSmoke(unsigned char tex[512][512][4],int xoffs,int yoffs,char* filename,char* alphafile)
{
	CBitmap TextureImage(filename);
	CBitmap AlphaImage(alphafile);
	for(int y=0;y<32;y++){
		for(int x=0;x<32;x++){
			tex[yoffs+y][xoffs+x][0]=TextureImage.mem[(y*32+x)*4];
			tex[yoffs+y][xoffs+x][1]=TextureImage.mem[(y*32+x)*4+1];
			tex[yoffs+y][xoffs+x][2]=TextureImage.mem[(y*32+x)*4+2];
			tex[yoffs+y][xoffs+x][3]=AlphaImage.mem[(y*32+x)*4];
		}
	}
}

void CProjectileHandler::CheckUnitCol()
{
	Projectile_List::iterator psi;
	for(psi=ps.begin();psi != ps.end();++psi){
		CProjectile* p=(*psi);
		if(p->checkCol && !p->deleteMe){
			CUnit* owner=p->owner;
			float speedf=p->speed.Length();
			float3 normSpeed=p->speed/speedf;

			vector<CUnit*> units=qf->GetUnitsExact(p->pos,p->radius+speedf);
			for(vector<CUnit*>::iterator ui(units.begin());ui!=units.end();++ui){
				if(owner ==(*ui))
					continue;
				float totalRadius=(*ui)->radius+p->radius;
				float3 dif=(*ui)->midPos-p->pos;
				float closeTime=dif.dot(normSpeed)/speedf;
				if(closeTime<0)
					closeTime=0;
				if(closeTime>1)
					closeTime=1;
				float3 closeVect=dif-(p->speed*closeTime);
				if(dif.SqLength() < totalRadius*totalRadius){
					p->Collision(*ui);
					break;
				}
			}

			vector<CFeature*> features=qf->GetFeaturesExact(p->pos,p->radius+speedf);
			for(vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
				if(!(*fi)->blocking)
					continue;
				float totalRadius=(*fi)->radius+p->radius;
				float3 dif=(*fi)->midPos-p->pos;
				float closeTime=dif.dot(normSpeed)/speedf;
				if(closeTime<0)
					closeTime=0;
				if(closeTime>1)
					closeTime=1;
				float3 closeVect=dif-(p->speed*closeTime);
				if(dif.SqLength() < totalRadius*totalRadius){
					p->Collision();
					break;
				}
			}
		}
	}	
}

void CProjectileHandler::AddGroundFlash(CGroundFlash* flash)
{
	groundFlashes.insert(flash);
}

void CProjectileHandler::RemoveGroundFlash(CGroundFlash* flash)
{
	toBeDeletedFlashes.push(flash);
}

void CProjectileHandler::DrawGroundFlashes(void)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, CGroundFlash::texture);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(0);
	glFogfv(GL_FOG_COLOR,FogBlack);

	CGroundFlash::va=GetVertexArray();
	CGroundFlash::va->Initialize();

	std::set<CGroundFlash*>::iterator gfi;
	for(gfi=groundFlashes.begin();gfi!=groundFlashes.end();++gfi){
		(*gfi)->Draw();
	}
	CGroundFlash::va->DrawArrayTC(GL_QUADS);

	glFogfv(GL_FOG_COLOR,FogLand);
	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}

void CProjectileHandler::ConvertTex(unsigned char tex[512][512][4], int startx, int starty, int endx, int endy, float absorb)
{
	for(int y=starty;y<endy;++y){
		for(int x=startx;x<endx;++x){
			float alpha=tex[y][x][3];
			unsigned char mul=(unsigned char) (alpha/255.0);
			tex[y][x][0]*=mul;
			tex[y][x][1]*=mul;
			tex[y][x][2]*=mul;
		}
	}
}


void CProjectileHandler::AddFlyingPiece(float3 pos,float3 speed,S3DO* object,SPrimitive* piece)
{
	FlyingPiece* fp=new FlyingPiece;
	fp->pos=pos;
	fp->speed=speed;
	fp->prim=piece;
	fp->object=object;

	fp->rotAxis=gu->usRandVector();
	fp->rotAxis.Normalize();
	fp->rotSpeed=gu->usRandFloat()*0.1;
	fp->rot=0;

	flyingPieces.push_back(fp);
}
