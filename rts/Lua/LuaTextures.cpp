/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaTextures.h"
#include "System/Util.h"


/******************************************************************************/
/******************************************************************************/

std::string LuaTextures::Create(const Texture& tex)
{	
	GLint currentBinding;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentBinding);

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(tex.target, texID);

	GLenum dataFormat = GL_RGBA;
	GLenum dataType   = GL_UNSIGNED_BYTE;
	if ((tex.format == GL_DEPTH_COMPONENT) ||
	    (tex.format == GL_DEPTH_COMPONENT16) ||
	    (tex.format == GL_DEPTH_COMPONENT24) ||
	    (tex.format == GL_DEPTH_COMPONENT32)) {
		dataFormat = GL_DEPTH_COMPONENT;
		dataType = GL_FLOAT;
	}

	glClearErrors();
	glTexImage2D(tex.target, 0, tex.format,
	             tex.xsize, tex.ysize, tex.border,
	             dataFormat, dataType, NULL);
	const GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, currentBinding);
		return std::string("");
	}

	glTexParameteri(tex.target, GL_TEXTURE_WRAP_S, tex.wrap_s);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_T, tex.wrap_t);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_R, tex.wrap_r);
	glTexParameteri(tex.target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
	glTexParameteri(tex.target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);
	glTexParameteri(tex.target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);

	if ((tex.aniso != 0.0f) && GLEW_EXT_texture_filter_anisotropic) {
		static GLfloat maxAniso = -1.0f;

		if (maxAniso == -1.0f)
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

		glTexParameterf(tex.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::max(1.0f, std::min(maxAniso, tex.aniso)));
	}

	glBindTexture(GL_TEXTURE_2D, currentBinding); // revert the current binding

	GLuint fbo = 0;
	GLuint fboDepth = 0;

	if (tex.fbo != 0) {
		if (!GLEW_EXT_framebuffer_object) {
			glDeleteTextures(1, &texID);
			return std::string("");
		}
		GLint currentFBO;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);

		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		if (tex.fboDepth != 0) {
			glGenRenderbuffersEXT(1, &fboDepth);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fboDepth);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,
			                         tex.xsize, tex.ysize);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
			                             GL_RENDERBUFFER_EXT, fboDepth);
		}

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		                          tex.target, texID, 0);

		const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			glDeleteTextures(1, &texID);
			glDeleteFramebuffersEXT(1, &fbo);
			glDeleteRenderbuffersEXT(1, &fboDepth);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
			return std::string("");
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
	}

	char buf[64];
	SNPRINTF(buf, sizeof(buf), "%c%i", prefix, ++lastCode);

	Texture newTex = tex;
	newTex.id = texID;
	newTex.name = buf;
	newTex.fbo = fbo;
	newTex.fboDepth = fboDepth;

	if (freeIndices.empty()) {
		textureMap[newTex.name] = textureVec.size();
		textureVec.push_back(newTex);
	} else {
		// recycle
		textureMap[newTex.name] = freeIndices.back();
		textureVec[freeIndices.back()] = newTex;
		freeIndices.pop_back();
	}

	return newTex.name;
}


bool LuaTextures::Bind(const std::string& name) const
{	
	const auto it = textureMap.find(name);

	if (it != textureMap.end()) {
		const Texture& tex = textureVec[it->second];
		glBindTexture(tex.target, tex.id);
		return true;
	}
	return false;
}


bool LuaTextures::Free(const std::string& name)
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end()) {
		const Texture& tex = textureVec[it->second];
		glDeleteTextures(1, &tex.id);

		if (GLEW_EXT_framebuffer_object) {
			glDeleteFramebuffersEXT(1, &tex.fbo);
			glDeleteRenderbuffersEXT(1, &tex.fboDepth);
		}

		freeIndices.push_back(it->second);
		textureMap.erase(it);
		return true;
	}

	return false;
}


bool LuaTextures::FreeFBO(const std::string& name)
{
	if (!GLEW_EXT_framebuffer_object)
		return false;

	const auto it = textureMap.find(name);

	if (it == textureMap.end())
		return false;

	Texture& tex = textureVec[it->second];
	glDeleteFramebuffersEXT(1, &tex.fbo);
	glDeleteRenderbuffersEXT(1, &tex.fboDepth);
	tex.fbo = 0;
	tex.fboDepth = 0;
	return true;
}


void LuaTextures::FreeAll()
{
	for (auto it = textureMap.begin(); it != textureMap.end(); ++it) {
		const Texture& tex = textureVec[it->second];
		glDeleteTextures(1, &tex.id);
		if (GLEW_EXT_framebuffer_object) {
			glDeleteFramebuffersEXT(1, &tex.fbo);
			glDeleteRenderbuffersEXT(1, &tex.fboDepth);
		}
	}

	textureMap.clear();
	textureVec.clear();
	freeIndices.clear();
}


size_t LuaTextures::GetIdx(const std::string& name) const
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end())
		return (it->second);

	return (size_t(-1));
}

const LuaTextures::Texture* LuaTextures::GetInfo(const std::string& name) const
{
	const size_t idx = GetIdx(name);

	if (idx != size_t(-1))
		return &textureVec[idx];

	return nullptr;
}


/******************************************************************************/
/******************************************************************************/
