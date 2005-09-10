/*
 * SGIXPBufferFramebuffer.cpp
 * SGIX PBuffer object class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "SGIXPBufferFramebuffer.h"

SGIXPBufferFramebuffer::SGIXPBufferFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h): BaseFramebuffer(t,w,h)
{
	g_pDisplay = glXGetCurrentDisplay();
	g_window = glXGetCurrentDrawable();
}

SGIXPBufferFramebuffer::~SGIXPBufferFramebuffer()
{
	if (active)
		deselect();
	if (initialized)
		uninit();
}

bool SGIXPBufferFramebuffer::init()
{
	if (initialized)
		return false;
	texinit();
	int attrib[] = {
		GLX_DOUBLEBUFFER, False,
		GLX_RENDER_TYPE_SGIX, GLX_RGBA_BIT_SGIX,
		GLX_DRAWABLE_TYPE_SGIX, GLX_PBUFFER_BIT_SGIX | GLX_WINDOW_BIT_SGIX,
		None
	};
	int pbufAttrib[] = {
		GLX_LARGEST_PBUFFER_SGIX, True,
		GLX_PRESERVED_CONTENTS_SGIX, False,
		None
	};
	g_windowContext = glXGetCurrentContext();
	int scrnum = DefaultScreen(g_pDisplay);
	GLXFBConfigSGIX *fbconfig;
	XVisualInfo *visinfo;
	int nitems;
	fbconfig = glXChooseFBConfigSGIX(g_pDisplay, scrnum, attrib, &nitems);
	if (fbconfig == NULL)
		return false;
	g_pbuffer = glXCreateGLXPbufferSGIX(g_pDisplay, fbconfig[0], width, height, pbufAttrib);
	visinfo = glXGetVisualFromFBConfigSGIX(g_pDisplay, fbconfig[0]);
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

bool SGIXPBufferFramebuffer::uninit()
{
	if (!initialized)
		return false;
	texuninit();
	glXDestroyGLXPbufferSGIX(g_pDisplay,g_pbuffer);
	initialized = false;
	return true;
}

bool SGIXPBufferFramebuffer::select()
{
	if (!initialized || active)
		return false;
	glXMakeCurrent(g_pDisplay, g_pbuffer, g_pbufferContext);
	active = true;
	return true;
}

bool SGIXPBufferFramebuffer::deselect()
{
	if (!initialized || !active)
		return false;
	glXMakeCurrent(g_pDisplay, g_window, g_windowContext);
	active = false;
	return true;
}

