/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "TerrainBase.h"
#include "TerrainVertexBuffer.h"

using namespace terrain;

int VertexBuffer::totalBufferSize = 0;

VertexBuffer::VertexBuffer() 
{
	id = 0;
	data = 0;
	size = 0;
	type = GL_ARRAY_BUFFER_ARB;
}

void VertexBuffer::Init (int bytesize)
{
	Free ();

	if (GLEW_ARB_vertex_buffer_object) {
		data=0;
		glGenBuffersARB(1,&id);
	} else {
		data=new char[bytesize];
	}
	size=bytesize;
	totalBufferSize+=size;
}

VertexBuffer::~VertexBuffer ()
{
	Free ();
}

void VertexBuffer::Free ()
{
	if (id) {
		glDeleteBuffersARB(1,&id);
		id=0;
	} else {
		delete [] data;
	}
	totalBufferSize-=size;
}

void* VertexBuffer::LockData ()
{
	if (id) {
		// Hurray for ATI, which has broken buffer memory mapping support :(
		/*glBindBufferARB(type, id);
		glBufferDataARB(type, size, 0, GL_STATIC_DRAW_ARB);
		return glMapBufferARB(type, GL_WRITE_ONLY);*/
		data = new char [size];
		return data;
	} else
		return data;
}

void VertexBuffer::UnlockData ()
{
	if (id) {
		glBindBufferARB(type, id);
		glBufferDataARB(type, size, data, GL_STATIC_DRAW_ARB);
		glBindBufferARB(type, 0);
		delete[] data;
	}
}

void* VertexBuffer::Bind ()
{
	if (id) {
		glBindBufferARB(type, id);
		return 0;
	}
	else return (void*)data;
}


void VertexBuffer::Unbind ()
{
	if (id)
		glBindBufferARB(type, 0);
}


IndexBuffer::IndexBuffer ()
{
	type = GL_ELEMENT_ARRAY_BUFFER_ARB;
}
