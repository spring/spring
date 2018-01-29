/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GLEXTRA_H
#define GLEXTRA_H

#include "myGL.h"

/*
 *  Draw a circle / rectangle on top of the top surface (ground/water).
 *  Note: Uses the current color.
 */

struct WeaponDef;

typedef void (*SurfaceCircleFunc)(const float3& center, float radius, unsigned int res);
typedef void (*SurfaceSquareFunc)(const float3& center, float xsize, float zsize);

extern SurfaceCircleFunc glSurfaceCircle;
extern SurfaceSquareFunc glSurfaceSquare;

// params.x := radius, params.y := slope, params.z := gravity
extern void glBallisticCircle(const WeaponDef* weaponDef, unsigned int resolution, const float3& center, const float3& params);

extern void setSurfaceCircleFunc(SurfaceCircleFunc func);
extern void setSurfaceSquareFunc(SurfaceSquareFunc func);

typedef void (*DrawVolumeFunc)(const void* data);
extern void glDrawVolume(DrawVolumeFunc drawFunc, const void* data);

extern void glWireCube(unsigned int* listID);
extern void glWireCylinder(unsigned int* listID, unsigned int numDivs, float zSize);
extern void glWireSphere(unsigned int* listID, unsigned int numRows, unsigned int numCols);

#endif
