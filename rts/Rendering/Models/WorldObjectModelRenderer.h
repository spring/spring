/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORLDOBJECT_MODEL_RENDERER_HDR
#define WORLDOBJECT_MODEL_RENDERER_HDR

#define MDL_TYPE(o) (o->model->type)
#define TEX_TYPE(o) (o->model->textureType)

#include <map>
#include <set>

#include "Rendering/Models/3DModel.h"

class CUnit;
class CFeature;
class CProjectile;

class IWorldObjectModelRenderer {
public:
	static IWorldObjectModelRenderer* GetInstance(int);

	IWorldObjectModelRenderer(int mdlType): modelType(mdlType) {
		numUnits = 0;
		numFeatures = 0;
		numProjectiles = 0;
	}
	virtual ~IWorldObjectModelRenderer();

	virtual void Draw();
	virtual void PushRenderState() {}
	virtual void PopRenderState() {}

	virtual void AddUnit(const CUnit*);
	virtual void DelUnit(const CUnit*);
	virtual void AddFeature(const CFeature*);
	virtual void DelFeature(const CFeature*);
	virtual void AddProjectile(const CProjectile*);
	virtual void DelProjectile(const CProjectile*);

	int GetNumUnits() const { return numUnits; }
	int GetNumFeatures() const { return numFeatures; }
	int GetNumProjectiles() const { return numProjectiles; }

protected:
	typedef std::set<CUnit*>                       UnitSet;
	typedef std::set<CUnit*>::const_iterator       UnitSetIt;
	typedef std::set<CFeature*>                    FeatureSet;
	typedef std::set<CFeature*>::const_iterator    FeatureSetIt;
	typedef std::set<CProjectile*>                 ProjectileSet;
	typedef std::set<CProjectile*>::const_iterator ProjectileSetIt;

	// textureType ==> modelSet
	typedef std::map<int, UnitSet>                       UnitRenderBin;
	typedef std::map<int, UnitSet>::const_iterator       UnitRenderBinIt;
	typedef std::map<int, FeatureSet>                    FeatureRenderBin;
	typedef std::map<int, FeatureSet>::const_iterator    FeatureRenderBinIt;
	typedef std::map<int, ProjectileSet>                 ProjectileRenderBin;
	typedef std::map<int, ProjectileSet>::const_iterator ProjectileRenderBinIt;

	virtual void DrawModels(const UnitSet&);
	virtual void DrawModels(const FeatureSet&);
	virtual void DrawModels(const ProjectileSet&);
	virtual void DrawModel(const CUnit*) {}
	virtual void DrawModel(const CFeature*) {}
	virtual void DrawModel(const CProjectile*) {}

	UnitRenderBin units;              // all opaque or all cloaked
	FeatureRenderBin features;        // opaque only, culled via GridVisibility()
	ProjectileRenderBin projectiles;  // opaque only, (synced && (piece || weapon)) only

	int modelType;

	int numUnits;
	int numFeatures;
	int numProjectiles;

public:
	const UnitSet& GetUnitSet(int texType) { return units[texType]; }
	const FeatureSet& GetFeatureSet(int texType) { return features[texType]; }
	const ProjectileSet& GetProjectileSet(int texType) { return projectiles[texType]; }

	const UnitRenderBin& GetUnitBin() const { return units; }
	const FeatureRenderBin& GetFeatureBin() const { return features; }
	const ProjectileRenderBin& GetProjectileBin() const { return projectiles; }

	UnitRenderBin& GetUnitBinMutable() { return units; }
	FeatureRenderBin& GetFeatureBinMutable() { return features; }
	ProjectileRenderBin& GetProjectileBinMutable() { return projectiles; }
};

class WorldObjectModelRenderer3DO: public IWorldObjectModelRenderer {
public:
	WorldObjectModelRenderer3DO(): IWorldObjectModelRenderer(MODELTYPE_3DO) {}
	void PushRenderState();
	void PopRenderState();
};

class WorldObjectModelRendererS3O: public IWorldObjectModelRenderer {
public:
	WorldObjectModelRendererS3O(): IWorldObjectModelRenderer(MODELTYPE_S3O) {}
	void PushRenderState();
	void PopRenderState();

	void DrawModel(const CUnit*);
	void DrawModel(const CFeature*);
	void DrawModel(const CProjectile*);
};

class WorldObjectModelRendererOBJ: public IWorldObjectModelRenderer {
public:
	WorldObjectModelRendererOBJ(): IWorldObjectModelRenderer(MODELTYPE_OBJ) {}
	void PushRenderState();
	void PopRenderState();
};

#endif
