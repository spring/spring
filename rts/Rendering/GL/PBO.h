/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PBO_H
#define PBO_H

#include "VBO.h"

/**
 * @brief PBO
 *
 * Pixelbuffer Object class (EXT_pixel_buffer_object).
 * WARNING: CANNOT BE USED IN COMBINATION WITH gluBuild2DMipmaps/glBuildMipmaps!!!
 */
class PBO : public VBO
{
public:
	PBO() : VBO(GL_PIXEL_UNPACK_BUFFER) {}
	virtual ~PBO() {}
};

#endif /* PBO_H */
