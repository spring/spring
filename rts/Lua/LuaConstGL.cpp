#include "StdAfx.h"
// LuaConstGL.cpp: implementation of the LuaConstGL class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaConstGL.h"

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaUtils.h"
#include "Rendering/GL/myGL.h"


/******************************************************************************/
/******************************************************************************/

bool LuaConstGL::PushEntries(lua_State* L)
{
#define PUSH_GL(cmd) LuaPushNamedNumber(L, #cmd, GL_ ## cmd)

	// Drawing Primitives
	PUSH_GL(POINTS);
	PUSH_GL(LINES);
	PUSH_GL(LINE_LOOP);
	PUSH_GL(LINE_STRIP);
	PUSH_GL(TRIANGLES);
	PUSH_GL(TRIANGLE_STRIP);
	PUSH_GL(TRIANGLE_FAN);
	PUSH_GL(QUADS);
	PUSH_GL(QUAD_STRIP);
	PUSH_GL(POLYGON);

	// BlendingFactorDest
	PUSH_GL(ZERO);
	PUSH_GL(ONE);
	PUSH_GL(SRC_COLOR);
	PUSH_GL(ONE_MINUS_SRC_COLOR);
	PUSH_GL(SRC_ALPHA);
	PUSH_GL(ONE_MINUS_SRC_ALPHA);
	PUSH_GL(DST_ALPHA);
	PUSH_GL(ONE_MINUS_DST_ALPHA);

	// BlendingFactorSrc
	// GL_ZERO
	// GL_ONE
	PUSH_GL(DST_COLOR);
	PUSH_GL(ONE_MINUS_DST_COLOR);
	PUSH_GL(SRC_ALPHA_SATURATE);

	// AlphaFunction and  DepthFunction
	PUSH_GL(NEVER);
	PUSH_GL(LESS);
	PUSH_GL(EQUAL);
	PUSH_GL(LEQUAL);
	PUSH_GL(GREATER);
	PUSH_GL(NOTEQUAL);
	PUSH_GL(GEQUAL);
	PUSH_GL(ALWAYS);

	// LogicOp
	PUSH_GL(CLEAR);
	PUSH_GL(AND);
	PUSH_GL(AND_REVERSE);
	PUSH_GL(COPY);
	PUSH_GL(AND_INVERTED);
	PUSH_GL(NOOP);
	PUSH_GL(XOR);
	PUSH_GL(OR);
	PUSH_GL(NOR);
	PUSH_GL(EQUIV);
	PUSH_GL(INVERT);
	PUSH_GL(OR_REVERSE);
	PUSH_GL(COPY_INVERTED);
	PUSH_GL(OR_INVERTED);
	PUSH_GL(NAND);
	PUSH_GL(SET);

	// Culling
	PUSH_GL(BACK);
	PUSH_GL(FRONT);
	PUSH_GL(FRONT_AND_BACK);

	// PolygonMode
	PUSH_GL(POINT);
	PUSH_GL(LINE);
	PUSH_GL(FILL);

	// Clear Bits
	PUSH_GL(DEPTH_BUFFER_BIT);
	PUSH_GL(ACCUM_BUFFER_BIT);
	PUSH_GL(STENCIL_BUFFER_BIT);
	PUSH_GL(COLOR_BUFFER_BIT);

	// ShadeModel
	PUSH_GL(FLAT);
	PUSH_GL(SMOOTH);

	// MatrixMode
	PUSH_GL(MODELVIEW);
	PUSH_GL(PROJECTION);
	PUSH_GL(TEXTURE);

	return true;
}


/******************************************************************************/
/******************************************************************************/
