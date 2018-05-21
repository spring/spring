/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GLEXTRA_H
#define GLEXTRA_H

#include "myGL.h"
#include "RenderDataBufferFwd.hpp"

class CWeapon;
struct WeaponDef;

namespace Shader {
	struct IProgramObject;
};


/*
 *  Draw a circle / rectangle on top of the top surface (ground/water).
 *  Note: Uses the current color.
 */
typedef void (*SurfaceCircleFunc)(const float3& center, float radius, unsigned int res);
typedef void (*SurfaceSquareFunc)(const float3& center, float xsize, float zsize);

extern SurfaceCircleFunc glSurfaceCircle;
extern SurfaceSquareFunc glSurfaceSquare;

extern void setSurfaceCircleFunc(SurfaceCircleFunc func);
extern void setSurfaceSquareFunc(SurfaceSquareFunc func);




extern void glSetupRangeRingDrawState(Shader::IProgramObject* ipo);
extern void glSetupWeaponArcDrawState(Shader::IProgramObject* ipo);
extern void glResetRangeRingDrawState(Shader::IProgramObject* ipo);
extern void glResetWeaponArcDrawState(Shader::IProgramObject* ipo);

// params.x := radius, params.y := slope, params.z := gravity
extern void glBallisticCircle(
	GL::RenderDataBufferC* rdBuffer,
	const CWeapon* weapon,
	uint32_t circleRes,
	uint32_t submitCtr,
	uint32_t lineMode,
	const float3& center,
	const float3& params,
	const float4& color
);
extern void glBallisticCircle(
	GL::RenderDataBufferC* rdBuffer,
	const WeaponDef* weaponDef,
	uint32_t circleRes,
	uint32_t submitCtr,
	uint32_t lineMode,
	const float3& center,
	const float3& params,
	const float4& color
);




extern void glDrawCone(GL::RenderDataBufferC* rdBuffer, uint32_t cullFace, uint32_t coneDivs, const float4& color);

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
