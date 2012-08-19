/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SolidObjectDef.h"
#include "Lua/LuaParser.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/EventHandler.h"

void SolidObjectDecalDef::Parse(const LuaTable& table) {
	groundDecalTypeName = table.GetString("groundDecalType", table.GetString("buildingGroundDecalType", ""));
	trackDecalTypeName = table.GetString("trackType", "StdTank");

	useGroundDecal = table.GetBool("useGroundDecal", table.GetBool("useBuildingGroundDecal", false));
	groundDecalSizeX = table.GetInt("groundDecalSizeX", table.GetInt("buildingGroundDecalSizeX", 4));
	groundDecalSizeY = table.GetInt("groundDecalSizeY", table.GetInt("buildingGroundDecalSizeY", 4));
	groundDecalDecaySpeed = table.GetFloat("groundDecalDecaySpeed", table.GetFloat("buildingGroundDecalDecaySpeed", 0.1f));
	groundDecalType = -1;

	leaveTrackDecals   = table.GetBool("leaveTracks", false);
	trackDecalWidth    = table.GetFloat("trackWidth",   32.0f);
	trackDecalOffset   = table.GetFloat("trackOffset",   0.0f);
	trackDecalStrength = table.GetFloat("trackStrength", 0.0f);
	trackDecalStretch  = table.GetFloat("trackStretch",  1.0f);
	trackDecalType = -1;
}



SolidObjectDef::SolidObjectDef()
	: id(-1)

	, xsize(0)
	, zsize(0)

	, metal(0.0f)
	, energy(0.0f)
	, health(0.0f)
	, mass(0.0f)
	, crushResistance(0.0f)

	, blocking(false)
	, upright(false)
	, reclaimable(true)

	, model(NULL)
	, collisionVolume(NULL)
{
}

SolidObjectDef::~SolidObjectDef() {
	delete collisionVolume;
	collisionVolume = NULL;
}

S3DModel* SolidObjectDef::LoadModel() const
{
	if (model == NULL) {
		model = modelParser->Load3DModel(modelName);
	} else {
		eventHandler.LoadedModelRequested();
	}

	return model;
}

float SolidObjectDef::GetModelRadius() const
{
	return (LoadModel()->radius);
}

void SolidObjectDef::SetCollisionVolume(const LuaTable& table)
{
	collisionVolume = new CollisionVolume(
		table.GetString("collisionVolumeType", ""),
		table.GetFloat3("collisionVolumeScales", ZeroVector),
		table.GetFloat3("collisionVolumeOffsets", ZeroVector),
		CollisionVolume::COLVOL_HITTEST_CONT
	);
}

