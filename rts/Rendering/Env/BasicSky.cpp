// Sky.cpp: implementation of the CBasicSky class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4258)

#include "StdAfx.h"
#include "BasicSky.h"

#include <math.h>
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include "Game/Camera.h"
#include "Sim/Map/ReadMap.h"
#include "Bitmap.h"
#include "TimeProfiler.h"
#include "Platform/ConfigHandler.h"
#include "Matrix44f.h"
#include "Game/UI/InfoConsole.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
#include <assert.h>

//////////////////////////////////////////////////////////////////////
extern GLfloat FogBlack[]; 
extern GLfloat FogLand[]; 

#define Y_PART 10.0
#define X_PART 10.0

#define CLOUD_DETAIL 6
#define CLOUD_SIZE 256

using namespace std;

CBasicSky::CBasicSky()
{	
	PrintLoadMsg("Creating sky");
	domeheight=cos(PI/16)*1.01;
	domeWidth=sin(2*PI/32)*400*1.7f;

	sundir2=gs->sunVector;
	sundir2.y=0;
	if(sundir2.Length()==0)
		sundir2.x=1;
	sundir2.Normalize();
	sundir1=sundir2.cross(UpVector);

	modSunDir.y=gs->sunVector.y;
	modSunDir.x=0;
	modSunDir.z=sqrt(gs->sunVector.x*gs->sunVector.x+gs->sunVector.z*gs->sunVector.z);

	sunTexCoordX=0.5;
	sunTexCoordY=GetTexCoordFromDir(modSunDir);

	readmap->mapDefParser.GetDef(cloudDensity,"0.5","MAP\\ATMOSPHERE\\CloudDensity");
	cloudDensity=0.25+cloudDensity*0.5;
	readmap->mapDefParser.GetDef(fogStart,"0.1","MAP\\ATMOSPHERE\\FogStart");
	if (fogStart>0.99) gu->drawFog = false;
	skyColor=readmap->mapDefParser.GetFloat3(float3(0.1,0.15,0.7),"MAP\\ATMOSPHERE\\SkyColor");
	sunColor=readmap->mapDefParser.GetFloat3(float3(1,1,1),"MAP\\ATMOSPHERE\\SunColor");
	cloudColor=readmap->mapDefParser.GetFloat3(float3(1,1,1),"MAP\\ATMOSPHERE\\CloudColor");

	for(int a=0;a<CLOUD_DETAIL;a++)
		cloudDown[a]=false;

	lastCloudUpdate=-30;
	cloudThickness=new unsigned char[CLOUD_SIZE*CLOUD_SIZE*4];
	CreateClouds();
	InitSun();
	oldCoverBaseX=-5;

	int y;
	glGetError();
	displist=glGenLists(1);
	glNewList(displist, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glFogf(GL_FOG_START,-20);
	glFogf(GL_FOG_END,50);
	glFogf(GL_FOG_DENSITY,1.0f);
	glEnable(GL_FOG);
	glColor4f(1,1,1,1);

	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, skyTex);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);		
  glActiveTextureARB(GL_TEXTURE0_ARB);

	for(y=0;y<Y_PART;y++){
		for(int x=0;x<X_PART;x++){
			glBegin(GL_TRIANGLE_STRIP);
			float3 c=GetCoord(x,y);
			
//			glTexCoord2f(c.x*0.025,c.z*0.025);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
			glVertex3f(c.x,c.y,c.z);
			
			c=GetCoord(x,y+1);
			
//			glTexCoord2f(c.x*0.025,c.z*0.025);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
			glVertex3f(c.x,c.y,c.z);
			
			c=GetCoord(x+1,y);
			
//			glTexCoord2f(c.x*0.025,c.z*0.025);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
			glVertex3f(c.x,c.y,c.z);
			
			c=GetCoord(x+1,y+1);
			
//			glTexCoord2f(c.x*0.025,c.z*0.025);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
			glVertex3f(c.x,c.y,c.z);

			glEnd();
		}
	}/**/

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
/*
  glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,sunTex);
	float3 sundir(0,0.5,1);
	sundir.Normalize();

	float3 ldir=sundir.cross(UpVector);
	float3 udir=sundir.cross(ldir);

	glDisable(GL_FOG);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
	glVertexf3(sundir*5+ldir*0.15f+udir*0.15f);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,1);
	glVertexf3(sundir*5+ldir*0.15f-udir*0.15f);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,1);
	glVertexf3(sundir*5-ldir*0.15f-udir*0.15f);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,0);
	glVertexf3(sundir*5-ldir*0.15f+udir*0.15f);
	glEnd();
*/  glActiveTextureARB(GL_TEXTURE0_ARB);
	glColor4f(1,1,1,1);
//	glEnable(GL_FOG);  Why not, glDisable is commented out above -- this text is so people don't accidentally uncomment just one

	if(GLEW_ARB_texture_env_dot3){
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_DOT3_RGB_ARB);

		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_ARB,GL_ONE_MINUS_SRC_ALPHA);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	
		for(y=0;y<Y_PART;y++){
			for(int x=0;x<X_PART;x++){
				glBegin(GL_TRIANGLE_STRIP);
				float3 c=GetCoord(x,y);
				
				glTexCoord2f(c.x*0.025,c.z*0.025);
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
//				glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x,c.z);
				glVertex3f(c.x,c.y,c.z);
				
				c=GetCoord(x,y+1);
				
				glTexCoord2f(c.x*0.025,c.z*0.025);
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
	//			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x,c.z);
				glVertex3f(c.x,c.y,c.z);
				
				c=GetCoord(x+1,y);
				
				glTexCoord2f(c.x*0.025,c.z*0.025);
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
		//		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x,c.z);
				glVertex3f(c.x,c.y,c.z);
				
				c=GetCoord(x+1,y+1);
				
				glTexCoord2f(c.x*0.025,c.z*0.025);
				glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5,c.z/domeWidth+0.5);
			//	glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x,c.z);
				glVertex3f(c.x,c.y,c.z);

				glEnd();
			}
		}
//		glDisable(GL_TEXTURE_SHADER_NV);

	}
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_ARB,GL_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
  glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_2D);
  glActiveTextureARB(GL_TEXTURE0_ARB);

	glEndList();
	dynamicSky=!!configHandler.GetInt("DynamicSky",0);
}

CBasicSky::~CBasicSky()
{
	glDeleteTextures(1, &skyTex);
	glDeleteTextures(1, &skyDot3Tex);
	glDeleteTextures(1, &cloudDot3Tex);
	glDeleteLists(displist,1);

	glDeleteTextures(1, &sunTex);
	glDeleteTextures(1, &sunFlareTex);
	glDeleteLists(sunFlareList,1);

	delete[] cloudThickness;
}

void CBasicSky::Draw()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glPushMatrix();
//	glTranslatef(camera->pos.x,camera->pos.y,camera->pos.z);
	CMatrix44f m(camera->pos,sundir1,UpVector,sundir2);
	glMultMatrixf(m.m);

	float3 modCamera=sundir1*camera->pos.x+sundir2*camera->pos.z;

	glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glTranslatef((gs->frameNum%20000)*0.00005+modCamera.x*0.000025,modCamera.z*0.000025,0);
	glMatrixMode(GL_MODELVIEW);



	glCallList(displist);

	glMatrixMode(GL_TEXTURE);						// Select The Projection Matrix
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	if (gu->drawFog) {
		glFogfv(GL_FOG_COLOR,FogLand);
		glFogi(GL_FOG_MODE,GL_LINEAR);
		glFogf(GL_FOG_START,gu->viewRange*fogStart);
		glFogf(GL_FOG_END,gu->viewRange);
		glFogf(GL_FOG_DENSITY,1.00f);
	} else {
		glDisable(GL_FOG);
	}
}

float3 CBasicSky::GetTexCoord(int x, int y)
{
	float3 c;
	float a=((float)y/Y_PART)*0.5f;
	float b=((float)x/X_PART)*2*PI;
	c.x=0.5f+sin(b)*a;
	c.y=0.5f+cos(b)*a;
	return c;
}

float3 CBasicSky::GetCoord(int x, int y)
{
	float3 c;
	float fy=((float)y/Y_PART)*2*PI;
	float fx=((float)x/X_PART)*2*PI;

	c.x=sin(fy/32)*sin(fx)*400;
	c.y=(cos(fy/32)-domeheight)*400;
	c.z=sin(fy/32)*cos(fx)*400;
	return c;
}

void CBasicSky::CreateClouds()
{
	glGenTextures(1, &skyTex);
	glGenTextures(1, &skyDot3Tex);
	glGenTextures(1, &cloudDot3Tex);
	int y;

	static unsigned char skytex[512][512][4];//=new unsigned char[512][512][4];
	static unsigned char skytex2[256][256][4];
	
	for(y=0;y<512;y++){
		for(int x=0;x<512;x++){
			float3 dir=GetDirFromTexCoord(x/512.0,y/512.0);
			float sunDist=acos(dir.dot(modSunDir))*70;
			float sunMod=12.0/(12+sunDist);

			float red=(skyColor.x+sunMod*sunColor.x);
			float green=(skyColor.y+sunMod*sunColor.y);
			float blue=(skyColor.z+sunMod*sunColor.z);
			if(red>1)
				red=1;
			if(green>1)
				green=1;
			if(blue>1)
				blue=1;
			skytex[y][x][0]=(unsigned char)(red*255);
			skytex[y][x][1]=(unsigned char)(green*255);
			skytex[y][x][2]=(unsigned char)(blue*255);
			skytex[y][x][3]=255;
		}
	}

	glBindTexture(GL_TEXTURE_2D, skyTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, skytex[0][0]);
//	delete[] skytex;

	for(y=0;y<256;y++){
		for(int x=0;x<256;x++){
			float3 dir=GetDirFromTexCoord(x/256.0,y/256.0);
			float sunDist=acos(dir.dot(modSunDir))*50;
			float sunMod=0.3/sqrt(sunDist)+2.0/sunDist;
			float green=(0.55f+sunMod);
			if(green>1)
				green=1;
			skytex2[y][x][0]=255-y/2;
			skytex2[y][x][1]=(unsigned char)(green*255);
			skytex2[y][x][2]=203;
			skytex2[y][x][3]=255;
		}
	}

	glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,256, 256, GL_RGBA, GL_UNSIGNED_BYTE, skytex2[0][0]);

	for(int a=0;a<CLOUD_DETAIL;a++){
		CreateRandMatrix(randMatrix[a],1-a*0.03);
		CreateRandMatrix(randMatrix[a+8],1-a*0.03);
	}

	char* scrap=new char[CLOUD_SIZE*CLOUD_SIZE*4];
	glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, CLOUD_SIZE, CLOUD_SIZE,0,GL_RGBA, GL_UNSIGNED_BYTE, scrap);
	delete[] scrap;

	dynamicSky=true;
	CreateTransformVectors();
	Update();
	dynamicSky=false;
}

void CBasicSky::Update()
{
	if(lastCloudUpdate>gs->frameNum-10 || !dynamicSky)
		return;
START_TIME_PROFILE
	lastCloudUpdate=gs->frameNum;

	int blendMatrix[8][32][32];
	for(int a=0;a<CLOUD_DETAIL;a++){
		float fade=(gs->frameNum/(70.0*(2<<(CLOUD_DETAIL-1-a))));
		fade-=floor(fade/2)*2;
		if(fade>1){
			fade=2-fade;
			if(!cloudDown[a]){
				cloudDown[a]=true;
				CreateRandMatrix(randMatrix[a+8],1-a*0.03);
			}
		} else {
			if(cloudDown[a]){
				cloudDown[a]=false;
				CreateRandMatrix(randMatrix[a],1-a*0.03);
			}

		}
		int ifade=(int)((3*fade*fade-2*fade*fade*fade)*256);

		for(int y=0;y<32;y++){
			for(int x=0;x<32;x++){
				blendMatrix[a][y][x]=(randMatrix[a][y][x]*ifade+randMatrix[a+8][y][x]*(256-ifade))>>8;
			}
		}
	}
	int rawClouds[CLOUD_SIZE][CLOUD_SIZE];//=new int[CLOUD_SIZE][CLOUD_SIZE];

	for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++){
		rawClouds[0][a]=0;
	}

	int kernel[CLOUD_SIZE/4*CLOUD_SIZE/4];
	for(int a=0;a<CLOUD_DETAIL;a++){
		int expLevel=1<<a;

		for(int y=0;y<(CLOUD_SIZE/4)>>a;++y){
			float ydist=fabs((float)1+y-((CLOUD_SIZE/8)>>a))/((CLOUD_SIZE/8)>>a);
			ydist=3*ydist*ydist-2*ydist*ydist*ydist;
			for(int x=0;x<(CLOUD_SIZE/4)>>a;++x){
				float xdist=fabs((float)1+x-((CLOUD_SIZE/8)>>a))/((CLOUD_SIZE/8)>>a);
				xdist=3*xdist*xdist-2*xdist*xdist*xdist;
				
				float contrib=(1-xdist)*(1-ydist);
				kernel[y*CLOUD_SIZE/4+x]=(int)( contrib*((4<<CLOUD_DETAIL)>>a));
			}
		}
		unsigned int by=0,bx=0;
		for(int y=0;y<CLOUD_SIZE-((CLOUD_SIZE/8)>>a);y+=(CLOUD_SIZE/8)>>a){
			for(int x=0;x<CLOUD_SIZE-((CLOUD_SIZE/8)>>a);x+=(CLOUD_SIZE/8)>>a){
				int blend=blendMatrix[a][by&31][bx&31];
				for(int y2=0;y2<((CLOUD_SIZE/4)>>a)-1;++y2){
					for(int x2=0;x2<((CLOUD_SIZE/4)>>a)-1;++x2){
						rawClouds[y+y2][x+x2]+=blend*kernel[y2*CLOUD_SIZE/4+x2];
					}
				}
				bx++;
			}
			by++;
		}
		by=0;
		bx=31;
		for(int y=0;y<CLOUD_SIZE-((CLOUD_SIZE/8)>>a);y+=(CLOUD_SIZE/8)>>a){
			int x=CLOUD_SIZE-((CLOUD_SIZE/8)>>a);
			int blend=blendMatrix[a][by&31][bx&31];
			for(int y2=0;y2<((CLOUD_SIZE/4)>>a)-1;++y2){
				for(int x2=0;x2<((CLOUD_SIZE/4)>>a)-1;++x2){
					if(x+x2<CLOUD_SIZE)
						rawClouds[y+y2][x+x2]+=blend*kernel[y2*CLOUD_SIZE/4+x2];
					else 
						rawClouds[y+y2][x+x2-CLOUD_SIZE]+=blend*kernel[y2*CLOUD_SIZE/4+x2];
				}
			}
			by++;
		}
		bx=0;
		by=31;
		for(int x=0;x<CLOUD_SIZE-((CLOUD_SIZE/8)>>a);x+=(CLOUD_SIZE/8)>>a){
			int y=CLOUD_SIZE-((CLOUD_SIZE/8)>>a);
			int blend=blendMatrix[a][by&31][bx&31];
			for(int y2=0;y2<((CLOUD_SIZE/4)>>a)-1;++y2){
				for(int x2=0;x2<((CLOUD_SIZE/4)>>a)-1;++x2){
					if(y+y2<CLOUD_SIZE)
						rawClouds[y+y2][x+x2]+=blend*kernel[y2*CLOUD_SIZE/4+x2];
					else 
						rawClouds[y+y2-CLOUD_SIZE][x+x2]+=blend*kernel[y2*CLOUD_SIZE/4+x2];
				}
			}
			bx++;
		}
	}

	int max=0;
	for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++){
		cloudThickness[a*4+3]=alphaTransform[rawClouds[0][a]>>7];
	}

	//create the cloud shading
	int ydif[CLOUD_SIZE];
	for(int a=0;a<CLOUD_SIZE;++a){
		ydif[a]=0;
		ydif[a]+=cloudThickness[(a+3*CLOUD_SIZE)*4+3];
		ydif[a]+=cloudThickness[(a+2*CLOUD_SIZE)*4+3];
		ydif[a]+=cloudThickness[(a+1*CLOUD_SIZE)*4+3];
		ydif[a]+=cloudThickness[(a+0*CLOUD_SIZE)*4+3];
		ydif[a]-=cloudThickness[(a-1*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)*4+3];
		ydif[a]-=cloudThickness[(a-2*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)*4+3];
		ydif[a]-=cloudThickness[(a-3*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)*4+3];
	}

	int a=0;
	ydif[(a)&255]+=cloudThickness[(a-3*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)*4+3];
	ydif[(a)&255]-=cloudThickness[(a)*4+3]*2;
	ydif[(a)&255]+=cloudThickness[(a+4*CLOUD_SIZE)*4+3];

	for(int a=0;a<CLOUD_SIZE*3-1;a++){
		ydif[(a+1)&255]+=cloudThickness[(a-3*CLOUD_SIZE+1+CLOUD_SIZE*CLOUD_SIZE)*4+3];
		ydif[(a+1)&255]-=cloudThickness[(a+1)*4+3]*2;
		ydif[(a+1)&255]+=cloudThickness[(a+4*CLOUD_SIZE+1)*4+3];

		int dif=0;

		dif+=ydif[(a)&255];
		dif+=ydif[(a+1)&255]>>1;
		dif+=ydif[(a-1)&255]>>1;
		dif+=ydif[(a+2)&255]>>2;
		dif+=ydif[(a-2)&255]>>2;
		dif/=16;

		cloudThickness[a*4+0]=128+dif;
		cloudThickness[a*4+1]=thicknessTransform[rawClouds[0][a]>>7];
		cloudThickness[a*4+2]=255;
	}

	for(int a=CLOUD_SIZE*3-1;a<CLOUD_SIZE*CLOUD_SIZE-CLOUD_SIZE*4-1;a++){
		ydif[(a+1)&255]+=cloudThickness[(a-3*CLOUD_SIZE+1)*4+3];
		ydif[(a+1)&255]-=cloudThickness[(a+1)*4+3]*2;
		ydif[(a+1)&255]+=cloudThickness[(a+4*CLOUD_SIZE+1)*4+3];

		int dif=0;

		dif+=ydif[(a)&255];
		dif+=ydif[(a+1)&255]>>1;
		dif+=ydif[(a-1)&255]>>1;
		dif+=ydif[(a+2)&255]>>2;
		dif+=ydif[(a-2)&255]>>2;
		dif/=16;

		cloudThickness[a*4+0]=128+dif;
		cloudThickness[a*4+1]=thicknessTransform[rawClouds[0][a]>>7];
		cloudThickness[a*4+2]=255;
	}

	for(int a=CLOUD_SIZE*CLOUD_SIZE-CLOUD_SIZE*4-1;a<CLOUD_SIZE*CLOUD_SIZE;a++){
		ydif[(a+1)&255]+=cloudThickness[(a-3*CLOUD_SIZE+1)*4+3];
		ydif[(a+1)&255]-=cloudThickness[(a+1)*4+3]*2;
		ydif[(a+1)&255]+=cloudThickness[(a+4*CLOUD_SIZE+1-CLOUD_SIZE*CLOUD_SIZE)*4+3];

		int dif=0;

		dif+=ydif[(a)&255];
		dif+=ydif[(a+1)&255]>>1;
		dif+=ydif[(a-1)&255]>>1;
		dif+=ydif[(a+2)&255]>>2;
		dif+=ydif[(a-2)&255]>>2;
		dif/=16;

		cloudThickness[a*4+0]=128+dif;
		cloudThickness[a*4+1]=thicknessTransform[rawClouds[0][a]>>7];
		cloudThickness[a*4+2]=255;
	}

//	delete[] rawClouds;
/*
	for(int a=0;a<CLOUD_SIZE;++a){
		cloudThickness[((int(48+camera->pos.z*CLOUD_SIZE*0.000025)%256)*CLOUD_SIZE+a)*4+3]=0;		
	}
	for(int a=0;a<CLOUD_SIZE;++a){
		cloudThickness[(a*CLOUD_SIZE+int(gs->frameNum*0.00009*256+camera->pos.x*CLOUD_SIZE*0.000025))*4+3]=0;		
	}
/**/
	glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
	glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,CLOUD_SIZE, CLOUD_SIZE,GL_RGBA, GL_UNSIGNED_BYTE, cloudThickness);

END_TIME_PROFILE("Drawing sky");
}

void CBasicSky::CreateRandMatrix(int matrix[32][32],float mod)
{
	for(int y=0;y<32;y++){
		for(int x=0;x<32;x++){
			double r = ((double)( rand() )) / (double)RAND_MAX;
			matrix[y][x]=((int)( r * 255.0 ));
		}
	}
}

void CBasicSky::CreateTransformVectors()
{
	for(int a=0;a<1024;++a){
		float f=(1023.0-(a+cloudDensity*1024-512))/1023.0;
		float alpha=pow(f*2,3);
		if(alpha>1)
			alpha=1;
		alphaTransform[a]=(unsigned char)(alpha*255);

		float d=f*2;
		if(d>1)
			d=1;
		thicknessTransform[a]=(unsigned char)(128+d*64+alphaTransform[a]*255/(4*255));
	}
}

void CBasicSky::DrawSun()
{
	glPushMatrix();
//	glTranslatef(camera->pos.x,camera->pos.y,camera->pos.z);
	CMatrix44f m(camera->pos,sundir1,UpVector,sundir2);
	glMultMatrixf(m.m);
	glDisable(GL_DEPTH_TEST);

	unsigned char buf[32];
	glEnable(GL_TEXTURE_2D);

	float3 modCamera=sundir1*camera->pos.x+sundir2*camera->pos.z;

	float ymod=(sunTexCoordY-0.5)*domeWidth*0.025*256;
	float fy=ymod+modCamera.z*CLOUD_SIZE*0.000025;
	int baseY=int(floor(fy))%256;
	fy-=floor(fy);
	float fx=gs->frameNum*0.00005*CLOUD_SIZE+modCamera.x*CLOUD_SIZE*0.000025;
	int baseX=int(floor(fx))%256;
	fx-=floor(fx);

	if(baseX!=oldCoverBaseX || baseY!=oldCoverBaseY){
		oldCoverBaseX=baseX;
		oldCoverBaseY=baseY;
		CreateCover(baseX,baseY,covers[0]);
		CreateCover(baseX+1,baseY,covers[1]);
		CreateCover(baseX,baseY+1,covers[2]);
		CreateCover(baseX+1,baseY+1,covers[3]);
	}

	for(int x=0;x<32;++x){
		float cx1=covers[0][x]*(1-fx)+covers[1][x]*fx;
		float cx2=covers[2][x]*(1-fx)+covers[3][x]*fx;

		float cover=cx1*(1-fy)+cx2*fy;

		if(cover>127.5)
			cover=127.5;

		buf[x]=(unsigned char)(255-cover*2);
	}
	glBindTexture(GL_TEXTURE_2D, sunFlareTex);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,32,1,GL_LUMINANCE,GL_UNSIGNED_BYTE,buf);

	glColor4f(0.4f*sunColor.x,0.4f*sunColor.y,0.4f*sunColor.z,0.0f);
	glCallList(sunFlareList);

	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
}

void CBasicSky::SetCloudShadow(int texunit)
{
}

void CBasicSky::ResetCloudShadow(int texunit)
{
}

void CBasicSky::DrawShafts()
{
}


void CBasicSky::InitSun()
{
	unsigned char* mem=new unsigned char[128*128*4];

	for(int y=0;y<128;++y){
		for(int x=0;x<128;++x){
			mem[(y*128+x)*4+0]=255;
			mem[(y*128+x)*4+1]=255;
			mem[(y*128+x)*4+2]=255;
			float dist=sqrt((float)(y-64)*(y-64)+(x-64)*(x-64));
			if(dist>60)
				mem[(y*128+x)*4+3]=0;
			else
				mem[(y*128+x)*4+3]=255;
		}
	}

	glGenTextures(1, &sunTex);
	glBindTexture(GL_TEXTURE_2D, sunTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,128, 128, GL_RGBA, GL_UNSIGNED_BYTE, mem);

	for(int y=0;y<2;++y){
		for(int x=0;x<32;++x){
			if(y==0 && x%2)
				mem[(y*32+x)]=255;
			else
				mem[(y*32+x)]=0;
		}
	}

	glGenTextures(1, &sunFlareTex);
	glBindTexture(GL_TEXTURE_2D, sunFlareTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
//	gluBuild2DMipmaps(GL_TEXTURE_2D,1 ,32, 2, GL_ALPHA, GL_UNSIGNED_BYTE, mem);
	glTexImage2D(GL_TEXTURE_2D,0,1 ,32, 2,0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mem);

	delete[] mem;

	float3 ldir=modSunDir.cross(UpVector);
	float3 udir=modSunDir.cross(ldir);

	sunFlareList=glGenLists(1);
	glNewList(sunFlareList, GL_COMPILE);
		glDisable(GL_FOG);
		glBindTexture(GL_TEXTURE_2D, sunFlareTex);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
		glBegin(GL_TRIANGLE_STRIP);
		for(int x=0;x<257;++x){
			float dx=sin(x*2*PI/256.0f);
			float dy=cos(x*2*PI/256.0f);

			glTexCoord2f(x/256.0f,0.25f);
			glVertexf3(modSunDir*5+ldir*dx*0.0014f+udir*dy*0.0014f);
			glTexCoord2f(x/256.0f,0.75f);
			glVertexf3(modSunDir*5+ldir*dx*4+udir*dy*4);
		}
		glEnd();
		if (gu->drawFog) glEnable(GL_FOG);
	glEndList();
}

inline unsigned char CBasicSky::GetCloudThickness(int x,int y)
{
	assert (CLOUD_SIZE==256);
	x &= 0xff;
	y &= 0xff;

	//assert (x>=0 && x<CLOUD_SIZE);
	//assert (y>=0 && y<CLOUD_SIZE);
	return cloudThickness [ (y * CLOUD_SIZE + x) * 4 + 3 ];
}

void CBasicSky::CreateCover(int baseX, int baseY, float *buf)
{
	static int line[]={ 5, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 5, 0, 1, 0, 2, 1, 3, 1, 4, 1, 5, 5, 0, 1, 1, 2, 1, 3, 2, 4, 2, 5, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 1, 1, 2, 2, 3, 3, 4, 4, 4, 1, 1, 2, 2, 3, 2, 4, 3, 5, 1, 0, 2, 1, 3, 1, 4, 2, 5, 2, 5, 1, 0, 2, 0, 3, 1, 4, 1, 5, 1};
	int i=0;

	for(int l=0;l<8;++l){
		int num=line[i++];
		int cover1=0;
		int cover2=0;
		int cover3=0;
		int cover4=0;
		float total=0;
		for(int x=0;x<num;++x){
			int dx=line[i++];
			int dy=line[i++];
			cover1+=255-GetCloudThickness(baseX-dx,baseY+dy);
			cover2+=255-GetCloudThickness(baseX-dy,baseY-dx);//*(num-x);
			cover3+=255-GetCloudThickness(baseX+dx,baseY-dy);//*(num-x);
			cover4+=255-GetCloudThickness(baseX+dy,baseY+dx);//*(num-x);

			total+=1;//(num-x);
		}

		buf[l]=cover1/total;
		buf[l+8]=cover2/total;
		buf[l+16]=cover3/total;
		buf[l+24]=cover4/total;
	}
}

float3 CBasicSky::GetDirFromTexCoord(float x, float y)
{
	float3 dir;

	dir.x=(x-0.5)*domeWidth;
	dir.z=(y-0.5)*domeWidth;

	float hdist=sqrt(dir.x*dir.x+dir.z*dir.z);
	float fy=asin(hdist/400);
	dir.y=(cos(fy)-domeheight)*400;

	dir.Normalize();

	return dir;
}

//should be improved
//only take stuff in yz plane
float CBasicSky::GetTexCoordFromDir(float3 dir)
{
	float tp=0.5;
	float step=0.25;
	
	for(int a=0;a<10;++a){
		float tx=0.5+tp;
		float3 d=GetDirFromTexCoord(tx,0.5);
		if(d.y<dir.y)
			tp-=step;
		else
			tp+=step;
		step*=0.5;
	}
	return 0.5+tp;
}
