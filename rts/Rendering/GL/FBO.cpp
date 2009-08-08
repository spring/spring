/**
 * @file FBO.cpp
 * @brief EXT_framebuffer_object implementation
 *
 * EXT_framebuffer_object class implementation
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include <assert.h>
#include <vector>
#include "mmgr.h"

#include "FBO.h"
#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "ConfigHandler.h"
#include "Rendering/Textures/Bitmap.h"

std::vector<FBO*> FBO::fboList;
std::map<GLuint,FBO::TexData*> FBO::texBuf;


/**
 * Returns if the current gpu supports Framebuffer Objects
 */
bool FBO::IsSupported()
{
	return (GLEW_EXT_framebuffer_object);
}


/**
 * Detects the textureTarget just by the textureName/ID
 */
GLenum FBO::GetTextureTargetByID(const GLuint id, const unsigned int i)
{
	static GLenum _targets[ 4 ] = { GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_1D, GL_TEXTURE_3D };
	GLint format;
	glBindTexture(_targets[i],id);
	glGetTexLevelParameteriv(_targets[i], 0, GL_TEXTURE_INTERNAL_FORMAT, &format);

	if (format!=1) {
		return _targets[i];
	} else if (i<3) {
		return GetTextureTargetByID(id, i+1);
	} else	return -1;
}


/**
 * Makes a copy of a texture/RBO in the system ram
 */
void FBO::DownloadAttachment(const GLenum attachment)
{
	GLuint target;
	glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT, attachment,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT,
		(GLint*)&target);
	GLuint id;
	glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT, attachment,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,
		(GLint*)&id);

	if (target==GL_NONE || id==0)
		return;

	if (texBuf.find(id)!=texBuf.end())
		return;

	if (target==GL_TEXTURE) {
		target = GetTextureTargetByID(id);

		if (target<0)
			return;
	}

	struct FBO::TexData* tex = new FBO::TexData;
	tex->id = id;
	tex->target = target;

	int bits = 0;

	if (target==GL_RENDERBUFFER_EXT) {
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, id);
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT,  &tex->xsize);
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &tex->ysize);
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_INTERNAL_FORMAT_EXT, (GLint*)&tex->format);

		GLint _cbits;
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_RED_SIZE_EXT, &_cbits); bits += _cbits;
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_GREEN_SIZE_EXT, &_cbits); bits += _cbits;
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_BLUE_SIZE_EXT, &_cbits); bits += _cbits;
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_ALPHA_SIZE_EXT, &_cbits); bits += _cbits;
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_DEPTH_SIZE_EXT, &_cbits); bits += _cbits;
		glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_STENCIL_SIZE_EXT, &_cbits); bits += _cbits;
	} else {
		glBindTexture(target,id);

		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &tex->xsize);
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &tex->ysize);
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH, &tex->zsize);
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&tex->format);

		GLint _cbits;
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_RED_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_SIZE, &_cbits); bits += _cbits;
	}

	if (configHandler->Get("AtiSwapRBFix",false)) {
		if (tex->format == GL_RGBA) {
			tex->format = GL_BGRA;
		} else if (tex->format == GL_RGB) {
			tex->format = GL_BGR;
		}
	}

	switch (target) {
		case GL_TEXTURE_3D:
			tex->pixels = new unsigned char[tex->xsize*tex->ysize*tex->zsize*(bits/8)];
			glGetTexImage(tex->target,0,/*FIXME*/GL_RGBA,/*FIXME*/GL_UNSIGNED_BYTE, tex->pixels);
			break;
		case GL_TEXTURE_1D:
			tex->pixels = new unsigned char[tex->xsize*(bits/8)];
			glGetTexImage(tex->target,0,/*FIXME*/GL_RGBA,/*FIXME*/GL_UNSIGNED_BYTE, tex->pixels);
			break;
		case GL_RENDERBUFFER_EXT:
			tex->pixels = new unsigned char[tex->xsize*tex->ysize*(bits/8)];
			glReadBuffer(attachment);
			glReadPixels(0, 0, tex->xsize, tex->ysize, /*FIXME*/GL_RGBA, /*FIXME*/GL_UNSIGNED_BYTE, tex->pixels);
			break;
		default: //GL_TEXTURE_2D & GL_TEXTURE_RECTANGLE
			tex->pixels = new unsigned char[tex->xsize*tex->ysize*(bits/8)];
			glGetTexImage(tex->target,0,/*FIXME*/GL_RGBA,/*FIXME*/GL_UNSIGNED_BYTE, tex->pixels);
	}
	texBuf[id] = tex;
}

/**
 * @brief GLContextLost
 */
void FBO::GLContextLost()
{
	if (!IsSupported()) return;

	GLint oldReadBuffer;

	for (std::vector<FBO*>::iterator fi=fboList.begin(); fi!=fboList.end(); ++fi) {
		if ((*fi)->reloadOnAltTab) {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (*fi)->fboId);
			glGetIntegerv(GL_READ_BUFFER,&oldReadBuffer);

			for(int i = 0; i < 15; ++i) {
				DownloadAttachment(GL_COLOR_ATTACHMENT0_EXT + i);
			}
			DownloadAttachment(GL_DEPTH_ATTACHMENT_EXT);
			DownloadAttachment(GL_STENCIL_ATTACHMENT_EXT);

			glReadBuffer(oldReadBuffer);
		}
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


/**
 * @brief GLContextReinit
 */
void FBO::GLContextReinit()
{
	if (!IsSupported()) return;

	for (std::map<GLuint,FBO::TexData*>::iterator ti=texBuf.begin(); ti!=texBuf.end(); ++ti) {
		FBO::TexData* tex = ti->second;

		if (glIsTexture(tex->id)) {
			glBindTexture(tex->target,tex->id);
			//todo: regen mipmaps?
			switch (tex->target) {
				case GL_TEXTURE_3D:
					//glTexSubImage3D(tex->target, 0, 0,0,0, tex->xsize, tex->ysize, tex->zsize, /*FIXME?*/GL_RGBA, /*FIXME?*/GL_UNSIGNED_BYTE, tex->pixels);
					glTexImage3D(tex->target, 0, tex->format, tex->xsize, tex->ysize, tex->zsize, 0, /*FIXME?*/GL_RGBA, /*FIXME?*/GL_UNSIGNED_BYTE, tex->pixels);
					break;
				case GL_TEXTURE_1D:
					//glTexSubImage1D(tex->target, 0, 0, tex->xsize, /*FIXME?*/GL_RGBA, /*FIXME?*/GL_UNSIGNED_BYTE, tex->pixels);
					glTexImage1D(tex->target, 0, tex->format, tex->xsize, /*FIXME?*/GL_RGBA, 0, /*FIXME?*/GL_UNSIGNED_BYTE, tex->pixels);
					break;
				default: //GL_TEXTURE_2D & GL_TEXTURE_RECTANGLE
					//glTexSubImage2D(tex->target, 0, 0,0, tex->xsize, tex->ysize, /*FIXME?*/GL_RGBA, /*FIXME?*/GL_UNSIGNED_BYTE, tex->pixels);
					glTexImage2D(tex->target, 0, tex->format, tex->xsize, tex->ysize, 0, /*FIXME?*/GL_RGBA, /*FIXME?*/GL_UNSIGNED_BYTE, tex->pixels);
			}
		}else if (glIsRenderbufferEXT(tex->id)) {
			//FIXME
		}

		delete[] tex->pixels;
		delete tex;
	}
	texBuf.clear();
}


/**
 * Tests for support of the EXT_framebuffer_object
 * extension, and generates a framebuffer if supported
 */
FBO::FBO() : fboId(0), reloadOnAltTab(false)
{
	if (!IsSupported()) return;

	glGenFramebuffersEXT(1,&fboId);

	// we need to bind it once, else it isn't valid
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	fboList.push_back(this);
}


/**
 * Unbinds the framebuffer and deletes it
 */
FBO::~FBO()
{
	if (!IsSupported()) return;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if (fboId)
		glDeleteFramebuffersEXT(1, &fboId);

	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	for (std::vector<GLuint>::iterator ri=myRBOs.begin(); ri!=myRBOs.end(); ++ri) {
		glDeleteRenderbuffersEXT(1, &(*ri));
	}

	for (std::vector<FBO*>::iterator fi=fboList.begin(); fi!=fboList.end(); ++fi) {
		if (*fi==this) {
			fboList.erase(fi);
			break;
		}
	}

	// seems the application exits and we are the last fbo left
	// so we delete the remaining alloc'ed stuff
	if (fboList.empty()) {
		for (std::map<GLuint,FBO::TexData*>::iterator ti=texBuf.begin(); ti!=texBuf.end(); ++ti) {
			FBO::TexData* tex = ti->second;
			delete[] tex->pixels;
			delete tex;
		}
		texBuf.clear();
	}
}


/**
 * Tests whether or not if we have a valid framebuffer
 */
bool FBO::IsValid()
{
	return (fboId!=0);
}


/**
 * Makes the framebuffer the active framebuffer context
 */
void FBO::Bind(void)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);
}


/**
 * Unbinds the framebuffer from the current context
 */
void FBO::Unbind()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


/**
 * Tests if the framebuffer is a complete and
 * legitimate framebuffer
 */
bool FBO::CheckStatus(std::string name)
{
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			return true;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			logOutput.Print("FBO-"+name+": has no images/buffers attached!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			logOutput.Print("FBO-"+name+": missing a required image/buffer attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			logOutput.Print("FBO-"+name+": has mismatched image/buffer dimensions!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			logOutput.Print("FBO-"+name+": colorbuffer attachments have different types!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			logOutput.Print("FBO-"+name+": incomplete draw buffers!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			logOutput.Print("FBO-"+name+": trying to read from a non-attached color buffer!");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			logOutput.Print("FBO-"+name+" error: GL_FRAMEBUFFER_UNSUPPORTED_EXT");
			break;
		default:
			logOutput.Print(std::string("FBO-"+name+" error: 0x%X").c_str(),status);
			break;
	}
	return false;
}


/**
 * Returns the current framebuffer status
 */
GLenum FBO::GetStatus()
{
	return glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
}


/**
 * Attaches a GL texture to the framebuffer
 */
void FBO::AttachTexture(const GLuint texId, const GLenum texTarget, const GLenum attachment, const int mipLevel, const int zSlice )
{
	if (texTarget == GL_TEXTURE_1D) {
		glFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_1D, texId, mipLevel);
	} else if (texTarget == GL_TEXTURE_3D) {
		glFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_3D, texId, mipLevel, zSlice);
	} else {
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, texTarget, texId, mipLevel);
	}
}


/**
 * Attaches a GL RenderBuffer to the framebuffer
 */
void FBO::AttachRenderBuffer(const GLuint rboId, const GLenum attachment)
{
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, rboId);
}


/**
 * Unattaches an attachment from the framebuffer
 */
void FBO::Unattach(const GLenum attachment)
{
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, 0, 0);
}


/**
 * Unattaches any attachments from the framebuffer
 */
void FBO::UnattachAll()
{
	for(int i = 0; i < 15; ++i) {
		Unattach(GL_COLOR_ATTACHMENT0_EXT + i);
	}
	Unattach(GL_DEPTH_ATTACHMENT_EXT);
	Unattach(GL_STENCIL_ATTACHMENT_EXT);
}


/**
 * Creates and attaches a RBO
 */
void FBO::CreateRenderBuffer(const GLenum attachment, const GLenum format, const GLsizei width, const GLsizei height)
{
	GLuint rbo;
	glGenRenderbuffersEXT(1, &rbo);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbo);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, format, width, height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, rbo);
	myRBOs.push_back(rbo);
}
