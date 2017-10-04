/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VAO.h"
#include "Rendering/GL/myGL.h"

VAO::VAO() { glGenVertexArrays(1, &id); }
VAO::~VAO() { glDeleteVertexArrays(1, &id); }

void VAO::Bind() const { glBindVertexArray(id); }
void VAO::Unbind() const { glBindVertexArray(0); }

