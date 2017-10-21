/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief ARB_vertex_buffer_object implementation
 * ARB_vertex_buffer_object class implementation
 */

#include <cassert>
#include <vector>

#include "VBO.h"

#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

VBO::VBO(GLenum _defTarget, const bool storage)
{
	curBoundTarget = _defTarget;
	defTarget = _defTarget;

	immutableStorage = storage;
}

// NOTE: if declared in global scope, user has to call these before exit
VBO::~VBO()
{
	UnmapIf();
	Delete();
}


VBO& VBO::operator=(VBO&& other)
{
	std::swap(vboId, other.vboId);
	std::swap(bound, other.bound);
	std::swap(mapped, other.mapped);

	std::swap(bufSize, other.bufSize);
	std::swap(memSize, other.memSize);

	std::swap(curBoundTarget, other.curBoundTarget);
	std::swap(defTarget, other.defTarget);
	std::swap(usage, other.usage);
	std::swap(nullSizeMapped, other.nullSizeMapped);

	std::swap(immutableStorage, other.immutableStorage);

	return *this;
}


void VBO::Generate() const { glGenBuffers(1, &vboId); }
void VBO::Delete() const { glDeleteBuffers(1, &vboId); vboId = 0; }

void VBO::Bind(GLenum target) const
{
	assert(!bound);
	bound = true;

	glBindBuffer(curBoundTarget = target, GetId());
}

void VBO::Unbind() const
{
	assert(bound);
	bound = false;

	glBindBuffer(curBoundTarget, 0);
}


bool VBO::Resize(GLsizeiptr newSize, GLenum newUsage)
{
	assert(bound);
	assert(!mapped);

	// no size change -> nothing to do
	if (newSize == bufSize && newUsage == usage)
		return true;

	// first call: no *BO exists yet to copy old data from, so use ::New() (faster)
	if (bufSize == 0)
		return New(newSize, newUsage, nullptr);

	const size_t oldSize = bufSize;

	glClearErrors("VBO", __func__, globalRendering->glDebugErrors);
	const GLenum oldBoundTarget = curBoundTarget;

	{
		VBO vbo(GL_COPY_WRITE_BUFFER, immutableStorage);

		vbo.Bind(GL_COPY_WRITE_BUFFER);
		vbo.New(newSize, GL_STREAM_DRAW);

		// gpu internal copy (fast)
		if (oldSize > 0)
			glCopyBufferSubData(curBoundTarget, GL_COPY_WRITE_BUFFER, 0, 0, oldSize);

		vbo.Unbind();
		Unbind();
		*this = std::move(vbo);
		Bind(oldBoundTarget);
	}

	const GLenum err = glGetError();

	if (err == GL_NO_ERROR) {
		bufSize = newSize;
		usage = newUsage;
		return true;
	}

	LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=%u)] id=%u tgt=0x%x err=0x%x", __func__, (unsigned long) bufSize, usage, vboId, curBoundTarget, err);
	Unbind();
	return false;
}


bool VBO::New(GLsizeiptr newSize, GLenum newUsage, const void* newData)
{
	assert(bound);
	assert(!mapped || (newData == nullptr && newSize == bufSize && newUsage == usage));

	// no-op new, allows e.g. repeated Bind+New with persistent buffers
	if (newData == nullptr && newSize == bufSize && newUsage == usage)
		return true;

	if (immutableStorage && bufSize != 0) {
		LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=0x%x)] cannot recreate persistent storage buffer", __func__, (unsigned long) bufSize, usage);
		return false;
	}


	glClearErrors("VBO", __func__, globalRendering->glDebugErrors);

	#ifdef GLEW_ARB_buffer_storage
	if (immutableStorage) {
		glBufferStorage(curBoundTarget, newSize, newData, /*newUsage =*/ GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT);
	} else
	#endif
	{
		glBufferData(curBoundTarget, newSize, newData, newUsage);
	}

	const GLenum err = glGetError();

	if (err == GL_NO_ERROR) {
		bufSize = newSize;
		usage = newUsage;
		return true;
	}

	LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=0x%x)] id=%u tgt=0x%x err=0x%x", __func__, (unsigned long) bufSize, usage, vboId, curBoundTarget, err);
	Unbind();
	return false;
}


GLubyte* VBO::MapBuffer(GLbitfield access)
{
	assert(!mapped);
	return MapBuffer(0, bufSize, access);
}


GLubyte* VBO::MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access)
{
	assert(!mapped);
	assert((offset + size) <= bufSize);
	mapped = true;

	// glMapBuffer & glMapBufferRange use different flags for their access argument
	// for easier handling convert the glMapBuffer ones here
	switch (access) {
		case GL_WRITE_ONLY:
			access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		#ifdef GLEW_ARB_buffer_storage
			if (immutableStorage) {
				access = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
			}
		#endif
			break;
		case GL_READ_WRITE:
			access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
			break;
		case GL_READ_ONLY:
			access = GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
			break;
		default: break;
	}

	if (size == 0) {
		// nvidia incorrectly returns GL_INVALID_VALUE when trying to call glMapBufferRange with size zero
		// so catch it ourselves
		nullSizeMapped = true;
		return nullptr;
	}


	GLubyte* ptr = (GLubyte*) glMapBufferRange(curBoundTarget, offset, size, access);
	assert(ptr != nullptr);
	return ptr;
}


void VBO::UnmapBuffer()
{
	assert(mapped);

	if (!nullSizeMapped)
		glUnmapBuffer(curBoundTarget);

	mapped = false;
	nullSizeMapped = false;
}


void VBO::Invalidate()
{
	assert(bound);
	assert(immutableStorage || !mapped);

#ifdef GLEW_ARB_invalidate_subdata
	// OpenGL4 way
	if (GLEW_ARB_invalidate_subdata) {
		glInvalidateBufferData(GetId());
		return;
	}
#endif
	if (globalRendering->atiHacks) {
		Unbind();
		glDeleteBuffers(1, &vboId);
		glGenBuffers(1, &vboId);
		Bind();
		bufSize = -bufSize; // else New() would early-exit
		New(-bufSize, usage, nullptr);
		return;
	}

	// note: allocating memory doesn't actually block the memory it just makes room in _virtual_ memory space
	New(bufSize, usage, nullptr);
}


const GLvoid* VBO::GetPtr(GLintptr offset) const
{
	assert(bound);
	return (GLvoid*)((char*)nullptr + (offset));
}

