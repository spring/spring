/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFReadMap.h"
#include "SMFGroundDrawer.h"
#include "SMFGroundTextures.h"
#include "SMFRenderState.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/SMF/Legacy/LegacyMeshDrawer.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FastMath.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"

CONFIG(int, GroundDetail).defaultValue(60).headlessValue(0).minimumValue(0).maximumValue(200).description("Controls how detailed the map geometry will be. On lowered settings, cliffs may appear to be jagged or \"melting\".");
CONFIG(bool, MapBorder).defaultValue(true).description("Draws a solid border at the edges of the map.");


CONFIG(int, MaxDynamicMapLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvMapShading).defaultValue(true).safemodeValue(false).description("Enable shaders for terrain rendering and enable so more effects.");
CONFIG(bool, AllowDeferredMapRendering).defaultValue(false).safemodeValue(false);

CONFIG(int, ROAM)
	.defaultValue(Patch::VBO)
	.safemodeValue(Patch::DL)
	.minimumValue(0)
	.maximumValue(Patch::VA)
	.description("Use ROAM for terrain mesh rendering. 0:=disable ROAM, 1=VBO mode, 2=DL mode, 3=VA mode");


CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* rm)
	: smfMap(rm)
	, meshDrawer(NULL)
{
	groundTextures = new CSMFGroundTextures(smfMap);
	meshDrawer = SwitchMeshDrawer((!!configHandler->GetInt("ROAM")) ? SMF_MESHDRAWER_ROAM : SMF_MESHDRAWER_LEGACY);

	smfRenderStates.resize(RENDER_STATE_CNT, nullptr);
	smfRenderStates[RENDER_STATE_SSP] = ISMFRenderState::GetInstance(globalRendering->haveARB, globalRendering->haveGLSL);
	smfRenderStates[RENDER_STATE_FFP] = ISMFRenderState::GetInstance(                   false,                     false);
	smfRenderStates[RENDER_STATE_LUA] = ISMFRenderState::GetInstance(                   false,                      true);

	// also set in ::Draw, but UpdateSunDir can be called
	// first if DynamicSun is enabled --> must be non-NULL
	smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_FFP];

	// LH must be initialized before render-state is initialized
	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicMapLights"));
	geomBuffer.SetName("GROUNDDRAWER-GBUFFER");

	drawForward = true;
	drawDeferred = geomBuffer.Valid();
	drawMapEdges = configHandler->GetBool("MapBorder");

	// NOTE:
	//   advShading can NOT change at runtime if initially false
	//   (see AdvMapShadingActionExecutor), so we will always use
	//   FFP render-state (in ::Draw) in that special case and it
	//   does not matter whether SSP render-state is initialized
	groundDetail = configHandler->GetInt("GroundDetail");
	advShading = smfRenderStates[RENDER_STATE_SSP]->Init(this, nullptr);

	waterPlaneDispLists[0] = 0;
	waterPlaneDispLists[1] = 0;

	if (mapInfo->water.hasWaterPlane) {
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
	// if ROAM _was_ enabled, the configvar is written in CRoamMeshDrawer's dtor
	if (dynamic_cast<CRoamMeshDrawer*>(meshDrawer) == NULL)
		configHandler->Set("ROAM", 0);
	configHandler->Set("GroundDetail", groundDetail);

	smfRenderStates[RENDER_STATE_FFP]->Kill(false); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_FFP]);
	smfRenderStates[RENDER_STATE_SSP]->Kill(false); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_SSP]);
	smfRenderStates[RENDER_STATE_LUA]->Kill( true); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_LUA]);
	smfRenderStates.clear();

	SafeDelete(groundTextures);
	SafeDelete(meshDrawer);

	glDeleteLists(waterPlaneDispLists[0], 2);
}



IMeshDrawer* CSMFGroundDrawer::SwitchMeshDrawer(int mode)
{
	const int curMode = (dynamic_cast<CRoamMeshDrawer*>(meshDrawer) ? SMF_MESHDRAWER_ROAM : SMF_MESHDRAWER_LEGACY);

	// mode == -1: toggle modes
	if (mode < 0) {
		mode = curMode + 1;
		mode %= SMF_MESHDRAWER_LAST;
	}

	if ((curMode == mode) && (meshDrawer != NULL))
		return meshDrawer;

	delete meshDrawer;

	switch (mode) {
		case SMF_MESHDRAWER_LEGACY:
			LOG("Switching to Legacy Mesh Rendering");
			meshDrawer = new CLegacyMeshDrawer(smfMap, this);
			break;
		default:
			LOG("Switching to ROAM Mesh Rendering");
			meshDrawer = new CRoamMeshDrawer(smfMap, this);
			break;
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
		(unsigned char)(255 * mapInfo->atmosphere.fogColor[0]),
		(unsigned char)(255 * mapInfo->atmosphere.fogColor[1]),
		(unsigned char)(255 * mapInfo->atmosphere.fogColor[2]),
		 255
	};

	const unsigned char planeColor[4] = {
		(unsigned char)(255 * mapInfo->water.planeColor[0]),
		(unsigned char)(255 * mapInfo->water.planeColor[1]),
		(unsigned char)(255 * mapInfo->water.planeColor[2]),
		 255
	};

	const float alphainc = fastmath::PI2 / 32;
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
		for (alpha = 0.0f; (alpha - fastmath::PI2) < alphainc ; alpha += alphainc) {
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
	glTranslatef(0.f, std::min(-200.0f, smfMap->GetCurrMinHeight() - 400.0f), 0.f);
	glCallList(waterPlaneDispLists[camera->GetPos().IsInBounds() && !mapInfo->map.voidWater]);
	glPopMatrix();
}



ISMFRenderState* CSMFGroundDrawer::SelectRenderState(const DrawPass::e& drawPass)
{
	ISMFRenderState* state = nullptr;

	// first, pick the state we want
	switch (int(smfRenderStates[RENDER_STATE_LUA]->HasValidShader(drawPass))) {
		case 1: { state = smfRenderStates[RENDER_STATE_LUA]; } break;
		case 0: { state = smfRenderStates[RENDER_STATE_SSP]; } break;
		default: {} break;
	}

	// second, check if it can be used
	// note: LUA reverts to FFP, not SSP
	switch (int(state->CanEnable(this))) {
		case 1: { smfRenderStates[RENDER_STATE_SEL] =                             state; } break;
		case 0: { smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_FFP]; } break;
		default: {} break;
	}

	return smfRenderStates[RENDER_STATE_SEL];
}

bool CSMFGroundDrawer::HaveLuaRenderState() const
{
	return (smfRenderStates[RENDER_STATE_SEL] == smfRenderStates[RENDER_STATE_LUA]);
}

void CSMFGroundDrawer::DrawDeferredPass(const DrawPass::e& drawPass)
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
	if (!smfRenderStates[RENDER_STATE_SEL]->CanDrawDeferred())
		return;

	geomBuffer.Bind();
	geomBuffer.Clear();

	{
		// switch selected state's SSP or Lua shader to deferred version
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(DrawPass::TerrainDeferred);
		smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass, HaveLuaRenderState());

		assert(smfRenderStates[RENDER_STATE_SEL]->HasValidShader(drawPass));

		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPreDeferred();

		meshDrawer->DrawMesh(drawPass);

		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glDisable(GL_ALPHA_TEST);
		}

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass, HaveLuaRenderState());
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(drawPass);
	}

	geomBuffer.UnBind();

	#if 0
	geomBuffer.DrawDebug(geomBuffer.GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX));
	#endif

	if (!drawForward) {
		eventHandler.DrawGroundPostDeferred();
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

	SelectRenderState(drawPass);

	if (drawDeferred) {
		// do the deferred pass first, will allow us to re-use
		// its output at some future point and eventually draw
		// the entire map deferred
		DrawDeferredPass(drawPass);
	}

	if (drawForward) {
		smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass, HaveLuaRenderState());

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPreForward();

		meshDrawer->DrawMesh(drawPass);

		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glDisable(GL_ALPHA_TEST);
		}
		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass, HaveLuaRenderState());
	}

	glDisable(GL_CULL_FACE);

	if (drawPass == DrawPass::Normal) {
		if (mapInfo->water.hasWaterPlane) {
			DrawWaterPlane(false);
		}

		if (drawMapEdges) {
			SCOPED_TIMER("CSMFGroundDrawer::DrawBorder");
			DrawBorder(drawPass);
		}
	}
}


void CSMFGroundDrawer::DrawBorder(const DrawPass::e drawPass)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_FFP];
	// smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass, false);

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

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		/*
		if (mapInfo->map.voidWater && (drawPass != DrawPass::WaterReflection)) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.9f);
		}
		*/

		meshDrawer->DrawBorderMesh(drawPass);

		/*
		if (mapInfo->map.voidWater && (drawPass != DrawPass::WaterReflection)) {
			glDisable(GL_ALPHA_TEST);
		}
		*/

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass, false);
	glDisable(GL_CULL_FACE);
}


void CSMFGroundDrawer::DrawShadowPass()
{
	if (!globalRendering->drawGround)
		return;
	if (readMap->HasOnlyVoidWater())
		return;

	Shader::IProgramObject* po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	glEnable(GL_POLYGON_OFFSET_FILL);

	glPolygonOffset(-1.f, -1.f);
		po->Enable();
			meshDrawer->DrawMesh(DrawPass::Shadow);
		po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
}



void CSMFGroundDrawer::SetLuaShader(const LuaMapShaderData* luaMapShaderData)
{
	smfRenderStates[RENDER_STATE_LUA]->Kill(true);
	smfRenderStates[RENDER_STATE_LUA]->Init(this, luaMapShaderData);
}

void CSMFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	groundTextures->BindSquareTexture(bigSquareX, bigSquareY);
	smfRenderStates[RENDER_STATE_SEL]->SetSquareTexGen(bigSquareX, bigSquareY);
}

void CSMFGroundDrawer::SetupBaseDrawPass() { smfRenderStates[RENDER_STATE_SSP]->SetCurrentShader(DrawPass::Normal); }
void CSMFGroundDrawer::SetupReflDrawPass() { smfRenderStates[RENDER_STATE_SSP]->SetCurrentShader(DrawPass::WaterReflection); }
void CSMFGroundDrawer::SetupRefrDrawPass() { smfRenderStates[RENDER_STATE_SSP]->SetCurrentShader(DrawPass::WaterRefraction); }



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

void CSMFGroundDrawer::UpdateSunDir() {
	// Lua has gl.GetSun
	if (HaveLuaRenderState())
		return;

	// always update, SSMF shader needs current sundir even when shadows are disabled
	smfRenderStates[RENDER_STATE_SEL]->UpdateCurrentShader(sky->GetLight());
}



bool CSMFGroundDrawer::UpdateGeometryBuffer(bool init)
{
	static const bool drawDeferredAllowed = configHandler->GetBool("AllowDeferredMapRendering");

	if (!drawDeferredAllowed)
		return false;

	return (geomBuffer.Update(init));
}



void CSMFGroundDrawer::IncreaseDetail()
{
	groundDetail += 2;
	LOG("GroundDetail is now %i", groundDetail);
}

void CSMFGroundDrawer::DecreaseDetail()
{
	if (groundDetail > 4) {
		groundDetail -= 2;
		LOG("GroundDetail is now %i", groundDetail);
	}
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

	return std::max(4, detail);
}

