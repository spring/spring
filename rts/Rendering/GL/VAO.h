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
	~VAO() { Delete(); }

	VAO& operator = (const VAO& v) = delete;
	VAO& operator = (VAO&& v) { id = v.id; v.id = 0; return *this; }

	uint32_t GetId() { if (id == 0) Generate(); return id; }

	void Bind();
	void Unbind() const;
private:
	void Generate();
	void Delete();
private:
	uint32_t id = 0;
};

#endif

