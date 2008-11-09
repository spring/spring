/*
 * IFramebuffer.h
 * IFramebuffer interface
 * Framebuffer abstraction by Jelmer Cnossen
 */
#ifndef _IFRAMEBUFFER_H
#define _IFRAMEBUFFER_H

#include "myGL.h"

enum FramebufferAttachType
{
	FBO_ATTACH_COLOR = 1,
	FBO_ATTACH_DEPTH = 2,
};

class IFramebuffer
{
public:
	virtual ~IFramebuffer();

	virtual bool checkFBOStatus(void) = 0;
	virtual void attachTexture(const GLuint tex, const unsigned int textype, FramebufferAttachType attachtype) = 0;
	virtual void select(void) = 0;
	virtual void deselect(void) = 0;
	virtual bool valid(void) = 0;
};

enum FramebufferProperties
{
	FBO_NEED_DEPTH = 1, // zbuffering is needed, but only for rendering
	FBO_NEED_DEPTH_TEXTURE = 2, // for shadow mapping
	FBO_NEED_COLOR = 4
};

IFramebuffer* instantiate_fb(const int w, const int h, const int requires);

#endif /* _IFRAMEBUFFER_H */
