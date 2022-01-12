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
		GLuint id = 0;

		// FIXME: obsolete, use raw FBO's
		GLuint fbo = 0;
		GLuint fboDepth = 0;

		GLenum target = GL_TEXTURE_2D;
		GLenum format = GL_RGBA8;

		GLsizei xsize   = 0;
		GLsizei ysize   = 0;
		GLsizei samples = 0;

		GLint border = 0;

		GLenum min_filter = GL_LINEAR;
		GLenum mag_filter = GL_LINEAR;

		GLenum wrap_s = GL_REPEAT;
		GLenum wrap_t = GL_REPEAT;
		GLenum wrap_r = GL_REPEAT;

		GLfloat aniso = 0.0f;
	};

	std::string Create(const Texture& tex);
	bool Bind(const std::string& name) const;
	bool Free(const std::string& name);
	bool FreeFBO(const std::string& name);
	void FreeAll();
	void ChangeParams(const Texture& tex) const;
	void ApplyParams(const Texture& tex) const;

	size_t GetIdx(const std::string& name) const;

	const Texture* GetInfo(size_t texIdx) const { return ((texIdx < textureVec.size())? &textureVec[texIdx]: nullptr); }
	      Texture* GetInfo(size_t texIdx)       { return ((texIdx < textureVec.size())? &textureVec[texIdx]: nullptr); }

	const Texture* GetInfo(const std::string& name) const { return (GetInfo(GetIdx(name))); }
	      Texture* GetInfo(const std::string& name)       { return (GetInfo(GetIdx(name))); }

private:
	int lastCode;

	// maps names to textureVec indices
	spring::unsynced_map<std::string, size_t> textureMap;

	std::vector<Texture> textureVec;
	std::vector<size_t> freeIndices;
};


#endif /* LUA_TEXTURES_H */
