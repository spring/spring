#ifndef FEATURE_DEF_H
#define FEATURE_DEF_H

#define DRAWTYPE_3DO 0
#define DRAWTYPE_TREE 1
#define DRAWTYPE_NONE -1

struct S3DOModel;

struct FeatureDef
{
	CR_DECLARE(FeatureDef);
	~FeatureDef();

	FeatureDef():
		metal(0), energy(0), maxHealth(0), radius(0), mass(0),
		upright(false), drawType(0), modelType(0),
		destructable(false), reclaimable(true), blocking(false),
		burnable(false), floating(false), geoThermal(false), noSelect(false),
		xsize(0), ysize(0) {}

	S3DOModel* LoadModel(int team) const;

	std::string myName;
	std::string description;

	int id;

	float metal;
	float energy;
	float maxHealth;

	float radius;
	float mass;            //used to see if the object can be overrun
	float collisionSphereScale;
	float3 collisionSphereOffset;
	bool useCSOffset;

	bool upright;
	int drawType;
	std::string modelname; //used by 3do obects
	int modelType;         //used by tree etc

	bool destructable;
	bool reclaimable;
	bool blocking;
	bool burnable;
	bool floating;
	bool noSelect;

	bool geoThermal;

	std::string deathFeature; //name of feature that this turn into when killed (not reclaimed)

	int xsize;             //each size is 8 units
	int ysize;             //each size is 8 units
};

#endif
