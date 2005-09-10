/*
 * FBOFramebuffer.h
 * EXT_framebuffer_object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef FBOFRAMEBUFFER_H
#define FBOFRAMEBUFFER_H

#include "glew.h"
#include "BaseFramebuffer.h"

class FBOFramebuffer: public BaseFramebuffer
{
public:
	FBOFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h);
	~FBOFramebuffer();
	virtual bool init();
	virtual bool uninit();
	virtual bool select();
	virtual bool deselect();
private:
	GLuint g_frameBuffer;
	GLuint g_depthRenderBuffer;
};

#endif /* FBOFRAMEBUFFER_H */
