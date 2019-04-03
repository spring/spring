/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IWater.h"
#include "ISky.h"
#include "LuaWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "DynWater.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

CONFIG(int, Water)
.defaultValue(IWater::WATER_RENDERER_REFLECTIVE)
.safemodeValue(IWater::WATER_RENDERER_LUA)
.headlessValue(0)
.minimumValue(0)
.maximumValue(IWater::NUM_WATER_RENDERERS - 1)
.description("Defines the type of water rendering. Can be set in game. Options are: 0 = No water, 1 = Reflective water, 2 = Reflective and Refractive water, 3 = Dynamic water, 4 = Bumpmapped water");


static std::array<int, 4> waterModes = {{0}};

static IWater tmpRenderer(false);
static bool allowedModes[IWater::NUM_WATER_RENDERERS] = {
	true,
	true,
	false,
	true,
	true,
};

IWater* water = nullptr;



IWater::IWater(bool wantEvents)
{
	if (wantEvents)
		CExplosionCreator::AddExplosionListener(this);
}


void IWater::PushWaterMode(int nxtRendererMode)
{
	if (waterModes[0] == int(waterModes.size() - 1))
		return;

	waterModes[ ++waterModes[0] ] = nxtRendererMode;
}

void IWater::ApplyPushedChanges(CGame* game) {
	for (int i = 1, n = waterModes[0]; i <= n; i++) {
		water = GetWater(water, waterModes[i]);
		LOG("Set water rendering mode to %i (%s)", water->GetID(), water->GetName());
	}

	waterModes.fill(0);
}

IWater* IWater::GetWater(IWater* curRenderer, int nxtRendererMode)
{
	IWater* nxtRenderer = nullptr;

	// -1 cycles
	if (nxtRendererMode < 0)
		nxtRendererMode = (curRenderer == nullptr)? Clamp(configHandler->GetInt("Water"), 0, NUM_WATER_RENDERERS - 1): curRenderer->GetID() + 1;

	nxtRendererMode %= NUM_WATER_RENDERERS;

	if (curRenderer != nullptr) {
		assert(water == curRenderer);

		if (curRenderer->GetID() == nxtRendererMode) {
			if (nxtRendererMode == IWater::WATER_RENDERER_LUA)
				return curRenderer;
		}

		// note: rendering thread(s) can concurrently dereference the
		// <water> global, so they may not see destructed memory while
		// it is being reinstantiated through <curRenderer>
		water = &tmpRenderer;

		// for BumpWater, this needs to happen before a new renderer
		// instance is created because its shaders must be recompiled
		// (delayed deletion will fail)
		delete curRenderer;
	}

	if (allowedModes[nxtRendererMode]) {
		switch (nxtRendererMode) {
			case WATER_RENDERER_DYNAMIC: {
				try {
					nxtRenderer = new CDynWater();
				} catch (const content_error& ex) {
					spring::SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "[%s] loading Dynamic Water failed, error: %s", __func__, ex.what());
				}
			} break;

			case WATER_RENDERER_BUMPMAPPED: {
				try {
					nxtRenderer = new CBumpWater();
				} catch (const content_error& ex) {
					spring::SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "[%s] loading Bumpmapped Water failed, error: %s", __func__, ex.what());
				}
			} break;

			case WATER_RENDERER_REFRACTIVE: {
				try {
					nxtRenderer = new CAdvWater(true);
				} catch (const content_error& ex) {
					spring::SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "[%s] loading Refractive Water failed, error: %s", __func__, ex.what());
				}
			} break;

			case WATER_RENDERER_REFLECTIVE: {
				try {
					nxtRenderer = new CAdvWater(false);
				} catch (const content_error& ex) {
					spring::SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "[%s] loading Reflective Water failed, error: %s", __func__, ex.what());
				}
			} break;
		}
	}

	if (nxtRenderer == nullptr)
		nxtRenderer = new CLuaWater();

	configHandler->Set("Water", nxtRenderer->GetID());
	return nxtRenderer;
}

void IWater::ExplosionOccurred(const CExplosionParams& event) {
	AddExplosion(event.pos, event.damages.GetDefault(), event.craterAreaOfEffect);
}


void IWater::DrawReflections(bool drawGround, bool drawSky) {
	game->SetDrawMode(Game::ReflectionDraw);

	{
		drawReflection = true;

		// opaque; do not clip skydome (is drawn in camera space)
		if (drawSky)
			sky->Draw(Game::ReflectionDraw);

		glEnable(GL_CLIP_DISTANCE0 + ClipPlaneIndex());

		if (drawGround)
			readMap->GetGroundDrawer()->Draw(DrawPass::WaterReflection);

		// Draw*Pass sets up the clipping plane
		unitDrawer->Draw();
		featureDrawer->Draw();

		// transparent
		unitDrawer->DrawAlphaPass(true);
		featureDrawer->DrawAlphaPass(true);
		projectileDrawer->Draw(true);
		// sun-disc does not blend well with water
		// sky->DrawSun(Game::ReflectionDraw);

		eventHandler.DrawWorldReflection();

		glDisable(GL_CLIP_DISTANCE0 + ClipPlaneIndex());

		drawReflection = false;
	}

	game->SetDrawMode(Game::NormalDraw);
}

void IWater::DrawRefractions(bool drawGround, bool drawSky) {
	game->SetDrawMode(Game::RefractionDraw);

	{
		drawRefraction = true;

		// opaque
		if (drawSky)
			sky->Draw(Game::RefractionDraw);

		glEnable(GL_CLIP_DISTANCE0 + ClipPlaneIndex());

		if (drawGround)
			readMap->GetGroundDrawer()->Draw(DrawPass::WaterRefraction);

		unitDrawer->Draw();
		featureDrawer->Draw();

		// transparent
		unitDrawer->DrawAlphaPass(false);
		featureDrawer->DrawAlphaPass(false);
		projectileDrawer->Draw(false, true);

		eventHandler.DrawWorldRefraction();

		glDisable(GL_CLIP_DISTANCE0 + ClipPlaneIndex());

		drawRefraction = false;
	}

	game->SetDrawMode(Game::NormalDraw);
}

