/*
 * GLXPBufferFramebuffer.h
 * GLX PBuffer object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef GLXPBUFFERFRAMEBUFFER_H
#define GLXPBUFFERFRAMEBUFFER_H

#include <GL/glx.h>
#include "BaseFramebuffer.h"

class GLXPBufferFramebuffer: public BaseFramebuffer
{
public:
	GLXPBufferFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h);
	~GLXPBufferFramebuffer();
	virtual bool init();
	virtual bool uninit();
	virtual bool select();
	virtual bool deselect();
private:
	Display *g_pDisplay;
	Window g_window;
	GLXContext g_windowContext;
	GLXContext g_pbufferContext;
	GLXPbuffer g_pbuffer;
};

#endif /* GLXPBUFFERFRAMEBUFFER_H */
