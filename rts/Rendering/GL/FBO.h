/*
 * FBO.h
 * IFramebuffer interface + EXT_framebuffer_object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 *
 * Framebuffer abstraction added by Jelmer Cnossen
 */
#ifndef FBO_H
#define FBO_H

#include <vector>
#include <GL/glew.h>

enum FramebufferAttachType
{
	FBO_ATTACH_COLOR,
	FBO_ATTACH_DEPTH
};

class IFramebuffer
{
public:
	virtual ~IFramebuffer() {}
    
	virtual void checkFBOStatus(void) = 0;
	virtual void attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype) = 0;
	virtual void select(void) = 0;
	virtual void deselect(void) = 0;
	virtual bool valid(void) = 0;
};

class FBO : public IFramebuffer
{
public:
	FBO();
	~FBO();
	void checkFBOStatus(void);
	void attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype);
	void select(void);
	void deselect(void);
	bool valid(void);
private:
	GLuint g_frameBuffer;
};

#endif /* FBO_H */
