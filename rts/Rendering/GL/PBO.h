/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PBO_H
#define PBO_H

#include "Rendering/GL/myGL.h"

/**
 * @brief PBO
 *
 * Pixelbuffer Object class (EXT_pixel_buffer_object).
 * WARNING: CANNOT BE USED IN COMBINATION WITH gluBuild2DMipmaps/glBuildMipmaps!!!
 */
class PBO
{
public:
	PBO();
	~PBO();

	static bool IsSupported();

	void Bind();
	void Unbind(bool discard = true);

	void Resize(GLsizeiptr size, GLenum usage = GL_STREAM_DRAW);

	GLubyte* MapBuffer(GLbitfield access = GL_WRITE_ONLY);
	GLubyte* MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access = GL_WRITE_ONLY);
	void UnmapBuffer();

	const GLvoid* GetPtr(GLintptr offset = 0);

protected:
	GLuint pboId;
	size_t size;

private:
	bool PBOused;
	bool bound;
	bool mapped;

	GLubyte* data;
};

#endif /* PBO_H */
