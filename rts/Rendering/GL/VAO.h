/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_VAO_H
#define GL_VAO_H

#include <cstdint>
#include <utility>

struct VAO {
public:
	VAO() = default;
	VAO(const VAO& v) = delete;
	VAO(VAO&& v) { *this = std::move(v); }

	VAO& operator = (const VAO& v) = delete;
	VAO& operator = (VAO&& v) { id = v.id; v.id = 0; return *this; }

	void Generate();
	void Delete();
	void Bind() const;
	void Unbind() const;

private:
	uint32_t id = 0;
};

#endif

