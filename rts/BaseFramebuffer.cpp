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

BaseFramebuffer *BaseFramebuffer::resolvetype(const unsigned int t, const unsigned int w, const unsigned int h, const unsigned int buffertype)
{
	if (buffertype == BUFFER_FBO) {
		if (GL_EXT_framebuffer_object)
			return new FBOFramebuffer(t,w,h);
		else
			return NULL;
	} else if (buffertype == BUFFER_WGLPBUFFER) {
#ifdef _WIN32
		if (WGLEW_ARB_pbuffer)
			return new WGLPBufferFramebuffer(t,w,h);
		else
#endif
			return NULL;
	} else if (buffertype == BUFFER_SGIXPBUFFER) {
#ifndef _WIN32
		if (GLX_SGIX_pbuffer)
			return new SGIXPBufferFramebuffer(t,w,h);
		else
#endif
			return NULL;
	} else if (buffertype == BUFFER_GLXPBUFFER) {
#ifndef _WIN32
		int errorBase;
		int eventBase;
		if (glXQueryExtension(glXGetCurrentDisplay(),&errorBase,&eventBase))
			return new GLXPBufferFramebuffer(t,w,h);
		else
#endif
			return NULL;
	}
//	else if (buffertype == BUFFER_EMUL)
//		return new EmulFramebuffer(t,w,h);
	else
		return NULL;
}

BaseFramebuffer *BaseFramebuffer::initialize(const unsigned int t, const unsigned int w, const unsigned int h, const unsigned int buffertype)
{
	BaseFramebuffer *instance = resolvetype(t,w,h,buffertype);
	if (instance != NULL)
		return instance;
	else {
		unsigned int *buflist = {0};
#ifdef _WIN32
		buflist = windows_preferred_buffers;
#elif defined(__linux__)
		buflist = linux_preferred_buffers;
#endif
		int i = 0;
		while (buflist[i]) {
			instance = resolvetype(t,w,h,buflist[i++]);
			if (instance != NULL)
				return instance;
		}
	}
	return NULL;
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
