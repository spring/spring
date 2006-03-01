/**
 * @file GLXPBuffer.h
 * @brief GLX_SGIX_pbuffer
 * @author Christopher Han <xiphux@gmail.com>
 *
 * GLX_SGIX_pbuffer class definition
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */
#ifndef _GLXPBUFFER_H
#define _GLXPBUFFER_H

#ifdef _WIN32
#error This file should not be compiled on windows
#endif

#include <GL/glxew.h>

/**
 * @brief GLX pbuffer
 *
 * GLX pbuffer class.  Derived from the
 * abstract IFramebuffer class
 */
class GLXPBuffer: public IFramebuffer
{
public:
	/**
	 * @brief Constructor
	 * @param shadowMapSize dimension of buffer
	 * @param sharedcontext whether to share context with the main context
	 * @param shareobjects whether to share objects with the main context
	 */
	GLXPBuffer(const int shadowMapSize, const bool sharedcontext = true, const bool shareobjects = true);

	/**
	 * @brief Destructor
	 */
	~GLXPBuffer();

	/**
	 * @brief check FBO status
	 */
	void checkFBOStatus(void);

	/**
	 * @brief attach texture
	 * @param tex texture to attach
	 * @param textype type of texture
	 * @param attachtype what kind of target to attach as
	 */
	void attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype);

	/**
	 * @brief select
	 */
	void select(void);

	/**
	 * @brief deselect
	 */
	void deselect(void);

	/**
	 * @brief valid
	 * @return whether a valid framebuffer exists
	 */
	bool valid(void);
protected:
	/**
	 * @brief display
	 *
	 * Pointer to the current display
	 */
	Display *m_pDisplay;

	/**
	 * @brief glxpbuffer
	 *
	 * Pointer to the current GLX drawable
	 */
	GLXPbuffer m_glxPbuffer;

	/**
	 * @brief glxcontext
	 *
	 * Pointer to the current GLX context
	 */
	GLXContext m_glxContext;

	/**
	 * @brief olddisplay
	 *
	 * Pointer to the previous display
	 */
	Display *m_pOldDisplay;

	/**
	 * @brief olddrawable
	 *
	 * Pointer to the previous drawable
	 */
	GLXPbuffer m_glxOldDrawable;

	/**
	 * @brief oldcontext
	 *
	 * Pointer to the previous context
	 */
	GLXContext m_glxOldContext;

	/**
	 * @brief sharedcontext
	 *
	 * Stores whether we're sharing the context
	 */
	bool m_bSharedContext;

	/**
	 * @brief shareobjects
	 *
	 * Stores whether we're sharing objects
	 */
	bool m_bShareObjects;

	/**
	 * @brief shadow map size
	 *
	 * Stores the dimension of the pbuffer
	 */
	int shadowMapSize;
	
	/**
	 * @brief shadow texture
	 *
	 * GLuint pointing to the attached texture
	 */
	unsigned int shadowTex;
};

#endif /* _GLXPBUFFER_H */
