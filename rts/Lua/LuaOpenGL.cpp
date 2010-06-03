/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

// TODO:
// - go back to counting matrix push/pops (just for modelview?)
//   would have to make sure that display lists are also handled
//   (GL_MODELVIEW_STACK_DEPTH could help current situation, but
//    requires the ARB_imaging extension)
// - use materials instead of raw calls (again, handle dlists)

#include <string>
#include <vector>
#include <algorithm>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

#include "mmgr.h"

#include "LuaOpenGL.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaShaders.h"
#include "LuaTextures.h"
//FIXME#include "LuaVBOs.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
#include "LuaFonts.h"
#include "LuaDisplayLists.h"
#include "Game/Camera.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/MiniMap.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/HeightMapTexture.h"
#include "Rendering/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitDefImage.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "System/LogOutput.h"
#include "System/Matrix44f.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"

using std::max;
using std::string;
using std::vector;
using std::set;

#undef far // avoid collision with windef.h
#undef near

static const int MAX_TEXTURE_UNITS = 32;


/******************************************************************************/
/******************************************************************************/

void (*LuaOpenGL::resetMatrixFunc)(void) = NULL;

unsigned int LuaOpenGL::resetStateList = 0;
set<unsigned int> LuaOpenGL::occlusionQueries;

LuaOpenGL::DrawMode LuaOpenGL::drawMode = LuaOpenGL::DRAW_NONE;
LuaOpenGL::DrawMode LuaOpenGL::prevDrawMode = LuaOpenGL::DRAW_NONE;

bool  LuaOpenGL::drawingEnabled = false;
bool  LuaOpenGL::safeMode = true;
bool  LuaOpenGL::canUseShaders = false;
float LuaOpenGL::screenWidth = 0.36f;
float LuaOpenGL::screenDistance = 0.60f;

/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::Init()
{
	resetStateList = glGenLists(1);
	glNewList(resetStateList, GL_COMPILE); {
		ResetGLState();
	}
	glEndList();

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	if (globalRendering->haveGLSL && !!configHandler->Get("LuaShaders", 1)) {
		canUseShaders = true;
	}
}


void LuaOpenGL::Free()
{
	glDeleteLists(resetStateList, 1);

	if (globalRendering->haveGLSL) {
		set<unsigned int>::const_iterator it;
		for (it = occlusionQueries.begin(); it != occlusionQueries.end(); ++it) {
			glDeleteQueries(1, &(*it));
		}
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaOpenGL::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(HasExtension);
	REGISTER_LUA_CFUNC(GetNumber);
	REGISTER_LUA_CFUNC(GetString);

	REGISTER_LUA_CFUNC(ConfigScreen);

	REGISTER_LUA_CFUNC(GetViewSizes);

	REGISTER_LUA_CFUNC(DrawMiniMap);
	REGISTER_LUA_CFUNC(SlaveMiniMap);
	REGISTER_LUA_CFUNC(ConfigMiniMap);

	REGISTER_LUA_CFUNC(ResetState);
	REGISTER_LUA_CFUNC(ResetMatrices);
	REGISTER_LUA_CFUNC(Clear);
	REGISTER_LUA_CFUNC(Lighting);
	REGISTER_LUA_CFUNC(ShadeModel);
	REGISTER_LUA_CFUNC(Scissor);
	REGISTER_LUA_CFUNC(Viewport);
	REGISTER_LUA_CFUNC(ColorMask);
	REGISTER_LUA_CFUNC(DepthMask);
	REGISTER_LUA_CFUNC(DepthTest);
	if (GLEW_NV_depth_clamp) {
		REGISTER_LUA_CFUNC(DepthClamp);
	}
	REGISTER_LUA_CFUNC(Culling);
	REGISTER_LUA_CFUNC(LogicOp);
	REGISTER_LUA_CFUNC(Fog);
	REGISTER_LUA_CFUNC(Smoothing);
	REGISTER_LUA_CFUNC(AlphaTest);
	REGISTER_LUA_CFUNC(LineStipple);
	REGISTER_LUA_CFUNC(Blending);
	REGISTER_LUA_CFUNC(BlendEquation);
	REGISTER_LUA_CFUNC(BlendFunc);
	if (globalRendering->haveGLSL) {
		REGISTER_LUA_CFUNC(BlendEquationSeparate);
		REGISTER_LUA_CFUNC(BlendFuncSeparate);
	}

	REGISTER_LUA_CFUNC(Material);
	REGISTER_LUA_CFUNC(Color);

	REGISTER_LUA_CFUNC(PolygonMode);
	REGISTER_LUA_CFUNC(PolygonOffset);

	REGISTER_LUA_CFUNC(StencilTest);
	REGISTER_LUA_CFUNC(StencilMask);
	REGISTER_LUA_CFUNC(StencilFunc);
	REGISTER_LUA_CFUNC(StencilOp);
	if (globalRendering->haveGLSL) {
		REGISTER_LUA_CFUNC(StencilMaskSeparate);
		REGISTER_LUA_CFUNC(StencilFuncSeparate);
		REGISTER_LUA_CFUNC(StencilOpSeparate);
	}

	REGISTER_LUA_CFUNC(LineWidth);
	REGISTER_LUA_CFUNC(PointSize);
	if (globalRendering->haveGLSL) {
		REGISTER_LUA_CFUNC(PointSprite);
		REGISTER_LUA_CFUNC(PointParameter);
	}

	REGISTER_LUA_CFUNC(Texture);
	REGISTER_LUA_CFUNC(CreateTexture);
	REGISTER_LUA_CFUNC(DeleteTexture);
	REGISTER_LUA_CFUNC(TextureInfo);
	REGISTER_LUA_CFUNC(CopyToTexture);
	if (GLEW_EXT_framebuffer_object) {
		// FIXME: obsolete
		REGISTER_LUA_CFUNC(DeleteTextureFBO);
		REGISTER_LUA_CFUNC(RenderToTexture);
	}
	if (glGenerateMipmapEXT_NONGML) {
		REGISTER_LUA_CFUNC(GenerateMipmap);
	}
	REGISTER_LUA_CFUNC(ActiveTexture);
	REGISTER_LUA_CFUNC(TexEnv);
	REGISTER_LUA_CFUNC(MultiTexEnv);
	REGISTER_LUA_CFUNC(TexGen);
	REGISTER_LUA_CFUNC(MultiTexGen);

	REGISTER_LUA_CFUNC(Shape);
	REGISTER_LUA_CFUNC(BeginEnd);
	REGISTER_LUA_CFUNC(Vertex);
	REGISTER_LUA_CFUNC(Normal);
	REGISTER_LUA_CFUNC(TexCoord);
	REGISTER_LUA_CFUNC(MultiTexCoord);
	REGISTER_LUA_CFUNC(SecondaryColor);
	REGISTER_LUA_CFUNC(FogCoord);
	REGISTER_LUA_CFUNC(EdgeFlag);

	REGISTER_LUA_CFUNC(Rect);
	REGISTER_LUA_CFUNC(TexRect);

	REGISTER_LUA_CFUNC(BeginText);
	REGISTER_LUA_CFUNC(Text);
	REGISTER_LUA_CFUNC(EndText);
	REGISTER_LUA_CFUNC(GetTextWidth);
	REGISTER_LUA_CFUNC(GetTextHeight);

	REGISTER_LUA_CFUNC(Map1);
	REGISTER_LUA_CFUNC(Map2);
	REGISTER_LUA_CFUNC(MapGrid1);
	REGISTER_LUA_CFUNC(MapGrid2);
	REGISTER_LUA_CFUNC(Eval);
	REGISTER_LUA_CFUNC(EvalEnable);
	REGISTER_LUA_CFUNC(EvalDisable);
	REGISTER_LUA_CFUNC(EvalMesh1);
	REGISTER_LUA_CFUNC(EvalMesh2);
	REGISTER_LUA_CFUNC(EvalCoord1);
	REGISTER_LUA_CFUNC(EvalCoord2);
	REGISTER_LUA_CFUNC(EvalPoint1);
	REGISTER_LUA_CFUNC(EvalPoint2);

	REGISTER_LUA_CFUNC(Unit);
	REGISTER_LUA_CFUNC(UnitRaw);
	REGISTER_LUA_CFUNC(UnitShape);
	REGISTER_LUA_CFUNC(UnitMultMatrix);
	REGISTER_LUA_CFUNC(UnitPiece);
	REGISTER_LUA_CFUNC(UnitPieceMatrix);
	REGISTER_LUA_CFUNC(UnitPieceMultMatrix);
	REGISTER_LUA_CFUNC(Feature);
	REGISTER_LUA_CFUNC(FeatureShape);
	REGISTER_LUA_CFUNC(DrawListAtUnit);
	REGISTER_LUA_CFUNC(DrawFuncAtUnit);
	REGISTER_LUA_CFUNC(DrawGroundCircle);
	REGISTER_LUA_CFUNC(DrawGroundQuad);

	REGISTER_LUA_CFUNC(Light);
	REGISTER_LUA_CFUNC(ClipPlane);

	REGISTER_LUA_CFUNC(MatrixMode);
	REGISTER_LUA_CFUNC(LoadIdentity);
	REGISTER_LUA_CFUNC(LoadMatrix);
	REGISTER_LUA_CFUNC(MultMatrix);
	REGISTER_LUA_CFUNC(Translate);
	REGISTER_LUA_CFUNC(Scale);
	REGISTER_LUA_CFUNC(Rotate);
	REGISTER_LUA_CFUNC(Ortho);
	REGISTER_LUA_CFUNC(Frustum);
	REGISTER_LUA_CFUNC(PushMatrix);
	REGISTER_LUA_CFUNC(PopMatrix);
	REGISTER_LUA_CFUNC(PushPopMatrix);
	REGISTER_LUA_CFUNC(Billboard);
	REGISTER_LUA_CFUNC(GetMatrixData);

	REGISTER_LUA_CFUNC(PushAttrib);
	REGISTER_LUA_CFUNC(PopAttrib);
	REGISTER_LUA_CFUNC(UnsafeState);

	REGISTER_LUA_CFUNC(CreateList);
	REGISTER_LUA_CFUNC(CallList);
	REGISTER_LUA_CFUNC(DeleteList);

	REGISTER_LUA_CFUNC(Flush);
	REGISTER_LUA_CFUNC(Finish);

	REGISTER_LUA_CFUNC(ReadPixels);
	REGISTER_LUA_CFUNC(SaveImage);

	if (globalRendering->haveGLSL) {
		REGISTER_LUA_CFUNC(CreateQuery);
		REGISTER_LUA_CFUNC(DeleteQuery);
		REGISTER_LUA_CFUNC(RunQuery);
		REGISTER_LUA_CFUNC(GetQuery);
	}

	REGISTER_LUA_CFUNC(GetGlobalTexNames);
	REGISTER_LUA_CFUNC(GetGlobalTexCoords);
	REGISTER_LUA_CFUNC(GetShadowMapParams);

	REGISTER_LUA_CFUNC(GetSun);

	if (canUseShaders) {
		LuaShaders::PushEntries(L);
	}

	if (GLEW_EXT_framebuffer_object) {
	 	LuaFBOs::PushEntries(L);
	 	LuaRBOs::PushEntries(L);
	}

	LuaFonts::PushEntries(L);

//FIXME		LuaVBOs::PushEntries(L);

	REGISTER_LUA_CFUNC(RenderMode);
	REGISTER_LUA_CFUNC(SelectBuffer);
	REGISTER_LUA_CFUNC(SelectBufferData);
	REGISTER_LUA_CFUNC(InitNames);
	REGISTER_LUA_CFUNC(PushName);
	REGISTER_LUA_CFUNC(PopName);
	REGISTER_LUA_CFUNC(LoadName);

	return true;
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::ClearMatrixStack(int stackDepthEnum)
{
	GLint depth = 0;

	glGetIntegerv(stackDepthEnum, &depth);

	for (int i = 0; i < depth - 1; i++) {
		glPopMatrix();
	}
}


void LuaOpenGL::ResetGLState()
{
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	if (GLEW_NV_depth_clamp) {
		glDisable(GL_DEPTH_CLAMP_NV);
	}
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_BLEND);
	if (glBlendEquation_NONGML != NULL) {
		glBlendEquation(GL_FUNC_ADD);
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);

	glDisable(GL_LIGHTING);

	glShadeModel(GL_SMOOTH);

	glDisable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);

	// FIXME glViewport(gl); depends on the mode

	// FIXME -- depends on the mode       glDisable(GL_FOG);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDisable(GL_SCISSOR_TEST);

	glDisable(GL_STENCIL_TEST);
	glStencilMask(~0);
	if (GLEW_EXT_stencil_two_side) {
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	// FIXME -- multitexturing
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glDisable(GL_POLYGON_OFFSET_POINT);

	glDisable(GL_LINE_STIPPLE);

	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);

	glLineWidth(1.0f);
	glPointSize(1.0f);

	if (globalRendering->haveGLSL) {
		glDisable(GL_POINT_SPRITE);
	}
	if (globalRendering->haveGLSL) {
		GLfloat atten[3] = { 1.0f, 0.0f, 0.0f };
		glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, atten);
		glPointParameterf(GL_POINT_SIZE_MIN, 0.0f);
		glPointParameterf(GL_POINT_SIZE_MAX, 1.0e9f); // FIXME?
		glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	const float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	const float diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	const float black[4]   = { 0.0f, 0.0f, 0.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

	if (glUseProgram_NONGML) {
		glUseProgram(0);
	}
}


/******************************************************************************/
/******************************************************************************/
//
//  Common routines
//

const GLbitfield AttribBits =
	GL_COLOR_BUFFER_BIT |
	GL_DEPTH_BUFFER_BIT |
	GL_ENABLE_BIT       |
	GL_LIGHTING_BIT     |
	GL_LINE_BIT         |
	GL_POINT_BIT        |
	GL_POLYGON_BIT      |
	GL_VIEWPORT_BIT;


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
	//glEnable(GL_NORMALIZE);
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
	if (glUseProgram_NONGML) {
		glUseProgram(0);
	}
}


/******************************************************************************/
//
//  Genesis
//

void LuaOpenGL::EnableDrawGenesis()
{
	EnableCommon(DRAW_GENESIS);
	resetMatrixFunc = ResetGenesisMatrices;
	ResetGenesisMatrices();
	SetupWorldLighting();
}


void LuaOpenGL::DisableDrawGenesis()
{
	if (safeMode) {
		ResetGenesisMatrices();
	}
	RevertWorldLighting();
	DisableCommon(DRAW_GENESIS);
}


void LuaOpenGL::ResetDrawGenesis()
{
	if (safeMode) {
		ResetGenesisMatrices();
		glCallList(resetStateList);
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
//  WorldPreUnit -- the same as World
//

void LuaOpenGL::EnableDrawWorldPreUnit()
{
	EnableCommon(DRAW_WORLD);
	resetMatrixFunc = ResetWorldMatrices;
	SetupWorldLighting();
}

void LuaOpenGL::DisableDrawWorldPreUnit()
{
	if (safeMode) {
		ResetWorldMatrices();
	}
	RevertWorldLighting();
	DisableCommon(DRAW_WORLD);
}

void LuaOpenGL::ResetDrawWorldPreUnit()
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

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();
}

void LuaOpenGL::DisableDrawWorldShadow()
{
	glDisable(GL_POLYGON_OFFSET_FILL);

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Disable();

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
//  ScreenEffects -- same as Screen
//

void LuaOpenGL::EnableDrawScreenEffects()
{
	EnableCommon(DRAW_SCREEN);
	resetMatrixFunc = ResetScreenMatrices;

	SetupScreenMatrices();
	SetupScreenLighting();
	glCallList(resetStateList);
	//glEnable(GL_NORMALIZE);
}


void LuaOpenGL::DisableDrawScreenEffects()
{
	RevertScreenLighting();
	RevertScreenMatrices();
	DisableCommon(DRAW_SCREEN);
}


void LuaOpenGL::ResetDrawScreenEffects()
{
	if (safeMode) {
		ResetScreenMatrices();
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
	//glEnable(GL_NORMALIZE);
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
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
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
	glLightfv(GL_LIGHT1, GL_POSITION, mapInfo->light.sunDir);
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
	const float vppx   = float(globalRendering->winPosX + globalRendering->viewPosX); // view pixel pos x
	const float vppy   = float(globalRendering->winPosY + globalRendering->viewPosY); // view pixel pos y
	const float vpsx   = float(globalRendering->viewSizeX);   // view pixel size x
	const float vpsy   = float(globalRendering->viewSizeY);   // view pixel size y
	const float spsx   = float(globalRendering->screenSizeX); // screen pixel size x
	const float spsy   = float(globalRendering->screenSizeY); // screen pixel size x
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
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack(GL_PROJECTION_STACK_DEPTH);
		glLoadIdentity();
		gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack(GL_MODELVIEW_STACK_DEPTH);
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
	glLoadMatrixd(camera->GetViewMat());
	glLightfv(GL_LIGHT1, GL_POSITION, mapInfo->light.sunDir);

	const float sunFactor = 1.0f;
	const float sf = sunFactor;
	const float* la = mapInfo->light.unitAmbientColor;
	const float* ld = mapInfo->light.unitSunColor;

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

void LuaOpenGL::ResetGenesisMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack(GL_PROJECTION_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack(GL_MODELVIEW_STACK_DEPTH);
		glLoadIdentity();
	}
}


void LuaOpenGL::ResetWorldMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack(GL_PROJECTION_STACK_DEPTH);
		glLoadMatrixd(camera->GetProjMat());
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack(GL_MODELVIEW_STACK_DEPTH);
		glLoadMatrixd(camera->GetViewMat());
	}
}


void LuaOpenGL::ResetWorldShadowMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack(GL_PROJECTION_STACK_DEPTH);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, -1.0);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack(GL_MODELVIEW_STACK_DEPTH);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
	}
}


void LuaOpenGL::ResetScreenMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack(GL_PROJECTION_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack(GL_MODELVIEW_STACK_DEPTH);
		glLoadIdentity();
	}
	SetupScreenMatrices();
}


void LuaOpenGL::ResetMiniMapMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		ClearMatrixStack(GL_TEXTURE_STACK_DEPTH);
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		ClearMatrixStack(GL_PROJECTION_STACK_DEPTH);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0e6, +1.0e6);
	}
	glMatrixMode(GL_MODELVIEW); {
		ClearMatrixStack(GL_MODELVIEW_STACK_DEPTH);
		glLoadIdentity();
		if (minimap) {
			glScalef(1.0f / (float)minimap->GetSizeX(),
			         1.0f / (float)minimap->GetSizeY(), 1.0f);
		}
	}
}


/******************************************************************************/
/******************************************************************************/

static inline bool CheckModUICtrl()
{
	return CLuaHandle::GetModUICtrl() ||
	       CLuaHandle::GetActiveHandle()->GetUserMode();
}


inline void LuaOpenGL::CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!drawingEnabled) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
}


static int ParseFloatArray(lua_State* L, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = lua_tofloat(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}


/******************************************************************************/

int LuaOpenGL::HasExtension(lua_State* L)
{
	const char* extName = luaL_checkstring(L, 1);
	lua_pushboolean(L, glewIsSupported(extName));
	return 1;
}


int LuaOpenGL::GetNumber(lua_State* L)
{
	const GLenum pname = (GLenum) luaL_checknumber(L, 1);
	const GLuint count = (GLuint) luaL_optnumber(L, 2, 1);
	if (count > 64) {
		return 0;
	}
	GLfloat values[64];
	glGetFloatv(pname, values);
	for (GLuint i = 0; i < count; i++) {
		lua_pushnumber(L, values[i]);
	}
	return count;
}


int LuaOpenGL::GetString(lua_State* L)
{
	const GLenum pname = (GLenum) luaL_checknumber(L, 1);
	lua_pushstring(L, (const char*)glGetString(pname));
	return 1;
}


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
	screenWidth = lua_tofloat(L, 1);
	screenDistance = lua_tofloat(L, 2);
	return 0;
}


int LuaOpenGL::GetViewSizes(lua_State* L)
{
	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);
	return 2;
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
	const int px = lua_toint(L, 1);
	const int py = lua_toint(L, 2);
	const int sx = lua_toint(L, 3);
	const int sy = lua_toint(L, 4);
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

	bool transform = true;
	if (lua_isboolean(L, 1)) {
		transform = lua_toboolean(L, 1);
	}

	if (transform) {
		glPushMatrix();
		glScalef(globalRendering->viewSizeX,globalRendering->viewSizeY,1.0f);

		minimap->DrawForReal(true);

		glPopMatrix();
	} else {
		minimap->DrawForReal(false);
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/
//
//  Font Renderer
//

int LuaOpenGL::BeginText(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	font->Begin();
	return 0;
}


int LuaOpenGL::EndText(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	font->End();
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
	const string text(lua_tostring(L, 1),lua_strlen(L, 1));
	const float x     = lua_tonumber(L, 2);
	const float y     = lua_tonumber(L, 3);

	float size = 12.0f;
	int options = FONT_NEAREST;
	bool outline = false;
	bool lightOut = false;

	if ((args >= 4) && lua_isnumber(L, 4)) {
		size = lua_tonumber(L, 4);
	}
	if ((args >= 5) && lua_isstring(L, 5)) {
		const char* c = lua_tostring(L, 5);
		while (*c != 0) {
	  		switch (*c) {
				case 'c': { options |= FONT_CENTER;       break; }
				case 'r': { options |= FONT_RIGHT;        break; }

				case 'a': { options |= FONT_ASCENDER;     break; }
				case 't': { options |= FONT_TOP;          break; }
				case 'v': { options |= FONT_VCENTER;      break; }
				case 'x': { options |= FONT_BASELINE;     break; }
				case 'b': { options |= FONT_BOTTOM;       break; }
				case 'd': { options |= FONT_DESCENDER;    break; }

				case 's': { options |= FONT_SHADOW;       break; }
				case 'o': { options |= FONT_OUTLINE; outline = true; lightOut = false;     break; }
				case 'O': { options |= FONT_OUTLINE; outline = true; lightOut = true;     break; }

				case 'n': { options ^= FONT_NEAREST;       break; }
			}
	  		c++;
		}
	}

	if (outline) {
		if (lightOut) {
			font->SetOutlineColor(0.95f, 0.95f, 0.95f, 0.8f);
		} else {
			font->SetOutlineColor(0.15f, 0.15f, 0.15f, 0.8f);
		}
	}

	font->glPrint(x, y, size, options, text);

	return 0;
}


int LuaOpenGL::GetTextWidth(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.GetTextWidth(\"text\")");
	}

	const string text(lua_tostring(L, 1),lua_strlen(L, 1));
	const float width = font->GetTextWidth(text);
	lua_pushnumber(L, width);
	return 1;
}


int LuaOpenGL::GetTextHeight(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.GetTextHeight(\"text\")");
	}

	const string text(lua_tostring(L, 1),lua_strlen(L, 1));
	float descender;
	int lines;

	const float height = font->GetTextHeight(text,&descender,&lines);
	lua_pushnumber(L, height);
	lua_pushnumber(L, descender);
	lua_pushnumber(L, lines);
	return 3;
}


/******************************************************************************/
/******************************************************************************/
//
//  GL evaluators
//

static int evalDepth = 0;


static int GetMap1TargetDataSize(GLenum target)
{
	switch (target) {
		case GL_MAP1_COLOR_4:         { return 4; }
		case GL_MAP1_INDEX:           { return 1; }
		case GL_MAP1_NORMAL:          { return 3; }
		case GL_MAP1_VERTEX_3:        { return 3; }
		case GL_MAP1_VERTEX_4:        { return 4; }
		case GL_MAP1_TEXTURE_COORD_1: { return 1; }
		case GL_MAP1_TEXTURE_COORD_2: { return 2; }
		case GL_MAP1_TEXTURE_COORD_3: { return 3; }
		case GL_MAP1_TEXTURE_COORD_4: { return 4; }
		default:                      { break; }
	}
	return 0;
}

static int GetMap2TargetDataSize(GLenum target)
{
	switch (target) {
		case GL_MAP2_COLOR_4:         { return 4; }
		case GL_MAP2_INDEX:           { return 1; }
		case GL_MAP2_NORMAL:          { return 3; }
		case GL_MAP2_VERTEX_3:        { return 3; }
		case GL_MAP2_VERTEX_4:        { return 4; }
		case GL_MAP2_TEXTURE_COORD_1: { return 1; }
		case GL_MAP2_TEXTURE_COORD_2: { return 2; }
		case GL_MAP2_TEXTURE_COORD_3: { return 3; }
		case GL_MAP2_TEXTURE_COORD_4: { return 4; }
		default:                      { break; }
	}
	return 0;
}



int LuaOpenGL::Eval(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	if (!lua_isfunction(L, 1)) {
		return 0;
	}

	if (evalDepth == 0) { glPushAttrib(GL_EVAL_BIT); }
	evalDepth++;
	const int error = lua_pcall(L, lua_gettop(L) - 1, 0, 0);
	evalDepth--;
	if (evalDepth == 0) { glPopAttrib(); }

	if (error != 0) {
		logOutput.Print("gl.Eval: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}
	return 0;
}


int LuaOpenGL::EvalEnable(lua_State* L)
{
	if (evalDepth <= 0) {
		luaL_error(L, "EvalState can only be used in Eval() blocks");
	}
	const GLenum target = (GLenum)luaL_checkint(L, 1);
	if ((GetMap1TargetDataSize(target) > 0) ||
	    (GetMap2TargetDataSize(target) > 0) ||
	    (target == GL_AUTO_NORMAL)) {
		glEnable(target);
	}
	return 0;
}


int LuaOpenGL::EvalDisable(lua_State* L)
{
	if (evalDepth <= 0) {
		luaL_error(L, "EvalState can only be used in Eval() blocks");
	}
	const GLenum target = (GLenum)luaL_checkint(L, 1);
	if ((GetMap1TargetDataSize(target) > 0) ||
	    (GetMap2TargetDataSize(target) > 0) ||
	    (target == GL_AUTO_NORMAL)) {
		glDisable(target);
	}
	return 0;
}


int LuaOpenGL::Map1(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	if (lua_gettop(L) != 6) { // NOTE: required for ParseFloatArray()
		return 0;
	}
	const GLenum  target = (GLenum)luaL_checkint(L, 1);
	const GLfloat u1     = luaL_checkfloat(L, 2);
	const GLfloat u2     = luaL_checkfloat(L, 3);
	const GLint   stride = luaL_checkint(L, 4);
	const GLint   order  = luaL_checkint(L, 5);

	const int dataSize = GetMap1TargetDataSize(target);
	if (dataSize <= 0) {
		return 0;
	}
	if ((order <= 0) || (stride != dataSize)) {
		return 0;
	}
	const int fullSize = (order * dataSize);
	float* points = new float[fullSize];
	if (ParseFloatArray(L, points, fullSize) == fullSize) {
		glMap1f(target, u1, u2, stride, order, points);
	}
	delete[] points;
	return 0;
}


int LuaOpenGL::Map2(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	if (lua_gettop(L) != 10) { // NOTE: required for ParseFloatArray()
		return 0;
	}

	const GLenum  target  = (GLenum)luaL_checkint(L, 1);
	const GLfloat u1      = luaL_checkfloat(L, 2);
	const GLfloat u2      = luaL_checkfloat(L, 3);
	const GLint   ustride = luaL_checkint(L, 4);
	const GLint   uorder  = luaL_checkint(L, 5);
	const GLfloat v1      = luaL_checkfloat(L, 6);
	const GLfloat v2      = luaL_checkfloat(L, 7);
	const GLint   vstride = luaL_checkint(L, 8);
	const GLint   vorder  = luaL_checkint(L, 9);

	const int dataSize = GetMap2TargetDataSize(target);
	if (dataSize <= 0) {
		return 0;
	}
	if ((uorder  <= 0) || (vorder  <= 0) ||
	    (ustride != dataSize) || (vstride != (dataSize * uorder))) {
		return 0;
	}
	const int fullSize = (uorder * vorder * dataSize);
	float* points = new float[fullSize];
	if (ParseFloatArray(L, points, fullSize) == fullSize) {
		glMap2f(target, u1, u2, ustride, uorder,
										v1, v2, vstride, vorder, points);
	}
	delete[] points;
	return 0;
}


int LuaOpenGL::MapGrid1(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLint   un = luaL_checkint(L, 1);
	const GLfloat u1 = luaL_checkfloat(L, 2);
	const GLfloat u2 = luaL_checkfloat(L, 3);
	glMapGrid1f(un, u1, u2);
	return 0;
}


int LuaOpenGL::MapGrid2(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLint   un = luaL_checkint(L, 1);
	const GLfloat u1 = luaL_checkfloat(L, 2);
	const GLfloat u2 = luaL_checkfloat(L, 3);
	const GLint   vn = luaL_checkint(L, 4);
	const GLfloat v1 = luaL_checkfloat(L, 5);
	const GLfloat v2 = luaL_checkfloat(L, 6);
	glMapGrid2f(un, u1, u2, vn, v1, v2);
	return 0;
}


int LuaOpenGL::EvalMesh1(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum mode = (GLenum)luaL_checkint(L, 1);
	const GLint  i1   = luaL_checkint(L, 2);
	const GLint  i2   = luaL_checkint(L, 3);
	glEvalMesh1(mode, i1, i2);
	return 0;
}


int LuaOpenGL::EvalMesh2(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum mode = (GLenum)luaL_checkint(L, 1);
	const GLint  i1   = luaL_checkint(L, 2);
	const GLint  i2   = luaL_checkint(L, 3);
	const GLint  j1   = luaL_checkint(L, 4);
	const GLint  j2   = luaL_checkint(L, 5);
	glEvalMesh2(mode, i1, i2, j1, j2);
	return 0;
}


int LuaOpenGL::EvalCoord1(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLfloat u = luaL_checkfloat(L, 1);
	glEvalCoord1f(u);
	return 0;
}


int LuaOpenGL::EvalCoord2(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLfloat u = luaL_checkfloat(L, 1);
	const GLfloat v = luaL_checkfloat(L, 2);
	glEvalCoord2f(u, v);
	return 0;
}


int LuaOpenGL::EvalPoint1(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLint i = luaL_checkint(L, 1);
	glEvalPoint1(i);
	return 0;
}


int LuaOpenGL::EvalPoint2(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLint i = luaL_checkint(L, 1);
	const GLint j = luaL_checkint(L, 1);
	glEvalPoint2(i, j);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		} else {
			return NULL;
		}
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}

	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	const int readAllyTeam = lh->GetReadAllyTeam();
	if (readAllyTeam < 0) {
		if (readAllyTeam == CEventClient::NoAccessTeam) {
			return NULL;
		}
	} else {
		if (!teamHandler->Ally(readAllyTeam, unit->allyteam) &&
		    !(unit->losStatus[readAllyTeam] & LOS_INLOS)) {
			return NULL;
		}
	}
	return unit;
}


/******************************************************************************/

int LuaOpenGL::Unit(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const bool rawDraw = lua_isboolean(L, 2) && lua_toboolean(L, 2);

	bool useLOD = true;
	if (unit->lodCount <= 0) {
		useLOD = false;
	}
	else {
		unsigned int lod;
		if (!lua_isnumber(L, 3)) {
			const LuaMatType matType =
				(water->drawReflection) ? LUAMAT_OPAQUE_REFLECT : LUAMAT_OPAQUE;
			lod = unit->CalcLOD(unit->luaMats[matType].GetLastLOD());
		} else {
			int tmpLod = lua_toint(L, 3);
			if (tmpLod < 0) {
				lod = 0;
				useLOD = false;
			} else {
				lod = std::min(unit->lodCount - 1, (unsigned int)tmpLod);
			}
		}
		unit->currentLOD = lod;
	}

	glPushAttrib(GL_ENABLE_BIT);

	if (rawDraw) {
		if (useLOD) {
			unitDrawer->DrawUnitRaw(unit);
		} else {
			const unsigned int origLodCount = unit->lodCount;
			unit->lodCount = 0;
			unitDrawer->DrawUnitRaw(unit);
			unit->lodCount = origLodCount;
		}
	}
	else {
		if (useLOD) {
			unitDrawer->DrawIndividual(unit);
		} else {
			const unsigned int origLodCount = unit->lodCount;
			unit->lodCount = 0;
			unitDrawer->DrawIndividual(unit);
			unit->lodCount = origLodCount;
		}
	}

	glPopAttrib();

	return 0;
}


int LuaOpenGL::UnitRaw(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const bool rawDraw = lua_isboolean(L, 2) && lua_toboolean(L, 2);

	bool useLOD = true;
	if (unit->lodCount <= 0) {
		useLOD = false;
	}
	else {
		unsigned int lod = 0;
		if (!lua_isnumber(L, 3)) {
			const LuaMatType matType =
				(water->drawReflection) ? LUAMAT_OPAQUE_REFLECT : LUAMAT_OPAQUE;
			lod = unit->CalcLOD(unit->luaMats[matType].GetLastLOD());
		} else {
			int tmpLod = lua_toint(L, 3);
			if (tmpLod < 0) {
				useLOD = false;
			} else {
				lod = std::min(unit->lodCount - 1, (unsigned int)tmpLod);
			}
		}
		unit->currentLOD = lod;
	}

	glPushAttrib(GL_ENABLE_BIT);

	if (rawDraw) {
		if (useLOD) {
			unitDrawer->DrawUnitRawModel(unit);
		} else {
			const unsigned int origLodCount = unit->lodCount;
			unit->lodCount = 0;
			// transformation is not applied
			unitDrawer->DrawUnitRawModel(unit);
			unit->lodCount = origLodCount;
		}
	}
	else {
/*
		if (useLOD) {
			unitDrawer->DrawIndividual(unit);
		} else {
			const unsigned int origLodCount = unit->lodCount;
			unit->lodCount = 0;
			unitDrawer->DrawIndividual(unit);
			unit->lodCount = origLodCount;
		}
*/
	}

	glPopAttrib();

	return 0;
}


int LuaOpenGL::UnitShape(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.UnitShape(unitDefID, team)");
	}
	const int unitDefID = lua_toint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const int teamID = lua_toint(L, 2);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}

	unitDrawer->DrawUnitDef(ud, teamID);

	return 0;
}


int LuaOpenGL::UnitMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unitDrawer->ApplyUnitTransformMatrix(unit);
	return 0;
}


int LuaOpenGL::UnitPiece(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localmodel;

	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}
	LocalModelPiece* localPiece = localModel->pieces[piece];

	glCallList(localPiece->displist);

	return 0;
}


int LuaOpenGL::UnitPieceMatrix(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}

	CMatrix44f matrix = localModel->GetRawPieceMatrix(piece);
	glMultMatrixf(matrix.m);

	return 0;
}


int LuaOpenGL::UnitPieceMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const LocalModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}

	localModel->ApplyRawPieceTransform(piece);

	return 0;
}


/******************************************************************************/

static inline bool IsFeatureVisible(const CFeature* feature)
{
	if (CLuaHandle::GetActiveFullRead())
		return true;

	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	const int readAllyTeam = lh->GetReadAllyTeam();
	if (readAllyTeam < 0) {
		return (readAllyTeam == CEventClient::AllAccessTeam);
	}
	return feature->IsInLosForAllyTeam(readAllyTeam);
}


static CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, index)) {
		luaL_error(L, "Incorrect arguments to %s(featureID)", caller);
	}
	const int featureID = lua_toint(L, index);
	CFeature* feature = featureHandler->GetFeature(featureID);

	if (!feature)
		return NULL;

	if (!IsFeatureVisible(feature)) {
		return NULL;
	}
	return feature;
}


int LuaOpenGL::Feature(lua_State* L) // FIXME -- implement properly
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	if (feature->model == NULL) {
	  return 0;
	}
	glPushMatrix();
	glMultMatrixf(feature->transMatrix.m);
	feature->model->DrawStatic();
	glPopMatrix();

	return 0;
}


int LuaOpenGL::FeatureShape(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int fDefID = luaL_checkint(L, 1);
	//const int teamID = luaL_checkint(L, 2);

	//if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
	//	return 0;
	//}
	const FeatureDef* fd = featureHandler->GetFeatureDefByID(fDefID);
	if (fd == NULL) {
		return 0;
	}
	const S3DModel* model = LoadModel(fd);
	if (model == NULL) {
		return 0;
	}
	//todo: teamcolor?
	model->DrawStatic(); // FIXME: finish this, 3do/s3o, copy DrawIndividual...

	return 0;
}


/******************************************************************************/
/******************************************************************************/

static const bool& fullRead     = CLuaHandle::GetActiveFullRead();
static const int&  readAllyTeam = CLuaHandle::GetActiveReadAllyTeam();


static inline CUnit* ParseDrawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		} else {
			return NULL;
		}
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	if (readAllyTeam < 0) {
		if (!fullRead) {
			return NULL;
		}
	} else {
		if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0) {
			return NULL;
		}
	}
	if (!camera->InView(unit->midPos, unit->radius)) {
		return NULL;
	}
	const float sqDist = (unit->pos - camera->pos).SqLength();
	//const float farLength = unit->sqRadius * unitDrawDist * unitDrawDist;
	//if (sqDist >= farLength) {
	//	return NULL;
	//}
	const float iconDistSqrMult = unit->unitDef->iconType->GetDistanceSqr();
	const float realIconLength = unitDrawer->iconLength * iconDistSqrMult;
	if (sqDist >= realIconLength) {
		return NULL;
	}

	return unit;
}


int LuaOpenGL::DrawListAtUnit(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	// is visible to current read team
	// is not an icon
	//
	const CUnit* unit = ParseDrawUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	if (!lua_isnumber(L, 2)) {
		luaL_error(L, "Bad listID parameter in DrawListAtUnit()\n");
		return 0;
	}
	const unsigned int listIndex = (unsigned int)lua_tonumber(L, 2);
	CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
	const unsigned int dlist = displayLists.GetDList(listIndex);
	if (dlist == 0) {
		return 0;
	}

	bool midPos = true;
	if (lua_isboolean(L, 3)) {
		midPos = lua_toboolean(L, 3);
	}

	float3 pos = midPos ? (float3)unit->midPos : (float3)unit->pos;
	CTransportUnit *trans=unit->GetTransporter();
	if (trans == NULL) {
		pos += (unit->speed * globalRendering->timeOffset);
	} else {
		pos += (trans->speed * globalRendering->timeOffset);
	}

	const float3 scale(luaL_optnumber(L, 4, 1.0f),
	                   luaL_optnumber(L, 5, 1.0f),
	                   luaL_optnumber(L, 6, 1.0f));
	const float degrees = luaL_optnumber(L, 7, 0.0f);
	const float3 rot(luaL_optnumber(L,  8, 0.0f),
	                 luaL_optnumber(L,  9, 1.0f),
	                 luaL_optnumber(L, 10, 0.0f));

	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(degrees, rot.x, rot.y, rot.z);
	glScalef(scale.x, scale.y, scale.z);
	glCallList(dlist);
	glPopMatrix();

	return 0;
}


int LuaOpenGL::DrawFuncAtUnit(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	// is visible to current read team
	// is not an icon
	//
	const CUnit* unit = ParseDrawUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	bool midPos = true;
	if (lua_isboolean(L, 2)) {
		midPos = lua_toboolean(L, 2);
	}

	if (!lua_isfunction(L, 3)) {
		luaL_error(L, "Missing function parameter in DrawFuncAtUnit()\n");
		return 0;
	}

	float3 pos = midPos ? (float3)unit->midPos : (float3)unit->pos;
	CTransportUnit *trans=unit->GetTransporter();
	if (trans == NULL) {
		pos += (unit->speed * globalRendering->timeOffset);
	} else {
		pos += (trans->speed * globalRendering->timeOffset);
	}

	const int args = lua_gettop(L); // number of arguments

	// call the function
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	const int error = lua_pcall(L, (args - 3), 0, 0);
	glPopMatrix();

	if (error != 0) {
		logOutput.Print("gl.DrawFuncAtUnit: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::DrawGroundCircle(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const int args = lua_gettop(L); // number of arguments
	if ((args < 5) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
	    !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
		luaL_error(L, "Incorrect arguments to gl.DrawGroundCircle()");
	}
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));
	const float r  = lua_tofloat(L, 4);
	const int divs =   lua_toint(L, 5);

	if ((args >= 6) && lua_isnumber(L, 6)) {
		// const float slope = lua_tofloat(L, 6);
		glBallisticCircle(pos, r, NULL, divs);
	} else {
		glSurfaceCircle(pos, r, divs);
	}
	return 0;
}


int LuaOpenGL::DrawGroundQuad(lua_State* L)
{
	// FIXME: incomplete (esp. texcoord clamping)
	CheckDrawingEnabled(L, __FUNCTION__);
	const float x0 = luaL_checknumber(L, 1);
	const float z0 = luaL_checknumber(L, 2);
	const float x1 = luaL_checknumber(L, 3);
	const float z1 = luaL_checknumber(L, 4);

	const int args = lua_gettop(L); // number of arguments

	bool useTxcd = false;
	bool useNorm = false;

	if (lua_isboolean(L, 5)) {
		useNorm = lua_toboolean(L, 5);
	}

	float tu0, tv0, tu1, tv1;
	if (args == 9) {
		tu0 = luaL_checknumber(L, 6);
		tv0 = luaL_checknumber(L, 7);
		tu1 = luaL_checknumber(L, 8);
		tv1 = luaL_checknumber(L, 9);
		useTxcd = true;
	}
	else {
		if (lua_isboolean(L, 6)) {
			useTxcd = lua_toboolean(L, 6);
			if (useTxcd) {
				tu0 = 0.0f;
				tv0 = 0.0f;
				tu1 = 1.0f;
				tv1 = 1.0f;
			}
		}
	}
	const int mapxi = (gs->mapx + 1);
	const int mapzi = (gs->mapy + 1);
	const float* heightmap = readmap->GetHeightmap();

	const float xs = std::max(0.0f, std::min(float3::maxxpos, x0)); // x start
	const float xe = std::max(0.0f, std::min(float3::maxxpos, x1)); // x end
	const float zs = std::max(0.0f, std::min(float3::maxzpos, z0)); // z start
	const float ze = std::max(0.0f, std::min(float3::maxzpos, z1)); // z end
	const int xis = std::max(0, std::min(mapxi, int((xs + 0.5f) / SQUARE_SIZE)));
	const int xie = std::max(0, std::min(mapxi, int((xe + 0.5f) / SQUARE_SIZE)));
	const int zis = std::max(0, std::min(mapzi, int((zs + 0.5f) / SQUARE_SIZE)));
	const int zie = std::max(0, std::min(mapzi, int((ze + 0.5f) / SQUARE_SIZE)));
	if ((xis >= xie) || (zis >= zie)) {
		return 0;
	}

	if (!useTxcd) {
		for (int xib = xis; xib < xie; xib++) {
			const int xit = xib + 1;
			const float xb = xib * SQUARE_SIZE;
			const float xt = xb + SQUARE_SIZE;
			glBegin(GL_TRIANGLE_STRIP);
			for (int zi = zis; zi <= zie; zi++) {
				const int ziOff = zi * mapxi;
				const float yb = heightmap[ziOff + xib];
				const float yt = heightmap[ziOff + xit];
				const float z = zi * SQUARE_SIZE;
				glVertex3f(xt, yt, z);
				glVertex3f(xb, yb, z);
			}
			glEnd();
		}
	}
	else {
		const float tuStep = (tu1 - tu0) / float(xie - xis);
		const float tvStep = (tv1 - tv0) / float(zie - zis);

		float tub = tu0;
		for (int xib = xis; xib < xie; xib++) {
			const int xit = xib + 1;
			const float xb = xib * SQUARE_SIZE;
			const float xt = xb + SQUARE_SIZE;
			const float tut = tub + tuStep;
			float tv = tv0;
			glBegin(GL_TRIANGLE_STRIP);
			for (int zi = zis; zi <= zie; zi++) {
				const int ziOff = zi * mapxi;
				const float yb = heightmap[ziOff + xib];
				const float yt = heightmap[ziOff + xit];
				const float z = zi * SQUARE_SIZE;
				glTexCoord2f(tut, tv);
				glVertex3f(xt, yt, z);
				glTexCoord2f(tub, tv);
				glVertex3f(xb, yb, z);
				tv += tvStep;
			}
			glEnd();
			tub += tuStep;
		}
	}
	return 0;
}


/******************************************************************************/
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


static bool ParseVertexData(lua_State* L, VertexData& vd)
{
	vd.hasVert = vd.hasNorm = vd.hasTxcd = vd.hasColor = false;

	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1) || !lua_israwstring(L, -2)) {
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
		luaL_error(L, "Incorrect arguments to gl.BeginEnd(type, func, ...)");
	}
	const GLuint primMode = (GLuint)lua_tonumber(L, 1);

	// call the function
	glBegin(primMode);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEnd();

	if (error != 0) {
		logOutput.Print("gl.BeginEnd: error(%i) = %s",
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
		const float x = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float y = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			glVertex2f(x, y);
			return 0;
		}
		const float z = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 4);
		if (!lua_isnumber(L, -1)) {
			glVertex3f(x, y, z);
			return 0;
		}
		const float w = lua_tofloat(L, -1);
		glVertex4f(x, y, z, w);
		return 0;
	}

	if (args == 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = lua_tofloat(L, 1);
		const float y = lua_tofloat(L, 2);
		const float z = lua_tofloat(L, 3);
		glVertex3f(x, y, z);
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = lua_tofloat(L, 1);
		const float y = lua_tofloat(L, 2);
		glVertex2f(x, y);
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Bad data passed to gl.Vertex()");
		}
		const float x = lua_tofloat(L, 1);
		const float y = lua_tofloat(L, 2);
		const float z = lua_tofloat(L, 3);
		const float w = lua_tofloat(L, 4);
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
		const float x = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Normal()");
		}
		const float y = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.Normal()");
		}
		const float z = lua_tofloat(L, -1);
		glNormal3f(x, y, z);
		return 0;
	}

	if (args < 3) {
		luaL_error(L, "Incorrect arguments to gl.Normal()");
	}
	const float x = lua_tofloat(L, 1);
	const float y = lua_tofloat(L, 2);
	const float z = lua_tofloat(L, 3);
	glNormal3f(x, y, z);
	return 0;
}


int LuaOpenGL::TexCoord(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (lua_isnumber(L, 1)) {
			const float x = lua_tofloat(L, 1);
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
		const float x = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			glTexCoord1f(x);
			return 0;
		}
		const float y = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			glTexCoord2f(x, y);
			return 0;
		}
		const float z = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 4);
		if (!lua_isnumber(L, -1)) {
			glTexCoord3f(x, y, z);
			return 0;
		}
		const float w = lua_tofloat(L, -1);
		glTexCoord4f(x, y, z, w);
		return 0;
	}

	if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Bad data passed to gl.TexCoord()");
		}
		const float x = lua_tofloat(L, 1);
		const float y = lua_tofloat(L, 2);
		glTexCoord2f(x, y);
	}
	else if (args == 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			luaL_error(L, "Bad data passed to gl.TexCoord()");
		}
		const float x = lua_tofloat(L, 1);
		const float y = lua_tofloat(L, 2);
		const float z = lua_tofloat(L, 3);
		glTexCoord3f(x, y, z);
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Bad data passed to gl.TexCoord()");
		}
		const float x = lua_tofloat(L, 1);
		const float y = lua_tofloat(L, 2);
		const float z = lua_tofloat(L, 3);
		const float w = lua_tofloat(L, 4);
		glTexCoord4f(x, y, z, w);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.TexCoord()");
	}
	return 0;
}


int LuaOpenGL::MultiTexCoord(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int texNum = luaL_checkint(L, 1);
	if ((texNum < 0) || (texNum >= MAX_TEXTURE_UNITS)) {
		luaL_error(L, "Bad texture unit passed to gl.MultiTexCoord()");
	}
	const GLenum texUnit = GL_TEXTURE0 + texNum;

	const int args = lua_gettop(L) - 1; // number of arguments

	if (args == 1) {
		if (lua_isnumber(L, 2)) {
			const float x = lua_tofloat(L, 2);
			glMultiTexCoord1f(texUnit, x);
			return 0;
		}
		if (!lua_istable(L, 2)) {
			luaL_error(L, "Bad data passed to gl.MultiTexCoord()");
		}
		lua_rawgeti(L, 2, 1);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.MultiTexCoord()");
		}
		const float x = lua_tofloat(L, -1);
		lua_rawgeti(L, 2, 2);
		if (!lua_isnumber(L, -1)) {
			glMultiTexCoord1f(texUnit, x);
			return 0;
		}
		const float y = lua_tofloat(L, -1);
		lua_rawgeti(L, 2, 3);
		if (!lua_isnumber(L, -1)) {
			glMultiTexCoord2f(texUnit, x, y);
			return 0;
		}
		const float z = lua_tofloat(L, -1);
		lua_rawgeti(L, 2, 4);
		if (!lua_isnumber(L, -1)) {
			glMultiTexCoord3f(texUnit, x, y, z);
			return 0;
		}
		const float w = lua_tofloat(L, -1);
		glMultiTexCoord4f(texUnit, x, y, z, w);
		return 0;
	}

	if (args == 2) {
		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			luaL_error(L, "Bad data passed to gl.MultiTexCoord()");
		}
		const float x = lua_tofloat(L, 2);
		const float y = lua_tofloat(L, 3);
		glMultiTexCoord2f(texUnit, x, y);
	}
	else if (args == 3) {
		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Bad data passed to gl.MultiTexCoord()");
		}
		const float x = lua_tofloat(L, 2);
		const float y = lua_tofloat(L, 3);
		const float z = lua_tofloat(L, 4);
		glMultiTexCoord3f(texUnit, x, y, z);
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
		    !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
			luaL_error(L, "Bad data passed to gl.MultiTexCoord()");
		}
		const float x = lua_tofloat(L, 2);
		const float y = lua_tofloat(L, 3);
		const float z = lua_tofloat(L, 4);
		const float w = lua_tofloat(L, 5);
		glMultiTexCoord4f(texUnit, x, y, z, w);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.MultiTexCoord()");
	}
	return 0;
}


int LuaOpenGL::SecondaryColor(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (!lua_istable(L, 1)) {
			luaL_error(L, "Bad data passed to gl.SecondaryColor()");
		}
		lua_rawgeti(L, 1, 1);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.SecondaryColor()");
		}
		const float x = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 2);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.SecondaryColor()");
		}
		const float y = lua_tofloat(L, -1);
		lua_rawgeti(L, 1, 3);
		if (!lua_isnumber(L, -1)) {
			luaL_error(L, "Bad data passed to gl.SecondaryColor()");
		}
		const float z = lua_tofloat(L, -1);
		glSecondaryColor3f(x, y, z);
		return 0;
	}

	if (args < 3) {
		luaL_error(L, "Incorrect arguments to gl.SecondaryColor()");
	}
	const float x = lua_tofloat(L, 1);
	const float y = lua_tofloat(L, 2);
	const float z = lua_tofloat(L, 3);
	glSecondaryColor3f(x, y, z);
	return 0;
}


int LuaOpenGL::FogCoord(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const float value = luaL_checkfloat(L, 1);
	glFogCoordf(value);
	return 0;
}


int LuaOpenGL::EdgeFlag(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	if (lua_isboolean(L, 1)) {
		glEdgeFlag(lua_toboolean(L, 1));
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
	const float x1 = lua_tofloat(L, 1);
	const float y1 = lua_tofloat(L, 2);
	const float x2 = lua_tofloat(L, 3);
	const float y2 = lua_tofloat(L, 4);

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
	const float x1 = lua_tofloat(L, 1);
	const float y1 = lua_tofloat(L, 2);
	const float x2 = lua_tofloat(L, 3);
	const float y2 = lua_tofloat(L, 4);

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
	const float s1 = lua_tofloat(L, 5);
	const float t1 = lua_tofloat(L, 6);
	const float s2 = lua_tofloat(L, 7);
	const float t2 = lua_tofloat(L, 8);
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
		if (!lua_israwstring(L, -2)) { // the key
			logOutput.Print("gl.Material: bad state type\n");
			return 0;;
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
		glScissor(x + globalRendering->viewPosX, y + globalRendering->viewPosY, w, h);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Scissor()");
	}

	return 0;
}


int LuaOpenGL::Viewport(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int x = luaL_checkint(L, 1);
	const int y = luaL_checkint(L, 1);
	const int w = luaL_checkint(L, 1);
	const int h = luaL_checkint(L, 1);

	glViewport(x, y, w, h);

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


int LuaOpenGL::DepthClamp(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	luaL_checktype(L, 1, LUA_TBOOLEAN);
	if (lua_toboolean(L, 1)) {
		glEnable(GL_DEPTH_CLAMP_NV);
	} else {
		glDisable(GL_DEPTH_CLAMP_NV);
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


int LuaOpenGL::Fog(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.Fog()");
	}

	if (lua_toboolean(L, 1)) {
		glEnable(GL_FOG);
	} else {
		glDisable(GL_FOG);
	}
	return 0;
}


int LuaOpenGL::Blending(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (lua_isboolean(L, 1)) {
			if (lua_toboolean(L, 1)) {
				glEnable(GL_BLEND);
			} else {
				glDisable(GL_BLEND);
			}
		}
		else if (lua_israwstring(L, 1)) {
			const string mode = lua_tostring(L, 1);
			if (mode == "add") {
				glBlendFunc(GL_ONE, GL_ONE);
				glEnable(GL_BLEND);
			}
			else if (mode == "alpha_add") {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glEnable(GL_BLEND);
			}
			else if ((mode == "alpha") ||
			         (mode == "reset")) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}
			else if (mode == "color") {
				glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
				glEnable(GL_BLEND);
			}
			else if (mode == "modulate") {
				glBlendFunc(GL_DST_COLOR, GL_ZERO);
				glEnable(GL_BLEND);
			}
			else if (mode == "disable") {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_BLEND);
			}
		}
		else {
			luaL_error(L, "Incorrect argument to gl.Blending()");
		}
	}
	else if (args == 2) {
		const GLenum src = (GLenum)luaL_checkint(L, 1);
		const GLenum dst = (GLenum)luaL_checkint(L, 2);
		glBlendFunc(src, dst);
		glEnable(GL_BLEND);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Blending()");
	}
	return 0;
}


int LuaOpenGL::BlendEquation(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum mode = (GLenum)luaL_checkint(L, 1);
	glBlendEquation(mode);
	return 0;
}


int LuaOpenGL::BlendFunc(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum src = (GLenum)luaL_checkint(L, 1);
	const GLenum dst = (GLenum)luaL_checkint(L, 2);
	glBlendFunc(src, dst);
	return 0;
}


int LuaOpenGL::BlendEquationSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum modeRGB   = (GLenum)luaL_checkint(L, 1);
	const GLenum modeAlpha = (GLenum)luaL_checkint(L, 2);
	glBlendEquationSeparate(modeRGB, modeAlpha);
	return 0;
}


int LuaOpenGL::BlendFuncSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum srcRGB   = (GLenum)luaL_checkint(L, 1);
	const GLenum dstRGB   = (GLenum)luaL_checkint(L, 2);
	const GLenum srcAlpha = (GLenum)luaL_checkint(L, 3);
	const GLenum dstAlpha = (GLenum)luaL_checkint(L, 4);
	glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
	return 0;
}


int LuaOpenGL::Smoothing(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const static struct {
		const char* name;
		GLenum hintEnum;
		GLenum enableEnum;
	} smoothTypes[3] = {
		{ "point",   GL_POINT_SMOOTH_HINT,   GL_POINT_SMOOTH   },
		{ "line",    GL_LINE_SMOOTH_HINT,    GL_LINE_SMOOTH    },
		{ "polygon", GL_POLYGON_SMOOTH_HINT, GL_POLYGON_SMOOTH }
	};

	for (int i = 0; i < 3; i++) {
		const GLenum hintEnum   = smoothTypes[i].hintEnum;
		const GLenum enableEnum = smoothTypes[i].enableEnum;
		const int luaIndex = (i + 1);
		const int type = lua_type(L, luaIndex);
		if (type == LUA_TBOOLEAN) {
			if (lua_toboolean(L, luaIndex)) {
				glEnable(enableEnum);
			} else {
				glDisable(enableEnum);
			}
		}
		else if (type == LUA_TNUMBER) {
			const GLenum hint = (GLenum)lua_tonumber(L, luaIndex);
			if ((hint == GL_FASTEST) || (hint == GL_NICEST) || (hint == GL_DONT_CARE)) {
				glHint(hintEnum, hint);
				glEnable(enableEnum);
			} else {
				luaL_error(L, "Bad %s hint in gl.Smoothing()", smoothTypes[i].name);
			}
		}
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


/******************************************************************************/

int LuaOpenGL::StencilTest(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	luaL_checktype(L, 1, LUA_TBOOLEAN);
	if (lua_toboolean(L, 1)) {
		glEnable(GL_STENCIL_TEST);
	} else {
		glDisable(GL_STENCIL_TEST);
	}
	return 0;
}


int LuaOpenGL::StencilMask(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint mask = luaL_checkint(L, 1);
	glStencilMask(mask);
	return 0;
}


int LuaOpenGL::StencilFunc(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum func = luaL_checkint(L, 1);
	const GLint  ref  = luaL_checkint(L, 2);
	const GLuint mask = luaL_checkint(L, 3);
	glStencilFunc(func, ref, mask);
	return 0;
}


int LuaOpenGL::StencilOp(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum fail  = luaL_checkint(L, 1);
	const GLenum zfail = luaL_checkint(L, 2);
	const GLenum zpass = luaL_checkint(L, 3);
	glStencilOp(fail, zfail, zpass);
	return 0;
}


int LuaOpenGL::StencilMaskSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum face = luaL_checkint(L, 1);
	const GLuint mask = luaL_checkint(L, 2);
	glStencilMaskSeparate(face, mask);
	return 0;
}


int LuaOpenGL::StencilFuncSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum face = luaL_checkint(L, 1);
	const GLenum func = luaL_checkint(L, 2);
	const GLint  ref  = luaL_checkint(L, 3);
	const GLuint mask = luaL_checkint(L, 4);
	glStencilFuncSeparate(face, func, ref, mask);
	return 0;
}


int LuaOpenGL::StencilOpSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum face  = luaL_checkint(L, 1);
	const GLenum fail  = luaL_checkint(L, 2);
	const GLenum zfail = luaL_checkint(L, 3);
	const GLenum zpass = luaL_checkint(L, 4);
	glStencilOpSeparate(face, fail, zfail, zpass);
	return 0;
}


/******************************************************************************/

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
	else if (args >= 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to gl.LineStipple()");
		}
		GLint factor     =    (GLint)lua_tonumber(L, 1);
		GLushort pattern = (GLushort)lua_tonumber(L, 2);
		if ((args >= 3) && lua_isnumber(L, 3)) {
			int shift = lua_toint(L, 3);
			while (shift < 0) { shift += 16; }
			shift = (shift % 16);
			unsigned int pat = pattern & 0xFFFF;
			pat = pat | (pat << 16);
			pattern = pat >> shift;
		}
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
	glLineWidth(lua_tofloat(L, 1));
	return 0;
}


int LuaOpenGL::PointSize(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.PointSize()");
	}
	glPointSize(lua_tofloat(L, 1));
	return 0;
}


int LuaOpenGL::PointSprite(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.PointSprite()");
	}
	if (lua_toboolean(L, 1)) {
		glEnable(GL_POINT_SPRITE);
	} else {
		glDisable(GL_POINT_SPRITE);
	}
	if ((args >= 2) && lua_isboolean(L, 2)) {
		if (lua_toboolean(L, 2)) {
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		} else {
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
		}
	}
	if ((args >= 3) && lua_isboolean(L, 3)) {
		if (lua_toboolean(L, 3)) {
			glTexEnvi(GL_POINT_SPRITE, GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT);
		} else {
			glTexEnvi(GL_POINT_SPRITE, GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
		}
	}
	return 0;
}


int LuaOpenGL::PointParameter(lua_State* L)
{
	GLfloat atten[3];
	atten[0] = (GLfloat)luaL_checknumber(L, 1);
	atten[1] = (GLfloat)luaL_checknumber(L, 2);
	atten[2] = (GLfloat)luaL_checknumber(L, 3);
	glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, atten);

	const int args = lua_gettop(L);
	if (args >= 4) {
		const float sizeMin = luaL_checkfloat(L, 4);
		glPointParameterf(GL_POINT_SIZE_MIN, sizeMin);
	}
	if (args >= 5) {
		const float sizeMax = luaL_checkfloat(L, 5);
		glPointParameterf(GL_POINT_SIZE_MAX, sizeMax);
	}
	if (args >= 6) {
		const float sizeFade = luaL_checkfloat(L, 6);
		glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, sizeFade);
	}

	return 0;
}


static bool ParseUnitTexture(const string& texture)
{
	if (texture.length()<4) {
		return false;
	}

	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int id = (int)strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (*endPtr != ':')) {
		return false;
	}

	endPtr++; // skip the ':'
	if ( (startPtr-1)+texture.length() <= endPtr ) {
		return false; // ':' is end of string, but we expect '%num:0'
	}


	if (id == 0) {
		glEnable(GL_TEXTURE_2D);
		if (*endPtr == '0') {
			glBindTexture(GL_TEXTURE_2D, texturehandler3DO->GetAtlasTex1ID() );
		}
		else if (*endPtr == '1') {
			glBindTexture(GL_TEXTURE_2D, texturehandler3DO->GetAtlasTex2ID() );
		}
		return true;
	}

	S3DModel* model;

	if (id < 0) {
		const FeatureDef* fd = featureHandler->GetFeatureDefByID(-id);
		if (fd == NULL) {
			return false;
		}
		model = LoadModel(fd);
	} else {
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(id);
		if (ud == NULL) {
			return false;
		}
		model = ud->LoadModel();
	}

	if (model == NULL) {
		return false;
	}

	const unsigned int texType = model->textureType;
	if (texType == 0) {
		return false;
	}

	const CS3OTextureHandler::S3oTex* stex = texturehandlerS3O->GetS3oTex(texType);
	if (stex == NULL) {
		return false;
	}

	if (*endPtr == '0') {
		glBindTexture(GL_TEXTURE_2D, stex->tex1);
		glEnable(GL_TEXTURE_2D);
		return true;
	}
	else if (*endPtr == '1') {
		glBindTexture(GL_TEXTURE_2D, stex->tex2);
		glEnable(GL_TEXTURE_2D);
		return true;
	}

	return false;
}


int LuaOpenGL::Texture(lua_State* L)
{
	// NOTE: current formats:
	//
	// #12          --  unitDef 12 buildpic
	// %34:0        --  unitDef 34 s3o tex1
	// %-34:1       --  featureDef 34 s3o tex2
	// !56          --  lua generated texture 56
	// $shadow      --  shadowmap
	// $specular    --  specular cube map
	// $reflection  --  reflection cube map
	// $heightmap   --  ground heightmap
	// ...          --  named textures
	//

	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args < 1) {
		luaL_error(L, "Incorrect arguments to gl.Texture()");
	}

	GLenum texUnit = GL_TEXTURE0;
	int nextArg = 1;

	if (lua_isnumber(L, 1)) {
		nextArg = 2;
		const int texNum = (GLenum)luaL_checknumber(L, 1);
		if ((texNum < 0) || (texNum >= MAX_TEXTURE_UNITS)) {
			luaL_error(L, "Bad texture unit given to gl.Texture()");
		}
		texUnit += texNum;
		if (texUnit != GL_TEXTURE0) {
			glActiveTexture(texUnit);
		}
	}

	if (lua_isboolean(L, nextArg)) {
		if (lua_toboolean(L, nextArg)) {
			glEnable(GL_TEXTURE_2D);
		} else {
			glDisable(GL_TEXTURE_2D);
		}
		if (texUnit != GL_TEXTURE0) {
			glActiveTexture(GL_TEXTURE0);
		}
		lua_pushboolean(L, true);
		return 1;
	}

	if (!lua_isstring(L, nextArg)) {
		if (texUnit != GL_TEXTURE0) {
			glActiveTexture(GL_TEXTURE0);
		}
		luaL_error(L, "Incorrect arguments to gl.Texture()");
	}

	const string texture = lua_tostring(L, nextArg);

	if (texture.empty()) {
		glDisable(GL_TEXTURE_2D); // equivalent to 'false'
		lua_pushboolean(L, true);
	}
	else if (texture[0] == '#') {
		// unit build picture
		char* endPtr;
		const char* startPtr = texture.c_str() + 1; // skip the '#'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			lua_pushboolean(L, false);
		} else {
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
			if (ud != NULL) {
				glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitDefImage(ud));
				glEnable(GL_TEXTURE_2D);
				lua_pushboolean(L, true);
			} else {
				lua_pushboolean(L, false);
			}
		}
	}
	else if (texture[0] == '^') {
		// unit icon
		char* endPtr;
		const char* startPtr = texture.c_str() + 1; // skip the '^'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			lua_pushboolean(L, false);
		} else {
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
			if (ud != NULL) {
				ud->iconType->BindTexture();
				glEnable(GL_TEXTURE_2D);
				lua_pushboolean(L, true);
			} else {
				lua_pushboolean(L, false);
			}
		}
	}
	else if (texture[0] == LuaTextures::prefix) { // '!'
		// dynamic texture
		LuaTextures& textures = CLuaHandle::GetActiveTextures();
		if (textures.Bind(texture)) {
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		} else {
			lua_pushboolean(L, false);
		}
	}
	else if (texture[0] == '$') {
		// never enables
		if (texture == "$units1" || texture == "$units") {
			glBindTexture(GL_TEXTURE_2D,  texturehandler3DO->GetAtlasTex1ID() );
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		if (texture == "$units2") {
			glBindTexture(GL_TEXTURE_2D, texturehandler3DO->GetAtlasTex2ID() );
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		else if (texture == "$shadow") {
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		else if (texture == "$specular") {
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSpecularTextureID());
			lua_pushboolean(L, true);
		}
		else if (texture == "$reflection") {
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetEnvReflectionTextureID());
			lua_pushboolean(L, true);
		}
		else if (texture == "$heightmap") {
			const GLuint texID = heightMapTexture.CheckTextureID();
			if (texID == 0) {
				lua_pushboolean(L, false);
			} else {
				glBindTexture(GL_TEXTURE_2D, texID);
				glEnable(GL_TEXTURE_2D);
				lua_pushboolean(L, true);
			}
		}
		else if (texture == "$shading") {
			glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		else if (texture == "$grass") {
			glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		else if (texture == "$font") {
			glBindTexture(GL_TEXTURE_2D, font->GetTexture());
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		else if (texture == "$smallfont") {
			glBindTexture(GL_TEXTURE_2D, smallFont->GetTexture());
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		}
		else {
			lua_pushboolean(L, false);
		}
	}
	else if (texture[0] == '%') {
		lua_pushboolean(L, ParseUnitTexture(texture));
	}
	else {
		if (CNamedTextures::Bind(texture)) {
			glEnable(GL_TEXTURE_2D);
			lua_pushboolean(L, true);
		} else {
			lua_pushboolean(L, false);
		}
	}

	if (texUnit != GL_TEXTURE0) {
		glActiveTexture(GL_TEXTURE0);
	}
	return 1;
}


int LuaOpenGL::CreateTexture(lua_State* L)
{
	LuaTextures::Texture tex;
	tex.xsize = (GLsizei)luaL_checknumber(L, 1);
	tex.ysize = (GLsizei)luaL_checknumber(L, 2);

	if (lua_istable(L, 3)) {
		const int table = 3;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string key = lua_tostring(L, -2);
				if (lua_israwnumber(L, -1)) {
					if (key == "target") {
						tex.target = (GLenum)lua_tonumber(L, -1);
					} else if (key == "format") {
						tex.format = (GLint)lua_tonumber(L, -1);
					} else if (key == "min_filter") {
						tex.min_filter = (GLenum)lua_tonumber(L, -1);
					} else if (key == "mag_filter") {
						tex.mag_filter = (GLenum)lua_tonumber(L, -1);
					} else if (key == "wrap_s") {
						tex.wrap_s = (GLenum)lua_tonumber(L, -1);
					} else if (key == "wrap_t") {
						tex.wrap_t = (GLenum)lua_tonumber(L, -1);
					} else if (key == "wrap_r") {
						tex.wrap_r = (GLenum)lua_tonumber(L, -1);
					} else if (key == "aniso") {
						tex.aniso = (GLfloat)lua_tonumber(L, -1);
					}
				}
				else if (lua_isboolean(L, -1)) {
					if (key == "border") {
						tex.border   = lua_toboolean(L, -1) ? 1 : 0;
					} else if (key == "fbo") {
						tex.fbo      = lua_toboolean(L, -1) ? 1 : 0;
					} else if (key == "fboDepth") {
						tex.fboDepth = lua_toboolean(L, -1) ? 1 : 0;
					}
				}
			}
		}
	}

	LuaTextures& textures = CLuaHandle::GetActiveTextures();
	const string name = textures.Create(tex);
	if (name.empty()) {
		return 0;
	}

	lua_pushstring(L, name.c_str());
	return 1;
}


int LuaOpenGL::DeleteTexture(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.DeleteTexture()");
	}
	const string texture = lua_tostring(L, 1);
	if (texture[0] == LuaTextures::prefix) { // '!'
		LuaTextures& textures = CLuaHandle::GetActiveTextures();
		lua_pushboolean(L, textures.Free(texture));
	} else {
		lua_pushboolean(L, CNamedTextures::Free(texture));
	}
	return 1;
}


// FIXME: obsolete
int LuaOpenGL::DeleteTextureFBO(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	const string texture = luaL_checkstring(L, 1);
	LuaTextures& textures = CLuaHandle::GetActiveTextures();
	lua_pushboolean(L, textures.FreeFBO(texture));
	return 1;
}



static bool PushUnitTextureInfo(lua_State* L, const string& texture)
{
	if (texture[1] == 0) {
		lua_newtable(L);
		HSTR_PUSH_NUMBER(L, "xsize", texturehandler3DO->GetAtlasTexSizeX());
		HSTR_PUSH_NUMBER(L, "ysize", texturehandler3DO->GetAtlasTexSizeY());
		return 1;
	}

	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (*endPtr != ':')) {
		return 0;
	}
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const S3DModel* model = ud->LoadModel();
	const unsigned int texType = model->textureType;
	if (texType == 0) {
		return 0;
	}

	const CS3OTextureHandler::S3oTex* stex = texturehandlerS3O->GetS3oTex(texType);
	if (stex == NULL) {
		return 0;
	}

	endPtr++; // skip the ':'
	if (*endPtr == '0') {
		lua_newtable(L);
		HSTR_PUSH_NUMBER(L, "xsize", stex->tex1SizeX);
		HSTR_PUSH_NUMBER(L, "ysize", stex->tex1SizeY);
		return 1;
	}
	else if (*endPtr == '1') {
		lua_newtable(L);
		HSTR_PUSH_NUMBER(L, "xsize", stex->tex2SizeX);
		HSTR_PUSH_NUMBER(L, "ysize", stex->tex2SizeY);
		return 1;
	}

	return 0;
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
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			return 0;
		}
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
		if (ud == NULL) {
			return 0;
		}
		lua_newtable(L);
		unitDefHandler->GetUnitDefImage(ud); // forced existance
		HSTR_PUSH_NUMBER(L, "xsize", ud->buildPic->imageSizeX);
		HSTR_PUSH_NUMBER(L, "ysize", ud->buildPic->imageSizeY);
	}
	else if (texture[0] == '^') {
		char* endPtr;
		const char* startPtr = texture.c_str() + 1; // skip the '^'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			return 0;
		}
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
		if (ud == NULL) {
			return 0;
		}
		lua_newtable(L);
		HSTR_PUSH_NUMBER(L, "xsize", ud->iconType->GetSizeX());
		HSTR_PUSH_NUMBER(L, "ysize", ud->iconType->GetSizeY());
	}
	else if (texture[0] == '%') {
		return PushUnitTextureInfo(L, texture);
	}
	else if (texture[0] == LuaTextures::prefix) { // '!'
		LuaTextures& textures = CLuaHandle::GetActiveTextures();
		const LuaTextures::Texture* tex = textures.GetInfo(texture);
		if (tex == NULL) {
			return 0;
		}
		lua_newtable(L);
		HSTR_PUSH_NUMBER(L, "xsize", tex->xsize);
		HSTR_PUSH_NUMBER(L, "ysize", tex->ysize);
	}
	else if (texture[0] == '$') {
		if (texture == "$units") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", texturehandler3DO->GetAtlasTexSizeX());
			HSTR_PUSH_NUMBER(L, "ysize", texturehandler3DO->GetAtlasTexSizeY());
		}
		else if (texture == "$shadow") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", shadowHandler->shadowMapSize);
			HSTR_PUSH_NUMBER(L, "ysize", shadowHandler->shadowMapSize);
		}
		else if (texture == "$reflection") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", cubeMapHandler->GetReflectionTextureSize());
			HSTR_PUSH_NUMBER(L, "ysize", cubeMapHandler->GetReflectionTextureSize());
		}
		else if (texture == "$specular") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", cubeMapHandler->GetSpecularTextureSize());
			HSTR_PUSH_NUMBER(L, "ysize", cubeMapHandler->GetSpecularTextureSize());
		}
		else if (texture == "$heightmap") {
			if (!heightMapTexture.CheckTextureID()) {
				return 0;
			} else {
				lua_newtable(L);
				HSTR_PUSH_NUMBER(L, "xsize", heightMapTexture.GetSizeX());
				HSTR_PUSH_NUMBER(L, "ysize", heightMapTexture.GetSizeY());
			}
		}
		else if (texture == "$shading") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", gs->pwr2mapx);
			HSTR_PUSH_NUMBER(L, "ysize", gs->pwr2mapy);
		}
		else if (texture == "$grass") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", 1024);
			HSTR_PUSH_NUMBER(L, "ysize", 1024);
		}
		else if (texture == "$font") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", font->GetTexWidth());
			HSTR_PUSH_NUMBER(L, "ysize", font->GetTexHeight());
		}
		else if (texture == "$smallfont") {
			lua_newtable(L);
			HSTR_PUSH_NUMBER(L, "xsize", smallFont->GetTexWidth());
			HSTR_PUSH_NUMBER(L, "ysize", smallFont->GetTexHeight());
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


int LuaOpenGL::CopyToTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const string texture = luaL_checkstring(L, 1);
	if (texture[0] != LuaTextures::prefix) { // '!'
		luaL_error(L, "gl.CopyToTexture() can only write to lua textures");
	}
	LuaTextures& textures = CLuaHandle::GetActiveTextures();
	const LuaTextures::Texture* tex = textures.GetInfo(texture);
	if (tex == NULL) {
		return 0;
	}
	glBindTexture(tex->target, tex->id);
	glEnable(tex->target); // leave it bound and enabled

	const GLint xoff =   (GLint)luaL_checknumber(L, 2);
	const GLint yoff =   (GLint)luaL_checknumber(L, 3);
	const GLint    x =   (GLint)luaL_checknumber(L, 4);
	const GLint    y =   (GLint)luaL_checknumber(L, 5);
	const GLsizei  w = (GLsizei)luaL_checknumber(L, 6);
	const GLsizei  h = (GLsizei)luaL_checknumber(L, 7);
	const GLenum target = (GLenum)luaL_optnumber(L, 8, tex->target);
	const GLenum level  = (GLenum)luaL_optnumber(L, 9, 0);

	glCopyTexSubImage2D(target, level, xoff, yoff, x, y, w, h);

	if (tex->target != GL_TEXTURE_2D) {glDisable(tex->target);}

	return 0;
}


// FIXME: obsolete
int LuaOpenGL::RenderToTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const string texture = luaL_checkstring(L, 1);
	if (texture[0] != LuaTextures::prefix) { // '!'
		luaL_error(L, "gl.RenderToTexture() can only write to fbo textures");
	}
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.RenderToTexture()");
	}
	LuaTextures& textures = CLuaHandle::GetActiveTextures();
	const LuaTextures::Texture* tex = textures.GetInfo(texture);
	if ((tex == NULL) || (tex->fbo == 0)) {
		return 0;
	}

	GLint currentFBO = 0;
	if (drawMode == DRAW_WORLD_SHADOW) {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tex->fbo);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, tex->xsize, tex->ysize);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

	const int error = lua_pcall(L, lua_gettop(L) - 2, 0, 0);

	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);  glPopMatrix();
	glPopAttrib();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);

	if (error != 0) {
		logOutput.Print("gl.RenderToTexture: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::GenerateMipmap(lua_State* L)
{
	//CheckDrawingEnabled(L, __FUNCTION__);
	const string& texStr = luaL_checkstring(L, 1);
	if (texStr[0] != LuaTextures::prefix) { // '!'
		return 0;
	}
	LuaTextures& textures = CLuaHandle::GetActiveTextures();
	const LuaTextures::Texture* tex = textures.GetInfo(texStr);
	if (tex == NULL) {
		return 0;
	}
	GLint currentBinding;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentBinding);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, currentBinding);
	return 0;
}


int LuaOpenGL::ActiveTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.ActiveTexture(number, func, ...)");
	}
	const int texNum = lua_toint(L, 1);
	if ((texNum < 0) || (texNum >= MAX_TEXTURE_UNITS)) {
		luaL_error(L, "Bad texture unit passed to gl.ActiveTexture()");
		return 0;
	}

	// call the function
	glActiveTexture(GL_TEXTURE0 + texNum);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glActiveTexture(GL_TEXTURE0);

	if (error != 0) {
		logOutput.Print("gl.ActiveTexture: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}
	return 0;
}


int LuaOpenGL::TexEnv(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const GLenum target = (GLenum)luaL_checknumber(L, 1);
	const GLenum pname  = (GLenum)luaL_checknumber(L, 2);

	const int args = lua_gettop(L); // number of arguments
	if (args == 3) {
		const GLfloat value = (GLfloat)luaL_checknumber(L, 3);
		glTexEnvf(target, pname, value);
	}
	else if (args == 6) {
		GLfloat array[4];
		array[0] = luaL_optnumber(L, 3, 0.0f);
		array[1] = luaL_optnumber(L, 4, 0.0f);
		array[2] = luaL_optnumber(L, 5, 0.0f);
		array[3] = luaL_optnumber(L, 6, 0.0f);
		glTexEnvfv(target, pname, array);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.TexEnv()");
	}

	return 0;
}


int LuaOpenGL::MultiTexEnv(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const int texNum    =    luaL_checkint(L, 1);
	const GLenum target = (GLenum)luaL_checknumber(L, 2);
	const GLenum pname  = (GLenum)luaL_checknumber(L, 3);

	if ((texNum < 0) || (texNum >= MAX_TEXTURE_UNITS)) {
		luaL_error(L, "Bad texture unit passed to gl.MultiTexEnv()");
	}

	const int args = lua_gettop(L); // number of arguments
	if (args == 4) {
		const GLfloat value = (GLfloat)luaL_checknumber(L, 4);
		glActiveTexture(GL_TEXTURE0 + texNum);
		glTexEnvf(target, pname, value);
		glActiveTexture(GL_TEXTURE0);
	}
	else if (args == 7) {
		GLfloat array[4];
		array[0] = luaL_optnumber(L, 4, 0.0f);
		array[1] = luaL_optnumber(L, 5, 0.0f);
		array[2] = luaL_optnumber(L, 6, 0.0f);
		array[3] = luaL_optnumber(L, 7, 0.0f);
		glActiveTexture(GL_TEXTURE0 + texNum);
		glTexEnvfv(target, pname, array);
		glActiveTexture(GL_TEXTURE0);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.MultiTexEnv()");
	}

	return 0;
}


static void SetTexGenState(GLenum target, bool state)
{
	if ((target >= GL_S) && (target <= GL_Q)) {
		const GLenum pname = GL_TEXTURE_GEN_S + (target - GL_S);
		if (state) {
			glEnable(pname);
		} else {
			glDisable(pname);
		}
	}
}


int LuaOpenGL::TexGen(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const GLenum target = (GLenum)luaL_checknumber(L, 1);

	const int args = lua_gettop(L); // number of arguments
	if ((args == 2) && lua_isboolean(L, 2)) {
		const bool state = lua_toboolean(L, 2);
		SetTexGenState(target, state);
		return 0;
	}

	const GLenum pname  = (GLenum)luaL_checknumber(L, 2);

	if (args == 3) {
		const GLfloat value = (GLfloat)luaL_checknumber(L, 3);
		glTexGenf(target, pname, value);
		SetTexGenState(target, true);
	}
	else if (args == 6) {
		GLfloat array[4];
		array[0] = luaL_optnumber(L, 3, 0.0f);
		array[1] = luaL_optnumber(L, 4, 0.0f);
		array[2] = luaL_optnumber(L, 5, 0.0f);
		array[3] = luaL_optnumber(L, 6, 0.0f);
		glTexGenfv(target, pname, array);
		SetTexGenState(target, true);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.TexGen()");
	}

	return 0;
}


int LuaOpenGL::MultiTexGen(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int texNum = luaL_checkint(L, 1);
	if ((texNum < 0) || (texNum >= MAX_TEXTURE_UNITS)) {
		luaL_error(L, "Bad texture unit passed to gl.MultiTexGen()");
	}

	const GLenum target = (GLenum)luaL_checknumber(L, 2);

	const int args = lua_gettop(L); // number of arguments
	if ((args == 3) && lua_isboolean(L, 3)) {
		const bool state = lua_toboolean(L, 3);
		glActiveTexture(GL_TEXTURE0 + texNum);
		SetTexGenState(target, state);
		glActiveTexture(GL_TEXTURE0);
		return 0;
	}

	const GLenum pname  = (GLenum)luaL_checknumber(L, 3);

	if (args == 4) {
		const GLfloat value = (GLfloat)luaL_checknumber(L, 4);
		glActiveTexture(GL_TEXTURE0 + texNum);
		glTexGenf(target, pname, value);
		SetTexGenState(target, true);
		glActiveTexture(GL_TEXTURE0);
	}
	else if (args == 7) {
		GLfloat array[4];
		array[0] = luaL_optnumber(L, 4, 0.0f);
		array[1] = luaL_optnumber(L, 5, 0.0f);
		array[2] = luaL_optnumber(L, 6, 0.0f);
		array[3] = luaL_optnumber(L, 7, 0.0f);
		glActiveTexture(GL_TEXTURE0 + texNum);
		glTexGenfv(target, pname, array);
		SetTexGenState(target, true);
		glActiveTexture(GL_TEXTURE0);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.MultiTexGen()");
	}

	return 0;
}


/******************************************************************************/

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
	const float x = lua_tofloat(L, 1);
	const float y = lua_tofloat(L, 2);
	const float z = lua_tofloat(L, 3);
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
	const float x = lua_tofloat(L, 1);
	const float y = lua_tofloat(L, 2);
	const float z = lua_tofloat(L, 3);
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
	const float r = lua_tofloat(L, 1);
	const float x = lua_tofloat(L, 2);
	const float y = lua_tofloat(L, 3);
	const float z = lua_tofloat(L, 4);
	glRotatef(r, x, y, z);
	return 0;
}


int LuaOpenGL::Ortho(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const float left   = luaL_checknumber(L, 1);
	const float right  = luaL_checknumber(L, 2);
	const float bottom = luaL_checknumber(L, 3);
	const float top    = luaL_checknumber(L, 4);
	const float near   = luaL_checknumber(L, 5);
	const float far    = luaL_checknumber(L, 6);
	glOrtho(left, right, bottom, top, near, far);
	return 0;
}


int LuaOpenGL::Frustum(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const float left   = luaL_checknumber(L, 1);
	const float right  = luaL_checknumber(L, 2);
	const float bottom = luaL_checknumber(L, 3);
	const float top    = luaL_checknumber(L, 4);
	const float near   = luaL_checknumber(L, 5);
	const float far    = luaL_checknumber(L, 6);
	glFrustum(left, right, bottom, top, near, far);
	return 0;
}


int LuaOpenGL::Billboard(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
//	glMultMatrixd(camera->billboard);
	glCallList(CCamera::billboardList);
	return 0;
}


/******************************************************************************/

int LuaOpenGL::Light(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const GLenum light = GL_LIGHT0 + (GLint)luaL_checknumber(L, 1);
	if ((light < GL_LIGHT0) || (light > GL_LIGHT7)) {
		luaL_error(L, "Bad light number in gl.Light");
	}

	if (lua_isboolean(L, 2)) {
		if (lua_toboolean(L, 2)) {
			glEnable(light);
		} else {
			glDisable(light);
		}
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args == 3) {
		const GLenum pname = (GLenum)luaL_checknumber(L, 2);
		const GLenum param = (GLenum)luaL_checknumber(L, 3);
		glLightf(light, pname, param);
	}
	else if (args == 5) {
		GLfloat array[4]; // NOTE: 4 instead of 3  (to be safe)
		const GLenum pname = (GLenum)luaL_checknumber(L, 2);
		array[0] = (GLfloat)luaL_checknumber(L, 3);
		array[1] = (GLfloat)luaL_checknumber(L, 4);
		array[2] = (GLfloat)luaL_checknumber(L, 5);
		glLightfv(light, pname, array);
	}
	else if (args == 6) {
		GLfloat array[4];
		const GLenum pname = (GLenum)luaL_checknumber(L, 2);
		array[0] = (GLfloat)luaL_checknumber(L, 3);
		array[1] = (GLfloat)luaL_checknumber(L, 4);
		array[2] = (GLfloat)luaL_checknumber(L, 5);
		array[3] = (GLfloat)luaL_checknumber(L, 6);
		glLightfv(light, pname, array);
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Light");
	}

	return 0;
}


int LuaOpenGL::ClipPlane(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.ClipPlane");
	}
	const int plane = lua_toint(L, 1);
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


static const double* GetNamedMatrix(const string& name)
{
	if (name == "shadow") {
		static double mat[16];
		for (int i =0; i <16; i++) {
			mat[i] = shadowHandler->shadowMatrix.m[i];
		}
		return mat;
	}
	else if (name == "camera") {
		return camera->GetViewMat();
	}
	else if (name == "caminv") {
		return camera->GetViewMatInv();
	}
	else if (name == "camprj") {
		return camera->GetProjMat();
	}
	else if (name == "billboard") {
		return camera->GetBBoardMat();
	}
	return NULL;
}


int LuaOpenGL::LoadMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	GLfloat matrix[16];

	const int luaType = lua_type(L, 1);
	if (luaType == LUA_TSTRING) {
		const double* matptr = GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			glLoadMatrixd(matptr);
		} else {
			luaL_error(L, "Incorrect arguments to gl.LoadMatrix()");
		}
		return 0;
	}
	else if (luaType == LUA_TTABLE) {
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

	const int luaType = lua_type(L, 1);
	if (luaType == LUA_TSTRING) {
		const double* matptr = GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			glMultMatrixd(matptr);
		} else {
			luaL_error(L, "Incorrect arguments to gl.MultMatrix()");
		}
		return 0;
	}
	else if (luaType == LUA_TTABLE) {
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


int LuaOpenGL::PushPopMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	vector<GLenum> matModes;
	int arg;
	for (arg = 1; lua_isnumber(L, arg); arg++) {
		const GLenum mode = (GLenum)lua_tonumber(L, arg);
		matModes.push_back(mode);
	}

	if (!lua_isfunction(L, arg)) {
		luaL_error(L, "Incorrect arguments to gl.PushPopMatrix()");
	}

	if (arg == 1) {
		glPushMatrix();
	} else {
		for (int i = 0; i < (int)matModes.size(); i++) {
			glMatrixMode(matModes[i]);
			glPushMatrix();
		}
	}

	const int args = lua_gettop(L); // number of arguments
	const int error = lua_pcall(L, (args - arg), 0, 0);

	if (arg == 1) {
		glPopMatrix();
	} else {
		for (int i = 0; i < (int)matModes.size(); i++) {
			glMatrixMode(matModes[i]);
			glPopMatrix();
		}
	}

	if (error != 0) {
		logOutput.Print("gl.PushPopMatrix: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::GetMatrixData(lua_State* L)
{
	const int luaType = lua_type(L, 1);

	if (luaType == LUA_TNUMBER) {
		const GLenum type = (GLenum)lua_tonumber(L, 1);
		GLenum pname = 0;
		switch (type) {
			case GL_PROJECTION: { pname = GL_PROJECTION_MATRIX; break; }
			case GL_MODELVIEW:  { pname = GL_MODELVIEW_MATRIX;  break; }
			case GL_TEXTURE:    { pname = GL_TEXTURE_MATRIX;    break; }
			default: {
				luaL_error(L, "Incorrect arguments to gl.GetMatrixData(id)");
			}
		}
		GLfloat matrix[16];
		glGetFloatv(pname, matrix);

		if (lua_isnumber(L, 2)) {
			const int index = lua_toint(L, 2);
			if ((index < 0) || (index >= 16)) {
				return 0;
			}
			lua_pushnumber(L, matrix[index]);
			return 1;
		}

		for (int i = 0; i < 16; i++) {
			lua_pushnumber(L, matrix[i]);
		}

		return 16;
	}
	else if (luaType == LUA_TSTRING) {
		const double* matptr = GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			for (int i = 0; i < 16; i++) {
				lua_pushnumber(L, matptr[i]);
			}
		}
		else {
			luaL_error(L, "Incorrect arguments to gl.GetMatrixData(name)");
		}
		return 16;
	}

	return 0;
}

/******************************************************************************/

int LuaOpenGL::PushAttrib(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glPushAttrib((GLbitfield)luaL_optnumber(L, 1, GL_ALL_ATTRIB_BITS));
	return 0;
}


int LuaOpenGL::PopAttrib(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glPopAttrib();
	return 0;
}


int LuaOpenGL::UnsafeState(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum state = (GLenum)luaL_checkint(L, 1);
	int funcLoc = 2;
	bool reverse = false;
	if (lua_isboolean(L, 2)) {
		funcLoc++;
		reverse = lua_toboolean(L, 2);
	}
	if (!lua_isfunction(L, funcLoc)) {
		luaL_error(L, "expecting a function");
	}

	reverse ? glDisable(state) : glEnable(state);
	const int error = lua_pcall(L, lua_gettop(L) - funcLoc, 0, 0);
	reverse ? glEnable(state) : glDisable(state);

	if (error != 0) {
		logOutput.Print("gl.UnsafeState: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_pushnumber(L, 0);
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::CreateList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isfunction(L, 1)) {
		luaL_error(L,
			"Incorrect arguments to gl.CreateList(func [, arg1, arg2, etc ...])");
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
		logOutput.Print("gl.CreateList: error(%i) = %s",
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


int LuaOpenGL::CallList(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.CallList(list)");
	}
	const unsigned int listIndex = (unsigned int)lua_tonumber(L, 1);
	CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
	const unsigned int dlist = displayLists.GetDList(listIndex);
	if (dlist != 0) {
		glCallList(dlist);
	}
	return 0;
}


int LuaOpenGL::DeleteList(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.DeleteList(list)");
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

int LuaOpenGL::Flush(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glFlush();
	return 0;
}


int LuaOpenGL::Finish(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glFinish();
	return 0;
}


/******************************************************************************/

static int PixelFormatSize(GLenum f)
{
	switch (f) {
		case GL_COLOR_INDEX:
		case GL_STENCIL_INDEX:
		case GL_DEPTH_COMPONENT:
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_LUMINANCE: {
			return 1;
		}
		case GL_LUMINANCE_ALPHA: {
			return 2;
		}
		case GL_RGB:
		case GL_BGR: {
			return 3;
		}
		case GL_RGBA:
		case GL_BGRA: {
			return 4;
		}
	}
	return -1;
}


static void PushPixelData(lua_State* L, int fSize, const float*& data)
{
	if (fSize == 1) {
		lua_pushnumber(L, *data);
		data++;
	} else {
		lua_newtable(L);
		for (int e = 1; e <= fSize; e++) {
			lua_pushnumber(L, e);
			lua_pushnumber(L, *data);
			lua_rawset(L, -3);
			data++;
		}
	}
}


int LuaOpenGL::ReadPixels(lua_State* L)
{
	const GLint x = luaL_checkint(L, 1);
	const GLint y = luaL_checkint(L, 2);
	const GLint w = luaL_checkint(L, 3);
	const GLint h = luaL_checkint(L, 4);
	const GLenum format = luaL_optint(L, 5, GL_RGBA);
	if ((w <= 0) || (h <= 0)) {
		return 0;
	}

	int fSize = PixelFormatSize(format);
	if (fSize < 0) {
		fSize = 4; // good enough?
	}

	float* data = new float[(h * w) * fSize * sizeof(float)];
	glReadPixels(x, y, w, h, format, GL_FLOAT, data);

	int retCount = 0;

	const float* d = data;

	if ((w == 1) && (h == 1)) {
		for (int e = 0; e < fSize; e++) {
			lua_pushnumber(L, data[e]);
		}
		retCount = fSize;
	}
	else if ((w == 1) && (h > 1)) {
		lua_newtable(L);
		for (int i = 1; i <= h; i++) {
			lua_pushnumber(L, i);
			PushPixelData(L, fSize, d);
			lua_rawset(L, -3);
		}
		retCount = 1;
	}
	else if ((w > 1) && (h == 1)) {
		lua_newtable(L);
		for (int i = 1; i <= w; i++) {
			lua_pushnumber(L, i);
			PushPixelData(L, fSize, d);
			lua_rawset(L, -3);
		}
		retCount = 1;
	}
	else {
		lua_newtable(L);
		for (int x = 1; x <= w; x++) {
			lua_pushnumber(L, x);
			lua_newtable(L);
			for (int y = 1; y <= h; y++) {
				lua_pushnumber(L, y);
				PushPixelData(L, fSize, d);
				lua_rawset(L, -3);
			}
			lua_rawset(L, -3);
		}
		retCount = 1;
	}

	delete[] data;

	return retCount;
}


int LuaOpenGL::SaveImage(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const GLint x = (GLint)luaL_checknumber(L, 1);
	const GLint y = (GLint)luaL_checknumber(L, 2);
	const GLsizei width  = (GLsizei)luaL_checknumber(L, 3);
	const GLsizei height = (GLsizei)luaL_checknumber(L, 4);
	const string filename = luaL_checkstring(L, 5);

	bool alpha = false;
	bool yflip = false;
	const int table = 6;
	if (lua_istable(L, table)) {
		lua_getfield(L, table, "alpha");
		if (lua_isboolean(L, -1)) {
			alpha = lua_toboolean(L, -1);
		}
		lua_pop(L, 1);
		lua_getfield(L, table, "yflip");
		if (lua_isboolean(L, -1)) {
			yflip = lua_toboolean(L, -1);
		}
		lua_pop(L, 1);
	}

	if ((width <= 0) || (height <= 0)) {
		return 0;
	}
	const int memsize = width * height * 4;

	unsigned char* img = new unsigned char[memsize];
	memset(img, 0, memsize);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, img);

	CBitmap bitmap(img, width, height);
	if (!yflip) {
		bitmap.ReverseYAxis();
	}

	// FIXME Check file path permission here

	lua_pushboolean(L, bitmap.Save(filename, !alpha));

	delete[] img;

	return 1;
}


/******************************************************************************/

int LuaOpenGL::CreateQuery(lua_State* L)
{
	GLuint q;
	glGenQueries(1, &q);
	if (q == 0) {
		return 0;
	}
	occlusionQueries.insert(q);
	lua_pushlightuserdata(L, (void*)q);
	return 1;
}


int LuaOpenGL::DeleteQuery(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	if (!lua_islightuserdata(L, 1)) {
		luaL_error(L, "invalid argument");
	}
	GLuint q = (unsigned long int)lua_topointer(L, 1);
	if (occlusionQueries.find(q) != occlusionQueries.end()) {
		occlusionQueries.erase(q);
		glDeleteQueries(1, &q);
	}
	return 0;
}


int LuaOpenGL::RunQuery(lua_State* L)
{
	static bool running = false;

	if (running) {
		luaL_error(L, "not re-entrant");
	}
	if (!lua_islightuserdata(L, 1)) {
		luaL_error(L, "expecting a query");
	}
	GLuint q = (unsigned long int)lua_topointer(L, 1);
	if (occlusionQueries.find(q) == occlusionQueries.end()) {
		return 0;
	}
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "expecting a function");
	}
	const int args = lua_gettop(L); // number of arguments

	running = true;
	glBeginQuery(GL_SAMPLES_PASSED, q);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEndQuery(GL_SAMPLES_PASSED);
	running = false;

	if (error != 0) {
		logOutput.Print("gl.RunQuery: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::GetQuery(lua_State* L)
{
	if (!lua_islightuserdata(L, 1)) {
		luaL_error(L, "invalid argument");
	}
	GLuint q = (unsigned long int)lua_topointer(L, 1);
	if (occlusionQueries.find(q) == occlusionQueries.end()) {
		return 0;
	}

	GLuint count;
	glGetQueryObjectuiv(q, GL_QUERY_RESULT, &count);

	lua_pushnumber(L, count);

	return 1;
}


/******************************************************************************/

int LuaOpenGL::GetGlobalTexNames(lua_State* L)
{
	map<string, C3DOTextureHandler::UnitTexture*>::const_iterator it;
	const map<string, C3DOTextureHandler::UnitTexture*>& textures =
		texturehandler3DO->GetAtlasTextures();

	lua_newtable(L);
	int count = 0;
	for (it = textures.begin(); it != textures.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushstring(L, it->first.c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;
}


int LuaOpenGL::GetGlobalTexCoords(lua_State* L)
{
	const string name = luaL_checkstring(L, 1);
	C3DOTextureHandler::UnitTexture* texCoords = NULL;
	texCoords = texturehandler3DO->Get3DOTexture(name);
	if (texCoords) {
		lua_pushnumber(L, texCoords->xstart);
		lua_pushnumber(L, texCoords->ystart);
		lua_pushnumber(L, texCoords->xend);
		lua_pushnumber(L, texCoords->yend);
		return 4;
	}
	return 0;
}


int LuaOpenGL::GetShadowMapParams(lua_State* L)
{
	if (!shadowHandler) {
		return 0;
	}
	lua_pushnumber(L, shadowHandler->GetShadowParams().x);
	lua_pushnumber(L, shadowHandler->GetShadowParams().y);
	lua_pushnumber(L, shadowHandler->GetShadowParams().z);
	lua_pushnumber(L, shadowHandler->GetShadowParams().w);
	return 4;
}


int LuaOpenGL::GetSun(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args == 0) {
		lua_pushnumber(L, mapInfo->light.sunDir[0]);
		lua_pushnumber(L, mapInfo->light.sunDir[1]);
		lua_pushnumber(L, mapInfo->light.sunDir[2]);
		return 3;
	}

	const string param = luaL_checkstring(L, 1);
	if (param == "pos") {
		lua_pushnumber(L, mapInfo->light.sunDir[0]);
		lua_pushnumber(L, mapInfo->light.sunDir[1]);
		lua_pushnumber(L, mapInfo->light.sunDir[2]);
		return 3;
	}

	const bool unitMode = lua_israwstring(L, 2) &&
	                      (strcmp(lua_tostring(L, 2), "unit") == 0);

	const float3* data = NULL;

	if (param == "shadowDensity") {
		if (!unitMode) {
			lua_pushnumber(L, mapInfo->light.groundShadowDensity);
		} else {
			lua_pushnumber(L, unitDrawer->unitShadowDensity);
		}
		return 1;
	}
	else if (param == "diffuse") {
		if (!unitMode) {
			data = &mapInfo->light.groundSunColor;
		} else {
			data = &mapInfo->light.unitSunColor;
		}
	}
	else if (param == "ambient") {
		if (!unitMode) {
			data = &mapInfo->light.groundAmbientColor;
		} else {
			data = &mapInfo->light.unitAmbientColor;
		}
	}
	else if (param == "specular") {
		if (!unitMode) {
			data = &mapInfo->light.groundSpecularColor;
		}
	}

	if (data != NULL) {
		lua_pushnumber(L, data->x);
		lua_pushnumber(L, data->y);
		lua_pushnumber(L, data->z);
		return 3;
	}

	return 0;
}

/******************************************************************************/
/******************************************************************************/

class SelectBuffer {
	public:
		static const GLsizei maxSize = (1 << 24); // float integer range
		static const GLsizei defSize = (256 * 1024);

		SelectBuffer() : size(0), buffer(NULL) {}
		~SelectBuffer() { delete[] buffer; }

		inline GLuint* GetBuffer() const { return buffer; }

		inline bool ValidIndex(int index) const {
			return ((index >= 0) && (index < size));
		}

		inline GLuint operator[](int index) const {
			return ValidIndex(index) ? buffer[index] : 0;
		}

		inline GLsizei Resize(GLsizei c) {
			c = (c < maxSize) ? c : maxSize;
			if (c != size) {
				delete[] buffer;
				buffer = new GLuint[c];
			}
			size = c;
			return size;
		}

	private:
		GLsizei size;
		GLuint* buffer;
};

static SelectBuffer selectBuffer;


/******************************************************************************/

int LuaOpenGL::RenderMode(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const GLenum mode = (GLenum)luaL_checkint(L, 1);
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.RenderMode(mode, func, ...)");
	}

	const int args = lua_gettop(L); // number of arguments

	// call the function
	glRenderMode(mode);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	const GLint count2 = glRenderMode(GL_RENDER);

	if (error != 0) {
		logOutput.Print("gl.RenderMode: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}

	lua_pushnumber(L, count2);
	return 1;
}


int LuaOpenGL::SelectBuffer(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLsizei selCount = (GLsizei)luaL_optint(L, 1, SelectBuffer::defSize);
	selCount = selectBuffer.Resize(selCount);
	glSelectBuffer(selCount, selectBuffer.GetBuffer());
	lua_pushnumber(L, selCount);
	return 1;
}


int LuaOpenGL::SelectBufferData(lua_State* L)
{
	const int index = luaL_checkint(L, 1);
	if (!selectBuffer.ValidIndex(index)) {
		return 0;
	}
	lua_pushnumber(L, selectBuffer[index]);
	return 1;
}


int LuaOpenGL::InitNames(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glInitNames();
	return 0;
}


int LuaOpenGL::LoadName(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint name = (GLenum)luaL_checkint(L, 1);
	glLoadName(name);
	return 0;
}


int LuaOpenGL::PushName(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint name = (GLenum)luaL_checkint(L, 1);
	glPushName(name);
	return 0;
}


int LuaOpenGL::PopName(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glPopName();
	return 0;
}


/******************************************************************************/
/******************************************************************************/
