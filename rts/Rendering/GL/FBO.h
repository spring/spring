/**
 * @file FBO.h
 * @brief EXT_framebuffer_object
 * @author Christopher Han <xiphux@gmail.com>
 *
 * EXT_framebuffer_object class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 *
 * Framebuffer abstraction added by Jelmer Cnossen
 */
#ifndef FBO_H
#define FBO_H

#include "myGL.h"

/**
 * @brief FBO
 *
 * Framebuffer Object class. Derived from the
 * abstract IFramebuffer class
 */
class FBO : public IFramebuffer
{
public:
	/**
	 * @brief check FBO status
	 */
	bool checkFBOStatus(void);

	/**
	 * @brief attach texture
	 * @param tex texture to attach
	 * @param textype type of texture
	 * @param attachtype what kind of target to attach as
	 */
	void attachTexture(const GLuint tex, const unsigned int textype, FramebufferAttachType attachtype);

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

private:
	/**
	 * @brief Constructor
	 */
	FBO(int requires, int w, int h);

	/**
	 * @brief Destructor
	 */
	~FBO();

	/**
	 * @brief framebuffer
	 *
	 * GLuint pointing to the current framebuffer
	 */
	GLuint frameBuffer;
	GLuint depthRenderBuffer;
	int requires;

	// instantiate_fb is the only code where new FBOs may be created
	friend IFramebuffer* instantiate_fb(const int w, const int h, const int requires);
};

#endif /* FBO_H */
