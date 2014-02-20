/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_DRAWER_HDR
#define PROJECTILE_DRAWER_HDR

#include "Rendering/GL/myGL.h"
#include <list>
#include <set>

#include "lib/gml/ThreadSafeContainers.h"
#include "Rendering/GL/FBO.h"
#include "Sim/Projectiles/ProjectileFunctors.h"
#include "System/EventClient.h"

class CTextureAtlas;
struct AtlasedTexture;
class CGroundFlash;
struct FlyingPiece;
class IWorldObjectModelRenderer;
class LuaTable;


typedef ThreadListSimRender<std::list<CGroundFlash*>, std::set<CGroundFlash*>, CGroundFlash*> GroundFlashContainer;
typedef ThreadListSimRender<std::set<FlyingPiece*, FlyingPieceComparator>, void, FlyingPiece*> FlyingPieceContainer;


class CProjectileDrawer: public CEventClient {
public:
	CProjectileDrawer();
	~CProjectileDrawer();

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawProjectilesMiniMap();
	bool DrawProjectileModel(const CProjectile* projectile, bool shadowPass);
	void DrawGroundFlashes();
	void DrawShadowPass();

	void LoadWeaponTextures();
	void UpdateTextures();

	void Update();


	bool WantsEvent(const std::string& eventName) {
		return (eventName == "RenderProjectileCreated" || eventName == "RenderProjectileDestroyed");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderProjectileCreated(const CProjectile* projectile);
	void RenderProjectileDestroyed(const CProjectile* projectile);

	void IncPerlinTexObjectCount() { perlinTexObjects++; }
	void DecPerlinTexObjectCount() { perlinTexObjects--; }


	CTextureAtlas* textureAtlas;  ///< texture atlas for projectiles
	CTextureAtlas* groundFXAtlas; ///< texture atlas for ground fx

	// texture-coordinates for projectiles
	AtlasedTexture* flaretex;
	AtlasedTexture* dguntex;            ///< dgun texture
	AtlasedTexture* flareprojectiletex; ///< texture used by flares that trick missiles
	AtlasedTexture* sbtrailtex;         ///< default first section of starburst missile trail texture
	AtlasedTexture* missiletrailtex;    ///< default first section of missile trail texture
	AtlasedTexture* muzzleflametex;     ///< default muzzle flame texture
	AtlasedTexture* repulsetex;         ///< texture of impact on repulsor
	AtlasedTexture* sbflaretex;         ///< default starburst  missile flare texture
	AtlasedTexture* missileflaretex;    ///< default missile flare texture
	AtlasedTexture* beamlaserflaretex;  ///< default beam laser flare texture
	AtlasedTexture* explotex;
	AtlasedTexture* explofadetex;
	AtlasedTexture* heatcloudtex;
	AtlasedTexture* circularthingytex;
	AtlasedTexture* bubbletex;          ///< torpedo trail texture
	AtlasedTexture* geosquaretex;       ///< unknown use
	AtlasedTexture* gfxtex;             ///< nanospray texture
	AtlasedTexture* projectiletex;      ///< appears to be unused
	AtlasedTexture* repulsegfxtex;      ///< used by repulsor
	AtlasedTexture* sphereparttex;      ///< sphere explosion texture
	AtlasedTexture* torpedotex;         ///< appears in-game as a 1 texel texture
	AtlasedTexture* wrecktex;           ///< smoking explosion part texture
	AtlasedTexture* plasmatex;          ///< default plasma texture
	AtlasedTexture* laserendtex;
	AtlasedTexture* laserfallofftex;
	AtlasedTexture* randdotstex;
	AtlasedTexture* smoketrailtex;
	AtlasedTexture* waketex;
	AtlasedTexture* perlintex;
	AtlasedTexture* flametex;

	AtlasedTexture* groundflashtex;
	AtlasedTexture* groundringtex;

	AtlasedTexture* seismictex;

	std::vector<const AtlasedTexture*> smoketex;

private:
	void ParseAtlasTextures(const bool, const LuaTable&, std::set<std::string>&, CTextureAtlas*);

	void DrawProjectiles(int modelType, int numFlyingPieces, int* drawnPieces, bool drawReflection, bool drawRefraction);
	void DrawProjectilesSet(std::set<CProjectile*>& projectiles, bool drawReflection, bool drawRefraction);
	void DrawProjectile(CProjectile* projectile, bool drawReflection, bool drawRefraction);
	void DrawProjectilesShadow(int modelType);
	void DrawProjectileShadow(CProjectile* projectile);
	void DrawProjectilesSetShadow(std::set<CProjectile*>& projectiles);
	void DrawFlyingPieces(int modelType, int numFlyingPieces, int* drawnPieces);

	void UpdatePerlin();
	void GenerateNoiseTex(unsigned int tex, int size);

	GLuint perlinTex[8];
	float perlinBlend[4];
	FBO perlinFB;
	int perlinTexObjects;
	bool drawPerlinTex;

	/// projectiles without a model
	std::set<CProjectile*> renderProjectiles;
	/// projectiles with a model
	std::vector<IWorldObjectModelRenderer*> modelRenderers;

	/**
	 * z-sorted set of all projectiles; used to
	 * render particle effects in back-to-front order
	 */
	std::set<CProjectile*, ProjectileDistanceComparator> zSortedProjectiles;
};

extern CProjectileDrawer* projectileDrawer;

#endif // PROJECTILE_DRAWER_HDR
