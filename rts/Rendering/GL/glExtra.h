/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "RenderBuffers.h"

/*
 *  Draw a circle / rectangle on top of the top surface (ground/water).
 *  Note: Uses the current color.
 */

class CWeapon;
struct WeaponDef;

typedef void (*SurfaceCircleFunc)(const float3& center, float radius, unsigned int res);
typedef void (*SurfaceColoredCircleFunc)(const float3& center, float radius, const SColor&, unsigned int res);
typedef void (*SurfaceSquareFunc)(const float3& center, float xsize, float zsize);

extern SurfaceCircleFunc glSurfaceCircle;
extern SurfaceColoredCircleFunc glSurfaceColoredCircle;
extern SurfaceSquareFunc glSurfaceSquare;

// params.x := radius, params.y := slope, params.z := gravity
extern void glBallisticCircle(const CWeapon* weapon, unsigned int resolution, const float3& center, const float3& params);
extern void glBallisticCircle(const WeaponDef* weaponDef, unsigned int resolution, const float3& center, const float3& params);

extern void setSurfaceCircleFunc(SurfaceCircleFunc func);
extern void setSurfaceColoredCircleFunc(SurfaceColoredCircleFunc func);
extern void setSurfaceSquareFunc(SurfaceSquareFunc func);

typedef void (*DrawVolumeFunc)(const void* data);
extern void glDrawVolume(DrawVolumeFunc drawFunc, const void* data);

extern void glWireCube(unsigned int* listID);
extern void glWireCylinder(unsigned int* listID, unsigned int numDivs, float zSize);
extern void glWireSphere(unsigned int* listID, unsigned int numRows, unsigned int numCols);

template<typename TQuad, typename TColor, typename TRenderBuffer> void gleDrawQuadC(const TQuad& quad, const TColor& color, TRenderBuffer& rb) {
	rb.SafeAppend({ {quad.x1, quad.y1, 0.0f}, color }); // tl
	rb.SafeAppend({ {quad.x1, quad.y2, 0.0f}, color }); // bl
	rb.SafeAppend({ {quad.x2, quad.y2, 0.0f}, color }); // br

	rb.SafeAppend({ {quad.x2, quad.y2, 0.0f}, color }); // br
	rb.SafeAppend({ {quad.x2, quad.y1, 0.0f}, color }); // tr
	rb.SafeAppend({ {quad.x1, quad.y1, 0.0f}, color }); // tl
}
