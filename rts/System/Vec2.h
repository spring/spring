/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VEC2_H
#define VEC2_H

#include "creg/creg_cond.h"

// can't easily use templates because of creg

struct int2
{
	CR_DECLARE_STRUCT(int2);

	int2() : x(0), y(0) {}
	int2(const int nx, const int ny) : x(nx), y(ny) {}

	int x;
	int y;
};

struct float2
{
	CR_DECLARE_STRUCT(float2);

	float2() : x(0.0f), y(0.0f) {}
	float2(const float nx, const float ny) : x(nx), y(ny) {}

	float x;
	float y;
};

#endif
