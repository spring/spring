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

// not used by shaders, only for the advshading=0 case!
static void SetupUnitLightFFP(const CMapInfo::light_t& light)
{
	glLightfv(GL_LIGHT1, GL_AMBIENT, light.unitAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light.unitSunColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light.unitAmbientColor);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
}



CWorldDrawer::CWorldDrawer(): numUpdates(0)
{
	LuaObjectDrawer::Init();
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
	SafeDelete(modelParser);

	SafeDelete(farTextureHandler);
	SafeDelete(heightMapTexture);

	SafeDelete(texturehandler3DO);
	SafeDelete(texturehandlerS3O);

	SafeDelete(cubeMapHandler);

	readMap->KillGroundDrawer();
	IGroundDecalDrawer::FreeInstance();
	CShaderHandler::FreeInstance(CShaderHandler::GetInstance());
	LuaObjectDrawer::Kill();
}



void CWorldDrawer::LoadPre() const
{
	// these need to be loaded before featureHandler is created
	// (maps with features have their models loaded at startup)
	modelParser = new C3DModelLoader();

	loadscreen->SetLoadMessage("Creating Unit Textures");
	texturehandler3DO = new C3DOTextureHandler();
	texturehandlerS3O = new CS3OTextureHandler();

	featureDrawer = new CFeatureDrawer();
	loadscreen->SetLoadMessage("Creating Sky");
	sky = ISky::GetSky();
}

void CWorldDrawer::LoadPost() const
{
	loadscreen->SetLoadMessage("Creating ShadowHandler & DecalHandler");
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

	loadscreen->SetLoadMessage("Creating ProjectileDrawer & UnitDrawer");
	projectileDrawer = new CProjectileDrawer();
	projectileDrawer->LoadWeaponTextures();

	unitDrawer = new CUnitDrawer();
	// see ::LoadPre
	// featureDrawer = new CFeatureDrawer();
	modelDrawer = IModelDrawer::GetInstance();

	loadscreen->SetLoadMessage("Creating Water");
	water = IWater::GetWater(NULL, -1);

	SetupUnitLightFFP(mapInfo->light);
	ISky::SetupFog();
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
	DrawOpaqueObjects();
	DrawAlphaObjects();

	{
		SCOPED_TIMER("WorldDrawer::Projectiles");
		projectileDrawer->Draw(false);
	}

	sky->DrawSun();

	eventHandler.DrawWorld();

	DrawMiscObjects();
	DrawBelowWaterOverlay();
}


void CWorldDrawer::DrawOpaqueObjects() const
{
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	sky->Draw();

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
}

void CWorldDrawer::DrawAlphaObjects() const
{
	// transparent objects
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	static const double plane_below[4] = {0.0f, -1.0f, 0.0f, 0.0f};
	static const double plane_above[4] = {0.0f,  1.0f, 0.0f, 0.0f};

	{
		glClipPlane(GL_CLIP_PLANE3, plane_below);
		glEnable(GL_CLIP_PLANE3);

		// draw cloaked objects below water surface
		unitDrawer->DrawAlphaPass(shadowHandler->shadowsLoaded);
		featureDrawer->DrawAlphaPass(shadowHandler->shadowsLoaded);

		glDisable(GL_CLIP_PLANE3);
	}

	// draw water
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

		// draw cloaked objects above water surface
		unitDrawer->DrawAlphaPass(shadowHandler->shadowsLoaded);
		featureDrawer->DrawAlphaPass(shadowHandler->shadowsLoaded);

		glDisable(GL_CLIP_PLANE3);
	}
}

void CWorldDrawer::DrawMiscObjects() const
{
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
}



void CWorldDrawer::DrawBelowWaterOverlay() const
{
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

