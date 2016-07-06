/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SolidObjectDef.h"
#include "Lua/LuaParser.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"

SolidObjectDecalDef::SolidObjectDecalDef()
	: useGroundDecal(false)
	, groundDecalType(-1)
	, groundDecalSizeX(-1)
	, groundDecalSizeY(-1)
	, groundDecalDecaySpeed(0.0f)

	, leaveTrackDecals(false)
	, trackDecalType(-1)
	, trackDecalWidth(0.0f)
	, trackDecalOffset(0.0f)
	, trackDecalStrength(0.0f)
	, trackDecalStretch(0.0f)
{}

void SolidObjectDecalDef::Parse(const LuaTable& table) {
	groundDecalTypeName = table.GetString("groundDecalType", table.GetString("buildingGroundDecalType", ""));
	trackDecalTypeName = table.GetString("trackType", "StdTank");

	useGroundDecal        = table.GetBool("useGroundDecal", table.GetBool("useBuildingGroundDecal", false));
	groundDecalType       = -1;
	groundDecalSizeX      = table.GetInt("groundDecalSizeX", table.GetInt("buildingGroundDecalSizeX", 4));
	groundDecalSizeY      = table.GetInt("groundDecalSizeY", table.GetInt("buildingGroundDecalSizeY", 4));
	groundDecalDecaySpeed = table.GetFloat("groundDecalDecaySpeed", table.GetFloat("buildingGroundDecalDecaySpeed", 0.1f));

	leaveTrackDecals   = table.GetBool("leaveTracks", false);
	trackDecalType     = -1;
	trackDecalWidth    = table.GetFloat("trackWidth",   32.0f);
	trackDecalOffset   = table.GetFloat("trackOffset",   0.0f);
	trackDecalStrength = table.GetFloat("trackStrength", 0.0f);
	trackDecalStretch  = table.GetFloat("trackStretch",  1.0f);
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

	, collidable(false)
	, selectable(true)
	, upright(false)
	, reclaimable(true)

	, model(nullptr)
{
}

void SolidObjectDef::PreloadModel() const
{
	if (model == nullptr && !modelName.empty()) {
		modelLoader.PreloadModel(modelName);
	}
}

S3DModel* SolidObjectDef::LoadModel() const
{
	if (model == nullptr) {
		if (!modelName.empty()) {
			model = modelLoader.LoadModel(modelName);
		} else {
			// not useful, too much spam
			// LOG_L(L_WARNING, "[SolidObjectDef::%s] object \"%s\" has no model defined", __FUNCTION__, name.c_str());
		}
	}

	return model;
}

float SolidObjectDef::GetModelRadius() const
{
	return (LoadModel()->radius);
}

void SolidObjectDef::ParseCollisionVolume(const LuaTable& table)
{
	collisionVolume = CollisionVolume(
		table.GetString("collisionVolumeType", ""),
		table.GetFloat3("collisionVolumeScales", ZeroVector),
		table.GetFloat3("collisionVolumeOffsets", ZeroVector)
	);

	// if this unit wants per-piece volumes, make
	// its main collision volume deferent and let
	// it ignore hits
	collisionVolume.SetDefaultToPieceTree(table.GetBool("usePieceCollisionVolumes", false));
	collisionVolume.SetDefaultToFootPrint(table.GetBool("useFootPrintCollisionVolume", false));
	collisionVolume.SetIgnoreHits(collisionVolume.DefaultToPieceTree());
}

