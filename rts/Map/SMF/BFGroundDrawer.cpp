#include "StdAfx.h"
#include "BFGroundDrawer.h"
#include "BFGroundTextures.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "LogOutput.h"
#include "SmfReadMap.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Platform/ConfigHandler.h"
#include "mmgr.h"

using namespace std;

CBFGroundDrawer::CBFGroundDrawer(CSmfReadMap *rm)
{
	map = rm;

	numBigTexX=gs->mapx/128;
	numBigTexY=gs->mapy/128;

	heightData=map->heightmap;
	heightDataX=gs->mapx+1;

	if(shadowHandler->canUseShadows){
		groundVP=LoadVertexProgram("ground.vp");
		groundShadowVP=LoadVertexProgram("groundshadow.vp");
		if(shadowHandler->useFPShadows){
			groundFPShadow=LoadFragmentProgram("groundFPshadow.fp");
		}
	}

	textures=SAFE_NEW CBFGroundTextures(map);

	viewRadius=configHandler.GetInt("GroundDetail",40);
	viewRadius+=viewRadius%2;
}

CBFGroundDrawer::~CBFGroundDrawer(void)
{
	delete textures;

	if(shadowHandler->canUseShadows){
		glSafeDeleteProgram( groundVP );
		glSafeDeleteProgram( groundShadowVP );
		if(shadowHandler->useFPShadows){
			glSafeDeleteProgram( groundFPShadow);
		}
	}

	configHandler.SetInt("GroundDetail",viewRadius);
}

static bool drawWater=false;
static float bigtexsubx,bigtexsuby;
static float invMapSizeX,invMapSizeY;

#define NUM_LODS 4

inline void CBFGroundDrawer::DrawVertexA(int x,int y)
{
	float height=heightData[y*heightDataX+x];
	if(drawWater && height<0){
		height*=2;
	}

	va->AddVertex0(float3(x*SQUARE_SIZE,height,y*SQUARE_SIZE));
}

inline void CBFGroundDrawer::DrawVertexA(int x,int y,float height)
{
	if(drawWater && height<0){
		height*=2;
	}
	va->AddVertex0(float3(x*SQUARE_SIZE,height,y*SQUARE_SIZE));
}

inline void CBFGroundDrawer::EndStrip()
{
	va->EndStrip();	
}

void CBFGroundDrawer::DrawGroundVertexArray()
{
	va->DrawArray0(GL_TRIANGLE_STRIP);

	va=GetVertexArray();
	va->Initialize();
}

void CBFGroundDrawer::Draw(bool drawWaterReflection,bool drawUnitReflection,unsigned int overrideVP)
{
	drawWater=drawWaterReflection;

	int baseViewRadius=max(4,viewRadius);
	if(drawUnitReflection)
		viewRadius=(viewRadius/2)&0xfffffe;
	float zoom=45/camera->fov;
	viewRadius=(int)(viewRadius*sqrt(zoom));
	viewRadius+=viewRadius%2;

	va=GetVertexArray();
	va->Initialize();

	textures->DrawUpdate();

	int x,y;
	int mapx=gs->mapx+1;
	int hmapx=mapx>>1;
	int mapy=gs->mapy+1;

	int neededLod=int(gu->viewRange/8/viewRadius*2);
	UpdateCamRestraints();

	invMapSizeX=1.0f/gs->mapx;
	invMapSizeY=1.0f/gs->mapy;

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	if(!overrideVP)
		glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetupTextureUnits(drawWaterReflection,overrideVP);
	bool inStrip=false;

	if(map->voidWater && !drawWater){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER,0.9f);
	}

  float camxpart=0,oldcamxpart;
	float camypart=0,oldcamypart;

	for(int bty=0;bty<numBigTexY;++bty){			//loop over the big texture squares (128 squares)
		bigtexsuby=bty;

		//only process the necessary big squares in the x direction
		int sx=0;
		int ex=numBigTexX;
		float xtest,xtest2;
		const int bigSquareSize=128;
		std::vector<fline>::iterator fli;
		for(fli=left.begin();fli!=left.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(bty*bigSquareSize)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((bty*bigSquareSize)+bigSquareSize)));
			if(xtest>xtest2)
				xtest=xtest2;
			xtest=xtest/bigSquareSize;
			if(xtest>sx)
				sx=(int) xtest;
		}
		for(fli=right.begin();fli!=right.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(bty*bigSquareSize)))+bigSquareSize;
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((bty*bigSquareSize)+bigSquareSize)))+bigSquareSize;
			if(xtest<xtest2)
				xtest=xtest2;
			xtest=xtest/bigSquareSize;
			if(xtest<ex)
				ex=(int) xtest;
		}
//		logOutput.Print("%i %i",sx,ex);
		for(int btx=sx;btx<ex;++btx){
			bigtexsubx=btx;

			if(DrawExtraTex() || !shadowHandler->drawShadows){
				textures->SetTexture(btx,bty);
				SetTexGen(1.0f/1024,1.0f/1024,-btx,-bty);
				if(overrideVP){
 					glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, -btx,-bty,0,0);
 				}
			} else {
				textures->SetTexture(btx,bty);
				glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, -btx,-bty,0,0);
			}
			for(int lod=1;lod<neededLod;lod*=2){
				int cx=(int)(cam2->pos.x/(SQUARE_SIZE));
				int cy=(int)(cam2->pos.z/(SQUARE_SIZE));

				cx=(cx/lod)*lod;
				cy=(cy/lod)*lod;
				int hlod=lod>>1;
				int ysquaremod=((cy)%(2*lod))/lod;
				int xsquaremod=((cx)%(2*lod))/lod;

				oldcamxpart=camxpart;
				float cx2=(cx/(2*lod))*lod*2;
				camxpart=(cam2->pos.x/(SQUARE_SIZE)-cx2)/(lod*2);

				oldcamypart=camypart;
				float cy2=(cy/(2*lod))*lod*2;
				camypart=(cam2->pos.z/(SQUARE_SIZE)-cy2)/(lod*2);

				int minty=bty*128;
				int maxty=(bty+1)*128;
				int mintx=btx*128;
				int maxtx=(btx+1)*128;

				int minly=cy+(-viewRadius+3-ysquaremod)*lod;
				int maxly=cy+(viewRadius-1-ysquaremod)*lod;
				int minlx=cx+(-viewRadius+3-xsquaremod)*lod;
				int maxlx=cx+(viewRadius-1-xsquaremod)*lod;

				int xstart=max(minlx,mintx);
				int xend=min(maxlx,maxtx);
				int ystart=max(minly,minty);
				int yend=min(maxly,maxty);

				for(y=ystart;y<yend;y+=lod){
					int xs=xstart;
					int xe=xend;
					int xtest,xtest2;
					std::vector<fline>::iterator fli;
					for(fli=left.begin();fli!=left.end();fli++){
						xtest=((int)(fli->base/(SQUARE_SIZE)+fli->dir*y))/lod*lod-lod;
						xtest2=((int)(fli->base/(SQUARE_SIZE)+fli->dir*(y+lod)))/lod*lod-lod;
						if(xtest>xtest2)
							xtest=xtest2;
						if(xtest>xs)
							xs=xtest;
					}
					for(fli=right.begin();fli!=right.end();fli++){
						xtest=((int)(fli->base/(SQUARE_SIZE)+fli->dir*y))/lod*lod+lod;
						xtest2=((int)(fli->base/(SQUARE_SIZE)+fli->dir*(y+lod)))/lod*lod+lod;
						if(xtest<xtest2)
							xtest=xtest2;
						if(xtest<xe)
							xe=xtest;
					}

					for(x=xs;x<xe;x+=lod){
						if((lod==1) || 
							(x>(cx)+viewRadius*hlod) || (x<(cx)-viewRadius*hlod) ||
							(y>(cy)+viewRadius*hlod) || (y<(cy)-viewRadius*hlod)){  //normal terr�g
								if(!inStrip){
									DrawVertexA(x,y);
									DrawVertexA(x,y+lod);
									inStrip=true;
								}
								DrawVertexA(x+lod,y);
								DrawVertexA(x+lod,y+lod);
							} else {  //inre begr�sning mot f�eg�nde lod
								if((x>=(cx)+viewRadius*hlod)){
									float h1=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x]*(oldcamxpart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(oldcamxpart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamxpart);
									float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(oldcamxpart);

									if(inStrip){
										EndStrip();
										inStrip=false;
									}
									DrawVertexA(x,y);                                            
									DrawVertexA(x,y+hlod,h1);
									DrawVertexA(x+hlod,y,h2);
									DrawVertexA(x+hlod,y+hlod,h3);
									EndStrip();
									DrawVertexA(x,y+hlod,h1);
									DrawVertexA(x,y+lod);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x+hlod,y+lod,h4);
									EndStrip();
									DrawVertexA(x+hlod,y+lod,h4);
									DrawVertexA(x+lod,y+lod);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x+lod,y);
									DrawVertexA(x+hlod,y,h2);
									EndStrip();
								}     
								if((x<=(cx)-viewRadius*hlod)){
									float h1=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamxpart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(1-oldcamxpart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamxpart);
									float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamxpart);

									if(inStrip){
										EndStrip();
										inStrip=false;
									}
									DrawVertexA(x+lod,y+hlod,h1);
									DrawVertexA(x+lod,y);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x+hlod,y,h2);
									EndStrip();
									DrawVertexA(x+lod,y+lod);
									DrawVertexA(x+lod,y+hlod,h1);
									DrawVertexA(x+hlod,y+lod,h4);
									DrawVertexA(x+hlod,y+hlod,h3);
									EndStrip();
									DrawVertexA(x+hlod,y,h2);
									DrawVertexA(x,y);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x,y+lod);
									DrawVertexA(x+hlod,y+lod,h4);
									EndStrip();        
								} 
								if((y>=(cy)+viewRadius*hlod)){
									float h1=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y)*heightDataX+x+hlod]*(oldcamypart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(oldcamypart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamypart);
									float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(oldcamypart);

									if(inStrip){
										EndStrip();
										inStrip=false;
									}
									DrawVertexA(x,y);                                            
									DrawVertexA(x,y+hlod,h2);
									DrawVertexA(x+hlod,y,h1);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x+lod,y);
									DrawVertexA(x+lod,y+hlod,h4);
									EndStrip();
									DrawVertexA(x,y+hlod,h2);
									DrawVertexA(x,y+lod);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x+lod,y+lod);
									DrawVertexA(x+lod,y+hlod,h4);
									EndStrip();        
								}
								if((y<=(cy)-viewRadius*hlod)){
									float h1=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamypart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(1-oldcamypart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamypart);
									float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamypart);

									if(inStrip){
										EndStrip();
										inStrip=false;
									}
									DrawVertexA(x,y+hlod,h2);
									DrawVertexA(x,y+lod);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x+hlod,y+lod,h1);
									DrawVertexA(x+lod,y+hlod,h4);
									DrawVertexA(x+lod,y+lod);
									EndStrip();
									DrawVertexA(x+lod,y+hlod,h4);
									DrawVertexA(x+lod,y);
									DrawVertexA(x+hlod,y+hlod,h3);
									DrawVertexA(x,y);
									DrawVertexA(x,y+hlod,h2);
									EndStrip();        
								}
							}
					}
					if(inStrip){
						EndStrip();
						inStrip=false;
					}
				}
				//rita yttre begr�snings yta mot n�ta lod
				if(maxlx<maxtx && maxlx>=mintx){
					x=maxlx;
					for(y=max(ystart-lod,minty);y<min(yend+lod,maxty);y+=lod){
						DrawVertexA(x,y);
						DrawVertexA(x,y+lod);
						if(y%(lod*2)){
							float h=((heightData[(y-lod)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f)*(1-camxpart)+heightData[(y)*heightDataX+x+lod]*(camxpart);
							DrawVertexA(x+lod,y,h);
							DrawVertexA(x+lod,y+lod);
						} else {
							DrawVertexA(x+lod,y);
							float h=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod*2)*heightDataX+x+lod])*0.5f*(1-camxpart)+heightData[(y+lod)*heightDataX+x+lod]*(camxpart);
							DrawVertexA(x+lod,y+lod,h);
						}
						EndStrip();
					}
				}
				if(minlx>mintx && minlx<maxtx){
					x=minlx-lod;
					for(y=max(ystart-lod,minty);y<min(yend+lod,maxty);y+=lod){
						if(y%(lod*2)){
							float h=((heightData[(y-lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f)*(camxpart)+heightData[(y)*heightDataX+x]*(1-camxpart);
							DrawVertexA(x,y,h);
							DrawVertexA(x,y+lod);
						} else {
							DrawVertexA(x,y);
							float h=(heightData[(y)*heightDataX+x]+heightData[(y+lod*2)*heightDataX+x])*0.5f*(camxpart)+heightData[(y+lod)*heightDataX+x]*(1-camxpart);
							DrawVertexA(x,y+lod,h);
						}
						DrawVertexA(x+lod,y);
						DrawVertexA(x+lod,y+lod);
						EndStrip();
					}
				}
				if(maxly<maxty && maxly>minty){
					y=maxly;
					int xs=max(xstart-lod,mintx);
					int xe=min(xend+lod,maxtx);
					int xtest,xtest2;
					std::vector<fline>::iterator fli;
					for(fli=left.begin();fli!=left.end();fli++){
						xtest=((int)(fli->base/(SQUARE_SIZE)+fli->dir*y))/lod*lod-lod;
						xtest2=((int)(fli->base/(SQUARE_SIZE)+fli->dir*(y+lod)))/lod*lod-lod;
						if(xtest>xtest2)
							xtest=xtest2;
						if(xtest>xs)
							xs=xtest;
					}
					for(fli=right.begin();fli!=right.end();fli++){
						xtest=((int)(fli->base/(SQUARE_SIZE)+fli->dir*y))/lod*lod+lod;
						xtest2=((int)(fli->base/(SQUARE_SIZE)+fli->dir*(y+lod)))/lod*lod+lod;
						if(xtest<xtest2)
							xtest=xtest2;
						if(xtest<xe)
							xe=xtest;
					}
					if(xs<xe){
						x=xs;
						if(x%(lod*2)){
							DrawVertexA(x,y);
							float h=((heightData[(y+lod)*heightDataX+x-lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f)*(1-camypart)+heightData[(y+lod)*heightDataX+x]*(camypart);
							DrawVertexA(x,y+lod,h);
						} else {
							DrawVertexA(x,y);
							DrawVertexA(x,y+lod);
						}
						for(x=xs;x<xe;x+=lod){
							if(x%(lod*2)){
								DrawVertexA(x+lod,y);
								DrawVertexA(x+lod,y+lod);
							} else {
								DrawVertexA(x+lod,y);
								float h=(heightData[(y+lod)*heightDataX+x+2*lod]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-camypart)+heightData[(y+lod)*heightDataX+x+lod]*(camypart);
								DrawVertexA(x+lod,y+lod,h);
							}
						}
						EndStrip();
					}
				}
				if(minly>minty && minly<maxty){
					y=minly-lod;
					int xs=max(xstart-lod,mintx);
					int xe=min(xend+lod,maxtx);
					int xtest,xtest2;
					std::vector<fline>::iterator fli;
					for(fli=left.begin();fli!=left.end();fli++){
						xtest=((int)(fli->base/(SQUARE_SIZE)+fli->dir*y))/lod*lod-lod;
						xtest2=((int)(fli->base/(SQUARE_SIZE)+fli->dir*(y+lod)))/lod*lod-lod;
						if(xtest>xtest2)
							xtest=xtest2;
						if(xtest>xs)
							xs=xtest;
					}
					for(fli=right.begin();fli!=right.end();fli++){
						xtest=((int)(fli->base/(SQUARE_SIZE)+fli->dir*y))/lod*lod+lod;
						xtest2=((int)(fli->base/(SQUARE_SIZE)+fli->dir*(y+lod)))/lod*lod+lod;
						if(xtest<xtest2)
							xtest=xtest2;
						if(xtest<xe)
							xe=xtest;
					}
					if(xs<xe){
						x=xs;
						if(x%(lod*2)){
							float h=((heightData[(y)*heightDataX+x-lod]+heightData[(y)*heightDataX+x+lod])*0.5f)*(camypart)+heightData[(y)*heightDataX+x]*(1-camypart);
							DrawVertexA(x,y,h);
							DrawVertexA(x,y+lod);
						} else {
							DrawVertexA(x,y);
							DrawVertexA(x,y+lod);
						}
						for(x=xs;x<xe;x+=lod){
							if(x%(lod*2)){
								DrawVertexA(x+lod,y);
								DrawVertexA(x+lod,y+lod);
							} else {
								float h=(heightData[(y)*heightDataX+x+2*lod]+heightData[(y)*heightDataX+x])*0.5f*(camypart)+heightData[(y)*heightDataX+x+lod]*(1-camypart);
								DrawVertexA(x+lod,y,h);
								DrawVertexA(x+lod,y+lod);
							}
						}
						EndStrip();
					}
				}
			}
			DrawGroundVertexArray();
		}
	}
	ResetTextureUnits(drawWaterReflection,overrideVP);
	glDisable(GL_CULL_FACE);

	if(map->voidWater && !drawWater){
		glDisable(GL_ALPHA_TEST);
	}

	if(map->hasWaterPlane)
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(map->waterPlaneColor.x, map->waterPlaneColor.y, map->waterPlaneColor.z);
		glBegin(GL_QUADS);//water color edge of map <0
		float xsize=gs->mapx*SQUARE_SIZE;
		float ysize=gs->mapy*SQUARE_SIZE;
		if(!drawWaterReflection){
			float xsize=gs->mapx*SQUARE_SIZE/4;
			float ysize=gs->mapy*SQUARE_SIZE/4;
			for(y=-4;y<8;++y)
				for(int x=-4;x<8;++x)
					if(x>3 || x<0 || y>3 || y<0 || camera->pos.x<0 || camera->pos.z<0 || camera->pos.x>float3::maxxpos || camera->pos.z>float3::maxzpos){
							glVertex3f(x*xsize,-200,y*ysize);
							glVertex3f((x+1)*xsize,-200,y*ysize);
							glVertex3f((x+1)*xsize,-200,(y+1)*ysize);
							glVertex3f(x*xsize,-200,(y+1)*ysize);
					}
		}
		glEnd();
	}

	if(groundDecals && !(drawWaterReflection || drawUnitReflection))
		groundDecals->Draw();

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

//	sky->SetCloudShadow(1);
//	if(drawWaterReflection)
//		treeDistance*=0.5f;

	ph->DrawGroundFlashes();
	if(treeDrawer->drawTrees){
		if(DrawExtraTex()){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glBindTexture(GL_TEXTURE_2D, infoTex);
			SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		  glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		treeDrawer->Draw(drawWaterReflection || drawUnitReflection);
		if(DrawExtraTex()){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_2D);
		  glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
	
	glDisable(GL_ALPHA_TEST);

	glDisable(GL_TEXTURE_2D);
	viewRadius=baseViewRadius;
}

void CBFGroundDrawer::DrawShadowPass(void)
{
	va=GetVertexArray();
	va->Initialize();

//	glEnable(GL_CULL_FACE);
	bool inStrip=false;

	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	int x,y;
  float camxpart=0,oldcamxpart;
	float camypart=0,oldcamypart;

	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, groundShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );

	for(int lod=1;lod<(2<<NUM_LODS);lod*=2){
		int cx=(int)(camera->pos.x/(SQUARE_SIZE));
		int cy=(int)(camera->pos.z/(SQUARE_SIZE));

		cx=(cx/lod)*lod;
		cy=(cy/lod)*lod;
		int hlod=lod>>1;
		int ysquaremod=((cy)%(2*lod))/lod;
		int xsquaremod=((cx)%(2*lod))/lod;

		oldcamxpart=camxpart;
		float cx2=(cx/(2*lod))*lod*2;
		camxpart=(camera->pos.x/(SQUARE_SIZE)-cx2)/(lod*2);

		oldcamypart=camypart;
		float cy2=(cy/(2*lod))*lod*2;
		camypart=(camera->pos.z/(SQUARE_SIZE)-cy2)/(lod*2);

		int minty=0;
		int maxty=gs->mapy;
		int mintx=0;
		int maxtx=gs->mapx;

		int minly=cy+(-viewRadius+3-ysquaremod)*lod;
		int maxly=cy+(viewRadius-1-ysquaremod)*lod;
		int minlx=cx+(-viewRadius+3-xsquaremod)*lod;
		int maxlx=cx+(viewRadius-1-xsquaremod)*lod;

		int xstart=max(minlx,mintx);
		int xend=min(maxlx,maxtx);
		int ystart=max(minly,minty);
		int yend=min(maxly,maxty);

		for(y=ystart;y<yend;y+=lod){
			int xs=xstart;
			int xe=xend;
				for(x=xs;x<xe;x+=lod){
				if((lod==1) || 
					(x>(cx)+viewRadius*hlod) || (x<(cx)-viewRadius*hlod) ||
					(y>(cy)+viewRadius*hlod) || (y<(cy)-viewRadius*hlod)){  //normal terr�g
						if(!inStrip){
							DrawVertexA(x,y);
							DrawVertexA(x,y+lod);
							inStrip=true;
						}
						DrawVertexA(x+lod,y);
						DrawVertexA(x+lod,y+lod);
					} else {  //inre begr�sning mot f�eg�nde lod
						if((x>=(cx)+viewRadius*hlod)){
							float h1=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x]*(oldcamxpart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(oldcamxpart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamxpart);
							float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(oldcamxpart);

							if(inStrip){
								EndStrip();
								inStrip=false;
							}
							DrawVertexA(x,y);                                            
							DrawVertexA(x,y+hlod,h1);
							DrawVertexA(x+hlod,y,h2);
							DrawVertexA(x+hlod,y+hlod,h3);
							EndStrip();
							DrawVertexA(x,y+hlod,h1);
							DrawVertexA(x,y+lod);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x+hlod,y+lod,h4);
							EndStrip();
							DrawVertexA(x+hlod,y+lod,h4);
							DrawVertexA(x+lod,y+lod);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x+lod,y);
							DrawVertexA(x+hlod,y,h2);
							EndStrip();
						}     
						if((x<=(cx)-viewRadius*hlod)){
							float h1=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamxpart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(1-oldcamxpart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamxpart);
							float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamxpart);

							if(inStrip){
								EndStrip();
								inStrip=false;
							}
							DrawVertexA(x+lod,y+hlod,h1);
							DrawVertexA(x+lod,y);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x+hlod,y,h2);
							EndStrip();
							DrawVertexA(x+lod,y+lod);
							DrawVertexA(x+lod,y+hlod,h1);
							DrawVertexA(x+hlod,y+lod,h4);
							DrawVertexA(x+hlod,y+hlod,h3);
							EndStrip();
							DrawVertexA(x+hlod,y,h2);
							DrawVertexA(x,y);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x,y+lod);
							DrawVertexA(x+hlod,y+lod,h4);
							EndStrip();        
						} 
						if((y>=(cy)+viewRadius*hlod)){
							float h1=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y)*heightDataX+x+hlod]*(oldcamypart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(oldcamypart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamypart);
							float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(oldcamypart);

							if(inStrip){
								EndStrip();
								inStrip=false;
							}
							DrawVertexA(x,y);                                            
							DrawVertexA(x,y+hlod,h2);
							DrawVertexA(x+hlod,y,h1);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x+lod,y);
							DrawVertexA(x+lod,y+hlod,h4);
							EndStrip();
							DrawVertexA(x,y+hlod,h2);
							DrawVertexA(x,y+lod);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x+lod,y+lod);
							DrawVertexA(x+lod,y+hlod,h4);
							EndStrip();        
						}
						if((y<=(cy)-viewRadius*hlod)){
							float h1=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamypart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(1-oldcamypart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamypart);
							float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamypart);

							if(inStrip){
								EndStrip();
								inStrip=false;
							}
							DrawVertexA(x,y+hlod,h2);
							DrawVertexA(x,y+lod);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x+hlod,y+lod,h1);
							DrawVertexA(x+lod,y+hlod,h4);
							DrawVertexA(x+lod,y+lod);
							EndStrip();
							DrawVertexA(x+lod,y+hlod,h4);
							DrawVertexA(x+lod,y);
							DrawVertexA(x+hlod,y+hlod,h3);
							DrawVertexA(x,y);
							DrawVertexA(x,y+hlod,h2);
							EndStrip();        
						}
					}
			}
			if(inStrip){
				EndStrip();
				inStrip=false;
			}
		}
		//rita yttre begr�snings yta mot n�ta lod
		if(maxlx<maxtx && maxlx>=mintx){
			x=maxlx;
			for(y=max(ystart-lod,minty);y<min(yend+lod,maxty);y+=lod){
				DrawVertexA(x,y);
				DrawVertexA(x,y+lod);
				if(y%(lod*2)){
					float h=((heightData[(y-lod)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f)*(1-camxpart)+heightData[(y)*heightDataX+x+lod]*(camxpart);
					DrawVertexA(x+lod,y,h);
					DrawVertexA(x+lod,y+lod);
				} else {
					DrawVertexA(x+lod,y);
					float h=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod*2)*heightDataX+x+lod])*0.5f*(1-camxpart)+heightData[(y+lod)*heightDataX+x+lod]*(camxpart);
					DrawVertexA(x+lod,y+lod,h);
				}
				EndStrip();
			}
		}
		if(minlx>mintx && minlx<maxtx){
			x=minlx-lod;
			for(y=max(ystart-lod,minty);y<min(yend+lod,maxty);y+=lod){
				if(y%(lod*2)){
					float h=((heightData[(y-lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f)*(camxpart)+heightData[(y)*heightDataX+x]*(1-camxpart);
					DrawVertexA(x,y,h);
					DrawVertexA(x,y+lod);
				} else {
					DrawVertexA(x,y);
					float h=(heightData[(y)*heightDataX+x]+heightData[(y+lod*2)*heightDataX+x])*0.5f*(camxpart)+heightData[(y+lod)*heightDataX+x]*(1-camxpart);
					DrawVertexA(x,y+lod,h);
				}
				DrawVertexA(x+lod,y);
				DrawVertexA(x+lod,y+lod);
				EndStrip();
			}
		}
		if(maxly<maxty && maxly>minty){
			y=maxly;
			int xs=max(xstart-lod,mintx);
			int xe=min(xend+lod,maxtx);
			if(xs<xe){
				x=xs;
				if(x%(lod*2)){
					DrawVertexA(x,y);
					float h=((heightData[(y+lod)*heightDataX+x-lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f)*(1-camypart)+heightData[(y+lod)*heightDataX+x]*(camypart);
					DrawVertexA(x,y+lod,h);
				} else {
					DrawVertexA(x,y);
					DrawVertexA(x,y+lod);
				}
				for(x=xs;x<xe;x+=lod){
					if(x%(lod*2)){
						DrawVertexA(x+lod,y);
						DrawVertexA(x+lod,y+lod);
					} else {
						DrawVertexA(x+lod,y);
						float h=(heightData[(y+lod)*heightDataX+x+2*lod]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-camypart)+heightData[(y+lod)*heightDataX+x+lod]*(camypart);
						DrawVertexA(x+lod,y+lod,h);
					}
				}
				EndStrip();
			}
		}
		if(minly>minty && minly<maxty){
			y=minly-lod;
			int xs=max(xstart-lod,mintx);
			int xe=min(xend+lod,maxtx);
			if(xs<xe){
				x=xs;
				if(x%(lod*2)){
					float h=((heightData[(y)*heightDataX+x-lod]+heightData[(y)*heightDataX+x+lod])*0.5f)*(camypart)+heightData[(y)*heightDataX+x]*(1-camypart);
					DrawVertexA(x,y,h);
					DrawVertexA(x,y+lod);
				} else {
					DrawVertexA(x,y);
					DrawVertexA(x,y+lod);
				}
				for(x=xs;x<xe;x+=lod){
					if(x%(lod*2)){
						DrawVertexA(x+lod,y);
						DrawVertexA(x+lod,y+lod);
					} else {
						float h=(heightData[(y)*heightDataX+x+2*lod]+heightData[(y)*heightDataX+x])*0.5f*(camypart)+heightData[(y)*heightDataX+x+lod]*(1-camypart);
						DrawVertexA(x+lod,y,h);
						DrawVertexA(x+lod,y+lod);
					}
				}
				EndStrip();
			}
		}
		DrawGroundVertexArray();
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glDisable( GL_VERTEX_PROGRAM_ARB );
}

void CBFGroundDrawer::SetupTextureUnits(bool drawReflection,unsigned int overrideVP)
{
	glColor4f(1,1,1,1);
	if(DrawExtraTex()){
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture ());
		SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		if (map->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,  map->detailTex);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			SetTexGen(0.02f,0.02f,-floor(camera->pos.x*0.02f),-floor(camera->pos.z*0.02f));
		} else glDisable (GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, infoTex);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);

		if(overrideVP){
 			glEnable( GL_VERTEX_PROGRAM_ARB );
 			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, overrideVP );
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0f/1024,1.0f/1024,0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x*0.02f),-floor(camera->pos.z*0.02f),0,0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02f,0.02f,0,1);
 			if(drawReflection){
 				glAlphaFunc(GL_GREATER,0.9f);
 				glEnable(GL_ALPHA_TEST);
 			}
 		}
	} else if(shadowHandler->drawShadows){
		glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, groundFPShadow );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );
		float3 ac=map->ambientColor*(210.0f/255.0f);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, ac.x,ac.y,ac.z,1);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, 0,0,0,map->shadowDensity);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		glActiveTextureARB(GL_TEXTURE2_ARB);
		if (map->detailTex)
			glBindTexture(GL_TEXTURE_2D,  map->detailTex);
		else
			glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glActiveTextureARB(GL_TEXTURE4_ARB);
		glBindTexture(GL_TEXTURE_2D,  shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

		glActiveTextureARB(GL_TEXTURE0_ARB);

		if(drawReflection){
			glAlphaFunc(GL_GREATER,0.8f);
			glEnable(GL_ALPHA_TEST);
		}
		if(overrideVP)
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, overrideVP );
		else
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, groundVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0f/1024,1.0f/1024,0,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x*0.02f),-floor(camera->pos.z*0.02f),0,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02f,0.02f,0,1);

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_MODELVIEW);
	} else {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture ());
		SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		

		glActiveTextureARB(GL_TEXTURE2_ARB);
		if(map->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,  map->detailTex);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			SetTexGen(0.02f,0.02f,-floor(camera->pos.x*0.02f),-floor(camera->pos.z*0.02f));
		} else glDisable (GL_TEXTURE_2D);
		if(overrideVP){
 			glEnable( GL_VERTEX_PROGRAM_ARB );
 			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, overrideVP );
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0f/1024,1.0f/1024,0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x*0.02f),-floor(camera->pos.z*0.02f),0,0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02f,0.02f,0,1);
 			if(drawReflection){
 				glAlphaFunc(GL_GREATER,0.9f);
 				glEnable(GL_ALPHA_TEST);
 			}
 		}
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void CBFGroundDrawer::ResetTextureUnits(bool drawReflection,unsigned int overrideVP)
{
	if(DrawExtraTex() || !shadowHandler->drawShadows){
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		if(overrideVP){
 			glDisable( GL_VERTEX_PROGRAM_ARB );
 			if(drawReflection){
 				glDisable(GL_ALPHA_TEST);
 			}
 		}
	} else {
		glDisable( GL_VERTEX_PROGRAM_ARB );
		glDisable( GL_FRAGMENT_PROGRAM_ARB );

		glActiveTextureARB(GL_TEXTURE4_ARB);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		if(drawReflection){
			glDisable(GL_ALPHA_TEST);
		}
	}
}



void CBFGroundDrawer::AddFrustumRestraint(float3 side)
{
	fline temp;
	float3 up(0,1,0);
	
	float3 b=up.cross(side);		//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001f)
		b.z=0.0001f;
	{
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		float3 colpoint;				//a point on the collision line
		
		if(side.y>0)								
			colpoint=cam2->pos-c*((cam2->pos.y-(readmap->minheight-100))/c.y);
		else
			colpoint=cam2->pos-c*((cam2->pos.y-(readmap->maxheight+30))/c.y);
		
		
		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		if(b.z>0){
			left.push_back(temp);			
		}else{
			right.push_back(temp);
		}
	}	
}

void CBFGroundDrawer::UpdateCamRestraints(void)
{
	left.clear();
	right.clear();

	//Add restraints for camera sides
	AddFrustumRestraint(cam2->bottom);
	AddFrustumRestraint(cam2->top);
	AddFrustumRestraint(cam2->rightside);
	AddFrustumRestraint(cam2->leftside);

	//Add restraint for maximum view distance
	fline temp;
	float3 up(0,1,0);
	float3 side=cam2->forward;
	float3 camHorizontal=cam2->forward;
	camHorizontal.y=0;
	camHorizontal.Normalize();
	float3 b=up.cross(camHorizontal);			//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)>0.0001f){
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(camHorizontal);			//get vector from camera to collision line
		float3 colpoint;				//a point on the collision line
		
		if(side.y>0)								
			colpoint=cam2->pos+camHorizontal*gu->viewRange*1.05f-c*(cam2->pos.y/c.y);
		else
			colpoint=cam2->pos+camHorizontal*gu->viewRange*1.05f-c*((cam2->pos.y-255/3.5f)/c.y);
		
		
		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		if(b.z>0){
			left.push_back(temp);			
		}else{
			right.push_back(temp);
		}
	}

}


void CBFGroundDrawer::IncreaseDetail()
{
	viewRadius+=2;
	logOutput << "ViewRadius is now " << viewRadius << "\n";
}

void CBFGroundDrawer::DecreaseDetail()
{
	viewRadius-=2;
	logOutput << "ViewRadius is now " << viewRadius << "\n";
}
