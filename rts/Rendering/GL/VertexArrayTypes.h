/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTEXARRAY_TYPES_H
#define VERTEXARRAY_TYPES_H

#include "System/Color.h"
#include "System/float4.h"

struct VA_TYPE_0 {
	float3 p;
};
struct VA_TYPE_N {
	float3 p;
	float3 n;
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
struct VA_TYPE_TNT {
	float3 p;
	float  s, t;
	float3 n;
	float3 uv1;
	float3 uv2;
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

static_assert(sizeof(SColor) == sizeof(float), "");

// number of elements (bytes / sizeof(float)) per vertex
#define VA_SIZE_0    (sizeof(VA_TYPE_0) / sizeof(float))
#define VA_SIZE_C    (sizeof(VA_TYPE_C) / sizeof(float))
#define VA_SIZE_N    (sizeof(VA_TYPE_N) / sizeof(float))
#define VA_SIZE_T    (sizeof(VA_TYPE_T) / sizeof(float))
#define VA_SIZE_T4   (sizeof(VA_TYPE_T4) / sizeof(float))
#define VA_SIZE_TN   (sizeof(VA_TYPE_TN) / sizeof(float))
#define VA_SIZE_TC   (sizeof(VA_TYPE_TC) / sizeof(float))
#define VA_SIZE_TNT  (sizeof(VA_TYPE_TNT) / sizeof(float))
#define VA_SIZE_2D0  (sizeof(VA_TYPE_2d0) / sizeof(float))
#define VA_SIZE_2DT  (sizeof(VA_TYPE_2dT) / sizeof(float))
#define VA_SIZE_2DTC (sizeof(VA_TYPE_2dTC) / sizeof(float))

// clang does not like reinterpret_cast inside constexpr's
// it apparently also can not handle 0-pointer arithmetic
#if (defined(__clang__)) || (defined(_MSC_VER))
#define CONSTQUAL const
// #define VA_TYPE_OFFSET(T, n) (static_cast<uint8_t*>(nullptr) + sizeof(T) * (n))
#define VA_TYPE_OFFSET(T, n) (reinterpret_cast<void*>(sizeof(T) * (n)))
#else
#define CONSTQUAL const
#define VA_TYPE_OFFSET(T, n) ((const T*)(0) + (n))
#endif

#endif

