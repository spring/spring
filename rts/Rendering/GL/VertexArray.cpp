// VertexArray.cpp: implementation of the CVertexArrayRange class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "VertexArray.h"
#include "myGL.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArray::CVertexArray() {
	drawArray=SAFE_NEW float[1000];
	stripArray=SAFE_NEW int[100];
	Initialize();
	drawArraySize=drawArray+1000;
	stripArraySize=stripArray+100;
}

CVertexArray::~CVertexArray() {
	delete [] drawArray;
	delete [] stripArray;
}

void CVertexArray::Initialize() {
	drawArrayPos=drawArray;
	stripArrayPos=stripArray;
}

int CVertexArray::drawIndex() {
	return drawArrayPos-drawArray;
}

bool CVertexArray::IsReady() {
	return true;
}

void CVertexArray::EndStrip() {
	if((char *)stripArrayPos>(char *)stripArraySize-4*sizeof(int))
		EnlargeStripArray();

	*stripArrayPos++=((char *)drawArrayPos-(char *)drawArray);
}

void CVertexArray::EnlargeArrays(int vertexes, int strips, int stripsize) {
	while((char *)drawArrayPos>(char *)drawArraySize-stripsize*sizeof(float)*vertexes)
		EnlargeDrawArray();

	while((char *)stripArrayPos>(char *)stripArraySize-sizeof(int)*strips)
		EnlargeStripArray();
}

void CVertexArray::EndStripQ() {
	*stripArrayPos++=((char *)drawArrayPos-(char *)drawArray);
}

void CVertexArray::DrawArrays(int drawType, int stride) {
	int newIndex,oldIndex=0;
	int *stripArrayPtr=stripArray;
	while(stripArrayPtr<stripArrayPos) {
		newIndex=(*stripArrayPtr++)/stride;
		glDrawArrays(drawType,oldIndex,newIndex-oldIndex);
		oldIndex=newIndex;
	}
}

void CVertexArray::DrawArray0(int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);                                          
}

void CVertexArray::DrawArrayC(int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,drawArray+3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);                                          
	glDisableClientState(GL_COLOR_ARRAY);
}

void CVertexArray::DrawArrayT(int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,stride,drawArray);
	glTexCoordPointer(2,GL_FLOAT,stride,drawArray+3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);                                          
}

void CVertexArray::DrawArrayT2(int drawType,int stride) {
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

void CVertexArray::DrawArrayTN(int drawType, int stride) {
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

void CVertexArray::DrawArrayTC(int drawType, int stride) {
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
	float* tempArray=SAFE_NEW float[newsize];
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

	int* tempArray=SAFE_NEW int[newsize];
	memcpy(tempArray,stripArray,oldsize*sizeof(int));

	delete [] stripArray;
	stripArray=tempArray;
	stripArraySize=stripArray+newsize;
	stripArrayPos=stripArray+pos;
}
