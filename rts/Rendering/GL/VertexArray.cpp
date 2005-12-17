// VertexArray.cpp: implementation of the CVertexArrayRange class.
//
//////////////////////////////////////////////////////////////////////

#include "System/StdAfx.h"
#include "VertexArray.h"
#include "myGL.h"
//#include "System/mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArray::CVertexArray()
{
	drawArray=new float[1000];
	stripArray=new int[100];

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

void CVertexArray::AddVertex0(const float3& pos)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
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

void CVertexArray::AddVertexC(const float3& pos,unsigned char* color)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=*((float*)(color));
}

void CVertexArray::DrawArrayC(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
	glVertexPointer(3,GL_FLOAT,stride,&drawArray[0]);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,&drawArray[3]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	int oldIndex=0;
	for(int a=0;a<stripIndex;a++){
		glDrawArrays(drawType,oldIndex*4/stride,stripArray[a]*4/stride-oldIndex*4/stride);
		oldIndex=stripArray[a];
	}
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::AddVertexT(const float3& pos,float tx,float ty)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
}

void CVertexArray::DrawArrayT(int drawType,int stride)
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
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
}

void CVertexArray::AddVertexT2(const float3& pos,float tx,float ty,float t2x,float t2y)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
	drawArray[drawIndex++]=t2x;
	drawArray[drawIndex++]=t2y;
}

void CVertexArray::DrawArrayT2(int drawType,int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
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

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
}

void CVertexArray::AddVertexTN(const float3& pos,float tx,float ty, const float3& norm)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
	drawArray[drawIndex++]=norm.x;
	drawArray[drawIndex++]=norm.y;
	drawArray[drawIndex++]=norm.z;
}

void CVertexArray::DrawArrayTN(int drawType, int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
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
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_NORMAL_ARRAY);
}

void CVertexArray::AddVertexTC(const float3& pos,float tx,float ty,unsigned char* col)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
	drawArray[drawIndex++]=*((float*)(col));
}

void CVertexArray::DrawArrayTC(int drawType, int stride)
{
	if(stripIndex==0 || stripArray[stripIndex-1]!=drawIndex)
		EndStrip();
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
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::EnlargeDrawArray()
{
	float* tempArray=new float[drawArraySize*2];
	for(int a=0;a<drawIndex;++a)
		tempArray[a]=drawArray[a];

	drawArraySize*=2;
	delete[] drawArray;
	drawArray=tempArray;
}

void CVertexArray::EnlargeStripArray()
{
	int* tempArray=new int[stripArraySize*2];
	for(int a=0;a<stripIndex;++a)
		tempArray[a]=stripArray[a];

	stripArraySize*=2;
	delete[] stripArray;
	stripArray=tempArray;
}
