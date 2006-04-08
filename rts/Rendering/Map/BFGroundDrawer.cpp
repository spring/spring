#include "StdAfx.h"
#include "BFGroundDrawer.h"
#include "BFGroundTextures.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Sim/Map/SmfReadMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "mmgr.h"

// MSVC compiler does not have std::min and max, but rather, it's own built in macro
#ifdef _MSC_VER
#define USE_MIN min
#define USE_MAX max
#else
#define USE_MIN std::min
#define USE_MAX std::max
#endif

CBFGroundDrawer::CBFGroundDrawer(void)
{
	numBigTexX=gs->mapx/128;
	numBigTexY=gs->mapy/128;

	heightData=readmap->heightmap;
	heightDataX=gs->mapx+1;

	infoTexMem=new unsigned char[gs->pwr2mapx*gs->pwr2mapy*4];
	for(int a=0;a<gs->pwr2mapx*gs->pwr2mapy*4;++a)
		infoTexMem[a]=255;

	drawMetalMap=false;
	drawPathMap=false;
	highResInfoTexWanted=false;
	extraTex=0;

	if(shadowHandler->drawShadows){
		groundVP=LoadVertexProgram("ground.vp");
		groundShadowVP=LoadVertexProgram("groundshadow.vp");
		if(shadowHandler->useFPShadows)
			groundFPShadow=LoadFragmentProgram("groundFPshadow.fp");
	}
}

CBFGroundDrawer::~CBFGroundDrawer(void)
{
	delete[] infoTexMem;
	if(infoTex!=0)
		glDeleteTextures(1,&infoTex);

	if(shadowHandler->drawShadows){
		glDeleteProgramsARB( 1, &groundVP );
		glDeleteProgramsARB( 1, &groundShadowVP );
		if(shadowHandler->useFPShadows)
			glDeleteProgramsARB( 1, &groundFPShadow);
	}
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

	int baseViewRadius=viewRadius;
	if(drawUnitReflection)
		viewRadius=(viewRadius/2)&0xfffffe;
	float zoom=45/camera->fov;
	viewRadius=(int)(viewRadius*sqrt(zoom));
	viewRadius+=viewRadius%2;

	va=GetVertexArray();
	va->Initialize();

	groundTextures->DrawUpdate();

	int x,y;
	int mapx=gs->mapx+1;
	int hmapx=mapx>>1;
	int mapy=gs->mapy+1;

	UpdateCamRestraints();

	invMapSizeX=1.0/gs->mapx;
	invMapSizeY=1.0/gs->mapy;

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	if(!overrideVP)
		glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetupTextureUnits(drawWaterReflection,overrideVP);
	bool inStrip=false;

	if(readmap->voidWater && !drawWater){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER,0.9);
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
//		info->AddLine("%i %i",sx,ex);
		for(int btx=sx;btx<ex;++btx){
			bigtexsubx=btx;

			if((drawExtraTex) || !shadowHandler->drawShadows){
				groundTextures->SetTexture(btx,bty);
				SetTexGen(1.0/1024,1.0/1024,-btx,-bty);
				if(overrideVP){
 					glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, -btx,-bty,0,0);
 				}
			} else {
				groundTextures->SetTexture(btx,bty);
				glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, -btx,-bty,0,0);
			}
			for(int lod=1;lod<(2<<NUM_LODS);lod*=2){
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

				int xstart=USE_MAX(minlx,mintx);
				int xend=USE_MIN(maxlx,maxtx);
				int ystart=USE_MAX(minly,minty);
				int yend=USE_MIN(maxly,maxty);

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
							(y>(cy)+viewRadius*hlod) || (y<(cy)-viewRadius*hlod)){  //normal terr�ng
								if(!inStrip){
									DrawVertexA(x,y);
									DrawVertexA(x,y+lod);
									inStrip=true;
								}
								DrawVertexA(x+lod,y);
								DrawVertexA(x+lod,y+lod);
							} else {  //inre begr�nsning mot f�reg�ende lod
								if((x>=(cx)+viewRadius*hlod)){
									float h1=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x]*(oldcamxpart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(oldcamxpart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamxpart);
									float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(1-oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(oldcamxpart);

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
									float h1=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamxpart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(1-oldcamxpart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamxpart);
									float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamxpart);

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
									float h1=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamypart)+heightData[(y)*heightDataX+x+hlod]*(oldcamypart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(oldcamypart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamypart);
									float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(oldcamypart);

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
									float h1=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(oldcamypart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamypart);
									float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5*(oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(1-oldcamypart);
									float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamypart);
									float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamypart);

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
				//rita yttre begr�nsnings yta mot n�sta lod
				if(maxlx<maxtx && maxlx>=mintx){
					x=maxlx;
					for(y=USE_MAX(ystart-lod,minty);y<USE_MIN(yend+lod,maxty);y+=lod){
						DrawVertexA(x,y);
						DrawVertexA(x,y+lod);
						if(y%(lod*2)){
							float h=((heightData[(y-lod)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5)*(1-camxpart)+heightData[(y)*heightDataX+x+lod]*(camxpart);
							DrawVertexA(x+lod,y,h);
							DrawVertexA(x+lod,y+lod);
						} else {
							DrawVertexA(x+lod,y);
							float h=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod*2)*heightDataX+x+lod])*0.5*(1-camxpart)+heightData[(y+lod)*heightDataX+x+lod]*(camxpart);
							DrawVertexA(x+lod,y+lod,h);
						}
						EndStrip();
					}
				}
				if(minlx>mintx && minlx<maxtx){
					x=minlx-lod;
					for(y=USE_MAX(ystart-lod,minty);y<USE_MIN(yend+lod,maxty);y+=lod){
						if(y%(lod*2)){
							float h=((heightData[(y-lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5)*(camxpart)+heightData[(y)*heightDataX+x]*(1-camxpart);
							DrawVertexA(x,y,h);
							DrawVertexA(x,y+lod);
						} else {
							DrawVertexA(x,y);
							float h=(heightData[(y)*heightDataX+x]+heightData[(y+lod*2)*heightDataX+x])*0.5*(camxpart)+heightData[(y+lod)*heightDataX+x]*(1-camxpart);
							DrawVertexA(x,y+lod,h);
						}
						DrawVertexA(x+lod,y);
						DrawVertexA(x+lod,y+lod);
						EndStrip();
					}
				}
				if(maxly<maxty && maxly>minty){
					y=maxly;
					int xs=USE_MAX(xstart-lod,mintx);
					int xe=USE_MIN(xend+lod,maxtx);
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
							float h=((heightData[(y+lod)*heightDataX+x-lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5)*(1-camypart)+heightData[(y+lod)*heightDataX+x]*(camypart);
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
								float h=(heightData[(y+lod)*heightDataX+x+2*lod]+heightData[(y+lod)*heightDataX+x])*0.5*(1-camypart)+heightData[(y+lod)*heightDataX+x+lod]*(camypart);
								DrawVertexA(x+lod,y+lod,h);
							}
						}
						EndStrip();
					}
				}
				if(minly>minty && minly<maxty){
					y=minly-lod;
					int xs=USE_MAX(xstart-lod,mintx);
					int xe=USE_MIN(xend+lod,maxtx);
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
							float h=((heightData[(y)*heightDataX+x-lod]+heightData[(y)*heightDataX+x+lod])*0.5)*(camypart)+heightData[(y)*heightDataX+x]*(1-camypart);
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
								float h=(heightData[(y)*heightDataX+x+2*lod]+heightData[(y)*heightDataX+x])*0.5*(camypart)+heightData[(y)*heightDataX+x+lod]*(1-camypart);
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

	if(readmap->voidWater && !drawWater){
		glDisable(GL_ALPHA_TEST);
	}

	if(((CSmfReadMap*)readmap)->hasWaterPlane)
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(((CSmfReadMap*)readmap)->waterPlaneColor.x, ((CSmfReadMap*)readmap)->waterPlaneColor.y, ((CSmfReadMap*)readmap)->waterPlaneColor.z);
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

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

//	sky->SetCloudShadow(1);
	float treeDistance=baseTreeDistance*sqrt((zoom));
//	if(drawWaterReflection)
//		treeDistance*=0.5;
	if(treeDistance>MAX_VIEW_RANGE/(SQUARE_SIZE*TREE_SQUARE_SIZE))
		treeDistance=MAX_VIEW_RANGE/(SQUARE_SIZE*TREE_SQUARE_SIZE);

	if(groundDecals && !(drawWaterReflection || drawUnitReflection))
		groundDecals->Draw();
	ph->DrawGroundFlashes();
	if(treeDrawer->drawTrees){
		if((drawExtraTex)){
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
			SetTexGen(1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		  glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		treeDrawer->Draw(treeDistance,drawWaterReflection || drawUnitReflection);
		if((drawExtraTex)){
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

		int xstart=USE_MAX(minlx,mintx);
		int xend=USE_MIN(maxlx,maxtx);
		int ystart=USE_MAX(minly,minty);
		int yend=USE_MIN(maxly,maxty);

		for(y=ystart;y<yend;y+=lod){
			int xs=xstart;
			int xe=xend;
				for(x=xs;x<xe;x+=lod){
				if((lod==1) || 
					(x>(cx)+viewRadius*hlod) || (x<(cx)-viewRadius*hlod) ||
					(y>(cy)+viewRadius*hlod) || (y<(cy)-viewRadius*hlod)){  //normal terr�ng
						if(!inStrip){
							DrawVertexA(x,y);
							DrawVertexA(x,y+lod);
							inStrip=true;
						}
						DrawVertexA(x+lod,y);
						DrawVertexA(x+lod,y+lod);
					} else {  //inre begr�nsning mot f�reg�ende lod
						if((x>=(cx)+viewRadius*hlod)){
							float h1=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x]*(oldcamxpart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(oldcamxpart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamxpart);
							float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(1-oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(oldcamxpart);

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
							float h1=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamxpart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(1-oldcamxpart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamxpart);
							float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamxpart);

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
							float h1=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamypart)+heightData[(y)*heightDataX+x+hlod]*(oldcamypart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(oldcamypart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamypart);
							float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(oldcamypart);

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
							float h1=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5*(oldcamypart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamypart);
							float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5*(oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(1-oldcamypart);
							float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamypart);
							float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamypart);

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
		//rita yttre begr�nsnings yta mot n�sta lod
		if(maxlx<maxtx && maxlx>=mintx){
			x=maxlx;
			for(y=USE_MAX(ystart-lod,minty);y<USE_MIN(yend+lod,maxty);y+=lod){
				DrawVertexA(x,y);
				DrawVertexA(x,y+lod);
				if(y%(lod*2)){
					float h=((heightData[(y-lod)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5)*(1-camxpart)+heightData[(y)*heightDataX+x+lod]*(camxpart);
					DrawVertexA(x+lod,y,h);
					DrawVertexA(x+lod,y+lod);
				} else {
					DrawVertexA(x+lod,y);
					float h=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod*2)*heightDataX+x+lod])*0.5*(1-camxpart)+heightData[(y+lod)*heightDataX+x+lod]*(camxpart);
					DrawVertexA(x+lod,y+lod,h);
				}
				EndStrip();
			}
		}
		if(minlx>mintx && minlx<maxtx){
			x=minlx-lod;
			for(y=USE_MAX(ystart-lod,minty);y<USE_MIN(yend+lod,maxty);y+=lod){
				if(y%(lod*2)){
					float h=((heightData[(y-lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5)*(camxpart)+heightData[(y)*heightDataX+x]*(1-camxpart);
					DrawVertexA(x,y,h);
					DrawVertexA(x,y+lod);
				} else {
					DrawVertexA(x,y);
					float h=(heightData[(y)*heightDataX+x]+heightData[(y+lod*2)*heightDataX+x])*0.5*(camxpart)+heightData[(y+lod)*heightDataX+x]*(1-camxpart);
					DrawVertexA(x,y+lod,h);
				}
				DrawVertexA(x+lod,y);
				DrawVertexA(x+lod,y+lod);
				EndStrip();
			}
		}
		if(maxly<maxty && maxly>minty){
			y=maxly;
			int xs=USE_MAX(xstart-lod,mintx);
			int xe=USE_MIN(xend+lod,maxtx);
			if(xs<xe){
				x=xs;
				if(x%(lod*2)){
					DrawVertexA(x,y);
					float h=((heightData[(y+lod)*heightDataX+x-lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5)*(1-camypart)+heightData[(y+lod)*heightDataX+x]*(camypart);
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
						float h=(heightData[(y+lod)*heightDataX+x+2*lod]+heightData[(y+lod)*heightDataX+x])*0.5*(1-camypart)+heightData[(y+lod)*heightDataX+x+lod]*(camypart);
						DrawVertexA(x+lod,y+lod,h);
					}
				}
				EndStrip();
			}
		}
		if(minly>minty && minly<maxty){
			y=minly-lod;
			int xs=USE_MAX(xstart-lod,mintx);
			int xe=USE_MIN(xend+lod,maxtx);
			if(xs<xe){
				x=xs;
				if(x%(lod*2)){
					float h=((heightData[(y)*heightDataX+x-lod]+heightData[(y)*heightDataX+x+lod])*0.5)*(camypart)+heightData[(y)*heightDataX+x]*(1-camypart);
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
						float h=(heightData[(y)*heightDataX+x+2*lod]+heightData[(y)*heightDataX+x])*0.5*(camypart)+heightData[(y)*heightDataX+x+lod]*(1-camypart);
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

//todo: this part of extra textures is a mess really ...
void CBFGroundDrawer::SetExtraTexture(unsigned char* tex,unsigned char* pal,bool highRes)
{
	drawMetalMap=false;
	drawPathMap=false;
	drawHeightMap=false;
	if(tex!=extraTex && tex!=0){
		extraTex=tex;
		extraTexPal=pal;
		drawExtraTex=true;
		highResInfoTexWanted=highRes;
	} else {
		drawExtraTex=false;
		extraTex=0;
		highResInfoTexWanted=false;
	}
	updateTextureState=0;
	while(!UpdateTextures());
}

void CBFGroundDrawer::SetHeightTexture()
{
	if(drawHeightMap){
		drawHeightMap=false;
		drawExtraTex=false;
	} else {
		drawHeightMap=true;
		drawMetalMap=false;
		drawPathMap=false;
		drawExtraTex=true;
		highResInfoTexWanted=true;
		extraTex=0;
	}
	updateTextureState=0;
	while(!UpdateTextures());
}

void CBFGroundDrawer::SetMetalTexture(unsigned char* tex,float* extractMap,unsigned char* pal,bool highRes)
{
	if(drawMetalMap){
		drawMetalMap=false;
		drawExtraTex=false;
	} else {
		drawHeightMap=false;
		drawMetalMap=true;
		drawPathMap=false;
		drawExtraTex=true;
		highResInfoTexWanted=false;
		extraTex=tex;
		extraTexPal=pal;
		extractDepthMap=extractMap;
	}
	updateTextureState=0;
	while(!UpdateTextures());
}

void CBFGroundDrawer::SetPathMapTexture()
{
	if(drawPathMap){
		drawPathMap=false;
		drawExtraTex=false;
	} else {
		drawPathMap=true;
		drawHeightMap=false;
		drawMetalMap=false;
		drawExtraTex=true;
		extraTex=0;
		highResInfoTexWanted=false;
	}
	updateTextureState=0;
	while(!UpdateTextures());
}

void CBFGroundDrawer::ToggleLosTexture()
{
	if(drawLos){
		drawLos=false;
		drawExtraTex=false;
	} else {
		drawLos=true;
		drawPathMap=false;
		drawHeightMap=false;
		drawMetalMap=false;
		drawExtraTex=true;
		extraTex=0;
		highResInfoTexWanted=false;
	}
	updateTextureState=0;
	while(!UpdateTextures());
}
	
bool CBFGroundDrawer::UpdateTextures()
{
	if(!drawExtraTex)
		return true;

	unsigned short* myLos=loshandler->losMap[gu->myAllyTeam];
	unsigned short* myAirLos=loshandler->airLosMap[gu->myAllyTeam];
	if(updateTextureState<50){
		int starty;
		int endy;
		if(highResInfoTexWanted){
			starty=updateTextureState*gs->mapy/50;
			endy=(updateTextureState+1)*gs->mapy/50;
		} else {
			starty=updateTextureState*gs->hmapy/50;
			endy=(updateTextureState+1)*gs->hmapy/50;
		}
		if(drawPathMap)
		{
			if(selectedUnits.selectedUnits.empty() || !(*selectedUnits.selectedUnits.begin())->unitDef->movedata)
				return true;

			MoveData *md=(*selectedUnits.selectedUnits.begin())->unitDef->movedata;

			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->hmapx;++x){
					int a=y*(gs->pwr2mapx>>1)+x;

					float m;
					//todo: fix for new gui
					if(guihandler->inCommand>0 && guihandler->inCommand<guihandler->commands.size() && guihandler->commands[guihandler->inCommand].type==CMDTYPE_ICON_BUILDING){
						if(!loshandler->InLos(float3(x*16+8,0,y*16+8),gu->myAllyTeam)){
							m=0.25;
						}else{
							UnitDef *unitdef = unitDefHandler->GetUnitByID(-guihandler->commands[guihandler->inCommand].id);

							CFeature* f;
							if(uh->TestUnitBuildSquare(float3(x*16+8,0,y*16+8),unitdef,f))
								m=1;
							else 
								m=0;
							if(f && m)
								m=0.5;
						}

					} else {
						m=md->moveMath->SpeedMod(*md, x*2,y*2);
						if(gs->cheatEnabled && md->moveMath->IsBlocked2(*md, x*2+1, y*2+1) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN))
							m=0;
						m=min(1.,(double)sqrt(m));
					}
					infoTexMem[a*4+0]=255-int(m*255.0f);
					infoTexMem[a*4+1]=int(m*255.0f);
					infoTexMem[a*4+2]=0;
				}
			}
		} else if(drawMetalMap){
			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->hmapx;++x){
					int a=y*(gs->pwr2mapx>>1)+x;
					if(myAirLos[(y/2)*gs->hmapx/2+x/2])
						infoTexMem[a*4]=(unsigned char)USE_MIN(255.,(double)sqrt(sqrt(extractDepthMap[y*gs->hmapx+x]))*900);
					else
						infoTexMem[a*4]=0;
					infoTexMem[a*4+1]=(extraTexPal[extraTex[y*gs->hmapx+x]*3+1]);
					infoTexMem[a*4+2]=(extraTexPal[extraTex[y*gs->hmapx+x]*3+2]);
				}
			}
		} else if(drawHeightMap){
			extraTexPal=readmap->heightLinePal;
			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->mapx;++x){
					int a=y*gs->pwr2mapx+x;
					float height=readmap->centerheightmap[y*gs->mapx+x];
					unsigned char value=(unsigned char)(height*8);
					infoTexMem[a*4]=64+(extraTexPal[value*3]>>1);
					infoTexMem[a*4+1]=64+(extraTexPal[value*3+1]>>1);
					infoTexMem[a*4+2]=64+(extraTexPal[value*3+2]>>1);
				}
			}
		} else if(drawLos){
			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->hmapx;++x){
					int a=y*(gs->pwr2mapx>>1)+x;
					int inRadar=0;
					if(radarhandler->radarMaps[gu->myAllyTeam][y/(RADAR_SIZE/2)*radarhandler->xsize+x/(RADAR_SIZE/2)])
						inRadar=50;
					int inJam=0;
					if(radarhandler->jammerMaps[gu->myAllyTeam][y/(RADAR_SIZE/2)*radarhandler->xsize+x/(RADAR_SIZE/2)])
						inJam=50;
					if(myLos[(y)*gs->hmapx+x]!=0){
						infoTexMem[a*4]=170+inJam;
						infoTexMem[a*4+1]=170+inRadar;
						infoTexMem[a*4+2]=170;
					} else if(myAirLos[(y/2)*gs->hmapx/2+x/2]!=0){
						infoTexMem[a*4]=120+inJam;
						infoTexMem[a*4+1]=120+inRadar;
						infoTexMem[a*4+2]=120;
					} else {
						infoTexMem[a*4]=90+inJam;
						infoTexMem[a*4+1]=90+inRadar;
						infoTexMem[a*4+2]=90;
					}
				}
			}
		} else if(drawExtraTex){
			if(highResInfoTexWanted){
				for(int y=starty;y<endy;++y){
					for(int x=0;x<gs->mapx;++x){
						int a=y*gs->pwr2mapx+x;
						int value=extraTex[y*gs->mapx+x];
						//value=readmap->groundBlockingObjectMap[y*gs->mapx+x] ? 0:128;
						infoTexMem[a*4]=64+(extraTexPal[value*3]>>1);
						infoTexMem[a*4+1]=64+(extraTexPal[value*3+1]>>1);
						infoTexMem[a*4+2]=64+(extraTexPal[value*3+2]>>1);
					}
				}
			} else {
				for(int y=starty;y<endy;++y){
					for(int x=0;x<gs->mapx;++x){
						int a=y*(gs->pwr2mapx>>1)+x;
						infoTexMem[a*4]=64+(extraTexPal[extraTex[y*gs->hmapx+x]*3]>>1);
						infoTexMem[a*4+1]=64+(extraTexPal[extraTex[y*gs->hmapx+x]*3+1]>>1);
						infoTexMem[a*4+2]=64+(extraTexPal[extraTex[y*gs->hmapx+x]*3+2]>>1);
					}
				}
			}
		}
	}

	if(updateTextureState==50){
		if(infoTex!=0 && highResInfoTexWanted!=highResInfoTex){
			glDeleteTextures(1,&infoTex);
			infoTex=0;
		}
		if(infoTex==0){
			glGenTextures(1,&infoTex);
			glBindTexture(GL_TEXTURE_2D, infoTex);
			//if(drawPathMap)
			//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			//else
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			if(highResInfoTexWanted)
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy,0,GL_RGBA, GL_UNSIGNED_BYTE, infoTexMem);
			else
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, gs->pwr2mapx>>1, gs->pwr2mapy>>1,0,GL_RGBA, GL_UNSIGNED_BYTE, infoTexMem);
			highResInfoTex=highResInfoTexWanted;
			updateTextureState=0;
			return true;
		}

	}
	if(updateTextureState>=50){
		glBindTexture(GL_TEXTURE_2D, infoTex);
		if(highResInfoTexWanted)
			glTexSubImage2D(GL_TEXTURE_2D,0, 0,(updateTextureState-50)*(gs->pwr2mapy/8),gs->pwr2mapx, (gs->pwr2mapy/8),GL_RGBA, GL_UNSIGNED_BYTE, &infoTexMem[(updateTextureState-50)*(gs->pwr2mapy/8)*gs->pwr2mapx*4]);
		else
			glTexSubImage2D(GL_TEXTURE_2D,0, 0,(updateTextureState-50)*(gs->pwr2mapy/16),gs->pwr2mapx>>1, (gs->pwr2mapy/16),GL_RGBA, GL_UNSIGNED_BYTE, &infoTexMem[(updateTextureState-50)*(gs->pwr2mapy/16)*(gs->pwr2mapx>>1)*4]);
		if(updateTextureState==57){
			updateTextureState=0;
			return true;
		}
	}
	updateTextureState++;
	return false;
}

void CBFGroundDrawer::SetupTextureUnits(bool drawReflection,unsigned int overrideVP)
{
	glColor4f(1,1,1,1);
	if((groundDrawer->drawExtraTex)){
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, ((CSmfReadMap*)readmap)->shadowTex);
		SetTexGen(1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		if (readmap->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,  readmap->detailTex);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			SetTexGen(0.02,0.02,-floor(camera->pos.x*0.02),-floor(camera->pos.z*0.02));
		} else glDisable (GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,  groundDrawer->infoTex);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		SetTexGen(1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,0);

		if(overrideVP){
 			glEnable( GL_VERTEX_PROGRAM_ARB );
 			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, overrideVP );
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0/1024,1.0/1024,0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x*0.02),-floor(camera->pos.z*0.02),0,0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02,0.02,0,1);
 			if(drawReflection){
 				glAlphaFunc(GL_GREATER,0.9);
 				glEnable(GL_ALPHA_TEST);
 			}
 		}
	} else if(shadowHandler->drawShadows){
		glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, groundFPShadow );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );
		float3 ac=readmap->ambientColor*(210.0/255.0);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, ac.x,ac.y,ac.z,1);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, 0,0,0,readmap->shadowDensity);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, ((CSmfReadMap*)readmap)->shadowTex);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		if (readmap->detailTex)
			glBindTexture(GL_TEXTURE_2D,  readmap->detailTex);
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
			glAlphaFunc(GL_GREATER,0.8);
			glEnable(GL_ALPHA_TEST);
		}
		if(overrideVP)
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, overrideVP );
		else
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, groundVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0/1024,1.0/1024,0,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x*0.02),-floor(camera->pos.z*0.02),0,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02,0.02,0,1);

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_MODELVIEW);
	} else {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, ((CSmfReadMap*)readmap)->shadowTex);
		SetTexGen(1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		

		glActiveTextureARB(GL_TEXTURE2_ARB);
		if(readmap->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,  readmap->detailTex);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			SetTexGen(0.02,0.02,-floor(camera->pos.x*0.02),-floor(camera->pos.z*0.02));
		} else glDisable (GL_TEXTURE_2D);
		if(overrideVP){
 			glEnable( GL_VERTEX_PROGRAM_ARB );
 			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, overrideVP );
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0/1024,1.0/1024,0,1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x*0.02),-floor(camera->pos.z*0.02),0,0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02,0.02,0,1);
 			if(drawReflection){
 				glAlphaFunc(GL_GREATER,0.9);
 				glEnable(GL_ALPHA_TEST);
 			}
 		}
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void CBFGroundDrawer::ResetTextureUnits(bool drawReflection,unsigned int overrideVP)
{
	if((drawExtraTex) || !shadowHandler->drawShadows){
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

void CBFGroundDrawer::SetTexGen(float scalex,float scaley, float offsetx, float offsety)
{
	GLfloat plan[]={scalex,0,0,offsetx};
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGenfv(GL_S,GL_EYE_PLANE,plan);
	glEnable(GL_TEXTURE_GEN_S);
	GLfloat plan2[]={0,0,scaley,offsety};
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGenfv(GL_T,GL_EYE_PLANE,plan2);
	glEnable(GL_TEXTURE_GEN_T);
}
