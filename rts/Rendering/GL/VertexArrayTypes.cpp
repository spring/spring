/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VertexArrayTypes.h"

#define VA_TYPE_OFFSET(T, m) reinterpret_cast<const void*>(offsetof(T, m))
#define VA_ATTR_DEF(T, idx, count, type, member, normalized, name) AttributeDef(idx, count, type, sizeof(T), VA_TYPE_OFFSET(T, member), normalized, name)

std::array<AttributeDef, 1> VA_TYPE_0::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_0, 0, 3, GL_FLOAT, p, false, "pos")
};

std::array<AttributeDef, 2> VA_TYPE_N::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_N, 0, 3, GL_FLOAT, p, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_N, 1, 3, GL_FLOAT, n, false, "normal")
};

std::array<AttributeDef, 2> VA_TYPE_C::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_C, 0, 3, GL_FLOAT, p, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_C, 1, 4, GL_UNSIGNED_BYTE, c, true, "color")
};

std::array<AttributeDef, 2> VA_TYPE_T::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_T, 0, 3, GL_FLOAT, p, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_T, 1, 2, GL_FLOAT, s, false, "uv")
};

std::array<AttributeDef, 3> VA_TYPE_TN::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_TN, 0, 3, GL_FLOAT, p, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_TN, 1, 2, GL_FLOAT, s, false, "uv"),
	VA_ATTR_DEF(VA_TYPE_TN, 2, 3, GL_FLOAT, n, false, "normal")
};

std::array<AttributeDef, 3> VA_TYPE_TC::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_TC, 0, 3, GL_FLOAT, p, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_TC, 1, 2, GL_FLOAT, s, false, "uv"),
	VA_ATTR_DEF(VA_TYPE_TC, 2, 4, GL_UNSIGNED_BYTE, c, true, "color")
};

std::array<AttributeDef, 5> VA_TYPE_TNT::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_TNT, 0, 3, GL_FLOAT, p, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_TNT, 1, 2, GL_FLOAT, s, false, "uv"),
	VA_ATTR_DEF(VA_TYPE_TNT, 2, 3, GL_FLOAT, n, false, "normal"),
	VA_ATTR_DEF(VA_TYPE_TNT, 3, 3, GL_FLOAT, uv1, false, "uvw1"),
	VA_ATTR_DEF(VA_TYPE_TNT, 4, 3, GL_FLOAT, uv2, false, "uvw2"),
};

std::array<AttributeDef, 1> VA_TYPE_2d0::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_2d0, 0, 2, GL_FLOAT, x, false, "pos")
};
std::array<AttributeDef, 2> VA_TYPE_2dC::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_2dC, 0, 2, GL_FLOAT, x, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_2dC, 1, 4, GL_UNSIGNED_BYTE, c, true, "color")
};

std::array<AttributeDef, 2> VA_TYPE_2dT::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_2dT, 0, 2, GL_FLOAT, x, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_2dT, 1, 2, GL_FLOAT, s, false, "uv"),
};

std::array<AttributeDef, 3> VA_TYPE_2dTC::attributeDefs = {
	VA_ATTR_DEF(VA_TYPE_2dTC, 0, 2, GL_FLOAT, x, false, "pos"),
	VA_ATTR_DEF(VA_TYPE_2dTC, 1, 2, GL_FLOAT, s, false, "uv"),
	VA_ATTR_DEF(VA_TYPE_2dTC, 2, 4, GL_UNSIGNED_BYTE, c, true, "color")
};

#undef VA_TYPE_OFFSET
#undef VA_ATTR_DEF
