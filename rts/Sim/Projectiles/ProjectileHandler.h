/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_HANDLER_H
#define PROJECTILE_HANDLER_H

#include <list>
#include <set>
#include <vector>
#include <stack>
#include "lib/gml/gmlcnf.h"
#include "lib/gml/ThreadSafeContainers.h"

#include "System/MemPool.h"
#include "System/float3.h"

#define UNSYNCED_PROJ_NOEVENT 1 // bypass id and event handling for unsynced projectiles (faster)

class CProjectile;
class CUnit;
class CFeature;
class CGroundFlash;
struct UnitDef;
struct FlyingPiece;
struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;

struct piececmp {
	bool operator() (const FlyingPiece* fp1, const FlyingPiece* fp2) const;
};
struct projdetach {
	static void Detach(CProjectile* p);
};

typedef std::pair<CProjectile*, int> ProjectileMapPair;
typedef std::map<int, ProjectileMapPair> ProjectileMap;
typedef ThreadListSim<std::list<CProjectile*>, std::set<CProjectile*>, CProjectile*, projdetach> ProjectileContainer;
typedef ThreadListSimRender<std::list<CGroundFlash*>, std::set<CGroundFlash*>, CGroundFlash*> GroundFlashContainer;
#if defined(USE_GML) && GML_ENABLE_SIM
typedef ThreadListSimRender<std::set<FlyingPiece*>, std::set<FlyingPiece*, piececmp>, FlyingPiece*> FlyingPieceContainer;
#else
typedef ThreadListSimRender<std::set<FlyingPiece*, piececmp>, void, FlyingPiece*> FlyingPieceContainer;
#endif

struct ProjectileID {
	static int Index(const CProjectile*);
	static CProjectile* Unindex(ProjectileMap::const_iterator i);
};
typedef ThreadMapRender<CProjectile*, int, ProjectileMapPair, ProjectileID> ProjectileRenderMap;


class CProjectileHandler
{
	CR_DECLARE(CProjectileHandler);

public:
	CProjectileHandler();
	virtual ~CProjectileHandler();
	void Serialize(creg::ISerializer* s);
	void PostLoad();

	inline const ProjectileMapPair* GetMapPairBySyncedID(int id) const {
		const bool renderAccess = (GML::SimEnabled() && !GML::IsSimThread());
		const ProjectileMap& projectileIDs = renderAccess ? syncedRenderProjectileIDs.get_render_map() : syncedProjectileIDs;
		ProjectileMap::const_iterator it = projectileIDs.find(id);
		if (it == projectileIDs.end()) {
			return NULL;
		}
		return &(it->second);
	}
	inline const ProjectileMapPair* GetMapPairByUnsyncedID(int id) const {
		if (UNSYNCED_PROJ_NOEVENT)
			return NULL; // unsynced projectiles have no IDs if UNSYNCED_PROJ_NOEVENT
		const bool renderAccess = (GML::SimEnabled() && !GML::IsSimThread());
		const ProjectileMap& projectileIDs = renderAccess ? unsyncedRenderProjectileIDs.get_render_map() : unsyncedProjectileIDs;
		ProjectileMap::const_iterator it = projectileIDs.find(id);
		if (it == projectileIDs.end()) {
			return NULL;
		}
		return &(it->second);
	}

	void CheckUnitCollisions(CProjectile*, std::vector<CUnit*>&, CUnit**, const float3&, const float3&);
	void CheckFeatureCollisions(CProjectile*, std::vector<CFeature*>&, CFeature**, const float3&, const float3&);
	void CheckUnitFeatureCollisions(ProjectileContainer&);
	void CheckGroundCollisions(ProjectileContainer&);
	void CheckCollisions();

	void SetMaxParticles(int value) { maxParticles = value; }
	void SetMaxNanoParticles(int value) { maxNanoParticles = value; }

	void Update();
	void UpdateProjectileContainer(ProjectileContainer&, bool);
	void UpdateParticleSaturation() {
		particleSaturation     = (maxParticles     > 0)? (currentParticles     / float(maxParticles    )): 1.0f;
		nanoParticleSaturation = (maxNanoParticles > 0)? (currentNanoParticles / float(maxNanoParticles)): 1.0f;
	}
	
	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash);
	void AddFlyingPiece(int team, float3 pos, float3 speed, const S3DOPiece* object, const S3DOPrimitive* piece);
	void AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, const SS3OVertex* geometry);
	void AddNanoParticle(const float3&, const float3&, const UnitDef*, int team, bool highPriority);
	void AddNanoParticle(const float3&, const float3&, const UnitDef*, int team, float radius, bool inverse, bool highPriority);
	bool RenderAccess(const CProjectile* p) const;

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
	float nanoParticleSaturation;

	ProjectileRenderMap syncedRenderProjectileIDs;        // same as syncedProjectileIDs, used by render thread
	ProjectileRenderMap unsyncedRenderProjectileIDs;      // same as unsyncedProjectileIDs, used by render thread

private:
	int maxUsedSyncedID;
	int maxUsedUnsyncedID;
	std::list<int> freeSyncedIDs;             // available synced (weapon, piece) projectile ID's
	std::list<int> freeUnsyncedIDs;           // available unsynced projectile ID's
	ProjectileMap syncedProjectileIDs;        // ID ==> <projectile, allyteam> map for living synced projectiles
	ProjectileMap unsyncedProjectileIDs;      // ID ==> <projectile, allyteam> map for living unsynced projectiles
};


extern CProjectileHandler* ph;

#endif /* PROJECTILE_HANDLER_H */
