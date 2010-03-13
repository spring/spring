#ifndef PROJECTILE_DRAWER_HDR
#define PROJECTILE_DRAWER_HDR

#include <list>
#include <set>

#include "lib/gml/ThreadSafeContainers.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"

class CTextureAtlas;
class AtlasedTexture;
class CProjectile;
class CGroundFlash;
struct FlyingPiece;
struct piececmp;

typedef ThreadListSimRender<std::list<CProjectile*>, std::set<CProjectile*>, CProjectile*> ProjectileContainer;
typedef ThreadListSimRender<std::list<CGroundFlash*>, std::set<CGroundFlash*>, CGroundFlash*> GroundFlashContainer;
#if defined(USE_GML) && GML_ENABLE_SIM
typedef ThreadListSimRender<std::set<FlyingPiece*>, std::set<FlyingPiece*, piececmp>, FlyingPiece*> FlyingPieceContainer;
#else
typedef ThreadListSimRender<std::set<FlyingPiece*, piececmp>, void, FlyingPiece*> FlyingPieceContainer;
#endif

// need this for distset ordering
struct distcmp {
	bool operator() (const CProjectile* arg1, const CProjectile* arg2) const;
};

class CProjectileDrawer {
public:
	CProjectileDrawer();
	~CProjectileDrawer();

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawProjectiles(const ProjectileContainer&, bool, bool);
	void DrawProjectilesShadow(const ProjectileContainer&);
	void DrawProjectilesMiniMap(const ProjectileContainer&);
	void DrawProjectilesMiniMap();
	void DrawShadowPass(void);
	void DrawGroundFlashes(void);

	void LoadWeaponTextures();
	void UpdateTextures();


	CTextureAtlas* textureAtlas;  //texture atlas for projectiles
	CTextureAtlas* groundFXAtlas; //texture atlas for ground fx

	// texture-coordinates for projectiles
	AtlasedTexture* flaretex;
	AtlasedTexture* dguntex;            // dgun texture
	AtlasedTexture* flareprojectiletex; // texture used by flares that trick missiles
	AtlasedTexture* sbtrailtex;         // default first section of starburst missile trail texture
	AtlasedTexture* missiletrailtex;    // default first section of missile trail texture
	AtlasedTexture* muzzleflametex;     // default muzzle flame texture
	AtlasedTexture* repulsetex;         // texture of impact on repulsor
	AtlasedTexture* sbflaretex;         // default starburst  missile flare texture
	AtlasedTexture* missileflaretex;    // default missile flare texture
	AtlasedTexture* beamlaserflaretex;  // default beam laser flare texture
	AtlasedTexture* explotex;
	AtlasedTexture* explofadetex;
	AtlasedTexture* heatcloudtex;
	AtlasedTexture* circularthingytex;
	AtlasedTexture* bubbletex;          // torpedo trail texture
	AtlasedTexture* geosquaretex;       // unknown use
	AtlasedTexture* gfxtex;             // nanospray texture
	AtlasedTexture* projectiletex;      // appears to be unused
	AtlasedTexture* repulsegfxtex;      // used by repulsor
	AtlasedTexture* sphereparttex;      // sphere explosion texture
	AtlasedTexture* torpedotex;         // appears in-game as a 1 texel texture
	AtlasedTexture* wrecktex;           // smoking explosion part texture
	AtlasedTexture* plasmatex;          // default plasma texture
	AtlasedTexture* laserendtex;
	AtlasedTexture* laserfallofftex;
	AtlasedTexture* randdotstex;
	AtlasedTexture* smoketrailtex;
	AtlasedTexture* waketex;
	std::vector<AtlasedTexture*> smoketex;
	AtlasedTexture* perlintex;
	AtlasedTexture* flametex;

	AtlasedTexture* groundflashtex;
	AtlasedTexture* groundringtex;

	AtlasedTexture* seismictex;

private:
	void UpdatePerlin();
	void GenerateNoiseTex(unsigned int tex, int size);

	GLuint perlinTex[8];
	float perlinBlend[4];
	FBO perlinFB;
	bool drawPerlinTex;

	std::set<CProjectile*, distcmp> distset;
};

extern CProjectileDrawer* projectileDrawer;

#endif
