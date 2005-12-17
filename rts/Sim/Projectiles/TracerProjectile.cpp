// TracerProjectile.cpp: implementation of the CTracerProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "System/StdAfx.h"
#include "TracerProjectile.h"
#include "Rendering/GL/myGL.h"			// Header File For The OpenGL32 Library
#include <math.h>
#include "ProjectileHandler.h"
//#include "System/mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTracerProjectile::CTracerProjectile(const float3 pos, const float3 speed,const float range,CUnit* owner)
: CProjectile(pos,speed,owner)
{
	SetRadius(1);
	speedf=this->speed.Length();
	dir=this->speed/speedf;
	length=range;
	drawLength=0;
	checkCol=false;
}

CTracerProjectile::~CTracerProjectile()
{

}

void CTracerProjectile::Update()
{
	pos+=speed;

	drawLength+=speedf;
	length-=speedf;
	if(length<0)
		deleteMe=true;
}

void CTracerProjectile::Draw()
{
	if(inArray)
		DrawArray();

	if(drawLength>3)
		drawLength=3;

	float3 interPos=pos+speed*gu->timeOffset;
	glTexCoord2f(1.0/16,1.0/16);
	glColor4f(1,1,0.1f,0.4f);
	glBegin(GL_LINES);
		glVertexf3( interPos);				
		glVertexf3( interPos-dir*drawLength);				
	glEnd();
}
