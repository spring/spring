/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "BuilderCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Map.h"


CR_BIND_DERIVED(CBuilderCAI ,CMobileCAI , )

CR_REG_METADATA(CBuilderCAI , (
	CR_MEMBER(ownerBuilder),
	CR_MEMBER(building),
	CR_MEMBER(range3D),
	CR_IGNORED(build),
	CR_IGNORED(buildOptions),

	CR_MEMBER(cachedRadiusId),
	CR_MEMBER(cachedRadius),

	CR_MEMBER(buildRetries),
	CR_MEMBER(randomCounter),

	CR_MEMBER(lastPC1),
	CR_MEMBER(lastPC2),
	CR_MEMBER(lastPC3),
	CR_POSTLOAD(PostLoad)
))

// not adding to members, should repopulate itself
spring::unordered_set<int> CBuilderCAI::reclaimers;
spring::unordered_set<int> CBuilderCAI::featureReclaimers;
spring::unordered_set<int> CBuilderCAI::resurrecters;

std::vector<int> CBuilderCAI::removees;


static std::string GetUnitDefBuildOptionToolTip(const UnitDef* ud, bool disabled) {
	std::string tooltip;

	if (disabled) {
		tooltip = "\xff\xff\x22\x22" "DISABLED: " "\xff\xff\xff\xff";
	} else {
		tooltip = "Build: ";
	}

	tooltip += (ud->humanName + " - " + ud->tooltip);
	tooltip += ("\nHealth "      + FloatToString(ud->health,    "%.0f"));
	tooltip += ("\nMetal cost "  + FloatToString(ud->metal,     "%.0f"));
	tooltip += ("\nEnergy cost " + FloatToString(ud->energy,    "%.0f"));
	tooltip += ("\nBuild time "  + FloatToString(ud->buildTime, "%.0f"));

	return tooltip;
}


CBuilderCAI::CBuilderCAI():
	CMobileCAI(),
	ownerBuilder(nullptr),
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
	ownerBuilder = static_cast<CBuilder*>(owner);

	if (owner->unitDef->canRepair) {
		SCommandDescription c;

		c.id   = CMD_REPAIR;
		c.type = CMDTYPE_ICON_UNIT_OR_AREA;

		c.action    = "repair";
		c.name      = "Repair";
		c.tooltip   = c.name + ": Repairs another unit";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}
	else if (owner->unitDef->canAssist) {
		SCommandDescription c;

		c.id   = CMD_REPAIR;
		c.type = CMDTYPE_ICON_UNIT_OR_AREA;

		c.action    = "assist";
		c.name      = "Assist";
		c.tooltip   = c.name + ": Help build something";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canReclaim) {
		SCommandDescription c;

		c.id   = CMD_RECLAIM;
		c.type = CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;

		c.action    = "reclaim";
		c.name      = "Reclaim";
		c.tooltip   = c.name + ": Sucks in the metal/energy content of a unit/feature\nand adds it to your storage";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canRestore && !mapDamage->Disabled()) {
		SCommandDescription c;

		c.id   = CMD_RESTORE;
		c.type = CMDTYPE_ICON_AREA;

		c.action    = "restore";
		c.name      = "Restore";
		c.tooltip   = c.name + ": Restores an area of the map to its original height";
		c.mouseicon = c.name;

		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canResurrect) {
		SCommandDescription c;

		c.id   = CMD_RESURRECT;
		c.type = CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;

		c.action    = "resurrect";
		c.name      = "Resurrect";
		c.tooltip   = c.name + ": Resurrects a unit from a feature";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}
	if (owner->unitDef->canCapture) {
		SCommandDescription c;

		c.id   = CMD_CAPTURE;
		c.type = CMDTYPE_ICON_UNIT_OR_AREA;

		c.action    = "capture";
		c.name      = "Capture";
		c.tooltip   = c.name + ": Captures a unit from the enemy";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	for (const auto& bi: ownerBuilder->unitDef->buildOptions) {
		const std::string& name = bi.second;
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(name);

		if (ud == nullptr) {
			string errmsg = "MOD ERROR: loading ";
			errmsg += name.c_str();
			errmsg += " for ";
			errmsg += owner->unitDef->name;
			throw content_error(errmsg);
		}

		{
			SCommandDescription c;

			c.id   = -ud->id; //build options are always negative
			c.type = CMDTYPE_ICON_BUILDING;

			c.action    = "buildunit_" + StringToLower(ud->name);
			c.name      = name;
			c.mouseicon = c.name;
			c.tooltip   = GetUnitDefBuildOptionToolTip(ud, c.disabled = (ud->maxThisUnit <= 0));

			buildOptions.insert(c.id);
			possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
		}
	}

	unitHandler.AddBuilderCAI(this);
}

CBuilderCAI::~CBuilderCAI()
{
	RemoveUnitFromReclaimers(owner);
	RemoveUnitFromFeatureReclaimers(owner);
	RemoveUnitFromResurrecters(owner);
	unitHandler.RemoveBuilderCAI(this);
}

void CBuilderCAI::InitStatic()
{
	spring::clear_unordered_set(reclaimers);
	spring::clear_unordered_set(featureReclaimers);
	spring::clear_unordered_set(resurrecters);
}

void CBuilderCAI::PostLoad()
{
	for (const SCommandDescription* cd: possibleCommands) {
		if (cd->id < 0)
			buildOptions.insert(cd->id);
	}
	if (commandQue.empty())
		return;

	ownerBuilder = static_cast<CBuilder*>(owner);

	const Command& c = commandQue.front();

	if (buildOptions.find(c.GetID()) != buildOptions.end()) {
		build.Parse(c);
		build.pos = CGameHelper::Pos2BuildPos(build, true);
	}
}



inline float CBuilderCAI::GetBuildRange(const float targetRadius) const
{
	// for immobile:
	// only use `buildDistance + radius` iff radius > buildDistance,
	// and so it would be impossible to get in buildrange (collision detection with units/features)
	//
	// what does this even mean?? IMMOBILE units cannot "get in range" of anything
	if (owner->immobile)
		return (ownerBuilder->buildDistance + std::max(targetRadius - ownerBuilder->buildDistance, 0.0f));

	return (ownerBuilder->buildDistance + targetRadius);
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
		const bool haveFailed = (owner->moveType->progressState == AMoveType::Failed && f3SqDist(owner->moveType->goalPos, objPos) > 1.0f);

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
		StopMoveAndKeepPointing(objPos, GetBuildRange(objRadius) * 0.9f, false);
	} else {
		StopMoveAndKeepPointing(owner->moveType->goalPos, GetBuildRange(objRadius) * 0.9f, false);
	}

	return true;
}


bool CBuilderCAI::IsBuildPosBlocked(const BuildInfo& bi, const CUnit** nanoFrame) const
{
	CFeature* feature = nullptr;
	CGameHelper::BuildSquareStatus status = CGameHelper::TestUnitBuildSquare(bi, feature, owner->allyteam, true);

	// buildjob is a feature and it is finished already
	if (feature != nullptr && bi.def->isFeature && bi.def->wreckName == feature->def->name)
		return true;

	// open area, reclaimable feature or movable unit
	if (status != CGameHelper::BUILDSQUARE_BLOCKED)
		return false;

	// status is blocked, check if there is a foreign object in the way
	const CSolidObject* s = groundBlockingObjectMap.GroundBlocked(bi.pos);

	// just ourselves, does not count
	if (s == owner)
		return false;

	const CUnit* u = dynamic_cast<const CUnit*>(s);

	// if a *unit* object is not present, then either
	// there is a feature or the terrain is unsuitable
	// (in the former case feature must be reclaimable)
	if (u == nullptr)
		return (feature == nullptr || !feature->def->reclaimable);

	// figure out if object is soft- or hard-blocking
	if (u->beingBuilt) {
		// we can't or don't want assist finishing the nanoframe
		// if a mobile unit blocks the position, wait until it is
		// finished & moved
		if (!ownerBuilder->CanAssistUnit(u, bi.def))
			return u->immobile;

		// unfinished nanoframe, assist it
		if (nanoFrame != nullptr && teamHandler.Ally(owner->allyteam, u->allyteam))
			*nanoFrame = u;

		return false;
	}

	// unit blocks the pos, can it move away?
	return (u->immobile);
}



inline bool CBuilderCAI::OutOfImmobileRange(const Command& cmd) const
{
	// builder can move
	if (owner->unitDef->canmove)
		return false;

	// not an internal object-targeted command
	if (!cmd.IsInternalOrder() || (cmd.GetNumParams() != 1))
		return false;

	const int objID = cmd.GetParam(0);
	const CWorldObject* obj = unitHandler.GetUnit(objID);

	if (obj == nullptr) {
		// features don't move, but maybe the unit was transported?
		obj = featureHandler.GetFeature(objID - unitHandler.MaxUnits());

		if (obj == nullptr)
			return false;
	}

	switch (cmd.GetID()) {
		case CMD_REPAIR:
		case CMD_RECLAIM:
		case CMD_RESURRECT:
		case CMD_CAPTURE: {
			if (!IsInBuildRange(obj))
				return true;

			break;
		}
	}

	return false;
}


float CBuilderCAI::GetBuildOptionRadius(const UnitDef* ud, int cmdId)
{
	float radius = cachedRadius;

	if (cachedRadiusId != cmdId) {
		radius = ud->GetModelRadius();
		cachedRadius = radius;
		cachedRadiusId = cmdId;
	}

	return radius;
}


void CBuilderCAI::CancelRestrictedUnit()
{
	if (owner->team == gu->myTeam) {
		LOG_L(L_WARNING, "%s: Build failed, unit type limit reached", owner->unitDef->humanName.c_str());
		eventHandler.LastMessagePosition(owner->pos);
	}
	StopMoveAndFinishCommand();
}


void CBuilderCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced))
		return;

	// don't guard yourself
	if ((c.GetID() == CMD_GUARD) &&
	    (c.GetNumParams() == 1) && ((int)c.GetParam(0) == owner->id)) {
		return;
	}

	// stop building/reclaiming/... if the new command is not queued, i.e. replaces our current activity
	// FIXME should happen just before CMobileCAI::GiveCommandReal? (the new cmd can still be skipped!)
	if ((c.GetID() != CMD_WAIT) && !(c.GetOpts() & SHIFT_KEY)) {
		if (nonQueingCommands.find(c.GetID()) == nonQueingCommands.end()) {
			building = false;
			static_cast<CBuilder*>(owner)->StopBuild();
		}
	}

	if (buildOptions.find(c.GetID()) != buildOptions.end()) {
		if (c.GetNumParams() < 3)
			return;

		BuildInfo bi;
		bi.pos = c.GetPos(0);

		if (c.GetNumParams() == 4)
			bi.buildFacing = abs((int)c.GetParam(3)) % NUM_FACINGS;

		bi.def = unitDefHandler->GetUnitDefByID(-c.GetID());
		bi.pos = CGameHelper::Pos2BuildPos(bi, true);

		// We are a static building, check if the buildcmd is in range
		if (!owner->unitDef->canmove) {
			if (!IsInBuildRange(bi.pos, GetBuildOptionRadius(bi.def, c.GetID())))
				return;
		}

		const CUnit* nanoFrame = nullptr;

		// check if the buildpos is blocked
		if (IsBuildPosBlocked(bi, &nanoFrame))
			return;

		// if it is a nanoframe help to finish it
		if (nanoFrame != nullptr) {
			Command c2(CMD_REPAIR, c.GetOpts() | INTERNAL_ORDER, nanoFrame->id);
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

	if (owner->beingBuilt || owner->IsStunned())
		return;

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
		StopMoveAndFinishCommand();
	} else {
		commandQue.push_front(Command(CMD_RECLAIM, 0, f->id + unitHandler.MaxUnits()));
		// this assumes that the reclaim command can never return directly
		// without having reclaimed the target
		SlowUpdate();
	}
}


void CBuilderCAI::FinishCommand()
{
	buildRetries = 0;
	CMobileCAI::FinishCommand();
}


void CBuilderCAI::ExecuteStop(Command& c)
{
	building = false;
	ownerBuilder->StopBuild();
	CMobileCAI::ExecuteStop(c);
}


void CBuilderCAI::ExecuteBuildCmd(Command& c)
{
	if (buildOptions.find(c.GetID()) == buildOptions.end())
		return;

	if (!inCommand) {
		BuildInfo bi;

		// note:
		//   need at least 3 parameters or BuildInfo will fail to parse
		//   this usually indicates a malformed command inserted by Lua
		//   (most common with patrolling pseudo-factory hubs)
		if (!bi.Parse(c)) {
			StopMoveAndFinishCommand();
			return;
		}

		#if 1
		// snap build-position to multiples of SQUARE_SIZE plus a half-square offset (4, 12, 20, ...)
		bi.pos.x = math::floor(c.GetParam(0) / SQUARE_SIZE + 0.5f) * SQUARE_SIZE;
		bi.pos.z = math::floor(c.GetParam(2) / SQUARE_SIZE + 0.5f) * SQUARE_SIZE;
		#endif

		CFeature* f = nullptr;
		CGameHelper::TestUnitBuildSquare(bi, f, owner->allyteam, true);

		if (f != nullptr) {
			if (!bi.def->isFeature || bi.def->wreckName != f->def->name) {
				ReclaimFeature(f);
			} else {
				StopMoveAndFinishCommand();
			}
			return;
		}

		// <build> is never parsed (except in PostLoad) so just copy it
		build = bi;
		inCommand = true;
	}

	assert(build.def != nullptr);
	assert(build.def->id == -c.GetID() && build.def->id != 0);

	if (building) {
		// keep moving until 3D distance to buildPos is LEQ our buildDistance
		MoveInBuildRange(build.pos, 0.0f);

		if (ownerBuilder->curBuild == nullptr && !ownerBuilder->terraforming) {
			building = false;
			StopMoveAndFinishCommand();
		}

		return;
	}

	// keep moving until 3D distance to buildPos is LEQ our buildDistance
	if (MoveInBuildRange(build.pos = CGameHelper::Pos2BuildPos(build, true), 0.0f, true)) {
		if (IsBuildPosBlocked(build)) {
			StopMoveAndFinishCommand();
			return;
		}

		if (!eventHandler.AllowUnitCreation(build.def, owner, &build)) {
			StopMoveAndFinishCommand();
			return;
		}

		if (teamHandler.Team(owner->team)->AtUnitLimit())
			return;

		CFeature* f = nullptr;

		bool inWaitStance = false;
		bool limitReached = false;

		if (ownerBuilder->StartBuild(build, f, inWaitStance, limitReached) || (++buildRetries > 30)) {
			building = true;
			return;
		}

		// we can't reliably check if the unit-limit has been reached until
		// the builder has reached the construction site, which is somewhat
		// annoying (since greyed-out icons can still be clicked, etc)
		if (limitReached) {
			CancelRestrictedUnit();
			StopMove();
			return;
		}

		if (f != nullptr && (!build.def->isFeature || build.def->wreckName != f->def->name)) {
			inCommand = false;
			ReclaimFeature(f);
			return;
		}

		if (!inWaitStance) {
			const float buildeeRadius = GetBuildOptionRadius(build.def, c.GetID());
			const float fpSqRadius = (build.def->xsize * build.def->xsize + build.def->zsize * build.def->zsize);
			const float fpRadius = (math::sqrt(fpSqRadius) * 0.5f) * SQUARE_SIZE;

			// tell everything within the radius of the soon-to-be buildee
			// to get out of the way; using the model radius is not correct
			// because this can be shorter than half the footprint diagonal
			// exclude owner iff it is outside the buildee footprint
			if (!build.FootPrintOverlap(owner->pos, owner->GetFootPrint(SQUARE_SIZE * 0.5f)))
				CGameHelper::BuggerOff(build.pos, std::max(buildeeRadius, fpRadius), false, true, owner->team, owner);
			else
				CGameHelper::BuggerOff(build.pos, std::max(buildeeRadius, fpRadius), false, true, owner->team, nullptr);

			NonMoving();
			return;
		}

		return;
	}

	if (owner->moveType->progressState == AMoveType::Failed) {
		if (++buildRetries > 5) {
			StopMoveAndFinishCommand();
			return;
		}
	}

	// we are on the way to the buildpos, meanwhile it can happen
	// that another builder already finished our buildcmd or blocked
	// the buildpos with another building (skip our buildcmd then)
	if ((++randomCounter % 5) == 0) {
		if (IsBuildPosBlocked(build)) {
			StopMoveAndFinishCommand();
			return;
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

	if (c.GetNumParams() == 1 || c.GetNumParams() == 5) {
		// repair unit
		CUnit* unit = unitHandler.GetUnit(c.GetParam(0));

		if (unit == nullptr) {
			StopMoveAndFinishCommand();
			return;
		}

		if (tempOrder && owner->moveState <= MOVESTATE_MANEUVER) {
			// limit how far away we go when not roaming
			if (LinePointDist(commandPos1, commandPos2, unit->pos) > std::max(500.0f, GetBuildRange(unit->radius))) {
				StopMoveAndFinishCommand();
				return;
			}
		}

		if (c.GetNumParams() == 5) {
			const float3& pos = c.GetPos(1);
			const float radius = c.GetParam(4) + 100.0f; // do not walk too far outside repair area

			if ((pos - unit->pos).SqLength2D() > radius * radius ||
				(unit->IsMoving() && ((c.IsInternalOrder() && !TargetInterceptable(unit, unit->speed.Length2D())) || ownerBuilder->curBuild == unit)
				&& !IsInBuildRange(unit))) {
				StopMoveAndFinishCommand();
				return;
			}
		}

		// do not consider units under construction irreparable
		// even if they can be repaired
		bool canRepairUnit = true;
		canRepairUnit &= ((unit->beingBuilt) || (unit->unitDef->repairable && (unit->health < unit->maxHealth)));
		canRepairUnit &= ((unit != owner) || owner->unitDef->canSelfRepair);
		canRepairUnit &= (!unit->soloBuilder || (unit->soloBuilder == owner));
		canRepairUnit &= (!c.IsInternalOrder() || (c.GetOpts() & CONTROL_KEY) || !IsUnitBeingReclaimed(unit, owner));
		canRepairUnit &= (UpdateTargetLostTimer(unit->id) != 0);

		if (canRepairUnit) {
			if (MoveInBuildRange(unit))
				ownerBuilder->SetRepairTarget(unit);
		} else {
			StopMoveAndFinishCommand();
		}
	} else if (c.GetNumParams() == 4) {
		// area repair
		const float3 pos = c.GetPos(0);
		const float radius = c.GetParam(3);

		ownerBuilder->StopBuild();
		if (FindRepairTargetAndRepair(pos, radius, c.GetOpts(), false, (c.GetOpts() & META_KEY))) {
			inCommand = false;
			SlowUpdate();
			return;
		}

		if (!(c.GetOpts() & ALT_KEY))
			StopMoveAndFinishCommand();

	} else {
		StopMoveAndFinishCommand();
	}
}


void CBuilderCAI::ExecuteCapture(Command& c)
{
	// not all builders are capture-capable by default
	if (!owner->unitDef->canCapture)
		return;

	if (c.GetNumParams() == 1 || c.GetNumParams() == 5) {
		// capture unit
		CUnit* unit = unitHandler.GetUnit(c.GetParam(0));

		if (unit == nullptr) {
			StopMoveAndFinishCommand();
			return;
		}

		if (c.GetNumParams() == 5) {
			const float3& pos = c.GetPos(1);
			const float radius = c.GetParam(4) + 100.0f; // do not walk too far outside capture area

			if (((pos - unit->pos).SqLength2D() > (radius * radius) ||
				(ownerBuilder->curCapture == unit && unit->IsMoving() && !IsInBuildRange(unit)))) {
				StopMoveAndFinishCommand();
				return;
			}
		}

		if (unit->unitDef->capturable && unit->team != owner->team && UpdateTargetLostTimer(unit->id)) {
			if (MoveInBuildRange(unit)) {
				ownerBuilder->SetCaptureTarget(unit);
			}
		} else {
			StopMoveAndFinishCommand();
		}
	} else if (c.GetNumParams() == 4) {
		// area capture
		const float3 pos = c.GetPos(0);
		const float radius = c.GetParam(3);

		ownerBuilder->StopBuild();

		if (FindCaptureTargetAndCapture(pos, radius, c.GetOpts(), (c.GetOpts() & META_KEY))) {
			inCommand = false;
			SlowUpdate();
			return;
		}

		if (!(c.GetOpts() & ALT_KEY))
			StopMoveAndFinishCommand();

	} else {
		StopMoveAndFinishCommand();
	}
}


void CBuilderCAI::ExecuteGuard(Command& c)
{
	if (!owner->unitDef->canGuard)
		return;

	CUnit* guardee = unitHandler.GetUnit(c.GetParam(0));

	if (guardee == nullptr) {
		StopMoveAndFinishCommand();
		return;
	}
	if (guardee == owner) {
		StopMoveAndFinishCommand();
		return;
	}
	if (UpdateTargetLostTimer(guardee->id) == 0) {
		StopMoveAndFinishCommand();
		return;
	}
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) {
		StopMoveAndFinishCommand();
		return;
	}


	if (CBuilder* b = dynamic_cast<CBuilder*>(guardee)) {
		if (b->terraforming) {
			if (MoveInBuildRange(b->terraformCenter, b->terraformRadius * 0.7f)) {
				ownerBuilder->HelpTerraform(b);
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
			ownerBuilder->StopBuild();
		}

		const bool pushRepairCommand =
			(  b->curBuild != nullptr) &&
			(  b->curBuild->soloBuilder == nullptr || b->curBuild->soloBuilder == owner) &&
			(( b->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			( !b->curBuild->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			Command nc(CMD_REPAIR, c.GetOpts(), b->curBuild->id);

			commandQue.push_front(nc);
			inCommand = false;
			SlowUpdate();
			return;
		}
	}

	if (CFactory* fac = dynamic_cast<CFactory*>(guardee)) {
		const bool pushRepairCommand =
			(  fac->curBuild != nullptr) &&
			(  fac->curBuild->soloBuilder == nullptr || fac->curBuild->soloBuilder == owner) &&
			(( fac->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			 (!fac->curBuild->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			commandQue.push_front(Command(CMD_REPAIR, c.GetOpts(), fac->curBuild->id));
			inCommand = false;
			// SlowUpdate();
			return;
		}
	}

	if (!(c.GetOpts() & CONTROL_KEY) && IsUnitBeingReclaimed(guardee, owner))
		return;

	const float3 pos    = guardee->pos;
	const float  radius = (guardee->immobile) ? guardee->radius : guardee->radius * 0.8f; // in case of mobile units reduce radius a bit

	if (MoveInBuildRange(pos, radius)) {
		StartSlowGuard(guardee->moveType->GetMaxSpeed());

		const bool pushRepairCommand =
			(  guardee->health < guardee->maxHealth) &&
			(  guardee->soloBuilder == nullptr || guardee->soloBuilder == owner) &&
			(( guardee->beingBuilt && owner->unitDef->canAssist) ||
			 (!guardee->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			commandQue.push_front(Command(CMD_REPAIR, c.GetOpts(), guardee->id));
			inCommand = false;
			return;
		}

		NonMoving();
	} else {
		StopSlowGuard();
	}
}


void CBuilderCAI::ExecuteReclaim(Command& c)
{
	// not all builders are reclaim-capable by default
	if (!owner->unitDef->canReclaim)
		return;

	if (c.GetNumParams() == 1 || c.GetNumParams() == 5) {
		const int signedId = (int) c.GetParam(0);

		if (signedId < 0) {
			LOG_L(L_WARNING, "Trying to reclaim unit or feature with id < 0 (%i), aborting.", signedId);
			return;
		}

		const unsigned int uid = signedId;

		const bool checkForBetterTarget = ((++randomCounter % 5) == 0);
		if (checkForBetterTarget && c.IsInternalOrder() && (c.GetNumParams() >= 5)) {
			// regular check if there is a closer reclaim target
			CSolidObject* obj;

			if (uid >= unitHandler.MaxUnits()) {
				obj = featureHandler.GetFeature(uid - unitHandler.MaxUnits());
			} else {
				obj = unitHandler.GetUnit(uid);
			}

			if (obj) {
				const float3& pos = c.GetPos(1);
				const float radius = c.GetParam(4);
				const float curdist = pos.SqDistance2D(obj->pos);

				const bool recUnits = !!(c.GetOpts() & META_KEY);
				const bool recEnemyOnly = (c.GetOpts() & META_KEY) && (c.GetOpts() & CONTROL_KEY);
				const bool recSpecial = !!(c.GetOpts() & CONTROL_KEY);

				ReclaimOption recopt = REC_NORESCHECK;
				if (recUnits)     recopt |= REC_UNITS;
				if (recEnemyOnly) recopt |= REC_ENEMYONLY;
				if (recSpecial)   recopt |= REC_SPECIAL;

				const int rid = FindReclaimTarget(pos, radius, c.GetOpts(), recopt, curdist);
				if ((rid > 0) && (rid != uid)) {
					StopMoveAndFinishCommand();
					RemoveUnitFromReclaimers(owner);
					RemoveUnitFromFeatureReclaimers(owner);
					return;
				}
			}
		}

		if (uid >= unitHandler.MaxUnits()) { // reclaim feature
			CFeature* feature = featureHandler.GetFeature(uid - unitHandler.MaxUnits());

			if (feature != nullptr) {
				bool featureBeingResurrected = IsFeatureBeingResurrected(feature->id, owner);
				featureBeingResurrected &= (c.IsInternalOrder()) && !(c.GetOpts() & CONTROL_KEY);

				if (featureBeingResurrected || !ReclaimObject(feature)) {
					StopMoveAndFinishCommand();
					RemoveUnitFromFeatureReclaimers(owner);
				} else {
					AddUnitToFeatureReclaimers(owner);
				}
			} else {
				StopMoveAndFinishCommand();
				RemoveUnitFromFeatureReclaimers(owner);
			}

			RemoveUnitFromReclaimers(owner);
		} else { // reclaim unit
			CUnit* unit = unitHandler.GetUnit(uid);

			if (unit != nullptr && c.GetNumParams() == 5) {
				const float3& pos = c.GetPos(1);
				const float radius = c.GetParam(4) + 100.0f; // do not walk too far outside reclaim area

				const bool outOfReclaimRange =
					(pos.SqDistance2D(unit->pos) > radius * radius) ||
					(ownerBuilder->curReclaim == unit && unit->IsMoving() && !IsInBuildRange(unit));
				const bool busyAlliedBuilder =
					unit->unitDef->builder &&
					!unit->commandAI->commandQue.empty() &&
					teamHandler.Ally(owner->allyteam, unit->allyteam);

				if (outOfReclaimRange || busyAlliedBuilder) {
					StopMoveAndFinishCommand();
					RemoveUnitFromReclaimers(owner);
					RemoveUnitFromFeatureReclaimers(owner);
					return;
				}
			}

			if (unit != nullptr && unit != owner && unit->unitDef->reclaimable && UpdateTargetLostTimer(unit->id) && unit->AllowedReclaim(owner)) {
				if (!ReclaimObject(unit)) {
					StopMoveAndFinishCommand();
				} else {
					AddUnitToReclaimers(owner);
				}
			} else {
				RemoveUnitFromReclaimers(owner);
				StopMoveAndFinishCommand();
			}

			RemoveUnitFromFeatureReclaimers(owner);
		}
	} else if (c.GetNumParams() == 4) {
		// area reclaim
		const float3 pos = c.GetPos(0);
		const float radius = c.GetParam(3);
		const bool recUnits = !!(c.GetOpts() & META_KEY);
		const bool recEnemyOnly = (c.GetOpts() & META_KEY) && (c.GetOpts() & CONTROL_KEY);
		const bool recSpecial = !!(c.GetOpts() & CONTROL_KEY);

		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		ownerBuilder->StopBuild();

		ReclaimOption recopt = REC_NORESCHECK;
		if (recUnits)     recopt |= REC_UNITS;
		if (recEnemyOnly) recopt |= REC_ENEMYONLY;
		if (recSpecial)   recopt |= REC_SPECIAL;

		if (FindReclaimTargetAndReclaim(pos, radius, c.GetOpts(), recopt)) {
			inCommand = false;
			SlowUpdate();
			return;
		}

		if (!(c.GetOpts() & ALT_KEY))
			StopMoveAndFinishCommand();

	} else {
		// wrong number of parameters
		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		StopMoveAndFinishCommand();
	}
}


bool CBuilderCAI::ResurrectObject(CFeature *feature) {
	if (MoveInBuildRange(feature, true)) {
		ownerBuilder->SetResurrectTarget(feature);
		return true;
	}

	return (owner->moveType->progressState != AMoveType::Failed);
}


void CBuilderCAI::ExecuteResurrect(Command& c)
{
	// not all builders are resurrect-capable by default
	if (!owner->unitDef->canResurrect)
		return;

	if (c.GetNumParams() == 1) {
		const unsigned int id = (unsigned int) c.GetParam(0);

		if (id >= unitHandler.MaxUnits()) { // resurrect feature
			CFeature* feature = featureHandler.GetFeature(id - unitHandler.MaxUnits());

			if (feature && feature->udef != nullptr) {
				if ((c.IsInternalOrder() && !(c.GetOpts() & CONTROL_KEY) && IsFeatureBeingReclaimed(feature->id, owner)) ||
					!ResurrectObject(feature)) {
					RemoveUnitFromResurrecters(owner);
					StopMoveAndFinishCommand();
				}
				else {
					AddUnitToResurrecters(owner);
				}
			} else {
				RemoveUnitFromResurrecters(owner);

				if (ownerBuilder->lastResurrected && unitHandler.GetUnitUnsafe(ownerBuilder->lastResurrected) != nullptr && owner->unitDef->canRepair) {
					// resurrection finished, start repair (by overwriting the current order)
					c = Command(CMD_REPAIR, c.GetOpts() | INTERNAL_ORDER, ownerBuilder->lastResurrected);

					ownerBuilder->lastResurrected = 0;
					inCommand = false;
					SlowUpdate();
					return;
				}

				StopMoveAndFinishCommand();
			}
		} else { // resurrect unit
			RemoveUnitFromResurrecters(owner);
			StopMoveAndFinishCommand();
		}
	} else if (c.GetNumParams() == 4) {
		// area resurrect
		const float3 pos = c.GetPos(0);
		const float radius = c.GetParam(3);

		if (FindResurrectableFeatureAndResurrect(pos, radius, c.GetOpts(), (c.GetOpts() & META_KEY))) {
			inCommand = false;
			SlowUpdate();
			return;
		}

		if (!(c.GetOpts() & ALT_KEY))
			StopMoveAndFinishCommand();

	} else {
		// wrong number of parameters
		RemoveUnitFromResurrecters(owner);
		StopMoveAndFinishCommand();
	}
}


void CBuilderCAI::ExecutePatrol(Command& c)
{
	if (!owner->unitDef->canPatrol)
		return;

	if (c.GetNumParams() < 3)
		return;

	Command temp(CMD_FIGHT, c.GetOpts() | INTERNAL_ORDER, c.GetPos(0));

	commandQue.push_back(c);
	commandQue.pop_front();
	commandQue.push_front(temp);
	Command tmpC(CMD_PATROL);
	eoh->CommandFinished(*owner, tmpC);
	SlowUpdate();
}


void CBuilderCAI::ExecuteFight(Command& c)
{
	assert(c.IsInternalOrder() || owner->unitDef->canFight);

	if (tempOrder) {
		tempOrder = false;
		inCommand = true;
	}
	if (c.GetNumParams() < 3) {
		LOG_L(L_ERROR, "[BuilderCAI::%s][f=%d][id=%d][#c.params=%d min=3]", __func__, gs->frameNum, owner->id, c.GetNumParams());
		return;
	}

	if (c.GetNumParams() >= 6) {
		if (!inCommand)
			commandPos1 = c.GetPos(3);

	} else {
		// Some hackery to make sure the line (commandPos1,commandPos2) is NOT
		// rotated (only shortened) if we reach this because the previous return
		// fight command finished by the 'if((curPos-pos).SqLength2D()<(64*64)){'
		// condition, but is actually updated correctly if you click somewhere
		// outside the area close to the line (for a new command).
		if (f3SqDist(owner->pos, commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, owner->pos)) > Square(96.0f))
			commandPos1 = owner->pos;
	}

	float3 pos = c.GetPos(0);
	if (!inCommand) {
		inCommand = true;
		commandPos2 = pos;
	}

	float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);

	if (c.GetNumParams() >= 6)
		pos = curPosOnLine;

	if (pos != owner->moveType->goalPos)
		SetGoal(pos, owner->pos);

	const UnitDef* ownerDef = owner->unitDef;

	const bool resurrectMode = !!(c.GetOpts() & ALT_KEY);
	const bool reclaimEnemyMode = !!(c.GetOpts() & META_KEY);
	const bool reclaimEnemyOnlyMode = (c.GetOpts() & CONTROL_KEY) && (c.GetOpts() & META_KEY);

	ReclaimOption recopt;
	if (resurrectMode       ) recopt |= REC_NONREZ;
	if (reclaimEnemyMode    ) recopt |= REC_ENEMY;
	if (reclaimEnemyOnlyMode) recopt |= REC_ENEMYONLY;

	const float searchRadius = (owner->immobile ? 0.0f : (300.0f * owner->moveState)) + ownerBuilder->buildDistance;

	// Priority 1: Repair
	if (!reclaimEnemyOnlyMode && (ownerDef->canRepair || ownerDef->canAssist) && FindRepairTargetAndRepair(curPosOnLine, searchRadius, c.GetOpts(), true, resurrectMode)){
		tempOrder = true;
		inCommand = false;

		if (lastPC1 != gs->frameNum) {  //avoid infinite loops
			lastPC1 = gs->frameNum;
			SlowUpdate();
		}

		return;
	}

	// Priority 2: Resurrect (optional)
	if (!reclaimEnemyOnlyMode && resurrectMode && ownerDef->canResurrect && FindResurrectableFeatureAndResurrect(curPosOnLine, searchRadius, c.GetOpts(), false)) {
		tempOrder = true;
		inCommand = false;

		if (lastPC2 != gs->frameNum) {  //avoid infinite loops
			lastPC2 = gs->frameNum;
			SlowUpdate();
		}

		return;
	}

	// Priority 3: Reclaim / reclaim non resurrectable (optional) / reclaim enemy units (optional)
	if (ownerDef->canReclaim && FindReclaimTargetAndReclaim(curPosOnLine, searchRadius, c.GetOpts(), recopt)) {
		tempOrder = true;
		inCommand = false;

		if (lastPC3 != gs->frameNum) {  //avoid infinite loops
			lastPC3 = gs->frameNum;
			SlowUpdate();
		}

		return;
	}

	if (f3SqDist(owner->pos, pos) < Square(64.0f)) {
		StopMoveAndFinishCommand();
		return;
	}

	if (owner->HaveTarget() && owner->moveType->progressState != AMoveType::Done) {
		StopMove();
	} else {
		SetGoal(owner->moveType->goalPos, owner->pos);
	}
}


void CBuilderCAI::ExecuteRestore(Command& c)
{
	if (!owner->unitDef->canRestore)
		return;

	if (inCommand) {
		if (!ownerBuilder->terraforming)
			StopMoveAndFinishCommand();

		return;
	}

	if (owner->unitDef->canRestore) {
		const float3 pos(c.GetParam(0), CGround::GetHeightReal(c.GetParam(0), c.GetParam(2)), c.GetParam(2));
		const float radius = std::min(c.GetParam(3), 200.0f);

		if (MoveInBuildRange(pos, radius * 0.7f)) {
			ownerBuilder->StartRestore(pos, radius);
			inCommand = true;
		}
	}
}


int CBuilderCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed != nullptr) {
		if (!teamHandler.Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack && (owner->maxRange > 0.0f))
				return CMD_ATTACK;

			if (owner->unitDef->canReclaim && pointed->unitDef->reclaimable)
				return CMD_RECLAIM;
		} else {
			const bool canAssistPointed = ownerBuilder->CanAssistUnit(pointed);
			const bool canRepairPointed = ownerBuilder->CanRepairUnit(pointed);

			if (canAssistPointed)
				return CMD_REPAIR;
			if (canRepairPointed)
				return CMD_REPAIR;

			if (pointed->CanTransport(owner))
				return CMD_LOAD_ONTO;
			if (owner->unitDef->canGuard)
				return CMD_GUARD;
		}
	}

	if (feature != nullptr) {
		if (owner->unitDef->canResurrect && feature->udef != nullptr)
			return CMD_RESURRECT;

		if (owner->unitDef->canReclaim && feature->def->reclaimable)
			return CMD_RECLAIM;
	}

	return CMD_MOVE;
}


void CBuilderCAI::AddUnitToReclaimers(CUnit* unit) { reclaimers.insert(unit->id); }
void CBuilderCAI::RemoveUnitFromReclaimers(CUnit* unit) { reclaimers.erase(unit->id); }

void CBuilderCAI::AddUnitToFeatureReclaimers(CUnit* unit) { featureReclaimers.insert(unit->id); }
void CBuilderCAI::RemoveUnitFromFeatureReclaimers(CUnit* unit) { featureReclaimers.erase(unit->id); }

void CBuilderCAI::AddUnitToResurrecters(CUnit* unit) { resurrecters.insert(unit->id); }
void CBuilderCAI::RemoveUnitFromResurrecters(CUnit* unit) { resurrecters.erase(unit->id); }


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
bool CBuilderCAI::IsUnitBeingReclaimed(const CUnit* unit, const CUnit* friendUnit)
{
	bool retval = false;

	removees.clear();
	removees.reserve(reclaimers.size());

	for (auto it = reclaimers.begin(); it != reclaimers.end(); ++it) {
		const CUnit* u = unitHandler.GetUnit(*it);
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		if (cq.empty()) {
			removees.push_back(u->id);
			continue;
		}
		const Command& c = cq.front();
		if (c.GetID() != CMD_RECLAIM || (c.GetNumParams() != 1 && c.GetNumParams() != 5)) {
			removees.push_back(u->id);
			continue;
		}
		const int cmdUnitId = (int)c.GetParam(0);
		if (cmdUnitId == unit->id && (friendUnit == nullptr || teamHandler.Ally(friendUnit->allyteam, u->allyteam))) {
			retval = true;
			break;
		}
	}

	for (auto it = removees.begin(); it != removees.end(); ++it)
		RemoveUnitFromReclaimers(unitHandler.GetUnit(*it));

	return retval;
}


bool CBuilderCAI::IsFeatureBeingReclaimed(int featureId, const CUnit* friendUnit)
{
	bool retval = false;

	removees.clear();
	removees.reserve(featureReclaimers.size());

	for (auto it = featureReclaimers.begin(); it != featureReclaimers.end(); ++it) {
		const CUnit* u = unitHandler.GetUnit(*it);
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		if (cq.empty()) {
			removees.push_back(u->id);
			continue;
		}
		const Command& c = cq.front();
		if (c.GetID() != CMD_RECLAIM || (c.GetNumParams() != 1 && c.GetNumParams() != 5)) {
			removees.push_back(u->id);
			continue;
		}
		const int cmdFeatureId = (int)c.GetParam(0);
		if ((cmdFeatureId - unitHandler.MaxUnits()) == featureId && (friendUnit == nullptr || teamHandler.Ally(friendUnit->allyteam, u->allyteam))) {
			retval = true;
			break;
		}
	}

	for (auto it = removees.begin(); it != removees.end(); ++it)
		RemoveUnitFromFeatureReclaimers(unitHandler.GetUnit(*it));

	return retval;
}


bool CBuilderCAI::IsFeatureBeingResurrected(int featureId, const CUnit* friendUnit)
{
	bool retval = false;

	removees.clear();
	removees.reserve(resurrecters.size());

	for (auto it = resurrecters.begin(); it != resurrecters.end(); ++it) {
		const CUnit* u = unitHandler.GetUnit(*it);
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		if (cq.empty()) {
			removees.push_back(u->id);
			continue;
		}
		const Command& c = cq.front();
		if (c.GetID() != CMD_RESURRECT || c.GetNumParams() != 1) {
			removees.push_back(u->id);
			continue;
		}
		const int cmdFeatureId = (int)c.GetParam(0);
		if ((cmdFeatureId - unitHandler.MaxUnits()) == featureId && (friendUnit == nullptr || teamHandler.Ally(friendUnit->allyteam, u->allyteam))) {
			retval = true;
			break;
		}
	}

	for (auto it = removees.begin(); it != removees.end(); ++it)
		RemoveUnitFromResurrecters(unitHandler.GetUnit(*it));

	return retval;
}


bool CBuilderCAI::ReclaimObject(CSolidObject* object) {
	if (MoveInBuildRange(object)) {
		ownerBuilder->SetReclaimTarget(object);
		return true;
	}

	return (owner->moveType->progressState != AMoveType::Failed);
}


int CBuilderCAI::FindReclaimTarget(const float3& pos, float radius, unsigned char cmdopt, ReclaimOption recoptions, float bestStartDist) const
{
	const bool noResCheck   = recoptions & REC_NORESCHECK;
	const bool recUnits     = recoptions & REC_UNITS;
	const bool recNonRez    = recoptions & REC_NONREZ;
	const bool recEnemy     = recoptions & REC_ENEMY;
	const bool recEnemyOnly = recoptions & REC_ENEMYONLY;
	const bool recSpecial   = recoptions & REC_SPECIAL;

	const CSolidObject* best = nullptr;
	float bestDist = bestStartDist;
	bool stationary = false;
	int rid = -1;

	if (recUnits || recEnemy || recEnemyOnly) {
		QuadFieldQuery qfQuery;
		quadField.GetUnitsExact(qfQuery, pos, radius, false);

		for (const CUnit* u: *qfQuery.units) {
			if (u == owner)
				continue;
			if (!u->unitDef->reclaimable)
				continue;
			if (!((!recEnemy && !recEnemyOnly) || !teamHandler.Ally(owner->allyteam, u->allyteam)))
				continue;
			if (!(u->losStatus[owner->allyteam] & (LOS_INRADAR|LOS_INLOS)))
				continue;

			// reclaim stationary targets first
			if (u->IsMoving() && stationary)
				continue;

			// do not reclaim friendly builders that are busy
			if (u->unitDef->builder && teamHandler.Ally(owner->allyteam, u->allyteam) && !u->commandAI->commandQue.empty())
				continue;

			const float dist = f3SqDist(u->pos, owner->pos);
			if (dist < bestDist || (!stationary && !u->IsMoving())) {
				if (owner->immobile && !IsInBuildRange(u))
					continue;

				if (!stationary && !u->IsMoving())
					stationary = true;

				bestDist = dist;
				best = u;
			}
		}
		if (best != nullptr)
			rid = best->id;
	}

	if ((!best || !stationary) && !recEnemyOnly) {
		best = nullptr;
		const CTeam* team = teamHandler.Team(owner->team);
		QuadFieldQuery qfQuery;
		quadField.GetFeaturesExact(qfQuery, pos, radius, false);
		bool metal = false;

		for (const CFeature* f: *qfQuery.features) {
			if (!f->def->reclaimable)
				continue;
			if (!recSpecial && !f->def->autoreclaim)
				continue;

			if (recNonRez && f->udef != nullptr)
				continue;

			if (recSpecial && metal && f->defResources.metal <= 0.0)
				continue;

			const float dist = f3SqDist(f->pos, owner->pos);

			if ((dist < bestDist || (recSpecial && !metal && f->defResources.metal > 0.0)) &&
				(noResCheck ||
				((f->defResources.metal  > 0.0f) && (team->res.metal  < team->resStorage.metal)) ||
				((f->defResources.energy > 0.0f) && (team->res.energy < team->resStorage.energy)))
			) {
				if (!f->IsInLosForAllyTeam(owner->allyteam))
					continue;

				if (!owner->unitDef->canmove && !IsInBuildRange(f))
					continue;

				if (!(cmdopt & CONTROL_KEY) && IsFeatureBeingResurrected(f->id, owner))
					continue;

				metal |= (recSpecial && !metal && f->defResources.metal > 0.0f);

				bestDist = dist;
				best = f;
			}
		}

		if (best != nullptr)
			rid = unitHandler.MaxUnits() + best->id;
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

	if (rid < 0)
		return false;

	// FIGHT commands always resource check
	if (!(recoptions & REC_NORESCHECK))
		PushOrUpdateReturnFight();

	Command c(CMD_RECLAIM, cmdopt | INTERNAL_ORDER, rid, pos);
	c.PushParam(radius);
	commandQue.push_front(c);
	return true;
}


bool CBuilderCAI::FindResurrectableFeatureAndResurrect(
	const float3& pos,
	float radius,
	unsigned char options,
	bool freshOnly
) {
	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, pos, radius, false);

	const CFeature* best = nullptr;
	float bestDist = 1.0e30f;

	for (const CFeature* f: *qfQuery.features) {
		if (f->udef == nullptr)
			continue;

		if (!f->IsInLosForAllyTeam(owner->allyteam))
			continue;

		if (freshOnly && f->reclaimLeft < 1.0f)
			continue;

		const float dist = f3SqDist(f->pos, owner->pos);
		if (dist < bestDist) {
			// dont lock-on to units outside of our reach (for immobile builders)
			if (owner->immobile && !IsInBuildRange(f))
				continue;

			if (!(options & CONTROL_KEY) && IsFeatureBeingReclaimed(f->id, owner))
				continue;

			bestDist = dist;
			best = f;
		}
	}

	if (best != nullptr) {
		commandQue.push_front(Command(CMD_RESURRECT, options | INTERNAL_ORDER, unitHandler.MaxUnits() + best->id));
		return true;
	}

	return false;
}


bool CBuilderCAI::FindCaptureTargetAndCapture(
	const float3& pos,
	float radius,
	unsigned char options,
	bool healthyOnly
) {
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius, false);

	const CUnit* best = nullptr;
	float bestDist = 1.0e30f;
	bool stationary = false;

	const bool ctrlOpt = (options & CONTROL_KEY);

	for (const CUnit* unit: *qfQuery.units) {
		const bool isAlliedUnit = teamHandler.Ally(owner->allyteam, unit->allyteam);
		const bool isVisibleUnit = (unit->losStatus[owner->allyteam] & (LOS_INRADAR | LOS_INLOS));
		const bool isCapturableUnit = !unit->beingBuilt && unit->unitDef->capturable;

		const bool isEnemyTarget = ((ctrlOpt && owner->team != unit->team) || !isAlliedUnit);
		const bool isValidTarget = ((unit != owner) && isVisibleUnit && isCapturableUnit);

		if (isEnemyTarget && isValidTarget) {
			// capture stationary targets first
			if (unit->IsMoving() && stationary)
				continue;

			if (healthyOnly && unit->health < unit->maxHealth && unit->captureProgress <= 0.0f)
				continue;

			const float dist = f3SqDist(unit->pos, owner->pos);

			if (dist < bestDist || (!stationary && !unit->IsMoving())) {
				if (!owner->unitDef->canmove && !IsInBuildRange(unit))
					continue;

				stationary |= (!stationary && !unit->IsMoving());

				bestDist = dist;
				best = unit;
			}
		}
	}

	if (best != nullptr) {
		commandQue.push_front(Command(CMD_CAPTURE, options | INTERNAL_ORDER, best->id));
		return true;
	}

	return false;
}


bool CBuilderCAI::FindRepairTargetAndRepair(
	const float3& pos,
	float radius,
	unsigned char options,
	bool attackEnemy,
	bool builtOnly
) {
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius, false);
	const CUnit* bestUnit = nullptr;

	const float maxSpeed = owner->moveType->GetMaxSpeed();
	float unitSpeed = 0.0f;
	float bestDist = 1.0e30f;

	bool haveEnemy = false;
	bool trySelfRepair = false;
	bool stationary = false;

	for (const CUnit* unit: *qfQuery.units) {
		if (teamHandler.Ally(owner->allyteam, unit->allyteam)) {
			if (!haveEnemy && (unit->health < unit->maxHealth)) {
				// don't help allies build unless set on roam
				if (unit->beingBuilt && owner->team != unit->team && (owner->moveState != MOVESTATE_ROAM))
					continue;

				// don't help factories produce units when set on hold pos
				if (unit->beingBuilt && unit->moveDef != nullptr && (owner->moveState == MOVESTATE_HOLDPOS))
					continue;

				// don't assist or repair if can't assist or repair
				if (!ownerBuilder->CanAssistUnit(unit) && !ownerBuilder->CanRepairUnit(unit))
					continue;

				if (unit == owner) {
					trySelfRepair = true;
					continue;
				}
				// repair stationary targets first
				if (unit->IsMoving() && stationary)
					continue;

				if (builtOnly && unit->beingBuilt)
					continue;

				float dist = f3SqDist(unit->pos, owner->pos);

				// avoid targets that are faster than our max speed
				if (unit->IsMoving()) {
					unitSpeed = unit->speed.Length2D();
					dist *= (1.0f + std::max(unitSpeed - maxSpeed, 0.0f));
				}
				if (dist < bestDist || (!stationary && !unit->IsMoving())) {
					// dont lock-on to units outside of our reach (for immobile builders)
					if ((owner->immobile || (unit->IsMoving() && !TargetInterceptable(unit, unitSpeed))) && !IsInBuildRange(unit))
						continue;

					// don't repair stuff that's being reclaimed
					if (!(options & CONTROL_KEY) && IsUnitBeingReclaimed(unit, owner))
						continue;

					stationary |= (!stationary && !unit->IsMoving());

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

	if (bestUnit == nullptr) {
		if (!trySelfRepair || !owner->unitDef->canSelfRepair || (owner->health >= owner->maxHealth))
			return false;

		bestUnit = owner;
	}

	if (!haveEnemy) {
		if (attackEnemy)
			PushOrUpdateReturnFight();

		Command c(CMD_REPAIR, options | INTERNAL_ORDER, bestUnit->id, pos);
		c.PushParam(radius);
		commandQue.push_front(c);
	} else {
		PushOrUpdateReturnFight(); // attackEnemy must be true
		commandQue.push_front(Command(CMD_ATTACK, options | INTERNAL_ORDER, bestUnit->id));
	}

	return true;
}



void CBuilderCAI::BuggerOff(const float3& pos, float radius) {
	if (owner->unitDef->IsStaticBuilderUnit())
		return;

	CMobileCAI::BuggerOff(pos, radius);
}

