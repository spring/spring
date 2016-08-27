/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


// TODO:
// - go back to counting matrix push/pops (just for modelview?)
//   would have to make sure that display lists are also handled
//   (GL_MODELVIEW_STACK_DEPTH could help current situation, but
//    requires the ARB_imaging extension)
// - use materials instead of raw calls (again, handle dlists)

#include "Rendering/GL/myGL.h"
#include <string>
#include <vector>
#include <algorithm>

#include "LuaOpenGL.h"

#include "LuaInclude.h"
#include "LuaContextData.h"
#include "LuaDisplayLists.h"
#include "LuaFBOs.h"
#include "LuaFonts.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaIO.h"
#include "LuaOpenGLUtils.h"
#include "LuaRBOs.h"
#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaUtils.h"
//FIXME#include "LuaVBOs.h"
#include "Game/Camera.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/MiniMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitDefImage.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"

using std::max;
using std::string;

#undef far // avoid collision with windef.h
#undef near

CONFIG(bool, LuaShaders).defaultValue(true).headlessValue(false).safemodeValue(false);

static const int MAX_TEXTURE_UNITS = 32;

/******************************************************************************/
/******************************************************************************/

void (*LuaOpenGL::resetMatrixFunc)(void) = NULL;

unsigned int LuaOpenGL::resetStateList = 0;

LuaOpenGL::DrawMode LuaOpenGL::drawMode = LuaOpenGL::DRAW_NONE;
LuaOpenGL::DrawMode LuaOpenGL::prevDrawMode = LuaOpenGL::DRAW_NONE;

bool  LuaOpenGL::safeMode = true;
bool  LuaOpenGL::canUseShaders = false;

float LuaOpenGL::screenWidth = 0.36f;
float LuaOpenGL::screenDistance = 0.60f;

std::set<unsigned int> LuaOpenGL::occlusionQueries;




static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index, bool allyCheck = true)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		return nullptr;
	}

	CUnit* unit = unitHandler->GetUnit(lua_toint(L, index));

	if (unit == nullptr)
		return nullptr;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return ((readAllyTeam == CEventClient::NoAccessTeam)? nullptr: unit);

	if (allyCheck && teamHandler->Ally(readAllyTeam, unit->allyteam))
		return unit;
	if ((unit->losStatus[readAllyTeam] & LOS_INLOS) != 0)
		return unit;

	return nullptr;
}

static inline CUnit* ParseDrawUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index, false);

	if (unit == nullptr)
		return nullptr;
	if (unit->isIcon)
		return nullptr;
	if (!camera->InView(unit->midPos, unit->radius))
		return nullptr;

	return unit;
}




static inline bool IsFeatureVisible(const lua_State* L, const CFeature* feature)
{
	if (CLuaHandle::GetHandleFullRead(L))
		return true;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return (readAllyTeam == CEventClient::AllAccessTeam);

	return (feature->IsInLosForAllyTeam(readAllyTeam));
}

static CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	CFeature* feature = featureHandler->GetFeature(luaL_checkint(L, index));

	if (feature == nullptr)
		return nullptr;

	if (!IsFeatureVisible(L, feature))
		return nullptr;

	return feature;
}




/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::Init()
{
	resetStateList = glGenLists(1);

	glNewList(resetStateList, GL_COMPILE);
	ResetGLState();
	glEndList();

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	canUseShaders = (globalRendering->haveGLSL && configHandler->GetBool("LuaShaders"));
}

void LuaOpenGL::Free()
{
	glDeleteLists(resetStateList, 1);
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

	if (!globalRendering->haveGLSL)
		return;

	for (auto it = occlusionQueries.begin(); it != occlusionQueries.end(); ++it) {
		glDeleteQueries(1, &(*it));
	}

	occlusionQueries.clear();
}


/******************************************************************************/
/******************************************************************************/

bool LuaOpenGL::PushEntries(lua_State* L)
{
	LuaOpenGLUtils::ResetState();

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
	if (GLEW_EXT_blend_equation_separate) {
		REGISTER_LUA_CFUNC(BlendEquationSeparate);
	}
	if (GLEW_EXT_blend_func_separate) {
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
	if (GLEW_EXT_stencil_two_side) {
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
	if (IS_GL_FUNCTION_AVAILABLE(glGenerateMipmapEXT)) {
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

	REGISTER_LUA_CFUNC(Unit);
	REGISTER_LUA_CFUNC(UnitRaw);
	REGISTER_LUA_CFUNC(UnitTextures);
	REGISTER_LUA_CFUNC(UnitShape);
	REGISTER_LUA_CFUNC(UnitShapeTextures);
	REGISTER_LUA_CFUNC(UnitMultMatrix);
	REGISTER_LUA_CFUNC(UnitPiece);
	REGISTER_LUA_CFUNC(UnitPieceMatrix);
	REGISTER_LUA_CFUNC(UnitPieceMultMatrix);

	REGISTER_LUA_CFUNC(Feature);
	REGISTER_LUA_CFUNC(FeatureRaw);
	REGISTER_LUA_CFUNC(FeatureTextures);
	REGISTER_LUA_CFUNC(FeatureShape);
	REGISTER_LUA_CFUNC(FeatureShapeTextures);
	REGISTER_LUA_CFUNC(FeatureMultMatrix);
	REGISTER_LUA_CFUNC(FeaturePiece);
	REGISTER_LUA_CFUNC(FeaturePieceMatrix);
	REGISTER_LUA_CFUNC(FeaturePieceMultMatrix);

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

	if (GLEW_ARB_occlusion_query) {
		REGISTER_LUA_CFUNC(CreateQuery);
		REGISTER_LUA_CFUNC(DeleteQuery);
		REGISTER_LUA_CFUNC(RunQuery);
		REGISTER_LUA_CFUNC(GetQuery);
	}

	REGISTER_LUA_CFUNC(GetGlobalTexNames);
	REGISTER_LUA_CFUNC(GetGlobalTexCoords);
	REGISTER_LUA_CFUNC(GetShadowMapParams);

	REGISTER_LUA_CFUNC(GetSun);
	REGISTER_LUA_CFUNC(GetAtmosphere);

	if (canUseShaders) {
		LuaShaders::PushEntries(L);
	}

	if (GLEW_EXT_framebuffer_object) {
	 	LuaFBOs::PushEntries(L);
	 	LuaRBOs::PushEntries(L);
	}

	LuaFonts::PushEntries(L);

//FIXME		LuaVBOs::PushEntries(L);

	return true;
}


/******************************************************************************/
/******************************************************************************/

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
	if (IS_GL_FUNCTION_AVAILABLE(glBlendEquation)) {
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

	if (IS_GL_FUNCTION_AVAILABLE(glUseProgram)) {
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


void LuaOpenGL::EnableCommon(DrawMode mode)
{
	assert(drawMode == DRAW_NONE);
	drawMode = mode;
	if (safeMode) {
		glPushAttrib(AttribBits);
		glCallList(resetStateList);
	}
	// FIXME  --  not needed by shadow or minimap   (use a WorldCommon ? )
	//glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
}


void LuaOpenGL::DisableCommon(DrawMode mode)
{
	assert(drawMode == mode);
	// FIXME  --  not needed by shadow or minimap
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	drawMode = DRAW_NONE;
	if (safeMode) {
		glPopAttrib();
	}
	if (IS_GL_FUNCTION_AVAILABLE(glUseProgram)) {
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
	glMatrixMode(GL_TEXTURE); {
		glPushMatrix();
	}
	glMatrixMode(GL_PROJECTION); {
		glPushMatrix();
	}
	glMatrixMode(GL_MODELVIEW); {
		glPushMatrix();
	}

	if (drawMode == DRAW_SCREEN) {
		prevDrawMode = DRAW_SCREEN;
		drawMode = DRAW_NONE;
	}
	EnableCommon(DRAW_MINIMAP);
	resetMatrixFunc = ResetMiniMapMatrices;
	ResetMiniMapMatrices();
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

	glMatrixMode(GL_TEXTURE); {
		glPopMatrix();
	}
	glMatrixMode(GL_PROJECTION); {
		glPopMatrix();
	}
	glMatrixMode(GL_MODELVIEW); {
		glPopMatrix();
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
//
//  MiniMap BG
//

void LuaOpenGL::EnableDrawInMiniMapBackground()
{
	glMatrixMode(GL_TEXTURE); {
		glPushMatrix();
	}
	glMatrixMode(GL_PROJECTION); {
		glPushMatrix();
	}
	glMatrixMode(GL_MODELVIEW); {
		glPushMatrix();
	}

	if (drawMode == DRAW_SCREEN) {
		prevDrawMode = DRAW_SCREEN;
		drawMode = DRAW_NONE;
	}
	EnableCommon(DRAW_MINIMAP_BACKGROUND);
	resetMatrixFunc = ResetMiniMapMatrices;
	ResetMiniMapMatrices();
}


void LuaOpenGL::DisableDrawInMiniMapBackground()
{
	if (prevDrawMode != DRAW_SCREEN) {
		DisableCommon(DRAW_MINIMAP_BACKGROUND);
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

	glMatrixMode(GL_TEXTURE); {
		glPopMatrix();
	}
	glMatrixMode(GL_PROJECTION); {
		glPopMatrix();
	}
	glMatrixMode(GL_MODELVIEW); {
		glPopMatrix();
	}
}


void LuaOpenGL::ResetDrawInMiniMapBackground()
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
	if (sky == nullptr)
		return;

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
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
	const int winPosY_bl = globalRendering->screenSizeY - globalRendering->winSizeY - globalRendering->winPosY; //! origin BOTTOMLEFT
	const float dist   = screenDistance;         // eye-to-screen (meters)
	const float width  = screenWidth;            // screen width (meters)
	const float vppx   = float(globalRendering->winPosX + globalRendering->viewPosX); // view pixel pos x
	const float vppy   = float(winPosY_bl + globalRendering->viewPosY); // view pixel pos y
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
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		glLoadIdentity();
		gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	}
	glMatrixMode(GL_MODELVIEW); {
		glLoadIdentity();
	}
}


void LuaOpenGL::SetupScreenLighting()
{
	if (camera == nullptr)
		return;

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
	glLoadMatrixf(camera->GetViewMatrix());
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());

	const float sunFactor = 1.0f;
	const float sf = sunFactor;
	const float* la = sunLighting->unitAmbientColor;
	const float* ld = sunLighting->unitDiffuseColor;

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
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		glLoadIdentity();
	}
	glMatrixMode(GL_MODELVIEW); {
		glLoadIdentity();
	}
}


void LuaOpenGL::ResetWorldMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		glLoadMatrixf(camera->GetProjectionMatrix());
	}
	glMatrixMode(GL_MODELVIEW); {
		glLoadMatrixf(camera->GetViewMatrix());
	}
}


void LuaOpenGL::ResetWorldShadowMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, -1.0);
	}
	glMatrixMode(GL_MODELVIEW); {
		glLoadMatrixf(shadowHandler->GetShadowMatrixRaw());
	}
}


void LuaOpenGL::ResetScreenMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		glLoadIdentity();
	}
	glMatrixMode(GL_MODELVIEW); {
		glLoadIdentity();
	}
	SetupScreenMatrices();
}


void LuaOpenGL::ResetMiniMapMatrices()
{
	glMatrixMode(GL_TEXTURE); {
		glLoadIdentity();
	}
	glMatrixMode(GL_PROJECTION); {
		glLoadIdentity();
		assert(minimap);
		glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0, -1.0);
		minimap->ApplyConstraintsMatrix();
	}
	glMatrixMode(GL_MODELVIEW); {
		glLoadIdentity();
		// engine draw minimap in 0..1 range, lua uses 0..minimapSizeX
		glScalef(1.0f / (float)minimap->GetSizeX(),
		         1.0f / (float)minimap->GetSizeY(), 1.0f);
	}
}


/******************************************************************************/
/******************************************************************************/


inline void LuaOpenGL::CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!IsDrawingEnabled(L)) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
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
	const char* pstring = (const char*) glGetString(pname);

	if (pstring != NULL) {
		lua_pushstring(L, pstring);
	} else {
		lua_pushstring(L, "[NULL]");
	}

	return 1;
}


int LuaOpenGL::ConfigScreen(lua_State* L)
{
//	CheckDrawingEnabled(L, __FUNCTION__);
	screenWidth = luaL_checkfloat(L, 1);
	screenDistance = luaL_checkfloat(L, 2);
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
	const bool mode = luaL_checkboolean(L, 1);
	if (minimap) {
		minimap->SetSlaveMode(mode);
	}
	return 0;
}


int LuaOpenGL::ConfigMiniMap(lua_State* L)
{
	if (!minimap) {
		return 0;
	}
	const int px = luaL_checkint(L, 1);
	const int py = luaL_checkint(L, 2);
	const int sx = luaL_checkint(L, 3);
	const int sy = luaL_checkint(L, 4);
	minimap->SetGeometry(px, py, sx, sy);
	return 0;
}


int LuaOpenGL::DrawMiniMap(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	if (!minimap) {
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

	bool transform = luaL_optboolean(L, 1, true);

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

	const string text = luaL_checksstring(L, 1);
	const float x     = luaL_checkfloat(L, 2);
	const float y     = luaL_checkfloat(L, 3);

	float size = luaL_optnumber(L, 4, 12.0f);
	int options = FONT_NEAREST;
	bool outline = false;
	bool lightOut = false;

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
				default: break;
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

	const string text = luaL_checksstring(L, 1);
	const float width = font->GetTextWidth(text);
	lua_pushnumber(L, width);
	return 1;
}


int LuaOpenGL::GetTextHeight(lua_State* L)
{
	const string text = luaL_checksstring(L, 1);
	float descender;
	int lines;

	const float height = font->GetTextHeight(text,&descender,&lines);
	lua_pushnumber(L, height);
	lua_pushnumber(L, descender);
	lua_pushnumber(L, lines);
	return 3;
}


/******************************************************************************/

static void GLObjectPiece(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, obj, 2);

	if (lmp == nullptr)
		return;

	glCallList(lmp->dispListID);
}

static void GLObjectPieceMultMatrix(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, obj, 2);

	if (lmp == nullptr)
		return;

	glMultMatrixf(lmp->GetModelSpaceMatrix());
}

static bool GLObjectDrawWithLuaMat(lua_State* L, CSolidObject* obj, LuaObjType objType)
{
	LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();

	if (!lmd->Enabled())
		return false;

	if (!lua_isnumber(L, 3)) {
		// calculate new LOD level
		lmd->UpdateCurrentLOD(objType, camera->ProjectedDistance(obj->pos), LuaObjectDrawer::GetDrawPassOpaqueMat());
		return true;
	}

	// set new LOD level manually
	if (lua_toint(L, 3) >= 0) {
		lmd->SetCurrentLOD(std::min(lmd->GetLODCount() - 1, static_cast<unsigned int>(lua_toint(L, 3))));
		return true;
	}

	return false;
}


static void GLObjectShape(lua_State* L, const SolidObjectDef* def)
{
	if (def == nullptr)
		return;
	if (def->LoadModel() == nullptr)
		return;

	const bool rawState = luaL_optboolean(L, 3,  true);
	const bool toScreen = luaL_optboolean(L, 4, false);

	// does not set the full state by default
	if (luaL_optboolean(L, 5, true)) {
		CUnitDrawer::DrawIndividualDefOpaque(def, luaL_checkint(L, 2), rawState, toScreen);
	} else {
		CUnitDrawer::DrawIndividualDefAlpha(def, luaL_checkint(L, 2), rawState, toScreen);
	}
}


static void GLObjectTextures(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return;
	if (obj->model == nullptr)
		return;

	if (luaL_checkboolean(L, 2)) {
		CUnitDrawer::PushModelRenderState(obj->model);
	} else {
		CUnitDrawer::PopModelRenderState(obj->model);
	}
}

static void GLObjectShapeTextures(lua_State* L, const SolidObjectDef* def)
{
	if (def == nullptr)
		return;
	if (def->LoadModel() == nullptr)
		return;

	// note: intended to accompany a *raw* ObjectDrawShape call
	// set textures and per-model(type) attributes, not shaders
	// or other drawpass state
	if (luaL_checkboolean(L, 2)) {
		CUnitDrawer::PushModelRenderState(def->model);
	} else {
		CUnitDrawer::PopModelRenderState(def->model);
	}
}


int LuaOpenGL::UnitCommon(lua_State* L, bool applyTransform, bool callDrawUnit)
{
	LuaOpenGL::CheckDrawingEnabled(L, __FUNCTION__);

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == nullptr)
		return 0;

	// NOTE:
	//   the "Raw" in UnitRaw means "no transform", not the same
	//   UnitRaw also skips the DrawUnit callin by default so any
	//   recursion is blocked; pass fullModel=true for lodCall by
	//   default to bypass nanoframe-drawing
	const bool doRawDraw = luaL_optboolean(L, 2, false);
	const bool useLuaMat = GLObjectDrawWithLuaMat(L, unit, LUAOBJ_UNIT);
	const bool noLuaCall = luaL_optboolean(L, 4, !callDrawUnit);
	const bool fullModel = luaL_optboolean(L, 5, true);

	glPushAttrib(GL_ENABLE_BIT);

	typedef void(CUnitDrawer::*RawDrawMemFunc)(const CUnit*, unsigned int, unsigned int, bool, bool);
	typedef void(CUnitDrawer::*MatDrawMemFunc)(const CUnit*, bool);

	const RawDrawMemFunc rawDrawFuncs[2] = {&CUnitDrawer::DrawUnitNoTrans, &CUnitDrawer::DrawUnit};
	const MatDrawMemFunc matDrawFuncs[2] = {&CUnitDrawer::DrawIndividualNoTrans, &CUnitDrawer::DrawIndividual};

	if (!useLuaMat) {
		// "scoped" draw; this prevents any Lua-assigned
		// material(s) from being used by the call below
		(unit->GetLuaMaterialData())->PushLODCount(0);
	}

	if (doRawDraw) {
		// draw with void material state
		(unitDrawer->*rawDrawFuncs[applyTransform])(unit, 0, 0, fullModel, noLuaCall);
	} else {
		// draw with full material state
		(unitDrawer->*matDrawFuncs[applyTransform])(unit, noLuaCall);
	}

	if (!useLuaMat) {
		(unit->GetLuaMaterialData())->PopLODCount();
	}

	glPopAttrib();
	return 0;
}

int LuaOpenGL::Unit(lua_State* L) { return (UnitCommon(L, true, true)); }
int LuaOpenGL::UnitRaw(lua_State* L) { return (UnitCommon(L, false, false)); }

int LuaOpenGL::UnitTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectTextures(L, unitHandler->GetUnit(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::UnitShape(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectShape(L, unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::UnitShapeTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectShapeTextures(L, unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1)));
	return 0;
}


int LuaOpenGL::UnitMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == nullptr)
		return 0;

	glMultMatrixf(unit->GetTransformMatrix());
	return 0;
}

int LuaOpenGL::UnitPiece(lua_State* L)
{
	GLObjectPiece(L, ParseUnit(L, __FUNCTION__, 1));
	return 0;
}

int LuaOpenGL::UnitPieceMatrix(lua_State* L) { return (UnitPieceMultMatrix(L)); }
int LuaOpenGL::UnitPieceMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectPieceMultMatrix(L, ParseUnit(L, __FUNCTION__, 1));
	return 0;
}


/******************************************************************************/

int LuaOpenGL::FeatureCommon(lua_State* L, bool applyTransform, bool callDrawFeature)
{
	LuaOpenGL::CheckDrawingEnabled(L, __FUNCTION__);

	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == nullptr)
		return 0;
	if (feature->model == nullptr)
		return 0;

	// NOTE:
	//   the "Raw" in FeatureRaw means "no transform", not the same
	//   FeatureRaw also skips the DrawFeature callin by default so
	//   any recursion is blocked
	const bool doRawDraw = luaL_optboolean(L, 2, false);
	const bool useLuaMat = GLObjectDrawWithLuaMat(L, feature, LUAOBJ_FEATURE);
	const bool noLuaCall = luaL_optboolean(L, 4, !callDrawFeature);

	glPushAttrib(GL_ENABLE_BIT);

	typedef void(CFeatureDrawer::*RawDrawMemFunc)(const CFeature*, unsigned int, unsigned int, bool, bool);
	typedef void(CFeatureDrawer::*MatDrawMemFunc)(const CFeature*, bool);

	const RawDrawMemFunc rawDrawFuncs[2] = {&CFeatureDrawer::DrawFeatureNoTrans, &CFeatureDrawer::DrawFeature};
	const MatDrawMemFunc matDrawFuncs[2] = {&CFeatureDrawer::DrawIndividualNoTrans, &CFeatureDrawer::DrawIndividual};

	if (!useLuaMat) {
		// "scoped" draw; this prevents any Lua-assigned
		// material(s) from being used by the call below
		(feature->GetLuaMaterialData())->PushLODCount(0);
	}

	if (doRawDraw) {
		// draw with void material state
		(featureDrawer->*rawDrawFuncs[applyTransform])(feature, 0, 0, false, noLuaCall);
	} else {
		// draw with full material state
		(featureDrawer->*matDrawFuncs[applyTransform])(feature, noLuaCall);
	}

	if (!useLuaMat) {
		(feature->GetLuaMaterialData())->PopLODCount();
	}

	glPopAttrib();
	return 0;
}

int LuaOpenGL::Feature(lua_State* L) { return (FeatureCommon(L, true, true)); }
int LuaOpenGL::FeatureRaw(lua_State* L) { return (FeatureCommon(L, false, false)); }

int LuaOpenGL::FeatureTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectTextures(L, featureHandler->GetFeature(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::FeatureShape(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectShape(L, featureDefHandler->GetFeatureDefByID(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::FeatureShapeTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectShapeTextures(L, featureDefHandler->GetFeatureDefByID(luaL_checkint(L, 1)));
	return 0;
}


int LuaOpenGL::FeatureMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == nullptr)
		return 0;

	glMultMatrixf(feature->GetTransformMatrixRef());
	return 0;
}

int LuaOpenGL::FeaturePiece(lua_State* L)
{
	GLObjectPiece(L, ParseFeature(L, __FUNCTION__, 1));
	return 0;
}


int LuaOpenGL::FeaturePieceMatrix(lua_State* L) { return (FeaturePieceMultMatrix(L)); }
int LuaOpenGL::FeaturePieceMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLObjectPieceMultMatrix(L, ParseFeature(L, __FUNCTION__, 1));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaOpenGL::DrawListAtUnit(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	// is visible to current read team, is not an icon
	const CUnit* unit = ParseDrawUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	while (CUnit* trans = unit->GetTransporter()) {
		unit = trans;
	}

	const unsigned int listIndex = (unsigned int)luaL_checkint(L, 2);
	const CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists(L);
	const unsigned int dlist = displayLists.GetDList(listIndex);

	if (dlist == 0)
		return 0;

	const bool useMidPos = luaL_optboolean(L, 3, true);
	const float3 drawPos = (useMidPos)? unit->drawMidPos: unit->drawPos;

	const float3 scale(luaL_optnumber(L, 4, 1.0f),
	                   luaL_optnumber(L, 5, 1.0f),
	                   luaL_optnumber(L, 6, 1.0f));
	const float degrees = luaL_optnumber(L, 7, 0.0f);
	const float3 rot(luaL_optnumber(L,  8, 0.0f),
	                 luaL_optnumber(L,  9, 1.0f),
	                 luaL_optnumber(L, 10, 0.0f));

	glPushMatrix();
	glTranslatef(drawPos.x, drawPos.y, drawPos.z);
	glRotatef(degrees, rot.x, rot.y, rot.z);
	glScalef(scale.x, scale.y, scale.z);
	glCallList(dlist);
	glPopMatrix();

	return 0;
}


int LuaOpenGL::DrawFuncAtUnit(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	// is visible to current read team, is not an icon
	const CUnit* unit = ParseDrawUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	while (CUnit* trans = unit->GetTransporter()) {
		unit = trans;
	}

	const bool useMidPos = luaL_checkboolean(L, 2);
	const float3 drawPos = (useMidPos)? unit->drawMidPos: unit->drawPos;

	if (!lua_isfunction(L, 3)) {
		luaL_error(L, "Missing function parameter in DrawFuncAtUnit()\n");
		return 0;
	}

	// call the function
	glPushMatrix();
	glTranslatef(drawPos.x, drawPos.y, drawPos.z);
	const int error = lua_pcall(L, (lua_gettop(L) - 3), 0, 0);
	glPopMatrix();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.DrawFuncAtUnit: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::DrawGroundCircle(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const int args = lua_gettop(L); // number of arguments
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));
	const float r  = luaL_checkfloat(L, 4);
	const int divs =   luaL_checkint(L, 5);

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
/*
	bool useNorm = false;

	if (lua_isboolean(L, 5)) {
		useNorm = lua_toboolean(L, 5);
	}
*/
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
	const int mapxi = mapDims.mapxp1;
	const int mapzi = mapDims.mapyp1;
	const float* heightmap = readMap->GetCornerHeightMapUnsynced();

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
			if (LuaUtils::ParseFloatArray(L, -1, vd.vert, 3) >= 2) {
				vd.hasVert = true;
			} else {
				luaL_error(L, "gl.Shape: bad vertex array");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "n") || (key == "normal")) {
			vd.norm[0] = 0.0f; vd.norm[1] = 1.0f; vd.norm[2] = 0.0f;
			if (LuaUtils::ParseFloatArray(L, -1, vd.norm, 3) == 3) {
				vd.hasNorm = true;
			} else {
				luaL_error(L, "gl.Shape: bad normal array");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "t") || (key == "texcoord")) {
			vd.txcd[0] = 0.0f; vd.txcd[1] = 0.0f;
			if (LuaUtils::ParseFloatArray(L, -1, vd.txcd, 2) == 2) {
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
			if (LuaUtils::ParseFloatArray(L, -1, vd.color, 4) >= 0) {
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

	if (!lua_istable(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.Shape(type, elements[])");
	}

	const GLuint type = (GLuint)luaL_checkint(L, 1);

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
	if ((args < 2) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.BeginEnd(type, func, ...)");
	}
	const GLuint primMode = (GLuint)luaL_checkint(L, 1);

	if (primMode == GL_POINTS) {
		WorkaroundATIPointSizeBug();
	}

	// call the function
	glBegin(primMode);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEnd();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.BeginEnd: error(%i) = %s",
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
		const float x = luaL_checkfloat(L, 1);
		const float y = luaL_checkfloat(L, 2);
		const float z = luaL_checkfloat(L, 3);
		glVertex3f(x, y, z);
	}
	else if (args == 2) {
		const float x = luaL_checkfloat(L, 1);
		const float y = luaL_checkfloat(L, 2);
		glVertex2f(x, y);
	}
	else if (args == 4) {
		const float x = luaL_checkfloat(L, 1);
		const float y = luaL_checkfloat(L, 2);
		const float z = luaL_checkfloat(L, 3);
		const float w = luaL_checkfloat(L, 4);
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

	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
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
		const float x = luaL_checkfloat(L, 1);
		const float y = luaL_checkfloat(L, 2);
		glTexCoord2f(x, y);
	}
	else if (args == 3) {
		const float x = luaL_checkfloat(L, 1);
		const float y = luaL_checkfloat(L, 2);
		const float z = luaL_checkfloat(L, 3);
		glTexCoord3f(x, y, z);
	}
	else if (args == 4) {
		const float x = luaL_checkfloat(L, 1);
		const float y = luaL_checkfloat(L, 2);
		const float z = luaL_checkfloat(L, 3);
		const float w = luaL_checkfloat(L, 4);
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
		const float x = luaL_checkfloat(L, 2);
		const float y = luaL_checkfloat(L, 3);
		glMultiTexCoord2f(texUnit, x, y);
	}
	else if (args == 3) {
		const float x = luaL_checkfloat(L, 2);
		const float y = luaL_checkfloat(L, 3);
		const float z = luaL_checkfloat(L, 4);
		glMultiTexCoord3f(texUnit, x, y, z);
	}
	else if (args == 4) {
		const float x = luaL_checkfloat(L, 2);
		const float y = luaL_checkfloat(L, 3);
		const float z = luaL_checkfloat(L, 4);
		const float w = luaL_checkfloat(L, 5);
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

	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
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
	const float x1 = luaL_checkfloat(L, 1);
	const float y1 = luaL_checkfloat(L, 2);
	const float x2 = luaL_checkfloat(L, 3);
	const float y2 = luaL_checkfloat(L, 4);
	glRectf(x1, y1, x2, y2);
	return 0;
}


int LuaOpenGL::TexRect(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	const float x1 = luaL_checkfloat(L, 1);
	const float y1 = luaL_checkfloat(L, 2);
	const float x2 = luaL_checkfloat(L, 3);
	const float y2 = luaL_checkfloat(L, 4);

	// Spring's textures get loaded with a vertical flip
	// We change that for the default settings.

	if (args <= 6) {
		float s1 = 0.0f;
		float t1 = 1.0f;
		float s2 = 1.0f;
		float t2 = 0.0f;
		if ((args >= 5) && luaL_optboolean(L, 5, false)) {
			// flip s-coords
			s1 = 1.0f;
			s2 = 0.0f;
		}
		if ((args >= 6) && luaL_optboolean(L, 6, false)) {
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

	const float s1 = luaL_checkfloat(L, 5);
	const float t1 = luaL_checkfloat(L, 6);
	const float s2 = luaL_checkfloat(L, 7);
	const float t2 = luaL_checkfloat(L, 8);
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
		const int count = LuaUtils::ParseFloatArray(L, -1, color, 4);
		if (count < 3) {
			luaL_error(L, "Incorrect arguments to gl.Color()");
		}
		if (count == 3) {
			color[3] = 1.0f;
		}
	}
	else if (args >= 3) {
		color[0] = (GLfloat)luaL_checkfloat(L, 1);
		color[1] = (GLfloat)luaL_checkfloat(L, 2);
		color[2] = (GLfloat)luaL_checkfloat(L, 3);
		color[3] = (GLfloat)luaL_optnumber(L, 4, 1.0f);
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
			LOG_L(L_WARNING, "gl.Material: bad state type");
			return 0;
		}
		const string key = lua_tostring(L, -2);

		if (key == "shininess") {
			if (lua_isnumber(L, -1)) {
				const GLfloat specExp = (GLfloat)lua_tonumber(L, -1);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, specExp);
			}
			continue;
		}

		const int count = LuaUtils::ParseFloatArray(L, -1, color, 4);
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
			LOG_L(L_WARNING, "gl.Material: unknown material type: %s",
					key.c_str());
		}
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::ResetState(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
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
	if (luaL_checkboolean(L, 1)) {
		glEnable(GL_LIGHTING);
	} else {
		glDisable(GL_LIGHTING);
	}
	return 0;
}


int LuaOpenGL::ShadeModel(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	glShadeModel((GLenum)luaL_checkint(L, 1));
	return 0;
}


int LuaOpenGL::Scissor(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}
	else if (args == 4) {
		glEnable(GL_SCISSOR_TEST);
		const GLint   x =   (GLint)luaL_checkint(L, 1);
		const GLint   y =   (GLint)luaL_checkint(L, 2);
		const GLsizei w = (GLsizei)luaL_checkint(L, 3);
		const GLsizei h = (GLsizei)luaL_checkint(L, 4);
		if (w < 0) luaL_argerror(L, 3, "<width> must be greater than or equal zero!");
		if (h < 0) luaL_argerror(L, 4, "<height> must be greater than or equal zero!");
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
	const int y = luaL_checkint(L, 2);
	const int w = luaL_checkint(L, 3);
	const int h = luaL_checkint(L, 4);
	if (w < 0) luaL_argerror(L, 3, "<width> must be greater than or equal zero!");
	if (h < 0) luaL_argerror(L, 4, "<height> must be greater than or equal zero!");

	glViewport(x, y, w, h);
	return 0;
}


int LuaOpenGL::ColorMask(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		} else {
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}
	}
	else if (args == 4) {
		glColorMask(luaL_checkboolean(L, 1), luaL_checkboolean(L, 2),
		            luaL_checkboolean(L, 3), luaL_checkboolean(L, 4));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.ColorMask()");
	}
	return 0;
}


int LuaOpenGL::DepthMask(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	if (luaL_checkboolean(L, 1)) {
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

	if (luaL_checkboolean(L, 1)) {
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
		if (luaL_checkboolean(L, 1)) {
			glEnable(GL_ALPHA_TEST);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
	}
	else if (args == 2) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc((GLenum)luaL_checkint(L, 1), (GLfloat)luaL_checkint(L, 2));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.AlphaTest()");
	}
	return 0;
}


int LuaOpenGL::PolygonMode(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLenum face = (GLenum)luaL_checkint(L, 1);
	const GLenum mode = (GLenum)luaL_checkint(L, 2);
	glPolygonMode(face, mode);
	return 0;
}


int LuaOpenGL::PolygonOffset(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
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
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glEnable(GL_POLYGON_OFFSET_POINT);
		glPolygonOffset((GLfloat)luaL_checkfloat(L, 1), (GLfloat)luaL_checkfloat(L, 2));
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
		GLint factor     =    (GLint)luaL_checkint(L, 1);
		GLushort pattern = (GLushort)luaL_checkint(L, 2);
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
	const float width = luaL_checkfloat(L, 1);
	if (width <= 0.0f) luaL_argerror(L, 1, "Incorrect Width (must be greater zero)");
	glLineWidth(width);
	return 0;
}


int LuaOpenGL::PointSize(lua_State* L)
{
	const float size = luaL_checkfloat(L, 1);
	if (size <= 0.0f) luaL_argerror(L, 1, "Incorrect Size (must be greater zero)");
	glPointSize(size);
	return 0;
}


int LuaOpenGL::PointSprite(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments

	if (luaL_checkboolean(L, 1)) {
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

	LuaMatTexture tex;
	const bool loaded = LuaOpenGLUtils::ParseTextureImage(L, tex, lua_tostring(L, nextArg));
	if (loaded) {
		tex.enable = true;
		tex.Bind();
	}

	if (texUnit != GL_TEXTURE0)
		glActiveTexture(GL_TEXTURE0);

	lua_pushboolean(L, loaded);
	return 1;
}


int LuaOpenGL::CreateTexture(lua_State* L)
{
	LuaTextures::Texture tex;
	tex.xsize = (GLsizei)luaL_checknumber(L, 1);
	tex.ysize = (GLsizei)luaL_checknumber(L, 2);
	if (tex.xsize <= 0) luaL_argerror(L, 1, "Texture Size must be greater than zero!");
	if (tex.ysize <= 0) luaL_argerror(L, 2, "Texture Size must be greater than zero!");

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

	LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
	const string name = textures.Create(tex);
	if (name.empty()) {
		return 0;
	}

	lua_pushsstring(L, name);
	return 1;
}


int LuaOpenGL::DeleteTexture(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	const string texture = luaL_checksstring(L, 1);
	if (texture[0] == LuaTextures::prefix) { // '!'
		LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
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

	LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
	lua_pushboolean(L, textures.FreeFBO(luaL_checksstring(L, 1)));
	return 1;
}


int LuaOpenGL::TextureInfo(lua_State* L)
{
	LuaMatTexture tex;

	if (!LuaOpenGLUtils::ParseTextureImage(L, tex, luaL_checkstring(L, 1))) {
		return 0;
	}

	lua_createtable(L, 0, 2);
	HSTR_PUSH_NUMBER(L, "xsize", tex.GetSize().x);
	HSTR_PUSH_NUMBER(L, "ysize", tex.GetSize().y);
	// HSTR_PUSH_BOOL(L,   "alpha", texInfo.alpha);  FIXME
	// HSTR_PUSH_NUMBER(L, "type",  texInfo.type);
	return 1;
}


int LuaOpenGL::CopyToTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const string texture = luaL_checkstring(L, 1);
	if (texture[0] != LuaTextures::prefix) { // '!'
		luaL_error(L, "gl.CopyToTexture() can only write to lua textures");
	}
	LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
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
	LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
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
		LOG_L(L_ERROR, "gl.RenderToTexture: error(%i) = %s",
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
	LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
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
		LOG_L(L_ERROR, "gl.ActiveTexture: error(%i) = %s",
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
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	glTranslatef(x, y, z);
	return 0;
}


int LuaOpenGL::Scale(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	glScalef(x, y, z);
	return 0;
}


int LuaOpenGL::Rotate(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const float r = luaL_checkfloat(L, 1);
	const float x = luaL_checkfloat(L, 2);
	const float y = luaL_checkfloat(L, 3);
	const float z = luaL_checkfloat(L, 4);
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
	glMultMatrixf(camera->GetBillBoardMatrix());
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

	const int plane = luaL_checkint(L, 1);
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
	GLdouble equation[4];
	equation[0] = (double)luaL_checknumber(L, 2);
	equation[1] = (double)luaL_checknumber(L, 3);
	equation[2] = (double)luaL_checknumber(L, 4);
	equation[3] = (double)luaL_checknumber(L, 5);
	glClipPlane(gl_plane, equation);
	glEnable(gl_plane);
	return 0;
}


/******************************************************************************/

int LuaOpenGL::MatrixMode(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	GLenum mode = (GLenum)luaL_checkint(L, 1);
	if (!GetLuaContextData(L)->glMatrixTracker.SetMatrixMode(mode))
		luaL_error(L, "Incorrect value to gl.MatrixMode");
	glMatrixMode(mode);
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

	const int luaType = lua_type(L, 1);
	if (luaType == LUA_TSTRING) {
		const CMatrix44f* matptr = LuaOpenGLUtils::GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			glLoadMatrixf(*matptr);
		} else {
			luaL_error(L, "Incorrect arguments to gl.LoadMatrix()");
		}
		return 0;
	} else {
		GLfloat matrix[16];
		if (luaType == LUA_TTABLE) {
			if (LuaUtils::ParseFloatArray(L, -1, matrix, 16) != 16) {
				luaL_error(L, "gl.LoadMatrix requires all 16 values");
			}
		}
		else {
			for (int i = 1; i <= 16; i++) {
				matrix[i-1] = (GLfloat)luaL_checknumber(L, i);
			}
		}
		glLoadMatrixf(matrix);
	}
	return 0;
}


int LuaOpenGL::MultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int luaType = lua_type(L, 1);
	if (luaType == LUA_TSTRING) {
		const CMatrix44f* matptr = LuaOpenGLUtils::GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			glMultMatrixf(*matptr);
		} else {
			luaL_error(L, "Incorrect arguments to gl.MultMatrix()");
		}
		return 0;
	} else {
		GLfloat matrix[16];
		if (luaType == LUA_TTABLE) {
			if (LuaUtils::ParseFloatArray(L, -1, matrix, 16) != 16) {
				luaL_error(L, "gl.LoadMatrix requires all 16 values");
			}
		}
		else {
			for (int i = 1; i <= 16; i++) {
				matrix[i-1] = (GLfloat)luaL_checknumber(L, i);
			}
		}
		glMultMatrixf(matrix);
	}
	return 0;
}


int LuaOpenGL::PushMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "gl.PushMatrix takes no arguments");
	}

	if (!GetLuaContextData(L)->glMatrixTracker.PushMatrix())
		luaL_error(L, "Matrix stack overflow");
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

	if (!GetLuaContextData(L)->glMatrixTracker.PopMatrix())
		luaL_error(L, "Matrix stack underflow");
	glPopMatrix();

	return 0;
}


int LuaOpenGL::PushPopMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	std::vector<GLenum> matModes;
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
		LOG_L(L_ERROR, "gl.PushPopMatrix: error(%i) = %s",
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
		const CMatrix44f* matptr = LuaOpenGLUtils::GetNamedMatrix(lua_tostring(L, 1));

		if (!matptr) {
			luaL_error(L, "Incorrect arguments to gl.GetMatrixData(name)");
		}

		if (lua_isnumber(L, 2)) {
			const int index = lua_toint(L, 2);
			if ((index < 0) || (index >= 16)) {
				return 0;
			}
			lua_pushnumber(L, (*matptr)[index]);
			return 1;
		}

		for (int i = 0; i < 16; i++) {
			lua_pushnumber(L, (*matptr)[i]);
		}

		return 16;
	}

	return 0;
}

/******************************************************************************/

int LuaOpenGL::PushAttrib(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	int mask = luaL_optnumber(L, 1, GL_ALL_ATTRIB_BITS);
	if (mask < 0) {
		mask = -mask;
		mask |= GL_ALL_ATTRIB_BITS;
	}
	glPushAttrib((GLbitfield)mask);
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
		LOG_L(L_ERROR, "gl.UnsafeState: error(%i) = %s",
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
	const bool origDrawingEnabled = IsDrawingEnabled(L);
	SetDrawingEnabled(L, true);

	// build the list with the specified lua call/args
	glNewList(list, GL_COMPILE);
	SMatrixStateData prevMSD = GetLuaContextData(L)->glMatrixTracker.PushMatrixState(true);
	const int error = lua_pcall(L, (args - 1), 0, 0);
	SMatrixStateData matData = GetLuaContextData(L)->glMatrixTracker.GetMatrixState();
	GetLuaContextData(L)->glMatrixTracker.PopMatrixState(prevMSD, false);
	glEndList();

	if (error != 0) {
		glDeleteLists(list, 1);
		LOG_L(L_ERROR, "gl.CreateList: error(%i) = %s",
				error, lua_tostring(L, -1));
		lua_pushnumber(L, 0);
	}
	else {
		CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists(L);
		const unsigned int index = displayLists.NewDList(list, matData);
		lua_pushnumber(L, index);
	}

	// restore the state
	SetDrawingEnabled(L, origDrawingEnabled);

	return 1;
}


int LuaOpenGL::CallList(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const unsigned int listIndex = luaL_checkint(L, 1);
	const CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists(L);
	const unsigned int dlist = displayLists.GetDList(listIndex);
	if (dlist) {
		SMatrixStateData matrixStateData = displayLists.GetMatrixState(listIndex);
		int error = GetLuaContextData(L)->glMatrixTracker.ApplyMatrixState(matrixStateData);
		if (error == 0) {
			glCallList(dlist);
			return 0;
		}
		luaL_error(L, "Matrix stack %sflow in gl.CallList", (error > 0) ? "over" : "under");
	}
	return 0;
}


int LuaOpenGL::DeleteList(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	const unsigned int listIndex = (unsigned int)luaL_checkint(L, 1);
	CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists(L);
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
	const GLint x = (GLint)luaL_checknumber(L, 1);
	const GLint y = (GLint)luaL_checknumber(L, 2);
	const GLsizei width  = (GLsizei)luaL_checknumber(L, 3);
	const GLsizei height = (GLsizei)luaL_checknumber(L, 4);
	const string filename = luaL_checkstring(L, 5);

	if (!LuaIO::SafeWritePath(filename) || !LuaIO::IsSimplePath(filename)) {
		LOG_L(L_WARNING, "gl.SaveImage: tried to write to illegal path localtion");
		return 0;
	}
	if ((width <= 0) || (height <= 0)) {
		LOG_L(L_WARNING, "gl.SaveImage: tried to write empty image");
		return 0;
	}

	bool alpha = false;
	bool yflip = false;
	bool gray16b = false;
	const int table = 6;
	if (lua_istable(L, table)) {
		lua_getfield(L, table, "alpha");
		alpha = luaL_optboolean(L, -1, false);
		lua_pop(L, 1);

		lua_getfield(L, table, "yflip");
		yflip = luaL_optboolean(L, -1, false);
		lua_pop(L, 1);

		lua_getfield(L, table, "grayscale16bit");
		gray16b = luaL_optboolean(L, -1, false);
		lua_pop(L, 1);
	}

	CBitmap bitmap;
	bitmap.Alloc(width, height);

	if (!gray16b) {
		glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &bitmap.mem[0]);
		if (!yflip) bitmap.ReverseYAxis();
		lua_pushboolean(L, bitmap.Save(filename, !alpha));
	} else {
		// single channel only!
		glReadPixels(x, y, width, height, GL_LUMINANCE, GL_FLOAT, &bitmap.mem[0]);
		if (!yflip) bitmap.ReverseYAxis();
		lua_pushboolean(L, bitmap.SaveFloat(filename));
	}

	return 1;
}


/******************************************************************************/

int LuaOpenGL::CreateQuery(lua_State* L)
{
	GLuint q;
	glGenQueries(1, &q);

	if (q == 0)
		return 0;

	// store a raw pointer to the inserted query value as userdata
	const auto pair = occlusionQueries.insert(q);
	const auto iter = pair.first;
	const void* addr = &(*iter);

	lua_pushlightuserdata(L, const_cast<void*>(addr));
	return 1;
}


int LuaOpenGL::DeleteQuery(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "invalid argument");

	const GLuint* ptr = reinterpret_cast<const unsigned int*>(lua_topointer(L, 1));
	const GLuint qry = *ptr;

	if (occlusionQueries.find(qry) != occlusionQueries.end()) {
		occlusionQueries.erase(qry);
		glDeleteQueries(1, &qry);
	}

	return 0;
}


int LuaOpenGL::RunQuery(lua_State* L)
{
	static bool running = false;

	if (running)
		luaL_error(L, "not re-entrant");

	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "expecting a query");

	const GLuint* ptr = reinterpret_cast<const unsigned int*>(lua_topointer(L, 1));
	const GLuint qry = *ptr;

	if (occlusionQueries.find(qry) == occlusionQueries.end())
		return 0;

	if (!lua_isfunction(L, 2))
		luaL_error(L, "expecting a function");

	const int args = lua_gettop(L); // number of arguments

	running = true;
	glBeginQuery(GL_SAMPLES_PASSED, qry);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEndQuery(GL_SAMPLES_PASSED);
	running = false;

	if (error != 0) {
		LOG_L(L_ERROR, "gl.RunQuery: error(%i) = %s",
				error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::GetQuery(lua_State* L)
{
	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "invalid argument");

	const GLuint* ptr = reinterpret_cast<const unsigned int*>(lua_topointer(L, 1));
	const GLuint qry = *ptr;

	if (occlusionQueries.find(qry) == occlusionQueries.end())
		return 0;

	GLuint count;
	glGetQueryObjectuiv(qry, GL_QUERY_RESULT, &count);

	lua_pushnumber(L, count);
	return 1;
}


/******************************************************************************/

int LuaOpenGL::GetGlobalTexNames(lua_State* L)
{
	map<string, C3DOTextureHandler::UnitTexture>::const_iterator it;
	const map<string, C3DOTextureHandler::UnitTexture>& textures =
		texturehandler3DO->GetAtlasTextures();

	lua_createtable(L, textures.size(), 0);
	int count = 1;
	for (it = textures.begin(); it != textures.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_rawseti(L, -2, count++);
	}
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
	lua_pushnumber(L, shadowHandler->GetShadowParams().x);
	lua_pushnumber(L, shadowHandler->GetShadowParams().y);
	lua_pushnumber(L, shadowHandler->GetShadowParams().z);
	lua_pushnumber(L, shadowHandler->GetShadowParams().w);
	return 4;
}

int LuaOpenGL::GetAtmosphere(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args == 0) {
		lua_pushnumber(L, sky->GetLight()->GetLightDir().x);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().y);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().z);
		return 3;
	}

	const string param = luaL_checkstring(L, 1);
	if (param == "pos") {
		lua_pushnumber(L, sky->GetLight()->GetLightDir().x);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().y);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().z);
		return 3;
	}

	const float3* data = NULL;

	// float
	if (param == "fogStart") {
		lua_pushnumber(L, sky->fogStart);
		return 1;
	} else if (param == "fogEnd") {
		lua_pushnumber(L, sky->fogEnd);
		return 1;
	// float3
	} else if (param == "fogColor") {
		data = &sky->fogColor;
	} else if (param == "skyColor") {
		data = &sky->skyColor;
	} else if (param == "skyDir") {
		//data = &sky->sunColor;
	} else if (param == "sunColor") {
		data = &sky->sunColor;
	} else if (param == "cloudColor") {
		data = &sky->cloudColor;
	}

	if (data != NULL) {
		lua_pushnumber(L, data->x);
		lua_pushnumber(L, data->y);
		lua_pushnumber(L, data->z);
		return 3;
	}

	return 0;
}

int LuaOpenGL::GetSun(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args == 0) {
		lua_pushnumber(L, sky->GetLight()->GetLightDir().x);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().y);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().z);
		return 3;
	}

	const string param = luaL_checkstring(L, 1);
	if (param == "pos") {
		lua_pushnumber(L, sky->GetLight()->GetLightDir().x);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().y);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().z);
		return 3;
	}

	const bool unitMode = lua_israwstring(L, 2) &&
	                      (strcmp(lua_tostring(L, 2), "unit") == 0);

	const float3* data = NULL;

	if (param == "shadowDensity") {
		if (!unitMode) {
			lua_pushnumber(L, sky->GetLight()->GetGroundShadowDensity());
		} else {
			lua_pushnumber(L, sky->GetLight()->GetUnitShadowDensity());
		}
		return 1;
	}
	else if (param == "diffuse") {
		if (!unitMode) {
			data = &sunLighting->groundDiffuseColor;
		} else {
			data = &sunLighting->unitDiffuseColor;
		}
	}
	else if (param == "ambient") {
		if (!unitMode) {
			data = &sunLighting->groundAmbientColor;
		} else {
			data = &sunLighting->unitAmbientColor;
		}
	}
	else if (param == "specular") {
		if (!unitMode) {
			data = &sunLighting->groundSpecularColor;
		} else {
			data = &sunLighting->unitSpecularColor;
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
