#include "StdAfx.h"
#include "DynWater.h"
#include "Game/Game.h"
#include "Rendering/GL/myGL.h"
#include <math.h>
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Map/ReadMap.h"
#include "Game/UI/InfoConsole.h"
#include "Rendering/Map/BaseGroundDrawer.h"
#include "BaseSky.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Map/SmfReadMap.h"

extern GLfloat FogLand[]; 

CDynWater::CDynWater(void)
{
	glGenTextures(1, &reflectTexture);
	unsigned char* scrap=new unsigned char[512*512*4];

	glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,512, 512, 0,GL_RGBA, GL_UNSIGNED_BYTE, scrap);

	glGenTextures(1, &bumpTexture);
	glBindTexture(GL_TEXTURE_2D, bumpTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,128, 128, 0,GL_RGBA, GL_UNSIGNED_BYTE, scrap);

	glGenTextures(4, rawBumpTexture);

	for(int y=0;y<64;++y){
		for(int x=0;x<64;++x){
			scrap[(y*64+x)*4+0]=(unsigned char)((sin(y*PI*2.0/64.0))*128)+128;
			scrap[(y*64+x)*4+1]=0;
			scrap[(y*64+x)*4+2]=0;
			scrap[(y*64+x)*4+3]=255;
		}
	}
	glBindTexture(GL_TEXTURE_2D, rawBumpTexture[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,64, 64, 0,GL_RGBA, GL_UNSIGNED_BYTE, scrap);

	delete[] scrap;

	waterFP=LoadFragmentProgram("water.fp");

	waterSurfaceColor = ((CSmfReadMap*)readmap)->waterSurfaceColor;
}

CDynWater::~CDynWater(void)
{
	glDeleteTextures (1, &reflectTexture);
	glDeleteTextures (1, &bumpTexture);
	glDeleteTextures (4, rawBumpTexture);
	glDeleteProgramsARB( 1, &waterFP );
}

void CDynWater::Draw()
{
	if(readmap->minheight>10)
	return;
	float3 dir,zpos;
	float3 base=camera->CalcPixelDir(0,gu->screeny);
	float3 dv=camera->CalcPixelDir(0,0)-camera->CalcPixelDir(0,gu->screeny);
	float3 dh=camera->CalcPixelDir(gu->screenx,0)-camera->CalcPixelDir(0,0);

	float3 xbase;
	const int numDivs=20;

	base*=numDivs;
	float maxY=-0.1f;
	float yInc=1.0f/numDivs;
	float screenY=1;

	unsigned char col[4];
	col[0]=(unsigned char)(waterSurfaceColor.x*255);
	col[1]=(unsigned char)(waterSurfaceColor.y*255);
	col[2]=(unsigned char)(waterSurfaceColor.z*255);

	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(0);
	glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, bumpTexture);
		GLfloat plan[]={0.02f,0,0,0};
		glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
		glTexGenfv(GL_S,GL_EYE_PLANE,plan);
		glEnable(GL_TEXTURE_GEN_S);
			
		GLfloat plan2[]={0,0,0.02f,0};
		glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
		glTexGenfv(GL_T,GL_EYE_PLANE,plan2);
		glEnable(GL_TEXTURE_GEN_T);
	glActiveTextureARB(GL_TEXTURE0_ARB);

	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, waterFP );
	glEnable( GL_FRAGMENT_PROGRAM_ARB );

	float3 forward=camera->forward;
	forward.y=0;
	forward.Normalize();

	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,0, forward.z,forward.x,0,0);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,1, -forward.x,forward.z,0,0);

	CVertexArray* va=GetVertexArray();
	va->Initialize();
	for(int a=0;a<5;++a){
		bool maxReached=false;
		for(int y=0;y<numDivs;++y){
			dir=base;
			dir.Normalize();

			if(dir.y>=maxY){
				maxReached=true;
				break;
			}

			xbase=base;
			for(int x=0;x<numDivs+1;++x){
				dir=xbase+dv;
				dir.Normalize();
				zpos=camera->pos+dir*(camera->pos.y/-dir.y);
				zpos.y=sin(zpos.z*0.1+gs->frameNum*0.06)*0.06+0.05;
				col[3]=(unsigned char)((0.8+0.7*(dir.y))*255);
				va->AddVertexTC(zpos,x*(1.0f/numDivs),screenY-yInc,col);

				dir=xbase;
				dir.Normalize();
				zpos=camera->pos+dir*(camera->pos.y/-dir.y);
				zpos.y=sin(zpos.z*0.1+gs->frameNum*0.06)*0.06+0.05;
				col[3]=(unsigned char)((0.8+0.7*(dir.y))*255);
				va->AddVertexTC(zpos,x*(1.0f/numDivs),screenY,col);

				xbase+=dh;
			}
			va->EndStrip();
			base+=dv;
			screenY-=yInc;
		}
		if(!maxReached)
			break;
		dv*=0.5;
		maxY*=0.5;
		yInc*=0.5;
	}
	va->DrawArrayTC(GL_TRIANGLE_STRIP);

	glDepthMask(1);
	glDisable( GL_FRAGMENT_PROGRAM_ARB );
	glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void CDynWater::UpdateWater(CGame* game)
{
	if(readmap->minheight>10)
		return;
	glViewport(0,0,128,128);

	glClearColor(0.0f,0.0f,0.0f,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColorMask(1,1,1,0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, rawBumpTexture[0]);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);

	glColor3f(1,1,1);
	glBegin(GL_QUADS);
	glColor3f(0.33,0.33,0.33);
	glTexCoord2f(0,0+gs->frameNum*0.0046);glVertex3f(0,0,0);
	glTexCoord2f(0,2+gs->frameNum*0.0046);glVertex3f(0,1,0);
	glTexCoord2f(2,2+gs->frameNum*0.0046);glVertex3f(1,1,0);
	glTexCoord2f(2,0+gs->frameNum*0.0046);glVertex3f(1,0,0);

	glTexCoord2f(0,0+gs->frameNum*0.0026);glVertex3f(0,0,0);
	glTexCoord2f(0,4+gs->frameNum*0.0026);glVertex3f(0,1,0);
	glTexCoord2f(2,4+gs->frameNum*0.0026);glVertex3f(1,1,0);
	glTexCoord2f(2,0+gs->frameNum*0.0026);glVertex3f(1,0,0);

	glTexCoord2f(0,0+gs->frameNum*0.0012);glVertex3f(0,0,0);
	glTexCoord2f(0,8+gs->frameNum*0.0012);glVertex3f(0,1,0);
	glTexCoord2f(2,8+gs->frameNum*0.0012);glVertex3f(1,1,0);
	glTexCoord2f(2,0+gs->frameNum*0.0012);glVertex3f(1,0,0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, bumpTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,128,128);

	CCamera realCam=*camera;

	camera->up.x=0;
	camera->up.y=1;
	camera->up.z=0;
	camera->forward.y*=-1;
	camera->pos.y*=-1;
	camera->pos.y+=0.2;
	camera->Update(false);

	glViewport(0,0,512,512);

	glClearColor(0.2f,0.4f,0.2f,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	sky->Draw();

	glEnable(GL_CLIP_PLANE2);
	double plane[4]={0,1,0,3};
	glClipPlane(GL_CLIP_PLANE2 ,plane);
	drawReflection=true;

	groundDrawer->Draw(true);
	unitDrawer->Draw(true);
	featureHandler->Draw();
	ph->Draw(true);

	drawReflection=false;
	glDisable(GL_CLIP_PLANE2);

	glColorMask(1,1,1,1);
	glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,512,512);

	glViewport(0,0,gu->screenx,gu->screeny);
	glClearColor(FogLand[0],FogLand[1],FogLand[2],1);

	*camera=realCam;
	camera->Update(false);
}

void CDynWater::Update()
{

}
