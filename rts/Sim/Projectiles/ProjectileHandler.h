/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_HANDLER_H
#define PROJECTILE_HANDLER_H

#include <array>
#include <vector>

#include "Rendering/Models/3DModel.h"
#include "Rendering/Env/Particles/Classes/FlyingPiece.h"
#include "System/float3.h"
#include "System/FreeListMap.h"


// bypass id and event handling for unsynced projectiles (faster)
#define PH_UNSYNCED_PROJECTILE_EVENTS 0

class CProjectile;
class CUnit;
class CFeature;
class CPlasmaRepulser;
class CGroundFlash;
struct UnitDef;

typedef std::vector<CGroundFlash*> GroundFlashContainer;
typedef std::vector<FlyingPiece> FlyingPieceContainer;

class CProjectileHandler
{
	CR_DECLARE_STRUCT(CProjectileHandler)

public:
	void Init();
	void Kill();

	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	CProjectile* GetProjectileBySyncedID(int id)   { return GetProjectileByID<true >(id); }
	CProjectile* GetProjectileByUnsyncedID(int id) { return GetProjectileByID<false>(id); }

	const auto& GetActiveProjectiles(bool synced) const {
		return projectiles[synced];
	}

	void CheckUnitCollisions(CProjectile*, std::vector<CUnit*>&, const float3, const float3);
	void CheckFeatureCollisions(CProjectile*, std::vector<CFeature*>&, const float3, const float3);
	void CheckShieldCollisions(CProjectile*, std::vector<CPlasmaRepulser*>&, const float3, const float3);
	void CheckUnitFeatureCollisions(bool synced);
	void CheckGroundCollisions(bool synced);
	void CheckCollisions();

	void SetMaxParticles(int value) { maxParticles = std::max(0, value); }
	void SetMaxNanoParticles(int value) { maxNanoParticles = std::max(0, value); }

	void Update();

	float GetParticleSaturation(bool randomized = true) const;
	float GetNanoParticleSaturation(float priority) const {
		const float total = std::max(1.0f, maxNanoParticles * priority);
		const float fract = std::max(int(currentNanoParticles >= maxNanoParticles), currentNanoParticles) / total;

		return std::min(1.0f, fract);
	}

	int GetCurrentParticles() const;

	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash) { groundFlashes.push_back(flash); }
	void AddFlyingPiece(
		int modelType,
		const S3DModelPiece* piece,
		const CMatrix44f& m,
		const float3 pos,
		const float3 speed,
		const float2 pieceParams,
		const int2 renderParams
	);
	void AddNanoParticle(const float3, const float3, const UnitDef*, int team, bool highPriority);
	void AddNanoParticle(const float3, const float3, const UnitDef*, int team, float radius, bool inverse, bool highPriority);

public:
	int maxParticles = 0;
	int maxNanoParticles = 0;
	int currentNanoParticles = 0;

	// these vars are used to precache parts of GetCurrentParticles() calculations
	mutable int frameCurrentParticles = 0;
	mutable int frameProjectileCounts[2] = {0, 0};

	// flying pieces (unsynced) are sorted from time to time to reduce GL state changes
	std::array<                bool, MODELTYPE_CNT> resortFlyingPieces{};
	std::array<FlyingPieceContainer, MODELTYPE_CNT> flyingPieces{};

	// unsynced
	GroundFlashContainer groundFlashes;

private:
	// event-notifiers
	void CreateProjectile(CProjectile*);
	void DestroyProjectile(CProjectile*);

	template<bool synced>
	CProjectile* GetProjectileByID(int id);

	template<bool synced>
	void UpdateProjectilesImpl();
	void UpdateProjectiles() {
		UpdateProjectilesImpl< true>();
		UpdateProjectilesImpl<false>();
	}

private:
	// [0] contains only projectiles that can not change simulation state
	// [1] contains only projectiles that can     change simulation state
	spring::FreeListMapCompact<CProjectile*, int> projectiles[2];

	static uint32_t UnsyncedRandInt(uint32_t N);
	static uint32_t   SyncedRandInt(uint32_t N);

	static constexpr decltype(&UnsyncedRandInt) rngFuncs[] = { &UnsyncedRandInt, &SyncedRandInt };
};

template<bool synced>
inline CProjectile* CProjectileHandler::GetProjectileByID(int id)
{
	size_t pos = projectiles[synced].Find(id);
	if (pos == size_t(-1))
		return nullptr;

	return projectiles[synced][pos];
}



extern CProjectileHandler projectileHandler;

#endif /* PROJECTILE_HANDLER_H */
