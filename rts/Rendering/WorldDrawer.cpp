/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"

#include "WorldDrawer.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/SmoothHeightMeshDrawer.h"
#include "Rendering/InMapDrawView.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/GuiHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"

void CWorldDrawer::InitPre() const
{
	LuaObjectDrawer::Init();
	DebugColVolDrawer::Init();
	cursorIcons.Init();

	CColorMap::InitStatic();

	// these need to be loaded before featureHandler is created
	// (maps with features have their models loaded at startup)
	modelLoader.Init();

	loadscreen->SetLoadMessage("Creating Unit Textures");
	textureHandler3DO.Init();
	textureHandlerS3O.Init();

	CFeatureDrawer::InitStatic();
	loadscreen->SetLoadMessage("Creating Sky");

	sky = ISky::GetSky();
	sunLighting->Init();
}

void CWorldDrawer::InitPost() const
{
	char buf[512] = {0};

	{
		loadscreen->SetLoadMessage("Creating ShadowHandler");
		shadowHandler.Init();
	}
	{
		// SMFGroundDrawer accesses InfoTextureHandler, create it first
		loadscreen->SetLoadMessage("Creating InfoTextureHandler");
		IInfoTextureHandler::Create();
	}

	try {
		loadscreen->SetLoadMessage("Creating GroundDrawer");
		readMap->InitGroundDrawer();
	} catch (const content_error& e) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "[WorldDrawer::%s] caught exception \"%s\"", __func__, e.what());
	}

	{
		loadscreen->SetLoadMessage("Creating TreeDrawer");
		treeDrawer = ITreeDrawer::GetTreeDrawer();
		grassDrawer = new CGrassDrawer();
	}
	{
		inMapDrawerView = new CInMapDrawView();
		pathDrawer = IPathDrawer::GetInstance();
	}
	{
		heightMapTexture = new HeightMapTexture();
		farTextureHandler = new CFarTextureHandler();
	}
	{
		IGroundDecalDrawer::Init();
	}
	{
		loadscreen->SetLoadMessage("Creating ProjectileDrawer & UnitDrawer");

		CProjectileDrawer::InitStatic();
		CUnitDrawer::InitStatic();
		// see ::InitPre
		// CFeatureDrawer::InitStatic();
	}

	// rethrow to force exit
	if (buf[0] != 0)
		throw content_error(buf);

	{
		loadscreen->SetLoadMessage("Creating Water");
		water = IWater::GetWater(nullptr, -1);
	}
}


void CWorldDrawer::Kill()
{
	spring::SafeDelete(infoTextureHandler);

	spring::SafeDelete(water);
	spring::SafeDelete(sky);
	spring::SafeDelete(treeDrawer);
	spring::SafeDelete(grassDrawer);
	spring::SafeDelete(pathDrawer);
	shadowHandler.Kill();
	spring::SafeDelete(inMapDrawerView);

	CFeatureDrawer::KillStatic(gu->globalReload);
	CUnitDrawer::KillStatic(gu->globalReload); // depends on unitHandler, cubeMapHandler
	CProjectileDrawer::KillStatic(gu->globalReload);

	modelLoader.Kill();

	spring::SafeDelete(farTextureHandler);
	spring::SafeDelete(heightMapTexture);

	textureHandler3DO.Kill();
	textureHandlerS3O.Kill();

	readMap->KillGroundDrawer();
	IGroundDecalDrawer::FreeInstance();
	LuaObjectDrawer::Kill();
	DebugColVolDrawer::Kill();

	numUpdates = 0;
}




void CWorldDrawer::Update(bool newSimFrame)
{
	SCOPED_TIMER("Update::WorldDrawer");
	LuaObjectDrawer::Update(numUpdates == 0);
	readMap->UpdateDraw(numUpdates == 0);

	if (globalRendering->drawGround)
		(readMap->GetGroundDrawer())->Update();

	// XXX: done in CGame, needs to get updated even when !doDrawWorld
	// (it updates unitdrawpos which is used for maximized minimap too)
	// unitDrawer->Update();
	// lineDrawer.UpdateLineStipple();
	treeDrawer->Update();
	featureDrawer->Update();
	IWater::ApplyPushedChanges(game);

	if (newSimFrame) {
		projectileDrawer->UpdateTextures();

		{
			SCOPED_TIMER("Update::WorldDrawer::{Sky,Water}");

			sky->Update();
			water->Update();
		}

		// once every simframe is frequent enough here
		// NB: errors will not be logged until frame 0
		modelLoader.LogErrors();
	}

	numUpdates += 1;
}



void CWorldDrawer::GenerateIBLTextures() const
{

	if (shadowHandler.ShadowsLoaded()) {
		SCOPED_TIMER("Draw::World::CreateShadows");
		game->SetDrawMode(Game::ShadowDraw);
		shadowHandler.CreateShadows();
		game->SetDrawMode(Game::NormalDraw);
	}

	{
		SCOPED_TIMER("Draw::World::UpdateReflTex");
		cubeMapHandler.UpdateReflectionTexture();
	}

	if (sky->GetLight()->Update()) {
		{
			SCOPED_TIMER("Draw::World::UpdateSkyTex");
			sky->UpdateSkyTexture();
		}
		{
			SCOPED_TIMER("Draw::World::UpdateShadingTex");
			readMap->UpdateShadingTexture();
		}
	}

	FBO::Unbind();

	// restore the normal active camera's VP
	camera->LoadViewPort();
}

void CWorldDrawer::SetupScreenState() const
{
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



void CWorldDrawer::Draw() const
{
	SCOPED_TIMER("Draw::World");

	glAttribStatePtr->ClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 0.0f);
	glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glAttribStatePtr->EnableDepthMask();
	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	sky->Draw(Game::NormalDraw);

	DrawOpaqueObjects();
	DrawAlphaObjects();

	{
		SCOPED_TIMER("Draw::World::Projectiles");
		projectileDrawer->Draw(false);
	}

	sky->DrawSun(Game::NormalDraw);

	{
		SCOPED_TIMER("Draw::World::DrawWorld");
		eventHandler.DrawWorld();
	}

	DrawMiscObjects();
	DrawBelowWaterOverlay();
}


void CWorldDrawer::DrawOpaqueObjects() const
{
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	if (globalRendering->drawGround) {
		{
			SCOPED_TIMER("Draw::World::Terrain");
			gd->Draw(DrawPass::Normal);
		}
		{
			SCOPED_TIMER("Draw::World::Decals");
			groundDecals->Draw();
			projectileDrawer->DrawGroundFlashes();
		}
		{
			SCOPED_TIMER("Draw::World::Foliage");
			grassDrawer->Draw();
			treeDrawer->Draw();
		}
		smoothHeightMeshDrawer->Draw(1.0f);
	}

	// run occlusion query here so it has more time to finish before UpdateWater
	if (globalRendering->drawWater && !mapRendering->voidWater) {
		water->OcclusionQuery();
	}

	selectedUnitsHandler.Draw();
	eventHandler.DrawWorldPreUnit();

	{
		SCOPED_TIMER("Draw::World::Models::Opaque");
		unitDrawer->Draw();
		featureDrawer->Draw();

		DebugColVolDrawer::Draw();
		pathDrawer->DrawAll();
	}
}

void CWorldDrawer::DrawAlphaObjects() const
{
	// transparent objects
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->DepthFunc(GL_LEQUAL);

	{
		SCOPED_TIMER("Draw::World::Models::Alpha");
		glEnable(GL_CLIP_DISTANCE0 + IWater::ClipPlaneIndex());

		// draw alpha-objects below water surface (farthest)
		unitDrawer->DrawAlphaPass(false);
		featureDrawer->DrawAlphaPass(false);

		glDisable(GL_CLIP_DISTANCE0 + IWater::ClipPlaneIndex());
	}

	// draw water (in-between)
	if (globalRendering->drawWater && !mapRendering->voidWater) {
		SCOPED_TIMER("Draw::World::Water");

		water->UpdateWater(game);
		water->Draw();
	}

	{
		SCOPED_TIMER("Draw::World::Models::Alpha");
		glEnable(GL_CLIP_DISTANCE0 + IWater::ClipPlaneIndex());

		// draw alpha-objects above water surface (closest)
		unitDrawer->DrawAlphaPass(true);
		featureDrawer->DrawAlphaPass(true);

		glDisable(GL_CLIP_DISTANCE0 + IWater::ClipPlaneIndex());
	}
}

void CWorldDrawer::DrawMiscObjects() const
{

	{
		// note: duplicated in CMiniMap::DrawWorldStuff()
		commandDrawer->DrawLuaQueuedUnitSetCommands(false);

		if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate())
			selectedUnitsHandler.DrawCommands(false);
	}

	// either draw from here, or make {Dyn,Bump}Water use blending
	// pro: icons are drawn only once per frame, not every pass
	// con: looks somewhat worse for underwater / obscured icons
	unitDrawer->DrawUnitIcons();

	lineDrawer.DrawAll(false);
	cursorIcons.Draw();

	mouse->DrawSelectionBox();
	guihandler->DrawMapStuff(false);

	if (!globalRendering->drawMapMarks || game->hideInterface)
		return;

	inMapDrawerView->Draw();
}



void CWorldDrawer::DrawBelowWaterOverlay() const
{
	#if 0
	if (!globalRendering->drawWater)
		return;
	if (mapRendering->voidWater)
		return;

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	{
		const float4 cpos = {camera->GetPos(), camera->GetFarPlaneDist() * 0.5f};

		if (cpos.y >= 0.0f)
			return;

		glAttribStatePtr->DisableDepthMask();

		{
			shader->Enable();
			shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
			shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());

			buffer->SafeAppend({{cpos.x - cpos.w, 0.0f, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x - cpos.w, 0.0f, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x + cpos.w, 0.0f, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});

			buffer->SafeAppend({{cpos.x + cpos.w, 0.0f, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x + cpos.w, 0.0f, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x - cpos.w, 0.0f, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->Submit(GL_TRIANGLES);
		}

		{
			buffer->SafeAppend({{cpos.x - cpos.w,     0.0f, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x - cpos.w,  -cpos.w, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x - cpos.w,     0.0f, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x - cpos.w,  -cpos.w, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});

			buffer->SafeAppend({{cpos.x + cpos.w,     0.0f, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x + cpos.w,  -cpos.w, cpos.z + cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x + cpos.w,     0.0f, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x + cpos.w,  -cpos.w, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});

			buffer->SafeAppend({{cpos.x - cpos.w,     0.0f, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->SafeAppend({{cpos.x - cpos.w,  -cpos.w, cpos.z - cpos.w}, {0.0f, 0.5f, 0.3f, 0.50f}});
			buffer->Submit(GL_QUAD_STRIP);
		}

		glAttribStatePtr->EnableDepthMask();
	}

	{
		glAttribStatePtr->DisableDepthTest();
		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// draw water-coloration quad in raw screenspace
		shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::Identity());

		buffer->SafeAppend({{0.0f, 0.0f, -1.0f}, {0.0f, 0.2f, 0.8f, 0.333f}});
		buffer->SafeAppend({{1.0f, 0.0f, -1.0f}, {0.0f, 0.2f, 0.8f, 0.333f}});
		buffer->SafeAppend({{1.0f, 1.0f, -1.0f}, {0.0f, 0.2f, 0.8f, 0.333f}});

		buffer->SafeAppend({{1.0f, 1.0f, -1.0f}, {0.0f, 0.2f, 0.8f, 0.333f}});
		buffer->SafeAppend({{0.0f, 1.0f, -1.0f}, {0.0f, 0.2f, 0.8f, 0.333f}});
		buffer->SafeAppend({{0.0f, 0.0f, -1.0f}, {0.0f, 0.2f, 0.8f, 0.333f}});
		buffer->Submit(GL_TRIANGLES);

		shader->Disable();
	}
	#endif
}
