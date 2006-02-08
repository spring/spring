/*
 * GLXPBuffer.cpp
 * GLX_SGIX_pbuffer class implementation
 * Copyright (C) 2006 Christopher Han <xiphux@gmail.com>
 */
#include "IFramebuffer.h"

GLXPBuffer::GLXPBuffer(const int shadowMapSize, const bool sharedcontext, const bool shareobjects): m_pDisplay(0), m_glxPbuffer(0), m_glxContext(0), m_pOldDisplay(0), m_glxOldDrawable(0), m_glxOldContext(0), shadowMapSize(shadowMapSize), m_bSharedContext(sharedcontext), m_bShareObjects(shareobjects)
{
	if (!GLX_SGIX_pbuffer)
		return;
	Display *pDisplay = glXGetCurrentDisplay();
	int iScreen = DefaultScreen(pDisplay);
	GLXContext glxContext = glXGetCurrentContext();
	int iConfigCount;
	GLXFBConfig *glxConfig;
	if (m_bSharedContext)
		glxConfig = glXGetFBConfigs(pDisplay, iScreen, &iConfigCount);
	else {
		int pf_attr[16] = {
			GLX_DRAWABLE_TYPE,
			GLX_PBUFFER_BIT,
			GLX_RENDER_TYPE,
			GLX_RGBA_BIT,
			0
		};
		glxConfig = glXChooseFBConfigSGIX(pDisplay, iScreen, pf_attr, &iConfigCount);
	}
	if (!glxConfig)
		return;
	int pb_attr[16] = {
		GLX_LARGEST_PBUFFER, false,
		GLX_PRESERVED_CONTENTS, true,
		0
	};
	m_glxPbuffer = glXCreateGLXPbufferSGIX(pDisplay, glxConfig[0], shadowMapSize, shadowMapSize, pb_attr);
	if (!m_glxPbuffer)
		return;
	if (m_bSharedContext)
		m_glxContext = glxContext;
	else {
		m_glxContext = glXCreateContextWithConfigSGIX(pDisplay, glxConfig[0], GLX_RGBA_TYPE, (m_bShareObjects?glxContext:NULL), true);
		if (!glxConfig)
			return;
	}
	m_pDisplay = pDisplay;
}

GLXPBuffer::~GLXPBuffer()
{
	if (m_glxContext && !m_bSharedContext)
		glXDestroyContext(m_pDisplay, m_glxContext);
	if (m_glxPbuffer)
		glXDestroyGLXPbufferSGIX(m_pDisplay, m_glxPbuffer);
}

void GLXPBuffer::checkFBOStatus(void)
{
}

void GLXPBuffer::attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype)
{
	if (attachtype == FBO_ATTACH_DEPTH)
		shadowTex = tex;
}

void GLXPBuffer::select(void)
{
	if (m_pOldDisplay || m_glxOldDrawable || m_glxOldContext)
		return;
	m_pOldDisplay = glXGetCurrentDisplay();
	m_glxOldDrawable = glXGetCurrentDrawable();
	m_glxOldContext = glXGetCurrentContext();
	if (!glXMakeCurrent(m_pDisplay, m_glxPbuffer, m_glxContext)) {
		m_pOldDisplay = 0;
		m_glxOldDrawable = 0;
		m_glxOldContext = 0;
	}
}

void GLXPBuffer::deselect(void)
{
	if (!(m_pOldDisplay && m_glxOldDrawable && m_glxOldContext))
		return;
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, shadowMapSize-2, shadowMapSize-2);
	if (!glXMakeCurrent(m_pOldDisplay, m_glxOldDrawable, m_glxOldContext))
		return;
	m_pOldDisplay = 0;
	m_glxOldDrawable = 0;
	m_glxOldContext = 0;
}

bool GLXPBuffer::valid(void)
{
	return m_glxPbuffer != 0;
}
