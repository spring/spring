/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "BaseWater.h"
#include "BasicWater.h"
#include "AdvWater.h"
#include "BumpWater.h"
#include "DynWater.h"
#include "RefractWater.h"
#include "Map/ReadMap.h" // struct HeightMapUpdate
#include "Sim/Projectiles/ExplosionListener.h"
#include "Game/GameHelper.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"

CBaseWater* water = NULL;
static std::vector<int> waterModes;
static std::vector<HeightMapUpdate> heightmapChanges;
bool CBaseWater::noWakeProjectiles = false;

CBaseWater::CBaseWater()
	: drawReflection(false)
	, drawRefraction(false)
 	, drawSolid(false)
{
	helper->AddExplosionListener(this);
}

CBaseWater::~CBaseWater()
{
	if (helper != NULL) helper->RemoveExplosionListener(this);
}


void CBaseWater::PushWaterMode(int nextWaterRenderMode) {
	GML_STDMUTEX_LOCK(water); // PushWaterMode

	waterModes.push_back(nextWaterRenderMode);
}

void CBaseWater::PushHeightmapChange(const int x1, const int y1, const int x2, const int y2) {
	GML_STDMUTEX_LOCK(water); // PushHeightMapChange

	heightmapChanges.push_back(HeightMapUpdate(x1, x2,  y1, y2));
}


void CBaseWater::ApplyPushedChanges(CGame* game) {
	std::vector<int> wm;
	std::vector<HeightMapUpdate> hc;

	{
		GML_STDMUTEX_LOCK(water); // UpdateBaseWater

		wm.swap(waterModes);
		hc.swap(heightmapChanges);
	}

	for (std::vector<int>::iterator i = wm.begin(); i != wm.end(); ++i) {
		int nextWaterRendererMode = *i;

		if (nextWaterRendererMode < 0)
			nextWaterRendererMode = (std::max(0, water->GetID()) + 1) % CBaseWater::NUM_WATER_RENDERERS;

		water = GetWater(water, nextWaterRendererMode);
		logOutput.Print("Set water rendering mode to %i (%s)", nextWaterRendererMode, water->GetName());
	}

	for (std::vector<HeightMapUpdate>::iterator i = hc.begin(); i != hc.end(); ++i) {
		const HeightMapUpdate& h = *i;
		water->HeightmapChanged(h.x1, h.y1, h.x2, h.y2);
	}
}

CBaseWater* CBaseWater::GetWater(CBaseWater* currWaterRenderer, int nextWaterRendererMode)
{
	static CBaseWater baseWaterRenderer;
	CBaseWater* nextWaterRenderer = NULL;

	if (currWaterRenderer != NULL) {
		assert(water == currWaterRenderer);

		if (currWaterRenderer->GetID() == nextWaterRendererMode) {
			if (nextWaterRendererMode == CBaseWater::WATER_RENDERER_BASIC) {
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

	if (nextWaterRendererMode < CBaseWater::WATER_RENDERER_BASIC) {
		nextWaterRendererMode = configHandler->Get("ReflectiveWater", int(CBaseWater::WATER_RENDERER_REFLECTIVE));
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
				} catch (content_error& e) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					logOutput.Print("Loading Dynamic Water failed");
					logOutput.Print("Error: %s", e.what());
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
				} catch (content_error& e) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					logOutput.Print("Loading Bumpmapped Water failed");
					logOutput.Print("Error: %s", e.what());
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
				} catch (content_error& e) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					logOutput.Print("Loading Refractive Water failed");
					logOutput.Print("Error: %s", e.what());
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
				} catch (content_error& e) {
					delete nextWaterRenderer;
					nextWaterRenderer = NULL;
					logOutput.Print("Loading Reflective Water failed");
					logOutput.Print("Error: %s", e.what());
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

void CBaseWater::ExplosionOccurred(const CExplosionEvent& event) {
	AddExplosion(event.GetPos(), event.GetDamage(), event.GetRadius());
}
