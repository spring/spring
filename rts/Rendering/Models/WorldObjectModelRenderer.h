/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORLDOBJECT_MODEL_RENDERER_HDR
#define WORLDOBJECT_MODEL_RENDERER_HDR

#define MDL_TYPE(o) (o->model->type)
#define TEX_TYPE(o) (o->model->textureType)

#include <unordered_map>

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
public:
	virtual void AddFeature(const CFeature*);
	virtual void DelFeature(const CFeature*);

	virtual void AddProjectile(const CProjectile*);
	virtual void DelProjectile(const CProjectile*);

	int GetNumUnits() const { return numUnits; }
	int GetNumFeatures() const { return numFeatures; }
	int GetNumProjectiles() const { return numProjectiles; }

protected:
	typedef std::vector<      CUnit*>       UnitSet;
	typedef std::vector<   CFeature*>    FeatureSet;
	typedef std::vector<CProjectile*> ProjectileSet;

	// textureType ==> modelSet
	typedef std::unordered_map<int,       UnitSet>       UnitRenderBin;
	typedef std::unordered_map<int,    FeatureSet>    FeatureRenderBin;
	typedef std::unordered_map<int, ProjectileSet> ProjectileRenderBin;

	virtual void DrawModels(const       UnitSet&);
	virtual void DrawModels(const    FeatureSet&);
	virtual void DrawModels(const ProjectileSet&);
	virtual void DrawModel(const       CUnit*) {}
	virtual void DrawModel(const    CFeature*) {}
	virtual void DrawModel(const CProjectile*) {}

	UnitRenderBin units;              // all opaque or all cloaked
	FeatureRenderBin features;        // all (opaque and fade) or all shadow
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

class WorldObjectModelRendererASS: public IWorldObjectModelRenderer {
public:
	WorldObjectModelRendererASS(): IWorldObjectModelRenderer(MODELTYPE_ASS) {}
	void PushRenderState();
	void PopRenderState();
};

#endif
