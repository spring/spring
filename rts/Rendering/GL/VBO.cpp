/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief ARB_vertex_buffer_object implementation
 * ARB_vertex_buffer_object class implementation
 */

#include <assert.h>
#include <vector>

#include "VBO.h"

#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"

CONFIG(bool, UseVBO).defaultValue(true).safemodeValue(false);
CONFIG(bool, UsePBO).defaultValue(true).safemodeValue(false);


/**
 * Returns if the current gpu drivers support VertexBufferObjects
 */
bool VBO::IsVBOSupported()
{
	return (GLEW_ARB_vertex_buffer_object && GLEW_ARB_map_buffer_range && configHandler->GetBool("UseVBO"));
}


/**
 * Returns if the current gpu drivers support PixelBufferObjects
 */
bool VBO::IsPBOSupported()
{
	return (GLEW_EXT_pixel_buffer_object && GLEW_ARB_map_buffer_range && configHandler->GetBool("UsePBO"));
}


bool VBO::IsSupported() const
{
	if (defTarget == GL_PIXEL_UNPACK_BUFFER) {
		return IsPBOSupported();
	} else {
		return IsVBOSupported();
	}
}


VBO::VBO(GLenum _defTarget) : vboId(0)
{
	bound = false;
	mapped = false;
	data = NULL;
	size = 0;
	curBoundTarget = _defTarget;
	defTarget = _defTarget;
	usage = GL_STREAM_DRAW;
	nullSizeMapped = false;

	if (IsSupported()) {
		glGenBuffers(1, &vboId);
		VBOused = true;
	} else {
		VBOused = false;
	}
}


VBO::~VBO()
{
	if (VBOused) {
		glDeleteBuffers(1, &vboId);
	} else {
		delete[] data;
		data = NULL;
	}
}


void VBO::Bind(GLenum target)
{
	assert(!bound);

	bound = true;
	if (VBOused) {
		curBoundTarget = target;
		glBindBuffer(target, vboId);
	}
}


void VBO::Unbind(bool discard)
{
	assert(bound);

	if (VBOused) {
		if (discard) {
			if (!globalRendering->atiHacks) {
				glBufferData(curBoundTarget, 0, 0, usage);
			} else {
				glBindBuffer(curBoundTarget, 0);
				glDeleteBuffers(1, &vboId);
				glGenBuffers(1, &vboId);
			}
		}
		glBindBuffer(curBoundTarget, 0);
	} else {
		if (discard) {
			delete[] data;
			data = NULL;
		}
	}

	bound = false;
	if (discard) {
		size = 0;
	}
}


void VBO::Resize(GLsizeiptr _size, GLenum usage, const void* data_)
{
	assert(bound);

	size = _size;
	if (VBOused) {
		this->usage = usage;
		glBufferData(curBoundTarget, size, data_, usage);
	} else {
		delete[] data;
		data = NULL; // to prevent a dead-pointer in case of an out-of-memory exception on the next line
		data = new GLubyte[size];
		if (data_) {
			memcpy(data, data_, size);
		}
	}
}


GLubyte* VBO::MapBuffer(GLbitfield access)
{
	assert(!mapped);
	return MapBuffer(0, size, access);
}


GLubyte* VBO::MapBuffer(GLintptr offset, GLsizeiptr _size, GLbitfield access)
{
	assert(!mapped);

	// glMapBuffer & glMapBufferRange use different flags for their access argument
	// for easier handling convert the glMapBuffer ones here
	switch (access) {
		case GL_WRITE_ONLY:
			access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
			break;
		case GL_READ_WRITE:
			access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
			break;
		case GL_READ_ONLY:
			access = GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
			break;
	}

	mapped = true;

	if (_size == 0) {
		// nvidia incorrectly returns GL_INVALID_VALUE when trying to call glMapBufferRange with size zero
		// so catch it ourselves
		nullSizeMapped = true;
		return NULL;
	}

	if (VBOused) {
		return (GLubyte*)glMapBufferRange(curBoundTarget, offset, _size, access);
	} else {
		assert(data);
		return data+offset;
	}
}


void VBO::UnmapBuffer()
{
	assert(mapped);

	if (nullSizeMapped) {
		nullSizeMapped = false;
	} else
	if (VBOused) {
		glUnmapBuffer(curBoundTarget);
	}
	else {
	}
	mapped = false;
}


const GLvoid* VBO::GetPtr(GLintptr offset)
{
	assert(bound);

	if (VBOused) {
		return (GLvoid*)((char*)NULL + (offset));
	} else {
		assert(data);
		return (GLvoid*)(data + offset);
	}
}

