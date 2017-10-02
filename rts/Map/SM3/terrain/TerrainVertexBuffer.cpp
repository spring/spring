/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TerrainBase.h"
#include "TerrainVertexBuffer.h"

using namespace terrain;

int VertexBuffer::totalBufferSize = 0;

VertexBuffer::VertexBuffer()
	: data(NULL)
	, id(0)
	, size(0)
	, type(GL_ARRAY_BUFFER_ARB)
{
}

void VertexBuffer::Init(int bytesize)
{
	Free();

	data = nullptr;
	glGenBuffersARB(1, &id);

	size = bytesize;
	totalBufferSize += size;
}

VertexBuffer::~VertexBuffer()
{
	Free();
}

void VertexBuffer::Free()
{
	glDeleteBuffersARB(1, &id);
	id = 0;

	totalBufferSize -= size;
}

void* VertexBuffer::LockData()
{
	if (id) {
		// Hurray for ATI, which has broken buffer memory mapping support :(
		/*glBindBufferARB(type, id);
		glBufferDataARB(type, size, 0, GL_STATIC_DRAW_ARB);
		return glMapBufferARB(type, GL_WRITE_ONLY);*/
		data = new char [size];
		return data;
	}

	return data;
}

void VertexBuffer::UnlockData()
{
	if (id) {
		glBindBufferARB(type, id);
		glBufferDataARB(type, size, data, GL_STATIC_DRAW_ARB);
		glBindBufferARB(type, 0);
		delete[] data;
	}
}

void* VertexBuffer::Bind()
{
	if (id) {
		glBindBufferARB(type, id);
		return 0;
	}
	else return (void*)data;
}


void VertexBuffer::Unbind()
{
	if (id) {
		glBindBufferARB(type, 0);
	}
}


IndexBuffer::IndexBuffer()
{
	type = GL_ELEMENT_ARRAY_BUFFER_ARB;
}
