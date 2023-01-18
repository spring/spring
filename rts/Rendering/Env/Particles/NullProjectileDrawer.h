// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NULL_PROJECTILE_DRAWER_H
#define NULL_PROJECTILE_DRAWER_H

#include "Rendering/Env/Particles/IProjectileDrawer.h"

class CNullProjectileDrawer: public IProjectileDrawer {
public:
	CNullProjectileDrawer() {}

	void Init() override;
	void Kill() override;

	void Draw(bool drawReflection, bool drawRefraction = false) override {}
	void DrawProjectilesMiniMap() override {}
	void DrawGroundFlashes() override {}
	void DrawShadowPass() override {}

	void UpdateTextures() override {}

	void RenderProjectileCreated(const CProjectile* projectile) override {}
	void RenderProjectileDestroyed(const CProjectile* projectile) override {}

	unsigned int NumSmokeTextures() const override;

	void IncPerlinTexObjectCount() override {}
	void DecPerlinTexObjectCount() override {}

	const AtlasedTexture* GetSmokeTexture(unsigned int i) const override;

private:
	AtlasedTexture* dummyTexture;
};

#endif /* NULL_PROJECTILE_DRAWER_H */
