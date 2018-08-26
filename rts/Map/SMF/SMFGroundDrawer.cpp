/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFReadMap.h"
#include "SMFGroundDrawer.h"
#include "SMFGroundTextures.h"
#include "SMFRenderState.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/SMF/Basic/BasicMeshDrawer.h"
#include "Map/SMF/Legacy/LegacyMeshDrawer.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FastMath.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"

static constexpr int MIN_GROUND_DETAIL[] = {                               0,   4};
static constexpr int MAX_GROUND_DETAIL[] = {CBasicMeshDrawer::LOD_LEVELS - 1, 200};

CONFIG(int, GroundDetail)
	.defaultValue(60)
	.headlessValue(0)
	.minimumValue(MIN_GROUND_DETAIL[1])
	.maximumValue(MAX_GROUND_DETAIL[1])
	.description("Controls how detailed the map geometry will be. On lowered settings, cliffs may appear to be jagged or \"melting\".");
CONFIG(bool, MapBorder).defaultValue(true).description("Draws a solid border at the edges of the map.");


CONFIG(int, MaxDynamicMapLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvMapShading).defaultValue(true).safemodeValue(false).description("Enable shaders for terrain rendering.");
CONFIG(bool, AllowDeferredMapRendering).defaultValue(false).safemodeValue(false);


CONFIG(int, ROAM)
	.defaultValue(Patch::VBO)
	.safemodeValue(Patch::DL)
	.minimumValue(0)
	.maximumValue(Patch::VA)
	.description("Use ROAM for terrain mesh rendering: 0 to disable, {1=VBO,2=DL,3=VA}-mode to enable.");


CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* rm)
	: smfMap(rm)
	, meshDrawer(nullptr)
{
	drawerMode = (configHandler->GetInt("ROAM") != 0)? SMF_MESHDRAWER_ROAM: SMF_MESHDRAWER_BASIC;
	groundDetail = configHandler->GetInt("GroundDetail");

	groundTextures = new CSMFGroundTextures(smfMap);
	meshDrawer = SwitchMeshDrawer(drawerMode);

	smfRenderStates.resize(RENDER_STATE_CNT, nullptr);
	smfRenderStates[RENDER_STATE_SSP] = ISMFRenderState::GetInstance(globalRendering->haveARB, globalRendering->haveGLSL, false);
	smfRenderStates[RENDER_STATE_FFP] = ISMFRenderState::GetInstance(                   false,                     false, false);
	smfRenderStates[RENDER_STATE_LUA] = ISMFRenderState::GetInstance(                   false,                      true,  true);

	// LH must be initialized before render-state is initialized
	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicMapLights"));
	geomBuffer.SetName("GROUNDDRAWER-GBUFFER");

	drawForward = true;
	drawDeferred = geomBuffer.Valid();
	drawMapEdges = configHandler->GetBool("MapBorder");


	// NOTE:
	//   advShading can NOT change at runtime if initially false
	//   (see AdvMapShadingActionExecutor), so we will always use
	//   states[FFP] (in ::Draw) in that special case and it does
	//   not matter whether states[SSP] is initialized
	if ((advShading = smfRenderStates[RENDER_STATE_SSP]->Init(this)))
		smfRenderStates[RENDER_STATE_SSP]->Update(this, nullptr);

	// always initialize this state; defer Update (allows re-use)
	smfRenderStates[RENDER_STATE_LUA]->Init(this);

	// note: state must be pre-selected before the first drawn frame
	// Sun*Changed can be called first, e.g. if DynamicSun is enabled
	smfRenderStates[RENDER_STATE_SEL] = SelectRenderState(DrawPass::Normal);



	waterPlaneDispLists[0] = 0;
	waterPlaneDispLists[1] = 0;

	if (waterRendering->hasWaterPlane) {
		glNewList((waterPlaneDispLists[0] = glGenLists(1)), GL_COMPILE);
		CreateWaterPlanes(true);
		glEndList();

		glNewList((waterPlaneDispLists[1] = glGenLists(1)), GL_COMPILE);
		CreateWaterPlanes(false);
		glEndList();
	}

	if (drawDeferred) {
		drawDeferred &= UpdateGeometryBuffer(true);
	}
}

CSMFGroundDrawer::~CSMFGroundDrawer()
{
	// remember which ROAM-mode was enabled (if any)
	configHandler->Set("ROAM", (dynamic_cast<CRoamMeshDrawer*>(meshDrawer) != nullptr)? Patch::GetRenderMode(): 0);

	smfRenderStates[RENDER_STATE_FFP]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_FFP]);
	smfRenderStates[RENDER_STATE_SSP]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_SSP]);
	smfRenderStates[RENDER_STATE_LUA]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_LUA]);
	smfRenderStates.clear();

	spring::SafeDelete(groundTextures);
	spring::SafeDelete(meshDrawer);

	// individually generated, individually deleted
	glDeleteLists(waterPlaneDispLists[0], 1);
	glDeleteLists(waterPlaneDispLists[1], 1);
}



IMeshDrawer* CSMFGroundDrawer::SwitchMeshDrawer(int wantedMode)
{
	// toggle
	if (wantedMode <= -1) {
		wantedMode = drawerMode + 1;
		wantedMode %= SMF_MESHDRAWER_LAST;
	}

	if ((wantedMode == drawerMode) && (meshDrawer != nullptr))
		return meshDrawer;

	spring::SafeDelete(meshDrawer);

	switch ((drawerMode = wantedMode)) {
		case SMF_MESHDRAWER_LEGACY: {
			LOG("Switching to Legacy Mesh Rendering");
			meshDrawer = new CLegacyMeshDrawer(smfMap, this);
		} break;
		case SMF_MESHDRAWER_BASIC: {
			LOG("Switching to Basic Mesh Rendering");
			meshDrawer = new CBasicMeshDrawer(this);
		} break;
		default: {
			LOG("Switching to ROAM Mesh Rendering");
			meshDrawer = new CRoamMeshDrawer(this);
		} break;
	}

	return meshDrawer;
}



void CSMFGroundDrawer::CreateWaterPlanes(bool camOufOfMap) {
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);

	const float xsize = (smfMap->mapSizeX) >> 2;
	const float ysize = (smfMap->mapSizeZ) >> 2;
	const float size = std::min(xsize, ysize);

	CVertexArray* va = GetVertexArray();
	va->Initialize();

	const unsigned char fogColor[4] = {
		(unsigned char)(255 * sky->fogColor[0]),
		(unsigned char)(255 * sky->fogColor[1]),
		(unsigned char)(255 * sky->fogColor[2]),
		 255
	};

	const unsigned char planeColor[4] = {
		(unsigned char)(255 * waterRendering->planeColor[0]),
		(unsigned char)(255 * waterRendering->planeColor[1]),
		(unsigned char)(255 * waterRendering->planeColor[2]),
		 255
	};

	const float alphainc = math::TWOPI / 32;
	float alpha,r1,r2;

	float3 p(0.0f, 0.0f, 0.0f);

	for (int n = (camOufOfMap) ? 0 : 1; n < 4 ; ++n) {
		if ((n == 1) && !camOufOfMap) {
			// don't render vertices under the map
			r1 = 2 * size;
		} else {
			r1 = n*n * size;
		}

		if (n == 3) {
			// last stripe: make it thinner (looks better with fog)
			r2 = (n+0.5)*(n+0.5) * size;
		} else {
			r2 = (n+1)*(n+1) * size;
		}
		for (alpha = 0.0f; (alpha - math::TWOPI) < alphainc ; alpha += alphainc) {
			p.x = r1 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r1 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertexC(p, planeColor );
			p.x = r2 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r2 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertexC(p, (n==3) ? fogColor : planeColor);
		}
	}
	va->DrawArrayC(GL_TRIANGLE_STRIP);

	glDepthMask(GL_TRUE);
}


inline void CSMFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (drawWaterReflection)
		return;

	glPushMatrix();
	glTranslatef(0.0f, std::min(-200.0f, smfMap->GetCurrMinHeight() - 400.0f), 0.0f);
	glCallList(waterPlaneDispLists[camera->GetPos().IsInBounds() && !mapRendering->voidWater]);
	glPopMatrix();
}



ISMFRenderState* CSMFGroundDrawer::SelectRenderState(const DrawPass::e& drawPass)
{
	// [0] := Lua GLSL, must have a valid shader for this pass
	// [1] := default ARB *or* GLSL, same condition
	const unsigned int stateEnums[2] = {RENDER_STATE_LUA, RENDER_STATE_SSP};

	for (unsigned int n = 0; n < 2; n++) {
		ISMFRenderState* state = smfRenderStates[ stateEnums[n] ];

		if (!state->CanEnable(this))
			continue;
		if (!state->HasValidShader(drawPass))
			continue;

		return (smfRenderStates[RENDER_STATE_SEL] = state);
	}

	// fallback
	return (smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_FFP]);
}

bool CSMFGroundDrawer::HaveLuaRenderState() const
{
	return (smfRenderStates[RENDER_STATE_SEL] == smfRenderStates[RENDER_STATE_LUA]);
}



void CSMFGroundDrawer::DrawDeferredPass(const DrawPass::e& drawPass, bool alphaTest)
{
	if (!geomBuffer.Valid())
		return;

	// some water renderers use FBO's for the reflection pass
	if (drawPass == DrawPass::WaterReflection)
		return;
	// some water renderers use FBO's for the refraction pass
	if (drawPass == DrawPass::WaterRefraction)
		return;
	// CubeMapHandler also uses an FBO for this pass
	if (drawPass == DrawPass::TerrainReflection)
		return;

	// deferred pass must be executed with GLSL shaders
	// if the FFP or ARB state was selected, bail early
	if (!SelectRenderState(DrawPass::TerrainDeferred)->CanDrawDeferred())
		return;

	{
		geomBuffer.Bind();
		geomBuffer.SetDepthRange(1.0f, 0.0f);
		geomBuffer.Clear();

		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(DrawPass::TerrainDeferred);
		smfRenderStates[RENDER_STATE_SEL]->Enable(this, DrawPass::TerrainDeferred);

		if (alphaTest) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPreDeferred();

		meshDrawer->DrawMesh(drawPass);

		if (alphaTest) {
			glDisable(GL_ALPHA_TEST);
		}

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(DrawPass::Normal);

		geomBuffer.SetDepthRange(0.0f, 1.0f);
		geomBuffer.UnBind();
	}

	#if 0
	geomBuffer.DrawDebug(geomBuffer.GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX));
	#endif

	// must be after the unbind
	if (!drawForward) {
		eventHandler.DrawGroundPostDeferred();
	}
}

void CSMFGroundDrawer::DrawForwardPass(const DrawPass::e& drawPass, bool alphaTest)
{
	if (!SelectRenderState(drawPass)->CanDrawForward())
		return;

	{
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(drawPass);
		smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass);

		glPushAttrib((GL_ENABLE_BIT * alphaTest) | (GL_POLYGON_BIT * wireframe));

		if (wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		if (alphaTest) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPreForward();

		meshDrawer->DrawMesh(drawPass);

		glPopAttrib();

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(DrawPass::Normal);
	}
}

void CSMFGroundDrawer::Draw(const DrawPass::e& drawPass)
{
	// must be here because water renderers also call us
	if (!globalRendering->drawGround)
		return;
	// if entire map is under voidwater, no need to draw *ground*
	if (readMap->HasOnlyVoidWater())
		return;

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if (drawDeferred) {
		// do the deferred pass first, will allow us to re-use
		// its output at some future point and eventually draw
		// the entire map deferred
		DrawDeferredPass(drawPass, mapRendering->voidGround || (mapRendering->voidWater && drawPass != DrawPass::WaterReflection));
	}

	if (drawForward) {
		DrawForwardPass(drawPass, mapRendering->voidGround || (mapRendering->voidWater && drawPass != DrawPass::WaterReflection));
	}

	glDisable(GL_CULL_FACE);

	if (drawPass == DrawPass::Normal) {
		if (waterRendering->hasWaterPlane) {
			DrawWaterPlane(false);
		}

		if (drawMapEdges) {
			DrawBorder(drawPass);
		}
	}
}


void CSMFGroundDrawer::DrawBorder(const DrawPass::e drawPass)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	ISMFRenderState* prvState = smfRenderStates[RENDER_STATE_SEL];

	smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_FFP];
	// smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());

	//glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f);
	//SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), -0.5f / mapDims.pwr2mapx, -0.5f / mapDims.pwr2mapy);

	static const GLfloat planeX[] = {0.005f, 0.0f, 0.005f, 0.5f};
	static const GLfloat planeZ[] = {0.0f, 0.005f, 0.0f, 0.5f};

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_S, GL_EYE_PLANE, planeX);
	glEnable(GL_TEXTURE_GEN_S);

	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_T, GL_EYE_PLANE, planeZ);
	glEnable(GL_TEXTURE_GEN_T);

	glActiveTexture(GL_TEXTURE3); glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1); glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0); glEnable(GL_TEXTURE_2D); // needed for the non-shader case

	glEnable(GL_BLEND);

		if (wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		/*
		if (mapRendering->voidWater && (drawPass != DrawPass::WaterReflection)) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.9f);
		}
		*/

		meshDrawer->DrawBorderMesh(drawPass);

		/*
		if (mapRendering->voidWater && (drawPass != DrawPass::WaterReflection)) {
			glDisable(GL_ALPHA_TEST);
		}
		*/

		if (wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
	smfRenderStates[RENDER_STATE_SEL] = prvState;

	glDisable(GL_CULL_FACE);
}


void CSMFGroundDrawer::DrawShadowPass()
{
	if (!globalRendering->drawGround)
		return;
	if (readMap->HasOnlyVoidWater())
		return;

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(spPolygonOffsetScale, spPolygonOffsetUnits); // dz*s + r*u

		// also render the border geometry to prevent light-visible backfaces
		po->Enable();
			meshDrawer->DrawMesh(DrawPass::Shadow);
			meshDrawer->DrawBorderMesh(DrawPass::Shadow);
		po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
}



void CSMFGroundDrawer::SetLuaShader(const LuaMapShaderData* luaMapShaderData)
{
	smfRenderStates[RENDER_STATE_LUA]->Update(this, luaMapShaderData);
}

void CSMFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	groundTextures->BindSquareTexture(bigSquareX, bigSquareY);
	smfRenderStates[RENDER_STATE_SEL]->SetSquareTexGen(bigSquareX, bigSquareY);
}



void CSMFGroundDrawer::Update()
{
	if (readMap->HasOnlyVoidWater())
		return;

	groundTextures->DrawUpdate();
	// done by DrawMesh; needs to know the actual draw-pass
	// meshDrawer->Update();

	if (drawDeferred) {
		drawDeferred &= UpdateGeometryBuffer(false);
	}
}

void CSMFGroundDrawer::UpdateRenderState()
{
	smfRenderStates[RENDER_STATE_SSP]->Update(this, nullptr);
}

void CSMFGroundDrawer::SunChanged() {
	// always update, SSMF shader needs current sundir even when shadows are disabled
	// note: only the active state is notified of a given change
	smfRenderStates[RENDER_STATE_SEL]->UpdateCurrentShaderSky(sky->GetLight());
}


bool CSMFGroundDrawer::UpdateGeometryBuffer(bool init)
{
	static const bool drawDeferredAllowed = configHandler->GetBool("AllowDeferredMapRendering");

	if (!drawDeferredAllowed)
		return false;

	return (geomBuffer.Update(init));
}



void CSMFGroundDrawer::SetDetail(int newGroundDetail)
{
	const int minGroundDetail = MIN_GROUND_DETAIL[drawerMode != SMF_MESHDRAWER_BASIC];
	const int maxGroundDetail = MAX_GROUND_DETAIL[drawerMode != SMF_MESHDRAWER_BASIC];

	configHandler->Set("GroundDetail", groundDetail = Clamp(newGroundDetail, minGroundDetail, maxGroundDetail));
	LOG("GroundDetail%s set to %i", ((drawerMode == SMF_MESHDRAWER_BASIC)? "[Bias]": ""), groundDetail);
}



int CSMFGroundDrawer::GetGroundDetail(const DrawPass::e& drawPass) const
{
	int detail = groundDetail;

	switch (drawPass) {
		case DrawPass::TerrainReflection:
			detail *= LODScaleTerrainReflection;
			break;
		case DrawPass::WaterReflection:
			detail *= LODScaleReflection;
			break;
		case DrawPass::WaterRefraction:
			detail *= LODScaleRefraction;
			break;
		case DrawPass::Shadow:
			// TODO:
			//   render a contour mesh for the SP? z-fighting / p-panning occur
			//   when the regular and shadow-mesh tessellations differ too much,
			//   more visible on larger or hillier maps
			//   detail *= LODScaleShadow;
			break;
		default:
			break;
	}

	return detail;
}
