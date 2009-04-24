#ifndef PROJECTILEHANDLER_H
#define PROJECTILEHANDLER_H
// ProjectileHandler.h: interface for the CProjectileHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <list>
#include <vector>

#include <stack>
#include "MemPool.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "float3.h"

class CProjectileHandler;
class CProjectile;
class CUnit;
class CFeature;
class CGroundFlash;
struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;

typedef std::list<CProjectile*> Projectile_List;
typedef std::pair<CProjectile*, int> ProjectileMapPair;
typedef std::map<int, ProjectileMapPair> ProjectileMap;


class CProjectileHandler
{
public:
	CR_DECLARE(CProjectileHandler);
	CProjectileHandler();
	virtual ~CProjectileHandler();
	void Serialize(creg::ISerializer *s);
	void PostLoad();

	inline const ProjectileMapPair* GetMapPairByID(int id) const {
		ProjectileMap::const_iterator it = weaponProjectileIDs.find(id);
		if (it == weaponProjectileIDs.end()) {
			return NULL;
		}
		return &(it->second);
	}

	void CheckUnitCollisions(CProjectile*, std::vector<CUnit*>&, CUnit**, const float3&, const float3&);
	void CheckFeatureCollisions(CProjectile*, std::vector<CFeature*>&, CFeature**, const float3&, const float3&);
	void CheckCollisions();

	void LoadSmoke(unsigned char tex[512][512][4], int xoffs, int yoffs, char* filename, char* alphafile);

	void SetMaxParticles(int value);

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawShadowPass(void);
	void DrawGroundFlashes(void);
	void Update();
	void UpdateTextures();
	void AddProjectile(CProjectile* p);
	void AddGroundFlash(CGroundFlash* flash);

	void ConvertTex(unsigned char tex[512][512][4], int startx, int starty, int endx, int endy, float absorb);
	void AddFlyingPiece(float3 pos, float3 speed, S3DOPiece* object, S3DOPrimitive* piece);
	void AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, SS3OVertex* verts);

	struct projdist {
		float dist;
		CProjectile* proj;
	};

	Projectile_List ps;						// contains both synced and unsynced projectiles
	int maxUsedID;
	std::list<int> freeIDs;
	ProjectileMap weaponProjectileIDs;		// ID ==> <projectile, allyteam> map for weapon projectiles

	std::vector<projdist> distlist;

	unsigned int projectileShadowVP;

	int maxParticles;						// different effects should start to cut down on unnececary(unsynced) particles when this number is reached
	int currentParticles;					// number of particles weighted by how complex they are
	float particleSaturation;				// currentParticles / maxParticles ratio

	int numPerlinProjectiles;

	CTextureAtlas* textureAtlas;  //texture atlas for projectiles
	CTextureAtlas* groundFXAtlas; //texture atlas for ground fx

	//texturcoordinates for projectiles
	AtlasedTexture flaretex;
	AtlasedTexture dguntex;            // dgun texture
	AtlasedTexture flareprojectiletex; // texture used by flares that trick missiles
	AtlasedTexture sbtrailtex;         // default first section of starburst missile trail texture
	AtlasedTexture missiletrailtex;    // default first section of missile trail texture
	AtlasedTexture muzzleflametex;     // default muzzle flame texture
	AtlasedTexture repulsetex;         // texture of impact on repulsor
	AtlasedTexture sbflaretex;         // default starburst  missile flare texture
	AtlasedTexture missileflaretex;    // default missile flare texture
	AtlasedTexture beamlaserflaretex;  // default beam laser flare texture
	AtlasedTexture explotex;
	AtlasedTexture explofadetex;
	AtlasedTexture heatcloudtex;
	AtlasedTexture circularthingytex;
	AtlasedTexture bubbletex;          // torpedo trail texture
	AtlasedTexture geosquaretex;       // unknown use
	AtlasedTexture gfxtex;             // nanospray texture
	AtlasedTexture projectiletex;      // appears to be unused
	AtlasedTexture repulsegfxtex;      // used by repulsor
	AtlasedTexture sphereparttex;      // sphere explosion texture
	AtlasedTexture torpedotex;         // appears in-game as a 1 texel texture
	AtlasedTexture wrecktex;           // smoking explosion part texture
	AtlasedTexture plasmatex;          // default plasma texture
	AtlasedTexture laserendtex;
	AtlasedTexture laserfallofftex;
	AtlasedTexture randdotstex;
	AtlasedTexture smoketrailtex;
	AtlasedTexture waketex;
	std::vector<AtlasedTexture> smoketex;
	AtlasedTexture perlintex;
	AtlasedTexture flametex;

	AtlasedTexture groundflashtex;
	AtlasedTexture groundringtex;

	AtlasedTexture seismictex;

private:
	void UpdatePerlin();
	void GenerateNoiseTex(unsigned int tex,int size);
	struct FlyingPiece{
#if !defined(USE_MMGR)
		inline void* operator new(size_t size){return mempool.Alloc(size);};
		inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
#endif

		S3DOPrimitive* prim;
		S3DOPiece* object;

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

	GLuint perlinTex[8];
	float perlinBlend[4];
	FBO perlinFB;
	bool drawPerlinTex;
	std::vector<CGroundFlash*> groundFlashes;

};


extern CProjectileHandler* ph;


#endif /* PROJECTILEHANDLER_H */
