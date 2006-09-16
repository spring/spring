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
struct SS3OVertex;

#include <list>
#include <vector>

#include <set>
#include <stack>
#include "MemPool.h"
#include "Rendering/Textures/TextureAtlas.h"

typedef std::list<CProjectile*> Projectile_List;

class CGroundFlash;
class IFramebuffer;

class CProjectileHandler  
{
public:
	CProjectileHandler();
	virtual ~CProjectileHandler();

	void CheckUnitCol();
	void LoadSmoke(unsigned char tex[512][512][4],int xoffs,int yoffs,char* filename,char* alphafile);

	void Draw(bool drawReflection,bool drawRefraction=false);
	void UpdateTextures();
	void AddProjectile(CProjectile* p);
	void Update();
	void AddGroundFlash(CGroundFlash* flash);
	void DrawGroundFlashes(void);

	void ConvertTex(unsigned char tex[512][512][4], int startx, int starty, int endx, int endy, float absorb);
	void DrawShadowPass(void);
	void AddFlyingPiece(float3 pos,float3 speed,S3DO* object,S3DOPrimitive* piece);
	void AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, SS3OVertex * verts);

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

	int numPerlinProjectiles;

	CTextureAtlas *textureAtlas;
	//texturcoordinates for projectiles
	AtlasedTexture flaretex;
	AtlasedTexture explotex;
	AtlasedTexture explofadetex;
	AtlasedTexture heatcloudtex;
	AtlasedTexture circularthingytex;
	AtlasedTexture laserendtex;
	AtlasedTexture laserfallofftex;
	AtlasedTexture randdotstex;
	AtlasedTexture smoketrailtex;
	AtlasedTexture waketex;
	AtlasedTexture smoketex[12];
	AtlasedTexture perlintex;
	AtlasedTexture flametex;

	AtlasedTexture groundflashtex;
	AtlasedTexture groundringtex;

	AtlasedTexture seismictex;

private:
	void UpdatePerlin();
	void GenerateNoiseTex(unsigned int tex,int size);
	struct FlyingPiece{
		inline void* operator new(size_t size){return mempool.Alloc(size);};
		inline void operator delete(void* p,size_t size){mempool.Free(p,size);};

		S3DOPrimitive* prim;
		S3DO* object;
		
		SS3OVertex* verts; /* SS3OVertex[4], our deletion. */

		float3 pos;
		float3 speed;

		float3 rotAxis;
		float rot;
		float rotSpeed;
	};
	typedef std::list<FlyingPiece*> FlyingPiece_List;
	std::list<FlyingPiece_List*> flyingPieces;
	FlyingPiece_List * flying3doPieces;
	// flyings3oPieces[textureType][team] 
	std::vector<std::vector<FlyingPiece_List*> > flyings3oPieces; 

	unsigned int perlinTex[8];
	float perlinBlend[4];
	IFramebuffer *perlinFB;
	bool drawPerlinTex;
	std::vector<CGroundFlash*> groundFlashes;
	CTextureAtlas *groundFXAtlas;
};
extern CProjectileHandler* ph;

#endif /* PROJECTILEHANDLER_H */
