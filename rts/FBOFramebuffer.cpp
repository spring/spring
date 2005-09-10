/*
 * FBOFramebuffer.cpp
 * EXT_framebuffer_object class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "FBOFramebuffer.h"

FBOFramebuffer::FBOFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h): BaseFramebuffer(t,w,h)
{
}

FBOFramebuffer::~FBOFramebuffer()
{
	if (active)
		deselect();
	if (initialized)
		uninit();
}

bool FBOFramebuffer::init()
{
	if (initialized)
		return false;
	glGenFramebuffersEXT(1,&g_frameBuffer);
	glGenRenderbuffersEXT(1,&g_depthRenderBuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, g_depthRenderBuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return false;
	texinit();
	initialized = true;
	return true;
}

bool FBOFramebuffer::uninit()
{
	if (!initialized)
		return false;
	texuninit();
	glDeleteRenderbuffersEXT(1,&g_depthRenderBuffer);
	glDeleteFramebuffersEXT(1,&g_frameBuffer);
	initialized = false;
	return true;
}

bool FBOFramebuffer::select()
{
	if (!initialized || active)
		return false;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frameBuffer);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture, 0);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, g_depthRenderBuffer);
	active = true;
	return true;
}

bool FBOFramebuffer::deselect()
{
	if (!initialized || !active)
		return false;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	active = false;
	return true;
}

