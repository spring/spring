/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"


#include "WorldDrawer.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/DebugColVolDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/SmoothHeightMeshDrawer.h"
#include "Rendering/InMapDrawView.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Models/ModelDrawer.h"
#include "Rendering/Shaders/ShaderHandler.h"
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
	// rendering components
	loadscreen->SetLoadMessage("Creating ShadowHandler & DecalHandler");
	cubeMapHandler = new CubeMapHandler();
	shadowHandler = new CShadowHandler();

	loadscreen->SetLoadMessage("Creating GroundDrawer");
	readMap->NewGroundDrawer();

	loadscreen->SetLoadMessage("Creating TreeDrawer");
	treeDrawer = ITreeDrawer::GetTreeDrawer();
	grassDrawer = new CGrassDrawer();

	inMapDrawerView = new CInMapDrawView();
	pathDrawer = IPathDrawer::GetInstance();

	farTextureHandler = new CFarTextureHandler();
	heightMapTexture = new HeightMapTexture();

	loadscreen->SetLoadMessage("Creating ProjectileDrawer & UnitDrawer");
	projectileDrawer = new CProjectileDrawer();
	projectileDrawer->LoadWeaponTextures();

	unitDrawer = new CUnitDrawer();
	// FIXME: see CGame::LoadSimulation (we only delete it)
	// featureDrawer = new CFeatureDrawer();
	modelDrawer = IModelDrawer::GetInstance();

	loadscreen->SetLoadMessage("Creating Water");
	water = IWater::GetWater(NULL, -1);
}


CWorldDrawer::~CWorldDrawer()
{
	SafeDelete(water);
	SafeDelete(sky);
	SafeDelete(treeDrawer);
	SafeDelete(grassDrawer);
	SafeDelete(pathDrawer);
	SafeDelete(modelDrawer);
	SafeDelete(shadowHandler);
	SafeDelete(inMapDrawerView);

	SafeDelete(featureDrawer);
	SafeDelete(unitDrawer); // depends on unitHandler, cubeMapHandler
	SafeDelete(projectileDrawer);

	SafeDelete(farTextureHandler);
	SafeDelete(heightMapTexture);

	SafeDelete(cubeMapHandler);
	IGroundDecalDrawer::FreeInstance();
	CShaderHandler::FreeInstance(CShaderHandler::GetInstance());
	LuaObjectDrawer::Kill();
}


void CWorldDrawer::Update()
{
	LuaObjectDrawer::Update(numUpdates == 0);
	readMap->UpdateDraw(numUpdates == 0);

	if (globalRendering->drawGround) {
		SCOPED_TIMER("GroundDrawer::Update");
		CBaseGroundDrawer* gd = readMap->GetGroundDrawer();
		gd->Update();
	}

	//XXX: do in CGame, cause it needs to get updated even when doDrawWorld==false, cause it updates unitdrawpos which is used for maximized minimap too
	//unitDrawer->Update();
	//lineDrawer.UpdateLineStipple();
	treeDrawer->Update();
	featureDrawer->Update();
	IWater::ApplyPushedChanges(game);

	numUpdates += 1;
}


void CWorldDrawer::GenerateIBL()
{
	if (shadowHandler->shadowsLoaded) {
		SCOPED_TIMER("ShadowHandler::CreateShadows");
		game->SetDrawMode(CGame::gameShadowDraw);
		shadowHandler->CreateShadows();
		game->SetDrawMode(CGame::gameNormalDraw);
	}

	{
		SCOPED_TIMER("CubeMapHandler::UpdateReflTex");
		cubeMapHandler->UpdateReflectionTexture();
	}

	if (sky->GetLight()->IsDynamic()) {
		{
			SCOPED_TIMER("CubeMapHandler::UpdateSpecTex");
			cubeMapHandler->UpdateSpecularTexture();
		}
		{
			SCOPED_TIMER("Sky::UpdateSkyTex");
			sky->UpdateSkyTexture();
		}
		{
			SCOPED_TIMER("ReadMap::UpdateShadingTex");
			readMap->UpdateShadingTexture();
		}
	}

	if (FBO::IsSupported())
		FBO::Unbind();

	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
}


void CWorldDrawer::Draw()
{
	SCOPED_TIMER("WorldDrawer::Total");

	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	if (globalRendering->drawSky) {
		sky->Draw();
	}

	if (globalRendering->drawGround) {
		{
			SCOPED_TIMER("WorldDrawer::Terrain");
			gd->Draw(DrawPass::Normal);
		}
		{
			SCOPED_TIMER("WorldDrawer::GroundDecals");
			groundDecals->Draw();
			projectileDrawer->DrawGroundFlashes();
		}
		{
			SCOPED_TIMER("WorldDrawer::Foliage");
			grassDrawer->Draw();
			gd->DrawTrees();
		}
		smoothHeightMeshDrawer->Draw(1.0f);
	}

	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("WorldDrawer::Water");

		water->OcclusionQuery();
		if (water->DrawSolid()) {
			water->UpdateWater(game);
			water->Draw();
		}
	}

	selectedUnitsHandler.Draw();
	eventHandler.DrawWorldPreUnit();

	{
		SCOPED_TIMER("WorldDrawer::Models");
		unitDrawer->Draw(false);
		modelDrawer->Draw();
		featureDrawer->Draw();
		DebugColVolDrawer::Draw();

		pathDrawer->DrawAll();
	}

	//! transparent stuff
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	static const double plane_below[4] = {0.0f, -1.0f, 0.0f, 0.0f};
	static const double plane_above[4] = {0.0f,  1.0f, 0.0f, 0.0f};

	{
		glClipPlane(GL_CLIP_PLANE3, plane_below);
		glEnable(GL_CLIP_PLANE3);

		//! draw cloaked objects below water surface
		unitDrawer->DrawCloakedUnits(shadowHandler->shadowsLoaded);
		featureDrawer->DrawFadeFeatures(shadowHandler->shadowsLoaded);

		glDisable(GL_CLIP_PLANE3);
	}

	//! draw water
	if (globalRendering->drawWater && !mapInfo->map.voidWater) {
		SCOPED_TIMER("WorldDrawer::Water");

		if (!water->DrawSolid()) {
			water->UpdateWater(game);
			water->Draw();
		}
	}

	{
		glClipPlane(GL_CLIP_PLANE3, plane_above);
		glEnable(GL_CLIP_PLANE3);

		//! draw cloaked objects above water surface
		unitDrawer->DrawCloakedUnits(shadowHandler->shadowsLoaded);
		featureDrawer->DrawFadeFeatures(shadowHandler->shadowsLoaded);

		glDisable(GL_CLIP_PLANE3);
	}

	{
		SCOPED_TIMER("WorldDrawer::Projectiles");
		projectileDrawer->Draw(false);
	}

	if (globalRendering->drawSky) {
		sky->DrawSun();
	}

	eventHandler.DrawWorld();

	{
		// note: duplicated in CMiniMap::DrawWorldStuff()
		commandDrawer->DrawLuaQueuedUnitSetCommands();

		if (cmdColors.AlwaysDrawQueue() || guihandler->GetQueueKeystate()) {
			selectedUnitsHandler.DrawCommands();
		}
	}

	lineDrawer.DrawAll();
	cursorIcons.Draw();
	cursorIcons.Clear();

	mouse->DrawSelectionBox();

	guihandler->DrawMapStuff(false);

	if (globalRendering->drawMapMarks && !game->hideInterface) {
		inMapDrawerView->Draw();
	}


	//! underwater overlay
	if (camera->GetPos().y < 0.0f && globalRendering->drawWater && !mapInfo->map.voidWater) {
		glEnableClientState(GL_VERTEX_ARRAY);
		const float3& cpos = camera->GetPos();
		const float vr = globalRendering->viewRange * 0.5f;
		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.5f, 0.3f, 0.50f);
		{
			float3 verts[] = {
				float3(cpos.x - vr, 0.0f, cpos.z - vr),
				float3(cpos.x - vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z + vr),
				float3(cpos.x + vr, 0.0f, cpos.z - vr)
			};
			glVertexPointer(3, GL_FLOAT, 0, verts);
			glDrawArrays(GL_QUADS, 0, 4);
		}

		{
			float3 verts[] = {
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

	//reset fov
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// underwater overlay, part 2
	if (camera->GetPos().y < 0.0f && globalRendering->drawWater && !mapInfo->map.voidWater) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.2f, 0.8f, 0.333f);
		float3 verts[] = {
			float3 (0.f, 0.f, -1.f),
			float3 (1.f, 0.f, -1.f),
			float3 (1.f, 1.f, -1.f),
			float3 (0.f, 1.f, -1.f),
		};
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}
