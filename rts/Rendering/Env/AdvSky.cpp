// Sky.cpp: implementation of the CAdvSky class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AdvSky.h"

#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/GL/VertexArray.h"
#include "LogOutput.h"
#include "TimeProfiler.h"
#include "Matrix44f.h"
#include "mmgr.h"

extern GLfloat FogBlack[];
extern GLfloat FogLand[];

#define Y_PART 10.0
#define X_PART 10.0

#define CLOUD_DETAIL 6
#define CLOUD_SIZE 256
#define CLOUD_MASK (CLOUD_SIZE-1)

//static unsigned int cdtex;

using namespace std;

CAdvSky::CAdvSky()
{
	PrintLoadMsg("Creating sky");
	domeheight=cos(PI/16)*1.01f;
	domeWidth=sin(2*PI/32)*400*1.7f;

	sundir2=mapInfo->light.sunDir;
	sundir2.y=0;
	if(sundir2.Length()==0)
		sundir2.x=1;
	sundir2.Normalize();
	sundir1=sundir2.cross(UpVector);

	modSunDir.y=mapInfo->light.sunDir.y;
	modSunDir.x=0;
	modSunDir.z=sqrt(mapInfo->light.sunDir.x*mapInfo->light.sunDir.x+mapInfo->light.sunDir.z*mapInfo->light.sunDir.z);

	sunTexCoordX=0.5f;
	sunTexCoordY=GetTexCoordFromDir(modSunDir);

	for(int a=0;a<CLOUD_DETAIL;a++)
		cloudDown[a]=false;
	for(int a=0;a<5;a++)
		cloudDetailDown[a]=false;

//	dynamicSky=!!regHandler.GetInt("DynamicSky",0);
	dynamicSky=false;
	lastCloudUpdate=-30;

	cloudDensity = mapInfo->atmosphere.cloudDensity;
	cloudColor = mapInfo->atmosphere.cloudColor;
	skyColor = mapInfo->atmosphere.skyColor;
	sunColor = mapInfo->atmosphere.sunColor;
	fogStart = mapInfo->atmosphere.fogStart;
	if (fogStart>0.99f) gu->drawFog = false;

	CreateClouds();
	InitSun();
	oldCoverBaseX=-5;

	cloudFP=LoadFragmentProgram("clouds.fp");

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
	glFogf(GL_FOG_DENSITY,1.0f/15);
	glEnable(GL_FOG);
	glColor4f(1,1,1,1);

	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, cloudFP );
	glEnable( GL_FRAGMENT_PROGRAM_ARB );
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, sunColor.x,sunColor.y,sunColor.z,0);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, sunColor.x*cloudColor.x,sunColor.y*cloudColor.y,sunColor.z*cloudColor.z,0);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,12, cloudColor.x,cloudColor.y,cloudColor.z,0);

	glBindTexture(GL_TEXTURE_2D, skyTex);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
	glActiveTextureARB(GL_TEXTURE3_ARB);
	glBindTexture(GL_TEXTURE_2D, cdtex);
	glActiveTextureARB(GL_TEXTURE0_ARB);

	for(int y=0;y<Y_PART;y++){
		for(int x=0;x<X_PART;x++){
			glBegin(GL_TRIANGLE_STRIP);
			float3 c=GetCoord(x,y);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			c=GetCoord(x,y+1);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			c=GetCoord(x+1,y);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			c=GetCoord(x+1,y+1);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			glEnd();
		}
	}

	glDisable( GL_FRAGMENT_PROGRAM_ARB );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);
	glEndList();
}

CAdvSky::~CAdvSky()
{
	glDeleteTextures(1, &skyTex);
	glDeleteTextures(1, &skyDot3Tex);
	glDeleteTextures(1, &cloudDot3Tex);
	glDeleteLists(displist,1);

	glDeleteTextures(1, &sunTex);
	glDeleteTextures(1, &sunFlareTex);
	glDeleteLists(sunFlareList,1);

	delete[] cloudThickness2;
	delete[] cloudTexMem;

	glSafeDeleteProgram( cloudFP );
}

void CAdvSky::Draw()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glPushMatrix();
//	glTranslatef(camera->pos.x,camera->pos.y,camera->pos.z);
	CMatrix44f m(camera->pos,sundir1,UpVector,sundir2);
	glMultMatrixf(m.m);

	float3 modCamera=sundir1*camera->pos.x+sundir2*camera->pos.z;

	glMatrixMode(GL_TEXTURE);
	  glActiveTextureARB(GL_TEXTURE2_ARB);
		glPushMatrix();
		glTranslatef((gs->frameNum%20000)*0.00005f+modCamera.x*0.000025f,modCamera.z*0.000025f,0);
	  glActiveTextureARB(GL_TEXTURE3_ARB);
		glPushMatrix();
		glTranslatef((gs->frameNum%20000)*0.0020f+modCamera.x*0.001f,modCamera.z*0.001f,0);
	  glActiveTextureARB(GL_TEXTURE0_ARB);
	glMatrixMode(GL_MODELVIEW);

	glCallList(displist);

	if (gu->drawFog) {
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR,FogLand);
		glFogf(GL_FOG_START,gu->viewRange*fogStart);
		glFogf(GL_FOG_END,gu->viewRange);
		glFogf(GL_FOG_DENSITY,1.0f);
		glFogi(GL_FOG_MODE,GL_LINEAR);
	} else {
		glDisable(GL_FOG);
	}

	glMatrixMode(GL_TEXTURE);
	  glActiveTextureARB(GL_TEXTURE2_ARB);
		glPopMatrix();
	  glActiveTextureARB(GL_TEXTURE3_ARB);
		glPopMatrix();
	  glActiveTextureARB(GL_TEXTURE0_ARB);
	glMatrixMode(GL_MODELVIEW);

	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

float3 CAdvSky::GetCoord(int x, int y)
{
	float3 c;
	float fy=((float)y/Y_PART)*2*PI;
	float fx=((float)x/X_PART)*2*PI;

	c.x=sin(fy/32)*sin(fx)*400;
	c.y=(cos(fy/32)-domeheight)*400;
	c.z=sin(fy/32)*cos(fx)*400;
	return c;
}

void CAdvSky::CreateClouds()
{
	cloudThickness2=SAFE_NEW unsigned char[CLOUD_SIZE*CLOUD_SIZE+1];
	cloudTexMem=SAFE_NEW unsigned char[CLOUD_SIZE*CLOUD_SIZE*4];

	glGenTextures(1, &skyTex);
	glGenTextures(1, &skyDot3Tex);
	glGenTextures(1, &cloudDot3Tex);

	unsigned char (* skytex)[512][4]=SAFE_NEW unsigned char[512][512][4];

	glGenTextures(1, &cdtex);
//	CBitmap pic("bitmaps/clouddetail.bmp");
	unsigned char mem[256*256];
//	for(int a=0;a<256*256;++a){
//		mem[a]=pic.mem[a*4];
//	}
	glBindTexture(GL_TEXTURE_2D, cdtex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE ,256, 256, GL_LUMINANCE, GL_UNSIGNED_BYTE, mem);

	unsigned char randDetailMatrix[32*32];
	glGenTextures(12, detailTextures);
	for(int a=0;a<6;++a){
		int size=min(32,256>>a);
		CreateRandDetailMatrix(randDetailMatrix,size);
		glBindTexture(GL_TEXTURE_2D, detailTextures[a]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE, size, size,0,GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);

		CreateRandDetailMatrix(randDetailMatrix,size);
		glBindTexture(GL_TEXTURE_2D, detailTextures[a+6]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE, size, size,0,GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);
	}

	for(int y=0;y<512;y++){
		for(int x=0;x<512;x++){
			float3 dir=GetDirFromTexCoord(x/512.0f,y/512.0f);
			float sunDist=acos(dir.dot(modSunDir))*70;
			float sunMod=12.0f/(12+sunDist);

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
	delete[] skytex;

	unsigned char skytex2[256][256][4];
	for(int y=0;y<256;y++){
		for(int x=0;x<256;x++){
			float3 dir=GetDirFromTexCoord(x/256.0f,y/256.0f);
			float sunDist=acos(dir.dot(modSunDir))*50;
			float sunMod=0.3f/sqrt(sunDist)+3.0f/(1+sunDist);
			float green=min(1.0f,(0.55f+sunMod));
			float blue=203-(40.0f/(3+sunDist));
			skytex2[y][x][0]=(unsigned char)(255-y/2);				//sun on borders
			skytex2[y][x][1]=(unsigned char)(green*255);			//sun light through
			skytex2[y][x][2]=(unsigned char) blue;						//ambient
			skytex2[y][x][3]=255;
		}
	}

	glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,256, 256, GL_RGBA, GL_UNSIGNED_BYTE, skytex2[0][0]);

	for(int a=0;a<CLOUD_DETAIL;a++){
		CreateRandMatrix(randMatrix[a],1-a*0.03f);
		CreateRandMatrix(randMatrix[a+8],1-a*0.03f);
	}

	char* scrap=SAFE_NEW char[CLOUD_SIZE*CLOUD_SIZE*4];
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

void CAdvSky::Update()
{
	if(!dynamicSky)
		return;


	SCOPED_TIMER("Updating sky");

	CreateDetailTex();

	if(lastCloudUpdate<=gs->frameNum-3){
		lastCloudUpdate=gs->frameNum;

	//perlin noise matrices for the clouds
		int blendMatrix[8][32][32];
		for(int a=0;a<CLOUD_DETAIL;a++){
			float fade=(gs->frameNum/(70.0f*(2<<(CLOUD_DETAIL-1-a))));
			fade-=floor(fade/2)*2;
			if(fade>1){
				fade=2-fade;
				if(!cloudDown[a]){
					cloudDown[a]=true;
					CreateRandMatrix(randMatrix[a+8],1-a*0.03f);
				}
			} else {
				if(cloudDown[a]){
					cloudDown[a]=false;
					CreateRandMatrix(randMatrix[a],1-a*0.03f);
				}
			}
			int ifade=(int) ((3*fade*fade-2*fade*fade*fade)*256);

			for(int y=0;y<32;y++){
				for(int x=0;x<32;x++){
					blendMatrix[a][y][x]=(randMatrix[a][y][x]*ifade+randMatrix[a+8][y][x]*(256-ifade))>>8;
				}
			}
		}

		//create the raw clouds from the perlin noice octaves
		int (*rawClouds)[CLOUD_SIZE]=SAFE_NEW int[CLOUD_SIZE][CLOUD_SIZE];

		for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++){
			rawClouds[0][a]=0;
		}

		int kernel[CLOUD_SIZE/4*CLOUD_SIZE/4];
		for(int a=0;a<CLOUD_DETAIL;a++){
			for(int y=0;y<(CLOUD_SIZE/4)>>a;++y){
				float ydist=fabs((float)1+y-((CLOUD_SIZE/8)>>a))/((CLOUD_SIZE/8)>>a);
				ydist=3*ydist*ydist-2*ydist*ydist*ydist;
				for(int x=0;x<(CLOUD_SIZE/4)>>a;++x){
					float xdist=fabs((float)1+x-((CLOUD_SIZE/8)>>a))/((CLOUD_SIZE/8)>>a);
					xdist=3*xdist*xdist-2*xdist*xdist*xdist;

					float contrib=(1-xdist)*(1-ydist);
					kernel[y*CLOUD_SIZE/4+x]=(int) (contrib*((4<<CLOUD_DETAIL)>>a));
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

		for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++){
			cloudThickness2[a]=alphaTransform[rawClouds[0][a]>>7];
		}
		cloudThickness2[CLOUD_SIZE*CLOUD_SIZE]=cloudThickness2[CLOUD_SIZE*CLOUD_SIZE-1];	//this one is read in one place, so to avoid reading uninitialized mem ...

		//create the cloud shading
		int ydif[CLOUD_SIZE];
		for(int a=0;a<CLOUD_SIZE;++a){
			ydif[a]=0;
			ydif[a]+=cloudThickness2[(a+3*CLOUD_SIZE)];
			ydif[a]+=cloudThickness2[(a+2*CLOUD_SIZE)];
			ydif[a]+=cloudThickness2[(a+1*CLOUD_SIZE)];
			ydif[a]+=cloudThickness2[(a+0*CLOUD_SIZE)];
			ydif[a]-=cloudThickness2[(a-1*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)];
			ydif[a]-=cloudThickness2[(a-2*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)];
			ydif[a]-=cloudThickness2[(a-3*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)];
		}

		int b=0;
		ydif[(b)&255]+=cloudThickness2[(b-3*CLOUD_SIZE+CLOUD_SIZE*CLOUD_SIZE)];
		ydif[(b)&255]-=cloudThickness2[(b)]*2;
		ydif[(b)&255]+=cloudThickness2[(b+4*CLOUD_SIZE)];

		for(int a=0;a<CLOUD_SIZE*3-1;a++){
			ydif[(a+1)&255]+=cloudThickness2[(a-3*CLOUD_SIZE+1+CLOUD_SIZE*CLOUD_SIZE)];
			ydif[(a+1)&255]-=cloudThickness2[(a+1)]*2;
			ydif[(a+1)&255]+=cloudThickness2[(a+4*CLOUD_SIZE+1)];

			int dif=0;

			dif+=ydif[(a)&255];
			dif+=ydif[(a+1)&255]>>1;
			dif+=ydif[(a-1)&255]>>1;
			dif+=ydif[(a+2)&255]>>2;
			dif+=ydif[(a-2)&255]>>2;
			dif/=16;

			cloudTexMem[a*4+0]=128+dif;
			cloudTexMem[a*4+1]=thicknessTransform[rawClouds[0][a]>>7];
			cloudTexMem[a*4+2]=255;
		}

		for(int a=CLOUD_SIZE*3-1;a<CLOUD_SIZE*CLOUD_SIZE-CLOUD_SIZE*4-1;a++){
			ydif[(a+1)&255]+=cloudThickness2[(a-3*CLOUD_SIZE+1)];
			ydif[(a+1)&255]-=cloudThickness2[(a+1)]*2;
			ydif[(a+1)&255]+=cloudThickness2[(a+4*CLOUD_SIZE+1)];

			int dif=0;

			dif+=ydif[(a)&255];
			dif+=ydif[(a+1)&255]>>1;
			dif+=ydif[(a-1)&255]>>1;
			dif+=ydif[(a+2)&255]>>2;
			dif+=ydif[(a-2)&255]>>2;
			dif/=16;

			cloudTexMem[a*4+0]=128+dif;
			cloudTexMem[a*4+1]=thicknessTransform[rawClouds[0][a]>>7];
			cloudTexMem[a*4+2]=255;
		}

		for(int a=CLOUD_SIZE*CLOUD_SIZE-CLOUD_SIZE*4-1;a<CLOUD_SIZE*CLOUD_SIZE;a++){
			ydif[(a+1)&255]+=cloudThickness2[(a-3*CLOUD_SIZE+1)];
			ydif[(a+1)&255]-=cloudThickness2[(a+1)]*2;
			ydif[(a+1)&255]+=cloudThickness2[(a+4*CLOUD_SIZE+1-CLOUD_SIZE*CLOUD_SIZE)];

			int dif=0;

			dif+=ydif[(a)&255];
			dif+=ydif[(a+1)&255]>>1;
			dif+=ydif[(a-1)&255]>>1;
			dif+=ydif[(a+2)&255]>>2;
			dif+=ydif[(a-2)&255]>>2;
			dif/=16;

			cloudTexMem[a*4+0]=128+dif;
			cloudTexMem[a*4+1]=thicknessTransform[rawClouds[0][a]>>7];
			cloudTexMem[a*4+2]=255;
		}

		int modDensity=(int) ((1-cloudDensity)*256);
		for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++){
			int f=(rawClouds[0][a]>>8)-modDensity;
			if(f<0)
				f=0;
			if(f>255)
				f=255;
			cloudTexMem[a*4+3]=f;
		}

		delete[] rawClouds;

		glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
		glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,CLOUD_SIZE, CLOUD_SIZE,GL_RGBA, GL_UNSIGNED_BYTE, cloudTexMem);
	}
}

void CAdvSky::CreateRandMatrix(int matrix[32][32],float mod)
{
	for(int y=0;y<32;y++){
		for(int x=0;x<32;x++){
			float r = ((float)( rand() )) / (float)RAND_MAX;
			matrix[y][x]=((int)(r * 255.0f));
		}
	}
}

void CAdvSky::CreateRandDetailMatrix(unsigned char* matrix,int size)
{
	for(int a=0;a<size*size;a++){
		float  r = ((float)( rand() )) / (float)RAND_MAX;
		matrix[a]=((int)(r * 255.0f));
	}
}

void CAdvSky::CreateTransformVectors()
{
	for(int a=0;a<1024;++a){
		float f=(1023.0f-(a+cloudDensity*1024-512))/1023.0f;
		float alpha=pow(f*2,3);
		if(alpha>1)
			alpha=1;
		alphaTransform[a]=(int) (alpha*255);

		float d=f*2;
		if(d>1)
			d=1;
		thicknessTransform[a]=(unsigned char) (128+d*64+alphaTransform[a]*255/(4*255));
	}
}

void CAdvSky::DrawSun()
{
	glPushMatrix();
	CMatrix44f m(camera->pos,sundir1,UpVector,sundir2);
	glMultMatrixf(m.m);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	unsigned char buf[128];
	glEnable(GL_TEXTURE_2D);

	float3 modCamera=sundir1*camera->pos.x+sundir2*camera->pos.z;

	float ymod=(sunTexCoordY-0.5f)*domeWidth*0.025f*256;
	float fy=ymod+modCamera.z*CLOUD_SIZE*0.000025f;
	int baseY=int(floor(fy))&CLOUD_MASK;
	fy-=floor(fy);
	float fx=gs->frameNum*0.00005f*CLOUD_SIZE+modCamera.x*CLOUD_SIZE*0.000025f;
	int baseX=int(floor(fx))&CLOUD_MASK;
	fx-=floor(fx);

	if(baseX!=oldCoverBaseX || baseY!=oldCoverBaseY){
		oldCoverBaseX=baseX;
		oldCoverBaseY=baseY;
		CreateCover(baseX,baseY,covers[0]);
		CreateCover(baseX+1,baseY,covers[1]);
		CreateCover(baseX,baseY+1,covers[2]);
		CreateCover(baseX+1,baseY+1,covers[3]);
	}

	float mid=0;
	for(int x=0;x<32;++x){
		float cx1=covers[0][x]*(1-fx)+covers[1][x]*fx;
		float cx2=covers[2][x]*(1-fx)+covers[3][x]*fx;

		float cover=cx1*(1-fy)+cx2*fy;
		if(cover>127.5f)
			cover=127.5f;
		mid+=cover;

		buf[x+32]=(unsigned char)(255-cover*2);
		buf[x+64]=(unsigned char)(128-cover);
	}
	mid*=1.0f/32;
	for(int x=0;x<32;++x){
		buf[x]=(unsigned char)(255-mid*2);
	}

	glBindTexture(GL_TEXTURE_2D, sunFlareTex);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,32,3,GL_LUMINANCE,GL_UNSIGNED_BYTE,buf);

	glColor4f(0.4f*sunColor.x,0.4f*sunColor.y,0.4f*sunColor.z,0.0f);
	glCallList(sunFlareList);

	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
}

void CAdvSky::SetCloudShadow(int texunit)
{
}

void CAdvSky::ResetCloudShadow(int texunit)
{
}

void CAdvSky::DrawShafts()
{
}


void CAdvSky::InitSun()
{
	unsigned char* mem=SAFE_NEW unsigned char[128*128*4];

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

	for(int y=0;y<4;++y){
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
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
//	gluBuild2DMipmaps(GL_TEXTURE_2D,1 ,32, 2, GL_ALPHA, GL_UNSIGNED_BYTE, mem);
	glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE ,32, 4,0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mem);

	delete[] mem;

	float3 ldir=modSunDir.cross(UpVector);
	ldir.Normalize();
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

			glTexCoord2f(x/256.0f,0.125f);
			glVertexf3(modSunDir*5+ldir*dx*0.0014f+udir*dy*0.0014f);
			glTexCoord2f(x/256.0f,0.875f);
			glVertexf3(modSunDir*5+ldir*dx*4+udir*dy*4);
		}
		glEnd();
		if (gu->drawFog) glEnable(GL_FOG);
	glEndList();
}

void CAdvSky::CreateCover(int baseX, int baseY, float *buf)
{
	static const int line[]={
		5, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5,
		5, 0, 1, 0, 2, 1, 3, 1, 4, 1, 5,
		5, 0, 1, 1, 2, 1, 3, 2, 4, 2, 5,
		4, 1, 1, 2, 2, 2, 3, 3, 4,
		4, 1, 1, 2, 2, 3, 3, 4, 4,
		4, 1, 1, 2, 2, 3, 2, 4, 3,
		5, 1, 0, 2, 1, 3, 1, 4, 2, 5, 2,
		5, 1, 0, 2, 0, 3, 1, 4, 1, 5, 1
	};
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
			int incy = ((baseY+dy) & CLOUD_MASK) * CLOUD_SIZE;
			int decy = ((baseY-dy) & CLOUD_MASK) * CLOUD_SIZE;
			int incx = (baseX+dx) & CLOUD_MASK;
			int decx = (baseX-dx) & CLOUD_MASK;

			cover1+=255-cloudThickness2[incy+decx];//*(num-x);
			cover2+=255-cloudThickness2[decy+decx];//*(num-x);
			cover3+=255-cloudThickness2[decy+incx];//*(num-x);
			cover4+=255-cloudThickness2[incy+incx];//*(num-x);
			total+=1;//(num-x);
		}

		buf[l]=cover1/total;
		buf[l+8]=cover2/total;
		buf[l+16]=cover3/total;
		buf[l+24]=cover4/total;
	}
}

void CAdvSky::CreateDetailTex(void)
{
	glViewport(0,0,256,256);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glDisable(GL_DEPTH_TEST);

	unsigned char randDetailMatrix[32*32];

	for(int a=0;a<5;++a){
		float fade=(gs->frameNum/float(30<<a));
		fade-=floor(fade/2)*2;
		int size=min(32,256>>a);

		if(fade>1){
			fade=2-fade;
			if(!cloudDetailDown[a]){
				cloudDetailDown[a]=true;
				CreateRandDetailMatrix(randDetailMatrix,size);
				glBindTexture(GL_TEXTURE_2D, detailTextures[a+6]);
				glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,size, size, GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);
			}
		} else {
			if(cloudDetailDown[a]){
				cloudDetailDown[a]=false;
				CreateRandDetailMatrix(randDetailMatrix,size);
				glBindTexture(GL_TEXTURE_2D, detailTextures[a]);
				glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,size, size, GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);
			}

		}
		float tSize=max(1,8>>a);
		float c=pow(2.0f,a)*6/255.0f;
//		logOutput.Print("%f",c);
		CVertexArray* va=GetVertexArray();
		va->Initialize();

		va->AddVertexT(ZeroVector,0,0);
		va->AddVertexT(float3(1,0,0),tSize,0);
		va->AddVertexT(float3(1,1,0),tSize,tSize);
		va->AddVertexT(UpVector,0,tSize);

		float ifade=(3*fade*fade-2*fade*fade*fade);

		glBindTexture(GL_TEXTURE_2D, detailTextures[a+6]);
		glColor4f(c,c,c,1-ifade);
		va->DrawArrayT(GL_QUADS);
		glBindTexture(GL_TEXTURE_2D, detailTextures[a]);
		glColor4f(c,c,c,ifade);
		va->DrawArrayT(GL_QUADS);
	}
/*	unsigned char buf[256*256*4];
	glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,buf);
	CBitmap bm(buf,256,256);
	bm.Save("dc.bmp");
*/
	glBindTexture(GL_TEXTURE_2D, cdtex);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,256,256);
//	SwapBuffers(hDC);
//	SleepEx(500,true);

	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
	glEnable(GL_DEPTH_TEST);
}

float3 CAdvSky::GetDirFromTexCoord(float x, float y)
{
	float3 dir;

	dir.x=(x-0.5f)*domeWidth;
	dir.z=(y-0.5f)*domeWidth;

	float hdist=sqrt(dir.x*dir.x+dir.z*dir.z);
	float fy=asin(hdist/400);
	dir.y=(cos(fy)-domeheight)*400;

	dir.Normalize();

	return dir;
}

//should be improved
//only take stuff in yz plane
float CAdvSky::GetTexCoordFromDir(float3 dir)
{
	float tp=0.5f;
	float step=0.25f;

	for(int a=0;a<10;++a){
		float tx=0.5f+tp;
		float3 d=GetDirFromTexCoord(tx,0.5f);
		if(d.y<dir.y)
			tp-=step;
		else
			tp+=step;
		step*=0.5f;
	}
	return 0.5f+tp;
}
