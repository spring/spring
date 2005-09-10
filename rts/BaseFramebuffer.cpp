/*
 * BaseFramebuffer.cpp
 * Offscreen buffer base class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "BaseFramebuffer.h"
#include "FBOFramebuffer.h"
#ifdef _WIN32
#include "WGLPBufferFramebuffer.h"
#else
#include "SGIXPBufferFramebuffer.h"
#include "GLXPBufferFramebuffer.h"
#endif

BaseFramebuffer::BaseFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h)
{
	initialized = false;
	active = false;
	type = t;
	width = w;
	height = h;
}

BaseFramebuffer::~BaseFramebuffer()
{
}

BaseFramebuffer *BaseFramebuffer::initialize(const unsigned int t, const unsigned int w, const unsigned int h)
{
	if (GL_EXT_framebuffer_object)
		return new FBOFramebuffer(t,w,h);
#ifdef _WIN32
	if (WGLEW_ARB_pbuffer)
		return new WGLPBufferFramebuffer(t,w,h);
#else
	if (GLX_SGIX_pbuffer)
		return new SGIXPBufferFramebuffer(t,w,h);
	int errorBase;
	int eventBase;
	if (glXQueryExtension(glXGetCurrentDisplay(),&errorBase,&eventBase))
		return new GLXPBufferFramebuffer(t,w,h);
#endif
//	return new EmulFramebuffer(t,w,h);
}

unsigned int BaseFramebuffer::getWidth()
{
	return width;
}

unsigned int BaseFramebuffer::getHeight()
{
	return height;
}

unsigned int BaseFramebuffer::getType()
{
	return type;
}

unsigned int BaseFramebuffer::getTexture()
{
	return texture;
}

void BaseFramebuffer::texinit()
{
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	unsigned int t;
	switch (type) {
		case FBTYPE_RGB:
			break;
		case FBTYPE_RGBA:
			break;
		case FBTYPE_DEPTH:
			t = GL_DEPTH_COMPONENT;
			break;
		case FBTYPE_STENCIL:
			break;
		default:
			break;
	};
	glTexImage2D(GL_TEXTURE_2D, 0, t, width, height, 0, t, GL_UNSIGNED_BYTE, NULL);
}

void BaseFramebuffer::texuninit()
{
	glDeleteTextures(1,&texture);
}
