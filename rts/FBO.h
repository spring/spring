/*
 * FBO.h
 * EXT_framebuffer_object class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef FBO_H
#define FBO_H

#include <vector>
#include "glew.h"

class FBO
{
public:
	FBO();
	~FBO();
	void checkFBOStatus(void);
	void attachTexture(const unsigned int tex, const unsigned int textype, const unsigned int attachtype);
	void select(void);
	void deselect(void);
	bool valid(void);
private:
	GLuint g_frameBuffer;
};

#endif /* FBO_H */
