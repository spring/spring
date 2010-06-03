/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief EXT_pixel_buffer_object implementation
 * EXT_pixel_buffer_object class implementation
 */

#include "StdAfx.h"
#include <assert.h>
#include <vector>
#include "mmgr.h"

#include "PBO.h"

#include "Rendering/GlobalRendering.h"
#include "System/LogOutput.h"
#include "System/GlobalUnsynced.h"
#include "System/ConfigHandler.h"


/**
 * Returns if the current gpu drivers support Pixelbuffer Objects
 */
bool PBO::IsSupported()
{
	return (GLEW_EXT_pixel_buffer_object && GLEW_ARB_map_buffer_range && !!configHandler->Get("UsePBO", 1));
}


PBO::PBO() : pboId(0)
{
	bound = false;
	mapped = false;
	data = NULL;

	if (IsSupported()) {
		glGenBuffers(1, &pboId);
		PBOused = true;
	} else {
		PBOused = false;
	}
}


PBO::~PBO()
{
	if (PBOused) {
		glDeleteBuffers(1, &pboId);
	} else {
		delete[] data;
		data = NULL;
	}
}


void PBO::Bind()
{
	assert(!bound);

	bound = true;
	if (PBOused) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
	}
}


void PBO::Unbind(bool discard)
{
	assert(bound);

	if (PBOused) {
		if (discard) {
			if (!globalRendering->atiHacks) {
				glBufferData(GL_PIXEL_UNPACK_BUFFER, 0, 0, GL_STREAM_DRAW);
			} else {
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				glDeleteBuffers(1, &pboId);
				glGenBuffers(1, &pboId);
			}
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
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


void PBO::Resize(GLsizeiptr _size, GLenum usage)
{
	assert(bound);

	size = _size;
	if (PBOused) {
		glBufferData(GL_PIXEL_UNPACK_BUFFER, size, 0, usage);
	} else {
		delete[] data;
		data = new GLubyte[size];
	}
}


GLubyte* PBO::MapBuffer(GLbitfield access)
{
	assert(!mapped);

	//! we don't use glMapBuffer, because glMapBufferRange seems to be a small
	//! step faster than it due to GL_MAP_UNSYNCHRONIZED_BIT
	/*mapped = true;
	if (PBOused) {
		return (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, access);
	} else {
		assert(data);
		return data;
	}*/

	return MapBuffer(0, size, access);
}


GLubyte* PBO::MapBuffer(GLintptr offset, GLsizeiptr _size, GLbitfield access)
{
	assert(!mapped);

	//! glMapBuffer & glMapBufferRange use different flags for their access argument
	//! for easier handling convert the glMapBuffer ones here
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
	if (PBOused) {
		return (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, offset, _size, access);
	} else {
		assert(data);
		return data+offset;
	}
}


void PBO::UnmapBuffer()
{
	assert(mapped);

	if (PBOused) {
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	} else {
	}
	mapped = false;
}


const GLvoid* PBO::GetPtr(GLintptr offset)
{
	assert(bound);

	if (PBOused) {
		return (GLvoid*)((char*)NULL + (offset));
	} else {
		assert(data);
		return (GLvoid*)(data + offset);
	}
}

