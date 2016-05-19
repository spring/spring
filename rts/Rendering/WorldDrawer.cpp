/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "WorldDrawer.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
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
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Game.h"
#include "Game/LoadScreen.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/GuiHandler.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"

CWorldDrawer::CWorldDrawer(): numUpdates(0)
{
	CShaderHandler::GetInstance(0);
	LuaObjectDrawer::Init();
}

CWorldDrawer::~CWorldDrawer()
{
	SafeDelete(water);
	SafeDelete(sky);
	SafeDelete(treeDrawer);
	SafeDelete(grassDrawer);
	SafeDelete(pathDrawer);
	SafeDelete(shadowHandler);
	SafeDelete(inMapDrawerView);

	SafeDelete(featureDrawer);
	SafeDelete(unitDrawer); // depends on unitHandler, cubeMapHandler
	SafeDelete(projectileDrawer);

	modelLoader.Kill();

	SafeDelete(farTextureHandler);
	SafeDelete(heightMapTexture);

	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);

	SafeDelete(cubeMapHandler);

	readMap->KillGroundDrawer();
	IGroundDecalDrawer::FreeInstance();
	CShaderHandler::FreeInstance(shaderHandler);
	LuaObjectDrawer::Kill();
}



void CWorldDrawer::LoadPre() const
{
	// these need to be loaded before featureHandler is created
	// (maps with features have their models loaded at startup)
	modelLoader.Init();

	loadscreen->SetLoadMessage("Creating Unit Textures");
	texturehandler3DO = new C3DOTextureHandler();
	texturehandlerS3O = new CS3OTextureHandler();

	featureDrawer = new CFeatureDrawer();
	loadscreen->SetLoadMessage("Creating Sky");

	sky = ISky::GetSky();
	sunLighting->Init();
}

void CWorldDrawer::LoadPost() const
{
	loadscreen->SetLoadMessage("Creating ShadowHandler");
	cubeMapHandler = new CubeMapHandler();
	shadowHandler = new CShadowHandler();

	loadscreen->SetLoadMessage("Creating GroundDrawer");
	readMap->InitGroundDrawer();

	loadscreen->SetLoadMessage("Creating TreeDrawer");
	treeDrawer = ITreeDrawer::GetTreeDrawer();
	grassDrawer = new CGrassDrawer();

	inMapDrawerView = new CInMapDrawView();
	pathDrawer = IPathDrawer::GetInstance();

	farTextureHandler = new CFarTextureHandler();
	heightMapTexture = new HeightMapTexture();

	IGroundDecalDrawer::Init();

	loadscreen->SetLoadMessage("Creating ProjectileDrawer & UnitDrawer");
	projectileDrawer = new CProjectileDrawer();
	projectileDrawer->LoadWeaponTextures();

	unitDrawer = new CUnitDrawer();
	// see ::LoadPre
	// featureDrawer = new CFeatureDrawer();

	loadscreen->SetLoadMessage("Creating Water");
	water = IWater::GetWater(NULL, -1);

	sky->SetupFog();
}



void CWorldDrawer::Update(bool newSimFrame)
{
	LuaObjectDrawer::Update(numUpdates == 0);
	readMap->UpdateDraw(numUpdates == 0);

	if (globalRendering->drawGround) {
		SCOPED_TIMER("GroundDrawer::Update");
		(readMap->GetGroundDrawer())->Update();
	}

	// XXX: done in CGame, needs to get updated even when !doDrawWorld
	// (it updates unitdrawpos which is used for maximized minimap too)
	// unitDrawer->Update();
	// lineDrawer.UpdateLineStipple();
	treeDrawer->Update();
	featureDrawer->Update();
	IWater::ApplyPushedChanges(game);

	if (newSimFrame) {
		projectileDrawer->UpdateTextures();
		sky->Update();
		sky->GetLight()->Update();
		water->Update();
	}

	numUpdates += 1;
}



void CWorldDrawer::GenerateIBLTextures() const
{
	SCOPED_GMARKER("WorldDrawer::GenerateIBLTextures");

	if (shadowHandler->ShadowsLoaded()) {
		SCOPED_TIMER("ShadowHandler::CreateShadows");
		SCOPED_GMARKER("ShadowHandler::CreateShadows");
		game->SetDrawMode(CGame::gameShadowDraw);
		shadowHandler->CreateShadows();
		game->SetDrawMode(CGame::gameNormalDraw);
	}

	{
		SCOPED_TIMER("CubeMapHandler::UpdateReflTex");
		SCOPED_GMARKER("CubeMapHandler::UpdateReflTex");
		cubeMapHandler->UpdateReflectionTexture();
	}

	if (sky->GetLight()->IsDynamic()) {
		{
			SCOPED_TIMER("CubeMapHandler::UpdateSpecTex");
			SCOPED_GMARKER("CubeMapHandler::UpdateSpecTex");
			cubeMapHandler->UpdateSpecularTexture();
		}
		{
			SCOPED_TIMER("Sky::UpdateSkyTex");
			SCOPED_GMARKER("Sky::UpdateSkyTex");
			sky->UpdateSkyTexture();
		}
		{
			SCOPED_TIMER("ReadMap::UpdateShadingTex");
			SCOPED_GMARKER("ReadMap::UpdateShadingTex");
			readMap->UpdateShadingTexture();
		}
	}

	if (FBO::IsSupported())
		FBO::Unbind();

	// restore the normal active camera's VP
	camera->LoadViewPort();
}

void CWorldDrawer::ResetMVPMatrices() const
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



void CWorldDrawer::Draw() const
{
	SCOPED_TIMER("WorldDrawer::Total");
	SCOPED_GMARKER("WorldDrawer::Draw");

	glClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	DrawOpaqueObjects();
	DrawAlphaObjects();

	{
		SCOPED_TIMER("WorldDrawer::Projectiles");
		SCOPED_GMARKER("WorldDrawer::Projectiles");
		projectileDrawer->Draw(false);
	}

	sky->DrawSun();

	{
		SCOPED_GMARKER("EventHandler::DrawWorld");
		eventHandler.DrawWorld();
	}

	DrawMiscObjects();
	DrawBelowWaterOverlay();

	glDisable(GL_FOG);
}


void CWorldDrawer::DrawOpaqueObjects() const
{
	SCOPED_GMARKER("WorldDrawer::DrawOpaqueObjects");

	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	sky->Draw();

	if (globalRendering->drawGround) {
		{
			SCOPED_TIMER("WorldDrawer::Terrain");
			SCOPED_GMARKER("WorldDrawer::Terrain");
			gd->Draw(DrawPass::Normal);
		}
		{
			SCOPED_TIMER("WorldDrawer::GroundDecals");
			SCOPED_GMARKER("WorldDrawer::GroundDecals");
			groundDecals->Draw();
			projectileDrawer->DrawGroundFlashes();
		}
		{
			SCOPED_TIMER("WorldDrawer::Foliage");
			SCOPED_GMARKER("WorldDrawer::Foliage");
			grassDrawer->Draw();
			gd->DrawTrees();
		}
		smoothHeightMeshDrawer->Draw(1.0f);
	}

	// run occlusion query here so it has more time to finish before UpdateWater
	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("WorldDrawer::Water::OcclusionQuery");
		SCOPED_GMARKER("WorldDrawer::Water::OcclusionQuery");
		water->OcclusionQuery();
	}

	selectedUnitsHandler.Draw();
	eventHandler.DrawWorldPreUnit();

	{
		SCOPED_TIMER("WorldDrawer::Models");
		SCOPED_GMARKER("WorldDrawer::Models");
		unitDrawer->Draw(false);
		featureDrawer->Draw();

		DebugColVolDrawer::Draw();
		pathDrawer->DrawAll();
	}
}

void CWorldDrawer::DrawAlphaObjects() const
{
	SCOPED_GMARKER("WorldDrawer::DrawAlphaObjects");

	// transparent objects
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	static const double belowPlaneEq[4] = {0.0f, -1.0f, 0.0f, 0.0f};
	static const double abovePlaneEq[4] = {0.0f,  1.0f, 0.0f, 0.0f};

	{
		// clip in model-space
		glPushMatrix();
		glLoadIdentity();
		glClipPlane(GL_CLIP_PLANE3, belowPlaneEq);
		glPopMatrix();
		glEnable(GL_CLIP_PLANE3);

		// draw alpha-objects below water surface (farthest)
		unitDrawer->DrawAlphaPass();
		featureDrawer->DrawAlphaPass();

		glDisable(GL_CLIP_PLANE3);
	}

	// draw water (in-between)
	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("WorldDrawer::Water");
		SCOPED_GMARKER("WorldDrawer::Water");

		water->UpdateWater(game);
		water->Draw();
	}

	{
		glPushMatrix();
		glLoadIdentity();
		glClipPlane(GL_CLIP_PLANE3, abovePlaneEq);
		glPopMatrix();
		glEnable(GL_CLIP_PLANE3);

		// draw alpha-objects above water surface (closest)
		unitDrawer->DrawAlphaPass();
		featureDrawer->DrawAlphaPass();

		glDisable(GL_CLIP_PLANE3);
	}
}

void CWorldDrawer::DrawMiscObjects() const
{
	SCOPED_GMARKER("WorldDrawer::DrawMiscObjects");

	{
		// note: duplicated in CMiniMap::DrawWorldStuff()
		commandDrawer->DrawLuaQueuedUnitSetCommands();

		if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
			selectedUnitsHandler.DrawCommands();
		}
	}

	// either draw from here, or make {Dyn,Bump}Water use blending
	// pro: icons are drawn only once per frame, not every pass
	// con: looks somewhat worse for underwater / obscured icons
	unitDrawer->DrawUnitIcons();

	lineDrawer.DrawAll();
	cursorIcons.Draw();

	mouse->DrawSelectionBox();
	guihandler->DrawMapStuff(false);

	if (globalRendering->drawMapMarks && !game->hideInterface) {
		inMapDrawerView->Draw();
	}
}



void CWorldDrawer::DrawBelowWaterOverlay() const
{
	SCOPED_GMARKER("WorldDrawer::DrawBelowWaterOverlay");

	if (!globalRendering->drawWater)
		return;
	if (mapInfo->map.voidWater)
		return;
	if (camera->GetPos().y >= 0.0f)
		return;

	{
		glEnableClientState(GL_VERTEX_ARRAY);

		const float3& cpos = camera->GetPos();
		const float vr = globalRendering->viewRange * 0.5f;

		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.5f, 0.3f, 0.50f);

		{
			const float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr)
			};

			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUADS, 0, 4);
		}

		{
			const float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr,  -vr, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x - vr,  -vr, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr,  -vr, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr),
				float3(cpos.x + vr,  -vr, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr,  -vr, cpos.z - vr),
			};

			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUAD_STRIP, 0, 10);
		}

		glDepthMask(GL_TRUE);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	{
		// draw water-coloration quad in raw screenspace
		ResetMVPMatrices();

		glEnableClientState(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.2f, 0.8f, 0.333f);

		const float3 verts[] = {
			float3(0.0f, 0.0f, -1.0f),
			float3(1.0f, 0.0f, -1.0f),
			float3(1.0f, 1.0f, -1.0f),
			float3(0.0f, 1.0f, -1.0f),
		};

		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

