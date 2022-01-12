/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTEXARRAY_TYPES_H
#define VERTEXARRAY_TYPES_H

#include "System/Color.h"
#include "System/float4.h"
#include "System/type2.h"

struct VA_TYPE_0 {
	float3 p;
};
struct VA_TYPE_C {
	float3 p;
	SColor c;
};
struct VA_TYPE_T {
	float3 p;
	float  s, t;
};
struct VA_TYPE_T4 {
	float3 p;
	float4 tc;
};
struct VA_TYPE_TN {
	float3 p;
	float  s, t;
	float3 n;
};
struct VA_TYPE_TC {
	float3 p;
	float  s, t;
	SColor c;
};
struct VA_TYPE_2d0 {
	float x, y;
};
struct VA_TYPE_2dT {
	float x, y;
	float s, t;
};
struct VA_TYPE_2dTC {
	float  x, y;
	float  s, t;
	SColor c;
};
struct VA_TYPE_L {
	float4 p; // Lua can freely set the w-component
	float3 n;
	float4 uv; // two channels for basic texturing
	SColor c0; // primary
	SColor c1; // secondary
};

static_assert(sizeof(SColor) == sizeof(float), "");

// number of elements (bytes / sizeof(float)) per vertex
#define VA_SIZE_0    (sizeof(VA_TYPE_0   ) / sizeof(float))
#define VA_SIZE_C    (sizeof(VA_TYPE_C   ) / sizeof(float))
#define VA_SIZE_T    (sizeof(VA_TYPE_T   ) / sizeof(float))
#define VA_SIZE_T4   (sizeof(VA_TYPE_T4  ) / sizeof(float))
#define VA_SIZE_TN   (sizeof(VA_TYPE_TN  ) / sizeof(float))
#define VA_SIZE_TC   (sizeof(VA_TYPE_TC  ) / sizeof(float))
#define VA_SIZE_2D0  (sizeof(VA_TYPE_2d0 ) / sizeof(float))
#define VA_SIZE_2DT  (sizeof(VA_TYPE_2dT ) / sizeof(float))
#define VA_SIZE_2DTC (sizeof(VA_TYPE_2dTC) / sizeof(float))
#define VA_SIZE_L    (sizeof(VA_TYPE_L   ) / sizeof(float))

#define VA_TYPE_OFFSET(T, n) (reinterpret_cast<void*>(sizeof(T) * (n)))

#endif

