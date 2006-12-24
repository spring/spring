#ifndef GLEXTRA_H
#define GLEXTRA_H

#include "myGL.h"

/*
 *  Draw a circle on top of the top surface (ground/water).
 &
 *  Note: Uses the current color.
 */
 
typedef void (*SurfaceCircleFunc)(const float3& center, float radius,
                                  unsigned int resolution);

extern SurfaceCircleFunc glSurfaceCircle;

extern void glBallisticCircle(const float3& center, float radius, float slope,
                              unsigned int resolution);

extern void setSurfaceCircleFunc(SurfaceCircleFunc func);

#endif
