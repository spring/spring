/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IPROJECTILE_DRAWER_H
#define IPROJECTILE_DRAWER_H

#include "System/EventClient.h"

class CTextureAtlas;
struct AtlasedTexture;

class IProjectileDrawer: public CEventClient {
public:
	IProjectileDrawer();

	static void InitStatic();
	static void KillStatic(bool reload);

	virtual void Init();
	virtual void Kill();

	virtual void Draw(bool drawReflection, bool drawRefraction = false) = 0;
	virtual void DrawProjectilesMiniMap() = 0;
	virtual void DrawGroundFlashes() = 0;
	virtual void DrawShadowPass() = 0;

	virtual void UpdateTextures() = 0;

	bool WantsEvent(const std::string& eventName) override;
	bool GetFullRead() const override;
	int GetReadAllyTeam() const override;

	virtual void RenderProjectileCreated(const CProjectile* projectile) override = 0;
	virtual void RenderProjectileDestroyed(const CProjectile* projectile) override = 0;

	virtual unsigned int NumSmokeTextures() const = 0;

	virtual void IncPerlinTexObjectCount() = 0;
	virtual void DecPerlinTexObjectCount() = 0;

	bool EnableSorting(bool b);
	bool ToggleSorting();

	virtual const AtlasedTexture* GetSmokeTexture(unsigned int i) const = 0;

	CTextureAtlas* textureAtlas = nullptr;  ///< texture atlas for projectiles
	CTextureAtlas* groundFXAtlas = nullptr; ///< texture atlas for ground fx

	// texture-coordinates for projectiles
	AtlasedTexture* flaretex = nullptr;
	AtlasedTexture* dguntex = nullptr;            ///< dgun texture
	AtlasedTexture* flareprojectiletex = nullptr; ///< texture used by flares that trick missiles
	AtlasedTexture* sbtrailtex = nullptr;         ///< default first section of starburst missile trail texture
	AtlasedTexture* missiletrailtex = nullptr;    ///< default first section of missile trail texture
	AtlasedTexture* muzzleflametex = nullptr;     ///< default muzzle flame texture
	AtlasedTexture* repulsetex = nullptr;         ///< texture of impact on repulsor
	AtlasedTexture* sbflaretex = nullptr;         ///< default starburst  missile flare texture
	AtlasedTexture* missileflaretex = nullptr;    ///< default missile flare texture
	AtlasedTexture* beamlaserflaretex = nullptr;  ///< default beam laser flare texture
	AtlasedTexture* explotex = nullptr;
	AtlasedTexture* explofadetex = nullptr;
	AtlasedTexture* heatcloudtex = nullptr;
	AtlasedTexture* circularthingytex = nullptr;
	AtlasedTexture* bubbletex = nullptr;          ///< torpedo trail texture
	AtlasedTexture* geosquaretex = nullptr;       ///< unknown use
	AtlasedTexture* gfxtex = nullptr;             ///< nanospray texture
	AtlasedTexture* projectiletex = nullptr;      ///< appears to be unused
	AtlasedTexture* repulsegfxtex = nullptr;      ///< used by repulsor
	AtlasedTexture* sphereparttex = nullptr;      ///< sphere explosion texture
	AtlasedTexture* torpedotex = nullptr;         ///< appears in-game as a 1 texel texture
	AtlasedTexture* wrecktex = nullptr;           ///< smoking explosion part texture
	AtlasedTexture* plasmatex = nullptr;          ///< default plasma texture
	AtlasedTexture* laserendtex = nullptr;
	AtlasedTexture* laserfallofftex = nullptr;
	AtlasedTexture* randdotstex = nullptr;
	AtlasedTexture* smoketrailtex = nullptr;
	AtlasedTexture* waketex = nullptr;
	AtlasedTexture* perlintex = nullptr;
	AtlasedTexture* flametex = nullptr;

	AtlasedTexture* groundflashtex = nullptr;
	AtlasedTexture* groundringtex = nullptr;

	AtlasedTexture* seismictex = nullptr;

protected:
	bool drawSorted = true;
};

extern IProjectileDrawer* projectileDrawer;

#endif /* IPROJECTILE_DRAWER_H */
