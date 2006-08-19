/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/
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
