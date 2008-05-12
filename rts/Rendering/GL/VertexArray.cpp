// VertexArray.cpp: implementation of the CVertexArrayRange class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "VertexArray.h"
#include "myGL.h"
#include "mmgr.h"
#include <GL/glew.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArray::CVertexArray()
{
	drawArray=SAFE_NEW float[1000];
	stripArray=SAFE_NEW int[100];

	drawArraySize=1000;
	stripArraySize=100;
}

CVertexArray::~CVertexArray()
{
	delete[] drawArray;
	delete[] stripArray;
}

void CVertexArray::Initialize()
{
	stripIndex=0;
	drawIndex=0;
}

bool CVertexArray::IsReady()
{
	return true;
}

void CVertexArray::EndStrip()
{
	if(stripIndex>stripArraySize-4)
		EnlargeStripArray();

	stripArray[stripIndex++]=drawIndex;
}

void CVertexArray::DrawArray0(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();

	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glEnableClientState(GL_VERTEX_ARRAY);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayC(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,&drawArray[3]);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glDisableClientState(GL_VERTEX_ARRAY);	
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::DrawArrayT(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayT2(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
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

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayTN(int drawType, int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	glNormalPointer(GL_FLOAT,stride,&drawArray[5]);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void CVertexArray::DrawArrayTC(int drawType, int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glTexCoordPointer(2,GL_FLOAT,stride,&drawArray[3]);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,&drawArray[5]);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::EnlargeDrawArray()
{
	float* tempArray=SAFE_NEW float[drawArraySize*2];
	for(int a=0;a<drawIndex;++a)
		tempArray[a]=drawArray[a];

	drawArraySize*=2;
	delete[] drawArray;
	drawArray=tempArray;
}

void CVertexArray::EnlargeStripArray()
{
	int* tempArray=SAFE_NEW int[stripArraySize*2];
	for(int a=0;a<stripIndex;++a)
		tempArray[a]=stripArray[a];

	stripArraySize*=2;
	delete[] stripArray;
	stripArray=tempArray;
}
