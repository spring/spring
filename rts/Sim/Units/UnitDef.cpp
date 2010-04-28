/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDef.h"

#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Rendering/Models/IModelParser.h"
#include "UnitDefHandler.h"

UnitDef::UnitDefWeapon::UnitDefWeapon()
: name("NOWEAPON")
, def(NULL)
, slavedTo(0)
, mainDir(0, 0, 1)
, maxAngleDif(-1)
, fuelUsage(0)
, badTargetCat(0)
, onlyTargetCat(0)
{
}

UnitDef::UnitDefWeapon::UnitDefWeapon(
	std::string name, const WeaponDef* def, int slavedTo, float3 mainDir, float maxAngleDif,
	unsigned int badTargetCat, unsigned int onlyTargetCat, float fuelUse):
	name(name),
	def(def),
	slavedTo(slavedTo),
	mainDir(mainDir),
	maxAngleDif(maxAngleDif),
	fuelUsage(fuelUse),
	badTargetCat(badTargetCat),
	onlyTargetCat(onlyTargetCat)
{}


UnitDef::~UnitDef()
{
	for (std::vector<CExplosionGenerator*>::iterator it = sfxExplGens.begin(); it != sfxExplGens.end(); ++it) {
		delete *it;
	}
}

S3DModel* UnitDef::LoadModel() const
{
	// not exactly kosher, but...
	UnitDef* udef = const_cast<UnitDef*>(this);

	if (udef->modelDef.model == NULL) {
		udef->modelDef.model = modelParser->Load3DModel(udef->modelDef.modelpath, udef->modelCenterOffset);
	}

	return (udef->modelDef.model);
}

BuildInfo::BuildInfo(const std::string& name, const float3& p, int facing)
{
	def = unitDefHandler->GetUnitDefByName(name);
	pos = p;
	buildFacing = abs(facing) % 4;
}


void BuildInfo::FillCmd(Command& c) const
{
	c.id = -def->id;
	c.params.resize(4);
	c.params[0] = pos.x;
	c.params[1] = pos.y;
	c.params[2] = pos.z;
	c.params[3] = (float) buildFacing;
}


bool BuildInfo::Parse(const Command& c)
{
	if (c.params.size() >= 3) {
		pos = float3(c.params[0],c.params[1],c.params[2]);

		if(c.id < 0) {
			def = unitDefHandler->GetUnitDefByID(-c.id);
			buildFacing = 0;

			if (c.params.size() == 4)
				buildFacing = int(abs(c.params[3])) % 4;

			return true;
		}
	}
	return false;
}
