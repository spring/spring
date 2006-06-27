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
	FBO_ATTACH_COLOR,
	FBO_ATTACH_DEPTH
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
#ifdef _WIN32
#include "WinPBuffer.h"
#elif defined(__APPLE__)
// No native apple PBuffer support yet...just stick with the FBOs for now.
#else
#include "GLXPBuffer.h"
#endif

static inline IFramebuffer* instantiate_fb(const int shadowMapSize)
{
	if (GLEW_EXT_framebuffer_object) {
		info->AddLine("Using EXT_framebuffer_object");
		return new FBO();
	}
#ifdef _WIN32
	else if (WGLEW_ARB_pbuffer) {
		info->AddLine("Using WGLEW_ARB_pbuffer");
		return new WinPBuffer(shadowMapSize);
	}
#elif defined(__APPLE__)
	// No native apple PBuffer support yet...just stick with the FBOs for now.
#else
	else if (GLXEW_SGIX_pbuffer) {
		info->AddLine("Using GLX_SGIX_pbuffer");
		return new GLXPBuffer(shadowMapSize,false);
	}
#endif
	info->AddLine("No supported pixel buffer found");
	return NULL;
}

#endif /* _IFRAMEBUFFER_H */
