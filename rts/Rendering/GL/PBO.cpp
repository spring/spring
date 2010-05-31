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
#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "ConfigHandler.h"


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
	mapped = true;
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
			if (!gu->atiHacks) {
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
}


void PBO::Resize(GLsizeiptr size, GLenum usage)
{
	assert(bound);

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

	mapped = true;
	if (PBOused) {
		return (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, access);
	} else {
		assert(data);
		return data;
	}
}


GLubyte* PBO::MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access)
{
	assert(!mapped);

	mapped = true;
	if (PBOused) {
		return (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, offset, size, access);
	} else {
		assert(data);
		return data;
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


const GLvoid* PBO::GetPtr()
{
	assert(bound);

	if (PBOused) {
		return 0;
	} else {
		assert(data);
		return (GLvoid*)data;
	}
}

