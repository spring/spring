/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_HANDLER_H
#define PROJECTILE_HANDLER_H

#include <list>
#include <set>
#include <deque>
#include <vector>

#include "lib/gml/ThreadSafeContainers.h"

#include "Sim/Projectiles/ProjectileFunctors.h"
#include "System/float3.h"

// bypass id and event handling for unsynced projectiles (faster)
#define UNSYNCED_PROJ_NOEVENT 1

class CProjectile;
class CUnit;
class CFeature;
class CGroundFlash;
struct UnitDef;
struct FlyingPiece;
struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;



typedef std::map<int, CProjectile*> ProjectileMap; // <id, proj*>

typedef std::list<CProjectile*> ProjectileContainer;
typedef ThreadListSimRender<std::list<CGroundFlash*>, std::set<CGroundFlash*>, CGroundFlash*> GroundFlashContainer;
typedef ThreadListSimRender<std::set<FlyingPiece*, FlyingPieceComparator>, void, FlyingPiece*> FlyingPieceContainer;


class CProjectileHandler
{
	CR_DECLARE_STRUCT(CProjectileHandler)

public:
	CProjectileHandler();
	~CProjectileHandler();
	void Serialize(creg::ISerializer* s);

	CProjectile* GetProjectileBySyncedID(int id);
	CProjectile* GetProjectileByUnsyncedID(int id);

	void CheckUnitCollisions(CProjectile*, std::vector<CUnit*>&, const float3&, const float3&);
	void CheckFeatureCollisions(CProjectile*, std::vector<CFeature*>&, const float3&, const float3&);
	void CheckUnitFeatureCollisions(ProjectileContainer&);
	void CheckGroundCollisions(ProjectileContainer&);
	void CheckCollisions();

	void SetMaxParticles(int value) { maxParticles = value; }
	void SetMaxNanoParticles(int value) { maxNanoParticles = value; }

	void Update();

	float GetParticleSaturation() const;
	int   GetCurrentParticles() const;

	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash);
	void AddFlyingPiece(const float3& pos, const float3& speed, int team, const S3DOPiece* piece, const S3DOPrimitive* chunk);
	void AddFlyingPiece(const float3& pos, const float3& speed, int team, int textureType, const SS3OVertex* chunk);
	void AddNanoParticle(const float3&, const float3&, const UnitDef*, int team, bool highPriority);
	void AddNanoParticle(const float3&, const float3&, const UnitDef*, int team, float radius, bool inverse, bool highPriority);
	bool RenderAccess(const CProjectile *p) const;

public:
	int maxParticles;              // different effects should start to cut down on unnececary(unsynced) particles when this number is reached
	int maxNanoParticles;
	int currentNanoParticles;

	ProjectileContainer syncedProjectiles;    // contains only projectiles that can change simulation state
	ProjectileContainer unsyncedProjectiles;  // contains only projectiles that cannot change simulation state
	FlyingPieceContainer flyingPieces3DO;     // unsynced
	FlyingPieceContainer flyingPiecesS3O;     // unsynced
	GroundFlashContainer groundFlashes;       // unsynced

private:
	void UpdateProjectileContainer(ProjectileContainer&, bool);


	int maxUsedSyncedID;
	int maxUsedUnsyncedID;
	std::deque<int> freeSyncedIDs;             // available synced (weapon, piece) projectile ID's
	std::deque<int> freeUnsyncedIDs;           // available unsynced projectile ID's
	ProjectileMap syncedProjectileIDs;        // ID ==> projectile* map for living synced projectiles
	ProjectileMap unsyncedProjectileIDs;      // ID ==> projectile* map for living unsynced projectiles

	ProjectileMap syncedRenderProjectileIDs;        // same as syncedProjectileIDs, used by render thread
	ProjectileMap unsyncedRenderProjectileIDs;      // same as unsyncedProjectileIDs, used by render thread
};


extern CProjectileHandler* projectileHandler;

#endif /* PROJECTILE_HANDLER_H */
