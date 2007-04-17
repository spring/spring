#include "StdAfx.h"
// LuaOpenGL.cpp: implementation of the CLuaOpenGL class.
//
//////////////////////////////////////////////////////////////////////
//
// TODO:
// - go back to counting matrix push/pops (just for modelview?)
//   would have to make sure that display lists are also handled
//   (GL_MODELVIEW_STACK_DEPTH could help current situation, but
//    requires the ARB_imaging extension)
// - use materials instead of raw calls (again, handle dlists)
// - add shader/multitex support
//

#include "LuaOpenGL.h"
#include <string>
#include <vector>
#include <algorithm>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaDisplayLists.h"
#include "Game/Camera.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/glFont.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "System/LogOutput.h"

using std::max;
using std::string;
using std::vector;

// from Game.cpp
extern GLfloat LightDiffuseLand[];
extern GLfloat LightAmbientLand[];


/******************************************************************************/
/******************************************************************************/

void (*LuaOpenGL::resetMatrixFunc)(void) = NULL;

unsigned int LuaOpenGL::resetStateList = 0;

LuaOpenGL::DrawMode LuaOpenGL::drawMode = LuaOpenGL::DRAW_NONE;
LuaOpenGL::DrawMode LuaOpenGL::prevDrawMode = LuaOpenGL::DRAW_NONE;

bool  LuaOpenGL::drawingEnabled = false;
bool  LuaOpenGL::safeMode = true;
float LuaOpenGL::fontHeight = 0.001f;
float LuaOpenGL::screenWidth = 0.36f;
float LuaOpenGL::screenDistance = 0.60f;


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::Init()
{
	CalcFontHeight();

	resetStateList = glGenLists(1);
	glNewList(resetStateList, GL_COMPILE); {
		ResetGLState();
	}
	glEndList();
}


void LuaOpenGL::Free()
{
	glDeleteLists(resetStateList, 1);
}


/******************************************************************************/

void LuaOpenGL::CalcFontHeight()
{
	// calculate the text height that we'll use to
	// provide a consistent display when rendering text
	// (note the missing characters)
	fontHeight = font->CalcTextHeight("abcdef hi klmno  rstuvwx z"
	                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "0123456789");
	fontHeight = max(1.0e-6f, fontHeight);  // safety for dividing
}


/******************************************************************************/
/******************************************************************************/

bool LuaOpenGL::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(ConfigScreen);

	REGISTER_LUA_CFUNC(DrawMiniMap);
	REGISTER_LUA_CFUNC(SlaveMiniMap);
	REGISTER_LUA_CFUNC(ConfigMiniMap);
	
	REGISTER_LUA_CFUNC(ResetState);
	REGISTER_LUA_CFUNC(ResetMatrices);
	REGISTER_LUA_CFUNC(Clear);
	REGISTER_LUA_CFUNC(Lighting);
	REGISTER_LUA_CFUNC(ShadeModel);
	REGISTER_LUA_CFUNC(Scissor);
	REGISTER_LUA_CFUNC(ColorMask);
	REGISTER_LUA_CFUNC(DepthMask);
	REGISTER_LUA_CFUNC(DepthTest);
	REGISTER_LUA_CFUNC(Culling);
	REGISTER_LUA_CFUNC(LogicOp);
	REGISTER_LUA_CFUNC(Blending);
	REGISTER_LUA_CFUNC(AlphaTest);
	REGISTER_LUA_CFUNC(LineStipple);
	REGISTER_LUA_CFUNC(PolygonMode);
	REGISTER_LUA_CFUNC(PolygonOffset);

	REGISTER_LUA_CFUNC(Texture);
	REGISTER_LUA_CFUNC(Material);
	REGISTER_LUA_CFUNC(Color);

	REGISTER_LUA_CFUNC(LineWidth);
	REGISTER_LUA_CFUNC(PointSize);

	REGISTER_LUA_CFUNC(FreeTexture);
	REGISTER_LUA_CFUNC(TextureInfo);

	REGISTER_LUA_CFUNC(Shape);
	REGISTER_LUA_CFUNC(BeginEnd);
	REGISTER_LUA_CFUNC(Vertex);
	REGISTER_LUA_CFUNC(Normal);
	REGISTER_LUA_CFUNC(TexCoord);
	REGISTER_LUA_CFUNC(Rect);
	REGISTER_LUA_CFUNC(TexRect);
	REGISTER_LUA_CFUNC(Unit);
	REGISTER_LUA_CFUNC(UnitShape);
	REGISTER_LUA_CFUNC(Text);
	REGISTER_LUA_CFUNC(GetTextWidth);

	REGISTER_LUA_CFUNC(ListCreate);
	REGISTER_LUA_CFUNC(ListRun);
	REGISTER_LUA_CFUNC(ListDelete);

	REGISTER_LUA_CFUNC(ClipPlane);

	REGISTER_LUA_CFUNC(MatrixMode);
	REGISTER_LUA_CFUNC(LoadIdentity);
	REGISTER_LUA_CFUNC(LoadMatrix);
	REGISTER_LUA_CFUNC(MultMatrix);
	REGISTER_LUA_CFUNC(Translate);
	REGISTER_LUA_CFUNC(Scale);
	REGISTER_LUA_CFUNC(Rotate);
	REGISTER_LUA_CFUNC(PushMatrix);
	REGISTER_LUA_CFUNC(PopMatrix);

	return true;
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::ClearMatrixStack()
{
	int i;
	GLenum err;
	for (i = 0; i < 12345; i++) {
		err = glGetError();
		if (err == GL_NONE) {
			break;
		}
	}
	for (i = 0; i < 64; i++) {
		glPopMatrix();
		err = glGetError();
		if (err != GL_NONE) {
			break; // we're looking for GL_STACK_UNDERFLOW
		}
	}
}


void LuaOpenGL::ResetGLState()
{
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);

	glDisable(GL_LIGHTING);

	glShadeModel(GL_SMOOTH);

	glDisable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDisable(GL_SCISSOR_TEST);

	glDisable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glDisable(GL_POLYGON_OFFSET_POINT);

	glDisable(GL_LINE_STIPPLE);

	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);

	glLineWidth(1.0f);
	glPointSize(1.0f);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	const float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	const float diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	const float black[4]   = { 0.0f, 0.0f, 0.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}


/******************************************************************************/
/******************************************************************************/
//
//  Common routines
//

const GLbitfield AttribBits = 
	GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_POINT_BIT |
	GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT;


inline void LuaOpenGL::EnableCommon(DrawMode mode)
{
	assert(drawMode == DRAW_NONE);
	drawMode = mode;
	drawingEnabled = true;
	if (safeMode) {
		glPushAttrib(AttribBits);
		glCallList(resetStateList);
	}
	// FIXME  --  not needed by shadow or minimap   (use a WorldCommon ? )
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
}


inline void LuaOpenGL::DisableCommon(DrawMode mode)
{
	assert(drawMode == mode);
	// FIXME  --  not needed by shadow or minimap
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	drawMode = DRAW_NONE;
	drawingEnabled = false;
	if (safeMode) {
		glPopAttrib();
	}
}


/******************************************************************************/
//
//  World
//

void LuaOpenGL::EnableDrawWorld()
{
	EnableCommon(DRAW_WORLD);
	resetMatrixFunc = ResetWorldMatrices;
	SetupWorldLighting();
}


void LuaOpenGL::DisableDrawWorld()
{
	if (safeMode) {
		ResetWorldMatrices();
	}
	RevertWorldLighting();
	DisableCommon(DRAW_WORLD);
}


void LuaOpenGL::ResetDrawWorld()
{
	if (safeMode) {
		ResetWorldMatrices();
		glCallList(resetStateList);
	}
}


/******************************************************************************/
//
//  WorldShadow
//

void LuaOpenGL::EnableDrawWorldShadow()
{
	EnableCommon(DRAW_WORLD_SHADOW);
	resetMatrixFunc = ResetWorldShadowMatrices;
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_VERTEX_PROGRAM_ARB);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, unitDrawer->unitShadowGenVP);
}


void LuaOpenGL::DisableDrawWorldShadow()
{
	glDisable(GL_VERTEX_PROGRAM_ARB);
	glDisable(GL_POLYGON_OFFSET_FILL);
	ResetWorldShadowMatrices();
	DisableCommon(DRAW_WORLD_SHADOW);
}


void LuaOpenGL::ResetDrawWorldShadow()
{
	if (safeMode) {
		ResetWorldShadowMatrices();
		glCallList(resetStateList);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glPolygonOffset(1.0f, 1.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}
}


/******************************************************************************/
//
//  WorldReflection
//

void LuaOpenGL::EnableDrawWorldReflection()
{
	EnableCommon(DRAW_WORLD_REFLECTION);
	resetMatrixFunc = ResetWorldMatrices;
	SetupWorldLighting();
}


void LuaOpenGL::DisableDrawWorldReflection()
{
	if (safeMode) {
		ResetWorldMatrices();
	}
	RevertWorldLighting();
	DisableCommon(DRAW_WORLD_REFLECTION);
}


void LuaOpenGL::ResetDrawWorldReflection()
{
	if (safeMode) {
		ResetWorldMatrices();
		glCallList(resetStateList);
	}
}


/******************************************************************************/
//
//  WorldRefraction
//

void LuaOpenGL::EnableDrawWorldRefraction()
{
	EnableCommon(DRAW_WORLD_REFRACTION);
	resetMatrixFunc = ResetWorldMatrices;
	SetupWorldLighting();
}


void LuaOpenGL::DisableDrawWorldRefraction()
{
	if (safeMode) {
		ResetWorldMatrices();
	}
	RevertWorldLighting();
	DisableCommon(DRAW_WORLD_REFRACTION);
}


void LuaOpenGL::ResetDrawWorldRefraction()
{
	if (safeMode) {
		ResetWorldMatrices();
		glCallList(resetStateList);
	}
}


/******************************************************************************/
//
//  Screen
//

void LuaOpenGL::EnableDrawScreen()
{
	EnableCommon(DRAW_SCREEN);
	resetMatrixFunc = ResetScreenMatrices;

	SetupScreenMatrices();
	SetupScreenLighting();
	glCallList(resetStateList);
	glEnable(GL_NORMALIZE);
}


void LuaOpenGL::DisableDrawScreen()
{
	RevertScreenLighting();
	RevertScreenMatrices();
	DisableCommon(DRAW_SCREEN);
}


void LuaOpenGL::ResetDrawScreen()
{
	if (safeMode) {
		ResetScreenMatrices();
		glCallList(resetStateList);
	}
}


/******************************************************************************/
//
//  MiniMap
//

void LuaOpenGL::EnableDrawInMiniMap()
{
	if (drawMode == DRAW_SCREEN) {
		prevDrawMode = DRAW_SCREEN;
		drawMode = DRAW_NONE;
	}
	EnableCommon(DRAW_MINIMAP);
	resetMatrixFunc = ResetMiniMapMatrices;
	// CMiniMap::DrawForReal() does not setup the texture matrix
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (minimap) {
		glScalef(1.0f / (float)minimap->GetSizeX(),
		         1.0f / (float)minimap->GetSizeY(), 1.0f);
	}
}


void LuaOpenGL::DisableDrawInMiniMap()
{
	if (prevDrawMode != DRAW_SCREEN) {
		DisableCommon(DRAW_MINIMAP);
	}
	else {
		if (safeMode) {
			glPopAttrib();
		} else {
			glCallList(resetStateList);
		}
		resetMatrixFunc = ResetScreenMatrices;
		ResetScreenMatrices();
		prevDrawMode = DRAW_NONE;
		drawMode = DRAW_SCREEN;
	}
}


void LuaOpenGL::ResetDrawInMiniMap()
{
	if (safeMode) {
		ResetMiniMapMatrices();
		glCallList(resetStateList);
	}
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::SetupWorldLighting()
{
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightfv(GL_LIGHT1, GL_POSITION, gs->sunVector4);
	glEnable(GL_LIGHT1);
}


void LuaOpenGL::RevertWorldLighting()
{
	glDisable(GL_LIGHT1);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
}


void LuaOpenGL::SetupScreenMatrices()
{
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	const float dist   = screenDistance;         // eye-to-screen (meters)
	const float width  = screenWidth;            // screen width (meters)
	const float vppx   = float(gu->winPosX + gu->viewPosX); // view pixel pos x
	const float vppy   = float(gu->winPosY + gu->viewPosY); // view pixel pos y
	const float vpsx   = float(gu->viewSizeX);   // view pixel size x
	const float vpsy   = float(gu->viewSizeY);   // view pixel size y
	const float spsx   = float(gu->screenSizeX); // screen pixel size x
	const float spsy   = float(gu->screenSizeY); // screen pixel size x
	const float halfSX = 0.5f * spsx;            // half screen pixel size x
	const float halfSY = 0.5f * spsy;            // half screen pixel size y

	const float zplane = dist * (spsx / width);
	const float znear  = zplane * 0.5;
	const float zfar   = zplane * 2.0;
	const float factor = (znear / zplane);
	const float left   = (vppx - halfSX) * factor;
	const float bottom = (vppy - halfSY) * factor;
	const float right  = ((vppx + vpsx) - halfSX) * factor;
	const float top    = ((vppy + vpsy) - halfSY) * factor;
	glFrustum(left, right, bottom, top, znear, zfar);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// translate so that (0,0,0) is on the zplane,
	// on the window's bottom left corner
	const float distAdj = (zplane / znear);
	glTranslatef(left * distAdj, bottom * distAdj, -zplane);
}


void LuaOpenGL::RevertScreenMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack();
		glLoadIdentity();
		gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack();
		glLoadIdentity();
	}
}


void LuaOpenGL::SetupScreenLighting()
{
	// back light
	const float backLightPos[4]  = { 1.0f, 2.0f, 2.0f, 0.0f };
	const float backLightAmbt[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const float backLightDiff[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const float backLightSpec[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, backLightPos);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  backLightAmbt);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  backLightDiff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, backLightSpec);

	// sun light -- needs the camera transformation
	glPushMatrix();
	glLoadMatrixd(camera->modelview);
	glLightfv(GL_LIGHT1, GL_POSITION, gs->sunVector4);

	const float sunFactor = 1.0f;
	const float sf = sunFactor;
	const float* la = LightAmbientLand;
	const float* ld = LightDiffuseLand;

	const float sunLightAmbt[4] = { la[0]*sf, la[1]*sf, la[2]*sf, la[3]*sf };
	const float sunLightDiff[4] = { ld[0]*sf, ld[1]*sf, ld[2]*sf, ld[3]*sf };
	const float sunLightSpec[4] = { la[0]*sf, la[1]*sf, la[2]*sf, la[3]*sf };
	glLightfv(GL_LIGHT1, GL_AMBIENT,  sunLightAmbt);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  sunLightDiff);
	glLightfv(GL_LIGHT1, GL_SPECULAR, sunLightSpec);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
	glPopMatrix();

	// Enable the GL lights
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}


void LuaOpenGL::RevertScreenLighting()
{
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT0);
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::ResetWorldMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack();
		glLoadMatrixd(camera->projection);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack();
		glLoadMatrixd(camera->modelview);
	}
}


void LuaOpenGL::ResetWorldShadowMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack();
		glLoadIdentity();             
		glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, -1.0);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack();
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
	}
}


void LuaOpenGL::ResetScreenMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	SetupScreenMatrices();
}


void LuaOpenGL::ResetMiniMapMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack();
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack();
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0e6, +1.0e6);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack();
		glLoadIdentity();
		if (minimap) {
			glScalef(1.0f / (float)minimap->GetSizeX(),
			         1.0f / (float)minimap->GetSizeY(), 1.0f);
		}
	}
}


/******************************************************************************/
/******************************************************************************/

inline void LuaOpenGL::CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!drawingEnabled) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
}


/******************************************************************************/

int LuaOpenGL::ConfigScreen(lua_State* L)
{
//	CheckDrawingEnabled(L, __FUNCTION__);
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.ConfigScreen()");
	}
	screenWidth = (float)lua_tonumber(L, 1);
	screenDistance = (float)lua_tonumber(L, 2);
	return 0;
}


int LuaOpenGL::SlaveMiniMap(lua_State* L)
{
	if (!minimap) {
		return 0;
	}
//	CheckDrawingEnabled(L, __FUNCTION__);
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.SlaveMiniMap()");
	}
	if (minimap) {
		const bool mode = lua_toboolean(L, 1);
		minimap->SetSlaveMode(mode);
	}
	return 0;
}


int LuaOpenGL::ConfigMiniMap(lua_State* L)
{
	if (!minimap) {
		return 0;
	}
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to gl.ConfigMiniMap()");
	}
	const int px = (int)lua_tonumber(L, 1);
	const int py = (int)lua_tonumber(L, 2);
	const int sx = (int)lua_tonumber(L, 3);
	const int sy = (int)lua_tonumber(L, 4);
	minimap->SetGeometry(px, py, sx, sy);
	return 0;
}


int LuaOpenGL::DrawMiniMap(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	if (!minimap) {
		return 0;
	}
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	if (drawMode != DRAW_SCREEN) {
		luaL_error(L,
			"gl.DrawMiniMap() can only be used within DrawScreenItems()");
	}
	if (!minimap->GetSlaveMode()) {
		luaL_error(L,
			"gl.DrawMiniMap() can only be used if the minimap is in slave mode");
	}
	minimap->DrawForReal();
	return 0;
}


int LuaOpenGL::Text(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L,
		  "Incorrect arguments to gl.Text(msg, x, y [,size] [,\"options\"]");
	}
	const string text = lua_tostring(L, 1);
	const float x     = lua_tonumber(L, 2);
	const float y     = lua_tonumber(L, 3);

	float size = 12.0f;
	bool right = false;
	bool center = false;
	bool outline = false;
	bool colorCodes = true;
	bool lightOut;

	if ((args >= 4) && lua_isnumber(L, 4)) {
		size = lua_tonumber(L, 4);
	}
	if ((args >= 5) && lua_isstring(L, 5)) {
	  const char* c = lua_tostring(L, 5);
	  while (*c != 0) {
	  	switch (*c) {
	  	  case 'c': { center = true;                    break; }
	  	  case 'r': { right = true;                     break; }
			  case 'n': { colorCodes = false;               break; }
			  case 'o': { outline = true; lightOut = false; break; }
			  case 'O': { outline = true; lightOut = true;  break; }
			}
	  	c++;
		}
	}

	const float yScale = size / fontHeight;
	const float xScale = yScale;
	float xj = x; // justified x position
	if (right) {
		xj -= xScale * font->CalcTextWidth(text.c_str());
	} else if (center) {
		xj -= xScale * font->CalcTextWidth(text.c_str()) * 0.5f;
	}
	glPushMatrix();
	glTranslatef(xj, y, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	if (!outline) {
		if (colorCodes) {
			font->glPrintColor("%s", text.c_str());
		} else {
			font->glPrint("%s", text.c_str());
		}
	} else {
		const float darkOutline[4]  = { 0.25f, 0.25f, 0.25f, 0.8f };
		const float lightOutline[4] = { 0.85f, 0.85f, 0.85f, 0.8f };
		const float noshow[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		const float xPixel = 1.0f / xScale;
		const float yPixel = 1.0f / yScale;
		if (lightOut) {
			font->glPrintOutlined(text.c_str(), xPixel, yPixel, noshow, lightOutline);
		} else {
			font->glPrintOutlined(text.c_str(), xPixel, yPixel, noshow, darkOutline);
		}
		if (colorCodes) {
			font->glPrintColor("%s", text.c_str());
		} else {
			font->glPrint("%s", text.c_str());
		}
	}
	glPopMatrix();

	return 0;
}


int LuaOpenGL::GetTextWidth(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.GetTextWidth(\"text\")");
	}
	const string text = lua_tostring(L, 1);
	const float width = font->CalcTextWidth(text.c_str()) / fontHeight;
	lua_pushnumber(L, width);
	return 1;
}


/******************************************************************************/

int LuaOpenGL::Unit(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.Unit(unitID)");
	}
	
	const int unitID = (int)lua_tonumber(L, 1);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		return 0;
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return 0;
	}
	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	const int readAllyTeam = lh->GetReadAllyTeam();
	if (readAllyTeam < 0) {
		if (readAllyTeam == CLuaHandle::NoAccessTeam) {
			return 0;
		}
	} else {
		if (!gs->Ally(readAllyTeam, unit->allyteam) &&
		    !(unit->losStatus[readAllyTeam] & LOS_INLOS)) {
			return 0;
		}
	}

	unitDrawer->DrawIndividual(unit);

	return 0;
}


int LuaOpenGL::UnitShape(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.UnitShape(unitDefID, team)");
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const int teamID = (int)lua_tonumber(L, 2);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}

	unitDrawer->DrawUnitDef(ud, teamID);

	return 0;
}


/******************************************************************************/

struct VertexData {
	float vert[3];
	float norm[3];
	float txcd[2];
	float color[4];
	bool hasVert;
	bool hasNorm;
	bool hasTxcd;
	bool hasColor;
};


static int ParseFloatArray(lua_State* L, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = (float)lua_tonumber(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}


static bool ParseVertexData(lua_State* L, VertexData& vd)
{
	vd.hasVert = vd.hasNorm = vd.hasTxcd = vd.hasColor = false;

	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1) || !lua_isstring(L, -2)) {
			luaL_error(L, "gl.Shape: bad vertex data row");
			lua_pop(L, 2); // pop the value and key
			return false;
		}

		const string key = lua_tostring(L, -2);

		if ((key == "v") || (key == "vertex")) {
			vd.vert[0] = 0.0f; vd.vert[1] = 0.0f; vd.vert[2] = 0.0f;
			if (ParseFloatArray(L, vd.vert, 3) >= 2) {
				vd.hasVert = true;
			} else {
				luaL_error(L, "gl.Shape: bad vertex array");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "n") || (key == "normal")) {
			vd.norm[0] = 0.0f; vd.norm[1] = 1.0f; vd.norm[2] = 0.0f;
			if (ParseFloatArray(L, vd.norm, 3) == 3) {
				vd.hasNorm = true;
			} else {
				luaL_error(L, "gl.Shape: bad normal array");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "t") || (key == "texcoord")) {
			vd.txcd[0] = 0.0f; vd.txcd[1] = 0.0f;
			if (ParseFloatArray(L, vd.txcd, 2) == 2) {
				vd.hasTxcd = true;
			} else {
				luaL_error(L, "gl.Shape: bad texcoord array");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "c") || (key == "color")) {
			vd.color[0] = 1.0f; vd.color[1] = 1.0f;
			vd.color[2] = 1.0f; vd.color[3] = 1.0f;
			if (ParseFloatArray(L, vd.color, 4) >= 0) {
				vd.hasColor = true;
			} else {
				luaL_error(L, "gl.Shape: bad color array");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
	}

	return vd.hasVert;
}


int LuaOpenGL::Shape(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_istable(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.Shape(type, elements[])");
	}

	const GLuint type = (GLuint)lua_tonumber(L, 1);

	glBegin(type);

	const int table = 2;
	int i = 1;
	for (lua_rawgeti(L, table, i);
	     lua_istable(L, -1);
       lua_pop(L, 1), i++, lua_rawgeti(L, table, i)) {
		VertexData vd;
		if (!ParseVertexData(L, vd)) {
			luaL_error(L, "Shape: bad vertex data");
			break;
		}
		if (vd.hasColor) { glColor4fv(vd.color);   }
		if (vd.hasTxcd)  { glTexCoord2fv(vd.txcd); }
		if (vd.hasNorm)  { glNormal3fv(vd.norm);   }
		if (vd.hasVert)  { glVertex3fv(vd.vert);   } // always last
	}
	if (!lua_isnil(L, -1)) {
		luaL_error(L, "Shape: bad vertex data, not a table");
	}
	// lua_pop(L, 1);

	glEnd();

	return 0;
}


/******************************************************************************/

int LuaOpenGL::BeginEnd(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.Shape(type, func, ...)");
	}
	const GLuint primMode = (GLuint)lua_tonumber(L, 1);

	// build the list with the specified lua call/args
	glBegin(primMode);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEnd();

	if (error != 0) {
		logOutput.Print("gl.BeginEnd: error(%i) = %s\n",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::Vertex(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (!lua_istable(L, 1)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		lua_rawgeti(L, 1, 1);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float y = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			glVertex2f(x, y);
			return 0;
		}
		const float z = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 4);
		if (!lua_isnumber(L, -1)) {
			glVertex3f(x, y, z);
			return 0;
		}
		const float w = (float)lua_tonumber(L, -1);
		glVertex4f(x, y, z, w);
		return 0;
	}

	if (args == 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = (float)lua_tonumber(L, 1);
		const float y = (float)lua_tonumber(L, 2);
		const float z = (float)lua_tonumber(L, 3);
		glVertex3f(x, y, z);
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = (float)lua_tonumber(L, 1);
		const float y = (float)lua_tonumber(L, 2);
		glVertex2f(x, y);
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = (float)lua_tonumber(L, 1);
		const float y = (float)lua_tonumber(L, 2);
		const float z = (float)lua_tonumber(L, 3);
		const float w = (float)lua_tonumber(L, 4);
		glVertex4f(x, y, z, w);
	}
	else {			
		luaL_error(L, "Incorrect arguments to gl.Vertex()");
	}

	return 0;
}


int LuaOpenGL::Normal(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (!lua_istable(L, 1)) {
			luaL_error(L, "Bad data passed to gl.Normal()");
		}
		lua_rawgeti(L, 1, 1);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Normal()");
		}
		const float x = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Normal()");
		}
		const float y = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Normal()");
		}
		const float z = (float)lua_tonumber(L, -1);
		glNormal3f(x, y, z);
		return 0;
	}

	if (args < 3) {
		luaL_error(L, "Incorrect arguments to gl.Normal()");
	}
	const float x = (float)lua_tonumber(L, 1);
	const float y = (float)lua_tonumber(L, 2);
	const float z = (float)lua_tonumber(L, 3);
	glNormal3f(x, y, z);
	return 0;
}


int LuaOpenGL::TexCoord(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (lua_isnumber(L, 1)) {
			const float x = (float)lua_tonumber(L, 1);
			glTexCoord1f(x);
			return 0;
		}
		if (!lua_istable(L, 1)) {
			luaL_error(L, "Bad 1data passed to gl.TexCoord()");
		}
		lua_rawgeti(L, 1, 1);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad 2data passed to gl.TexCoord()");
		}
		const float x = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			glTexCoord1f(x);
			return 0;
		}
		const float y = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			glTexCoord2f(x, y);
			return 0;
		}
		const float z = (float)lua_tonumber(L, -1);
		lua_rawgeti(L, 1, 4);
		if (!lua_isnumber(L, -1)) {
			glTexCoord3f(x, y, z);
			return 0;
		}
		const float w = (float)lua_tonumber(L, -1);
		glTexCoord4f(x, y, z, w);
		return 0;
	}

	if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Bad data passed to gl.TexCoord()");
		}
		const float x = (float)lua_tonumber(L, 1);
		const float y = (float)lua_tonumber(L, 2);
		glTexCoord2f(x, y);
	}
	else if (args == 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			luaL_error(L, "Bad data passed to gl.TexCoord()");
		}
		const float x = (float)lua_tonumber(L, 1);
		const float y = (float)lua_tonumber(L, 2);
		const float z = (float)lua_tonumber(L, 3);
		glTexCoord3f(x, y, z);
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Bad data passed to gl.TexCoord()");
		}
		const float x = (float)lua_tonumber(L, 1);
		const float y = (float)lua_tonumber(L, 2);
		const float z = (float)lua_tonumber(L, 3);
		const float w = (float)lua_tonumber(L, 4);
		glTexCoord4f(x, y, z, w);
	}
	else {			
		luaL_error(L, "Incorrect arguments to gl.TexCoord()");
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::Rect(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to gl.Rect()");
	}
	const float x1 = (float)lua_tonumber(L, 1);
	const float y1 = (float)lua_tonumber(L, 2);
	const float x2 = (float)lua_tonumber(L, 3);
	const float y2 = (float)lua_tonumber(L, 4);

	glRectf(x1, y1, x2, y2);
	return 0;
}


int LuaOpenGL::TexRect(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to gl.TexRect()");
	}
	const float x1 = (float)lua_tonumber(L, 1);
	const float y1 = (float)lua_tonumber(L, 2);
	const float x2 = (float)lua_tonumber(L, 3);
	const float y2 = (float)lua_tonumber(L, 4);

	// Spring's textures get loaded with a vertical flip
	// We change that for the default settings.

	if (args <= 6) {
		float s1 = 0.0f;
		float t1 = 1.0f;
		float s2 = 1.0f;
		float t2 = 0.0f;
		if ((args >= 5) && lua_isboolean(L, 5) && lua_toboolean(L, 5)) {
			// flip s-coords
			s1 = 1.0f;
			s2 = 0.0f;
		}
		if ((args >= 6) && lua_isboolean(L, 6) && lua_toboolean(L, 6)) {
			// flip t-coords
			t1 = 0.0f;
			t2 = 1.0f;
		}
		glBegin(GL_QUADS); {
			glTexCoord2f(s1, t1); glVertex2f(x1, y1);
			glTexCoord2f(s2, t1); glVertex2f(x2, y1);
			glTexCoord2f(s2, t2); glVertex2f(x2, y2);
			glTexCoord2f(s1, t2); glVertex2f(x1, y2);
		}
		glEnd();
		return 0;
	}

	if ((args < 8) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6) ||
	    !lua_isnumber(L, 7) || !lua_isnumber(L, 8)) {
		luaL_error(L, "Incorrect texcoord arguments to gl.TexRect()");
	}
	const float s1 = (float)lua_tonumber(L, 5);
	const float t1 = (float)lua_tonumber(L, 6);
	const float s2 = (float)lua_tonumber(L, 7);
	const float t2 = (float)lua_tonumber(L, 8);
	glBegin(GL_QUADS); {
		glTexCoord2f(s1, t1); glVertex2f(x1, y1);
		glTexCoord2f(s2, t1); glVertex2f(x2, y1);
		glTexCoord2f(s2, t2); glVertex2f(x2, y2);
		glTexCoord2f(s1, t2); glVertex2f(x1, y2);
	}
	glEnd();

	return 0;
}


/******************************************************************************/

int LuaOpenGL::Color(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args < 1) {
		luaL_error(L, "Incorrect arguments to gl.Color()");
	}

	float color[4];

	if (args == 1) {
		if (!lua_istable(L, 1)) {
			luaL_error(L, "Incorrect arguments to gl.Color()");
		}
		const int count = ParseFloatArray(L, color, 4);
		if (count < 3) {
			luaL_error(L, "Incorrect arguments to gl.Color()");
		}
		if (count == 3) {
			color[3] = 1.0f;
		}
	}
	else if (args >= 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			luaL_error(L, "Incorrect arguments to gl.Color()");
		}
		color[0] = (GLfloat)lua_tonumber(L, 1);
		color[1] = (GLfloat)lua_tonumber(L, 2);
		color[2] = (GLfloat)lua_tonumber(L, 3);
		if (args < 4) {
			color[3] = 1.0f;
		} else {
			if (!lua_isnumber(L, 4)) {
				luaL_error(L, "Incorrect arguments to gl.Color()");
			}
			color[3] = (GLfloat)lua_tonumber(L, 4);
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Color()");
	}

	glColor4fv(color);

	return 0;
}


int LuaOpenGL::Material(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.Material(table)");
	}

	float color[4];

	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isstring(L, -2)) { // the key
			logOutput.Print("gl.Material: bad state type\n");
			break;
		}
		const string key = lua_tostring(L, -2);

		if (key == "shininess") {
			if (lua_isnumber(L, -1)) {
				const GLfloat specExp = (GLfloat)lua_tonumber(L, -1);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, specExp);
			}
			continue;
		}

		const int count = ParseFloatArray(L, color, 4);
		if (count == 3) {
			color[3] = 1.0f;
		}

		if (key == "ambidiff") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
			}
		}
		else if (key == "ambient") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
			}
		}
		else if (key == "diffuse") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
			}
		}
		else if (key == "specular") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
			}
		}
		else if (key == "emission") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
			}
		}
		else {
			logOutput.Print("gl.Material: unknown material type: %s\n", key.c_str());
		}
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::ResetState(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "gl.ResetState takes no arguments");
	}

	glCallList(resetStateList);

	return 0;
}


int LuaOpenGL::ResetMatrices(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "gl.ResetMatrices takes no arguments");
	}

	resetMatrixFunc();

	return 0;
}


int LuaOpenGL::Lighting(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.Lighting()");
	}
	if (lua_toboolean(L, 1)) {
		glEnable(GL_LIGHTING);
	} else {
		glDisable(GL_LIGHTING);
	}
	return 0;
}


int LuaOpenGL::ShadeModel(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.ShadeModel()");
	}

	glShadeModel((GLenum)lua_tonumber(L, 1));

	return 0;
}


int LuaOpenGL::Scissor(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			luaL_error(L, "Incorrect arguments to gl.Scissor()");
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Incorrect arguments to gl.Scissor()");
		}
		glEnable(GL_SCISSOR_TEST);
		const GLint   x =   (GLint)lua_tonumber(L, 1);
		const GLint   y =   (GLint)lua_tonumber(L, 2);
		const GLsizei w = (GLsizei)lua_tonumber(L, 3);
		const GLsizei h = (GLsizei)lua_tonumber(L, 4);
		glScissor(x + gu->viewPosX, y + gu->viewPosY, w, h);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Scissor()");
	}

	return 0;
}


int LuaOpenGL::ColorMask(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args == 1) && lua_isboolean(L, 1)) {
		if (!lua_isboolean(L, 1)) {
			luaL_error(L, "Incorrect arguments to gl.ColorMask()");
		}
		if (lua_toboolean(L, 1)) {
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		} else {
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}
	}
	else if ((args == 4) &&
	         lua_isboolean(L, 1) && lua_isboolean(L, 1) &&
	         lua_isboolean(L, 3) && lua_isboolean(L, 4)) {
		glColorMask(lua_toboolean(L, 1), lua_toboolean(L, 2),
		            lua_toboolean(L, 3), lua_toboolean(L, 4));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.ColorMask()");
	}
	return 0;
}


int LuaOpenGL::DepthMask(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.DepthMask()");
	}
	if (lua_toboolean(L, 1)) {
		glDepthMask(GL_TRUE);
	} else {
		glDepthMask(GL_FALSE);
	}
	return 0;
}


int LuaOpenGL::DepthTest(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		luaL_error(L, "Incorrect arguments to gl.DepthTest()");
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
	}
	else if (lua_isnumber(L, 1)) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc((GLenum)lua_tonumber(L, 1));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.DepthTest()");
	}
	return 0;
}


int LuaOpenGL::Culling(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		luaL_error(L, "Incorrect arguments to gl.Culling()");
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_CULL_FACE);
		} else {
			glDisable(GL_CULL_FACE);
		}
	}
	else if (lua_isnumber(L, 1)) {
		glEnable(GL_CULL_FACE);
		glCullFace((GLenum)lua_tonumber(L, 1));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Culling()");
	}
	return 0;
}


int LuaOpenGL::LogicOp(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		luaL_error(L, "Incorrect arguments to gl.LogicOp()");
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_COLOR_LOGIC_OP);
		} else {
			glDisable(GL_COLOR_LOGIC_OP);
		}
	}
	else if (lua_isnumber(L, 1)) {
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp((GLenum)lua_tonumber(L, 1));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.LogicOp()");
	}
	return 0;
}


int LuaOpenGL::Blending(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			luaL_error(L, "Incorrect arguments to gl.Blending()");
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to gl.Blending()");
		}
		glEnable(GL_BLEND);
		glBlendFunc((GLenum)lua_tonumber(L, 1), (GLenum)lua_tonumber(L, 2));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Blending()");
	}
	return 0;
}


int LuaOpenGL::AlphaTest(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			luaL_error(L, "Incorrect arguments to gl.AlphaTest()");
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_ALPHA_TEST);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to gl.AlphaTest()");
		}
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc((GLenum)lua_tonumber(L, 1), (GLfloat)lua_tonumber(L, 2));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.AlphaTest()");
	}
	return 0;
}


int LuaOpenGL::PolygonMode(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.PolygonMode()");
	}
	const GLenum face = (GLenum)lua_tonumber(L, 1);
	const GLenum mode = (GLenum)lua_tonumber(L, 2);
	glPolygonMode(face, mode);
	return 0;
}


int LuaOpenGL::PolygonOffset(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			luaL_error(L, "Incorrect arguments to gl.PolygonOffset()");
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_POLYGON_OFFSET_FILL);
			glEnable(GL_POLYGON_OFFSET_LINE);
			glEnable(GL_POLYGON_OFFSET_POINT);
		} else {
			glDisable(GL_POLYGON_OFFSET_FILL);
			glDisable(GL_POLYGON_OFFSET_LINE);
			glDisable(GL_POLYGON_OFFSET_POINT);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to gl.PolygonOffset()");
		}
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glEnable(GL_POLYGON_OFFSET_POINT);
		glPolygonOffset((GLfloat)lua_tonumber(L, 1), (GLfloat)lua_tonumber(L, 2));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.PolygonOffset()");
	}
	return 0;
}


int LuaOpenGL::LineStipple(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (lua_isstring(L, 1)) { // we're ignoring the string value
			const unsigned int stipPat = (0xffff & cmdColors.StipplePattern());
			if ((stipPat != 0x0000) && (stipPat != 0xffff)) {
				glEnable(GL_LINE_STIPPLE);
				lineDrawer.SetupLineStipple();
			} else {
				glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (lua_isboolean(L, 1)) {
			if (lua_toboolean(L, 1)) {
				glEnable(GL_LINE_STIPPLE);
			} else {
				glDisable(GL_LINE_STIPPLE);
			}
		}
		else {
			luaL_error(L, "Incorrect arguments to gl.LineStipple()");
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to gl.LineStipple()");
		}
		GLint factor     =    (GLint)lua_tonumber(L, 1);
		GLushort pattern = (GLushort)lua_tonumber(L, 2);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(factor, pattern);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.LineStipple()");
	}
	return 0;
}


int LuaOpenGL::LineWidth(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.LineWidth()");
	}
	glLineWidth((float)lua_tonumber(L, 1));
	return 0;
}


int LuaOpenGL::PointSize(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.PointSize()");
	}
	glPointSize((float)lua_tonumber(L, 1));
	return 0;
}


int LuaOpenGL::Texture(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		luaL_error(L, "Incorrect arguments to gl.Texture()");
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_TEXTURE_2D);
		} else {
			glDisable(GL_TEXTURE_2D);
		}
		lua_pushboolean(L, 1);
		return 1;
	}

	if (!lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.Texture()");
	}

	const string texture = lua_tostring(L, 1);
	if (texture.empty()) {
		glDisable(GL_TEXTURE_2D); // equivalent to 'false'
		lua_pushboolean(L, 1);
		return 1;
	}
	else if (texture[0] != '#') {
		if (!CNamedTextures::Bind(texture)) {
			lua_pushboolean(L, 0);
			return 1;
		}
		glEnable(GL_TEXTURE_2D);
	}
	else {
		// unit build picture
		char* endPtr;
		const char* startPtr = texture.c_str() + 1; // skip the '#'
		int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		// UnitDefHandler's array size is (numUnits + 1)
		if (endPtr != startPtr) {
			UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
			if (ud != NULL) {
				glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitImage(ud));
				glEnable(GL_TEXTURE_2D);
			} else {
				lua_pushboolean(L, 0);
				return 1;
			}
		} else {
			lua_pushboolean(L, 0);
			return 1;
		}
	}
	lua_pushboolean(L, 1);
	return 1;
}


int LuaOpenGL::FreeTexture(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.FreeTexture()");
	}
	const string texture = lua_tostring(L, 1);
	if (!CNamedTextures::Free(texture)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, 1);
	}
	return 1;
}


int LuaOpenGL::TextureInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.TextureInfo()");
	}
	const string texture = lua_tostring(L, 1);
	if (texture[0] == '#') {
		char* endPtr;
		const char* startPtr = texture.c_str() + 1; // skip the '#'
		int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		// UnitDefHandler's array size is (numUnits + 1)
		if (endPtr == startPtr) {
			return 0;
		}
		else {
			UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
			if (ud == NULL) {
				return 0;
			}
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", ud->imageSizeX);
			HSTR_PUSH_NUMBER(L, "ysize", ud->imageSizeY);
			// HSTR_PUSH_BOOL(L,   "alpha", texInfo->alpha);  FIXME
			// HSTR_PUSH_NUMBER(L, "type",  texInfo->type);
		}
	}
	else {
		const CNamedTextures::TexInfo* texInfo;
		texInfo = CNamedTextures::GetInfo(texture);
		if (texInfo == NULL) {
			return 0;
		}
		lua_newtable(L);
		HSTR_PUSH_NUMBER(L, "xsize", texInfo->xsize);
		HSTR_PUSH_NUMBER(L, "ysize", texInfo->ysize);
		// HSTR_PUSH_BOOL(L,   "alpha", texInfo->alpha);  FIXME
		// HSTR_PUSH_NUMBER(L, "type",  texInfo->type);
	}
	return 1;
}


int LuaOpenGL::Clear(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.Clear()");
	}

	const GLbitfield bits = (GLbitfield)lua_tonumber(L, 1);
	if (args == 5) {
		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
		    !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
			luaL_error(L, "Incorrect arguments to Clear()");
		}
		if (bits == GL_COLOR_BUFFER_BIT) {
			glClearColor((GLfloat)lua_tonumber(L, 2), (GLfloat)lua_tonumber(L, 3),
			             (GLfloat)lua_tonumber(L, 4), (GLfloat)lua_tonumber(L, 5));
		}
		else if (bits == GL_ACCUM_BUFFER_BIT) {
			glClearAccum((GLfloat)lua_tonumber(L, 2), (GLfloat)lua_tonumber(L, 3),
			             (GLfloat)lua_tonumber(L, 4), (GLfloat)lua_tonumber(L, 5));
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to gl.Clear()");
		}
		if (bits == GL_DEPTH_BUFFER_BIT) {
			glClearDepth((GLfloat)lua_tonumber(L, 2));
		}
		else if (bits == GL_STENCIL_BUFFER_BIT) {
			glClearStencil((GLint)lua_tonumber(L, 2));
		}
	}

	glClear(bits);

	return 0;
}


/******************************************************************************/

int LuaOpenGL::Translate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to gl.Translate(x, y, z)");
	}
	const float x = (float)lua_tonumber(L, 1);
	const float y = (float)lua_tonumber(L, 2);
	const float z = (float)lua_tonumber(L, 3);
	glTranslatef(x, y, z);
	return 0;
}


int LuaOpenGL::Scale(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to gl.Scale(x, y, z)");
	}
	const float x = (float)lua_tonumber(L, 1);
	const float y = (float)lua_tonumber(L, 2);
	const float z = (float)lua_tonumber(L, 3);
	glScalef(x, y, z);
	return 0;
}


int LuaOpenGL::Rotate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to gl.Rotate(r, x, y, z)");
	}
	const float r = (float)lua_tonumber(L, 1);
	const float x = (float)lua_tonumber(L, 2);
	const float y = (float)lua_tonumber(L, 3);
	const float z = (float)lua_tonumber(L, 4);
	glRotatef(r, x, y, z);
	return 0;
}


/******************************************************************************/

int LuaOpenGL::ClipPlane(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.ClipPlane");
	}
	const int plane = (int)lua_tonumber(L, 1);
	if ((plane < 1) || (plane > 2)) {
		luaL_error(L, "gl.ClipPlane: bad plane number (use 1 or 2)");
	}
	// use GL_CLIP_PLANE4 and GL_CLIP_PLANE5 for LuaOpenGL  (6 are guaranteed)
	const GLenum gl_plane = GL_CLIP_PLANE4 + plane - 1;
	if (lua_isboolean(L, 2)) {
		if (lua_toboolean(L, 2)) {
			glEnable(gl_plane);
		} else {
			glDisable(gl_plane);
		}
		return 0;
	}
	if ((args < 5) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
	    !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
		luaL_error(L, "Incorrect arguments to gl.ClipPlane");
	}
	GLdouble equation[4];
	equation[0] = (double)lua_tonumber(L, 2);
	equation[1] = (double)lua_tonumber(L, 3);
	equation[2] = (double)lua_tonumber(L, 4);
	equation[3] = (double)lua_tonumber(L, 5);
	glClipPlane(gl_plane, equation); 
	glEnable(gl_plane);
	return 0;
}


/******************************************************************************/

int LuaOpenGL::MatrixMode(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.MatrixMode");
	}
	glMatrixMode((GLenum)lua_tonumber(L, 1));
	return 0;
}


int LuaOpenGL::LoadIdentity(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "gl.LoadIdentity takes no arguments");
	}
	glLoadIdentity();
	return 0;
}


int LuaOpenGL::LoadMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	GLfloat matrix[16];
	if (lua_istable(L, 1)) {
		if (ParseFloatArray(L, matrix, 16) != 16) {
			luaL_error(L, "gl.LoadMatrix requires all 16 values");
		}
	}
	else {
		for (int i = 1; i <= 16; i++) {
			matrix[i] = (GLfloat)luaL_checknumber(L, i);
		}
	}
	glLoadMatrixf(matrix);
	return 0;
}


int LuaOpenGL::MultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	GLfloat matrix[16];
	if (lua_istable(L, 1)) {
		if (ParseFloatArray(L, matrix, 16) != 16) {
			luaL_error(L, "gl.MultMatrix requires all 16 values");
		}
	}
	else {
		for (int i = 1; i <= 16; i++) {
			matrix[i] = (GLfloat)luaL_checknumber(L, i);
		}
	}
	glMultMatrixf(matrix);
	return 0;
}


int LuaOpenGL::PushMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "gl.PushMatrix takes no arguments");
	}

	glPushMatrix();

	return 0;
}


int LuaOpenGL::PopMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "gl.PopMatrix takes no arguments");
	}

	glPopMatrix();

	return 0;
}

/******************************************************************************/

int LuaOpenGL::ListCreate(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isfunction(L, 1)) {
		luaL_error(L,
			"Incorrect arguments to gl.ListCreate(func [, arg1, arg2, etc ...])");
	}

	// generate the list id
	const GLuint list = glGenLists(1);
	if (list == 0) {
		lua_pushnumber(L, 0);
		return 1;
	}

	// save the current state
	const bool origDrawingEnabled = drawingEnabled;
	drawingEnabled = true;

	// build the list with the specified lua call/args
	glNewList(list, GL_COMPILE);
	const int error = lua_pcall(L, (args - 1), 0, 0);
	glEndList();

	if (error != 0) {
		glDeleteLists(list, 1);
		logOutput.Print("gl.ListCreate: error(%i) = %s\n",
		                error, lua_tostring(L, -1));
		lua_pushnumber(L, 0);
	}
	else {
		CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
		const unsigned int index = displayLists.NewDList(list);
		lua_pushnumber(L, index);
	}

	// restore the state
	drawingEnabled = origDrawingEnabled;

	return 1;
}


int LuaOpenGL::ListRun(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.ListRun(list)");
	}
	const unsigned int listIndex = (unsigned int)lua_tonumber(L, 1);
	CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
	const unsigned int dlist = displayLists.GetDList(listIndex);
	if (dlist != 0) {
		glCallList(dlist);
	}
	return 0;
}


int LuaOpenGL::ListDelete(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.ListDelete(list)");
	}
	const unsigned int listIndex = (unsigned int)lua_tonumber(L, 1);
	CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
	const unsigned int dlist = displayLists.GetDList(listIndex);
	displayLists.FreeDList(listIndex);
	if (dlist != 0) {
		glDeleteLists(dlist, 1);
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/
