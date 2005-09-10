/*
 * BaseFramebuffer.h
 * Offscreen buffer base class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef BASEFRAMEBUFFER_H
#define BASEFRAMEBUFFER_H

#define FBTYPE_RGB 1
#define FBTYPE_RGBA 2
#define FBTYPE_DEPTH 3
#define FBTYPE_STENCIL 4

class BaseFramebuffer
{
public:
	BaseFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h);
	virtual ~BaseFramebuffer();
	static BaseFramebuffer *initialize(const unsigned int t, const unsigned int w, const unsigned int h);
	virtual unsigned int getWidth();
	virtual unsigned int getHeight();
	virtual unsigned int getType();
	virtual unsigned int getTexture();
	virtual bool init() = 0;
	virtual bool uninit() = 0;
	virtual void texinit();
	virtual void texuninit();
	virtual bool select() = 0;
	virtual bool deselect() = 0;
	bool initialized;
	bool active;
protected:
	unsigned int type;
	unsigned int width;
	unsigned int height;
	unsigned int texture;
};

#endif /* BASEFRAMEBUFFER_H */
