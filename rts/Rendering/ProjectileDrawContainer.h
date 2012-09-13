/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_DRAW_CONTAINER_H
#define PROJECTILE_DRAW_CONTAINER_H

class CProjectile;
class IProjectileDrawContainer {
public:
	static IProjectileDrawContainer* GetInstance(unsigned int projectileType);
	static void FreeInstance(IProjectileDrawContainer*);

	virtual ~IProjectileDrawContainer() {}
	virtual void DrawNormal(unsigned int modelType, bool drawReflection, bool drawRefraction) = 0;
	virtual void DrawShadow(unsigned int modelType                                          ) = 0;
	virtual void AddProjectile(const CProjectile*) = 0;
	virtual void DelProjectile(const CProjectile*) = 0;
};

#endif
