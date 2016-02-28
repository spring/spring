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
	if (defTarget == GL_PIXEL_UNPACK_BUFFER) {
		return IsPBOSupported();
	} else {
		return IsVBOSupported();
	}
}


VBO::VBO(GLenum _defTarget, const bool storage) : vboId(0), VBOused(true)
{
	bound = false;
	mapped = false;
	data = nullptr;
	size = 0;
	curBoundTarget = _defTarget;
	defTarget = _defTarget;
	usage = GL_STREAM_DRAW;
	nullSizeMapped = false;

	VBOused = IsSupported();
	immutableStorage = storage;

#ifdef GLEW_ARB_buffer_storage
	if (immutableStorage && !GLEW_ARB_buffer_storage) {
		//note: We can't fallback to traditional BufferObjects, cause then we would have to map/unmap on each change.
		//      Only sysram/cpu VAs give an equivalent behaviour.
		VBOused = false;
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
	if (GLEW_ARB_vertex_buffer_object) {
		glDeleteBuffers(1, &vboId);
	}
	delete[] data;
	data = nullptr;
}


VBO& VBO::operator=(VBO&& other)
{
	std::swap(vboId, other.vboId);
	std::swap(bound, other.bound);
	std::swap(mapped, other.mapped);
	std::swap(data, other.data);
	std::swap(size, other.size);
	std::swap(curBoundTarget, other.curBoundTarget);
	std::swap(defTarget, other.defTarget);
	std::swap(usage, other.usage);
	std::swap(nullSizeMapped, other.nullSizeMapped);

	std::swap(VBOused, other.VBOused);
	std::swap(immutableStorage, other.immutableStorage);

	return *this;
}


void VBO::Bind(GLenum target) const
{
	assert(!bound);

	bound = true;
	if (VBOused) {
		curBoundTarget = target;
		glBindBuffer(target, GetId());
	}
}


void VBO::Unbind() const
{
	assert(bound);

	if (VBOused) {
		glBindBuffer(curBoundTarget, 0);
	}

	bound = false;
}


void VBO::Resize(GLsizeiptr _size, GLenum usage)
{
	assert(bound);
	assert(!mapped);

	if (_size == size && usage == this->usage) // no size change -> nothing to do
		return;

	if (size == 0) // first call: none *BO there yet to copy old data from, so use ::New() (faster)
		return New(_size, usage, nullptr);

	assert(_size > size);
	auto osize = size;
	size = _size;
	this->usage = usage;

	if (VBOused) {
		glClearErrors();
		auto oldBoundTarget = curBoundTarget;

	#ifdef GLEW_ARB_copy_buffer
		if (GLEW_ARB_copy_buffer) {
			VBO vbo(GL_COPY_WRITE_BUFFER, immutableStorage);
			vbo.Bind(GL_COPY_WRITE_BUFFER);
			vbo.New(size, GL_STREAM_DRAW);

			// gpu internal copy (fast)
			if (osize > 0) glCopyBufferSubData(curBoundTarget, GL_COPY_WRITE_BUFFER, 0, 0, osize);

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
			vbo.New(size, GL_STREAM_DRAW);
			void* memdst = vbo.MapBuffer(GL_WRITE_ONLY);

			// cpu download & copy (slow)
			memcpy(memdst, memsrc, osize);
			vbo.UnmapBuffer(); vbo.Unbind();
			Bind(); UnmapBuffer(); Unbind();

			*this = std::move(vbo);
			Bind(oldBoundTarget);
		}

		const GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "VBO/PBO: out of memory");
			Unbind();
			VBOused = false; // disable VBO and fallback to VA/sysmem
			immutableStorage = false;
			Bind();
			Resize(_size, usage); //FIXME copy old vbo data to new sysram one
		}
	} else {
		auto newdata = new GLubyte[size];
		assert(newdata);
		if (data) memcpy(newdata, data, osize);
		delete[] data;
		data = newdata;
	}
}


void VBO::New(GLsizeiptr _size, GLenum usage, const void* data_)
{
	assert(bound);
	assert(!mapped);

	if (data_ == nullptr && _size == size && usage == this->usage)
		return;

	if (immutableStorage && size != 0) {
		LOG_L(L_ERROR, "VBO/PBO: cannot recreate already existing persistent storage buffer");
		return;
	}

	size = _size;
	this->usage = usage;

	if (VBOused) {
		glClearErrors();

	#ifdef GLEW_ARB_buffer_storage
		if (immutableStorage) {
			usage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT;
			glBufferStorage(curBoundTarget, size, data_, usage);
		} else
	#endif
		{
			glBufferData(curBoundTarget, size, data_, usage);
		}

		const GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "VBO/PBO: out of memory");
			Unbind();
			VBOused = false; // disable VBO and fallback to VA/sysmem
			immutableStorage = false;
			Bind(curBoundTarget);
			New(_size, usage, data_);
		}
	} else {
		delete[] data;
		data = nullptr; // to prevent a dead-pointer in case of an out-of-memory exception on the next line
		data = new GLubyte[size];
		assert(data);
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
	assert(offset + _size <= size);
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

	if (_size == 0) {
		// nvidia incorrectly returns GL_INVALID_VALUE when trying to call glMapBufferRange with size zero
		// so catch it ourselves
		nullSizeMapped = true;
		return NULL;
	}

	if (VBOused) {
		GLubyte* ptr = (GLubyte*)glMapBufferRange(curBoundTarget, offset, _size, access);
		assert(ptr);
		return ptr;
	} else {
		assert(data);
		return data + offset;
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


void VBO::Invalidate()
{
	assert(bound);
	assert(immutableStorage || !mapped);

#ifdef GLEW_ARB_invalidate_subdata
	// OpenGL4 way
	if (VBOused && GLEW_ARB_invalidate_subdata) {
		glInvalidateBufferData(GetId());
		return;
	}
#endif
	if (VBOused && globalRendering->atiHacks) {
		Unbind();
		glDeleteBuffers(1, &vboId);
		glGenBuffers(1, &vboId);
		Bind();
		size = -size; // else New() would early-exit
		New(-size, usage, nullptr);
		return;
	}

	// note: allocating memory doesn't actually block the memory it just makes room in _virtual_ memory space
	New(size, usage, nullptr);
}


const GLvoid* VBO::GetPtr(GLintptr offset) const
{
	assert(bound);

	if (VBOused) {
		return (GLvoid*)((char*)NULL + (offset));
	} else {
		if (!data) return nullptr;
		return (GLvoid*)(data + offset);
	}
}
