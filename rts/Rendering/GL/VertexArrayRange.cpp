// VertexArrayRange.cpp: implementation of the CVertexArrayRange class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "VertexArrayRange.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArrayRange::CVertexArrayRange(float* mem,int size)
{
	delete[] drawArray;

	drawArray=mem;
	drawArraySize=size;

  glGenFencesNV(1, &fence);
  glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

CVertexArrayRange::~CVertexArrayRange()
{
	glDeleteFencesNV(1,&fence);
}

void CVertexArrayRange::Initialize()
{
	stripIndex=0;
	drawIndex=0;

	glFinishFenceNV(fence);
}

bool CVertexArrayRange::IsReady()
{
	return !!glTestFenceNV(fence);
}

void CVertexArrayRange::DrawArrayT(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
  glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
}

void CVertexArrayRange::DrawArrayT2(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
  glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[5]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

void CVertexArrayRange::DrawArrayTN(int drawType, int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
//  glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	glNormalPointer(GL_FLOAT,stride,&drawArray[5]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
  glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_NORMAL_ARRAY);
//  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

void CVertexArrayRange::DrawArrayTC(int drawType, int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
//  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,&drawArray[5]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
  glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_COLOR_ARRAY);
//  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

void CVertexArrayRange::EnlargeDrawArray()
{
	drawIndex-=40;
}
