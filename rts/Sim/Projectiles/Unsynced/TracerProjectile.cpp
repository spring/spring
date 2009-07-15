// TracerProjectile.cpp: implementation of the CTracerProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "TracerProjectile.h"
#include "Rendering/GL/myGL.h"			// Header File For The OpenGL32 Library
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CTracerProjectile, CProjectile, )

CR_REG_METADATA(CTracerProjectile, 
(
	CR_MEMBER(speedf),
	CR_MEMBER(drawLength),
	CR_MEMBER(dir),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(length),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTracerProjectile::CTracerProjectile(const float3 pos, const float3 speed, const float range, CUnit* owner GML_PARG_C):
	CProjectile(pos, speed, owner, false, false, false GML_PARG_P)
{
	SetRadius(1);
	speedf=this->speed.Length();
	dir=this->speed/speedf;
	length=range;
	drawLength=0;
	checkCol=false;
}

CTracerProjectile::CTracerProjectile()
{
	speedf=0.0f;
	length=drawLength=0.0f;
	checkCol=false;
}

void CTracerProjectile::Init(const float3& pos, CUnit* owner GML_PARG_C)
{
	speedf=this->speed.Length();
	if (speedf==0.0f) speed=float3(1.0f,0.0f,0.0f);
	dir=this->speed/speedf;

	CProjectile::Init (pos, owner GML_PARG_P);
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
	if(drawLength>3)
		drawLength=3;

	glTexCoord2f(1.0f/16,1.0f/16);
	glColor4f(1,1,0.1f,0.4f);
	glBegin(GL_LINES);
		glVertexf3( drawPos);				
		glVertexf3( drawPos-dir*drawLength);				
	glEnd();
	glColor4f(1,1,1,1);
	glTexCoord2f(0,0);
}
