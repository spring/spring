/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VBO_H
#define VBO_H

#include "Rendering/GL/myGL.h"

/**
 * @brief VBO
 *
 * Vertex buffer Object class (ARB_vertex_buffer_object).
 */
class VBO
{
public:
	VBO(GLenum defTarget = GL_ARRAY_BUFFER);
	virtual ~VBO();

	bool IsSupported() const;
	static bool IsVBOSupported();
	static bool IsPBOSupported();

	/**
	 * @param target can be either GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER or GL_UNIFORM_BUFFER_EXT
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glBindBuffer.xml
	 */
	void Bind(GLenum target) const;
	void Unbind() const;
	void Unbind(bool discard);

	/**
	 * @param usage can be either GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, or GL_DYNAMIC_COPY
	 * @param data (optional) initialize the VBO with the data (the array must have minimum `size` length!)
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glBindBuffer.xml
	 */
	void Resize(GLsizeiptr size, GLenum usage = GL_STREAM_DRAW, const void* data = NULL);

	GLubyte* MapBuffer(GLbitfield access = GL_WRITE_ONLY);
	GLubyte* MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access = GL_WRITE_ONLY);
	void UnmapBuffer();

	GLuint GetId() const {
		if (VBOused && vboId == 0) glGenBuffers(1, &vboId);
		return vboId;
	}

	size_t GetSize() const { return size; }
	const GLvoid* GetPtr(GLintptr offset = 0) const;

protected:
	mutable GLuint vboId;
	size_t size;
	mutable GLenum curBoundTarget;
	GLenum defTarget;
	GLenum usage;

private:
	bool VBOused;
	mutable bool bound;
	bool mapped;
	bool nullSizeMapped; //< Nvidia workaround

	GLubyte* data;
};

#endif /* VBO_H */
