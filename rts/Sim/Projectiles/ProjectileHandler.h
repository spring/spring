/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_HANDLER_H
#define PROJECTILE_HANDLER_H

#include <array>
#include <deque>
#include <vector>
#include "Rendering/Models/3DModel.h"
#include "Sim/Projectiles/ProjectileFunctors.h"
#include "System/float3.h"

// bypass id and event handling for unsynced projectiles (faster)
#define UNSYNCED_PROJ_NOEVENT 1

class CProjectile;
class CUnit;
class CFeature;
class CPlasmaRepulser;
class CGroundFlash;
struct UnitDef;
struct FlyingPiece;


typedef std::vector<CProjectile*> ProjectileMap;
typedef std::vector<CProjectile*> ProjectileContainer; // <unsorted>
typedef std::vector<CGroundFlash*> GroundFlashContainer;
typedef std::vector<FlyingPiece> FlyingPieceContainer;


class CProjectileHandler
{
	CR_DECLARE_STRUCT(CProjectileHandler)

public:
	CProjectileHandler();
	~CProjectileHandler();

	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	CProjectile* GetProjectileBySyncedID(int id);
	CProjectile* GetProjectileByUnsyncedID(int id);

	void CheckUnitCollisions(CProjectile*, std::vector<CUnit*>&, const float3, const float3);
	void CheckFeatureCollisions(CProjectile*, std::vector<CFeature*>&, const float3, const float3);
	void CheckShieldCollisions(CProjectile*, std::vector<CPlasmaRepulser*>&, const float3, const float3);
	void CheckUnitFeatureCollisions(ProjectileContainer&);
	void CheckGroundCollisions(ProjectileContainer&);
	void CheckCollisions();

	void SetMaxParticles(int value) { maxParticles = value; }
	void SetMaxNanoParticles(int value) { maxNanoParticles = value; }

	void Update();

	float GetParticleSaturation(const bool withRandomization = true) const;
	int   GetCurrentParticles() const;

	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash);
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
	int maxParticles;              // different effects should start to cut down on unnececary(unsynced) particles when this number is reached
	int maxNanoParticles;
	int currentNanoParticles;

	// these vars are used to precache parts of GetCurrentParticles() calculations
	int lastCurrentParticles;
	int lastSyncedProjectilesCount;
	int lastUnsyncedProjectilesCount;

	// flying pieces are sorted from time to time to reduce gl state changes
	std::array<                bool, MODELTYPE_OTHER> resortFlyingPieces;
	std::array<FlyingPieceContainer, MODELTYPE_OTHER> flyingPieces;  // unsynced

	ProjectileContainer syncedProjectiles;    // contains only projectiles that can change simulation state
	ProjectileContainer unsyncedProjectiles;  // contains only projectiles that cannot change simulation state
	GroundFlashContainer groundFlashes;       // unsynced

private:
	void UpdateProjectileContainer(ProjectileContainer&, bool);

	std::deque<int> freeSyncedIDs;            // available synced (weapon, piece) projectile ID's
	std::deque<int> freeUnsyncedIDs;          // available unsynced projectile ID's
	ProjectileMap syncedProjectileIDs;        // ID ==> projectile* map for living synced projectiles
	ProjectileMap unsyncedProjectileIDs;      // ID ==> projectile* map for living unsynced projectiles
};


extern CProjectileHandler* projectileHandler;

#endif /* PROJECTILE_HANDLER_H */
