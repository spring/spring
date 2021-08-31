/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_DRAWER_HDR
#define PROJECTILE_DRAWER_HDR

#include <array>

#include "Sim/Projectiles/Projectile.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "System/EventClient.h"
#include "System/UnorderedSet.hpp"

class CSolidObject;
class CTextureAtlas;
class CVertexArray;
struct AtlasedTexture;
class CGroundFlash;
struct FlyingPiece;
class LuaTable;


class CProjectileDrawer: public CEventClient {
public:
	CProjectileDrawer(): CEventClient("[CProjectileDrawer]", 123456, false), perlinFB(true) {}

	static void InitStatic();
	static void KillStatic(bool reload);

	void Init();
	void Kill();

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawProjectilesMiniMap();
	void DrawGroundFlashes();
	void DrawShadowPass();

	void LoadWeaponTextures();
	void UpdateTextures();


	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "RenderProjectileCreated") ||
			(eventName == "RenderProjectileDestroyed") ||
			(eventName == "ViewResize");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderProjectileCreated(const CProjectile* projectile);
	void RenderProjectileDestroyed(const CProjectile* projectile);

	void ViewResize() override;

	unsigned int NumSmokeTextures() const { return (smokeTextures.size()); }

	void IncPerlinTexObjectCount() { perlinTexObjects++; }
	void DecPerlinTexObjectCount() { perlinTexObjects--; }

	bool EnableSorting(bool b) { return (drawSorted =           b); }
	bool ToggleSorting(      ) { return (drawSorted = !drawSorted); }

	static bool CheckSoftenExt();
	bool CanDrawSoften() {
		return
			CheckSoftenExt() &&
			fxShader && fxShader->IsValid() &&
			depthTexture != 0u &&
			depthFBO && depthFBO->IsValid();
	};

	int EnableSoften(int b) { return CanDrawSoften() ? (wantSoften = std::clamp(b, 0, WANT_SOFTEN_COUNT - 1)) : 0; }
	int ToggleSoften() { return EnableSoften((wantSoften + 1) % WANT_SOFTEN_COUNT); }

	int EnableDrawOrder(int b) { return wantDrawOrder = b; }
	int ToggleDrawOrder() { return EnableDrawOrder((wantDrawOrder + 1) % 2); }

	void CopyDepthBufferToTexture();

	const AtlasedTexture* GetSmokeTexture(unsigned int i) const { return smokeTextures[i]; }

	CVertexArray* fxVA = nullptr;
	CVertexArray* gfVA = nullptr;

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
private:
	static void ParseAtlasTextures(const bool, const LuaTable&, spring::unordered_set<std::string>&, CTextureAtlas*);

	void DrawProjectiles(int modelType, bool drawReflection, bool drawRefraction);
	void DrawProjectilesShadow(int modelType);
	void DrawFlyingPieces(int modelType);

	void DrawProjectilesSet(const std::vector<CProjectile*>& projectiles, bool drawReflection, bool drawRefraction);
	static void DrawProjectilesSetShadow(const std::vector<CProjectile*>& projectiles);

	static bool CanDrawProjectile(const CProjectile* pro, const CSolidObject* owner);
	void DrawProjectileNow(CProjectile* projectile, bool drawReflection, bool drawRefraction);

	static void DrawProjectileShadow(CProjectile* projectile);
	static bool DrawProjectileModel(const CProjectile* projectile);

	void UpdatePerlin();
	static void GenerateNoiseTex(unsigned int tex);

private:
	static constexpr int perlinBlendTexSize = 16;
	static constexpr int perlinTexSize = 128;

	// start edge fading of regular CEGs if height difference is less than [0]
	// fade out groundflashes to 0 as height difference reaches [1]
	static constexpr float softenThreshold[2] = { 8.0f, 350.0f };
	static constexpr float softenExponent[2]  = { 0.6f, 8.0f };

	GLuint perlinBlendTex[8];
	float perlinBlend[4];

	int perlinTexObjects = 0;
	bool drawPerlinTex = false;

	FBO perlinFB;

	std::vector<const AtlasedTexture*> smokeTextures;

	/// projectiles without a model, e.g. nano-particles
	std::vector<CProjectile*> renderProjectiles;
	/// projectiles with a model
	std::array<ModelRenderContainer<CProjectile>, MODELTYPE_OTHER> modelRenderers;

	/// {[0] := unsorted, [1] := distance-sorted} projectiles;
	/// used to render particle effects in back-to-front order
	std::vector<CProjectile*> sortedProjectiles[2];

	std::function<bool(const CProjectile*, const CProjectile*)> sortingPredicate = [this](const CProjectile* p1, const CProjectile* p2)
	{
		if (wantDrawOrder && p1->drawOrder != p2->drawOrder)
			return (p1->drawOrder < p2->drawOrder);

		if (p1->GetSortDist() != p2->GetSortDist()) // strict ordering required
			return (p1->GetSortDist() > p2->GetSortDist());

		return (p1 > p2);
	};

	bool drawSorted = true;

	GLuint depthTexture = 0u;
	FBO* depthFBO = nullptr;
	Shader::IProgramObject* fxShader = nullptr;

	constexpr static int WANT_SOFTEN_COUNT = 3;
	int wantSoften = 0;

	bool wantDrawOrder = true;
};

extern CProjectileDrawer* projectileDrawer;

#endif // PROJECTILE_DRAWER_HDR
