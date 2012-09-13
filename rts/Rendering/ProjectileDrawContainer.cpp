/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if 0
ProjectileDrawer draw sequence:
	Draw() currently looks like
		// zSortedProjectiles.clear();
		// ...
		// <modelRenderers> stores projectiles with a non-NULL model
		// NOTE: DrawProjectiles() also calls DrawFlyingPieces
		//
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			modelRenderers[modelType]->PushRenderState();
			DrawProjectiles(modelType, numFlyingPieces, &drawnPieces, drawReflection, drawRefraction);
			modelRenderers[modelType]->PopRenderState();
		}
		// ...
		// <renderProjectiles> stores projectiles with a NULL model
		DrawProjectilesSet(renderProjectiles, drawReflection, drawRefraction);
		// ...
		// zSortedProjectiles is filled by DrawProjectile, which only actually
		// draws projectiles with non-NULL models -- the rest (all projectiles
		// without models) get drawn here
		for (std::set<CProjectile*, distcmp>::iterator it = zSortedProjectiles.begin(); it != zSortedProjectiles.end(); ++it) {
			(*it)->Draw();
		}


	replacement Draw() would look like
		unitDrawer->SetupForUnitDrawing();

		// draw MODELS for those projectiles that have them
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			modelRenderers[modelType]->PushRenderState();

			for (int projectileType = CProjectile::UNSYNCED_BUBBLE_PROJECTILE; projectileType < CProjectile::NUM_PROJECTILE_TYPES; projectileType++) {
				projectileDrawContainers[modelType][projectileType]->Draw(modelType, drawReflection, drawRefraction);
			}

			modelRenderers[modelType]->PopRenderState();
		}

		unitDrawer->CleanUpUnitDrawing();

		// draw EFFECTS for all projectiles (whether they have models or not) in-order FIXME
		for (int projectileType = CProjectile::UNSYNCED_BUBBLE_PROJECTILE; projectileType < CProjectile::NUM_PROJECTILE_TYPES; projectileType++) {
			projectileDrawContainers[MODELTYPE_OTHER][projectileType]->Draw(MODELTYPE_OTHER, drawReflection, drawRefraction);
		}
#endif






#include <cassert>
#include <set>

#include "ProjectileDrawContainer.h"
#include "GlobalRendering.h"
#include "Models/WorldObjectModelRenderer.h" // for MDL_TYPE and TEX_TYPE macros
#include "Textures/S3OTextureHandler.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"

#include "Sim/Projectiles/FireProjectile.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/PieceProjectile.h"

#include "Sim/Projectiles/WeaponProjectiles/EmgProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/ExplosiveProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/FireBallProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/FlameProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LargeBeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LightningProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"

#include "Sim/Projectiles/Unsynced/BubbleProjectile.h"
#include "Sim/Projectiles/Unsynced/DirtProjectile.h"
#include "Sim/Projectiles/Unsynced/ExploSpikeProjectile.h"
#include "Sim/Projectiles/Unsynced/GenericParticleProjectile.h"
#include "Sim/Projectiles/Unsynced/GeoSquareProjectile.h"
#include "Sim/Projectiles/Unsynced/GeoThermSmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/GfxProjectile.h"
#include "Sim/Projectiles/Unsynced/RepulseGfx.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/MuzzleFlame.h"
#include "Sim/Projectiles/Unsynced/BitmapMuzzleFlame.h"
#include "Sim/Projectiles/Unsynced/ShieldProjectile.h"
#include "Sim/Projectiles/Unsynced/SimpleParticleSystem.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile2.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Projectiles/Unsynced/SpherePartProjectile.h"
#include "Sim/Projectiles/Unsynced/TracerProjectile.h"
#include "Sim/Projectiles/Unsynced/WakeProjectile.h"
#include "Sim/Projectiles/Unsynced/WreckProjectile.h"

#include "System/Misc/SpringTime.h"



template<typename ProjectileTypeName> class ProjectileDrawContainer: public IProjectileDrawContainer {
public:
	typedef ProjectileTypeName ProjectileType;

	~ProjectileDrawContainer() {
		projectiles.clear();
	}

	void DrawNormal(unsigned int modelType, bool drawReflection, bool drawRefraction) {
		const bool setTexture = (modelType != MODELTYPE_3DO);
		const bool drawModels = (modelType != MODELTYPE_OTHER);

		if (drawModels) {
			/// zSortedProjectiles.clear();

			if (projectiles.empty())
				return;

			typename std::set<ProjectileTypeName*>::iterator it;

			unsigned int prevTexType = -1u;
			unsigned int currTexType = -1u;

			float timeAlpha = globalRendering->timeOffset;

			for (it = projectiles.begin(); it != projectiles.end(); ++it) {
				ProjectileTypeName* p = *it;

				{
					// NOTE:
					//   the ally-check means we can see any projectile fired by an
					//   allied unit (including our own) anywhere on the map so long
					//   as it is within camera bounds, do we still want this?
					//   ... "|| (owner != NULL && teamHandler->Ally(owner->allyteam, gu->myAllyTeam))"
					const CUnit* owner = p->owner();
					const bool inLOS = (gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam));

					if (!inLOS)
						continue;
					if (!camera->InView(p->pos, p->drawRadius))
						continue;
				}

				#if 0
				if (drawReflection) {
					// ??
					const float dif = p->pos.y - camera->pos.y;
					const float3 zeroPos = camera->pos * (p->pos.y / dif)  +  p->pos * (-camera->pos.y / dif);

					if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z, false) > (3.0f + 0.5f * p->drawRadius))
						continue;
				}
				#endif

				if (drawReflection && (p->pos.y < -p->drawRadius))
					continue;
				if (drawRefraction && (p->pos.y >  p->drawRadius))
					continue;

				if (GML::SimEnabled()) {
					timeAlpha = (spring_tomsecs(globalRendering->lastFrameStart) - p->lastProjUpdate) * globalRendering->weightedSpeedFactor;
				}

				assert(p->model != NULL);

				p->drawPos = p->pos + (p->speed * timeAlpha);
				p->tempdist = p->pos.dot(camera->forward);

				// need this to draw the effects in-order later during the MODELTYPE_OTHER pass
				// FIXME:
				//   we have projectiles without models whose effects need to be drawn
				//   we also have projectiles WITH models whose effects need to be drawn (plus their models)
				//   we (ab)use MODELTYPE_OTHER to store projectiles without models, but that is not enough
				// zSortedProjectiles.insert(p);

				if (setTexture) {
					currTexType = TEX_TYPE(p);

					if (prevTexType != currTexType) {
						texturehandlerS3O->SetS3oTexture(currTexType);
					}

					prevTexType = currTexType;
				}

				DrawProjectileModel(p);
			}
		} else {
			typename std::set<CProjectile*>::iterator it;

		// AARGH NOW WE NEED THE SPECIFIC DERIVED TYPE BUT ZSORTEDPROJECTILES ONLY CONTAINS BASETYPES
		// IT CAN NOT CONTAIN ANYTHING ELSE BECAUSE WE FURTHERMORE NEED THE ORDER ACROSS _ALL_ CONTAINERS --> ONE GLOBAL SORT
		///	for (it = zSortedProjectiles.begin(); it != zSortedProjectiles.end(); ++it) {
		///		DrawProjectileEffect(*it);
		///	}
		}
	}

	void DrawShadow(unsigned int modelType) {
		typename std::set<ProjectileTypeName*>::iterator it;

		if (projectiles.empty())
			return;

		for (it = projectiles.begin(); it != projectiles.end(); ++it) {
			ProjectileTypeName* p = *it;

			// ... "|| (owner != NULL && teamHandler->Ally(owner->allyteam, gu->myAllyTeam))"
			const CUnit* owner = p->owner();
			const bool inLOS = (gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam));

			if (!p->castShadow)
				continue;
			if (!inLOS)
				continue;
			if (!camera->InView(p->pos, p->drawRadius))
				continue;

			if (p->model != NULL) {
				assert(modelType != MODELTYPE_OTHER);
				DrawProjectileModel(p);
			}

			// for the shadow-pass we do not (need to) care about z-ordering first
			DrawProjectileEffect(p);
		}
	}

	void AddProjectile(const CProjectile* p) {
		assert(dynamic_cast<const ProjectileTypeName*>(p) != NULL);
		projectiles.insert(static_cast<ProjectileTypeName*>(const_cast<CProjectile*>(p)));
	}
	void DelProjectile(const CProjectile* p) {
		assert(dynamic_cast<const ProjectileTypeName*>(p) != NULL);
		projectiles.erase(static_cast<ProjectileTypeName*>(const_cast<CProjectile*>(p)));
	}

private:
	void DrawProjectileModel(ProjectileTypeName* p) {} // TODO, see ProjectileDrawer
	void DrawProjectileEffect(ProjectileTypeName* p); // TODO, see Sim/Projectiles/*

private:
	struct MdlTypeCmp {
		bool operator() (const ProjectileTypeName* p, const ProjectileTypeName* q) const {
			if (p->model == NULL || q->model == NULL)
				return (p > q);

			return (MDL_TYPE(p) > MDL_TYPE(q));
		}
	};
	struct TexTypeCmp {
		bool operator() (const ProjectileTypeName* p, const ProjectileTypeName* q) const {
			if (p->model == NULL || q->model == NULL)
				return (p > q);

			return (TEX_TYPE(p) > TEX_TYPE(q));
		}
	};

	// keep the projectiles sorted by texture-type (if any)
	std::set<ProjectileTypeName*, TexTypeCmp> projectiles;
};












template<> void ProjectileDrawContainer<         CBubbleProjectile>::DrawProjectileEffect(         CBubbleProjectile* p) {}
template<> void ProjectileDrawContainer<           CDirtProjectile>::DrawProjectileEffect(           CDirtProjectile* p) {}
template<> void ProjectileDrawContainer<     CExploSpikeProjectile>::DrawProjectileEffect(     CExploSpikeProjectile* p) {}
template<> void ProjectileDrawContainer<CGenericParticleProjectile>::DrawProjectileEffect(CGenericParticleProjectile* p) {}
template<> void ProjectileDrawContainer<      CGeoSquareProjectile>::DrawProjectileEffect(      CGeoSquareProjectile* p) {}
template<> void ProjectileDrawContainer<  CGeoThermSmokeProjectile>::DrawProjectileEffect(  CGeoThermSmokeProjectile* p) {}
template<> void ProjectileDrawContainer<            CGfxProjectile>::DrawProjectileEffect(            CGfxProjectile* p) {}
template<> void ProjectileDrawContainer<               CRepulseGfx>::DrawProjectileEffect(               CRepulseGfx* p) {}
template<> void ProjectileDrawContainer<      CHeatCloudProjectile>::DrawProjectileEffect(      CHeatCloudProjectile* p) {}
template<> void ProjectileDrawContainer<              CMuzzleFlame>::DrawProjectileEffect(              CMuzzleFlame* p) {}
template<> void ProjectileDrawContainer<        CBitmapMuzzleFlame>::DrawProjectileEffect(        CBitmapMuzzleFlame* p) {}
template<> void ProjectileDrawContainer<          ShieldProjectile>::DrawProjectileEffect(          ShieldProjectile* p) {}
template<> void ProjectileDrawContainer<   ShieldSegmentProjectile>::DrawProjectileEffect(   ShieldSegmentProjectile* p) {}
template<> void ProjectileDrawContainer<     CSimpleParticleSystem>::DrawProjectileEffect(     CSimpleParticleSystem* p) {}
template<> void ProjectileDrawContainer<          CSmokeProjectile>::DrawProjectileEffect(          CSmokeProjectile* p) {}
template<> void ProjectileDrawContainer<         CSmokeProjectile2>::DrawProjectileEffect(         CSmokeProjectile2* p) {}
template<> void ProjectileDrawContainer<     CSmokeTrailProjectile>::DrawProjectileEffect(     CSmokeTrailProjectile* p) {}
template<> void ProjectileDrawContainer<     CSpherePartProjectile>::DrawProjectileEffect(     CSpherePartProjectile* p) {}
template<> void ProjectileDrawContainer<         CTracerProjectile>::DrawProjectileEffect(         CTracerProjectile* p) {}
template<> void ProjectileDrawContainer<           CWakeProjectile>::DrawProjectileEffect(           CWakeProjectile* p) {}
template<> void ProjectileDrawContainer<          CWreckProjectile>::DrawProjectileEffect(          CWreckProjectile* p) {}

template<> void ProjectileDrawContainer< CFireProjectile>::DrawProjectileEffect( CFireProjectile* p) {}
template<> void ProjectileDrawContainer<CFlareProjectile>::DrawProjectileEffect(CFlareProjectile* p) {}
template<> void ProjectileDrawContainer<CPieceProjectile>::DrawProjectileEffect(CPieceProjectile* p) {}

template<> void ProjectileDrawContainer<        CWeaponProjectile>::DrawProjectileEffect(        CWeaponProjectile* p) {}
template<> void ProjectileDrawContainer<           CEmgProjectile>::DrawProjectileEffect(           CEmgProjectile* p) {}
template<> void ProjectileDrawContainer<     CExplosiveProjectile>::DrawProjectileEffect(     CExplosiveProjectile* p) {}
template<> void ProjectileDrawContainer<      CFireBallProjectile>::DrawProjectileEffect(      CFireBallProjectile* p) {}
template<> void ProjectileDrawContainer<         CFlameProjectile>::DrawProjectileEffect(         CFlameProjectile* p) {}
template<> void ProjectileDrawContainer<         CLaserProjectile>::DrawProjectileEffect(         CLaserProjectile* p) {}
template<> void ProjectileDrawContainer<     CBeamLaserProjectile>::DrawProjectileEffect(     CBeamLaserProjectile* p) {}
template<> void ProjectileDrawContainer<CLargeBeamLaserProjectile>::DrawProjectileEffect(CLargeBeamLaserProjectile* p) {}
template<> void ProjectileDrawContainer<     CLightningProjectile>::DrawProjectileEffect(     CLightningProjectile* p) {}
template<> void ProjectileDrawContainer<       CMissileProjectile>::DrawProjectileEffect(       CMissileProjectile* p) {}
template<> void ProjectileDrawContainer<     CStarburstProjectile>::DrawProjectileEffect(     CStarburstProjectile* p) {}
template<> void ProjectileDrawContainer<       CTorpedoProjectile>::DrawProjectileEffect(       CTorpedoProjectile* p) {}












IProjectileDrawContainer* IProjectileDrawContainer::GetInstance(unsigned int projectileType) {
	IProjectileDrawContainer* pdc = NULL;

	#define PDC ProjectileDrawContainer
	switch (projectileType) {
		case CProjectile::UNSYNCED_BUBBLE_PROJECTILE:               { pdc = new PDC<         CBubbleProjectile>(); } break;
		case CProjectile::UNSYNCED_DIRT_PROJECTILE:                 { pdc = new PDC<           CDirtProjectile>(); } break;
		case CProjectile::UNSYNCED_EXPLOSPIKE_PROJECTILE:           { pdc = new PDC<     CExploSpikeProjectile>(); } break;
		case CProjectile::UNSYNCED_GENERICPARTICLE_PROJECTILE:      { pdc = new PDC<CGenericParticleProjectile>(); } break;
		case CProjectile::UNSYNCED_GEOSQUARE_PROJECTILE:            { pdc = new PDC<      CGeoSquareProjectile>(); } break;
		case CProjectile::UNSYNCED_GEOTHERM_SMOKE_PROJECTILE:       { pdc = new PDC<  CGeoThermSmokeProjectile>(); } break;
		case CProjectile::UNSYNCED_GFX_PROJECTILE:                  { pdc = new PDC<            CGfxProjectile>(); } break;
		case CProjectile::UNSYNCED_REPULSE_GFX_PROJECTILE:          { pdc = new PDC<               CRepulseGfx>(); } break;
		case CProjectile::UNSYNCED_HEATCLOUD_PROJECTILE:            { pdc = new PDC<      CHeatCloudProjectile>(); } break;
		case CProjectile::UNSYNCED_MUZZLEFLAME_PROJECTILE:          { pdc = new PDC<              CMuzzleFlame>(); } break;
		case CProjectile::UNSYNCED_BITMAP_MUZZLEFLAME_PROJECTILE:   { pdc = new PDC<        CBitmapMuzzleFlame>(); } break;
		case CProjectile::UNSYNCED_SHIELD_PROJECTILE:               { pdc = new PDC<          ShieldProjectile>(); } break;
		case CProjectile::UNSYNCED_SHIELDSEGMENT_PROJECTILE:        { pdc = new PDC<   ShieldSegmentProjectile>(); } break;
		case CProjectile::UNSYNCED_SIMPLEPARTICLESYSTEM_PROJECTILE: { pdc = new PDC<     CSimpleParticleSystem>(); } break;
		case CProjectile::UNSYNCED_SMOKE1_PROJECTILE:               { pdc = new PDC<          CSmokeProjectile>(); } break;
		case CProjectile::UNSYNCED_SMOKE2_PROJECTILE:               { pdc = new PDC<         CSmokeProjectile2>(); } break;
		case CProjectile::UNSYNCED_SMOKETRAIL_PROJECTILE:           { pdc = new PDC<     CSmokeTrailProjectile>(); } break;
		case CProjectile::UNSYNCED_SPHEREPART_PROJECTILE:           { pdc = new PDC<     CSpherePartProjectile>(); } break;
		case CProjectile::UNSYNCED_TRACER_PROJECTILE:               { pdc = new PDC<         CTracerProjectile>(); } break;
		case CProjectile::UNSYNCED_WAKE_PROJECTILE:                 { pdc = new PDC<           CWakeProjectile>(); } break;
		case CProjectile::UNSYNCED_WRECK_PROJECTILE:                { pdc = new PDC<          CWreckProjectile>(); } break;

		case CProjectile::SYNCED_FIRE_PROJECTILE:  { pdc = new PDC< CFireProjectile>(); } break;
		case CProjectile::SYNCED_FLARE_PROJECTILE: { pdc = new PDC<CFlareProjectile>(); } break;
		case CProjectile::SYNCED_PIECE_PROJECTILE: { pdc = new PDC<CPieceProjectile>(); } break;

		// NOTE:
		//   CWeaponProjectile is never allocated directly,
		//   so we don't need to create a container for it
		//   we do so anyway to prevent NULL-entry holes in
		//   ProjectileDrawer::projectileDrawContainers
		case CProjectile::WEAPON_BASE_PROJECTILE:           { pdc = new PDC<        CWeaponProjectile>(); } break;
		case CProjectile::WEAPON_EMG_PROJECTILE:            { pdc = new PDC<           CEmgProjectile>(); } break;
		case CProjectile::WEAPON_EXPLOSIVE_PROJECTILE:      { pdc = new PDC<     CExplosiveProjectile>(); } break;
		case CProjectile::WEAPON_FIREBALL_PROJECTILE:       { pdc = new PDC<      CFireBallProjectile>(); } break;
		case CProjectile::WEAPON_FLAME_PROJECTILE:          { pdc = new PDC<         CFlameProjectile>(); } break;
		case CProjectile::WEAPON_LASER_PROJECTILE:          { pdc = new PDC<         CLaserProjectile>(); } break;
		case CProjectile::WEAPON_BEAMLASER_PROJECTILE:      { pdc = new PDC<     CBeamLaserProjectile>(); } break;
		case CProjectile::WEAPON_LARGEBEAMLASER_PROJECTILE: { pdc = new PDC<CLargeBeamLaserProjectile>(); } break;
		case CProjectile::WEAPON_LIGHTNING_PROJECTILE:      { pdc = new PDC<     CLightningProjectile>(); } break;
		case CProjectile::WEAPON_MISSILE_PROJECTILE:        { pdc = new PDC<       CMissileProjectile>(); } break;
		case CProjectile::WEAPON_STARBURST_PROJECTILE:      { pdc = new PDC<     CStarburstProjectile>(); } break;
		case CProjectile::WEAPON_TORPEDO_PROJECTILE:        { pdc = new PDC<       CTorpedoProjectile>(); } break;
	}
	#undef PDC

	return pdc;
}

void IProjectileDrawContainer::FreeInstance(IProjectileDrawContainer* pdc) {
	delete pdc;
}

