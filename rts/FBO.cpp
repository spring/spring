/*
 * FBO.cpp
 * EXT_framebuffer_object class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "FBO.h"

FBO::FBO()
{
	g_frameBuffer = 0;
	if (!GL_EXT_framebuffer_object)
		return;
	glGenFramebuffersEXT(1,&g_frameBuffer);
	select();
	// Other init stuff
	deselect();
}

FBO::~FBO()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if (g_frameBuffer)
		glDeleteFramebuffersEXT(1,&g_frameBuffer);
}

bool FBO::valid(void)
{
	return g_frameBuffer != 0;
}

void FBO::select(void)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_frameBuffer);
}

void FBO::deselect(void)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void FBO::checkFBOStatus(void)
{
	GLenum status;
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		assert(0);
}

void FBO::attachTexture(GLuint tex, const unsigned int textype, const unsigned int attachtype)
{
	select();
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachtype, textype, tex, 0);
	deselect();
}

