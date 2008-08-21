/**
 * @file FBO.cpp
 * @brief EXT_framebuffer_object implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * EXT_framebuffer_object class implementation
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include <assert.h>
#include <vector>
#include "mmgr.h"

#include "IFramebuffer.h"
#include "FBO.h"
#include "LogOutput.h"


IFramebuffer::~IFramebuffer()
{
}


IFramebuffer* instantiate_fb(const int w, const int h, const int requires)
{
	if (GLEW_EXT_framebuffer_object) {
		//logOutput.Print("Using EXT_framebuffer_object");
		return SAFE_NEW FBO(requires, w, h);
	}
	//logOutput.Print("No supported pixel buffer found");
	return NULL;
}


/**
 * Tests for support of the EXT_framebuffer_object
 * extension, and generates a framebuffer if supported
 */
FBO::FBO(int requires, int w, int h) : frameBuffer(0), depthRenderBuffer(0), requires(requires)
{
	glGenFramebuffersEXT(1,&frameBuffer);
	// Is a depth renderbuffer needed?
	if ((requires & FBO_NEED_DEPTH) && !(requires & FBO_NEED_DEPTH_TEXTURE))
	{
		select();
		glGenRenderbuffersEXT(1, &depthRenderBuffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthRenderBuffer);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
										GL_RENDERBUFFER_EXT, depthRenderBuffer);
		deselect();
	}
}

/**
 * Unbinds the framebuffer and deletes it
 */
FBO::~FBO()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if (frameBuffer)
		glDeleteFramebuffersEXT(1,&frameBuffer);

	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	if (depthRenderBuffer)
		glDeleteRenderbuffersEXT(1, &depthRenderBuffer);
}

/**
 * Tests whether or not we have a valid framebuffer
 */
bool FBO::valid(void)
{
	return frameBuffer != 0;
}

/**
 * Makes the framebuffer the active framebuffer context
 */
void FBO::select(void)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBuffer);
}

/**
 * Unbinds the framebuffer from the current context
 */
void FBO::deselect(void)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

/**
 * Tests if the framebuffer is a complete and
 * legitimate framebuffer
 */
bool FBO::checkFBOStatus(void)
{
	GLenum status;
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		//logOutput.Print("FBO is not GL_FRAMEBUFFER_COMPLETE_EXT: 0x%X", status);
		assert(false);
		return false;
	}
	return true;
}

/**
 * Attaches a GL texture to the framebuffer
 */
void FBO::attachTexture(GLuint tex, const unsigned int textype, FramebufferAttachType attachtype)
{
	GLenum glattachtype;

	switch (attachtype)
	{
		case FBO_ATTACH_DEPTH:
			assert (requires & FBO_NEED_DEPTH_TEXTURE);
			glattachtype = GL_DEPTH_ATTACHMENT_EXT;
			break;
		case FBO_ATTACH_COLOR:
			assert (requires & FBO_NEED_COLOR);
			glattachtype = GL_COLOR_ATTACHMENT0_EXT;
			break;
		default:
			assert(false);
			break;
	}
	select();
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, glattachtype, textype, tex, 0);
	deselect();
}
