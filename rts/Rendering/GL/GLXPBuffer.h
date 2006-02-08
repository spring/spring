/*
 * GLXPBuffer.h
 * GLX_SGIX_pbuffer class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef _GLXPBUFFER_H
#define _GLXPBUFFER_H

#ifdef _WIN32
#error This file should not be compiled on windows
#endif

#include <GL/glxew.h>

class GLXPBuffer: public IFramebuffer
{
public:
	GLXPBuffer(const int shadowMapSize, const bool sharedcontext = true, const bool shareobjects = true);
	~GLXPBuffer();
	void checkFBOStatus(void);
	void attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype);
	void select(void);
	void deselect(void);
	bool valid(void);
protected:
	Display *m_pDisplay;
	GLXPbuffer m_glxPbuffer;
	GLXContext m_glxContext;
	
	Display *m_pOldDisplay;
	GLXPbuffer m_glxOldDrawable;
	GLXContext m_glxOldContext;

	bool m_bSharedContext;
	bool m_bShareObjects;

	int shadowMapSize;
	unsigned int shadowTex;
};

#endif /* _GLXPBUFFER_H */
