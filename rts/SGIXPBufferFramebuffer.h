/*
 * SGIXPBufferFramebuffer.h
 * SGIX PBuffer object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef SGIXPBUFFERFRAMEBUFFER_H
#define SGIXPBUFFERFRAMEBUFFER_H

#include "BaseFramebuffer.h"
#include "glxew.h"

class SGIXPBufferFramebuffer: public BaseFramebuffer
{
public:
	SGIXPBufferFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h);
	~SGIXPBufferFramebuffer();
	virtual bool init();
	virtual bool uninit();
	virtual bool select();
	virtual bool deselect();
private:
	Display *g_pDisplay;
	Window g_window;
	GLXContext g_windowContext;
	GLXContext g_pbufferContext;
	GLXPbufferSGIX g_pbuffer;
};

#endif /* SGIXPBUFFERFRAMEBUFFER_H */
