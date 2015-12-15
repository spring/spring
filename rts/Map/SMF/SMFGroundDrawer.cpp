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

	smfRenderStateSSP = ISMFRenderState::GetInstance(globalRendering->haveARB, globalRendering->haveGLSL);
	smfRenderStateFFP = ISMFRenderState::GetInstance(                   false,                     false);

	// also set in ::Draw, but UpdateSunDir can be called
	// first if DynamicSun is enabled --> must be non-NULL
	SelectRenderState(false);

	// LH must be initialized before render-state is initialized
	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicMapLights"));
	geomBuffer.SetName("GROUNDDRAWER-GBUFFER");

	drawForward = true;
	drawDeferred = geomBuffer.Valid();
	drawMapEdges = configHandler->GetBool("MapBorder");

	// NOTE:
	//     advShading can NOT change at runtime if initially false
	//     (see AdvMapShadingActionExecutor), so we will always use
	//     smfRenderStateFFP (in ::Draw) in that special case and it
	//     does not matter if smfRenderStateSSP is initialized
	groundDetail = configHandler->GetInt("GroundDetail");
	advShading = smfRenderStateSSP->Init(this);

	waterPlaneCamInDispList  = 0;
	waterPlaneCamOutDispList = 0;

	if (mapInfo->water.hasWaterPlane) {
		waterPlaneCamInDispList = glGenLists(1);
		glNewList(waterPlaneCamInDispList, GL_COMPILE);
		CreateWaterPlanes(false);
		glEndList();

		waterPlaneCamOutDispList = glGenLists(1);
		glNewList(waterPlaneCamOutDispList, GL_COMPILE);
		CreateWaterPlanes(true);
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

	smfRenderStateSSP->Kill(); ISMFRenderState::FreeInstance(smfRenderStateSSP);
	smfRenderStateFFP->Kill(); ISMFRenderState::FreeInstance(smfRenderStateFFP);

	delete groundTextures;
	delete meshDrawer;

	if (waterPlaneCamInDispList) {
		glDeleteLists(waterPlaneCamInDispList, 1);
		glDeleteLists(waterPlaneCamOutDispList, 1);
	}
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
	if (!drawWaterReflection) {
		const bool skipUnderground = (camera->GetPos().IsInBounds() && !mapInfo->map.voidWater);
		const unsigned int dispList = skipUnderground ? waterPlaneCamInDispList: waterPlaneCamOutDispList;

		glPushMatrix();
		glTranslatef(0.f, std::min(-200.0f, smfMap->GetCurrMinHeight() - 400.0f), 0.f);
		glCallList(dispList);
		glPopMatrix();
	}
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
	if (!smfRenderState->CanDrawDeferred())
		return;

	geomBuffer.Bind();
	geomBuffer.Clear();

	{
		// switch current SSP shader to deferred version
		smfRenderState->SetCurrentShader(DrawPass::TerrainDeferred);
		smfRenderState->Enable(this, drawPass);

		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		meshDrawer->DrawMesh(drawPass);

		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glDisable(GL_ALPHA_TEST);
		}

		smfRenderState->Disable(this, drawPass);
		smfRenderState->SetCurrentShader(drawPass);
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


	// note: shared by deferred pass
	SelectRenderState(smfRenderStateSSP->CanEnable(this));

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if (drawDeferred) {
		// do the deferred pass first, will allow us to re-use
		// its output at some future point and eventually draw
		// the entire map deferred
		DrawDeferredPass(drawPass);
	}

	if (drawForward) {
		smfRenderState->Enable(this, drawPass);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, mapInfo->map.voidAlphaMin);
		}

		meshDrawer->DrawMesh(drawPass);

		if (mapInfo->map.voidGround || (mapInfo->map.voidWater && drawPass != DrawPass::WaterReflection)) {
			glDisable(GL_ALPHA_TEST);
		}
		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		smfRenderState->Disable(this, drawPass);
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

	SelectRenderState(false);
	// smfRenderState->Enable(this, drawPass);

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

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D); // needed for the non-shader case

	glEnable(GL_BLEND);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
			/*if (mapInfo->map.voidWater && (drawPass != DrawPass::WaterReflection)) {
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER, 0.9f);
			}*/

				meshDrawer->DrawBorderMesh(drawPass);

			/*if (mapInfo->map.voidWater && (drawPass != DrawPass::WaterReflection)) {
				glDisable(GL_ALPHA_TEST);
			}*/
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

	smfRenderState->Disable(this, drawPass);
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



void CSMFGroundDrawer::SetupBaseDrawPass() { smfRenderStateSSP->SetCurrentShader(DrawPass::Normal); }
void CSMFGroundDrawer::SetupReflDrawPass() { smfRenderStateSSP->SetCurrentShader(DrawPass::WaterReflection); }
void CSMFGroundDrawer::SetupRefrDrawPass() { smfRenderStateSSP->SetCurrentShader(DrawPass::WaterRefraction); }

void CSMFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	groundTextures->BindSquareTexture(bigSquareX, bigSquareY);
	smfRenderState->SetSquareTexGen(bigSquareX, bigSquareY);
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

void CSMFGroundDrawer::UpdateSunDir() {
	// always update, SSMF shader needs current sundir even when shadows are disabled
	smfRenderState->UpdateCurrentShader(sky->GetLight());
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
		//TODO: currently the shadow mesh needs to be idential with the normal pass one
		//  else we get z-fighting issues in the shadows. Ideal would be a special
		//  shadow pass mesh renderer that reduce the mesh to `walls`/contours that cause the
		//  same shadows as the original terrain
		//case DrawPass::Shadow:
		//	detail *= LODScaleShadow;
		//	break;
		default:
			break;
	}

	return std::max(4, detail);
}

