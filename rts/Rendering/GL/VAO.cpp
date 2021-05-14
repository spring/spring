/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VAO.h"
#include "Rendering/GL/myGL.h"

bool VAO::IsSupported()
{
	static bool supported = GLEW_ARB_vertex_array_object;
	return supported;
}

void VAO::Generate() const
{
	if (id > 0)
		return;

	glGenVertexArrays(1, &id);
}

void VAO::Delete() const
{
	if (id > 0) {
		glDeleteVertexArrays(1, &id);
		id = 0;
	}
}

void VAO::Bind() const
{
	glBindVertexArray(GetId());
}

void VAO::Unbind() const
{
	glBindVertexArray(0);
}
