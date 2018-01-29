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

template<typename TQuad, typename TColor, typename TRenderBuffer> void gleDrawQuadC(const TQuad& quad, const TColor& color, TRenderBuffer* buffer) {
	buffer->SafeAppend({{quad.x1, quad.y1, 0.0f}, color});
	buffer->SafeAppend({{quad.x1, quad.y2, 0.0f}, color});
	buffer->SafeAppend({{quad.x2, quad.y2, 0.0f}, color});
	buffer->SafeAppend({{quad.x2, quad.y1, 0.0f}, color});
}



void gleGenColVolMeshBuffers(unsigned int* meshData);
void gleDelColVolMeshBuffers(unsigned int* meshData);
void gleBindColVolMeshBuffers(const unsigned int* meshData);
void gleDrawColVolMeshSubBuffer(const unsigned int* meshData, unsigned int meshType);

#endif
