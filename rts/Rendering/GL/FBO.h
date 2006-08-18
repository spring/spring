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

#include <vector>

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
	 * @brief Constructor
	 */
	FBO(int requires, int w, int h);

	/**
	 * @brief Destructor
	 */
	~FBO();

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

private:
	/**
	 * @brief framebuffer
	 *
	 * GLuint pointing to the current framebuffer
	 */
	GLuint frameBuffer;
	GLuint depthRenderBuffer;
	int requires;
};

#endif /* FBO_H */
