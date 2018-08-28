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
#include "Map/SMF/SMFGroundDrawer.h"
#include "Map/SMF/SMFReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
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
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"

using std::max;
using std::string;

#undef far // avoid collision with windef.h
#undef near

static const int MAX_TEXTURE_UNITS = 32;

/******************************************************************************/
/******************************************************************************/

void (*LuaOpenGL::resetMatrixFunc)(void) = nullptr;

LuaOpenGL::DrawMode LuaOpenGL::drawMode = LuaOpenGL::DRAW_NONE;
LuaOpenGL::DrawMode LuaOpenGL::prevDrawMode = LuaOpenGL::DRAW_NONE;

bool  LuaOpenGL::safeMode = true;

float LuaOpenGL::screenWidth = 0.36f; // screen width (meters)
float LuaOpenGL::screenDistance = 0.60f; // eye-to-screen (meters)

std::vector<LuaOpenGL::OcclusionQuery*> LuaOpenGL::occlusionQueries;




static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index, bool allyCheck = true)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		return nullptr;
	}

	CUnit* unit = unitHandler.GetUnit(lua_toint(L, index));

	if (unit == nullptr)
		return nullptr;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return ((readAllyTeam == CEventClient::NoAccessTeam)? nullptr: unit);

	if (allyCheck && teamHandler.Ally(readAllyTeam, unit->allyteam))
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
	CFeature* feature = featureHandler.GetFeature(luaL_checkint(L, index));

	if (feature == nullptr)
		return nullptr;

	if (!IsFeatureVisible(L, feature))
		return nullptr;

	return feature;
}




/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::Free()
{
	for (const OcclusionQuery* q: occlusionQueries) {
		glDeleteQueries(1, &q->id);
	}

	occlusionQueries.clear();
}

/******************************************************************************/
/******************************************************************************/

bool LuaOpenGL::PushEntries(lua_State* L)
{
	LuaOpenGLUtils::ResetState();

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
	REGISTER_LUA_CFUNC(SwapBuffers);
	REGISTER_LUA_CFUNC(Lighting);
	REGISTER_LUA_CFUNC(ShadeModel);
	REGISTER_LUA_CFUNC(Scissor);
	REGISTER_LUA_CFUNC(Viewport);
	REGISTER_LUA_CFUNC(ColorMask);
	REGISTER_LUA_CFUNC(DepthMask);
	REGISTER_LUA_CFUNC(DepthTest);
	REGISTER_LUA_CFUNC(DepthClamp);

	REGISTER_LUA_CFUNC(Culling);
	REGISTER_LUA_CFUNC(LogicOp);
	REGISTER_LUA_CFUNC(Fog);
	REGISTER_LUA_CFUNC(AlphaTest);
	REGISTER_LUA_CFUNC(LineStipple);
	REGISTER_LUA_CFUNC(Blending);
	REGISTER_LUA_CFUNC(BlendEquation);
	REGISTER_LUA_CFUNC(BlendFunc);
	REGISTER_LUA_CFUNC(BlendEquationSeparate);
	REGISTER_LUA_CFUNC(BlendFuncSeparate);

	REGISTER_LUA_CFUNC(Material);
	REGISTER_LUA_CFUNC(Color);

	REGISTER_LUA_CFUNC(PolygonMode);
	REGISTER_LUA_CFUNC(PolygonOffset);

	REGISTER_LUA_CFUNC(StencilTest);
	REGISTER_LUA_CFUNC(StencilMask);
	REGISTER_LUA_CFUNC(StencilFunc);
	REGISTER_LUA_CFUNC(StencilOp);
	REGISTER_LUA_CFUNC(StencilMaskSeparate);
	REGISTER_LUA_CFUNC(StencilFuncSeparate);
	REGISTER_LUA_CFUNC(StencilOpSeparate);

	REGISTER_LUA_CFUNC(LineWidth);
	REGISTER_LUA_CFUNC(PointSize);

	REGISTER_LUA_CFUNC(PointSprite);
	REGISTER_LUA_CFUNC(PointParameter);

	REGISTER_LUA_CFUNC(Texture);
	REGISTER_LUA_CFUNC(CreateTexture);
	REGISTER_LUA_CFUNC(DeleteTexture);
	REGISTER_LUA_CFUNC(TextureInfo);
	REGISTER_LUA_CFUNC(CopyToTexture);

	// FIXME: obsolete
	REGISTER_LUA_CFUNC(DeleteTextureFBO);
	REGISTER_LUA_CFUNC(RenderToTexture);

	REGISTER_LUA_CFUNC(GenerateMipmap);

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
	REGISTER_LUA_CFUNC(EndText);
	REGISTER_LUA_CFUNC(DrawBufferedText);
	REGISTER_LUA_CFUNC(Text);
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

	REGISTER_LUA_CFUNC(CreateQuery);
	REGISTER_LUA_CFUNC(DeleteQuery);
	REGISTER_LUA_CFUNC(RunQuery);
	REGISTER_LUA_CFUNC(GetQuery);

	REGISTER_LUA_CFUNC(GetGlobalTexNames);
	REGISTER_LUA_CFUNC(GetGlobalTexCoords);
	REGISTER_LUA_CFUNC(GetShadowMapParams);

	REGISTER_LUA_CFUNC(GetAtmosphere);
	REGISTER_LUA_CFUNC(GetSun);
	REGISTER_LUA_CFUNC(GetWaterRendering);
	REGISTER_LUA_CFUNC(GetMapRendering);
	REGISTER_LUA_CFUNC(GetMapShaderUniform);

	LuaShaders::PushEntries(L);
 	LuaFBOs::PushEntries(L);
 	LuaRBOs::PushEntries(L);
	// TODO LuaVBOs::PushEntries(L);

	LuaFonts::PushEntries(L);
	return true;
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::ResetGLState()
{
	glAttribStatePtr->DisableDepthTest(); // ::DepthTest
	glAttribStatePtr->DepthFunc(GL_LEQUAL); // ::DepthTest
	glAttribStatePtr->DisableDepthMask(); // ::DepthMask
	#if 0
	glAttribStatePtr->DisableDepthClamp(); // ::DepthClamp
	#endif
	glAttribStatePtr->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // ::ColorMask, ::*DrawWorldShadow (!)

	glAttribStatePtr->EnableBlendMask(); // ::Blending
	#if 0
	glBlendEquation(GL_FUNC_ADD); // ::BlendEquation*
	#endif
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ::Blending, ::BlendFunc*

	glAttribStatePtr->DisableAlphaTest();
	glAttribStatePtr->AlphaFunc(GL_GREATER, 0.5f);

	#if 0
	glDisable(GL_COLOR_LOGIC_OP); // ::LogicOp
	glLogicOp(GL_INVERT); // ::LogicOp
	#endif

	glAttribStatePtr->DisableCullFace(); // ::Culling
	glAttribStatePtr->CullFace(GL_BACK); // ::Culling

	#if 0
	glAttribStatePtr->DisableScissorTest(); // ::Scissor
	#endif

	#if 0
	glAttribStatePtr->DisableStencilTest(); // ::StencilTest
	glAttribStatePtr->StencilMask(~0); // ::StencilMask*
	// glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	#endif

	#if 0
	// FIXME -- multitexturing
	// glDisable(GL_TEXTURE_2D);
	#endif

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL); // ::PolygonMode
	#if 0
	glAttribStatePtr->PolygonOffsetFill(GL_FALSE); // ::PolygonOffset, ::*DrawWorldShadow (!)
	glAttribStatePtr->PolygonOffsetLine(GL_FALSE);
	glAttribStatePtr->PolygonOffsetPoint(GL_FALSE);
	#endif

	#if 0
	glDisable(GL_LINE_STIPPLE); // ::LineStipple

	glDisable(GL_CLIP_PLANE4); // ::ClipPlane
	glDisable(GL_CLIP_PLANE5); // ::ClipPlane
	#endif
	glAttribStatePtr->LineWidth(1.0f); // ::LineWidth

	#if 0
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // ::Color, ::Shape
	#endif
	glUseProgram(0);
}


/******************************************************************************/
/******************************************************************************/
//
//  Common routines
//

constexpr GLbitfield AttribBits =
	GL_COLOR_BUFFER_BIT |
	GL_DEPTH_BUFFER_BIT |
	GL_ENABLE_BIT       |
	GL_LINE_BIT         |
	GL_POLYGON_BIT      |
	GL_VIEWPORT_BIT;


void LuaOpenGL::EnableCommon(DrawMode mode)
{
	assert(drawMode == DRAW_NONE);
	drawMode = mode;

	if (!safeMode)
		return;

	glAttribStatePtr->PushBits(AttribBits);
	ResetGLState();
}


void LuaOpenGL::DisableCommon(DrawMode mode)
{
	assert(drawMode == mode);
	drawMode = DRAW_NONE;

	if (safeMode)
		glAttribStatePtr->PopBits();

	glUseProgram(0);

}


/******************************************************************************/
//
//  Genesis
//

void LuaOpenGL::EnableDrawGenesis()
{
	EnableCommon(DRAW_GENESIS);

	resetMatrixFunc = ResetGenesisMatrices;
	resetMatrixFunc();
}


void LuaOpenGL::DisableDrawGenesis()
{
	if (safeMode)
		ResetGenesisMatrices();

	DisableCommon(DRAW_GENESIS);
}


void LuaOpenGL::ResetDrawGenesis()
{
	if (!safeMode)
		return;

	ResetGenesisMatrices();
	ResetGLState();
}


/******************************************************************************/
//
//  World
//

void LuaOpenGL::EnableDrawWorld()
{
	EnableCommon(DRAW_WORLD);
	resetMatrixFunc = ResetWorldMatrices;
}

void LuaOpenGL::DisableDrawWorld()
{
	if (safeMode)
		ResetWorldMatrices();

	DisableCommon(DRAW_WORLD);
}

void LuaOpenGL::ResetDrawWorld()
{
	if (!safeMode)
		return;

	ResetWorldMatrices();
	ResetGLState();
}


/******************************************************************************/
//
//  WorldPreUnit -- the same as World
//

void LuaOpenGL::EnableDrawWorldPreUnit()
{
	EnableCommon(DRAW_WORLD);
	resetMatrixFunc = ResetWorldMatrices;
}

void LuaOpenGL::DisableDrawWorldPreUnit()
{
	if (safeMode)
		ResetWorldMatrices();

	DisableCommon(DRAW_WORLD);
}

void LuaOpenGL::ResetDrawWorldPreUnit()
{
	if (!safeMode)
		return;

	ResetWorldMatrices();
	ResetGLState();
}


/******************************************************************************/
//
//  WorldShadow
//

void LuaOpenGL::EnableDrawWorldShadow()
{
	EnableCommon(DRAW_WORLD_SHADOW);
	resetMatrixFunc = ResetWorldShadowMatrices;

	glAttribStatePtr->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glAttribStatePtr->PolygonOffset(1.0f, 1.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);

	// FIXME: map/proj/tree passes
	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();
}

void LuaOpenGL::DisableDrawWorldShadow()
{
	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Disable();

	ResetWorldShadowMatrices();
	DisableCommon(DRAW_WORLD_SHADOW);
}

void LuaOpenGL::ResetDrawWorldShadow()
{
	if (!safeMode)
		return;

	ResetWorldShadowMatrices();
	ResetGLState();

	glAttribStatePtr->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glAttribStatePtr->PolygonOffset(1.0f, 1.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);
}


/******************************************************************************/
//
//  WorldReflection
//

void LuaOpenGL::EnableDrawWorldReflection()
{
	EnableCommon(DRAW_WORLD_REFLECTION);
	resetMatrixFunc = ResetWorldMatrices;
}

void LuaOpenGL::DisableDrawWorldReflection()
{
	if (safeMode)
		ResetWorldMatrices();

	DisableCommon(DRAW_WORLD_REFLECTION);
}

void LuaOpenGL::ResetDrawWorldReflection()
{
	if (!safeMode)
		return;

	ResetWorldMatrices();
	ResetGLState();
}


/******************************************************************************/
//
//  WorldRefraction
//

void LuaOpenGL::EnableDrawWorldRefraction()
{
	EnableCommon(DRAW_WORLD_REFRACTION);
	resetMatrixFunc = ResetWorldMatrices;
}

void LuaOpenGL::DisableDrawWorldRefraction()
{
	if (safeMode)
		ResetWorldMatrices();

	DisableCommon(DRAW_WORLD_REFRACTION);
}

void LuaOpenGL::ResetDrawWorldRefraction()
{
	if (!safeMode)
		return;

	ResetWorldMatrices();
	ResetGLState();
}

/******************************************************************************/
//
//  Screen, ScreenEffects, ScreenPost
//

void LuaOpenGL::EnableDrawScreenCommon()
{
	EnableCommon(DRAW_SCREEN);
	resetMatrixFunc = ResetScreenMatrices;

	SetupScreenMatrices();
	ResetGLState();
}


void LuaOpenGL::DisableDrawScreenCommon()
{
	RevertScreenMatrices();
	DisableCommon(DRAW_SCREEN);
}


void LuaOpenGL::ResetDrawScreenCommon()
{
	if (!safeMode)
		return;

	ResetScreenMatrices();
	ResetGLState();
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
	resetMatrixFunc();
}


void LuaOpenGL::DisableDrawInMiniMap()
{
	if (prevDrawMode != DRAW_SCREEN) {
		DisableCommon(DRAW_MINIMAP);
		return;
	}

	if (safeMode) {
		glAttribStatePtr->PopBits();
	} else {
		ResetGLState();
	}

	resetMatrixFunc = ResetScreenMatrices;
	resetMatrixFunc();

	prevDrawMode = DRAW_NONE;
	drawMode = DRAW_SCREEN;
}

void LuaOpenGL::ResetDrawInMiniMap()
{
	if (!safeMode)
		return;

	ResetMiniMapMatrices();
	ResetGLState();
}


/******************************************************************************/
//
//  MiniMap BG
//

void LuaOpenGL::EnableDrawInMiniMapBackground()
{
	if (drawMode == DRAW_SCREEN) {
		prevDrawMode = DRAW_SCREEN;
		drawMode = DRAW_NONE;
	}

	EnableCommon(DRAW_MINIMAP_BACKGROUND);

	resetMatrixFunc = ResetMiniMapMatrices;
	resetMatrixFunc();
}


void LuaOpenGL::DisableDrawInMiniMapBackground()
{
	if (prevDrawMode != DRAW_SCREEN) {
		DisableCommon(DRAW_MINIMAP_BACKGROUND);
		return;
	}

	if (safeMode) {
		glAttribStatePtr->PopBits();
	} else {
		ResetGLState();
	}

	resetMatrixFunc = ResetScreenMatrices;
	resetMatrixFunc();

	prevDrawMode = DRAW_NONE;
	drawMode = DRAW_SCREEN;
}

void LuaOpenGL::ResetDrawInMiniMapBackground()
{
	if (!safeMode)
		return;

	ResetMiniMapMatrices();
	ResetGLState();
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::SetupScreenMatrices()
{
	const int remScreenSize = globalRendering->screenSizeY - globalRendering->winSizeY; // remaining desktop size (ssy >= wsy)
	const int bottomWinCoor = remScreenSize - globalRendering->winPosY; // *bottom*-left origin

	const float vpx  = float(globalRendering->viewPosX + globalRendering->winPosX);
	const float vpy  = float(globalRendering->viewPosY + bottomWinCoor);
	const float vsx  = float(globalRendering->viewSizeX); // same as winSizeX except in dual-screen mode
	const float vsy  = float(globalRendering->viewSizeY); // same as winSizeY
	const float ssx  = float(globalRendering->screenSizeX);
	const float ssy  = float(globalRendering->screenSizeY);
	const float hssx = 0.5f * ssx;
	const float hssy = 0.5f * ssy;

	const float zplane = screenDistance * (ssx / screenWidth);
	const float znear  = zplane * 0.5;
	const float zfar   = zplane * 2.0;
	const float factor = (zplane / znear);
	const float left   = (vpx - hssx) / factor;
	const float bottom = (vpy - hssy) / factor;
	const float right  = ((vpx + vsx) - hssx) / factor;
	const float top    = ((vpy + vsy) - hssy) / factor;

	GL::MatrixMode(GL_PROJECTION);
	GL::LoadMatrix(CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl) * CMatrix44f::PerspProj(left, right, bottom, top, znear, zfar));

	GL::MatrixMode(GL_MODELVIEW);
	GL::LoadIdentity();
	// translate s.t. (0,0,0) is on the zplane, on the window's bottom left corner
	GL::Translate(left * factor, bottom * factor, -zplane);
}

void LuaOpenGL::RevertScreenMatrices()
{
	GL::MatrixMode(GL_TEXTURE);
	GL::LoadIdentity();

	glSpringMatrix2dSetupPV(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
}


/******************************************************************************/
/******************************************************************************/

void LuaOpenGL::ResetGenesisMatrices()
{
	GL::MatrixMode(GL_TEXTURE   ); GL::LoadIdentity();
	GL::MatrixMode(GL_PROJECTION); GL::LoadIdentity();
	GL::MatrixMode(GL_MODELVIEW ); GL::LoadIdentity();
}


void LuaOpenGL::ResetWorldMatrices()
{
	GL::MatrixMode(GL_TEXTURE   ); GL::LoadIdentity();
	GL::MatrixMode(GL_PROJECTION); GL::LoadMatrix(camera->GetProjectionMatrix());
	GL::MatrixMode(GL_MODELVIEW ); GL::LoadMatrix(camera->GetViewMatrix());
}

void LuaOpenGL::ResetWorldShadowMatrices()
{
	GL::MatrixMode(GL_TEXTURE   ); GL::LoadIdentity();
	GL::MatrixMode(GL_PROJECTION); GL::LoadMatrix(shadowHandler.GetShadowProjMatrix());
	GL::MatrixMode(GL_MODELVIEW ); GL::LoadMatrix(shadowHandler.GetShadowViewMatrix());
}


void LuaOpenGL::ResetScreenMatrices()
{
	GL::MatrixMode(GL_TEXTURE   ); GL::LoadIdentity();
	GL::MatrixMode(GL_PROJECTION); GL::LoadIdentity();
	GL::MatrixMode(GL_MODELVIEW ); GL::LoadIdentity();

	SetupScreenMatrices();
}


void LuaOpenGL::ResetMiniMapMatrices()
{
	assert(minimap != nullptr);
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
	lua_pushboolean(L, glewIsSupported(luaL_checkstring(L, 1)));
	return 1;
}


int LuaOpenGL::GetNumber(lua_State* L)
{
	const GLenum pname = (GLenum) luaL_checknumber(L, 1);
	const GLuint count = (GLuint) luaL_optnumber(L, 2, 1);

	if (count > 64)
		return 0;

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
//	CheckDrawingEnabled(L, __func__);
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
	if (minimap == nullptr)
		return 0;

//	CheckDrawingEnabled(L, __func__);
	minimap->SetSlaveMode(luaL_checkboolean(L, 1));
	return 0;
}


int LuaOpenGL::ConfigMiniMap(lua_State* L)
{
	if (minimap == nullptr)
		return 0;

	const int px = luaL_checkint(L, 1);
	const int py = luaL_checkint(L, 2);
	const int sx = luaL_checkint(L, 3);
	const int sy = luaL_checkint(L, 4);

	minimap->SetGeometry(px, py, sx, sy);
	return 0;
}


int LuaOpenGL::DrawMiniMap(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	if (minimap == nullptr)
		return 0;

	if (drawMode != DRAW_SCREEN)
		luaL_error(L, "gl.DrawMiniMap() can only be used within DrawScreenItems()");

	if (!minimap->GetSlaveMode())
		luaL_error(L, "gl.DrawMiniMap() can only be used if the minimap is in slave mode");

	if (luaL_optboolean(L, 1, true)) {
		GL::PushMatrix();
		GL::Scale(globalRendering->viewSizeX, globalRendering->viewSizeY, 1.0f);

		minimap->DrawForReal(true, false, true);

		GL::PopMatrix();
	} else {
		minimap->DrawForReal(false, false, true);
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
	CheckDrawingEnabled(L, __func__);
	font->BeginGL4();
	return 0;
}

int LuaOpenGL::EndText(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	font->EndGL4();
	return 0;
}

int LuaOpenGL::DrawBufferedText(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	font->DrawBufferedGL4();
	return 0;
}


int LuaOpenGL::Text(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const std::string& text = luaL_checksstring(L, 1);

	const float xpos = luaL_checkfloat(L, 2);
	const float ypos = luaL_checkfloat(L, 3);
	const float size = luaL_optnumber(L, 4, 12.0f);

	unsigned int options = FONT_NEAREST;

	bool outline = false;
	bool lightOut = false;

	if ((lua_gettop(L) >= 5) && lua_isstring(L, 5)) {
		const char* c = lua_tostring(L, 5);

		while (*c != 0) {
	  		switch (*c) {
				case 'c': { options |= FONT_CENTER;    } break;
				case 'r': { options |= FONT_RIGHT;     } break;

				case 'a': { options |= FONT_ASCENDER;  } break;
				case 't': { options |= FONT_TOP;       } break;
				case 'v': { options |= FONT_VCENTER;   } break;
				case 'x': { options |= FONT_BASELINE;  } break;
				case 'b': { options |= FONT_BOTTOM;    } break;
				case 'd': { options |= FONT_DESCENDER; } break;

				case 's': { options |= FONT_SHADOW;    } break;
				case 'o': { options |= FONT_OUTLINE;   } break;
				case 'O': { options |= FONT_OUTLINE;   } break;

				case 'n': { options ^= FONT_NEAREST;   } break;
				case 'B': { options |= FONT_BUFFERED;  } break; // for DrawBufferedText
				default: break;
			}

			lightOut |= (*(c++) == 'O');
		}

		outline = ((options & FONT_OUTLINE) != 0);
	}

	if (outline) {
		if (lightOut) {
			font->SetOutlineColor(0.95f, 0.95f, 0.95f, 0.8f);
		} else {
			font->SetOutlineColor(0.15f, 0.15f, 0.15f, 0.8f);
		}
	}

	font->glPrint(xpos, ypos, size, options, text);
	return 0;
}


int LuaOpenGL::GetTextWidth(lua_State* L)
{
	const std::string& text = luaL_checksstring(L, 1);
	const float width = font->GetTextWidth(text);
	lua_pushnumber(L, width);
	return 1;
}

int LuaOpenGL::GetTextHeight(lua_State* L)
{
	const std::string& text = luaL_checksstring(L, 1);
	float descender;
	int lines;

	lua_pushnumber(L, font->GetTextHeight(text, &descender, &lines));
	lua_pushnumber(L, descender);
	lua_pushnumber(L, lines);
	return 3;
}


/******************************************************************************/

static void GLObjectPiece(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return;

	const LocalModel* lm = &obj->localModel;
	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, obj, 2);

	if (lmp == nullptr)
		return;

	lm->DrawPiece(lmp);
}

static void GLObjectPieceMultMatrix(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, obj, 2);

	if (lmp == nullptr)
		return;

	GL::MultMatrix(lmp->GetModelSpaceMatrix());
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
		CUnitDrawer::DrawObjectDefOpaque(def, luaL_checkint(L, 2), rawState, toScreen);
	} else {
		CUnitDrawer::DrawObjectDefAlpha(def, luaL_checkint(L, 2), rawState, toScreen);
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
	LuaOpenGL::CheckDrawingEnabled(L, __func__);

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	// NOTE:
	//   the "Raw" in UnitRaw means "no transform", not the same
	//   UnitRaw also skips the DrawUnit callin by default so any
	//   recursion is blocked
	const bool doRawDraw = luaL_optboolean(L, 2, false);
	const bool useLuaMat = GLObjectDrawWithLuaMat(L, unit, LUAOBJ_UNIT);
	const bool noLuaCall = luaL_optboolean(L, 4, !callDrawUnit);
	const bool fullModel = luaL_optboolean(L, 5, true);

	glAttribStatePtr->PushEnableBit();

	typedef void(             *RawDrawMemFunc)(const CUnit*, bool, bool, bool);
	typedef void(CUnitDrawer::*MatDrawMemFunc)(const CUnit*, bool);

	constexpr RawDrawMemFunc rawDrawFuncs[2] = {&CUnitDrawer::DrawUnitLuaTrans, &CUnitDrawer::DrawUnitDefTrans};
	constexpr MatDrawMemFunc matDrawFuncs[2] = {&CUnitDrawer::DrawIndividualLuaTrans, &CUnitDrawer::DrawIndividualDefTrans};

	// "scoped" draw; this prevents any Lua-assigned
	// material(s) from being used by the call below
	if (!useLuaMat)
		(unit->GetLuaMaterialData())->PushLODCount(0);

	if (doRawDraw) {
		// draw with void material state
		unitDrawer->SetDrawerState(DRAWER_STATE_NOP);
		(rawDrawFuncs[applyTransform])(unit, false, noLuaCall, fullModel);
		unitDrawer->SetDrawerState(DRAWER_STATE_SSP);
	} else {
		// draw with full material state
		(unitDrawer->*matDrawFuncs[applyTransform])(unit, noLuaCall);
	}

	if (!useLuaMat)
		(unit->GetLuaMaterialData())->PopLODCount();

	glAttribStatePtr->PopBits();
	return 0;
}

int LuaOpenGL::Unit(lua_State* L) { return (UnitCommon(L, true, true)); }
int LuaOpenGL::UnitRaw(lua_State* L) { return (UnitCommon(L, false, false)); }

int LuaOpenGL::UnitTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectTextures(L, unitHandler.GetUnit(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::UnitShape(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectShape(L, unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::UnitShapeTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectShapeTextures(L, unitDefHandler->GetUnitDefByID(luaL_checkint(L, 1)));
	return 0;
}


int LuaOpenGL::UnitMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	GL::MultMatrix(unit->GetTransformMatrix(false, CLuaHandle::GetHandleFullRead(L)));
	return 0;
}

int LuaOpenGL::UnitPiece(lua_State* L)
{
	GLObjectPiece(L, ParseUnit(L, __func__, 1));
	return 0;
}

int LuaOpenGL::UnitPieceMatrix(lua_State* L) { return (UnitPieceMultMatrix(L)); }
int LuaOpenGL::UnitPieceMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectPieceMultMatrix(L, ParseUnit(L, __func__, 1));
	return 0;
}


/******************************************************************************/

int LuaOpenGL::FeatureCommon(lua_State* L, bool applyTransform, bool callDrawFeature)
{
	LuaOpenGL::CheckDrawingEnabled(L, __func__);

	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	switch (feature->def->drawType) {
		case DRAWTYPE_NONE: {
			return 0;
		} break;
		case DRAWTYPE_MODEL: {
			assert(feature->model != nullptr);
		}; break;
		default: {
			// default tree; re-interpret arguments to control state setup and reset
			treeDrawer->DrawTree(feature, luaL_optboolean(L, 2, false), luaL_optboolean(L, 3, false));
			return 0;
		} break;
	}

	// NOTE:
	//   the "Raw" in FeatureRaw means "no transform", not the same
	//   FeatureRaw also skips the DrawFeature callin by default so
	//   any recursion is blocked
	const bool doRawDraw = luaL_optboolean(L, 2, false);
	const bool useLuaMat = GLObjectDrawWithLuaMat(L, feature, LUAOBJ_FEATURE);
	const bool noLuaCall = luaL_optboolean(L, 4, !callDrawFeature);

	glAttribStatePtr->PushEnableBit();

	typedef void(                *RawDrawMemFunc)(const CFeature*, bool, bool);
	typedef void(CFeatureDrawer::*MatDrawMemFunc)(const CFeature*, bool);

	constexpr RawDrawMemFunc rawDrawFuncs[2] = {&CFeatureDrawer::DrawFeatureLuaTrans, &CFeatureDrawer::DrawFeatureDefTrans};
	constexpr MatDrawMemFunc matDrawFuncs[2] = {&CFeatureDrawer::DrawIndividualLuaTrans, &CFeatureDrawer::DrawIndividualDefTrans};

	// "scoped" draw; this prevents any Lua-assigned
	// material(s) from being used by the call below
	if (!useLuaMat)
		(feature->GetLuaMaterialData())->PushLODCount(0);

	if (doRawDraw) {
		// draw with void material state
		unitDrawer->SetDrawerState(DRAWER_STATE_NOP);
		(rawDrawFuncs[applyTransform])(feature, false, noLuaCall);
		unitDrawer->SetDrawerState(DRAWER_STATE_SSP);
	} else {
		// draw with full material state
		(featureDrawer->*matDrawFuncs[applyTransform])(feature, noLuaCall);
	}

	if (!useLuaMat)
		(feature->GetLuaMaterialData())->PopLODCount();

	glAttribStatePtr->PopBits();
	return 0;
}

int LuaOpenGL::Feature(lua_State* L) { return (FeatureCommon(L, true, true)); }
int LuaOpenGL::FeatureRaw(lua_State* L) { return (FeatureCommon(L, false, false)); }

int LuaOpenGL::FeatureTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectTextures(L, featureHandler.GetFeature(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::FeatureShape(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectShape(L, featureDefHandler->GetFeatureDefByID(luaL_checkint(L, 1)));
	return 0;
}

int LuaOpenGL::FeatureShapeTextures(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectShapeTextures(L, featureDefHandler->GetFeatureDefByID(luaL_checkint(L, 1)));
	return 0;
}


int LuaOpenGL::FeatureMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	GL::MultMatrix(feature->GetTransformMatrixRef());
	return 0;
}

int LuaOpenGL::FeaturePiece(lua_State* L)
{
	GLObjectPiece(L, ParseFeature(L, __func__, 1));
	return 0;
}


int LuaOpenGL::FeaturePieceMatrix(lua_State* L) { return (FeaturePieceMultMatrix(L)); }
int LuaOpenGL::FeaturePieceMultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GLObjectPieceMultMatrix(L, ParseFeature(L, __func__, 1));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaOpenGL::DrawFuncAtUnit(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	// is visible to current read team, is not an icon
	const CUnit* unit = ParseDrawUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	while (CUnit* trans = unit->GetTransporter()) {
		unit = trans;
	}

	const float3 drawPos = (luaL_checkboolean(L, 2))? unit->drawMidPos: unit->drawPos;

	if (!lua_isfunction(L, 3)) {
		luaL_error(L, "Missing function parameter in DrawFuncAtUnit()\n");
		return 0;
	}

	// call the function
	GL::PushMatrix();
	GL::Translate(drawPos.x, drawPos.y, drawPos.z);
	const int error = lua_pcall(L, (lua_gettop(L) - 3), 0, 0);
	GL::PopMatrix();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.DrawFuncAtUnit: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::DrawGroundCircle(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	if ((lua_gettop(L) >= 6) && lua_isnumber(L, 6)) {
		const WeaponDef* wd = weaponDefHandler->GetWeaponDefByID(luaL_optint(L, 8, 0));

		if (wd == nullptr)
			return 0;

		const float  radius = luaL_checkfloat(L, 4);
		const float   slope = luaL_checkfloat(L, 6);
		const float gravity = luaL_optfloat(L, 7, mapInfo->map.gravity);

		const float4 defColor = cmdColors.rangeAttack;
		const float4 argColor = {luaL_optfloat(L, 8, defColor.x), luaL_optfloat(L, 9, defColor.y), luaL_optfloat(L, 10, defColor.z), 1.0f};

		GL::RenderDataBufferC*  buffer = GL::GetRenderBufferC();
		Shader::IProgramObject* shader = (luaL_optboolean(L, 11, true))? buffer->GetShader(): nullptr;

		#if 1
		// if null, expect caller to supply a shader
		if (shader != nullptr) {
			shader->Enable();
			shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
			shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());
		}
		#endif

		glSetupRangeRingDrawState();
		glBallisticCircle(buffer, wd,  luaL_checkint(L, 5), GL_LINE_LOOP,  pos, {radius, slope, gravity}, argColor);
		glResetRangeRingDrawState();

		#if 1
		if (shader != nullptr)
			shader->Disable();
		#endif
	} else {
		glSurfaceCircleVA(GetVertexArray(), {pos, luaL_checkfloat(L, 4)}, {luaL_optfloat(L, 4, 1.0f), luaL_optfloat(L, 5, 1.0f), luaL_optfloat(L, 6, 1.0f), 1.0f}, luaL_checkint(L, 5));
	}

	return 0;
}


int LuaOpenGL::DrawGroundQuad(lua_State* L)
{
	// FIXME: incomplete (esp. texcoord clamping)
	CheckDrawingEnabled(L, __func__);
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
	} else {
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
	if ((xis >= xie) || (zis >= zie))
		return 0;

	if (!useTxcd) {
		#if 0
		GL::RenderDataBuffer0* rdb = GL::GetRenderBuffer0();
		Shader::IProgramObject* ipo = rdb->GetShader();

		ipo->Enable();
		ipo->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
		ipo->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());

		for (int xib = xis; xib < xie; xib++) {
			const int xit = xib + 1;
			const float xb = xib * SQUARE_SIZE;
			const float xt = xb + SQUARE_SIZE;

			for (int zi = zis; zi <= zie; zi++) {
				const int ziOff = zi * mapxi;
				const float yb = heightmap[ziOff + xib];
				const float yt = heightmap[ziOff + xit];
				const float z = zi * SQUARE_SIZE;

				rdb->SafeAppend({xt, yt, z});
				rdb->SafeAppend({xb, yb, z});
			}

			rdb->Submit(GL_TRIANGLE_STRIP);
		}

		ipo->Disable();
		#endif



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
	} else {
		const float tuStep = (tu1 - tu0) / float(xie - xis);
		const float tvStep = (tv1 - tv0) / float(zie - zis);

		float tub = tu0;



		#if 0
		GL::RenderDataBufferT* rdb = GL::GetRenderBufferT();
		Shader::IProgramObject* ipo = rdb->GetShader();

		ipo->Enable();
		ipo->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
		ipo->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());

		for (int xib = xis; xib < xie; xib++) {
			const int xit = xib + 1;
			const float xb = xib * SQUARE_SIZE;
			const float xt = xb + SQUARE_SIZE;
			const float tut = tub + tuStep;
			float tv = tv0;

			for (int zi = zis; zi <= zie; zi++) {
				const int ziOff = zi * mapxi;
				const float yb = heightmap[ziOff + xib];
				const float yt = heightmap[ziOff + xit];
				const float z = zi * SQUARE_SIZE;

				rdb->SafeAppend({{xt, yt, z}, tut, tv});
				rdb->SafeAppend({{xb, yb, z}, tub, tv});

				tv += tvStep;
			}

			rdb->Submit(GL_TRIANGLE_STRIP);
		}

		ipo->Disable();
		#endif



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
	CheckDrawingEnabled(L, __func__);

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
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments

	if ((args < 2) || !lua_isfunction(L, 2))
		luaL_error(L, "Incorrect arguments to gl.BeginEnd(type, func, ...)");

	// call the function
	glBegin((GLuint)luaL_checkint(L, 1));
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEnd();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.BeginEnd: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::Vertex(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

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
	CheckDrawingEnabled(L, __func__);

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
	CheckDrawingEnabled(L, __func__);

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


/******************************************************************************/

int LuaOpenGL::Rect(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const float x1 = luaL_checkfloat(L, 1);
	const float y1 = luaL_checkfloat(L, 2);
	const float x2 = luaL_checkfloat(L, 3);
	const float y2 = luaL_checkfloat(L, 4);
	glRectf(x1, y1, x2, y2);
	return 0;
}


int LuaOpenGL::TexRect(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

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
	CheckDrawingEnabled(L, __func__);

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



/******************************************************************************/

int LuaOpenGL::ResetState(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	ResetGLState();
	return 0;
}


int LuaOpenGL::ResetMatrices(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	if (lua_gettop(L) != 0)
		luaL_error(L, "gl.ResetMatrices takes no arguments");

	resetMatrixFunc();
	return 0;
}


int LuaOpenGL::Scissor(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
			glAttribStatePtr->EnableScissorTest();
		} else {
			glAttribStatePtr->DisableScissorTest();
		}
	}
	else if (args == 4) {
		glAttribStatePtr->EnableScissorTest();
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
	CheckDrawingEnabled(L, __func__);

	const int x = luaL_checkint(L, 1);
	const int y = luaL_checkint(L, 2);
	const int w = luaL_checkint(L, 3);
	const int h = luaL_checkint(L, 4);
	if (w < 0) luaL_argerror(L, 3, "<width> must be greater than or equal zero!");
	if (h < 0) luaL_argerror(L, 4, "<height> must be greater than or equal zero!");

	glAttribStatePtr->ViewPort(x, y, w, h);
	return 0;
}


int LuaOpenGL::ColorMask(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
			glAttribStatePtr->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		} else {
			glAttribStatePtr->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}
	}
	else if (args == 4) {
		glAttribStatePtr->ColorMask(luaL_checkboolean(L, 1), luaL_checkboolean(L, 2), luaL_checkboolean(L, 3), luaL_checkboolean(L, 4));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.ColorMask()");
	}
	return 0;
}


int LuaOpenGL::DepthMask(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	if (luaL_checkboolean(L, 1)) {
		glAttribStatePtr->EnableDepthMask();
	} else {
		glAttribStatePtr->DisableDepthMask();
	}
	return 0;
}


int LuaOpenGL::DepthTest(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1)
		luaL_error(L, "Incorrect arguments to gl.DepthTest()");

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glAttribStatePtr->EnableDepthTest();
		} else {
			glAttribStatePtr->DisableDepthTest();
		}
	}
	else if (lua_isnumber(L, 1)) {
		glAttribStatePtr->EnableDepthTest();
		glAttribStatePtr->DepthFunc((GLenum)lua_tonumber(L, 1));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.DepthTest()");
	}
	return 0;
}


int LuaOpenGL::DepthClamp(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	luaL_checktype(L, 1, LUA_TBOOLEAN);
	if (lua_toboolean(L, 1)) {
		glAttribStatePtr->EnableDepthClamp();
	} else {
		glAttribStatePtr->DisableDepthClamp();
	}
	return 0;
}


int LuaOpenGL::Culling(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1)
		luaL_error(L, "Incorrect arguments to gl.Culling()");


	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glAttribStatePtr->EnableCullFace();
		} else {
			glAttribStatePtr->DisableCullFace();
		}
	}
	else if (lua_isnumber(L, 1)) {
		glAttribStatePtr->EnableCullFace();
		glAttribStatePtr->CullFace((GLenum)lua_tonumber(L, 1));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Culling()");
	}
	return 0;
}


int LuaOpenGL::LogicOp(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 1)
		luaL_error(L, "Incorrect arguments to gl.LogicOp()");

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
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (lua_isboolean(L, 1)) {
			if (lua_toboolean(L, 1)) {
				glAttribStatePtr->EnableBlendMask();
			} else {
				glAttribStatePtr->DisableBlendMask();
			}
		}
		else if (lua_israwstring(L, 1)) {
			switch (hashString(lua_tostring(L, 1))) {
				case hashString("add"): {
					glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE);
					glAttribStatePtr->EnableBlendMask();
				} break;
				case hashString("alpha_add"): {
					glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);
					glAttribStatePtr->EnableBlendMask();
				} break;

				case hashString("alpha"):
				case hashString("reset"): {
					glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glAttribStatePtr->EnableBlendMask();
				} break;
				case hashString("color"): {
					glAttribStatePtr->BlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
					glAttribStatePtr->EnableBlendMask();
				} break;
				case hashString("modulate"): {
					glAttribStatePtr->BlendFunc(GL_DST_COLOR, GL_ZERO);
					glAttribStatePtr->EnableBlendMask();
				} break;
				case hashString("disable"): {
					glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glAttribStatePtr->DisableBlendMask();
				} break;
				default: {
				} break;
			}
		}
		else {
			luaL_error(L, "Incorrect argument to gl.Blending()");
		}
	}
	else if (args == 2) {
		const GLenum src = (GLenum)luaL_checkint(L, 1);
		const GLenum dst = (GLenum)luaL_checkint(L, 2);
		glAttribStatePtr->BlendFunc(src, dst);
		glAttribStatePtr->EnableBlendMask();
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.Blending()");
	}
	return 0;
}


int LuaOpenGL::BlendEquation(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum mode = (GLenum)luaL_checkint(L, 1);
	glBlendEquation(mode);
	return 0;
}


int LuaOpenGL::BlendFunc(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum src = (GLenum)luaL_checkint(L, 1);
	const GLenum dst = (GLenum)luaL_checkint(L, 2);
	glAttribStatePtr->BlendFunc(src, dst);
	return 0;
}


int LuaOpenGL::BlendEquationSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum modeRGB   = (GLenum)luaL_checkint(L, 1);
	const GLenum modeAlpha = (GLenum)luaL_checkint(L, 2);
	glBlendEquationSeparate(modeRGB, modeAlpha);
	return 0;
}


int LuaOpenGL::BlendFuncSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum srcRGB   = (GLenum)luaL_checkint(L, 1);
	const GLenum dstRGB   = (GLenum)luaL_checkint(L, 2);
	const GLenum srcAlpha = (GLenum)luaL_checkint(L, 3);
	const GLenum dstAlpha = (GLenum)luaL_checkint(L, 4);
	glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
	return 0;
}



int LuaOpenGL::AlphaTest(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
			glAttribStatePtr->EnableAlphaTest();
		} else {
			glAttribStatePtr->DisableAlphaTest();
		}
	}
	else if (args == 2) {
		glAttribStatePtr->EnableAlphaTest();
		glAttribStatePtr->AlphaFunc((GLenum)luaL_checkint(L, 1), (GLfloat)luaL_checkint(L, 2));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.AlphaTest()");
	}
	return 0;
}


int LuaOpenGL::PolygonMode(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum face = (GLenum)luaL_checkint(L, 1);
	const GLenum mode = (GLenum)luaL_checkint(L, 2);
	glAttribStatePtr->PolygonMode(face, mode);
	return 0;
}


int LuaOpenGL::PolygonOffset(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (luaL_checkboolean(L, 1)) {
			glAttribStatePtr->PolygonOffsetFill(GL_TRUE);
			glAttribStatePtr->PolygonOffsetLine(GL_TRUE);
			glAttribStatePtr->PolygonOffsetPoint(GL_TRUE);
		} else {
			glAttribStatePtr->PolygonOffsetFill(GL_FALSE);
			glAttribStatePtr->PolygonOffsetLine(GL_FALSE);
			glAttribStatePtr->PolygonOffsetPoint(GL_FALSE);
		}
	}
	else if (args == 2) {
		glAttribStatePtr->PolygonOffsetFill(GL_TRUE);
		glAttribStatePtr->PolygonOffsetLine(GL_TRUE);
		glAttribStatePtr->PolygonOffsetPoint(GL_TRUE);
		glAttribStatePtr->PolygonOffset((GLfloat)luaL_checkfloat(L, 1), (GLfloat)luaL_checkfloat(L, 2));
	}
	else {
		luaL_error(L, "Incorrect arguments to gl.PolygonOffset()");
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::StencilTest(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	luaL_checktype(L, 1, LUA_TBOOLEAN);
	if (lua_toboolean(L, 1)) {
		glAttribStatePtr->EnableStencilTest();
	} else {
		glAttribStatePtr->DisableStencilTest();
	}
	return 0;
}


int LuaOpenGL::StencilMask(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLuint mask = luaL_checkint(L, 1);
	glAttribStatePtr->StencilMask(mask);
	return 0;
}


int LuaOpenGL::StencilFunc(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum func = luaL_checkint(L, 1);
	const GLint  ref  = luaL_checkint(L, 2);
	const GLuint mask = luaL_checkint(L, 3);
	glAttribStatePtr->StencilFunc(func, ref, mask);
	return 0;
}


int LuaOpenGL::StencilOp(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum fail  = luaL_checkint(L, 1);
	const GLenum zfail = luaL_checkint(L, 2);
	const GLenum zpass = luaL_checkint(L, 3);
	glAttribStatePtr->StencilOper(fail, zfail, zpass);
	return 0;
}


int LuaOpenGL::StencilMaskSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum face = luaL_checkint(L, 1);
	const GLuint mask = luaL_checkint(L, 2);
	glStencilMaskSeparate(face, mask);
	return 0;
}


int LuaOpenGL::StencilFuncSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum face = luaL_checkint(L, 1);
	const GLenum func = luaL_checkint(L, 2);
	const GLint  ref  = luaL_checkint(L, 3);
	const GLuint mask = luaL_checkint(L, 4);
	glStencilFuncSeparate(face, func, ref, mask);
	return 0;
}


int LuaOpenGL::StencilOpSeparate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
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
	CheckDrawingEnabled(L, __func__);

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
	glAttribStatePtr->LineWidth(width);
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

	CheckDrawingEnabled(L, __func__);

	if (lua_gettop(L) < 1)
		luaL_error(L, "Incorrect [number of] arguments to gl.Texture()");

	GLenum texUnit = GL_TEXTURE0;
	int nextArg = 1;

	if (lua_isnumber(L, 1)) {
		nextArg = 2;
		const int texNum = (GLenum)luaL_checknumber(L, 1);

		if ((texNum < 0) || (texNum >= MAX_TEXTURE_UNITS))
			luaL_error(L, "Bad texture unit given to gl.Texture()");

		texUnit += texNum;

		if (texUnit != GL_TEXTURE0)
			glActiveTexture(texUnit);
	}

	if (lua_isboolean(L, nextArg)) {
		if (lua_toboolean(L, nextArg)) {
			glEnable(GL_TEXTURE_2D);
		} else {
			glDisable(GL_TEXTURE_2D);
		}

		if (texUnit != GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);

		lua_pushboolean(L, true);
		return 1;
	}

	if (!lua_isstring(L, nextArg)) {
		if (texUnit != GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);

		luaL_error(L, "Incorrect arguments to gl.Texture()");
	}

	LuaMatTexture tex;

	if (LuaOpenGLUtils::ParseTextureImage(L, tex, lua_tostring(L, nextArg))) {
		lua_pushboolean(L, true);

		tex.enable = true;
		tex.Bind();
	} else {
		lua_pushboolean(L, false);
	}

	if (texUnit != GL_TEXTURE0)
		glActiveTexture(GL_TEXTURE0);

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
	if (name.empty())
		return 0;

	lua_pushsstring(L, name);
	return 1;
}


int LuaOpenGL::DeleteTexture(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

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

	if (!LuaOpenGLUtils::ParseTextureImage(L, tex, luaL_checkstring(L, 1)))
		return 0;

	lua_createtable(L, 0, 2);
	HSTR_PUSH_NUMBER(L, "xsize", tex.GetSize().x);
	HSTR_PUSH_NUMBER(L, "ysize", tex.GetSize().y);
	// HSTR_PUSH_BOOL(L,   "alpha", texInfo.alpha);  FIXME
	// HSTR_PUSH_NUMBER(L, "type",  texInfo.type);
	return 1;
}


int LuaOpenGL::CopyToTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const std::string& texture = luaL_checkstring(L, 1);

	if (texture[0] != LuaTextures::prefix) // '!'
		luaL_error(L, "gl.CopyToTexture() can only write to lua textures");

	const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* tex = textures.GetInfo(texture);

	if (tex == nullptr)
		return 0;

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
	CheckDrawingEnabled(L, __func__);

	const std::string& texture = luaL_checkstring(L, 1);

	if (texture[0] != LuaTextures::prefix) // '!'
		luaL_error(L, "gl.RenderToTexture() can only write to fbo textures");

	if (!lua_isfunction(L, 2))
		luaL_error(L, "Incorrect arguments to gl.RenderToTexture()");

	const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* tex = textures.GetInfo(texture);

	if ((tex == nullptr) || (tex->fbo == 0))
		return 0;

	GLint currentFBO = 0;
	if (drawMode == DRAW_WORLD_SHADOW)
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tex->fbo);

	glAttribStatePtr->PushBits(GL_VIEWPORT_BIT);
	glAttribStatePtr->ViewPort(0, 0, tex->xsize, tex->ysize);
	GL::MatrixMode(GL_PROJECTION); GL::PushMatrix(); GL::LoadIdentity();
	GL::MatrixMode(GL_MODELVIEW);  GL::PushMatrix(); GL::LoadIdentity();

	const int error = lua_pcall(L, lua_gettop(L) - 2, 0, 0);

	GL::MatrixMode(GL_PROJECTION); GL::PopMatrix();
	GL::MatrixMode(GL_MODELVIEW);  GL::PopMatrix();
	glAttribStatePtr->PopBits();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);

	if (error != 0) {
		LOG_L(L_ERROR, "gl.RenderToTexture: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaOpenGL::GenerateMipmap(lua_State* L)
{
	//CheckDrawingEnabled(L, __func__);
	const std::string& texStr = luaL_checkstring(L, 1);

	if (texStr[0] != LuaTextures::prefix) // '!'
		return 0;

	const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* tex = textures.GetInfo(texStr);

	if (tex == nullptr)
		return 0;

	GLint currentBinding;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentBinding);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, currentBinding);
	return 0;
}


int LuaOpenGL::ActiveTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments

	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to gl.ActiveTexture(number, func, ...)");
		return 0;
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
		LOG_L(L_ERROR, "gl.ActiveTexture: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


/******************************************************************************/


int LuaOpenGL::Clear(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments

	if ((args < 1) || !lua_isnumber(L, 1))
		luaL_error(L, "Incorrect arguments to gl.Clear()");

	const GLbitfield bits = (GLbitfield)lua_tonumber(L, 1);

	switch (args) {
		case 5: {
			if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5))
				luaL_error(L, "Incorrect arguments to gl.Clear(bits, r, g, b, a)");

			switch (bits) {
				case GL_COLOR_BUFFER_BIT: { glClearColor((GLfloat)lua_tonumber(L, 2), (GLfloat)lua_tonumber(L, 3), (GLfloat)lua_tonumber(L, 4), (GLfloat)lua_tonumber(L, 5)); } break;
				case GL_ACCUM_BUFFER_BIT: { glClearAccum((GLfloat)lua_tonumber(L, 2), (GLfloat)lua_tonumber(L, 3), (GLfloat)lua_tonumber(L, 4), (GLfloat)lua_tonumber(L, 5)); } break;
				default: {} break;
			}
		} break;
		case 2: {
			if (!lua_isnumber(L, 2))
				luaL_error(L, "Incorrect arguments to gl.Clear(bits, val)");

			switch (bits) {
				case GL_DEPTH_BUFFER_BIT: { glClearDepth((GLfloat)lua_tonumber(L, 2)); } break;
				case GL_STENCIL_BUFFER_BIT: { glClearStencil((GLint)lua_tonumber(L, 2)); } break;
				default: {} break;
			}
		} break;
	}

	glClear(bits);
	return 0;
}

int LuaOpenGL::SwapBuffers(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	// only meant for frame-limited LuaMenu's that want identical content in both buffers
	if (!CLuaHandle::GetHandle(L)->PersistOnReload())
		return 0;

	globalRendering->SwapBuffers(true, true);
	return 0;
}


/******************************************************************************/

int LuaOpenGL::Translate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	GL::Translate(x, y, z);
	return 0;
}


int LuaOpenGL::Scale(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	GL::Scale(x, y, z);
	return 0;
}


int LuaOpenGL::Rotate(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const float r = luaL_checkfloat(L, 1);
	const float x = luaL_checkfloat(L, 2);
	const float y = luaL_checkfloat(L, 3);
	const float z = luaL_checkfloat(L, 4);
	GL::Rotate(r, x, y, z);
	return 0;
}


int LuaOpenGL::Ortho(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const float left   = luaL_checknumber(L, 1);
	const float right  = luaL_checknumber(L, 2);
	const float bottom = luaL_checknumber(L, 3);
	const float top    = luaL_checknumber(L, 4);
	const float near   = luaL_checknumber(L, 5);
	const float far    = luaL_checknumber(L, 6);

	GL::MultMatrix(CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl) * CMatrix44f::OrthoProj(left, right, bottom, top, near, far));
	return 0;
}


int LuaOpenGL::Frustum(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const float left   = luaL_checknumber(L, 1);
	const float right  = luaL_checknumber(L, 2);
	const float bottom = luaL_checknumber(L, 3);
	const float top    = luaL_checknumber(L, 4);
	const float near   = luaL_checknumber(L, 5);
	const float far    = luaL_checknumber(L, 6);

	GL::MultMatrix(CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl) * CMatrix44f::PerspProj(left, right, bottom, top, near, far));
	return 0;
}


int LuaOpenGL::Billboard(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	GL::MultMatrix(camera->GetBillBoardMatrix());
	return 0;
}


/******************************************************************************/

int LuaOpenGL::ClipPlane(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

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
	CheckDrawingEnabled(L, __func__);
	GLenum mode = (GLenum)luaL_checkint(L, 1);
	if (!GetLuaContextData(L)->glMatrixTracker.SetMatrixMode(mode))
		luaL_error(L, "Incorrect value to gl.MatrixMode");
	GL::MatrixMode(mode);
	return 0;
}


int LuaOpenGL::LoadIdentity(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	if (lua_gettop(L) != 0)
		luaL_error(L, "gl.LoadIdentity takes no arguments");

	GL::LoadIdentity();
	return 0;
}


int LuaOpenGL::LoadMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int luaType = lua_type(L, 1);
	if (luaType == LUA_TSTRING) {
		const CMatrix44f* matptr = LuaOpenGLUtils::GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			GL::LoadMatrix(*matptr);
		} else {
			luaL_error(L, "Incorrect arguments to gl.LoadMatrix()");
		}
		return 0;
	} else {
		CMatrix44f matrix;
		if (luaType == LUA_TTABLE) {
			if (LuaUtils::ParseFloatArray(L, -1, &matrix.m[0], 16) != 16) {
				luaL_error(L, "gl.LoadMatrix requires all 16 values");
			}
		}
		else {
			for (int i = 1; i <= 16; i++) {
				matrix.m[i - 1] = (GLfloat)luaL_checknumber(L, i);
			}
		}
		GL::LoadMatrix(matrix);
	}
	return 0;
}


int LuaOpenGL::MultMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int luaType = lua_type(L, 1);
	if (luaType == LUA_TSTRING) {
		const CMatrix44f* matptr = LuaOpenGLUtils::GetNamedMatrix(lua_tostring(L, 1));
		if (matptr != NULL) {
			GL::MultMatrix(*matptr);
		} else {
			luaL_error(L, "Incorrect arguments to gl.MultMatrix()");
		}
		return 0;
	} else {
		CMatrix44f matrix;
		if (luaType == LUA_TTABLE) {
			if (LuaUtils::ParseFloatArray(L, -1, &matrix.m[0], 16) != 16) {
				luaL_error(L, "gl.LoadMatrix requires all 16 values");
			}
		}
		else {
			for (int i = 1; i <= 16; i++) {
				matrix.m[i - 1] = (GLfloat)luaL_checknumber(L, i);
			}
		}
		GL::MultMatrix(matrix);
	}
	return 0;
}


int LuaOpenGL::PushMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0)
		luaL_error(L, "gl.PushMatrix takes no arguments");

	if (!GetLuaContextData(L)->glMatrixTracker.PushMatrix())
		luaL_error(L, "Matrix stack overflow");
	GL::PushMatrix();

	return 0;
}


int LuaOpenGL::PopMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int args = lua_gettop(L); // number of arguments
	if (args != 0)
		luaL_error(L, "gl.PopMatrix takes no arguments");

	if (!GetLuaContextData(L)->glMatrixTracker.PopMatrix())
		luaL_error(L, "Matrix stack underflow");
	GL::PopMatrix();

	return 0;
}


int LuaOpenGL::PushPopMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	std::vector<GLenum> matModes;
	int arg;
	for (arg = 1; lua_isnumber(L, arg); arg++) {
		const GLenum mode = (GLenum)lua_tonumber(L, arg);
		matModes.push_back(mode);
	}

	if (!lua_isfunction(L, arg))
		luaL_error(L, "Incorrect arguments to gl.PushPopMatrix()");

	if (arg == 1) {
		GL::PushMatrix();
	} else {
		for (int i = 0; i < (int)matModes.size(); i++) {
			GL::MatrixMode(matModes[i]);
			GL::PushMatrix();
		}
	}

	const int args = lua_gettop(L); // number of arguments
	const int error = lua_pcall(L, (args - arg), 0, 0);

	if (arg == 1) {
		GL::PopMatrix();
	} else {
		for (int i = 0; i < (int)matModes.size(); i++) {
			GL::MatrixMode(matModes[i]);
			GL::PopMatrix();
		}
	}

	if (error != 0) {
		LOG_L(L_ERROR, "gl.PushPopMatrix: error(%i) = %s", error, lua_tostring(L, -1));
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

			if ((index < 0) || (index >= 16))
				return 0;

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

		if (matptr == nullptr)
			luaL_error(L, "Incorrect arguments to gl.GetMatrixData(name)");

		if (lua_isnumber(L, 2)) {
			const int index = lua_toint(L, 2);

			if ((index < 0) || (index >= 16))
				return 0;

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
	CheckDrawingEnabled(L, __func__);
	int mask = luaL_optnumber(L, 1, GL_ALL_ATTRIB_BITS);
	if (mask < 0) {
		mask = -mask;
		mask |= GL_ALL_ATTRIB_BITS;
	}
	glAttribStatePtr->PushBits((GLbitfield)mask);
	return 0;
}


int LuaOpenGL::PopAttrib(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	glAttribStatePtr->PopBits();
	return 0;
}


int LuaOpenGL::UnsafeState(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	const GLenum state = (GLenum)luaL_checkint(L, 1);
	int funcLoc = 2;
	bool reverse = false;
	if (lua_isboolean(L, 2)) {
		funcLoc++;
		reverse = lua_toboolean(L, 2);
	}
	if (!lua_isfunction(L, funcLoc))
		luaL_error(L, "expecting a function");

	reverse ? glDisable(state) : glAttribStatePtr->Enable(state);
	const int error = lua_pcall(L, lua_gettop(L) - funcLoc, 0, 0);
	reverse ? glAttribStatePtr->Enable(state) : glAttribStatePtr->Disable(state);

	if (error != 0) {
		LOG_L(L_ERROR, "gl.UnsafeState: error(%i) = %s", error, lua_tostring(L, -1));
		lua_pushnumber(L, 0);
	}
	return 0;
}


/******************************************************************************/

int LuaOpenGL::Flush(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	glFlush();
	return 0;
}

int LuaOpenGL::Finish(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
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
		lua_pushnumber(L, *(data++));
	} else {
		lua_createtable(L, fSize, 0);
		for (int e = 1; e <= fSize; e++) {
			lua_pushnumber(L, e);
			lua_pushnumber(L, *(data++));
			lua_rawset(L, -3);
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
	if ((w <= 0) || (h <= 0))
		return 0;

	int fSize = PixelFormatSize(format);
	if (fSize < 0)
		fSize = 4; // good enough?

	CBitmap bmp;
	bmp.Alloc(w, h, fSize * sizeof(float));
	glReadPixels(x, y, w, h, format, GL_FLOAT, reinterpret_cast<float*>(bmp.GetRawMem()));

	int retCount = 0;

	const float* data = reinterpret_cast<const float*>(bmp.GetRawMem());
	const float* d = data;

	if ((w == 1) && (h == 1)) {
		// single pixel
		for (int e = 0; e < fSize; e++) {
			lua_pushnumber(L, data[e]);
		}
		return fSize;
	}

	if ((w == 1) && (h > 1)) {
		lua_createtable(L, h, 0);
		// single column
		for (int i = 1; i <= h; i++) {
			lua_pushnumber(L, i);
			PushPixelData(L, fSize, d);
			lua_rawset(L, -3);
		}

		return 1;
	}

	if ((w > 1) && (h == 1)) {
		lua_createtable(L, w, 0);
		// single row
		for (int i = 1; i <= w; i++) {
			lua_pushnumber(L, i);
			PushPixelData(L, fSize, d);
			lua_rawset(L, -3);
		}

		return 1;
	}

	lua_createtable(L, w, 0);
	for (int x = 1; x <= w; x++) {
		lua_pushnumber(L, x);
		lua_createtable(L, h, 0);
		for (int y = 1; y <= h; y++) {
			lua_pushnumber(L, y);
			PushPixelData(L, fSize, d);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	return 1;
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
		glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetRawMem());
		if (!yflip)
			bitmap.ReverseYAxis();
		lua_pushboolean(L, bitmap.Save(filename, !alpha));
	} else {
		// single channel only!
		glReadPixels(x, y, width, height, GL_LUMINANCE, GL_FLOAT, bitmap.GetRawMem());
		if (!yflip)
			bitmap.ReverseYAxis();
		lua_pushboolean(L, bitmap.SaveFloat(filename));
	}

	return 1;
}


/******************************************************************************/

int LuaOpenGL::CreateQuery(lua_State* L)
{
	GLuint id;
	glGenQueries(1, &id);

	if (id == 0)
		return 0;

	OcclusionQuery* qry = static_cast<OcclusionQuery*>(lua_newuserdata(L, sizeof(OcclusionQuery)));

	if (qry == nullptr)
		return 0;

	*qry = {static_cast<unsigned int>(occlusionQueries.size()), id};
	occlusionQueries.push_back(qry);

	lua_pushlightuserdata(L, reinterpret_cast<void*>(qry));
	return 1;
}

int LuaOpenGL::DeleteQuery(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "gl.DeleteQuery(q) expects a userdata query");

	const OcclusionQuery* qry = static_cast<const OcclusionQuery*>(lua_touserdata(L, 1));

	if (qry == nullptr)
		return 0;
	if (qry->index >= occlusionQueries.size())
		return 0;

	glDeleteQueries(1, &qry->id);

	occlusionQueries[qry->index] = occlusionQueries.back();
	occlusionQueries[qry->index]->index = qry->index;
	occlusionQueries.pop_back();
	return 0;
}

int LuaOpenGL::RunQuery(lua_State* L)
{
	static bool running = false;

	if (running)
		luaL_error(L, "gl.RunQuery(q,f) can not be called recursively");

	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "gl.RunQuery(q,f) expects a userdata query");

	const OcclusionQuery* qry = static_cast<const OcclusionQuery*>(lua_touserdata(L, 1));

	if (qry == nullptr)
		return 0;
	if (qry->index >= occlusionQueries.size())
		return 0;

	if (!lua_isfunction(L, 2))
		luaL_error(L, "gl.RunQuery(q,f) expects a function");

	const int args = lua_gettop(L); // number of arguments

	running = true;
	glBeginQuery(GL_SAMPLES_PASSED, qry->id);
	const int error = lua_pcall(L, (args - 2), 0, 0);
	glEndQuery(GL_SAMPLES_PASSED);
	running = false;

	if (error != 0) {
		LOG_L(L_ERROR, "gl.RunQuery: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}

int LuaOpenGL::GetQuery(lua_State* L)
{
	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "gl.GetQuery(q) expects a userdata query");

	const OcclusionQuery* qry = static_cast<const OcclusionQuery*>(lua_touserdata(L, 1));

	if (qry == nullptr)
		return 0;
	if (qry->index >= occlusionQueries.size())
		return 0;

	GLuint count;
	glGetQueryObjectuiv(qry->id, GL_QUERY_RESULT, &count);

	lua_pushnumber(L, count);
	return 1;
}


/******************************************************************************/

int LuaOpenGL::GetGlobalTexNames(lua_State* L)
{
	const auto& textures = textureHandler3DO.GetAtlasTextures();

	lua_createtable(L, textures.size(), 0);
	int count = 1;
	for (auto it = textures.begin(); it != textures.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_rawseti(L, -2, count++);
	}
	return 1;
}


int LuaOpenGL::GetGlobalTexCoords(lua_State* L)
{
	const C3DOTextureHandler::UnitTexture* texCoords = textureHandler3DO.Get3DOTexture(luaL_checkstring(L, 1));

	if (texCoords == nullptr)
		return 0;

	lua_pushnumber(L, texCoords->xstart);
	lua_pushnumber(L, texCoords->ystart);
	lua_pushnumber(L, texCoords->xend);
	lua_pushnumber(L, texCoords->yend);
	return 4;
}


int LuaOpenGL::GetShadowMapParams(lua_State* L)
{
	lua_pushnumber(L, shadowHandler.GetShadowParams().x);
	lua_pushnumber(L, shadowHandler.GetShadowParams().y);
	lua_pushnumber(L, shadowHandler.GetShadowParams().z);
	lua_pushnumber(L, shadowHandler.GetShadowParams().w);
	return 4;
}

int LuaOpenGL::GetAtmosphere(lua_State* L)
{
	if (lua_gettop(L) == 0) {
		lua_pushnumber(L, sky->GetLight()->GetLightDir().x);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().y);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().z);
		return 3;
	}

	const char* param = luaL_checkstring(L, 1);
	const float* data = nullptr;

	switch (hashString(param)) {
		// float
		case hashString("fogStart"): {
			lua_pushnumber(L, sky->fogStart);
			return 1;
		} break;
		case hashString("fogEnd"): {
			lua_pushnumber(L, sky->fogEnd);
			return 1;
		} break;

		// float3
		case hashString("pos"): {
			data = &sky->GetLight()->GetLightDir().x;
		} break;
		case hashString("fogColor"): {
			data = &sky->fogColor.x;
		} break;
		case hashString("skyColor"): {
			data = &sky->skyColor.x;
		} break;
		case hashString("skyDir"): {
			// data = &sky->sunColor.x;
		} break;
		case hashString("sunColor"): {
			data = &sky->sunColor.x;
		} break;
		case hashString("cloudColor"): {
			data = &sky->cloudColor.x;
		} break;
	}

	if (data != nullptr) {
		lua_pushnumber(L, data[0]);
		lua_pushnumber(L, data[1]);
		lua_pushnumber(L, data[2]);
		return 3;
	}

	return 0;
}

int LuaOpenGL::GetSun(lua_State* L)
{
	if (lua_gettop(L) == 0) {
		lua_pushnumber(L, sky->GetLight()->GetLightDir().x);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().y);
		lua_pushnumber(L, sky->GetLight()->GetLightDir().z);
		return 3;
	}

	const char* param = luaL_checkstring(L, 1);
	const float* data = nullptr;

	const bool unitMode = lua_israwstring(L, 2) && (strcmp(lua_tostring(L, 2), "unit") == 0);

	switch (hashString(param)) {
		case hashString("pos"): {
			data = &sky->GetLight()->GetLightDir().x;
		} break;
		case hashString("shadowDensity"): {
			if (!unitMode) {
				lua_pushnumber(L, sunLighting->groundShadowDensity);
			} else {
				lua_pushnumber(L, sunLighting->modelShadowDensity);
			}
			return 1;
		} break;
		case hashString("diffuse"): {
			if (!unitMode) {
				data = &sunLighting->groundDiffuseColor.x;
			} else {
				data = &sunLighting->modelDiffuseColor.x;
			}
		} break;
		case hashString("ambient"): {
			if (!unitMode) {
				data = &sunLighting->groundAmbientColor.x;
			} else {
				data = &sunLighting->modelAmbientColor.x;
			}
		} break;
		case hashString("specular"): {
			if (!unitMode) {
				data = &sunLighting->groundSpecularColor.x;
			} else {
				data = &sunLighting->modelSpecularColor.x;
			}
		} break;
	}

	if (data != nullptr) {
		lua_pushnumber(L, data[0]);
		lua_pushnumber(L, data[1]);
		lua_pushnumber(L, data[2]);
		return 3;
	}

	return 0;
}

int LuaOpenGL::GetWaterRendering(lua_State* L)
{
	const char* key = luaL_checkstring(L, 1);

	switch (hashString(key)) {
		// float3
		case hashString("absorb"): {
			lua_pushnumber(L, waterRendering->absorb[0]);
			lua_pushnumber(L, waterRendering->absorb[1]);
			lua_pushnumber(L, waterRendering->absorb[2]);
			return 3;
		} break;
		case hashString("baseColor"): {
			lua_pushnumber(L, waterRendering->baseColor[0]);
			lua_pushnumber(L, waterRendering->baseColor[1]);
			lua_pushnumber(L, waterRendering->baseColor[2]);
			return 3;
		} break;
		case hashString("minColor"): {
			lua_pushnumber(L, waterRendering->minColor[0]);
			lua_pushnumber(L, waterRendering->minColor[1]);
			lua_pushnumber(L, waterRendering->minColor[2]);
			return 3;
		} break;
		case hashString("surfaceColor"): {
			lua_pushnumber(L, waterRendering->surfaceColor[0]);
			lua_pushnumber(L, waterRendering->surfaceColor[1]);
			lua_pushnumber(L, waterRendering->surfaceColor[2]);
			return 3;
		} break;
		case hashString("diffuseColor"): {
			lua_pushnumber(L, waterRendering->diffuseColor[0]);
			lua_pushnumber(L, waterRendering->diffuseColor[1]);
			lua_pushnumber(L, waterRendering->diffuseColor[2]);
			return 3;
		} break;
		case hashString("specularColor"): {
			lua_pushnumber(L, waterRendering->specularColor[0]);
			lua_pushnumber(L, waterRendering->specularColor[1]);
			lua_pushnumber(L, waterRendering->specularColor[2]);
			return 3;
		} break;
		case hashString("planeColor"): {
			lua_pushnumber(L, waterRendering->planeColor.x);
			lua_pushnumber(L, waterRendering->planeColor.y);
			lua_pushnumber(L, waterRendering->planeColor.z);
			return 3;
		}
		// string
		case hashString("texture"): {
			lua_pushsstring(L, waterRendering->texture);
			return 1;
		} break;
		case hashString("foamTexture"): {
			lua_pushsstring(L, waterRendering->foamTexture);
			return 1;
		} break;
		case hashString("normalTexture"): {
			lua_pushsstring(L, waterRendering->normalTexture);
			return 1;
		}
		// scalar
		case hashString("repeatX"): {
			lua_pushnumber(L, waterRendering->repeatX);
			return 1;
		} break;
		case hashString("repeatY"): {
			lua_pushnumber(L, waterRendering->repeatY);
			return 1;
		} break;
		case hashString("surfaceAlpha"): {
			lua_pushnumber(L, waterRendering->surfaceAlpha);
			return 1;
		} break;
		case hashString("ambientFactor"): {
			lua_pushnumber(L, waterRendering->ambientFactor);
			return 1;
		} break;
		case hashString("diffuseFactor"): {
			lua_pushnumber(L, waterRendering->diffuseFactor);
			return 1;
		} break;
		case hashString("specularFactor"): {
			lua_pushnumber(L, waterRendering->specularFactor);
			return 1;
		} break;
		case hashString("specularPower"): {
			lua_pushnumber(L, waterRendering->specularPower);
			return 1;
		} break;
		case hashString("fresnelMin"): {
			lua_pushnumber(L, waterRendering->fresnelMin);
			return 1;
		} break;
		case hashString("fresnelMax"): {
			lua_pushnumber(L, waterRendering->fresnelMax);
			return 1;
		} break;
		case hashString("fresnelPower"): {
			lua_pushnumber(L, waterRendering->fresnelPower);
			return 1;
		} break;
		case hashString("reflectionDistortion"): {
			lua_pushnumber(L, waterRendering->reflDistortion);
			return 1;
		} break;
		case hashString("blurBase"): {
			lua_pushnumber(L, waterRendering->blurBase);
			return 1;
		} break;
		case hashString("blurExponent"): {
			lua_pushnumber(L, waterRendering->blurExponent);
			return 1;
		} break;
		case hashString("perlinStartFreq"): {
			lua_pushnumber(L, waterRendering->perlinStartFreq);
			return 1;
		} break;
		case hashString("perlinLacunarity"): {
			lua_pushnumber(L, waterRendering->perlinLacunarity);
			return 1;
		} break;
		case hashString("perlinAmplitude"): {
			lua_pushnumber(L, waterRendering->perlinAmplitude);
			return 1;
		} break;
		case hashString("numTiles"): {
			lua_pushnumber(L, waterRendering->numTiles);
			return 1;
		}
		// boolean
		case hashString("shoreWaves"): {
			lua_pushboolean(L, waterRendering->shoreWaves);
			return 1;
		} break;
		case hashString("forceRendering"): {
			lua_pushboolean(L, waterRendering->forceRendering);
			return 1;
		} break;
		case hashString("hasWaterPlane"): {
			lua_pushboolean(L, waterRendering->hasWaterPlane);
			return 1;
		} break;
	}

	luaL_error(L, "[%s] unknown key %s", __func__, key);
	return 0;
}

int LuaOpenGL::GetMapRendering(lua_State* L)
{
	CSMFReadMap* smfMap = (CSMFReadMap*)readMap;
	const char* key = luaL_checkstring(L, 1);

	switch (hashString(key)) {
		// float4
		case hashString("splatTexScales"): {
			lua_pushnumber(L, mapRendering->splatTexScales[0]);
			lua_pushnumber(L, mapRendering->splatTexScales[1]);
			lua_pushnumber(L, mapRendering->splatTexScales[2]);
			lua_pushnumber(L, mapRendering->splatTexScales[3]);
			return 4;
		} break;
		case hashString("splatTexMults"): {
			lua_pushnumber(L, mapRendering->splatTexMults[0]);
			lua_pushnumber(L, mapRendering->splatTexMults[1]);
			lua_pushnumber(L, mapRendering->splatTexMults[2]);
			lua_pushnumber(L, mapRendering->splatTexMults[3]);
			return 4;
		} break;
		// boolean
		case hashString("voidWater"): {
			lua_pushboolean(L, mapRendering->voidWater);
			return 1;
		} break;
		case hashString("voidGround"): {
			lua_pushboolean(L, mapRendering->voidGround);
			return 1;
		} break;
		case hashString("specularLighting"): {
			lua_pushboolean(L, smfMap->GetSpecularTexture() != 0);
			return 1;
		} break;
		case hashString("splatDetailTexture"): {
			lua_pushboolean(L, smfMap->GetSplatDistrTexture() != 0 &&
			                   smfMap->GetSplatDetailTexture() != 0);
			return 1;
		} break;
		case hashString("splatDetailNormalTexture"): {
			lua_pushboolean(L, smfMap->GetSplatDistrTexture() != 0 &&
			                   smfMap->HaveSplatNormalTexture());
			return 1;
		} break;
		case hashString("splatDetailNormalDiffuseAlpha"): {
			lua_pushboolean(L, mapRendering->splatDetailNormalDiffuseAlpha);
			return 1;
		} break;
		case hashString("waterAbsortion"): {
			lua_pushboolean(L, smfMap->HasVisibleWater());
			return 1;
		} break;
		case hashString("skyReflection"): {
			lua_pushboolean(L, smfMap->GetSkyReflectModTexture() != 0);
			return 1;
		} break;
		case hashString("blendNormals"): {
			lua_pushboolean(L, smfMap->GetBlendNormalsTexture() != 0);
			return 1;
		} break;
		case hashString("lightEmission"): {
			lua_pushboolean(L, smfMap->GetLightEmissionTexture() != 0);
			return 1;
		} break;
		case hashString("parallaxMapping"): {
			lua_pushboolean(L, smfMap->GetParallaxHeightTexture() != 0);
			return 1;
		} break;
		case hashString("haveShadows"): {
			lua_pushboolean(L, shadowHandler.ShadowsLoaded());
			return 1;
		} break;
		case hashString("haveInfoTex"): {
			lua_pushboolean(L, infoTextureHandler->IsEnabled());
			return 1;
		} break;
	}

	luaL_error(L, "[%s] unknown key %s", __func__, key);
	return 0;
}

int LuaOpenGL::GetMapShaderUniform(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.GetMapShaderUniform(table)");
	}

	// Store the arguments in a list
	std::array<char[64], 256> keys;
	const int table = lua_gettop(L);
	unsigned int numKeys = 0;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (numKeys >= keys.size())
			break;

		if (!lua_israwstring(L, -1)) {
			LOG_L(L_WARNING, "gl.GetMapShaderUniform: bad uniform name type");
			return 0;
		}

		strncpy(keys[numKeys], lua_tostring(L, -1), sizeof(keys[0]));
		keys[numKeys++][sizeof(keys[0]) - 1] = '\0';
		numKeys++;
	}

	lua_createtable(L, numKeys, 0);
	for (unsigned int count = 0; count < numKeys; count++) {
		switch (hashString(keys[count])) {
			case hashString("diffuseTex"): {
				lua_pushnumber(L, 0);
			} break;
			case hashString("detailTex"): {
				lua_pushnumber(L, 2);
			} break;
			case hashString("shadowTex"): {
				lua_pushnumber(L, 4);
			} break;
			case hashString("normalsTex"): {
				lua_pushnumber(L, 5);
			} break;
			case hashString("specularTex"): {
				lua_pushnumber(L, 6);
			} break;
			case hashString("splatDetailTex"): {
				lua_pushnumber(L, 7);
			} break;
			case hashString("splatDistrTex"): {
				lua_pushnumber(L, 8);
			} break;
			case hashString("skyReflectTex"): {
				lua_pushnumber(L, 9);
			} break;
			case hashString("skyReflectModTex"): {
				lua_pushnumber(L, 10);
			} break;
			case hashString("blendNormalsTex"): {
				lua_pushnumber(L, 11);
			} break;
			case hashString("lightEmissionTex"): {
				lua_pushnumber(L, 12);
			} break;
			case hashString("parallaxHeightTex"): {
				lua_pushnumber(L, 13);
			} break;
			case hashString("infoTex"): {
				lua_pushnumber(L, 14);
			} break;
			case hashString("splatDetailNormalTex1"): {
				lua_pushnumber(L, 15);
			} break;
			case hashString("splatDetailNormalTex2"): {
				lua_pushnumber(L, 16);
			} break;
			case hashString("splatDetailNormalTex3"): {
				lua_pushnumber(L, 17);
			} break;
			case hashString("splatDetailNormalTex4"): {
				lua_pushnumber(L, 18);
			} break;
			case hashString("mapSizePO2"): {
				lua_createtable(L, 2, 0);
				lua_pushnumber(L, mapDims.pwr2mapx * SQUARE_SIZE * 1.0f);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, mapDims.pwr2mapy * SQUARE_SIZE * 1.0f);
				lua_rawseti(L, -2, 2);
			} break;
			case hashString("mapSize"): {
				lua_createtable(L, 2, 0);
				lua_pushnumber(L, mapDims.mapx * SQUARE_SIZE * 1.0f);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, mapDims.mapy * SQUARE_SIZE * 1.0f);
				lua_rawseti(L, -2, 2);
			} break;
			case hashString("lightDir"): {
				lua_createtable(L, 4, 0);
				for (int i = 0; i < 4; i++) {
					lua_pushnumber(L, sky->GetLight()->GetLightDir()[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("cameraPos"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, camera->GetPos()[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("groundAmbientColor"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, sunLighting->groundAmbientColor[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("groundDiffuseColor"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, sunLighting->groundDiffuseColor[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("groundSpecularColor"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, sunLighting->groundSpecularColor[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("groundSpecularExponent"): {
				lua_pushnumber(L, sunLighting->specularExponent);
			} break;
			case hashString("groundShadowDensity"): {
				lua_pushnumber(L, sunLighting->groundShadowDensity);
			} break;
			case hashString("viewMat"): {
				lua_createtable(L, 16, 0);
				for (int i = 0; i < 16; i++) {
					lua_pushnumber(L, camera->GetViewMatrix()[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("viewMatInv"): {
				lua_createtable(L, 16, 0);
				for (int i = 0; i < 16; i++) {
					lua_pushnumber(L, camera->GetViewMatrixInverse()[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("viewProjMat"): {
				lua_createtable(L, 16, 0);
				for (int i = 0; i < 16; i++) {
					lua_pushnumber(L, camera->GetViewProjectionMatrix()[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("shadowMat"): {
				lua_createtable(L, 16, 0);
				for (int i = 0; i < 16; i++) {
					lua_pushnumber(L, shadowHandler.GetShadowViewMatrix()[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("shadowParams"): {
				lua_createtable(L, 4, 0);
				lua_pushnumber(L, shadowHandler.GetShadowParams().x);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, shadowHandler.GetShadowParams().y);
				lua_rawseti(L, -2, 2);
				lua_pushnumber(L, shadowHandler.GetShadowParams().z);
				lua_rawseti(L, -2, 3);
				lua_pushnumber(L, shadowHandler.GetShadowParams().w);
				lua_rawseti(L, -2, 4);
			} break;
			case hashString("fogParams"): {
				lua_createtable(L, 3, 0);
				lua_pushnumber(L, sky->fogStart);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, sky->fogEnd);
				lua_rawseti(L, -2, 2);
				lua_pushnumber(L, globalRendering->viewRange);
				lua_rawseti(L, -2, 3);
			} break;
			case hashString("fogColor"): {
				lua_createtable(L, 4, 0);
				for (int i = 0; i < 4; i++) {
					lua_pushnumber(L, sky->fogColor[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("waterMinColor"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, waterRendering->minColor[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("waterBaseColor"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, waterRendering->baseColor[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("waterAbsorbColor"): {
				lua_createtable(L, 3, 0);
				for (int i = 0; i < 3; i++) {
					lua_pushnumber(L, waterRendering->absorb[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("splatTexScales"): {
				lua_createtable(L, 4, 0);
				for (int i = 0; i < 4; i++) {
					lua_pushnumber(L, mapRendering->splatTexScales[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("splatTexMults"): {
				lua_createtable(L, 4, 0);
				for (int i = 0; i < 4; i++) {
					lua_pushnumber(L, mapRendering->splatTexMults[i]);
					lua_rawseti(L, -2, i + 1);
				}
			} break;
			case hashString("infoTexIntensityMul"): {
				lua_pushnumber(L, float(infoTextureHandler->InMetalMode()) + 1.0f);
			} break;
			case hashString("normalTexGen"): {
				CSMFReadMap* smfMap = (CSMFReadMap*)readMap;
				const int2 normTexSize = smfMap->GetTextureSize(MAP_BASE_NORMALS_TEX);
				lua_createtable(L, 2, 0);
				lua_pushnumber(L, 1.0f / ((normTexSize.x - 1) * SQUARE_SIZE));
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, 1.0f / ((normTexSize.y - 1) * SQUARE_SIZE));
				lua_rawseti(L, -2, 2);
			} break;
			case hashString("specularTexGen"): {
				lua_createtable(L, 2, 0);
				lua_pushnumber(L, 1.0f / (   mapDims.mapx     * SQUARE_SIZE));
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, 1.0f / (   mapDims.mapy     * SQUARE_SIZE));
				lua_rawseti(L, -2, 2);
			} break;
			case hashString("infoTexGen"): {
				lua_createtable(L, 2, 0);
				lua_pushnumber(L, 1.0f / (   mapDims.pwr2mapx * SQUARE_SIZE));
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, 1.0f / (   mapDims.pwr2mapy * SQUARE_SIZE));
				lua_rawseti(L, -2, 2);
			} break;
			case hashString("mapHeights"): {
				lua_createtable(L, 2, 0);
				lua_pushnumber(L, readMap->GetCurrMinHeight());
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, readMap->GetCurrMaxHeight());
				lua_rawseti(L, -2, 2);
			} break;
			default: {
				LOG_L(L_WARNING, "gl.Material: invalid map uniform '%s'", keys[count - 1]);
				lua_pushnil(L);
			} break;
		}

		lua_rawseti(L, -2, count + 1);
	}
    
	return 1;
}

/******************************************************************************/
/******************************************************************************/
