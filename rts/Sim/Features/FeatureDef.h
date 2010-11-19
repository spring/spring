/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURE_DEF_H
#define FEATURE_DEF_H

#include <string>
#include <map>

#include "float3.h"

#define DRAWTYPE_MODEL 0
#define DRAWTYPE_TREE 1 // >= different types of trees
#define DRAWTYPE_NONE -1


struct S3DModel;
struct CollisionVolume;

struct FeatureDef
{
	CR_DECLARE_STRUCT(FeatureDef);

	FeatureDef()
		: collisionVolume(NULL)
		, id(-1)
		, metal(0), energy(0), maxHealth(0), reclaimTime(0), mass(0),
		upright(false), drawType(0), model(NULL),
		resurrectable(false), smokeTime(0), destructable(false), reclaimable(true), autoreclaim(true), blocking(false),
		burnable(false), floating(false), noSelect(false), geoThermal(false),
		xsize(0), zsize(0) {}

	S3DModel* LoadModel() const;
	CollisionVolume* collisionVolume;

	std::string myName;
	std::string description;
	std::string filename;

	int id;

	float metal;
	float energy;
	float maxHealth;
	float reclaimTime;

	/// used to see if the object can be overrun
	float mass;

	bool upright;
	int drawType;
	S3DModel* model;
	std::string modelname;

	/// -1 := only if it is the 1st wreckage of the unitdef (default), 0 := no it isn't, 1 := yes it is
	int  resurrectable;

	int smokeTime;

	bool destructable;
	bool reclaimable;
	bool autoreclaim;
	bool blocking;
	bool burnable;
	bool floating;
	bool noSelect;

	bool geoThermal;

	/// name of feature that this turn into when killed (not reclaimed)
	std::string deathFeature;

	/// each size is 8 units
	int xsize;
	/// each size is 8 units
	int zsize;

	std::map<std::string, std::string> customParams;
};

#endif
