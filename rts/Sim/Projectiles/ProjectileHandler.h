/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_HANDLER_H
#define PROJECTILE_HANDLER_H

#include <list>
#include <set>
#include <vector>
#include <stack>

#include "lib/gml/ThreadSafeContainers.h"

#include "Sim/Projectiles/ProjectileFunctors.h"
#include "System/float3.h"
#include "System/Platform/Threading.h"

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



typedef std::pair<CProjectile*, int> ProjectileMapValPair;
typedef std::pair<int, ProjectileMapValPair> ProjectileMapKeyPair;
typedef std::map<int, ProjectileMapValPair> ProjectileMap;

typedef ThreadListSim<std::list<CProjectile*>, std::set<CProjectile*>, CProjectile*, ProjectileDetacher> ProjectileContainer;
typedef ThreadListSimRender<std::list<CGroundFlash*>, std::set<CGroundFlash*>, CGroundFlash*> GroundFlashContainer;

typedef ThreadListSimRender<std::set<FlyingPiece*, FlyingPieceComparator>, void, FlyingPiece*> FlyingPieceContainer;

typedef ThreadMapRender<CProjectile*, int, ProjectileMapValPair, ProjectileIndexer> ProjectileRenderMap;


class CProjectileHandler
{
	CR_DECLARE_STRUCT(CProjectileHandler);

public:
	CProjectileHandler();
	~CProjectileHandler();
	void Serialize(creg::ISerializer* s);
	void PostLoad();

	inline const ProjectileMapValPair* GetMapPairBySyncedID(int id) const {
		const ProjectileMap& projectileIDs = syncedProjectileIDs;
		const ProjectileMap::const_iterator it = projectileIDs.find(id);

		if (it == projectileIDs.end())
			return NULL;

		return &(it->second);
	}

	inline const ProjectileMapValPair* GetMapPairByUnsyncedID(int id) const {
		if (UNSYNCED_PROJ_NOEVENT)
			return NULL; // unsynced projectiles have no IDs if UNSYNCED_PROJ_NOEVENT

		const ProjectileMap& projectileIDs = unsyncedProjectileIDs;
		const ProjectileMap::const_iterator it = projectileIDs.find(id);

		if (it == projectileIDs.end())
			return NULL;

		return &(it->second);
	}

	ProjectileRenderMap& GetSyncedRenderProjectileIDs() { return syncedRenderProjectileIDs; }
	ProjectileRenderMap& GetUnsyncedRenderProjectileIDs() { return unsyncedRenderProjectileIDs; }

	void CheckUnitCollisions(CProjectile*, std::vector<CUnit*>&, const float3&, const float3&);
	void CheckFeatureCollisions(CProjectile*, std::vector<CFeature*>&, const float3&, const float3&);
	void CheckUnitFeatureCollisions(ProjectileContainer&);
	void CheckGroundCollisions(ProjectileContainer&);
	void CheckCollisions();

	void SetMaxParticles(int value) { maxParticles = value; }
	void SetMaxNanoParticles(int value) { maxNanoParticles = value; }

	void Update();
	void UpdateParticleSaturation() {
		particleSaturation = (maxParticles > 0)? (currentParticles / float(maxParticles)): 1.0f;
	}

	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash);
	void AddFlyingPiece(const float3& pos, const float3& speed, int team, const S3DOPiece* piece, const S3DOPrimitive* chunk);
	void AddFlyingPiece(const float3& pos, const float3& speed, int team, int textureType, const SS3OVertex* chunk);
	void AddNanoParticle(const float3&, const float3&, const UnitDef*, int team, bool highPriority);
	void AddNanoParticle(const float3&, const float3&, const UnitDef*, int team, float radius, bool inverse, bool highPriority);
	bool RenderAccess(const CProjectile *p) const;

public:
	ProjectileContainer syncedProjectiles;    // contains only projectiles that can change simulation state
	ProjectileContainer unsyncedProjectiles;  // contains only projectiles that cannot change simulation state
	FlyingPieceContainer flyingPieces3DO;     // unsynced
	FlyingPieceContainer flyingPiecesS3O;     // unsynced
	GroundFlashContainer groundFlashes;       // unsynced

	int maxParticles;              // different effects should start to cut down on unnececary(unsynced) particles when this number is reached
	int maxNanoParticles;
	int currentParticles;          // number of particles weighted by how complex they are
	int currentNanoParticles;
	float particleSaturation;      // currentParticles / maxParticles ratio

private:
	void UpdateProjectileContainer(ProjectileContainer&, bool);

	ProjectileRenderMap syncedRenderProjectileIDs;        // same as syncedProjectileIDs, used by render thread
	ProjectileRenderMap unsyncedRenderProjectileIDs;      // same as unsyncedProjectileIDs, used by render thread

	int maxUsedSyncedID;
	int maxUsedUnsyncedID;
	std::list<int> freeSyncedIDs;             // available synced (weapon, piece) projectile ID's
	std::list<int> freeUnsyncedIDs;           // available unsynced projectile ID's
	ProjectileMap syncedProjectileIDs;        // ID ==> <projectile, allyteam> map for living synced projectiles
	ProjectileMap unsyncedProjectileIDs;      // ID ==> <projectile, allyteam> map for living unsynced projectiles
};


extern CProjectileHandler* projectileHandler;

#endif /* PROJECTILE_HANDLER_H */
