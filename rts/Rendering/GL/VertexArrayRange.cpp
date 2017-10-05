/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "VertexArrayRange.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArrayRange::CVertexArrayRange(float* mem,int size) {
	delete[] drawArray;

	drawArray = mem;
	drawArraySize = mem + size;

	glGenFencesNV(1, &fence);
	glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

CVertexArrayRange::~CVertexArrayRange() {
	glDeleteFencesNV(1, &fence);
}

void CVertexArrayRange::Initialize() {
	CVertexArray::Initialize();

	glFinishFenceNV(fence);
}

bool CVertexArrayRange::IsReady() const {
	return !!glTestFenceNV(fence);
}

void CVertexArrayRange::DrawArrayT(int drawType,int stride) {
	CheckEndStrip();
	glVertexPointer(3, GL_FLOAT, stride, &drawArray[0]);
	glTexCoordPointer(2, GL_FLOAT, stride, &drawArray[3]);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	DrawArrays(drawType, stride);

	glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
}

void CVertexArrayRange::DrawArrayT2(int drawType,int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3, GL_FLOAT, stride, &drawArray[0]);
	glTexCoordPointer(2, GL_FLOAT, stride, &drawArray[3]);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FLOAT, stride, &drawArray[5]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);

	DrawArrays(drawType, stride);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);

	glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

void CVertexArrayRange::DrawArrayTN(int drawType, int stride) {
	CheckEndStrip();
//  glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3, GL_FLOAT, stride, &drawArray[0]);
	glTexCoordPointer(2, GL_FLOAT, stride, &drawArray[3]);
	glNormalPointer(GL_FLOAT, stride, &drawArray[5]);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	DrawArrays(drawType, stride);

	glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_NORMAL_ARRAY);
//  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

void CVertexArrayRange::DrawArrayTC(int drawType, int stride) {
	CheckEndStrip();
//  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3, GL_FLOAT, stride, &drawArray[0]);
	glTexCoordPointer(2, GL_FLOAT, stride, &drawArray[3]);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, &drawArray[5]);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	DrawArrays(drawType, stride);

	glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);						
	glDisableClientState(GL_COLOR_ARRAY);
//  glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}

void CVertexArrayRange::EnlargeDrawArray() {
	drawArrayPos -= 40; // WTF?
}

