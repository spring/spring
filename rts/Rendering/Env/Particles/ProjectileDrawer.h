/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_DRAWER_HDR
#define PROJECTILE_DRAWER_HDR

#include <array>

#include "Rendering/Env/Particles/IProjectileDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Sim/Projectiles/ProjectileFunctors.h"
#include "System/EventClient.h"
#include "System/UnorderedSet.hpp"

class CSolidObject;
class CGroundFlash;
struct FlyingPiece;
class LuaTable;

namespace Shader {
	struct IProgramObject;
};


class CProjectileDrawer: public IProjectileDrawer {
public:
	CProjectileDrawer(): perlinNoiseFBO(true) {}

	void Init() override;
	void Kill() override;

	void Draw(bool drawReflection, bool drawRefraction = false) override;
	void DrawProjectilesMiniMap() override;
	void DrawGroundFlashes() override;
	void DrawShadowPass() override;

	void LoadWeaponTextures();
	void UpdateTextures() override;


	void RenderProjectileCreated(const CProjectile* projectile) override;
	void RenderProjectileDestroyed(const CProjectile* projectile) override;


	unsigned int NumSmokeTextures() const override { return (smokeTextures.size()); }

	void IncPerlinTexObjectCount() override { perlinData.texObjects++; }
	void DecPerlinTexObjectCount() override { perlinData.texObjects--; }


	const AtlasedTexture* GetSmokeTexture(unsigned int i) const override { return smokeTextures[i]; }

	GL::RenderDataBufferTC* fxBuffer = nullptr;
	GL::RenderDataBufferTC* gfBuffer = nullptr;
	Shader::IProgramObject* fxShader = nullptr;
	Shader::IProgramObject* gfShader = nullptr;

private:
	static void ParseAtlasTextures(const bool, const LuaTable&, spring::unordered_set<std::string>&, CTextureAtlas*);

	void DrawProjectilePass(Shader::IProgramObject*, bool, bool);
	void DrawParticlePass(Shader::IProgramObject*, bool, bool);
	void DrawProjectileShadowPass(Shader::IProgramObject*);
	void DrawParticleShadowPass(Shader::IProgramObject*);

	void DrawProjectiles(int modelType, bool drawReflection, bool drawRefraction);
	void DrawProjectilesShadow(int modelType);
	void DrawFlyingPieces(int modelType);

	void DrawProjectilesSet(const std::vector<CProjectile*>& projectiles, bool drawReflection, bool drawRefraction);
	void DrawProjectilesSetShadow(const std::vector<CProjectile*>& projectiles);

	static bool CanDrawProjectile(const CProjectile* pro, const CSolidObject* owner);
	void DrawProjectileNow(CProjectile* projectile, bool drawReflection, bool drawRefraction);

	void DrawProjectileShadow(const CProjectile* projectile);
	static bool DrawProjectileModel(const CProjectile* projectile);

	void UpdatePerlin();
	static void GenerateNoiseTex(unsigned int tex);

private:
	struct PerlinData {
		static constexpr int blendTexSize =  16;
		static constexpr int noiseTexSize = 128;

		GLuint blendTextures[8];
		float blendWeights[4];

		int texObjects = 0;
		bool fboComplete = false;
	};

	PerlinData perlinData;

	FBO perlinNoiseFBO;

	VAO flyingPieceVAO;

	ProjectileDistanceComparator zSortCmp;


	std::vector<const AtlasedTexture*> smokeTextures;

	/// projectiles without a model, e.g. nano-particles
	std::vector<CProjectile*> renderProjectiles;
	/// projectiles with a model
	std::array<ModelRenderContainer<CProjectile>, MODELTYPE_OTHER> modelRenderers;

	/// {[0] := unsorted, [1] := distance-sorted} projectiles;
	/// used to render particle effects in back-to-front order
	std::vector<CProjectile*> sortedProjectiles[2];
};

#endif // PROJECTILE_DRAWER_HDR
