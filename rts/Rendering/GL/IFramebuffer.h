/*
 * IFramebuffer.h
 * IFramebuffer interface
 * Framebuffer abstraction by Jelmer Cnossen
 */
#ifndef _IFRAMEBUFFER_H
#define _IFRAMEBUFFER_H

#include <GL/glew.h>
enum FramebufferAttachType
{
	FBO_ATTACH_COLOR = 1,
	FBO_ATTACH_DEPTH = 2,
};

class IFramebuffer
{
public:
	virtual ~IFramebuffer();
    
	virtual void checkFBOStatus(void) = 0;
	virtual void attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype) = 0;
	virtual void select(void) = 0;
	virtual void deselect(void) = 0;
	virtual bool valid(void) = 0;
};

#include "Game/UI/InfoConsole.h"
#include "FBO.h"

enum FramebufferProperties
{
	FBO_NEED_DEPTH, // zbuffering is needed, but only for rendering
	FBO_NEED_DEPTH_TEXTURE, // for shadow mapping
	FBO_NEED_COLOR 
};

static inline IFramebuffer* instantiate_fb(const int w, const int h, int requires)
{
	if (GLEW_EXT_framebuffer_object) {
		info->AddLine("Using EXT_framebuffer_object");
		return new FBO(requires, w, h);
	}
	info->AddLine("No supported pixel buffer found");
	return NULL;
}

#endif /* _IFRAMEBUFFER_H */
