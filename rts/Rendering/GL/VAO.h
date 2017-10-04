/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_VAO_H
#define GL_VAO_H

struct VAO {
public:
	VAO();
	~VAO();

	void Bind() const;
	void Unbind() const;

private:
	uint32_t id = 0;
};

#endif

