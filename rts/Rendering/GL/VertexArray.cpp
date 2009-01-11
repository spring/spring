// VertexArray.cpp: implementation of the CVertexArrayRange class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <cstring>
#include "mmgr.h"

#include "VertexArray.h"
#include "myGL.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArray::CVertexArray() {
	drawArray=new float[VA_INIT_VERTEXES]; // please don't change this, some files rely on specific initial sizes
	stripArray=new int[VA_INIT_STRIPS];
	Initialize();
	drawArraySize=drawArray+VA_INIT_VERTEXES;
	stripArraySize=stripArray+VA_INIT_STRIPS;
}

CVertexArray::~CVertexArray() {
	delete [] drawArray;
	delete [] stripArray;
}

void CVertexArray::Initialize() {
	drawArrayPos=drawArray;
	stripArrayPos=stripArray;
}

bool CVertexArray::IsReady() {
	return true;
}

void CVertexArray::EndStrip() {
	if((char *)stripArrayPos>(char *)stripArraySize-4*sizeof(int))
		EnlargeStripArray();

	*stripArrayPos++=((char *)drawArrayPos-(char *)drawArray);
}

void CVertexArray::DrawArray0(const int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayC(const int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,drawArray+3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::DrawArrayT(const int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glTexCoordPointer(2,GL_FLOAT,stride,drawArray+3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayT2(const int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glTexCoordPointer(2,GL_FLOAT,stride,drawArray+3);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2,GL_FLOAT,stride,drawArray+5);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	DrawArrays(drawType, stride);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayTN(const int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glTexCoordPointer(2,GL_FLOAT,stride,drawArray+3);
	glNormalPointer(GL_FLOAT,stride,drawArray+5);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void CVertexArray::DrawArrayTC(const int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glTexCoordPointer(2,GL_FLOAT,stride,drawArray+3);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,drawArray+5);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::EnlargeDrawArray() {
	int pos=drawArrayPos-drawArray;
	int oldsize=drawArraySize-drawArray;
	int newsize=oldsize*2;
	float* tempArray=new float[newsize];
	memcpy(tempArray,drawArray,oldsize*sizeof(float));

	delete [] drawArray;
	drawArray=tempArray;
	drawArraySize=drawArray+newsize;
	drawArrayPos=drawArray+pos;
}

void CVertexArray::EnlargeStripArray() {
	int pos=stripArrayPos-stripArray;
	int oldsize=stripArraySize-stripArray;
	int newsize=oldsize*2;

	int* tempArray=new int[newsize];
	memcpy(tempArray,stripArray,oldsize*sizeof(int));

	delete [] stripArray;
	stripArray=tempArray;
	stripArraySize=stripArray+newsize;
	stripArrayPos=stripArray+pos;
}
