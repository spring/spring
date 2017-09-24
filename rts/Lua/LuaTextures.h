/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_TEXTURES_H
#define LUA_TEXTURES_H

#include <string>
#include <vector>

#include "Rendering/GL/myGL.h"
#include "System/UnorderedMap.hpp"


class LuaTextures {
public:
	static const char prefix = '!';

	~LuaTextures() { FreeAll(); }
	LuaTextures() {
		textureVec.reserve(128);
		textureMap.reserve(128);
		lastCode = 0;
	}

	void Clear() {
		textureVec.clear();
		textureMap.clear();
		freeIndices.clear();
	}

	struct Texture {
		Texture()
		: name(""), id(0), fbo(0), fboDepth(0),
		  target(GL_TEXTURE_2D), format(GL_RGBA8),
		  xsize(0), ysize(0), border(0),
		  min_filter(GL_LINEAR), mag_filter(GL_LINEAR),
		  wrap_s(GL_REPEAT), wrap_t(GL_REPEAT), wrap_r(GL_REPEAT),
		  aniso(0.0f)
		{}

		std::string name;

		GLuint id;

		// FIXME: obsolete, use raw FBO's
		GLuint fbo;
		GLuint fboDepth;

		GLenum target;
		GLenum format;

		GLsizei xsize;
		GLsizei ysize;
		GLint border;

		GLenum min_filter;
		GLenum mag_filter;

		GLenum wrap_s;
		GLenum wrap_t;
		GLenum wrap_r;

		GLfloat aniso;
	};

	std::string Create(const Texture& tex);
	bool Bind(const std::string& name) const;
	bool Free(const std::string& name);
	bool FreeFBO(const std::string& name);
	void FreeAll();

	size_t GetIdx(const std::string& name) const;

	const Texture* GetInfo(size_t texIdx) const { return &textureVec[texIdx]; }
	const Texture* GetInfo(const std::string& name) const;

private:
	int lastCode;

	// maps names to textureVec indices
	spring::unsynced_map<std::string, size_t> textureMap;

	std::vector<Texture> textureVec;
	std::vector<size_t> freeIndices;
};


#endif /* LUA_TEXTURES_H */
