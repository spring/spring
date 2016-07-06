/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IWater.h"
#include "ISky.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "DynWater.h"
#include "RefractWater.h"
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
#include "System/Log/ILog.h"

CONFIG(int, Water)
.defaultValue(IWater::WATER_RENDERER_REFLECTIVE)
.safemodeValue(IWater::WATER_RENDERER_BASIC)
.headlessValue(0)
.minimumValue(0)
.maximumValue(IWater::NUM_WATER_RENDERERS - 1)
.description("Defines the type of water rendering. Can be set in game. Options are: 0 = Basic water, 1 = Reflective water, 2 = Reflective and Refractive water, 3 = Dynamic water, 4 = Bumpmapped water");

IWater* water = nullptr;
static std::vector<int> waterModes;


IWater::IWater()
	: drawReflection(false)
	, drawRefraction(false)
{
	CExplosionCreator::AddExplosionListener(this);
}


void IWater::PushWaterMode(int nxtRendererMode)
{
	waterModes.push_back(nxtRendererMode);
}

void IWater::ApplyPushedChanges(CGame* game) {
	for (auto i = waterModes.cbegin(); i != waterModes.cend(); ++i) {
		water = GetWater(water, *i);
		LOG("Set water rendering mode to %i (%s)", water->GetID(), water->GetName());
	}

	waterModes.clear();
}

IWater* IWater::GetWater(IWater* curRenderer, int nxtRendererMode)
{
	static IWater tmpRenderer;
	static bool allowedModes[NUM_WATER_RENDERERS] = {
		true,
		GLEW_ARB_fragment_program && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/water.fp"),
		GLEW_ARB_fragment_program && GLEW_ARB_texture_float && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/waterDyn.fp"),
		GLEW_ARB_fragment_program && GLEW_ARB_texture_rectangle,
		GLEW_ARB_shading_language_100 && GLEW_ARB_fragment_shader && GLEW_ARB_vertex_shader,
	};

	IWater* nxtRenderer = nullptr;

	// -1 cycles
	if (nxtRendererMode < 0) {
		nxtRendererMode = (curRenderer == nullptr)? Clamp(configHandler->GetInt("Water"), 0, NUM_WATER_RENDERERS - 1): curRenderer->GetID() + 1;
	}
	nxtRendererMode %= NUM_WATER_RENDERERS;

	if (curRenderer != nullptr) {
		assert(water == curRenderer);

		if (curRenderer->GetID() == nxtRendererMode) {
			if (nxtRendererMode == IWater::WATER_RENDERER_BASIC) {
				return curRenderer;
			}
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
					SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "Loading Dynamic Water failed, error: %s", ex.what());
				}
			} break;

			case WATER_RENDERER_BUMPMAPPED: {
				try {
					nxtRenderer = new CBumpWater();
				} catch (const content_error& ex) {
					SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "Loading Bumpmapped Water failed, error: %s", ex.what());
				}
			} break;

			case WATER_RENDERER_REFL_REFR: {
				try {
					nxtRenderer = new CRefractWater();
				} catch (const content_error& ex) {
					SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "Loading Refractive Water failed, error: %s", ex.what());
				}
			} break;

			case WATER_RENDERER_REFLECTIVE: {
				try {
					nxtRenderer = new CAdvWater();
				} catch (const content_error& ex) {
					SafeDelete(nxtRenderer);
					LOG_L(L_ERROR, "Loading Reflective Water failed, error: %s", ex.what());
				}
			} break;
		}
	}

	if (nxtRenderer == nullptr)
		nxtRenderer = new CBasicWater();

	configHandler->Set("Water", nxtRenderer->GetID());
	return nxtRenderer;
}

void IWater::ExplosionOccurred(const CExplosionParams& event) {
	AddExplosion(event.pos, event.damages.GetDefault(), event.craterAreaOfEffect);
}

void IWater::SetModelClippingPlane(const double* planeEq) {
	glPushMatrix();
	glLoadIdentity();
	glClipPlane(GL_CLIP_PLANE2, planeEq);
	glPopMatrix();
}


void IWater::DrawReflections(const double* clipPlaneEqs, bool drawGround, bool drawSky) {
	game->SetDrawMode(CGame::gameReflectionDraw);

	{
		drawReflection = true;

		// opaque; do not clip skydome (is drawn in camera space)
		if (drawSky)
			sky->Draw();

		glEnable(GL_CLIP_PLANE2);
		glClipPlane(GL_CLIP_PLANE2, &clipPlaneEqs[0]);

		if (drawGround)
			readMap->GetGroundDrawer()->Draw(DrawPass::WaterReflection);


		// rest needs the plane in model-space; V is combined with P
		SetModelClippingPlane(&clipPlaneEqs[4]);
		unitDrawer->Draw(true);
		featureDrawer->Draw();

		// transparent
		unitDrawer->DrawAlphaPass();
		featureDrawer->DrawAlphaPass();
		projectileDrawer->Draw(true);
		// sun-disc does not blend well with water
		// sky->DrawSun();

		eventHandler.DrawWorldReflection();
		glDisable(GL_CLIP_PLANE2);

		drawReflection = false;
	}

	game->SetDrawMode(CGame::gameNormalDraw);
}

void IWater::DrawRefractions(const double* clipPlaneEqs, bool drawGround, bool drawSky) {
	game->SetDrawMode(CGame::gameRefractionDraw);

	{
		drawRefraction = true;

		glEnable(GL_CLIP_PLANE2);
		glClipPlane(GL_CLIP_PLANE2, &clipPlaneEqs[0]);

		// opaque
		if (drawSky)
			sky->Draw();
		if (drawGround)
			readMap->GetGroundDrawer()->Draw(DrawPass::WaterRefraction);


		SetModelClippingPlane(&clipPlaneEqs[4]);
		unitDrawer->Draw(false, true);
		featureDrawer->Draw();

		// transparent
		unitDrawer->DrawAlphaPass();
		featureDrawer->DrawAlphaPass();
		projectileDrawer->Draw(false, true);

		eventHandler.DrawWorldRefraction();
		glDisable(GL_CLIP_PLANE2);

		drawRefraction = false;
	}

	game->SetDrawMode(CGame::gameNormalDraw);
}

