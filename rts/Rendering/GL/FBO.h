/*
 * FBO.h
 * EXT_framebuffer_object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 *
 * Framebuffer abstraction added by Jelmer Cnossen
 */
#ifndef FBO_H
#define FBO_H

#include <vector>

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
