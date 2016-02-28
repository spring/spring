/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>

#include "VertexArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVertexArray::CVertexArray(unsigned int maxVerts): maxVertices(maxVerts)
{
	drawArray = new float[VA_INIT_VERTEXES]; // please don't change this, some files rely on specific initial sizes
	stripArray = new unsigned int[VA_INIT_STRIPS];
	Initialize();
	drawArraySize = drawArray + VA_INIT_VERTEXES;
	stripArraySize = stripArray + VA_INIT_STRIPS;
}

CVertexArray::~CVertexArray()
{
	delete[] drawArray;
	delete[] stripArray;
	drawArray = nullptr;
	stripArray = nullptr;

	drawArrayPos = nullptr;
	drawArraySize = nullptr;

	stripArrayPos = nullptr;
	stripArraySize = nullptr;
}


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

void CVertexArray::Initialize()
{
	drawArrayPos  = drawArray;
	stripArrayPos = stripArray;
}

bool CVertexArray::IsReady() const
{
	return true;
}

void CVertexArray::EndStrip()
{
	if ((char*)stripArrayPos > ((char*)stripArraySize - 4 * sizeof(unsigned int))) {
		EnlargeStripArray();
	}

	*stripArrayPos++ = ((char*)drawArrayPos - (char*)drawArray);
}


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////


static inline bool IsPrimitiveSplitable(GLenum mode)
{
	switch (mode) {
		case GL_TRIANGLE_FAN:
		case GL_POLYGON:
		case GL_LINE_LOOP:
			{ return false; }
		default:
			{ return true; }
	}
}


static inline int GetPrimitiveRestartEach(GLenum mode)
{
	switch (mode) {
		case GL_TRIANGLE_STRIP:
			{ return 2; }
		case GL_QUAD_STRIP:
			{ return 2; }
		case GL_TRIANGLES:
			{ return 3; }
		case GL_QUADS:
			{ return 4; }
		case GL_LINES:
			{ return 2; }
		default:
			{ return 1; }
	}
}


static inline int GetStripStartOffset(GLenum mode)
{
	switch (mode) {
		case GL_TRIANGLES:
		case GL_QUAD_STRIP:
			{ return 2; }
		case GL_LINE_STRIP:
			{ return 1; }
		default:
			{ return 0; }
	}
}


void CVertexArray::DrawArrays(const GLenum mode, const unsigned int stride)
{
	unsigned int length;
	unsigned int newIndex, oldIndex = 0;
	const unsigned int* stripArrayPtr = stripArray;

	if (!IsPrimitiveSplitable(mode)) {
		//! we can't split this primitive types
		while (stripArrayPtr < stripArrayPos) {
			newIndex = (*stripArrayPtr++) / stride;
			length = newIndex - oldIndex;
			glDrawArrays(mode, oldIndex, length);
			oldIndex = newIndex;
		}
	} else {
		//! split the va in optimal strip sizes, to increase the performance
		const int optVertCount = maxVertices - (maxVertices % GetPrimitiveRestartEach(mode));
		const int stripOffset  = GetStripStartOffset(mode);

		while (stripArrayPtr < stripArrayPos) {
			newIndex=(*stripArrayPtr++) / stride;
			length = newIndex - oldIndex;

			if (length > 1.5f * optVertCount) {
				int spliti = length / optVertCount;
				do {
					glDrawArrays(mode, oldIndex, optVertCount);
					oldIndex += optVertCount - stripOffset;
				} while (--spliti > 0);

				if (newIndex > oldIndex)
					glDrawArrays(mode, oldIndex, newIndex - oldIndex);
			} else {
				glDrawArrays(mode, oldIndex, length);
			}
			oldIndex = newIndex;
		}
	}
}


void CVertexArray::DrawArraysCallback(const GLenum mode, const unsigned int stride, StripCallback callback, void* data)
{
	unsigned int length;
	unsigned int newIndex, oldIndex = 0;
	const unsigned int* stripArrayPtr = stripArray;

	if (!IsPrimitiveSplitable(mode)) {
		//! we can't split those primitive types
		while (stripArrayPtr < stripArrayPos) {
			callback(data);
			newIndex = (*stripArrayPtr++)/stride;
			length = newIndex - oldIndex;
			glDrawArrays(mode, oldIndex, length);
			oldIndex = newIndex;
		}
	} else {
		//! split the va in optimal strip sizes, to increase the performance
		const int optVertCount = maxVertices - (maxVertices % GetPrimitiveRestartEach(mode));
		const int stripOffset  = GetStripStartOffset(mode);

		while (stripArrayPtr < stripArrayPos) {
			callback(data);
			newIndex = (*stripArrayPtr++) / stride;
			length = newIndex - oldIndex;

			if (length > 1.25f * optVertCount) {
				int spliti = length / optVertCount;
				do {
					glDrawArrays(mode, oldIndex, optVertCount);
					oldIndex += optVertCount - stripOffset;
				} while(--spliti>0);

				if (newIndex > oldIndex)
					glDrawArrays(mode, oldIndex, newIndex - oldIndex);
			} else {
				glDrawArrays(mode, oldIndex, length);
			}
			oldIndex=newIndex;
		}
	}
}




//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

void CVertexArray::DrawArray0(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArray2d0(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, stride, drawArray);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void CVertexArray::DrawArrayN(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glNormalPointer(GL_FLOAT, stride, drawArray + 3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}


void CVertexArray::DrawArrayC(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, drawArray + 3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}


void CVertexArray::DrawArrayT(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void CVertexArray::DrawArray2dT(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 2);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void CVertexArray::DrawArray2dTC(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 2);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, drawArray + 4);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}


void CVertexArray::DrawArray2dT(const int drawType, StripCallback callback, void* data, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 2);
	DrawArraysCallback(drawType, stride, callback, data);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void CVertexArray::DrawArrayTN(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);
	glNormalPointer(GL_FLOAT, stride, drawArray + 5);
	DrawArrays(drawType, stride);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void CVertexArray::DrawArrayTNT(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	#define SET_ENABLE_ACTIVE_TEX(texUnit)            \
		glClientActiveTexture(texUnit);               \
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	#define SET_DISABLE_ACTIVE_TEX(texUnit)           \
		glClientActiveTexture(texUnit);               \
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	SET_ENABLE_ACTIVE_TEX(GL_TEXTURE0); glTexCoordPointer(2, GL_FLOAT, stride, drawArray +  3);
	SET_ENABLE_ACTIVE_TEX(GL_TEXTURE1); glTexCoordPointer(2, GL_FLOAT, stride, drawArray +  3); // FIXME? (format-specific)
	SET_ENABLE_ACTIVE_TEX(GL_TEXTURE5); glTexCoordPointer(3, GL_FLOAT, stride, drawArray +  8);
	SET_ENABLE_ACTIVE_TEX(GL_TEXTURE6); glTexCoordPointer(3, GL_FLOAT, stride, drawArray + 11);

	glVertexPointer(3, GL_FLOAT, stride, drawArray + 0);
	glNormalPointer(GL_FLOAT, stride, drawArray + 5);

	DrawArrays(drawType, stride);

	SET_DISABLE_ACTIVE_TEX(GL_TEXTURE6);
	SET_DISABLE_ACTIVE_TEX(GL_TEXTURE5);
	SET_DISABLE_ACTIVE_TEX(GL_TEXTURE1);
	SET_DISABLE_ACTIVE_TEX(GL_TEXTURE0);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	#undef SET_ENABLE_ACTIVE_TEX
	#undef SET_DISABLE_ACTIVE_TEX
}


void CVertexArray::DrawArrayTC(const int drawType, unsigned int stride)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, drawArray + 5);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}




//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

void CVertexArray::EnlargeDrawArray()
{
	const unsigned int pos = drawArrayPos - drawArray;
	const unsigned int oldsize = drawArraySize - drawArray;
	const unsigned int newsize = oldsize * 2;

	float* tempArray = new float[newsize];
	memcpy(tempArray, drawArray, oldsize * sizeof(float));

	delete[] drawArray;

	drawArray = tempArray;
	drawArraySize = drawArray + newsize;
	drawArrayPos = drawArray + pos;
}

void CVertexArray::EnlargeStripArray()
{
	const unsigned int pos = stripArrayPos - stripArray;
	const unsigned int oldsize = stripArraySize - stripArray;
	const unsigned int newsize = oldsize * 2;

	unsigned int* tempArray = new unsigned int[newsize];
	memcpy(tempArray, stripArray, oldsize * sizeof(unsigned int));

	delete[] stripArray;

	stripArray = tempArray;
	stripArraySize = stripArray + newsize;
	stripArrayPos = stripArray + pos;
}
