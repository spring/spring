/*
 * WGLPBufferFramebuffer.h
 * WGL PBuffer object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef WGLPBUFFERFRAMEBUFFER_H
#define WGLPBUFFERFRAMEBUFFER_H

#include "BaseFramebuffer.h"

class WGLPBufferFramebuffer: public BaseFramebuffer
{
public:
	WGLPBufferFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h);
	~WGLPBufferFramebuffer();
	virtual bool init();
	virtual bool uninit();
	virtual bool select();
	virtual bool deselect();
private:
	HPBUFFERARB hPBuffer;
	HDC hDCPBuffer;
	HGLRC hRCPBuffer;
};

#endif /* WGLPBUFFERFRAMEBUFFER_H */
