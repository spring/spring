/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFReadMap.h"
#include "SMFGroundDrawer.h"
#include "SMFGroundTextures.h"
#include "SMFRenderState.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/SMF/Basic/BasicMeshDrawer.h"
// #include "Map/SMF/Legacy/LegacyMeshDrawer.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FastMath.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

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

CONFIG(bool, AllowDeferredMapRendering).defaultValue(false).safemodeValue(false);


CONFIG(int, ROAM)
	.defaultValue(1)
	.safemodeValue(1)
	.minimumValue(0)
	.maximumValue(1)
	.description("Use ROAM for terrain mesh rendering: 0 to disable, 1 to enable.");


CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* rm)
	: smfMap(rm)
	, meshDrawer(nullptr)
{
	drawerMode = (configHandler->GetInt("ROAM") != 0)? SMF_MESHDRAWER_ROAM: SMF_MESHDRAWER_BASIC;
	groundDetail = configHandler->GetInt("GroundDetail");

	groundTextures = new CSMFGroundTextures(smfMap);
	meshDrawer = SwitchMeshDrawer(drawerMode);

	smfRenderStates.resize(RENDER_STATE_CNT, nullptr);
	smfRenderStates[RENDER_STATE_NOP] = ISMFRenderState::GetInstance( true, false);
	smfRenderStates[RENDER_STATE_SSP] = ISMFRenderState::GetInstance(false, false);
	smfRenderStates[RENDER_STATE_LUA] = ISMFRenderState::GetInstance(false,  true);

	// LH must be initialized before render-state is initialized
	lightHandler.Init(configHandler->GetInt("MaxDynamicMapLights"));
	geomBuffer.SetName("GROUNDDRAWER-GBUFFER");

	drawForward = true;
	drawDeferred = geomBuffer.Valid();
	drawMapEdges = configHandler->GetBool("MapBorder");
	drawWaterPlane = false; // waterRendering->hasWaterPlane;

	smfRenderStates[RENDER_STATE_SSP]->Init(this);
	smfRenderStates[RENDER_STATE_SSP]->Update(this, nullptr);

	// always initialize this state; defer Update (allows re-use)
	smfRenderStates[RENDER_STATE_LUA]->Init(this);

	// note: state must be pre-selected before the first drawn frame
	// Sun*Changed can be called first, e.g. if DynamicSun is enabled
	smfRenderStates[RENDER_STATE_SEL] = SelectRenderState(DrawPass::Normal);


	if (drawWaterPlane) {
		CreateWaterPlanes( true);
		CreateWaterPlanes(false);
	}
	if (drawMapEdges)
		CreateBorderShader();

	if (drawDeferred) {
		drawDeferred &= UpdateGeometryBuffer(true);
	}
}

CSMFGroundDrawer::~CSMFGroundDrawer()
{
	// remember if ROAM-mode was enabled
	configHandler->Set("ROAM", int(dynamic_cast<CRoamMeshDrawer*>(meshDrawer) != nullptr));

	smfRenderStates[RENDER_STATE_NOP]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_NOP]);
	smfRenderStates[RENDER_STATE_SSP]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_SSP]);
	smfRenderStates[RENDER_STATE_LUA]->Kill(); ISMFRenderState::FreeInstance(smfRenderStates[RENDER_STATE_LUA]);
	smfRenderStates.clear();

	waterPlaneBuffers[0].Kill();
	waterPlaneBuffers[1].Kill();

	// no-op if the shader was never created, but necessary
	// in case the game or user turned border rendering off
	// at runtime
	borderShader.Release(false);

	spring::SafeDelete(groundTextures);
	spring::SafeDelete(meshDrawer);
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

	switch ((drawerMode = wantedMode)) {
		#if 0
		case SMF_MESHDRAWER_LEGACY: {
			LOG("Switching to Legacy Mesh Rendering");
			spring::SafeDelete(meshDrawer);
			meshDrawer = new CLegacyMeshDrawer(smfMap, this);
		} break;
		#endif
		case SMF_MESHDRAWER_BASIC: {
			LOG("Switching to Basic Mesh Rendering");
			spring::SafeDelete(meshDrawer);
			meshDrawer = new CBasicMeshDrawer(this);
		} break;
		default: {
			LOG("Switching to ROAM Mesh Rendering");
			spring::SafeDelete(meshDrawer);
			meshDrawer = new CRoamMeshDrawer(this);
		} break;
	}

	return meshDrawer;
}



void CSMFGroundDrawer::CreateWaterPlanes(bool camOutsideMap) {
	{
		const float xsize = (smfMap->mapSizeX) >> 2;
		const float ysize = (smfMap->mapSizeZ) >> 2;
		const float size = std::min(xsize, ysize);
		const float alphainc = math::TWOPI / 32.0f;

		static std::vector<VA_TYPE_C> verts;

		verts.clear();
		verts.reserve(4 * (32 + 1) * 2);

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

		float alpha;
		float r1;
		float r2;

		float3 p;

		for (int n = (camOutsideMap) ? 0 : 1; n < 4; ++n) {
			if ((n == 1) && !camOutsideMap) {
				// don't render vertices under the map
				r1 = 2.0f * size;
			} else {
				r1 = n * n * size;
			}

			if (n == 3) {
				// last stripe: make it thinner (looks better with fog)
				r2 = (n + 0.5) * (n + 0.5) * size;
			} else {
				r2 = (n + 1) * (n + 1) * size;
			}

			for (alpha = 0.0f; (alpha - math::TWOPI) < alphainc; alpha += alphainc) {
				p.x = r1 * fastmath::sin(alpha) + 2.0f * xsize;
				p.z = r1 * fastmath::cos(alpha) + 2.0f * ysize;

				verts.push_back(VA_TYPE_C{p, {planeColor}});

				p.x = r2 * fastmath::sin(alpha) + 2.0f * xsize;
				p.z = r2 * fastmath::cos(alpha) + 2.0f * ysize;

				verts.push_back(VA_TYPE_C{p, {(n == 3)? fogColor : planeColor}});
			}
		}

		waterPlaneBuffers[camOutsideMap].Init();
		waterPlaneBuffers[camOutsideMap].UploadC(verts.size(), 0, verts.data(), nullptr);
	}
	{
		char vsBuf[65536];
		char fsBuf[65536];

		const char* vsVars = "uniform float u_plane_offset;\n";
		const char* vsCode =
			"\tgl_Position = u_proj_mat * u_movi_mat * vec4(a_vertex_xyz + vec3(0.0, u_plane_offset, 0.0), 1.0);\n"
			"\tv_color_rgba = a_color_rgba;\n";
		const char* fsVars =
			"uniform float u_gamma_exponent;\n"
			"const float v_color_mult = 1.0 / 255.0;\n";
		const char* fsCode =
			"\tf_color_rgba = v_color_rgba * v_color_mult;\n"
			"\tf_color_rgba.rgb = pow(f_color_rgba.rgb, vec3(u_gamma_exponent));\n";

		GL::RenderDataBuffer::FormatShaderC(vsBuf, vsBuf + sizeof(vsBuf), "", vsVars, vsCode, "VS");
		GL::RenderDataBuffer::FormatShaderC(fsBuf, fsBuf + sizeof(fsBuf), "", fsVars, fsCode, "FS");
		GL::RenderDataBuffer& renderDataBuffer = waterPlaneBuffers[camOutsideMap];

		Shader::GLSLShaderObject shaderObjs[2] = {{GL_VERTEX_SHADER, &vsBuf[0], ""}, {GL_FRAGMENT_SHADER, &fsBuf[0], ""}};
		Shader::IProgramObject* shaderProg = renderDataBuffer.CreateShader((sizeof(shaderObjs) / sizeof(shaderObjs[0])), 0, &shaderObjs[0], nullptr);

		shaderProg->Enable();
		shaderProg->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shaderProg->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::Identity());
		shaderProg->SetUniform("u_plane_offset", 0.0f);
		shaderProg->SetUniform("u_gamma_exponent", globalRendering->gammaExponent);
		shaderProg->Disable();
	}
}

void CSMFGroundDrawer::CreateBorderShader() {
	std::string vsCode = std::move(Shader::GetShaderSource("GLSL/SMFBorderVertProg4.glsl"));
	std::string fsCode = std::move(Shader::GetShaderSource("GLSL/SMFBorderFragProg4.glsl"));

	Shader::GLSLShaderObject shaderObjs[2] = {{GL_VERTEX_SHADER, vsCode, ""}, {GL_FRAGMENT_SHADER, fsCode, ""}};
	Shader::IProgramObject* shaderProg = &borderShader;

	borderShader.AttachShaderObject(&shaderObjs[0]);
	borderShader.AttachShaderObject(&shaderObjs[1]);
	borderShader.ReloadShaderObjects();
	borderShader.CreateAndLink();
	borderShader.RecalculateShaderHash();
	borderShader.ClearAttachedShaderObjects();
	borderShader.Validate();

	shaderProg->Enable();
	shaderProg->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shaderProg->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::Identity());
	shaderProg->SetUniform("u_diffuse_tex_sqr", -1, -1, -1);
	shaderProg->SetUniform("u_diffuse_tex", 0);
	shaderProg->SetUniform("u_detail_tex", 2);
	shaderProg->SetUniform("u_gamma_exponent", globalRendering->gammaExponent);
	shaderProg->Disable();
}


void CSMFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (drawWaterReflection)
		return;

	GL::RenderDataBuffer& buffer = waterPlaneBuffers[1 - (camera->GetPos().IsInBounds() && !mapRendering->voidWater)];
	Shader::IProgramObject* shader = &buffer.GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
	shader->SetUniform("u_plane_offset", std::min(-200.0f, smfMap->GetCurrMinHeight() - 400.0f));
	shader->SetUniform("u_gamma_exponent", globalRendering->gammaExponent);
	buffer.Submit(GL_TRIANGLE_STRIP, 0, buffer.GetNumElems<VA_TYPE_C>());
	shader->Disable();
}



ISMFRenderState* CSMFGroundDrawer::SelectRenderState(const DrawPass::e& drawPass)
{
	// [0] := Lua GLSL, must have a valid shader for this pass
	// [1] := default GLSL, same condition
	const unsigned int stateEnums[2] = {RENDER_STATE_LUA, RENDER_STATE_SSP};

	for (const unsigned int stateEnum: stateEnums) {
		ISMFRenderState* state = smfRenderStates[stateEnum];

		if (!state->CanEnable(this))
			continue;
		if (!state->HasValidShader(drawPass))
			continue;

		return (smfRenderStates[RENDER_STATE_SEL] = state);
	}

	// fallback
	return (smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_NOP]);
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

		if (alphaTest)
			smfRenderStates[RENDER_STATE_SEL]->SetAlphaTest({mapInfo->map.voidAlphaMin, 1.0f, 0.0f, 0.0f}); // test > min

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPreDeferred();

		meshDrawer->DrawMesh(drawPass);

		if (alphaTest)
			smfRenderStates[RENDER_STATE_SEL]->SetAlphaTest({0.0f, 0.0f, 0.0f, 1.0f}); // no test

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(DrawPass::Normal);

		geomBuffer.SetDepthRange(0.0f, 1.0f);
		geomBuffer.UnBind();
	}

	#if 0
	geomBuffer.DrawDebug(geomBuffer.GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX));
	#endif

	// send event if no forward pass will follow; must be done after the unbind
	if (!drawForward)
		eventHandler.DrawGroundPostDeferred();
}

void CSMFGroundDrawer::DrawForwardPass(const DrawPass::e& drawPass, bool alphaTest)
{
	if (!SelectRenderState(drawPass)->CanDrawForward())
		return;

	{
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(drawPass);
		smfRenderStates[RENDER_STATE_SEL]->Enable(this, drawPass);

		glAttribStatePtr->PushBits((GL_ENABLE_BIT * alphaTest) | (GL_POLYGON_BIT * wireframe));

		if (wireframe)
			glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		if (alphaTest)
			smfRenderStates[RENDER_STATE_SEL]->SetAlphaTest({mapInfo->map.voidAlphaMin, 1.0f, 0.0f, 0.0f}); // test > min

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPreForward();

		meshDrawer->DrawMesh(drawPass);

		if (alphaTest)
			smfRenderStates[RENDER_STATE_SEL]->SetAlphaTest({0.0f, 0.0f, 0.0f, 1.0f}); // no test


		glAttribStatePtr->PopBits();

		smfRenderStates[RENDER_STATE_SEL]->Disable(this, drawPass);
		smfRenderStates[RENDER_STATE_SEL]->SetCurrentShader(DrawPass::Normal);

		if (HaveLuaRenderState())
			eventHandler.DrawGroundPostForward();
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

	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->EnableCullFace();
	glAttribStatePtr->CullFace(GL_BACK);

	groundTextures->BindSquareTextureArray();

	// do the deferred pass first, will allow us to re-use
	// its output at some future point and eventually draw
	// the entire map deferred
	if (drawDeferred)
		DrawDeferredPass(drawPass, mapRendering->voidGround || (mapRendering->voidWater && drawPass != DrawPass::WaterReflection));

	if (drawForward)
		DrawForwardPass(drawPass, mapRendering->voidGround || (mapRendering->voidWater && drawPass != DrawPass::WaterReflection));

	groundTextures->UnBindSquareTextureArray();
	glAttribStatePtr->DisableCullFace();

	if (drawPass != DrawPass::Normal)
		return;

	if (drawWaterPlane)
		DrawWaterPlane(false);

	if (drawMapEdges)
		DrawBorder(drawPass);
}


void CSMFGroundDrawer::DrawBorder(const DrawPass::e drawPass)
{
	ISMFRenderState* prvState = smfRenderStates[RENDER_STATE_SEL];
	Shader::IProgramObject* shaderProg = &borderShader;

	// no need to enable, does nothing
	smfRenderStates[RENDER_STATE_SEL] = smfRenderStates[RENDER_STATE_NOP];


	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());


	glAttribStatePtr->EnableCullFace();
	glAttribStatePtr->CullFace(GL_BACK);

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireframe + GL_FILL * (1 - wireframe));

	shaderProg->Enable();
	shaderProg->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	shaderProg->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
	shaderProg->SetUniform<float>("u_gamma_exponent", globalRendering->gammaExponent);
	// shaderProg->SetUniform("u_alpha_test_ctrl", 0.9f, 1.0f, 0.0f, 0.0f); // test > 0.9 if (mapRendering->voidWater && (drawPass != DrawPass::WaterReflection))

	groundTextures->BindSquareTextureArray();
	meshDrawer->DrawBorderMesh(drawPass); // calls back into ::SetupBigSquare
	groundTextures->UnBindSquareTextureArray();

	// shaderProg->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	shaderProg->Disable();

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glAttribStatePtr->DisableBlendMask();

	glAttribStatePtr->DisableCullFace();


	smfRenderStates[RENDER_STATE_SEL] = prvState;
}


void CSMFGroundDrawer::DrawShadowPass()
{
	if (!globalRendering->drawGround)
		return;
	if (readMap->HasOnlyVoidWater())
		return;

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);
	glAttribStatePtr->PolygonOffset(spPolygonOffsetScale, spPolygonOffsetUnits); // dz*s + r*u

		// also render the border geometry to prevent light-visible backfaces
		po->Enable();
		po->SetUniformMatrix4fv(1, false, shadowHandler.GetShadowViewMatrix());
		po->SetUniformMatrix4fv(2, false, shadowHandler.GetShadowProjMatrix());
			meshDrawer->DrawMesh(DrawPass::Shadow);
			meshDrawer->DrawBorderMesh(DrawPass::Shadow);
		po->Disable();

	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);
}



void CSMFGroundDrawer::SetLuaShader(const LuaMapShaderData* luaMapShaderData)
{
	smfRenderStates[RENDER_STATE_LUA]->Update(this, luaMapShaderData);
}

void CSMFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	const int sqrIdx = bigSquareY * smfMap->numBigTexX + bigSquareX;
	const int sqrMip = groundTextures->GetSquareMipLevel(sqrIdx);

	groundTextures->DrawUpdateSquare(bigSquareX, bigSquareY);
	smfRenderStates[RENDER_STATE_SEL]->SetSquareTexGen(bigSquareX, bigSquareY, smfMap->numBigTexX, sqrMip);

	Shader::IProgramObject& ipo = borderShader;

	if (!ipo.IsBound())
		return;

	ipo.SetUniform("u_diffuse_tex_sqr", bigSquareX, bigSquareY, sqrIdx);
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
	// Lua has gl.GetSun
	if (HaveLuaRenderState())
		return;

	// always update, SSMF shader needs current sundir even when shadows are disabled
	// note: only the active state is notified of a given change
	smfRenderStates[RENDER_STATE_SEL]->SetSkyLight(sky->GetLight());
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
