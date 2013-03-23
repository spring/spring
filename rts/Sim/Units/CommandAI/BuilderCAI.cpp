/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BuilderCAI.h"

#include <assert.h>

#include "TransportCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitSet.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Map.h"


CR_BIND_DERIVED(CBuilderCAI ,CMobileCAI , );

CR_REG_METADATA(CBuilderCAI , (
				CR_MEMBER(buildOptions),
				CR_MEMBER(owner_builder),
				CR_MEMBER(building),
				CR_MEMBER(range3D),
				CR_IGNORED(build),

				CR_MEMBER(cachedRadiusId),
				CR_MEMBER(cachedRadius),

				CR_MEMBER(buildRetries),
				CR_MEMBER(randomCounter),

				CR_MEMBER(lastPC1),
				CR_MEMBER(lastPC2),
				CR_MEMBER(lastPC3),
				CR_POSTLOAD(PostLoad)
				));

// not adding to members, should repopulate itself
CUnitSet CBuilderCAI::reclaimers;
CUnitSet CBuilderCAI::featureReclaimers;
CUnitSet CBuilderCAI::resurrecters;


CBuilderCAI::CBuilderCAI():
	CMobileCAI(),
	owner_builder(NULL),
	building(false),
	cachedRadiusId(0),
	cachedRadius(0),
	buildRetries(0),
	randomCounter(0),
	lastPC1(-1),
	lastPC2(-1),
	lastPC3(-1),
	range3D(true)
{}


CBuilderCAI::CBuilderCAI(CUnit* owner):
	CMobileCAI(owner),
	building(false),
	cachedRadiusId(0),
	cachedRadius(0),
	buildRetries(0),
	randomCounter(0),
	lastPC1(-1),
	lastPC2(-1),
	lastPC3(-1),
	range3D(owner->unitDef->buildRange3D)
{
	owner_builder = static_cast<CBuilder*>(owner);

	CommandDescription c;
	if (owner->unitDef->canRepair) {
		c.id = CMD_REPAIR;
		c.action = "repair";
		c.type = CMDTYPE_ICON_UNIT_OR_AREA;
		c.name = "Repair";
		c.mouseicon = c.name;
		c.tooltip = "Repair: Repairs another unit";
		possibleCommands.push_back(c);
	}
	else if (owner->unitDef->canAssist) {
		c.id = CMD_REPAIR;
		c.action = "assist";
		c.type = CMDTYPE_ICON_UNIT_OR_AREA;
		c.name = "Assist";
		c.mouseicon = c.name;
		c.tooltip = "Assist: Help build something";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canReclaim) {
		c.id = CMD_RECLAIM;
		c.action = "reclaim";
		c.type = CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
		c.name = "Reclaim";
		c.mouseicon = c.name;
		c.tooltip = "Reclaim: Sucks in the metal/energy content of a unit/feature and add it to your storage";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canRestore && !mapDamage->disabled) {
		c.id = CMD_RESTORE;
		c.action = "restore";
		c.type = CMDTYPE_ICON_AREA;
		c.name = "Restore";
		c.mouseicon = c.name;
		c.tooltip = "Restore: Restores an area of the map to its original height";
		c.params.push_back("200");
		possibleCommands.push_back(c);
	}
	c.params.clear();

	if (owner->unitDef->canResurrect) {
		c.id = CMD_RESURRECT;
		c.action = "resurrect";
		c.type = CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
		c.name = "Resurrect";
		c.mouseicon = c.name;
		c.tooltip = "Resurrect: Resurrects a unit from a feature";
		possibleCommands.push_back(c);
	}
	if (owner->unitDef->canCapture) {
		c.id = CMD_CAPTURE;
		c.action = "capture";
		c.type = CMDTYPE_ICON_UNIT_OR_AREA;
		c.name = "Capture";
		c.mouseicon = c.name;
		c.tooltip = "Capture: Captures a unit from the enemy";
		possibleCommands.push_back(c);
	}

	map<int, string>::const_iterator bi;
	for (bi = owner_builder->unitDef->buildOptions.begin(); bi != owner_builder->unitDef->buildOptions.end(); ++bi) {
		const string name = bi->second;
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(name);
		if (ud == NULL) {
		  string errmsg = "MOD ERROR: loading ";
		  errmsg += name.c_str();
		  errmsg += " for ";
		  errmsg += owner->unitDef->name;
			throw content_error(errmsg);
		}
		CommandDescription c;
		c.id = -ud->id; //build options are always negative
		c.action = "buildunit_" + StringToLower(ud->name);
		c.type = CMDTYPE_ICON_BUILDING;
		c.name = name;
		c.mouseicon = c.name;
		c.disabled = (ud->maxThisUnit <= 0);

		char tmp[1024];
		sprintf(tmp, "\nHealth %.0f\nMetal cost %.0f\nEnergy cost %.0f Build time %.0f",
		        ud->health, ud->metal, ud->energy, ud->buildTime);
		if (c.disabled) {
			c.tooltip = "\xff\xff\x22\x22" "DISABLED: " "\xff\xff\xff\xff";
		} else {
			c.tooltip = "Build: ";
		}
		c.tooltip += ud->humanName + " - " + ud->tooltip + tmp;

		possibleCommands.push_back(c);
		buildOptions[c.id] = name;
	}
	unitHandler->AddBuilderCAI(this);
}

CBuilderCAI::~CBuilderCAI()
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (unitHandler != NULL) {
		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		RemoveUnitFromResurrecters(owner);
		unitHandler->RemoveBuilderCAI(this);
	}
}

void CBuilderCAI::PostLoad()
{
	if (!commandQue.empty()) {
		owner_builder = static_cast<CBuilder*>(owner);

		Command& c = commandQue.front();

		map<int, string>::iterator boi = buildOptions.find(c.GetID());
		if (boi != buildOptions.end()) {
			build.Parse(c);
			build.pos = CGameHelper::Pos2BuildPos(build, true);
		}
	}
}



inline float CBuilderCAI::GetBuildRange(const float targetRadius) const
{
	// for immobile:
	// only use `buildDistance + radius` iff radius > buildDistance,
	// and so it would be impossible to get in buildrange (collision detection with units/features)
	//
	// what does this even mean?? IMMOBILE units cannot "get in range" of anything
	if (owner->immobile) {
		return (owner_builder->buildDistance + std::max(targetRadius - owner_builder->buildDistance, 0.0f));
	}

	return (owner_builder->buildDistance + targetRadius);
}



bool CBuilderCAI::IsInBuildRange(const CWorldObject* obj) const
{
	return IsInBuildRange(obj->pos, obj->radius);
}

bool CBuilderCAI::IsInBuildRange(const float3& objPos, const float objRadius) const
{
	const float immDistSqr = f3SqDist(owner->pos, objPos);
	const float buildDist = GetBuildRange(objRadius);

	return (immDistSqr <= (buildDist * buildDist));
}



inline bool CBuilderCAI::MoveInBuildRange(const CWorldObject* obj, const bool checkMoveTypeForFailed)
{
	return MoveInBuildRange(obj->pos, obj->radius, checkMoveTypeForFailed);
}

bool CBuilderCAI::MoveInBuildRange(const float3& objPos, float objRadius, const bool checkMoveTypeForFailed)
{
	if (!IsInBuildRange(objPos, objRadius)) {
		// NOTE:
		//   ignore the fail-check if we are an aircraft, movetype code
		//   is unreliable wrt. setting it correctly and causes (landed)
		//   aircraft to discard orders
		const bool checkFailed = (checkMoveTypeForFailed && !owner->unitDef->IsAirUnit());
		// check if the AMoveType::Failed belongs to the same goal position
		const bool haveFailed = (owner->moveType->progressState == AMoveType::Failed && f3SqDist(goalPos, objPos) > 1.0f);

		if (checkFailed && haveFailed) {
			// don't call SetGoal() it would reset moveType->progressState
			// and so later code couldn't check if the move command failed
			return false;
		}

		// too far away, start a move command
		SetGoal(objPos, owner->pos, GetBuildRange(objRadius) * 0.9f);
		return false;
	}

	if (owner->unitDef->IsAirUnit()) {
		StopMoveAndKeepPointing(objPos, objRadius);
	} else {
		StopMoveAndKeepPointing(goalPos, goalRadius);
	}

	return true;
}


bool CBuilderCAI::IsBuildPosBlocked(const BuildInfo& build, const CUnit* nanoFrame) const
{
	CFeature* feature = NULL;
	CGameHelper::BuildSquareStatus status = CGameHelper::TestUnitBuildSquare(build, feature, owner->allyteam, true);

	if (feature && build.def->isFeature && build.def->wreckName == feature->def->name) {
		// buildjob is a feature and it is finished already
		return true;
	}

	if (status != CGameHelper::BUILDSQUARE_BLOCKED) {
		// open area, reclaimable feature or movable unit
		return false;
	}

	const int yardxpos = int(build.pos.x + (SQUARE_SIZE >> 1)) / SQUARE_SIZE;
	const int yardypos = int(build.pos.z + (SQUARE_SIZE >> 1)) / SQUARE_SIZE;
	const CSolidObject* s = groundBlockingObjectMap->GroundBlocked(yardxpos, yardypos);

	if (s == owner) {
		return false;
	}

	const CUnit* u = dynamic_cast<const CUnit*>(s);
	if (!u /*|| u->pos != build.pos*/) {
		return true;
	}

	if (u->beingBuilt) {
		if (!owner_builder->CanAssistUnit(u, build.def)) {
			if (u->immobile) {
				// we can't or don't want assist finishing the nanoframe
				return true;
			} else {
				// mobile unit blocks the position, wait till it is finished & moved
				return false;
			}
		}

		// unfinished nanoframe, assist it
		nanoFrame = u;
		return false;
	}

	// unit blocks the pos, can it move away?
	return u->immobile;
}



inline bool CBuilderCAI::OutOfImmobileRange(const Command& cmd) const
{
	if (owner->unitDef->canmove) {
		return false; // builder can move
	}
	if (((cmd.options & INTERNAL_ORDER) == 0) || (cmd.params.size() != 1)) {
		return false; // not an internal object targetted command
	}

	const int objID = cmd.params[0];
	const CWorldObject* obj = unitHandler->GetUnit(objID);

	if (obj == NULL) {
		// features don't move, but maybe the unit was transported?
		obj = featureHandler->GetFeature(objID - unitHandler->MaxUnits());

		if (obj == NULL) {
			return false;
		}
	}

	switch (cmd.GetID()) {
		case CMD_REPAIR:
		case CMD_RECLAIM:
		case CMD_RESURRECT:
		case CMD_CAPTURE: {
			if (!IsInBuildRange(obj)) {
				return true;
			}
			break;
		}
	}
	return false;
}


float CBuilderCAI::GetBuildOptionRadius(const UnitDef* ud, int cmdId)
{
	float radius;
	if (cachedRadiusId == cmdId) {
		radius = cachedRadius;
	} else {
		radius = ud->GetModelRadius();
		cachedRadius = radius;
		cachedRadiusId = cmdId;
	}
	return radius;
}


void CBuilderCAI::CancelRestrictedUnit(const std::string& buildOption)
{
	if (owner->team == gu->myTeam) {
		LOG_L(L_WARNING, "%s: Build failed, unit type limit reached",
				owner->unitDef->humanName.c_str());
		eventHandler.LastMessagePosition(owner->pos);
	}
	FinishCommand();
}


void CBuilderCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced))
		return;

	// don't guard yourself
	if ((c.GetID() == CMD_GUARD) &&
	    (c.params.size() == 1) && ((int)c.params[0] == owner->id)) {
		return;
	}

	// stop current build if the new command is a not queued and replaces the current buidcmd
	//FIXME should happen just before CMobileCAI::GiveCommandReal? (the new cmd can still be skipped!)
	if (!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.GetID()) == nonQueingCommands.end()
			&& c.GetID() != CMD_WAIT) {
		building = false;
		static_cast<CBuilder*>(owner)->StopBuild();
	}

	const map<int,string>::const_iterator boi = buildOptions.find(c.GetID());

	if (boi != buildOptions.end()) {
		if (c.params.size() < 3) {
			return;
		}

		BuildInfo bi;
		bi.pos = c.GetPos(0);

		if (c.params.size() == 4)
			bi.buildFacing = abs((int)c.params[3]) % NUM_FACINGS;

		bi.def = unitDefHandler->GetUnitDefByName(boi->second);
		bi.pos = CGameHelper::Pos2BuildPos(bi, true);

		// We are a static building, check if the buildcmd is in range
		if (!owner->unitDef->canmove) {
			const float radius = GetBuildOptionRadius(bi.def, c.GetID());
			if (!IsInBuildRange(bi.pos, radius)) {
				return;
			}
		}

		const CUnit* nanoFrame = NULL;
		// check if the buildpos is blocked
		if (IsBuildPosBlocked(bi, nanoFrame)) {
			return;
		}

		// if it is a nanoframe help to finish it
		if (nanoFrame) {
			Command c2(CMD_REPAIR, c.options | INTERNAL_ORDER, nanoFrame->id);
			CMobileCAI::GiveCommandReal(c2, fromSynced);
			CMobileCAI::GiveCommandReal(c, fromSynced);
			return;
		}
	} else {
		if (c.GetID() < 0)
			return;
	}

	CMobileCAI::GiveCommandReal(c, fromSynced);
}


void CBuilderCAI::SlowUpdate()
{
	if (gs->paused) // Commands issued may invoke SlowUpdate when paused
		return;

	if (commandQue.empty()) {
		CMobileCAI::SlowUpdate();
		return;
	}

	if (owner->beingBuilt || owner->IsStunned()) {
		return;
	}

	Command& c = commandQue.front();

	if (OutOfImmobileRange(c)) {
		FinishCommand();
		return;
	}

	switch (c.GetID()) {
		case CMD_STOP:      { ExecuteStop(c);      return; }
		case CMD_REPAIR:    { ExecuteRepair(c);    return; }
		case CMD_CAPTURE:   { ExecuteCapture(c);   return; }
		case CMD_GUARD:     { ExecuteGuard(c);     return; }
		case CMD_RECLAIM:   { ExecuteReclaim(c);   return; }
		case CMD_RESURRECT: { ExecuteResurrect(c); return; }
		case CMD_PATROL:    { ExecutePatrol(c);    return; }
		case CMD_FIGHT:     { ExecuteFight(c);     return; }
		case CMD_RESTORE:   { ExecuteRestore(c);   return; }
		default: {
			if (c.GetID() < 0) {
				ExecuteBuildCmd(c);
			} else {
				CMobileCAI::SlowUpdate();
			}
			return;
		}
	}
}


void CBuilderCAI::ReclaimFeature(CFeature* f)
{
	if (!owner->unitDef->canReclaim || !f->def->reclaimable) {
		// FIXME user shouldn't be able to queue buildings on top of features
		// in the first place (in this case).
		StopMove();
		FinishCommand();
	} else {
		Command c2(CMD_RECLAIM, 0, f->id + unitHandler->MaxUnits());
		commandQue.push_front(c2);
		// this assumes that the reclaim command can never return directly
		// without having reclaimed the target
		SlowUpdate();
	}
}


void CBuilderCAI::FinishCommand()
{
	if (commandQue.front().timeOut == INT_MAX) {
		buildRetries = 0;
	}
	CMobileCAI::FinishCommand();
}


void CBuilderCAI::ExecuteStop(Command& c)
{
	building = false;
	owner_builder->StopBuild();
	CMobileCAI::ExecuteStop(c);
}


void CBuilderCAI::ExecuteBuildCmd(Command& c)
{
	const map<int, string>::const_iterator boi = buildOptions.find(c.GetID());

	if (boi == buildOptions.end())
		return;

	if (!inCommand) {
		BuildInfo bi;
		bi.pos.x = math::floor(c.params[0] / SQUARE_SIZE + 0.5f) * SQUARE_SIZE;
		bi.pos.z = math::floor(c.params[2] / SQUARE_SIZE + 0.5f) * SQUARE_SIZE;
		bi.pos.y = c.params[1];

		if (c.params.size() == 4)
			bi.buildFacing = abs((int)c.params[3]) % NUM_FACINGS;

		bi.def = unitDefHandler->GetUnitDefByName(boi->second);

		CFeature* f = NULL;
		CGameHelper::TestUnitBuildSquare(bi, f, owner->allyteam, true);

		if (f != NULL) {
			if (!bi.def->isFeature || bi.def->wreckName != f->def->name) {
				ReclaimFeature(f);
			} else {
				FinishCommand();
			}
			return;
		}

		inCommand = true;
		build.Parse(c);
	}

	assert(build.def->id == -c.GetID() && build.def->id != 0);
	const float buildeeRadius = GetBuildOptionRadius(build.def, c.GetID());

	if (building) {
		// keep moving until 3D distance to buildPos is LEQ our buildDistance
		MoveInBuildRange(build.pos, 0.0f);

		if (!owner_builder->curBuild && !owner_builder->terraforming) {
			building = false;
			StopMove(); // cancel the effect of KeepPointingTo
			FinishCommand();
		}
		// This can only be true if two builders started building
		// the restricted unit in the same simulation frame
		else if (unitHandler->unitsByDefs[owner->team][build.def->id].size() > build.def->maxThisUnit) {
			// unit restricted
			building = false;
			owner_builder->StopBuild();
			CancelRestrictedUnit(boi->second);
		}
	} else {
		if (unitHandler->unitsByDefs[owner->team][build.def->id].size() >= build.def->maxThisUnit) {
			// unit restricted, don't bother moving all the way
			// to the construction site first before telling us
			// (since greyed-out icons can still be clicked etc,
			// would be better to prevent that but doesn't cover
			// case where limit reached while builder en-route)
			CancelRestrictedUnit(boi->second);
			StopMove();
			return;
		}

		build.pos = CGameHelper::Pos2BuildPos(build, true);

		// keep moving until until 3D distance to buildPos is LEQ our buildDistance
		if (MoveInBuildRange(build.pos, 0.0f, true)) {
			if (IsBuildPosBlocked(build)) {
				StopMove();
				FinishCommand();
				return;
			}

			if (luaRules && !luaRules->AllowUnitCreation(build.def, owner, &build)) {
				StopMove(); // cancel KeepPointingTo
				FinishCommand();
				return;
			}

			if (teamHandler->Team(owner->team)->AtUnitLimit()) {
				return;
			}

			CFeature* f = NULL;

			bool waitstance = false;
			if (owner_builder->StartBuild(build, f, waitstance) || (++buildRetries > 30)) {
				building = true;
			}
			else if (f != NULL && (!build.def->isFeature || build.def->wreckName != f->def->name)) {
				inCommand = false;
				ReclaimFeature(f);
			}
			else if (!waitstance) {
				const float fpSqRadius = (build.def->xsize * build.def->xsize + build.def->zsize * build.def->zsize);
				const float fpRadius = (math::sqrt(fpSqRadius) * 0.5f) * SQUARE_SIZE;

				// tell everything within the radius of the soon-to-be buildee
				// to get out of the way; using the model radius is not correct
				// because this can be shorter than half the footprint diagonal
				CGameHelper::BuggerOff(build.pos, std::max(buildeeRadius, fpRadius), false, true, owner->team, NULL);
				NonMoving();
			}
		} else {
			if (owner->moveType->progressState == AMoveType::Failed) {
				if (++buildRetries > 5) {
					StopMove();
					FinishCommand();
					return;
				}
			}

			// we are on the way to the buildpos, meanwhile it can happen
			// that another builder already finished our buildcmd or blocked
			// the buildpos with another building (skip our buildcmd then)
			if ((++randomCounter % 5) == 0) {
				if (IsBuildPosBlocked(build)) {
					StopMove();
					FinishCommand();
					return;
				}
			}
		}
	}
}


bool CBuilderCAI::TargetInterceptable(const CUnit* unit, float targetSpeed) {
	// if the target is moving away at a higher speed than we can manage, there is little point in chasing it
	const float maxSpeed = owner->moveType->GetMaxSpeed();
	if (targetSpeed <= maxSpeed)
		return true;
	const float3 unitToPos = unit->pos - owner->pos;
	return (unitToPos.dot2D(unit->speed) <= unitToPos.Length2D() * maxSpeed);
}


void CBuilderCAI::ExecuteRepair(Command& c)
{
	// not all builders are repair-capable by default
	if (!owner->unitDef->canRepair)
		return;

	if (c.params.size() == 1 || c.params.size() == 5) {
		// repair unit
		CUnit* unit = unitHandler->GetUnit(c.params[0]);

		if (unit == NULL) {
			FinishCommand();
			return;
		}

		if (tempOrder && owner->moveState <= MOVESTATE_MANEUVER) {
			// limit how far away we go when not roaming
			if (LinePointDist(commandPos1, commandPos2, unit->pos) > std::max(500.0f, GetBuildRange(unit->radius))) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		if (c.params.size() == 5) {
			const float3& pos = c.GetPos(1);
			const float radius = c.params[4] + 100.0f; // do not walk too far outside repair area

			if ((pos - unit->pos).SqLength2D() > radius * radius ||
				(unit->isMoving && (((c.options & INTERNAL_ORDER) && !TargetInterceptable(unit, unit->speed.Length2D())) || owner_builder->curBuild == unit)
				&& !IsInBuildRange(unit))) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		// do not consider units under construction irreparable
		// even if they can be repaired
		bool canRepairUnit = true;
		canRepairUnit &= ((unit->beingBuilt) || (unit->unitDef->repairable && (unit->health < unit->maxHealth)));
		canRepairUnit &= ((unit != owner) || owner->unitDef->canSelfRepair);
		canRepairUnit &= (!unit->soloBuilder || (unit->soloBuilder == owner));
		canRepairUnit &= (!(c.options & INTERNAL_ORDER) || (c.options & CONTROL_KEY) || !IsUnitBeingReclaimed(unit, owner));
		canRepairUnit &= (UpdateTargetLostTimer(unit->id) != 0);

		if (canRepairUnit) {
			if (MoveInBuildRange(unit)) {
				owner_builder->SetRepairTarget(unit);
			}
		} else {
			StopMove();
			FinishCommand();
		}
	} else if (c.params.size() == 4) {
		// area repair
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];

		owner_builder->StopBuild();
		if (FindRepairTargetAndRepair(pos, radius, c.options, false, (c.options & META_KEY))) {
			inCommand = false;
			SlowUpdate();
			return;
		}
		if (!(c.options & ALT_KEY)) {
			FinishCommand();
		}
	} else {
		FinishCommand();
	}
}


void CBuilderCAI::ExecuteCapture(Command& c)
{
	// not all builders are capture-capable by default
	if (!owner->unitDef->canCapture)
		return;

	if (c.params.size() == 1 || c.params.size() == 5) {
		// capture unit
		CUnit* unit = unitHandler->GetUnit(c.params[0]);

		if (unit == NULL) {
			FinishCommand();
			return;
		}

		if (c.params.size() == 5) {
			const float3& pos = c.GetPos(1);
			const float radius = c.params[4] + 100; // do not walk too far outside capture area

			if (((pos - unit->pos).SqLength2D() > (radius * radius) ||
				(owner_builder->curCapture == unit && unit->isMoving && !IsInBuildRange(unit)))) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		if (unit->unitDef->capturable && unit->team != owner->team && UpdateTargetLostTimer(unit->id)) {
			if (MoveInBuildRange(unit)) {
				owner_builder->SetCaptureTarget(unit);
			}
		} else {
			StopMove();
			FinishCommand();
		}
	} else if (c.params.size() == 4) {
		// area capture
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];

		owner_builder->StopBuild();

		if (FindCaptureTargetAndCapture(pos, radius, c.options, (c.options & META_KEY))) {
			inCommand = false;
			SlowUpdate();
			return;
		}
		if (!(c.options & ALT_KEY)) {
			FinishCommand();
		}
	} else {
		FinishCommand();
	}
}


void CBuilderCAI::ExecuteGuard(Command& c)
{
	if (!owner->unitDef->canGuard)
		return;

	CUnit* guardee = unitHandler->GetUnit(c.params[0]);

	if (guardee == NULL) { FinishCommand(); return; }
	if (guardee == owner) { FinishCommand(); return; }
	if (UpdateTargetLostTimer(guardee->id) == 0) { FinishCommand(); return; }
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) { FinishCommand(); return; }


	if (CBuilder* b = dynamic_cast<CBuilder*>(guardee)) {
		if (b->terraforming) {
			if (MoveInBuildRange(b->terraformCenter, b->terraformRadius * 0.7f)) {
				owner_builder->HelpTerraform(b);
			} else {
				StopSlowGuard();
			}
			return;
		} else if (b->curReclaim && owner->unitDef->canReclaim) {
			StopSlowGuard();
			if (!ReclaimObject(b->curReclaim)) {
				StopMove();
			}
			return;
		} else if (b->curResurrect && owner->unitDef->canResurrect) {
			StopSlowGuard();
			if (!ResurrectObject(b->curResurrect)) {
				StopMove();
			}
			return;
		} else {
			owner_builder->StopBuild();
		}

		const bool pushRepairCommand =
			(  b->curBuild != NULL) &&
			(  b->curBuild->soloBuilder == NULL || b->curBuild->soloBuilder == owner) &&
			(( b->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			( !b->curBuild->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			Command nc(CMD_REPAIR, c.options, b->curBuild->id);

			commandQue.push_front(nc);
			inCommand = false;
			SlowUpdate();
			return;
		}
	}

	if (CFactory* fac = dynamic_cast<CFactory*>(guardee)) {
		const bool pushRepairCommand =
			(  fac->curBuild != NULL) &&
			(  fac->curBuild->soloBuilder == NULL || fac->curBuild->soloBuilder == owner) &&
			(( fac->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			 (!fac->curBuild->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			Command nc(CMD_REPAIR, c.options, fac->curBuild->id);

			commandQue.push_front(nc);
			inCommand = false;
			// SlowUpdate();
			return;
		}
	}

	if (!(c.options & CONTROL_KEY) && IsUnitBeingReclaimed(guardee, owner))
		return;

	const float3 pos    = guardee->pos;
	const float  radius = (guardee->immobile) ? guardee->radius : guardee->radius * 0.8f; // in case of mobile units reduce radius a bit

	if (MoveInBuildRange(pos, radius)) {
		StartSlowGuard(guardee->moveType->GetMaxSpeed());

		const bool pushRepairCommand =
			(  guardee->health < guardee->maxHealth) &&
			(  guardee->soloBuilder == NULL || guardee->soloBuilder == owner) &&
			(( guardee->beingBuilt && owner->unitDef->canAssist) ||
			 (!guardee->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			Command nc(CMD_REPAIR, c.options, guardee->id);

			commandQue.push_front(nc);
			inCommand = false;
			return;
		} else {
			NonMoving();
		}
	} else {
		StopSlowGuard();
	}
}


void CBuilderCAI::ExecuteReclaim(Command& c)
{
	// not all builders are reclaim-capable by default
	if (!owner->unitDef->canReclaim)
		return;

	if (c.params.size() == 1 || c.params.size() == 5) {
		const int signedId = (int) c.params[0];

		if (signedId < 0) {
			LOG_L(L_WARNING, "Trying to reclaim unit or feature with id < 0 (%i), aborting.", signedId);
			return;
		}

		const unsigned int uid = signedId;

		const bool checkForBetterTarget = ((++randomCounter % 5) == 0);
		if (checkForBetterTarget && (c.options & INTERNAL_ORDER) && (c.params.size() >= 5)) {
			// regular check if there is a closer reclaim target
			CSolidObject* obj;
			if (uid >= unitHandler->MaxUnits()) {
				obj = featureHandler->GetFeature(uid - unitHandler->MaxUnits());
			} else {
				obj = unitHandler->GetUnit(uid);
			}
			if (obj) {
				const float3& pos = c.GetPos(1);
				const float radius = c.params[4];
				const float curdist = pos.SqDistance2D(obj->pos);

				const bool recUnits = !!(c.options & META_KEY);
				const bool recEnemyOnly = (c.options & META_KEY) && (c.options & CONTROL_KEY);
				const bool recSpecial = !!(c.options & CONTROL_KEY);

				ReclaimOption recopt = REC_NORESCHECK;
				if (recUnits)     recopt |= REC_UNITS;
				if (recEnemyOnly) recopt |= REC_ENEMYONLY;
				if (recSpecial)   recopt |= REC_SPECIAL;

				const int rid = FindReclaimTarget(pos, radius, c.options, recopt, curdist);
				if ((rid > 0) && (rid != uid)) {
					FinishCommand();
					RemoveUnitFromReclaimers(owner);
					RemoveUnitFromFeatureReclaimers(owner);
					return;
				}
			}
		}

		if (uid >= unitHandler->MaxUnits()) { // reclaim feature
			CFeature* feature = featureHandler->GetFeature(uid - unitHandler->MaxUnits());

			if (feature != NULL) {
				bool featureBeingResurrected = IsFeatureBeingResurrected(feature->id, owner);
				featureBeingResurrected &= (c.options & INTERNAL_ORDER) && !(c.options & CONTROL_KEY);

				if (featureBeingResurrected || !ReclaimObject(feature)) {
					StopMove();
					FinishCommand();
					RemoveUnitFromFeatureReclaimers(owner);
				} else {
					AddUnitToFeatureReclaimers(owner);
				}
			} else {
				StopMove();
				FinishCommand();
				RemoveUnitFromFeatureReclaimers(owner);
			}

			RemoveUnitFromReclaimers(owner);
		} else { // reclaim unit
			CUnit* unit = unitHandler->GetUnit(uid);

			if (unit != NULL && c.params.size() == 5) {
				const float3& pos = c.GetPos(1);
				const float radius = c.params[4] + 100.0f; // do not walk too far outside reclaim area

				const bool outOfReclaimRange =
					(pos.SqDistance2D(unit->pos) > radius * radius) ||
					(owner_builder->curReclaim == unit && unit->isMoving && !IsInBuildRange(unit));
				const bool busyAlliedBuilder =
					unit->unitDef->builder &&
					!unit->commandAI->commandQue.empty() &&
					teamHandler->Ally(owner->allyteam, unit->allyteam);

				if (outOfReclaimRange || busyAlliedBuilder) {
					StopMove();
					RemoveUnitFromReclaimers(owner);
					FinishCommand();
					RemoveUnitFromFeatureReclaimers(owner);
					return;
				}
			}

			if (unit != NULL && unit != owner && unit->unitDef->reclaimable && UpdateTargetLostTimer(unit->id) && unit->AllowedReclaim(owner)) {
				if (!ReclaimObject(unit)) {
					StopMove();
					FinishCommand();
				} else {
					AddUnitToReclaimers(owner);
				}
			} else {
				RemoveUnitFromReclaimers(owner);
				FinishCommand();
			}

			RemoveUnitFromFeatureReclaimers(owner);
		}
	} else if (c.params.size() == 4) {
		// area reclaim
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];
		const bool recUnits = !!(c.options & META_KEY);
		const bool recEnemyOnly = (c.options & META_KEY) && (c.options & CONTROL_KEY);
		const bool recSpecial = !!(c.options & CONTROL_KEY);

		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		owner_builder->StopBuild();

		ReclaimOption recopt = REC_NORESCHECK;
		if (recUnits)     recopt |= REC_UNITS;
		if (recEnemyOnly) recopt |= REC_ENEMYONLY;
		if (recSpecial)   recopt |= REC_SPECIAL;

		if (FindReclaimTargetAndReclaim(pos, radius, c.options, recopt)) {
			inCommand = false;
			SlowUpdate();
			return;
		}
		if(!(c.options & ALT_KEY)){
			FinishCommand();
		}
	} else {
		// wrong number of parameters
		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		FinishCommand();
	}
}


bool CBuilderCAI::ResurrectObject(CFeature *feature) {
	if (MoveInBuildRange(feature, true)) {
		owner_builder->SetResurrectTarget(feature);
	} else {
		if (owner->moveType->progressState == AMoveType::Failed) {
			return false;
		}
	}

	return true;
}


void CBuilderCAI::ExecuteResurrect(Command& c)
{
	// not all builders are resurrect-capable by default
	if (!owner->unitDef->canResurrect)
		return;

	if (c.params.size() == 1) {
		unsigned int id = (unsigned int) c.params[0];

		if (id >= unitHandler->MaxUnits()) { // resurrect feature
			CFeature* feature = featureHandler->GetFeature(id - unitHandler->MaxUnits());

			if (feature && feature->udef != NULL) {
				if (((c.options & INTERNAL_ORDER) && !(c.options & CONTROL_KEY) && IsFeatureBeingReclaimed(feature->id, owner)) ||
					!ResurrectObject(feature)) {
					StopMove();
					RemoveUnitFromResurrecters(owner);
					FinishCommand();
				}
				else {
					AddUnitToResurrecters(owner);
				}
			} else {
				RemoveUnitFromResurrecters(owner);

				if (owner_builder->lastResurrected && unitHandler->GetUnitUnsafe(owner_builder->lastResurrected) != NULL && owner->unitDef->canRepair) {
					// resurrection finished, start repair (by overwriting the current order)
					c = Command(CMD_REPAIR, c.options | INTERNAL_ORDER, owner_builder->lastResurrected);

					owner_builder->lastResurrected = 0;
					inCommand = false;
					SlowUpdate();
					return;
				}
				StopMove();
				FinishCommand();
			}
		} else { // resurrect unit
			RemoveUnitFromResurrecters(owner);
			FinishCommand();
		}
	} else if (c.params.size() == 4) {
		// area resurrect
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];

		if (FindResurrectableFeatureAndResurrect(pos, radius, c.options, (c.options & META_KEY))) {
			inCommand = false;
			SlowUpdate();
			return;
		}
		if (!(c.options & ALT_KEY)) {
			FinishCommand();
		}
	} else {
		// wrong number of parameters
		RemoveUnitFromResurrecters(owner);
		FinishCommand();
	}
}


void CBuilderCAI::ExecutePatrol(Command& c)
{
	if (!owner->unitDef->canPatrol)
		return;

	if (c.params.size() < 3) {
		return;
	}

	Command temp(CMD_FIGHT, c.options|INTERNAL_ORDER, c.GetPos(0));

	commandQue.push_back(c);
	commandQue.pop_front();
	commandQue.push_front(temp);
	Command tmpC(CMD_PATROL);
	eoh->CommandFinished(*owner, tmpC);
	SlowUpdate();
}


void CBuilderCAI::ExecuteFight(Command& c)
{
	assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);

	if (tempOrder) {
		tempOrder = false;
		inCommand = true;
	}
	if (c.params.size() < 3) { // this shouldnt happen but anyway ...
		LOG_L(L_ERROR,
				"Received a Fight command with less than 3 params on %s in BuilderCAI",
				owner->unitDef->humanName.c_str());
		return;
	}

	if (c.params.size() >= 6) {
		if (!inCommand) {
			commandPos1 = c.GetPos(3);
		}
	} else {
		// Some hackery to make sure the line (commandPos1,commandPos2) is NOT
		// rotated (only shortened) if we reach this because the previous return
		// fight command finished by the 'if((curPos-pos).SqLength2D()<(64*64)){'
		// condition, but is actually updated correctly if you click somewhere
		// outside the area close to the line (for a new command).
		commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		if (f3SqDist(owner->pos, commandPos1) > (96 * 96)) {
			commandPos1 = owner->pos;
		}
	}

	float3 pos = c.GetPos(0);
	if (!inCommand) {
		inCommand = true;
		commandPos2 = pos;
	}

	float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	if (c.params.size() >= 6) {
		pos = curPosOnLine;
	}

	if (pos != goalPos) {
		SetGoal(pos, owner->pos);
	}

	const bool resurrectMode = !!(c.options & ALT_KEY);
	const bool reclaimEnemyMode = !!(c.options & META_KEY);
	const bool reclaimEnemyOnlyMode = (c.options & CONTROL_KEY) && (c.options & META_KEY);

	ReclaimOption recopt;
	if (resurrectMode)        recopt |= REC_NONREZ;
	if (reclaimEnemyMode)     recopt |= REC_ENEMY;
	if (reclaimEnemyOnlyMode) recopt |= REC_ENEMYONLY;

	const float searchRadius = (owner->immobile ? 0 : (300 * owner->moveState)) + owner_builder->buildDistance;

	if (!reclaimEnemyOnlyMode && (owner->unitDef->canRepair || owner->unitDef->canAssist) && // Priority 1: Repair
	    FindRepairTargetAndRepair(curPosOnLine, searchRadius, c.options, true, resurrectMode)){
		tempOrder = true;
		inCommand = false;
		if (lastPC1 != gs->frameNum) {  //avoid infinite loops
			lastPC1 = gs->frameNum;
			SlowUpdate();
		}
		return;
	}
	if (!reclaimEnemyOnlyMode && resurrectMode && owner->unitDef->canResurrect && // Priority 2: Resurrect (optional)
	    FindResurrectableFeatureAndResurrect(curPosOnLine, searchRadius, c.options, false)) {
		tempOrder = true;
		inCommand = false;
		if (lastPC2 != gs->frameNum) {  //avoid infinite loops
			lastPC2 = gs->frameNum;
			SlowUpdate();
		}
		return;
	}
	if (owner->unitDef->canReclaim && // Priority 3: Reclaim / reclaim non resurrectable (optional) / reclaim enemy units (optional)
	    FindReclaimTargetAndReclaim(curPosOnLine, searchRadius, c.options, recopt)) {
		tempOrder = true;
		inCommand = false;
		if (lastPC3 != gs->frameNum) {  //avoid infinite loops
			lastPC3 = gs->frameNum;
			SlowUpdate();
		}
		return;
	}
	if (f3SqDist(owner->pos, pos) < (64*64)) {
		FinishCommand();
		return;
	}

	if (owner->haveTarget && owner->moveType->progressState != AMoveType::Done) {
		StopMove();
	} else if (owner->moveType->progressState != AMoveType::Active) {
		owner->moveType->StartMoving(goalPos, 8);
	}
}


void CBuilderCAI::ExecuteRestore(Command& c)
{
	if (!owner->unitDef->canRestore)
		return;

	if (inCommand) {
		if (!owner_builder->terraforming) {
			FinishCommand();
		}
	} else if (owner->unitDef->canRestore) {
		const float3 pos(c.params[0], ground->GetHeightReal(c.params[0], c.params[2]), c.params[2]);
		const float radius = std::min(c.params[3], 200.0f);

		if (MoveInBuildRange(pos, radius * 0.7f)) {
			owner_builder->StartRestore(pos, radius);
			inCommand = true;
		}
	}
}


int CBuilderCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed) {
		if (!teamHandler->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack && (owner->maxRange > 0)) {
				return CMD_ATTACK;
			} else if (owner->unitDef->canReclaim && pointed->unitDef->reclaimable) {
				return CMD_RECLAIM;
			}
		} else {
			const CTransportCAI* tran = dynamic_cast<CTransportCAI*>(pointed->commandAI);

			const bool canAssistPointed = owner_builder->CanAssistUnit(pointed);
			const bool canRepairPointed = owner_builder->CanRepairUnit(pointed);

			if (canAssistPointed) {
				return CMD_REPAIR;
			} else if (canRepairPointed) {
				return CMD_REPAIR;
			} else if (tran && tran->CanTransport(owner)) {
				return CMD_LOAD_ONTO;
			} else if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	if (feature) {
		if (owner->unitDef->canResurrect && feature->udef != NULL) {
			return CMD_RESURRECT;
		} else if(owner->unitDef->canReclaim && feature->def->reclaimable) {
			return CMD_RECLAIM;
		}
	}
	return CMD_MOVE;
}


void CBuilderCAI::AddUnitToReclaimers(CUnit* unit)
{
	reclaimers.insert(unit);
}


void CBuilderCAI::RemoveUnitFromReclaimers(CUnit* unit)
{
	reclaimers.erase(unit);
}


void CBuilderCAI::AddUnitToFeatureReclaimers(CUnit* unit)
{
	featureReclaimers.insert(unit);
}

void CBuilderCAI::RemoveUnitFromFeatureReclaimers(CUnit* unit)
{
	featureReclaimers.erase(unit);
}

void CBuilderCAI::AddUnitToResurrecters(CUnit* unit)
{
	resurrecters.insert(unit);
}

void CBuilderCAI::RemoveUnitFromResurrecters(CUnit* unit)
{
	resurrecters.erase(unit);
}


/**
 * Checks if a unit is being reclaimed by a friendly con.
 *
 * We assume that there will not be a lot of reclaimers, because performance
 * would suck if there were. Ideally, reclaimers should be assigned on a
 * per-unit basis, but this requires tracking of deaths, which albeit
 * already done, is not exactly simple to follow.
 *
 * TODO easy: store reclaiming units per allyteam
 * TODO harder: update reclaimers as they start/finish reclaims and/or die
 */
bool CBuilderCAI::IsUnitBeingReclaimed(const CUnit* unit, CUnit *friendUnit)
{
	bool retval = false;
	std::list<CUnit*> rm;

	for (CUnitSet::iterator it = reclaimers.begin(); it != reclaimers.end(); ++it) {
		if ((*it)->commandAI->commandQue.empty()) {
			rm.push_back(*it);
			continue;
		}
		const Command& c = (*it)->commandAI->commandQue.front();
		if (c.GetID() != CMD_RECLAIM || (c.params.size() != 1 && c.params.size() != 5)) {
			rm.push_back(*it);
			continue;
		}
		const int cmdUnitId = (int)c.params[0];
		if (cmdUnitId == unit->id && (!friendUnit || teamHandler->Ally(friendUnit->allyteam, (*it)->allyteam))) {
			retval = true;
			break;
		}
	}
	for (std::list<CUnit*>::iterator it = rm.begin(); it != rm.end(); ++it)
		RemoveUnitFromReclaimers(*it);
	return retval;
}


bool CBuilderCAI::IsFeatureBeingReclaimed(int featureId, CUnit *friendUnit)
{
	bool retval = false;
	std::list<CUnit*> rm;

	for (CUnitSet::iterator it = featureReclaimers.begin(); it != featureReclaimers.end(); ++it) {
		if ((*it)->commandAI->commandQue.empty()) {
			rm.push_back(*it);
			continue;
		}
		const Command& c = (*it)->commandAI->commandQue.front();
		if (c.GetID() != CMD_RECLAIM || (c.params.size() != 1 && c.params.size() != 5)) {
			rm.push_back(*it);
			continue;
		}
		const int cmdFeatureId = (int)c.params[0];
		if (cmdFeatureId-unitHandler->MaxUnits() == featureId && (!friendUnit || teamHandler->Ally(friendUnit->allyteam, (*it)->allyteam))) {
			retval = true;
			break;
		}
	}
	for (std::list<CUnit*>::iterator it = rm.begin(); it != rm.end(); ++it)
		RemoveUnitFromFeatureReclaimers(*it);
	return retval;
}


bool CBuilderCAI::IsFeatureBeingResurrected(int featureId, CUnit *friendUnit)
{
	bool retval = false;
	std::list<CUnit*> rm;

	for (CUnitSet::iterator it = resurrecters.begin(); it != resurrecters.end(); ++it) {
		if ((*it)->commandAI->commandQue.empty()) {
			rm.push_back(*it);
			continue;
		}
		const Command& c = (*it)->commandAI->commandQue.front();
		if (c.GetID() != CMD_RESURRECT || c.params.size() != 1) {
			rm.push_back(*it);
			continue;
		}
		const int cmdFeatureId = (int)c.params[0];
		if (cmdFeatureId-unitHandler->MaxUnits() == featureId && (!friendUnit || teamHandler->Ally(friendUnit->allyteam, (*it)->allyteam))) {
			retval = true;
			break;
		}
	}
	for (std::list<CUnit*>::iterator it = rm.begin(); it != rm.end(); ++it)
		RemoveUnitFromResurrecters(*it);
	return retval;
}


bool CBuilderCAI::ReclaimObject(CSolidObject* object) {
	if (MoveInBuildRange(object)) {
		owner_builder->SetReclaimTarget(object);
	} else {
		if (owner->moveType->progressState == AMoveType::Failed) {
			return false;
		}
	}

	return true;
}


int CBuilderCAI::FindReclaimTarget(const float3& pos, float radius, unsigned char cmdopt, ReclaimOption recoptions, float bestStartDist) const
{
	const bool noResCheck   = recoptions & REC_NORESCHECK;
	const bool recUnits     = recoptions & REC_UNITS;
	const bool recNonRez    = recoptions & REC_NONREZ;
	const bool recEnemy     = recoptions & REC_ENEMY;
	const bool recEnemyOnly = recoptions & REC_ENEMYONLY;
	const bool recSpecial   = recoptions & REC_SPECIAL;

	const CSolidObject* best = NULL;
	float bestDist = bestStartDist;
	bool stationary = false;
	int rid = -1;

	if (recUnits || recEnemy || recEnemyOnly) {
		const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
		for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
			const CUnit* u = *ui;

			if (u == owner)
				continue;
			if (!u->unitDef->reclaimable)
				continue;
			if (!((!recEnemy && !recEnemyOnly) || !teamHandler->Ally(owner->allyteam, u->allyteam)))
				continue;
			if (!(u->losStatus[owner->allyteam] & (LOS_INRADAR|LOS_INLOS)))
				continue;

			// reclaim stationary targets first
			if (u->isMoving && stationary)
				continue;

			// do not reclaim friendly builders that are busy
			if (u->unitDef->builder && teamHandler->Ally(owner->allyteam, u->allyteam) && !u->commandAI->commandQue.empty())
				continue;

			const float dist = f3SqDist(u->pos, owner->pos);
			if (dist < bestDist || (!stationary && !u->isMoving)) {
				if (owner->immobile && !IsInBuildRange(u)) {
					continue;
				}
				if(!stationary && !u->isMoving) {
					stationary = true;
				}
				bestDist = dist;
				best = u;
			}
		}
		if (best)
			rid = best->id;
	}

	if ((!best || !stationary) && !recEnemyOnly) {
		best = NULL;
		const CTeam* team = teamHandler->Team(owner->team);
		const std::vector<CFeature*>& features = quadField->GetFeaturesExact(pos, radius);
		bool metal = false;
		for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
			const CFeature* f = *fi;
			if (f->def->reclaimable && (recSpecial || f->def->autoreclaim) &&
				(!recNonRez || !(f->def->destructable && f->udef != NULL))
			) {
				if (recSpecial && metal && f->def->metal <= 0.0) {
					continue;
				}
				const float dist = f3SqDist(f->pos, owner->pos);
				if ((dist < bestDist || (recSpecial && !metal && f->def->metal > 0.0)) &&
					(noResCheck ||
					((f->def->metal  > 0.0f) && (team->metal  < team->metalStorage)) ||
					((f->def->energy > 0.0f) && (team->energy < team->energyStorage)))
				) {
					if (!f->IsInLosForAllyTeam(owner->allyteam)) {
						continue;
					}
					if (!owner->unitDef->canmove && !IsInBuildRange(f)) {
						continue;
					}
					if (!(cmdopt & CONTROL_KEY) && IsFeatureBeingResurrected(f->id, owner)) {
						continue;
					}
					if (recSpecial && !metal && f->def->metal > 0.0f) {
						metal = true;
					}
					bestDist = dist;
					best = f;
				}
			}
		}
		if (best)
			rid = unitHandler->MaxUnits() + best->id;
	}

	return rid;
}


/******************************************************************************/
//
//  Area searches
//

bool CBuilderCAI::FindReclaimTargetAndReclaim(const float3& pos, float radius, unsigned char cmdopt, ReclaimOption recoptions)
{
	const int rid = FindReclaimTarget(pos, radius, cmdopt, recoptions);

	if (rid >= 0) {
		if (!(recoptions & REC_NORESCHECK)) {
			// FIGHT commands always resource check
			PushOrUpdateReturnFight();
		}

		Command cmd(CMD_RECLAIM, cmdopt | INTERNAL_ORDER, rid, pos);
			cmd.PushParam(radius);
		commandQue.push_front(cmd);
		return true;
	}

	return false;
}


bool CBuilderCAI::FindResurrectableFeatureAndResurrect(const float3& pos,
                                                       float radius,
                                                       unsigned char options,
													   bool freshOnly)
{
	const std::vector<CFeature*> &features = quadField->GetFeaturesExact(pos, radius);

	const CFeature* best = NULL;
	float bestDist = 1.0e30f;

	for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
		const CFeature* f = *fi;
		if (f->def->destructable && f->udef != NULL) {
			if (!f->IsInLosForAllyTeam(owner->allyteam)) {
				continue;
			}
			if (freshOnly && f->reclaimLeft < 1.0f && f->resurrectProgress <= 0.0f) {
				continue;
			}
			float dist = f3SqDist(f->pos, owner->pos);
			if (dist < bestDist) {
				// dont lock-on to units outside of our reach (for immobile builders)
				if (owner->immobile && !IsInBuildRange(f)) {
					continue;
				}
				if(!(options & CONTROL_KEY) && IsFeatureBeingReclaimed(f->id, owner)) {
					continue;
				}
				bestDist = dist;
				best = f;
			}
		}
	}

	if (best) {
		Command c2(CMD_RESURRECT, options | INTERNAL_ORDER, unitHandler->MaxUnits() + best->id);
		commandQue.push_front(c2);
		return true;
	}

	return false;
}


bool CBuilderCAI::FindCaptureTargetAndCapture(const float3& pos, float radius,
                                              unsigned char options,
											  bool healthyOnly)
{
	const std::vector<CUnit*> &cu = quadField->GetUnits(pos, radius);
	std::vector<CUnit*>::const_iterator ui;

	const CUnit* best = NULL;
	float bestDist = 1.0e30f;
	bool stationary = false;

	for (ui = cu.begin(); ui != cu.end(); ++ui) {
		CUnit* unit = *ui;

		if ((((options & CONTROL_KEY) && owner->team != unit->team) ||
			!teamHandler->Ally(owner->allyteam, unit->allyteam)) && (unit != owner) &&
			(unit->losStatus[owner->allyteam] & (LOS_INRADAR|LOS_INLOS)) &&
			!unit->beingBuilt && unit->unitDef->capturable) {
			if(unit->isMoving && stationary) { // capture stationary targets first
				continue;
			}
			if(healthyOnly && unit->health < unit->maxHealth && unit->captureProgress <= 0.0f) {
				continue;
			}
			const float dist = f3SqDist(unit->pos, owner->pos);
			if (dist < bestDist || (!stationary && !unit->isMoving)) {
				if (!owner->unitDef->canmove && !IsInBuildRange(unit)) {
					continue;
				}
				if(!stationary && !unit->isMoving) {
					stationary = true;
				}
				bestDist = dist;
				best = unit;
			}
		}
	}

	if (best) {
		Command nc(CMD_CAPTURE, options | INTERNAL_ORDER, best->id);
		commandQue.push_front(nc);
		return true;
	}

	return false;
}


bool CBuilderCAI::FindRepairTargetAndRepair(const float3& pos, float radius,
                                            unsigned char options,
                                            bool attackEnemy,
											bool builtOnly)
{
	const std::vector<CUnit*>& cu = quadField->GetUnitsExact(pos, radius);
	const CUnit* bestUnit = NULL;

	const float maxSpeed = owner->moveType->GetMaxSpeed();
	float unitSpeed = 0.0f;
	float bestDist = 1.0e30f;

	bool haveEnemy = false;
	bool trySelfRepair = false;
	bool stationary = false;

	for (std::vector<CUnit*>::const_iterator ui = cu.begin(); ui != cu.end(); ++ui) {
		const CUnit* unit = *ui;

		if (teamHandler->Ally(owner->allyteam, unit->allyteam)) {
			if (!haveEnemy && (unit->health < unit->maxHealth)) {
				// don't help allies build unless set on roam
				if (unit->beingBuilt && owner->team != unit->team && (owner->moveState != MOVESTATE_ROAM)) {
					continue;
				}
				// don't help factories produce units when set on hold pos
				if (unit->beingBuilt && unit->moveDef != NULL && (owner->moveState == MOVESTATE_HOLDPOS)) {
					continue;
				}
				// don't assist or repair if can't assist or repair
				if (!owner_builder->CanAssistUnit(unit) && !owner_builder->CanRepairUnit(unit)) {
					continue;
				}
				if (unit == owner) {
					trySelfRepair = true;
					continue;
				}
				// repair stationary targets first
				if (unit->isMoving && stationary) {
					continue;
				}
				if (builtOnly && unit->beingBuilt) {
					continue;
				}

				float dist = f3SqDist(unit->pos, owner->pos);

				// avoid targets that are faster than our max speed
				if (unit->isMoving) {
					unitSpeed = unit->speed.Length2D();
					dist *= (1.0f + std::max(unitSpeed - maxSpeed, 0.0f));
				}
				if (dist < bestDist || (!stationary && !unit->isMoving)) {
					// dont lock-on to units outside of our reach (for immobile builders)
					if ((owner->immobile || (unit->isMoving && !TargetInterceptable(unit, unitSpeed))) && !IsInBuildRange(unit)) {
						continue;
					}
					// don't repair stuff that's being reclaimed
					if (!(options & CONTROL_KEY) && IsUnitBeingReclaimed(unit, owner)) {
						continue;
					}
					if (!stationary && !unit->isMoving) {
						stationary = true;
					}

					bestDist = dist;
					bestUnit = unit;
				}
			}
		} else {
			if (unit->IsNeutral())
				continue;

			if (!attackEnemy || !owner->unitDef->canAttack || (owner->maxRange <= 0) )
				continue;

			if (!(unit->losStatus[owner->allyteam] & (LOS_INRADAR | LOS_INLOS)))
				continue;

			const float dist = f3SqDist(unit->pos, owner->pos);

			if ((dist < bestDist) || !haveEnemy) {
				if (owner->immobile && ((dist - unit->radius) > owner->maxRange))
					continue;

				bestUnit = unit;
				bestDist = dist;
				haveEnemy = true;
			}
		}
	}

	if (bestUnit == NULL) {
		if (trySelfRepair &&
		    owner->unitDef->canSelfRepair &&
		    (owner->health < owner->maxHealth)) {
			bestUnit = owner;
		} else {
			return false;
		}
	}

	if (!haveEnemy) {
		if (attackEnemy) {
			PushOrUpdateReturnFight();
		}
		Command cmd(CMD_REPAIR, options | INTERNAL_ORDER, bestUnit->id, pos);
			cmd.PushParam(radius);
		commandQue.push_front(cmd);
	} else {
		PushOrUpdateReturnFight(); // attackEnemy must be true
		Command cmd(CMD_ATTACK, options | INTERNAL_ORDER, bestUnit->id);
		commandQue.push_front(cmd);
	}

	return true;
}



void CBuilderCAI::BuggerOff(const float3& pos, float radius) {
	if (owner->unitDef->IsStaticBuilderUnit())
		return;

	CMobileCAI::BuggerOff(pos, radius);
}

