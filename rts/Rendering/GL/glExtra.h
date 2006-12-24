#ifndef GLEXTRA_H
#define GLEXTRA_H

#include "myGL.h"

/*
 *  Draw a circle on top of the top surface (ground/water).
 &
 *  Note: Uses the current color.
 */
void glSurfaceCircle(const float3& center, float radius,
                     unsigned int resolution = 20);

void glBallisticCircle(const float3& center, float radius, float slope,
                       unsigned int resolution = 20);


#endif
