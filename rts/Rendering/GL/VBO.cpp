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

CONFIG(bool, UseVBO).defaultValue(true).safemodeValue(false);
CONFIG(bool, UsePBO).defaultValue(true).safemodeValue(false).headlessValue(false);


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
	return ((defTarget == GL_PIXEL_UNPACK_BUFFER)? IsPBOSupported(): IsVBOSupported());
}


VBO::VBO(GLenum _defTarget, const bool storage) : vboId(0), isSupported(true)
{
	bound = false;
	mapped = false;
	data = nullptr;
	bufSize = 0;
	memSize = 0;
	curBoundTarget = _defTarget;
	defTarget = _defTarget;
	usage = GL_STREAM_DRAW;
	nullSizeMapped = false;

	isSupported = IsSupported();
	immutableStorage = storage;

#ifdef GLEW_ARB_buffer_storage
	if (immutableStorage && !GLEW_ARB_buffer_storage) {
		//note: We can't fallback to traditional BufferObjects, cause then we would have to map/unmap on each change.
		//      Only sysram/cpu VAs give an equivalent behaviour.
		isSupported = false;
		immutableStorage = false;
		//LOG_L(L_ERROR, "VBO/PBO: cannot create immutable storage, gpu drivers missing support for it!");
	}
#endif
}


VBO::~VBO()
{
	if (mapped) {
		Bind();
		UnmapBuffer();
		Unbind();
	}
	if (GLEW_ARB_vertex_buffer_object)
		glDeleteBuffers(1, &vboId);

	delete[] data;
	data = nullptr;
}


VBO& VBO::operator=(VBO&& other)
{
	std::swap(vboId, other.vboId);
	std::swap(bound, other.bound);
	std::swap(mapped, other.mapped);
	std::swap(data, other.data);
	std::swap(bufSize, other.bufSize);
	std::swap(memSize, other.memSize);
	std::swap(curBoundTarget, other.curBoundTarget);
	std::swap(defTarget, other.defTarget);
	std::swap(usage, other.usage);
	std::swap(nullSizeMapped, other.nullSizeMapped);

	std::swap(isSupported, other.isSupported);
	std::swap(immutableStorage, other.immutableStorage);

	return *this;
}


void VBO::Bind(GLenum target) const
{
	assert(!bound);

	bound = true;
	if (isSupported) {
		curBoundTarget = target;
		glBindBuffer(target, GetId());
	}
}


void VBO::Unbind() const
{
	assert(bound);

	if (isSupported)
		glBindBuffer(curBoundTarget, 0);

	bound = false;
}


void VBO::Resize(GLsizeiptr newSize, GLenum newUsage)
{
	assert(bound);
	assert(!mapped);

	// no size change -> nothing to do
	if (newSize == bufSize && newUsage == usage)
		return;

	// first call: no *BO exists yet to copy old data from, so use ::New() (faster)
	if (bufSize == 0)
		return New(newSize, usage, nullptr);

	assert(newSize > bufSize);

	const size_t oldSize = bufSize;
	bufSize = newSize;
	usage = newUsage;

	if (isSupported) {
		glClearErrors("VBO", __func__, globalRendering->glDebugErrors);
		const GLenum oldBoundTarget = curBoundTarget;

	#ifdef GLEW_ARB_copy_buffer
		if (GLEW_ARB_copy_buffer) {
			VBO vbo(GL_COPY_WRITE_BUFFER, immutableStorage);

			vbo.Bind(GL_COPY_WRITE_BUFFER);
			vbo.New(bufSize, GL_STREAM_DRAW);

			// gpu internal copy (fast)
			if (oldSize > 0)
				glCopyBufferSubData(curBoundTarget, GL_COPY_WRITE_BUFFER, 0, 0, oldSize);

			vbo.Unbind();
			Unbind();
			*this = std::move(vbo);
			Bind(oldBoundTarget);
		} else
	#endif
		{
			void* memsrc = MapBuffer(GL_READ_ONLY);
			Unbind();

			VBO vbo(oldBoundTarget, immutableStorage);
			vbo.Bind(oldBoundTarget);
			vbo.New(bufSize, GL_STREAM_DRAW);
			void* memdst = vbo.MapBuffer(GL_WRITE_ONLY);

			// cpu download & copy (slow)
			memcpy(memdst, memsrc, oldSize);
			vbo.UnmapBuffer(); vbo.Unbind();
			Bind(); UnmapBuffer(); Unbind();

			*this = std::move(vbo);
			Bind(oldBoundTarget);
		}

		const GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=%u)] id=%u tgt=0x%x err=0x%x", __func__, (unsigned long) bufSize, usage, vboId, curBoundTarget, err);
			Unbind();

			// disable VBO and fallback to VA/sysmem
			isSupported = false;
			immutableStorage = false;

			// FIXME: copy old vbo data to sysram
			Bind();
			Resize(newSize, usage);
		}

		return;
	}


	if (bufSize > memSize) {
		// grow the buffer if needed, only bufSize is adjusted when shrinking it
		GLubyte* newData = new GLubyte[memSize = bufSize];
		assert(newData != nullptr);

		if (data != nullptr) {
			memcpy(newData, data, oldSize);
			delete[] data;
		}

		data = newData;
	}
}


void VBO::New(GLsizeiptr newSize, GLenum newUsage, const void* newData)
{
	assert(bound);
	assert(!mapped || (newData == nullptr && newSize == bufSize && newUsage == usage));

	// no-op new, allows e.g. repeated Bind+New with persistent buffers
	if (newData == nullptr && newSize == bufSize && newUsage == usage)
		return;

	if (immutableStorage && bufSize != 0) {
		LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=0x%x,data=%p)] cannot recreate persistent storage buffer", __func__, (unsigned long) bufSize, usage, data);
		return;
	}

	if (isSupported) {
		glClearErrors("VBO", __func__, globalRendering->glDebugErrors);

	#ifdef GLEW_ARB_buffer_storage
		if (immutableStorage) {
			glBufferStorage(curBoundTarget, newSize, newData, newUsage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT);
		} else
	#endif
		{
			glBufferData(curBoundTarget, newSize, newData, newUsage);
		}

		const GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=0x%x,data=%p)] id=%u tgt=0x%x err=0x%x", __func__, (unsigned long) bufSize, usage, data, vboId, curBoundTarget, err);
			Unbind();

			// disable VBO and fallback to VA/sysmem
			isSupported = false;
			immutableStorage = false;

			Bind(curBoundTarget);
			New(newSize, newUsage, newData);
		}

		bufSize = newSize;
		usage = newUsage;
		return;
	}


	usage = newUsage;
	bufSize = newSize;

	if (newSize > memSize) {
		delete[] data;

		// prevent a dead-pointer in case of an OOM exception on the next line
		data = nullptr;
		data = new GLubyte[memSize = bufSize];

		if (newData == nullptr)
			return;

		assert(data != nullptr);
		memcpy(data, newData, newSize);
	} else {
		// keep the larger buffer; reduces fragmentation from repeated New's
		memset(data, 0, memSize);

		if (newData == nullptr)
			return;

		memcpy(data, newData, newSize);
	}
}


GLubyte* VBO::MapBuffer(GLbitfield access)
{
	assert(!mapped);
	return MapBuffer(0, bufSize, access);
}


GLubyte* VBO::MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access)
{
	assert(!mapped);
	assert(offset + size <= bufSize);
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

	if (isSupported) {
		GLubyte* ptr = (GLubyte*)glMapBufferRange(curBoundTarget, offset, size, access);
		assert(ptr);
		return ptr;
	}
	assert(data);
	return data + offset;
}


void VBO::UnmapBuffer()
{
	assert(mapped);

	if (nullSizeMapped)
		nullSizeMapped = false;
	else if (isSupported)
		glUnmapBuffer(curBoundTarget);

	mapped = false;
}


void VBO::Invalidate()
{
	assert(bound);
	assert(immutableStorage || !mapped);

#ifdef GLEW_ARB_invalidate_subdata
	// OpenGL4 way
	if (isSupported && GLEW_ARB_invalidate_subdata) {
		glInvalidateBufferData(GetId());
		return;
	}
#endif
	if (isSupported && globalRendering->atiHacks) {
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

	if (isSupported)
		return (GLvoid*)((char*)nullptr + (offset));

	if (data == nullptr)
		return nullptr;

	return (GLvoid*)(data + offset);
}

