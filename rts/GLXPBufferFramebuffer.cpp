/*
 * GLXPBufferFramebuffer.cpp
 * GLX PBuffer object class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "GLXPBufferFramebuffer.h"

GLXPBufferFramebuffer::GLXPBufferFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h): BaseFramebuffer(t,w,h)
{
	g_pDisplay = glXGetCurrentDisplay();
	g_window = glXGetCurrentDrawable();
}

GLXPBufferFramebuffer::~GLXPBufferFramebuffer()
{
	if (active)
		deselect();
	if (initialized)
		uninit();
}

bool GLXPBufferFramebuffer::init()
{
	if (initialized)
		return false;
	texinit(texture);
	int attrib[] = {
		GLX_DOUBLEBUFFER, False,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
		None
	};
	int pbufAttrib[] = {
		GLX_PBUFFER_WIDTH, width,
		GLX_PBUFFER_HEIGHT, height,
		GLX_LARGEST_PBUFFER, True,
		None
	};
	g_windowContext = glXGetCurrentContext();
	int scrnum = DefaultScreen(g_pDisplay);
	GLXFBConfig *fbconfig;
	XVisualInfo *visinfo;
	int nitems;
	fbconfig = glXChooseFBConfig(g_pDisplay, scrnum, attrib, &nitems);
	if (fbconfig == NULL)
		return false;
	g_pbuffer = glXCreatePbuffer(g_pDisplay, fbconfig[0], pbufAttrib);
	visinfo = glXGetVisualFromFBConfig(g_pDisplay, fbconfig[0]);
	if (!visinfo) {
		XFree(fbconfig);
		return false;
	}
	g_pbufferContext = glXCreateContext(g_pDisplay, visinfo, g_windowContext, GL_TRUE);
	if (!g_pbufferContext) {
		XFree(fbconfig);
		XFree(visinfo);
		return false;
	}
	XFree(fbconfig);
	XFree(visinfo);
	initialized = true;
	return true;
}

bool GLXPBufferFramebuffer::uninit()
{
	if (!initialized)
		return false;
	texuninit(texture);
	glXDestroyContext(g_pDisplay, g_pbufferContext);
	glXDestroyPbuffer(g_pDisplay, g_pbuffer);
	if (g_windowContext != NULL)
		glXMakeCurrent(g_pDisplay, g_window, g_windowContext);
	initialized = false;
	return true;
}

bool GLXPBufferFramebuffer::select()
{
	if (!initialized || active)
		return false;
	glXMakeCurrent(g_pDisplay, g_pbuffer, g_pbufferContext);
	active = true;
	return true;
}

bool GLXPBufferFramebuffer::deselect()
{
	if (!initialized || !active)
		return false;
	glXMakeCurrent(g_pDisplay, g_window, g_windowContext);
	active = false;
	return true;
}

