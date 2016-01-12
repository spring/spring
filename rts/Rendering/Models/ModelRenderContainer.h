/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_RENDER_CONTAINER_HDR
#define MODEL_RENDER_CONTAINER_HDR

#define MDL_TYPE(o) (o->model->type)
#define TEX_TYPE(o) (o->model->textureType)

#include <unordered_map>

#include "Rendering/Models/3DModel.h"

class CUnit;
class CFeature;
class CProjectile;

class IModelRenderContainer {
public:
	static IModelRenderContainer* GetInstance(int);

	IModelRenderContainer(int mdlType): modelType(mdlType) {
		numUnits = 0;
		numFeatures = 0;
		numProjectiles = 0;
	}
	virtual ~IModelRenderContainer();

	virtual void AddUnit(const CUnit*);
	virtual void DelUnit(const CUnit*);

public:
	virtual void AddFeature(const CFeature*);
	virtual void DelFeature(const CFeature*);

	virtual void AddProjectile(const CProjectile*);
	virtual void DelProjectile(const CProjectile*);

	unsigned int GetNumUnits() const { return numUnits; }
	unsigned int GetNumFeatures() const { return numFeatures; }
	unsigned int GetNumProjectiles() const { return numProjectiles; }

protected:
	typedef std::vector<      CUnit*>       UnitSet;
	typedef std::vector<   CFeature*>    FeatureSet;
	typedef std::vector<CProjectile*> ProjectileSet;

	// textureType ==> modelSet
	typedef std::unordered_map<int,       UnitSet>       UnitRenderBin;
	typedef std::unordered_map<int,    FeatureSet>    FeatureRenderBin;
	typedef std::unordered_map<int, ProjectileSet> ProjectileRenderBin;

	UnitRenderBin units;              // all opaque or all cloaked
	FeatureRenderBin features;        // all (opaque and fade) or all shadow
	ProjectileRenderBin projectiles;  // opaque only, (synced && (piece || weapon)) only

	int modelType;

	unsigned int numUnits;
	unsigned int numFeatures;
	unsigned int numProjectiles;

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

class ModelRenderContainer3DO: public IModelRenderContainer {
public:
	ModelRenderContainer3DO(): IModelRenderContainer(MODELTYPE_3DO) {}
};

class ModelRenderContainerS3O: public IModelRenderContainer {
public:
	ModelRenderContainerS3O(): IModelRenderContainer(MODELTYPE_S3O) {}
};

class ModelRenderContainerOBJ: public IModelRenderContainer {
public:
	ModelRenderContainerOBJ(): IModelRenderContainer(MODELTYPE_OBJ) {}
};

class ModelRenderContainerASS: public IModelRenderContainer {
public:
	ModelRenderContainerASS(): IModelRenderContainer(MODELTYPE_ASS) {}
};

#endif
