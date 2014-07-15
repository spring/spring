/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VertexArray.h"
#include "Rendering/GlobalRendering.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

#ifndef VERTEXARRAY_H

//////////////////////////////////////////////////////////////////////
/// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStreamBuffer::CStreamBuffer()
: VBO(GL_ARRAY_BUFFER, true)
, CRangeLocker(true)
, curPos(0)
, mem(nullptr)
, bufOffset(0)
, bufSize(0)
, vao0(0)
, vaoC(0)
, vaoT(0)
, vaoTC(0)
, vaoTN(0)
, vao2d0(0)
, vao2dT(0)
, vao2dTC(0)
{
	VBO::Bind(GL_ARRAY_BUFFER);
	VBO::Resize(VA_INIT_VERTEXES, GL_STREAM_DRAW);
	mem = reinterpret_cast<char*>(VBO::MapBuffer());
	VBO::Unbind();

	//FIXME VAOs fail atm (render nothing and/or garbage) :(
	//GenVAOs();
}


CStreamBuffer::~CStreamBuffer()
{
	if (vao0) {
		glDeleteVertexArrays(1, &vao0);
		glDeleteVertexArrays(1, &vaoC);
		glDeleteVertexArrays(1, &vaoT);
		glDeleteVertexArrays(1, &vaoTC);
		glDeleteVertexArrays(1, &vaoTN);
		glDeleteVertexArrays(1, &vao2d0);
		glDeleteVertexArrays(1, &vao2dT);
		glDeleteVertexArrays(1, &vao2dTC);
	}
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::Initialize(size_t size, const size_t stride)
{
	SCOPED_TIMER("CStreamBuffer::Initialize");
	strips.clear();
	curPos = 0;

	bufOffset += bufSize;
	bufSize    = size * stride;
	bufOffset  = GetMem(bufSize, stride);
	assert((bufOffset % stride) == 0);
}


void CStreamBuffer::SetFence()
{
	if (!VBOused) return;
	SCOPED_TIMER("CStreamBuffer::SetFence");
	LockRange(bufOffset, bufSize);
}


size_t CStreamBuffer::GetMem(size_t size, size_t stride)
{
	if (size > (VBO::GetSize() / 4)) {
		LOG("CStreamBuffer::GetMem %i %i", int(size), int(VBO::GetSize()));
		//FIXME
	}

	// glDrawArrays only allows us to define start _index_ (in multiple of stride size)
	// so we need to align our memory to that
	bufOffset += (stride - (bufOffset % stride)) % stride;
	assert((bufOffset % stride) == 0);
	if ((bufOffset + size) > VBO::GetSize())
		bufOffset = 0;

	SCOPED_TIMER("CStreamBuffer::WaitForFence");
	WaitForLockedRange(bufOffset, bufSize);
	return bufOffset;
}


bool CStreamBuffer::EnlargeMem(size_t size)
{
	if ((bufOffset + bufSize + size) > VBO::GetSize())
		return false;

	bufSize += size;
	WaitForLockedRange(bufOffset, bufSize);
	return true;
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::EnlargeDrawArray(size_t _size)
{
	SCOPED_TIMER("CStreamBuffer::EnlargeDrawArray");
	if (_size == -1)
		_size = std::max<size_t>(bufSize, 1024);

	LOG("CStreamBuffer::EnlargeDrawArray %i %i", int(bufSize), int(bufSize + _size));
	assert(bufSize > 0);

	if (!EnlargeMem(_size)) {
		size_t oldBufSize   = bufSize;
		size_t oldBufOffset = bufOffset;

		bufSize  += _size;
		bufOffset = GetMem(bufSize, 1);
		assert(bufOffset == 0); // only 0 aligns with everything

		//FIXME what if mem ranges share space?
		memcpy(mem + bufOffset, mem + oldBufOffset, oldBufSize);
	}
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::CheckEndStrip()
{
	if (strips.empty() || strips.back() != drawIndex())
		EndStrip();
}


void CStreamBuffer::EndStrip()
{
	strips.push_back(drawIndex());
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::GenVAOs()
{
	if (!GLEW_ARB_vertex_array_object)
		return;

	glDeleteVertexArrays(1, &vao0);
	glDeleteVertexArrays(1, &vaoC);
	glDeleteVertexArrays(1, &vaoT);
	glDeleteVertexArrays(1, &vaoTC);
	glDeleteVertexArrays(1, &vaoTN);
	glDeleteVertexArrays(1, &vao2d0);
	glDeleteVertexArrays(1, &vao2dT);
	glDeleteVertexArrays(1, &vao2dTC);

	if (!VBOused) return;

	VBO::Bind(GL_ARRAY_BUFFER);

	glGenVertexArrays(1, &vao0);
	glBindVertexArray(vao0);
	glVertexPointer(  3, GL_FLOAT, sizeof(VA_TYPE_0), GetPtr(offsetof(VA_TYPE_0, p)));
	glEnableClientState(GL_VERTEX_ARRAY);

	glGenVertexArrays(1, &vaoC);
	glBindVertexArray(vaoC);
	glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(VA_TYPE_C), GetPtr(offsetof(VA_TYPE_C, c)));
	glVertexPointer(  3, GL_FLOAT,         sizeof(VA_TYPE_C), GetPtr(offsetof(VA_TYPE_C, p)));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glGenVertexArrays(1, &vaoT);
	glBindVertexArray(vaoT);
	glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_T), GetPtr(offsetof(VA_TYPE_T, s)));
	glVertexPointer(  3, GL_FLOAT, sizeof(VA_TYPE_T), GetPtr(offsetof(VA_TYPE_T, p)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glGenVertexArrays(1, &vaoTC);
	glBindVertexArray(vaoTC);
	glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(VA_TYPE_TC), GetPtr(offsetof(VA_TYPE_TC, c)));
	glTexCoordPointer(2, GL_FLOAT,         sizeof(VA_TYPE_TC), GetPtr(offsetof(VA_TYPE_TC, s)));
	glVertexPointer(  3, GL_FLOAT,         sizeof(VA_TYPE_TC), GetPtr(offsetof(VA_TYPE_TC, p)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glGenVertexArrays(1, &vaoTN);
	glBindVertexArray(vaoTN);
	glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_TN), GetPtr(offsetof(VA_TYPE_TN, s)));
	glNormalPointer(     GL_FLOAT, sizeof(VA_TYPE_TN), GetPtr(offsetof(VA_TYPE_TN, n)));
	glVertexPointer(  3, GL_FLOAT, sizeof(VA_TYPE_TN), GetPtr(offsetof(VA_TYPE_TN, p)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glGenVertexArrays(1, &vao2d0);
	glBindVertexArray(vao2d0);
	glVertexPointer(  2, GL_FLOAT, sizeof(VA_TYPE_2d0), GetPtr(offsetof(VA_TYPE_2d0, x)));
	glEnableClientState(GL_VERTEX_ARRAY);

	glGenVertexArrays(1, &vao2dT);
	glBindVertexArray(vao2dT);
	glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_2dT), GetPtr(offsetof(VA_TYPE_2dT, s)));
	glVertexPointer(  2, GL_FLOAT, sizeof(VA_TYPE_2dT), GetPtr(offsetof(VA_TYPE_2dT, x)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glGenVertexArrays(1, &vao2dTC);
	glBindVertexArray(vao2dTC);
	glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(VA_TYPE_2dTC), GetPtr(offsetof(VA_TYPE_2dTC, c)));
	glTexCoordPointer(2, GL_FLOAT,         sizeof(VA_TYPE_2dTC), GetPtr(offsetof(VA_TYPE_2dTC, s)));
	glVertexPointer(  2, GL_FLOAT,         sizeof(VA_TYPE_2dTC), GetPtr(offsetof(VA_TYPE_2dTC, x)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glBindVertexArray(0);
	VBO::Unbind();
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::DrawArrays(const GLenum mode, const size_t stride) const
{
	size_t length;
	size_t newIndex, oldIndex = 0;
	int i = 0;
	size_t startIdx = bufOffset / stride;
	assert((bufOffset % stride) == 0);

	while (i < strips.size()) {
		newIndex = strips[i] / stride;
		length = newIndex - oldIndex;
		if (length > 0) glDrawArrays(mode, startIdx + oldIndex, length);
		oldIndex = newIndex;
		++i;
	}
}


void CStreamBuffer::DrawArraysCallback(const GLenum mode, const size_t stride, StripCallback callback, void* data) const
{
	size_t length;
	size_t newIndex, oldIndex = 0;
	int i = 0;
	size_t startIdx = bufOffset / stride;
	assert((bufOffset % stride) == 0);

	while (i < strips.size()) {
		callback(data);
		newIndex = strips[i] / stride;
		length = newIndex - oldIndex;
		if (length > 0) glDrawArrays(mode, startIdx + oldIndex, length);
		oldIndex = newIndex;
		++i;
	}
}




//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::DrawArray0(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vao0) {
		glBindVertexArray(vao0);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glVertexPointer(3, GL_FLOAT, sizeof(VA_TYPE_0), GetPtr(offsetof(VA_TYPE_0, p)));
		glEnableClientState(GL_VERTEX_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_0));

	if (vao0) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArrayC(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vaoC) {
		glBindVertexArray(vaoC);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(VA_TYPE_C), GetPtr(offsetof(VA_TYPE_C, c)));
		glVertexPointer(3, GL_FLOAT,         sizeof(VA_TYPE_C), GetPtr(offsetof(VA_TYPE_C, p)));
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_C));

	if (vaoC) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArrayT(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vaoT) {
		glBindVertexArray(vaoT);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_T), GetPtr(offsetof(VA_TYPE_T, s)));
		glVertexPointer(  3, GL_FLOAT, sizeof(VA_TYPE_T), GetPtr(offsetof(VA_TYPE_T, p)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_T));

	if (vaoT) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArrayTN(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vaoTN) {
		glBindVertexArray(vaoTN);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_TN), GetPtr(offsetof(VA_TYPE_TN, s)));
		glNormalPointer(     GL_FLOAT, sizeof(VA_TYPE_TN), GetPtr(offsetof(VA_TYPE_TN, n)));
		glVertexPointer(  3, GL_FLOAT, sizeof(VA_TYPE_TN), GetPtr(offsetof(VA_TYPE_TN, p)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_TN));

	if (vaoTN) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArrayTC(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vaoTC) {
		glBindVertexArray(vaoTC);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glTexCoordPointer(2, GL_FLOAT,         sizeof(VA_TYPE_TC), GetPtr(offsetof(VA_TYPE_TC, s)));
		glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(VA_TYPE_TC), GetPtr(offsetof(VA_TYPE_TC, c)));
		glVertexPointer(  3, GL_FLOAT,         sizeof(VA_TYPE_TC), GetPtr(offsetof(VA_TYPE_TC, p)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_TC));

	if (vaoTC) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	SetFence();
}



void CStreamBuffer::DrawArray2d0(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vao2d0) {
		glBindVertexArray(vao2d0);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glVertexPointer(2, GL_FLOAT, sizeof(VA_TYPE_2d0), GetPtr(offsetof(VA_TYPE_2d0, x)));
		glEnableClientState(GL_VERTEX_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_2d0));

	if (vao2d0) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArray2dT(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vao2dT) {
		glBindVertexArray(vao2dT);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_2dT), GetPtr(offsetof(VA_TYPE_2dT, s)));
		glVertexPointer(  2, GL_FLOAT, sizeof(VA_TYPE_2dT), GetPtr(offsetof(VA_TYPE_2dT, x)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_2dT));

	if (vao2dT) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArray2dTC(const int drawType)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vao2dTC) {
		glBindVertexArray(vao2dTC);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glTexCoordPointer(2, GL_FLOAT,         sizeof(VA_TYPE_2dTC), GetPtr(offsetof(VA_TYPE_2dTC, s)));
		glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(VA_TYPE_2dTC), GetPtr(offsetof(VA_TYPE_2dTC, c)));
		glVertexPointer(  2, GL_FLOAT,         sizeof(VA_TYPE_2dTC), GetPtr(offsetof(VA_TYPE_2dTC, x)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		VBO::Unbind();
	}

	DrawArrays(drawType, sizeof(VA_TYPE_2dTC));

	if (vao2dTC) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	SetFence();
}


void CStreamBuffer::DrawArray2dT(const int drawType, StripCallback callback, void* data)
{
	if (drawIndex() == 0)
		return;

	CheckEndStrip();

	if (vao2dT) {
		glBindVertexArray(vao2dT);
	} else {
		VBO::Bind(GL_ARRAY_BUFFER);
		glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_2dT), GetPtr(offsetof(VA_TYPE_2dT, s)));
		glVertexPointer(  2, GL_FLOAT, sizeof(VA_TYPE_2dT), GetPtr(offsetof(VA_TYPE_2dT, x)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		VBO::Unbind();
	}

	DrawArraysCallback(drawType, sizeof(VA_TYPE_2dT), callback, data);

	if (vao2dT) {
		glBindVertexArray(0);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	SetFence();
}
#endif
