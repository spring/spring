/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILEHANDLER_H
#define PROJECTILEHANDLER_H

#include <list>
#include <set>
#include <vector>
#include <stack>
#include "lib/gml/ThreadSafeContainers.h"

#include "MemPool.h"
#include "float3.h"

#define UNSYNCED_PROJ_NOEVENT 1 // bypass id and event handling for unsynced projectiles (faster)

class CProjectile;
class CUnit;
class CFeature;
class CGroundFlash;
struct FlyingPiece;
struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;
struct piececmp {
	bool operator() (const FlyingPiece* fp1, const FlyingPiece* fp2) const;
};

typedef std::pair<CProjectile*, int> ProjectileMapPair;
typedef std::map<int, ProjectileMapPair> ProjectileMap;
typedef ThreadListSim<std::list<CProjectile*>, std::set<CProjectile*>, CProjectile*> ProjectileContainer;
typedef ThreadListSimRender<std::list<CGroundFlash*>, std::set<CGroundFlash*>, CGroundFlash*> GroundFlashContainer;
#if defined(USE_GML) && GML_ENABLE_SIM
typedef ThreadListSimRender<std::set<FlyingPiece*>, std::set<FlyingPiece*, piececmp>, FlyingPiece*> FlyingPieceContainer;
#else
typedef ThreadListSimRender<std::set<FlyingPiece*, piececmp>, void, FlyingPiece*> FlyingPieceContainer;
#endif



class CProjectileHandler
{
public:
	CR_DECLARE(CProjectileHandler);
	CProjectileHandler();
	virtual ~CProjectileHandler();
	void Serialize(creg::ISerializer* s);
	void PostLoad();

	inline const ProjectileMapPair* GetMapPairBySyncedID(int id) const {
		ProjectileMap::const_iterator it = syncedProjectileIDs.find(id);
		if (it == syncedProjectileIDs.end()) {
			return NULL;
		}
		return &(it->second);
	}
	inline const ProjectileMapPair* GetMapPairByUnsyncedID(int id) const {
		ProjectileMap::const_iterator it = unsyncedProjectileIDs.find(id);
		if (it == unsyncedProjectileIDs.end()) {
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
	
	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash);
	void AddFlyingPiece(int team, float3 pos, float3 speed, const S3DOPiece* object, const S3DOPrimitive* piece);
	void AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, SS3OVertex* verts);

	ProjectileContainer syncedProjectiles;    //! contains only projectiles that can change simulation state
	ProjectileContainer unsyncedProjectiles;  //! contains only projectiles that cannot change simulation state
	FlyingPieceContainer flyingPieces3DO;     //! unsynced
	FlyingPieceContainer flyingPiecesS3O;     //! unsynced
	GroundFlashContainer groundFlashes;       //! unsynced

	int maxUsedSyncedID;
	int maxUsedUnsyncedID;
	std::list<int> freeSyncedIDs;             //! available synced (weapon, piece) projectile ID's
	std::list<int> freeUnsyncedIDs;           //! available unsynced projectile ID's
	ProjectileMap syncedProjectileIDs;        //! ID ==> <projectile, allyteam> map for living synced projectiles
	ProjectileMap unsyncedProjectileIDs;      //! ID ==> <projectile, allyteam> map for living unsynced projectiles

	int maxParticles;              // different effects should start to cut down on unnececary(unsynced) particles when this number is reached
	int maxNanoParticles;
	int currentParticles;          // number of particles weighted by how complex they are
	int currentNanoParticles;
	float particleSaturation;      // currentParticles / maxParticles ratio
	float nanoParticleSaturation;

	int numPerlinProjectiles; //! unsynced
};


extern CProjectileHandler* ph;

#endif /* PROJECTILEHANDLER_H */
