/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "IWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "DynWater.h"
#include "RefractWater.h"
#include "Map/ReadMap.h" // struct HeightMapUpdate
#include "Sim/Projectiles/ExplosionListener.h"
#include "Game/GameHelper.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

CONFIG(int, ReflectiveWater).defaultValue(IWater::WATER_RENDERER_REFLECTIVE);

IWater* water = NULL;
static std::vector<int> waterModes;
static std::vector<HeightMapUpdate> heightmapChanges;
bool IWater::noWakeProjectiles = false;


IWater::IWater()
	: drawReflection(false)
	, drawRefraction(false)
 	, drawSolid(false)
{
	helper->AddExplosionListener(this);
}

IWater::~IWater()
{
	if (helper != NULL) {
		helper->RemoveExplosionListener(this);
	}
}


void IWater::PushWaterMode(int nextWaterRenderMode) {
	GML_STDMUTEX_LOCK(water); // PushWaterMode

	waterModes.push_back(nextWaterRenderMode);
}

void IWater::PushHeightmapChange(const int x1, const int y1, const int x2, const int y2) {
	GML_STDMUTEX_LOCK(water); // PushHeightMapChange

	heightmapChanges.push_back(HeightMapUpdate(x1, x2,  y1, y2));
}


void IWater::ApplyPushedChanges(CGame* game) {
	std::vector<int> wm;
	std::vector<HeightMapUpdate> hc;

	{
		GML_STDMUTEX_LOCK(water); // UpdateIWater

		wm.swap(waterModes);
		hc.swap(heightmapChanges);
	}

	for (std::vector<int>::iterator i = wm.begin(); i != wm.end(); ++i) {
		int nextWaterRendererMode = *i;

		if (nextWaterRendererMode < 0) {
			nextWaterRendererMode = (std::max(0, water->GetID()) + 1) % IWater::NUM_WATER_RENDERERS;
		}

		water = GetWater(water, nextWaterRendererMode);
		LOG("Set water rendering mode to %i (%s)", nextWaterRendererMode, water->GetName());
	}

	for (std::vector<HeightMapUpdate>::iterator i = hc.begin(); i != hc.end(); ++i) {
		const HeightMapUpdate& h = *i;
		water->HeightmapChanged(h.x1, h.y1, h.x2, h.y2);
	}
}

IWater* IWater::GetWater(IWater* currWaterRenderer, int nextWaterRendererMode)
{
	static IWater baseWaterRenderer;
	IWater* nextWaterRenderer = NULL;

	if (currWaterRenderer != NULL) {
		assert(water == currWaterRenderer);

		if (currWaterRenderer->GetID() == nextWaterRendererMode) {
			if (nextWaterRendererMode == IWater::WATER_RENDERER_BASIC) {
				return currWaterRenderer;
			}
		}

		// note: rendering thread(s) can concurrently dereference the
		// <water> global, so they may not see destructed memory while
		// it is being reinstantiated through <currWaterRenderer>
		water = &baseWaterRenderer;

		// for BumpWater, this needs to happen before a new renderer
		// instance is created because its shaders must be recompiled
		// (delayed deletion will fail)
		delete currWaterRenderer;
	}

	if (nextWaterRendererMode < IWater::WATER_RENDERER_BASIC) {
		nextWaterRendererMode = configHandler->GetInt("ReflectiveWater");
	}

	switch (nextWaterRendererMode) {
		case WATER_RENDERER_DYNAMIC: {
			const bool canLoad =
				GLEW_ARB_fragment_program &&
				GLEW_ARB_texture_float &&
				ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/waterDyn.fp");

			if (canLoad) {
				try {
					nextWaterRenderer = new CDynWater();
				} catch (const content_error& ex) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					LOG_L(L_ERROR, "Loading Dynamic Water failed, error: %s",
							ex.what());
				}
			}
		} break;

		case WATER_RENDERER_BUMPMAPPED: {
			const bool canLoad =
				GLEW_ARB_shading_language_100 &&
				GL_ARB_fragment_shader &&
				GL_ARB_vertex_shader;

			if (canLoad) {
				try {
					nextWaterRenderer = new CBumpWater();
				} catch (const content_error& ex) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					LOG_L(L_ERROR, "Loading Bumpmapped Water failed, error: %s",
							ex.what());
				}
			}
		} break;

		case WATER_RENDERER_REFL_REFR: {
			const bool canLoad =
				GLEW_ARB_fragment_program &&
				GLEW_ARB_texture_rectangle;

			if (canLoad) {
				try {
					nextWaterRenderer = new CRefractWater();
				} catch (const content_error& ex) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					LOG_L(L_ERROR, "Loading Refractive Water failed, error: %s",
							ex.what());
				}
			}
		} break;

		case WATER_RENDERER_REFLECTIVE: {
			const bool canLoad =
				GLEW_ARB_fragment_program &&
				ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/water.fp");

			if (canLoad) {
				try {
					nextWaterRenderer = new CAdvWater();
				} catch (const content_error& ex) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					LOG_L(L_ERROR, "Loading Reflective Water failed, error: %s",
							ex.what());
				}
			}
		} break;
	}

	if (nextWaterRenderer == NULL) {
		nextWaterRenderer = new CBasicWater();
	}

	configHandler->Set("ReflectiveWater", nextWaterRenderer->GetID());
	return nextWaterRenderer;
}

void IWater::ExplosionOccurred(const CExplosionEvent& event) {
	AddExplosion(event.GetPos(), event.GetDamage(), event.GetRadius());
}
