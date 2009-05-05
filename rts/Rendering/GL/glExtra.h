#ifndef GLEXTRA_H
#define GLEXTRA_H

#include "myGL.h"

/*
 *  Draw a circle / rectangle on top of the top surface (ground/water).
 *  Note: Uses the current color.
 */

class CWeapon;

typedef void (*SurfaceCircleFunc)(const float3& center, float radius, unsigned int res);
typedef void (*SurfaceSquareFunc)(const float3& center, float xsize, float zsize);

extern SurfaceCircleFunc glSurfaceCircle;
extern SurfaceSquareFunc glSurfaceSquare;

extern void glBallisticCircle(const float3& center, float radius,
	const CWeapon* weapon, unsigned int resolution, float slope = 0.0f);

extern void setSurfaceCircleFunc(SurfaceCircleFunc func);
extern void setSurfaceSquareFunc(SurfaceSquareFunc func);

typedef void (*DrawVolumeFunc)(const void* data);
extern void glDrawVolume(DrawVolumeFunc drawFunc, const void* data);

extern void gluMyCube(float);

#endif
