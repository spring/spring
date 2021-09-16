/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_VAO_H
#define GL_VAO_H

#include <cstdint>
#include <utility>

class VAO {
public:
	static bool IsSupported();
public:
	VAO() = default;
	VAO(const VAO& v) = delete;
	VAO(VAO&& v) noexcept { *this = std::move(v); }
	~VAO() { Delete(); }

	VAO& operator = (const VAO& v) = delete;
	VAO& operator = (VAO&& v) noexcept { id = v.id; v.id = 0; return *this; }

	uint32_t GetId() const { Generate(); return GetIdRaw(); }
	uint32_t GetIdRaw() const { return id; }

	void Generate() const;

	void Delete() const;
	void Bind() const;
	void Unbind() const;

private:
	mutable uint32_t id = 0;
};

#endif

