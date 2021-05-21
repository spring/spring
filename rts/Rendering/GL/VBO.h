/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_VBO_H
#define GL_VBO_H

#include <unordered_map>

#include "Rendering/GL/myGL.h"

/**
 * @brief VBO
 *
 * Vertex buffer Object class (ARB_vertex_buffer_object).
 */
class VBO
{
public:
	VBO(GLenum _defTarget = GL_ARRAY_BUFFER, const bool storage = false, bool readable = false);
	VBO(const VBO& other) = delete;
	VBO(VBO&& other) noexcept { *this = std::move(other); }
	virtual ~VBO();

	VBO& operator=(const VBO& other) = delete;
	VBO& operator=(VBO&& other) noexcept;

	bool IsSupported() const;

	// NOTE: if declared in global scope, user has to call these before exit
	void Release() {
		UnmapIf();
		Delete();
	}
	void Generate() const;
	void Delete();

	/**
	 * @param target can be either GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER or GL_UNIFORM_BUFFER_EXT
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glBindBuffer.xml
	 */
	void Bind() const { Bind(curBoundTarget); }
	void Bind(GLenum target) const;
	void Unbind() const;

	bool BindBufferRange(GLuint index, GLuint offset, GLsizeiptr size) const { return BindBufferRangeImpl(curBoundTarget, index, vboId, offset, size); }
	bool BindBufferRange(GLenum target, GLuint index, GLuint offset, GLsizeiptr size) const { return BindBufferRangeImpl(target, index, vboId, offset, size); };
	bool UnbindBufferRange(GLuint index, GLuint offset, GLsizeiptr size) const { return BindBufferRangeImpl(curBoundTarget, index, 0u, offset, size); };
	bool UnbindBufferRange(GLenum target, GLuint index, GLuint offset, GLsizeiptr size) const { return BindBufferRangeImpl(target, index, 0u, offset, size); };

	/**
	 * @param usage can be either GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, or GL_DYNAMIC_COPY
	 * @param data (optional) initialize the VBO with the data (the array must have minimum `size` length!)
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glBufferData.xml
	 */
	void Resize(GLsizeiptr newSize, GLenum newUsage = GL_STREAM_DRAW);

	template<typename TData>
	void New(const std::vector<TData>& data, GLenum newUsage = GL_STATIC_DRAW) { New(sizeof(TData) * data.size(), newUsage, data.data()); };
	void New(GLsizeiptr newSize, GLenum newUsage = GL_STREAM_DRAW, const void* newData = nullptr);

	void Invalidate(); //< discards all current data (frees the memory w/o resizing)

	/**
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glMapBufferRange.xml
	 */
	template<typename TData>
	GLubyte* MapBuffer(const std::vector<TData>& data, GLintptr elemOffset = 0, GLbitfield access = GL_WRITE_ONLY) { return MapBuffer(sizeof(TData) * elemOffset, sizeof(TData) * data.size(), access); };
	GLubyte* MapBuffer(GLbitfield access = GL_WRITE_ONLY) { return MapBuffer(0, bufSize, access); };
	GLubyte* MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access = GL_WRITE_ONLY);

	void UnmapBuffer();
	void UnmapIf() {
		if (!mapped)
			return;

		Bind();
		UnmapBuffer();
		Unbind();
	}

	template<typename TData>
	void SetBufferSubData(const std::vector<TData>& data, GLintptr elemOffset = 0) { SetBufferSubData(sizeof(TData) * elemOffset, sizeof(TData) * data.size(), data.data()); }
	void SetBufferSubData(GLintptr offset, GLsizeiptr size, void* data);

	GLuint GetId() const {
		if (vboId == 0)
			Generate();
		return vboId;
	}

	GLuint GetIdRaw() const {
		return vboId;
	};

	size_t GetSize() const { return bufSize; }
	size_t GetAlignedSize(size_t sz) const { return VBO::GetAlignedSize(curBoundTarget, sz); };;
	size_t GetOffsetAlignment() const { return VBO::GetOffsetAlignment(curBoundTarget); };

	GLenum GetUsage() const { return usage; }
	void SetUsage(const GLenum _usage) { usage = _usage; }

	const GLvoid* GetPtr(GLintptr offset = 0) const;
public:
	static bool IsSupported(GLenum target);
	static size_t GetAlignedSize(GLenum target, size_t sz);
	static size_t GetOffsetAlignment(GLenum target);
private:
	bool BindBufferRangeImpl(GLenum target, GLuint index, GLuint _vboId, GLuint offset, GLsizeiptr size) const;
private:
	struct BoundBufferRangeIndex {
		BoundBufferRangeIndex() : target{ 0u }, index{ 0u } {};
		BoundBufferRangeIndex(GLenum target, GLuint index) : target{ target }, index{ index } {};
		bool operator == (const BoundBufferRangeIndex& rhs) const {
			return target == rhs.target && index == rhs.index;
		};
		GLenum target;
		GLuint index;
	};

	struct BoundBufferRangeIndexHash {
		std::size_t operator() (const BoundBufferRangeIndex& bbri) const {
			return std::hash<GLenum>()(bbri.target) ^ std::hash<GLuint>()(bbri.index);
		};
	};

	struct BoundBufferRangeData {
		BoundBufferRangeData() : offset{ ~0u }, size{ 0 } {};
		BoundBufferRangeData(GLuint offset, GLsizeiptr size) : offset{ offset }, size{ size } {};
		bool operator == (const BoundBufferRangeData& rhs) const {
			return offset == rhs.offset && size == rhs.size;
		};
		void operator = (const BoundBufferRangeData& rhs) {
			offset = std::min(offset, rhs.offset);
			size = std::max(size, rhs.size);
		};
		GLuint offset;
		GLsizeiptr size;
	};
public:
	bool immutableStorage = false;
	bool readableStorage = false;
	GLuint mapUnsyncedBit;

	mutable bool bound = false;
	bool mapped = false;
private:
	mutable GLuint vboId = 0;

	size_t bufSize = 0; // can be smaller than memSize
	size_t memSize = 0; // actual length of <data>; only set when !isSupported

	mutable GLenum curBoundTarget = 0;
	constexpr static GLenum defTarget = GL_ARRAY_BUFFER;
	GLenum usage = GL_STREAM_DRAW;
private:
	bool isSupported = true; // if false, data is allocated in main memory

	bool nullSizeMapped = false; // Nvidia workaround
	mutable std::unordered_map<BoundBufferRangeIndex, BoundBufferRangeData, BoundBufferRangeIndexHash> bbrItems;
	GLubyte* data = nullptr;
};

#endif /* VBO_H */
