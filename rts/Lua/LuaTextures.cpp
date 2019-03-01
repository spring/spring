/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaTextures.h"
#include "Rendering/GlobalRendering.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


/******************************************************************************/
/******************************************************************************/

std::string LuaTextures::Create(const Texture& tex)
{	
	GLint currentBinding;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentBinding);

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(tex.target, texID);

	glClearErrors("LuaTex", __func__, globalRendering->glDebugErrors);

	GLenum dataFormat = GL_RGBA;
	GLenum dataType   = GL_UNSIGNED_BYTE;

	switch (tex.format) {
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32: {
			dataFormat = GL_DEPTH_COMPONENT;
			dataType = GL_FLOAT;
		} break;
		default: {
		} break;
	}

	switch (tex.target) {
		case GL_TEXTURE_2D_MULTISAMPLE: {
			assert(tex.samples > 1);
			glTexImage2DMultisample(tex.target, tex.samples, tex.format, tex.xsize, tex.ysize, GL_TRUE);
		} break;
		case GL_TEXTURE_2D: {
			glTexImage2D(tex.target, 0, tex.format, tex.xsize, tex.ysize, tex.border, dataFormat, dataType, nullptr);
		} break;
		default: {
			LOG_L(L_ERROR, "[LuaTextures::%s] texture-target %d is not supported yet", __func__, tex.target);
		} break;
	}


	if (glGetError() != GL_NO_ERROR) {
		glDeleteTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, currentBinding);
		return "";
	}

	ApplyParams(tex);

	glBindTexture(GL_TEXTURE_2D, currentBinding); // revert the current binding

	GLuint fbo = 0;
	GLuint fboDepth = 0;

	if (tex.fbo != 0) {
		GLint currentFBO;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		if (tex.fboDepth != 0) {
			glGenRenderbuffers(1, &fboDepth);
			glBindRenderbuffer(GL_RENDERBUFFER, fboDepth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, tex.xsize, tex.ysize);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepth);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.target, texID, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			glDeleteTextures(1, &texID);
			glDeleteFramebuffers(1, &fbo);
			glDeleteRenderbuffers(1, &fboDepth);
			glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
			return "";
		}

		glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
	}

	char buf[64];
	SNPRINTF(buf, sizeof(buf), "%c%i", prefix, ++lastCode);

	Texture newTex = tex;
	newTex.id = texID;
	newTex.fbo = fbo;
	newTex.fboDepth = fboDepth;

	if (freeIndices.empty()) {
		textureMap.insert(buf, textureVec.size());
		textureVec.emplace_back(newTex);
		return buf;
	}

	// recycle
	textureMap[buf] = freeIndices.back();
	textureVec[freeIndices.back()] = newTex;
	freeIndices.pop_back();
	return buf;
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

		glDeleteFramebuffers(1, &tex.fbo);
		glDeleteRenderbuffers(1, &tex.fboDepth);

		freeIndices.push_back(it->second);
		textureMap.erase(it);
		return true;
	}

	return false;
}


bool LuaTextures::FreeFBO(const std::string& name)
{
	const auto it = textureMap.find(name);

	if (it == textureMap.end())
		return false;

	Texture& tex = textureVec[it->second];

	glDeleteFramebuffers(1, &tex.fbo);
	glDeleteRenderbuffers(1, &tex.fboDepth);

	tex.fbo = 0;
	tex.fboDepth = 0;
	return true;
}


void LuaTextures::FreeAll()
{
	for (const auto& item: textureMap) {
		const Texture& tex = textureVec[item.second];
		glDeleteTextures(1, &tex.id);

		glDeleteFramebuffers(1, &tex.fbo);
		glDeleteRenderbuffers(1, &tex.fboDepth);
	}

	textureMap.clear();
	textureVec.clear();
	freeIndices.clear();
}


void LuaTextures::ApplyParams(const Texture& tex) const
{
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_S, tex.wrap_s);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_T, tex.wrap_t);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_R, tex.wrap_r);
	glTexParameteri(tex.target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
	glTexParameteri(tex.target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);
	glTexParameteri(tex.target, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	if (tex.aniso != 0.0f)
		glTexParameterf(tex.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Clamp(tex.aniso, 1.0f, globalRendering->maxTexAnisoLvl));
}

void LuaTextures::ChangeParams(const Texture& tex)  const
{
	GLint currentBinding = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentBinding);
	glBindTexture(tex.target, tex.id);
	ApplyParams(tex);
	glBindTexture(GL_TEXTURE_2D, currentBinding); // revert the current binding
}


size_t LuaTextures::GetIdx(const std::string& name) const
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end())
		return (it->second);

	return (size_t(-1));
}

