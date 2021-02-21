/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_VAO_H
#define GL_VAO_H

#include <cstdint>
#include <utility>
#include "Rendering/GL/myGL.h"

struct VAO {
public:
	static bool IsSupported() {
		static bool supported = GLEW_ARB_vertex_array_object;
		return supported;
	}
public:
	VAO() = default;
	VAO(const VAO& v) = delete;
	VAO(VAO&& v) noexcept { *this = std::move(v); }
	~VAO() { Delete(); }

	VAO& operator = (const VAO& v) = delete;
	VAO& operator = (VAO&& v) noexcept { id = v.id; v.id = 0; return *this; }

	uint32_t GetId() const { Generate(); return id; }

	void Generate() const {
		if (id > 0)
			return;

		glGenVertexArrays(1, &id);
	};

	void Delete() {
		if (id > 0) {
			glDeleteVertexArrays(1, &id);
			id = 0;
		}
	}
	void Bind() const { glBindVertexArray(GetId()); }
	void Unbind() const { glBindVertexArray(0); }

private:
	mutable uint32_t id = 0;
};

#endif

