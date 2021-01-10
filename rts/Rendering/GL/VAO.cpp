/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VAO.h"

void VAO::Generate() { glGenVertexArrays(1, &id); }
void VAO::Delete() {
	if (id > 0) {
		glDeleteVertexArrays(1, &id);
		id = 0;
	}
}
void VAO::Bind() {
	glBindVertexArray(GetId());
}
void VAO::Unbind() const { glBindVertexArray(0); }