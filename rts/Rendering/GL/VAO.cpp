/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VAO.h"
#include "Rendering/GL/myGL.h"

void VAO::Generate() { glGenVertexArrays(1, &id); }
void VAO::Delete() { glDeleteVertexArrays(1, &id); id = 0; }
void VAO::Bind() const { glBindVertexArray(id); }
void VAO::Unbind() const { glBindVertexArray(0); }

