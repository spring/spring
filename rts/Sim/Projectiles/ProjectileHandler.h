#ifndef PROJECTILEHANDLER_H
#define PROJECTILEHANDLER_H
// ProjectileHandler.h: interface for the CProjectileHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

class CProjectileHandler;
class CProjectile;
struct S3DOPrimitive;
struct S3DO;

#include <list>
#include <vector>

#include <set>
#include <stack>
#include "System/MemPool.h"

typedef std::list<CProjectile*> Projectile_List;

class CGroundFlash;

class CProjectileHandler  
{
public:
	CProjectileHandler();
	virtual ~CProjectileHandler();

	void CheckUnitCol();
	void LoadSmoke(unsigned char tex[512][512][4],int xoffs,int yoffs,char* filename,char* alphafile);

	void Draw(bool drawReflection);
	void AddProjectile(CProjectile* p);
	void Update();
	void AddGroundFlash(CGroundFlash* flash);
	void RemoveGroundFlash(CGroundFlash* flash);
	void DrawGroundFlashes(void);

	void ConvertTex(unsigned char tex[512][512][4], int startx, int starty, int endx, int endy, float absorb);
	void DrawShadowPass(void);
	void AddFlyingPiece(float3 pos,float3 speed,S3DO* object,S3DOPrimitive* piece);

	struct projdist{
		float dist;
		CProjectile* proj;
	};

	Projectile_List ps;

	std::vector<projdist> distlist;

	unsigned int projectileShadowVP;

	int maxParticles;					//different effects should start to cut down on unnececary(unsynced) particles when this number is reached
	int currentParticles;			//number of particles weighted by how complex they are
	float particleSaturation;	//currentParticles/maxParticles

	std::set<CGroundFlash*> groundFlashes;
	std::stack<CGroundFlash*> toBeDeletedFlashes;

	struct FlyingPiece{
		inline void* operator new(size_t size){return mempool.Alloc(size);};
		inline void operator delete(void* p,size_t size){mempool.Free(p,size);};

		S3DOPrimitive* prim;
		S3DO* object;

		float3 pos;
		float3 speed;

		float3 rotAxis;
		float rot;
		float rotSpeed;
	};
	std::list<FlyingPiece*> flyingPieces;
};
extern CProjectileHandler* ph;

#endif /* PROJECTILEHANDLER_H */
