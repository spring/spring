#include "StdAfx.h"
// DrawWater.cpp: implementation of the CBasicWater class.
//
//////////////////////////////////////////////////////////////////////

#include "BasicWater.h"
#include <windows.h>
#include "myGL.h"
#include <GL/glu.h>
#include <math.h>

#include "Bitmap.h"
#include "ReadMap.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBasicWater::CBasicWater()
{
	glGenTextures(1, &texture);
	CBitmap pic("bitmaps\\ocean.jpg");

	// create mipmapped texture
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,pic.xsize, pic.ysize, GL_RGBA, GL_UNSIGNED_BYTE, pic.mem);
	
	displist=0;
}

CBasicWater::~CBasicWater()
{
	glDeleteTextures (1, &texture);
}

void CBasicWater::Draw()
{
	if(readmap->minheight>10)
		return;
	if(displist==0){
		displist=glGenLists(1);
		glNewList(displist, GL_COMPILE);
		float height=0;//sin(gs->frameNum/40.0f)/4;
		glDisable(GL_ALPHA_TEST);
		glDepthMask(0);
		glColor4f(0.7f,0.7f,0.7f,0.5f);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);
		float mapSizeX=gs->mapx*SQUARE_SIZE;
		float mapSizeY=gs->mapy*SQUARE_SIZE;
		for(int y=0;y<16;y++){
			for(int x=0;x<16;x++){
				glTexCoord2f(x*0.5f,y*0.5f);
				glVertex3f(x*mapSizeX/16,height,y*mapSizeY/16);
				glTexCoord2f(x*0.5f,(y+1)*0.5f);
				glVertex3f(x*mapSizeX/16,height,(y+1)*mapSizeY/16);
				glTexCoord2f((x+1)*0.5f,(y+1)*0.5f);
				glVertex3f((x+1)*mapSizeX/16,height,(y+1)*mapSizeY/16);
				glTexCoord2f((x+1)*0.5f,y*0.5f);
				glVertex3f((x+1)*mapSizeX/16,height,y*mapSizeY/16);
			}
		}
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glDepthMask(1);
		glEndList();
	}
	glCallList(displist);
}
