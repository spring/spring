/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "UnitLoader.h"
#include "Unit.h"
#include "UnitDef.h"
#include "UnitDefHandler.h"

#include "COB/UnitScript.h"

#include "UnitTypes/Builder.h"
#include "UnitTypes/ExtractorBuilding.h"
#include "UnitTypes/Factory.h"
#include "UnitTypes/TransportUnit.h"

#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/MobileCAI.h"
#include "CommandAI/TransportCAI.h"

#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"

#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/bombdropper.h"
#include "Sim/Weapons/Cannon.h"
#include "Sim/Weapons/DGunWeapon.h"
#include "Sim/Weapons/EmgCannon.h"
#include "Sim/Weapons/FlameThrower.h"
#include "Sim/Weapons/LaserCannon.h"
#include "Sim/Weapons/LightningCannon.h"
#include "Sim/Weapons/MeleeWeapon.h"
#include "Sim/Weapons/MissileLauncher.h"
#include "Sim/Weapons/NoWeapon.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Rifle.h"
#include "Sim/Weapons/StarburstLauncher.h"
#include "Sim/Weapons/TorpedoLauncher.h"

#include "System/EventBatchHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitLoader* unitLoader = NULL;

CUnit* CUnitLoader::LoadUnit(const std::string& name, const float3& pos, int team,
                             bool build, int facing, const CUnit* builder)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByName(name);
	if (ud == NULL) {
		throw content_error("Couldn't find unittype " +  name);
	}

	return LoadUnit(ud, pos, team, build, facing, builder);
}


CUnit* CUnitLoader::LoadUnit(const UnitDef* ud, const float3& pos, int team,
                             bool build, int facing, const CUnit* builder)
{
	GML_RECMUTEX_LOCK(sel); // LoadUnit - for anti deadlock purposes.
	GML_RECMUTEX_LOCK(quad); // LoadUnit - make sure other threads cannot access an incomplete unit

	SCOPED_TIMER("UnitLoader::LoadUnit");

	if (team < 0) {
		team = teamHandler->GaiaTeamID(); // FIXME use gs->gaiaTeamID ?  (once it is always enabled)
		if (team < 0)
			throw content_error("Invalid team and no gaia team to put unit in");
	}

	CUnit* unit = NULL;

	if (ud->IsTransportUnit()) {
		unit = new CTransportUnit();
	} else if (ud->IsFactoryUnit()) {
		// static builders that can be given move-orders
		// (which are passed on to all mobile buildees)
		unit = new CFactory();
	} else if (ud->IsMobileBuilderUnit() || ud->IsStaticBuilderUnit()) {
		// all other types of "builders", including hubs and nano-towers
		// (the latter should not have any build-options at all, whereas
		// the former should be unable to build any mobile units)
		unit = new CBuilder();
	} else if (ud->IsImmobileUnit()) {
		// static non-builder structure
		if (ud->IsExtractorUnit()) {
			unit = new CExtractorBuilding();
		} else {
			unit = new CBuilding();
		}
	} else {
		// regular mobile unit
		unit = new CUnit();
	}

	unit->PreInit(ud, team, facing, pos, build);

	if (ud->IsTransportUnit()) {
		new CTransportCAI(unit);
	} else if (ud->IsFactoryUnit()) {
		new CFactoryCAI(unit);
	} else if (ud->IsMobileBuilderUnit() || ud->IsStaticBuilderUnit()) {
		new CBuilderCAI(unit);
	} else if (ud->IsFighterUnit() || ud->IsBomberUnit()) {
		// non-hovering aircraft types; coupled to AirMoveType
		new CAirCAI(unit);
	} else if (ud->IsAirUnit()) {
		// all other aircraft
		new CMobileCAI(unit);
	} else if (ud->IsGroundUnit()) {
		new CMobileCAI(unit);
	} else {
		new CCommandAI(unit);
	}

	unit->PostInit(builder);
	(eventBatchHandler->GetUnitCreatedDestroyedBatch()).enqueue(EventBatchHandler::UD(unit, unit->isCloaked));

	return unit;
}

CWeapon* CUnitLoader::LoadWeapon(CUnit* owner, const UnitDefWeapon* udw)
{
	CWeapon* weapon = NULL;
	const WeaponDef* weaponDef = udw->def;

	if (udw->name == "NOWEAPON") {
		weapon = new CNoWeapon(owner);
	} else if (weaponDef->type == "Cannon") {
		weapon = new CCannon(owner);
		((CCannon*)weapon)->selfExplode = weaponDef->selfExplode;
	} else if (weaponDef->type == "Rifle") {
		weapon = new CRifle(owner);
	} else if (weaponDef->type == "Melee") {
		weapon = new CMeleeWeapon(owner);
	} else if (weaponDef->type == "AircraftBomb") {
		weapon = new CBombDropper(owner, false);
	} else if (weaponDef->type == "Shield") {
		weapon = new CPlasmaRepulser(owner);
	} else if (weaponDef->type == "Flame") {
		weapon = new CFlameThrower(owner);
	} else if (weaponDef->type == "MissileLauncher") {
		weapon = new CMissileLauncher(owner);
	} else if (weaponDef->type == "TorpedoLauncher") {
		if (owner->unitDef->canfly && !weaponDef->submissile) {
			weapon = new CBombDropper(owner, true);
			if (weaponDef->tracks)
				((CBombDropper*) weapon)->tracking = weaponDef->turnrate;
			((CBombDropper*) weapon)->bombMoveRange = weaponDef->range;
		} else {
			weapon = new CTorpedoLauncher(owner);
			if (weaponDef->tracks)
				((CTorpedoLauncher*) weapon)->tracking = weaponDef->turnrate;
		}
	} else if (weaponDef->type == "LaserCannon") {
		weapon = new CLaserCannon(owner);
		((CLaserCannon*) weapon)->color = weaponDef->visuals.color;
	} else if (weaponDef->type == "BeamLaser") {
		weapon = new CBeamLaser(owner);
		((CBeamLaser*) weapon)->color = weaponDef->visuals.color;
	} else if (weaponDef->type == "LightningCannon") {
		weapon = new CLightningCannon(owner);
		((CLightningCannon*) weapon)->color = weaponDef->visuals.color;
	} else if (weaponDef->type == "EmgCannon") {
		weapon = new CEmgCannon(owner);
	} else if (weaponDef->type == "DGun") {
		weapon = new CDGunWeapon(owner);
	} else if (weaponDef->type == "StarburstLauncher") {
		weapon = new CStarburstLauncher(owner);
		if (weaponDef->tracks)
			((CStarburstLauncher*) weapon)->tracking = weaponDef->turnrate;
		else
			((CStarburstLauncher*) weapon)->tracking = 0;
		((CStarburstLauncher*) weapon)->uptime = weaponDef->uptime * GAME_SPEED;
	} else {
		weapon = new CNoWeapon(owner);
		LogObject() << "Unknown weapon type " << weaponDef->type.c_str() << "\n";
	}
	weapon->weaponDef = weaponDef;

	weapon->reloadTime = int(weaponDef->reload * GAME_SPEED);
	if (weapon->reloadTime == 0)
		weapon->reloadTime = 1;
	weapon->range = weaponDef->range;
//	weapon->baseRange = weaponDef->range;
	weapon->heightMod = weaponDef->heightmod;
	weapon->projectileSpeed = weaponDef->projectilespeed;
//	weapon->baseProjectileSpeed = weaponDef->projectilespeed / GAME_SPEED;

	weapon->areaOfEffect = weaponDef->areaOfEffect;
	weapon->accuracy = weaponDef->accuracy;
	weapon->sprayAngle = weaponDef->sprayAngle;

	weapon->stockpileTime = int(weaponDef->stockpileTime * GAME_SPEED);

	weapon->salvoSize = weaponDef->salvosize;
	weapon->salvoDelay = int(weaponDef->salvodelay * GAME_SPEED);
	weapon->projectilesPerShot = weaponDef->projectilespershot;

	weapon->metalFireCost = weaponDef->metalcost;
	weapon->energyFireCost = weaponDef->energycost;

	weapon->fireSoundId = weaponDef->firesound.getID(0);
	weapon->fireSoundVolume = weaponDef->firesound.getVolume(0);

	weapon->onlyForward = weaponDef->onlyForward;
	if (owner->unitDef->IsFighterUnit() && !owner->unitDef->hoverAttack) {
		// fighter aircraft have too big tolerance in TA
		weapon->maxAngleDif = cos(weaponDef->maxAngle * 0.4f / 180 * PI);
	} else {
		weapon->maxAngleDif = cos(weaponDef->maxAngle / 180 * PI);
	}

	weapon->SetWeaponNum(owner->weapons.size());

	weapon->badTargetCategory = udw->badTargetCat;
	weapon->onlyTargetCategory = weaponDef->onlyTargetCategory & udw->onlyTargetCat;

	if (udw->slavedTo) {
		const int index = (udw->slavedTo - 1);
		if ((index < 0) || (static_cast<size_t>(index) >= owner->weapons.size())) {
			throw content_error("Bad weapon slave in " + owner->unitDef->name);
		}
		weapon->slavedTo = owner->weapons[index];
	}

	weapon->mainDir = udw->mainDir;
	weapon->maxMainDirAngleDif = udw->maxAngleDif;

	weapon->fuelUsage = udw->fuelUsage;
	weapon->avoidFriendly = weaponDef->avoidFriendly;
	weapon->avoidFeature = weaponDef->avoidFeature;
	weapon->avoidNeutral = weaponDef->avoidNeutral;
	weapon->targetBorder = weaponDef->targetBorder;
	weapon->cylinderTargetting = weaponDef->cylinderTargetting;
	weapon->minIntensity = weaponDef->minIntensity;
	weapon->heightBoostFactor = weaponDef->heightBoostFactor;
	weapon->collisionFlags = weaponDef->collisionFlags;
	weapon->Init();

	return weapon;
}


void CUnitLoader::FlattenGround(const CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;
	const float groundheight = ground->GetHeightReal(unit->pos.x, unit->pos.z);

	if (!mapDamage->disabled && unitDef->levelGround &&
		!(unitDef->floater && groundheight <= 0) &&
		!(unitDef->canmove && (unitDef->speed > 0.0f))) {
		// if we are float-capable, only flatten
		// if the terrain here is above sea level

		BuildInfo bi(unitDef, unit->pos, unit->buildFacing);
		bi.pos = helper->Pos2BuildPos(bi);
		const float hss = 0.5f * SQUARE_SIZE;
		const int tx1 = (int) std::max(0.0f ,(bi.pos.x - (bi.GetXSize() * hss)) / SQUARE_SIZE);
		const int tz1 = (int) std::max(0.0f ,(bi.pos.z - (bi.GetZSize() * hss)) / SQUARE_SIZE);
		const int tx2 = std::min(gs->mapx, tx1 + bi.GetXSize());
		const int tz2 = std::min(gs->mapy, tz1 + bi.GetZSize());

		for (int z = tz1; z <= tz2; z++) {
			for (int x = tx1; x <= tx2; x++) {
				readmap->SetHeight(z * (gs->mapx + 1) + x, bi.pos.y);
			}
		}

		mapDamage->RecalcArea(tx1, tx2, tz1, tz2);
	}
}

void CUnitLoader::RestoreGround(const CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;
	const float groundheight = ground->GetHeightReal(unit->pos.x, unit->pos.z);

	if (!mapDamage->disabled && unitDef->levelGround &&
		!(unitDef->floater && groundheight <= 0) &&
		!(unitDef->canmove && (unitDef->speed > 0.0f))) {

		BuildInfo bi(unitDef, unit->pos, unit->buildFacing);
		bi.pos = helper->Pos2BuildPos(bi);
		const float hss = 0.5f * SQUARE_SIZE;
		const int tx1 = (int) std::max(0.0f ,(bi.pos.x - (bi.GetXSize() * hss)) / SQUARE_SIZE);
		const int tz1 = (int) std::max(0.0f ,(bi.pos.z - (bi.GetZSize() * hss)) / SQUARE_SIZE);
		const int tx2 = std::min(gs->mapx, tx1 + bi.GetXSize());
		const int tz2 = std::min(gs->mapy, tz1 + bi.GetZSize());


		const float* heightmap = readmap->GetHeightmap();
		int num = 0;
		float heightdiff = 0.0f;
		for (int z = tz1; z <= tz2; z++) {
			for (int x = tx1; x <= tx2; x++) {
				int index = z * (gs->mapx + 1) + x;
				heightdiff += heightmap[index] - readmap->orgheightmap[index];
				++num;
			}
		}
		// adjust the terrain profile to match orgheightmap
		heightdiff /= (float)num;
		heightdiff += unit->pos.y - bi.pos.y;
		for (int z = tz1; z <= tz2; z++) {
			for (int x = tx1; x <= tx2; x++) {
				int index = z * (gs->mapx + 1) + x;
				readmap->SetHeight(index, heightdiff + readmap->orgheightmap[index]);
			}
		}
		// but without affecting the build height
		heightdiff = bi.pos.y - helper->Pos2BuildPos(bi).y;
		for (int z = tz1; z <= tz2; z++) {
			for (int x = tx1; x <= tx2; x++) {
				int index = z * (gs->mapx + 1) + x;
				readmap->SetHeight(index, heightdiff + heightmap[index]);
			}
		}

		mapDamage->RecalcArea(tx1, tx2, tz1, tz2);
	}
}
