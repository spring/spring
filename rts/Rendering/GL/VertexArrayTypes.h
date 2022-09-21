/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/Color.h"
#include "System/float3.h"
#include "System/type2.h"
#include "myGL.h"

#include <array>
#include <string>

struct AttributeDef {
	AttributeDef(uint32_t index_, uint32_t count_, uint32_t type_, uint32_t stride_, const void* data_, bool normalize_ = false, std::string name_ = "")
		: name{ std::move(name_) }
		, index{ index_ }
		, count{ count_ }
		, type{ type_ }
		, stride{ stride_ }
		, data{ data_ }
		, normalize{ static_cast<uint8_t>(normalize_) }
	{}

	const std::string name;

	uint32_t index;
	uint32_t count;   // in number of elements of type
	uint32_t type;    // GL_FLOAT, etc
	uint32_t stride;  // in bytes

	const void* data; // offset

	uint8_t normalize;
};

struct VA_TYPE_0 {
	float3 pos;

	VA_TYPE_0 operator* (float t) const {
		VA_TYPE_0 v = *this;
		v.pos *= t;

		return v;
	}

	static std::array<AttributeDef, 1> attributeDefs;
};
struct VA_TYPE_N {
	float3 pos;
	float3 n;

	VA_TYPE_N operator* (float t) const {
		VA_TYPE_N v = *this;
		v.pos *= t;

		v.n *= t; v.n.ANormalize();

		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_C {
	float3 pos;
	SColor c;

	VA_TYPE_C operator* (float t) const {
		VA_TYPE_C v = *this;
		v.pos *= t;

		v.c *= t;

		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_T {
	float3 pos;
	float  s, t;

	VA_TYPE_T operator* (float t) const {
		VA_TYPE_T v = *this;
		v.pos *= t;

		v.s *= t;
		v.t *= t;

		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_TN {
	float3 pos;
	float  s, t;
	float3 n;

	VA_TYPE_TN operator* (float t) const {
		VA_TYPE_TN v = *this;
		v.pos *= t;

		v.s *= t;
		v.t *= t;

		v.n *= t; v.n.ANormalize();

		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};
struct VA_TYPE_TC {
	float3 pos;
	float  s, t;
	SColor c;

	VA_TYPE_TC operator* (float t) const {
		VA_TYPE_TC v = *this;
		v.pos *= t;

		v.s *= t;
		v.t *= t;

		v.c *= t;

		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};
struct VA_TYPE_PROJ {
	float3 pos;
	float3 uvw;
	float4 uvInfo;
	float3 aparams;
	SColor c;

	VA_TYPE_PROJ operator* (float t) const {
		VA_TYPE_PROJ v = *this;
		v.pos *= t;

		v.uvw *= t;

		v.uvInfo *= t;

		v.aparams *= t;

		v.c *= t;

		return v;
	}

	static std::array<AttributeDef, 5> attributeDefs;
};
struct VA_TYPE_TNT {
	float3 pos;
	float  s, t;
	float3 n;
	float3 uv1;
	float3 uv2;

	VA_TYPE_TNT operator* (float t) const {
		VA_TYPE_TNT v = *this;
		v.pos *= t;

		v.s *= t;
		v.t *= t;

		v.n *= t;   v.n.ANormalize();

		v.uv1 *= t; v.uv1.ANormalize();

		v.uv2 *= t; v.uv2.ANormalize();

		return v;
	}

	static std::array<AttributeDef, 5> attributeDefs;
};
struct VA_TYPE_2D0 {
	float x, y;

	VA_TYPE_2D0 operator* (float t) const {
		VA_TYPE_2D0 v = *this;
		v.x *= t;
		v.y *= t;

		return v;
	}

	static std::array<AttributeDef, 1> attributeDefs;
};
struct VA_TYPE_2DC {
	float x, y;
	SColor c;

	VA_TYPE_2DC operator* (float t) const {
		VA_TYPE_2DC v = *this;
		v.x *= t;
		v.y *= t;

		v.c *= t;

		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_2DT {
	float x, y;
	float s, t;

	VA_TYPE_2DT operator* (float t) const {
		VA_TYPE_2DT v = *this;
		v.x *= t;
		v.y *= t;

		v.s *= t;
		v.t *= t;

		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_2DTC {
	float  x, y;
	float  s, t;
	SColor c;

	VA_TYPE_2DTC operator* (float t) const {
		VA_TYPE_2DTC v = *this;
		v.x *= t;
		v.y *= t;

		v.s *= t;
		v.t *= t;

		v.c *= t;

		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};

// number of elements (bytes / sizeof(float)) per vertex
constexpr size_t VA_SIZE_0    = (sizeof(VA_TYPE_0) / sizeof(float));
constexpr size_t VA_SIZE_C    = (sizeof(VA_TYPE_C) / sizeof(float));
constexpr size_t VA_SIZE_N    = (sizeof(VA_TYPE_N) / sizeof(float));
constexpr size_t VA_SIZE_T    = (sizeof(VA_TYPE_T) / sizeof(float));
constexpr size_t VA_SIZE_TN   = (sizeof(VA_TYPE_TN) / sizeof(float));
constexpr size_t VA_SIZE_TC   = (sizeof(VA_TYPE_TC) / sizeof(float));
constexpr size_t VA_SIZE_T4C  = (sizeof(VA_TYPE_PROJ) / sizeof(float));
constexpr size_t VA_SIZE_TNT  = (sizeof(VA_TYPE_TNT) / sizeof(float));
constexpr size_t VA_SIZE_2D0  = (sizeof(VA_TYPE_2D0) / sizeof(float));
constexpr size_t VA_SIZE_2DT  = (sizeof(VA_TYPE_2DT) / sizeof(float));
constexpr size_t VA_SIZE_2DTC = (sizeof(VA_TYPE_2DTC) / sizeof(float));