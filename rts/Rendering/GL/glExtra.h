/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GLEXTRA_H
#define GLEXTRA_H

#include "myGL.h"
#include "RenderDataBufferFwd.hpp"
#include "WideLineAdapterFwd.hpp"

class CWeapon;
struct WeaponDef;

namespace Shader {
	struct IProgramObject;
};


extern void glSetupRangeRingDrawState();
extern void glResetRangeRingDrawState();
extern void glSetupWeaponArcDrawState();
extern void glResetWeaponArcDrawState();

// params.x := radius, params.y := slope, params.z := gravity
extern void glBallisticCircle(
	GL::RenderDataBufferC* rdBuffer,
	const CWeapon* weapon,
	uint32_t circleRes,
	uint32_t lineMode,
	const float3& center,
	const float3& params,
	const float4& color
);
extern void glBallisticCircle(
	GL::RenderDataBufferC* rdBuffer,
	const WeaponDef* weaponDef,
	uint32_t circleRes,
	uint32_t lineMode,
	const float3& center,
	const float3& params,
	const float4& color
);
extern void glBallisticCircleW(
	GL::WideLineAdapterC* wla,
	const CWeapon* weapon,
	uint32_t circleRes,
	uint32_t lineMode,
	const float3& center,
	const float3& params,
	const float4& color
);
extern void glBallisticCircleW(
	GL::WideLineAdapterC* wla,
	const WeaponDef* weaponDef,
	uint32_t circleRes,
	uint32_t lineMode,
	const float3& center,
	const float3& params,
	const float4& color
);




extern void glDrawCone(GL::RenderDataBufferC* rdBuffer, uint32_t cullFace, uint32_t coneDivs, const float4& color);

typedef void (*DrawVolumeFunc)(const void* data);
// default implementation draws a circle on top of the world
// map surface (ground/water); expects center.w to be radius
typedef void (*DrawSurfaceCircleFunc)(GL::RenderDataBufferC* rb, const float4& center, const float4& color, unsigned int res);
typedef void (*DrawSurfaceCircleWFunc)(GL::WideLineAdapterC* wla, const float4& center, const float4& color, unsigned int res);

extern DrawSurfaceCircleFunc glSurfaceCircle;
extern DrawSurfaceCircleWFunc glSurfaceCircleW;

extern void glDrawVolume(DrawVolumeFunc drawFunc, const void* data);
extern void SetDrawSurfaceCircleFuncs(DrawSurfaceCircleFunc func, DrawSurfaceCircleWFunc funcw);


template<typename TQuad, typename TColor, typename TRenderBuffer> void gleDrawQuadC(const TQuad& quad, const TColor& color, TRenderBuffer* buffer) {
	buffer->SafeAppend({{quad.x1, quad.y1, 0.0f}, color}); // tl
	buffer->SafeAppend({{quad.x1, quad.y2, 0.0f}, color}); // bl
	buffer->SafeAppend({{quad.x2, quad.y2, 0.0f}, color}); // br

	buffer->SafeAppend({{quad.x2, quad.y2, 0.0f}, color}); // br
	buffer->SafeAppend({{quad.x2, quad.y1, 0.0f}, color}); // tr
	buffer->SafeAppend({{quad.x1, quad.y1, 0.0f}, color}); // tl
}



enum {
	GLE_MESH_TYPE_BOX = 0,
	GLE_MESH_TYPE_CYL = 1,
	GLE_MESH_TYPE_SPH = 2,
	GLE_MESH_TYPE_CNT = 3, // count
};

void gleGenMeshBuffers(unsigned int* meshData);
void gleDelMeshBuffers(unsigned int* meshData);
void gleBindMeshBuffers(const unsigned int* meshData);
void gleDrawMeshSubBuffer(const unsigned int* meshData, unsigned int meshType);

// [0] := VBO, [1] := IBO, [2] := VAO, [3 + i, 4 + i] := {#verts[i], #indcs[i]}
extern unsigned int COLVOL_MESH_BUFFERS[3 + GLE_MESH_TYPE_CNT * 2];
// [0] := cylinder divs, [1] := sphere rows, [2] := sphere cols
extern unsigned int COLVOL_MESH_PARAMS[3];

#endif
