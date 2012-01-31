/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

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
				CR_MEMBER(building),
				CR_MEMBER(range3D),
//				CR_MEMBER(build),

				CR_MEMBER(cachedRadiusId),
				CR_MEMBER(cachedRadius),

				CR_MEMBER(buildRetries),

				CR_MEMBER(lastPC1),
				CR_MEMBER(lastPC2),
				CR_MEMBER(lastPC3),
				CR_RESERVED(16),
				CR_POSTLOAD(PostLoad)
				));

// not adding to members, should repopulate itself
CUnitSet CBuilderCAI::reclaimers;
CUnitSet CBuilderCAI::featureReclaimers;
CUnitSet CBuilderCAI::resurrecters;


CBuilderCAI::CBuilderCAI():
	CMobileCAI(),
	building(false),
	cachedRadiusId(0),
	cachedRadius(0),
	buildRetries(0),
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
	lastPC1(-1),
	lastPC2(-1),
	lastPC3(-1),
	range3D(owner->unitDef->buildRange3D)
{
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

	CBuilder* builder = (CBuilder*) owner;

	map<int, string>::const_iterator bi;
	for (bi = builder->unitDef->buildOptions.begin(); bi != builder->unitDef->buildOptions.end(); ++bi) {
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
		        ud->health, ud->metalCost, ud->energyCost, ud->buildTime);
		if (c.disabled) {
			c.tooltip = "\xff\xff\x22\x22" "DISABLED: " "\xff\xff\xff\xff";
		} else {
			c.tooltip = "Build: ";
		}
		c.tooltip += ud->humanName + " - " + ud->tooltip + tmp;

		possibleCommands.push_back(c);
		buildOptions[c.id] = name;
	}
	uh->AddBuilderCAI(this);
}

CBuilderCAI::~CBuilderCAI()
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (uh) {
		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		RemoveUnitFromResurrecters(owner);
		uh->RemoveBuilderCAI(this);
	}
}

void CBuilderCAI::PostLoad()
{
	if (!commandQue.empty()) {
		Command& c = commandQue.front();
//		float3 curPos = owner->pos;

		map<int, string>::iterator boi = buildOptions.find(c.GetID());
		if (boi != buildOptions.end()) {
			build.Parse(c);
			build.pos = helper->Pos2BuildPos(build, true);
		}
	}
}


inline bool CBuilderCAI::ObjInBuildRange(const CWorldObject* obj) const
{
	const CBuilder* builder = (CBuilder*)owner;
	const float immDistSqr = f3SqLen(owner->pos - obj->pos);
	const float buildDist = builder->buildDistance + obj->radius - 9.0f;
	return (immDistSqr < (buildDist * buildDist));
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
	const CWorldObject* obj = uh->GetUnit(objID);

	if (obj == NULL) {
		// features don't move, but maybe the unit was transported?
		obj = featureHandler->GetFeature(objID - uh->MaxUnits());

		if (obj == NULL) {
			return false;
		}
	}

	switch (cmd.GetID()) {
		case CMD_REPAIR:
		case CMD_RECLAIM:
		case CMD_RESURRECT:
		case CMD_CAPTURE: {
			if (!ObjInBuildRange(obj)) {
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

	if ((c.GetID() == CMD_GUARD) &&
	    (c.params.size() == 1) && ((int)c.params[0] == owner->id)) {
		return;
	}

	if (!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.GetID()) == nonQueingCommands.end()
			&& c.GetID() != CMD_WAIT) {
		building = false;
		((CBuilder*) owner)->StopBuild();
	}

	map<int,string>::iterator boi = buildOptions.find(c.GetID());
	if (boi != buildOptions.end()) {
		if (c.params.size() < 3) {
			return;
		}

		BuildInfo bi;
		bi.pos = float3(c.params[0], c.params[1], c.params[2]);

		if (c.params.size() == 4)
			bi.buildFacing = int(abs(c.params[3])) % NUM_FACINGS;

		bi.def = unitDefHandler->GetUnitDefByName(boi->second);
		bi.pos = helper->Pos2BuildPos(bi, true);

		if (!owner->unitDef->canmove) {
			const CBuilder* builder = (CBuilder*)owner;
			const float dist = f3Len(builder->pos - bi.pos);
			const float radius = GetBuildOptionRadius(bi.def, c.GetID());
			if (dist > (builder->buildDistance + radius - 8.0f)) {
				return;
			}
		}
		CFeature* feature = NULL;
		if (!uh->TestUnitBuildSquare(bi, feature, owner->allyteam, true)) {
			if (!feature && owner->unitDef->canAssist) {
				int yardxpos=int(bi.pos.x+4)/SQUARE_SIZE;
				int yardypos=int(bi.pos.z+4)/SQUARE_SIZE;
				CSolidObject* s;
				CUnit* u;
				if((s=groundBlockingObjectMap->GroundBlocked(yardypos*gs->mapx+yardxpos)) &&
				   (u=dynamic_cast<CUnit*>(s)) &&
				   u->beingBuilt && (u->buildProgress == 0.0f) &&
				   (!u->soloBuilder || (u->soloBuilder == owner)))
				{
					Command c2(CMD_REPAIR, c.options | INTERNAL_ORDER);
					c2.params.push_back(u->id);
					CMobileCAI::GiveCommandReal(c2);
					CMobileCAI::GiveCommandReal(c);
				}
			}
			return;
		}
	}
	CMobileCAI::GiveCommandReal(c);
}


void CBuilderCAI::SlowUpdate()
{
	if(gs->paused) // Commands issued may invoke SlowUpdate when paused
		return;
	if (commandQue.empty()) {
		CMobileCAI::SlowUpdate();
		return;
	}

	if (owner->stunned) {
		return;
	}

	CBuilder* builder = (CBuilder*) owner;
	Command& c = commandQue.front();

	if (OutOfImmobileRange(c)) {
		FinishCommand();
		return;
	}

	map<int, string>::iterator boi = buildOptions.find(c.GetID());
	if (!owner->beingBuilt && boi != buildOptions.end()) {
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(boi->second);
		const float radius = GetBuildOptionRadius(ud, c.GetID());

		if (inCommand) {
			if (building) {
				if (f3SqDist(build.pos, builder->pos) > Square(builder->buildDistance + radius - 8.0f)) {
					owner->moveType->StartMoving(build.pos, builder->buildDistance * 0.5f + radius);
				} else {
					StopMove();
					// needed since above startmoving cancels this
					owner->moveType->KeepPointingTo(build.pos, (builder->buildDistance + radius) * 0.6f, false);
				}
				if (!builder->curBuild && !builder->terraforming) {
					building = false;
					StopMove();				//cancel the effect of KeepPointingTo
					FinishCommand();
				}
				// This can only be true if two builders started building
				// the restricted unit in the same simulation frame
				else if (uh->unitsByDefs[owner->team][build.def->id].size() > build.def->maxThisUnit) {
					// unit restricted
					building = false;
					builder->StopBuild();
					CancelRestrictedUnit(boi->second);
				}
			} else {
				build.Parse(c);

				if (uh->unitsByDefs[owner->team][build.def->id].size() >= build.def->maxThisUnit) {
					// unit restricted, don't bother moving all the way
					// to the construction site first before telling us
					// (since greyed-out icons can still be clicked etc,
					// would be better to prevent that but doesn't cover
					// case where limit reached while builder en-route)
					CancelRestrictedUnit(boi->second);
					StopMove();
				} else {
					build.pos = helper->Pos2BuildPos(build, true);
					const float sqdist = f3SqDist(build.pos, builder->pos);

					if ((sqdist < Square(builder->buildDistance * 0.6f + radius)) ||
						(!owner->unitDef->canmove && (sqdist <= Square(builder->buildDistance + radius - 8.0f)))) {
						StopMove();

						if (luaRules && !luaRules->AllowUnitCreation(build.def, owner, &build)) {
							FinishCommand();
						}
						else if (!teamHandler->Team(owner->team)->AtUnitLimit()) {
							// unit-limit not yet reached
							CFeature* f = NULL;
							buildRetries++;
							owner->moveType->KeepPointingTo(build.pos, builder->buildDistance * 0.7f + radius, false);

							bool waitstance = false;
							if (builder->StartBuild(build, f, waitstance) || (buildRetries > 30)) {
								building = true;
							}
							else if (f) {
								inCommand = false;
								ReclaimFeature(f);
							}
							else if (!waitstance) {
								const float fpSqRadius = (ud->xsize * ud->xsize + ud->zsize * ud->zsize);
								const float fpRadius = (math::sqrt(fpSqRadius) * 0.5f) * SQUARE_SIZE;

								// tell everything within the radius of the soon-to-be buildee
								// to get out of the way; using the model radius is not correct
								// because this can be shorter than half the footprint diagonal
								helper->BuggerOff(build.pos, std::max(radius, fpRadius), false, true, owner->team, NULL);
								NonMoving();
							}
						}
					} else {
						if (owner->moveType->progressState == AMoveType::Failed) {
							if (++buildRetries > 5) {
								StopMove();
								FinishCommand();
							}
						}
						SetGoal(build.pos, owner->pos, builder->buildDistance * 0.4f + radius);
					}
				}
			}
		} else {		//!inCommand
			BuildInfo bi;
			bi.pos.x = floor(c.params[0] / SQUARE_SIZE + 0.5f) * SQUARE_SIZE;
			bi.pos.z = floor(c.params[2] / SQUARE_SIZE + 0.5f) * SQUARE_SIZE;
			bi.pos.y = c.params[1];

			if (c.params.size() == 4)
				bi.buildFacing = int(abs(c.params[3])) % NUM_FACINGS;

			bi.def = unitDefHandler->GetUnitDefByName(boi->second);

			CFeature* f = 0;
			uh->TestUnitBuildSquare(bi, f, owner->allyteam, true);

			if (f) {
				ReclaimFeature(f);
			} else {
				inCommand=true;
				SlowUpdate();
			}
		}
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
			CMobileCAI::SlowUpdate();
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
		Command c2(CMD_RECLAIM);
		c2.params.push_back(f->id + uh->MaxUnits());
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
	CBuilder* builder = (CBuilder*) owner;
	building = false;
	builder->StopBuild();
	CMobileCAI::ExecuteStop(c);
}


void CBuilderCAI::ExecuteRepair(Command& c)
{
	// not all builders are repair-capable by default
	if (!owner->unitDef->canRepair)
		return;

	CBuilder* builder = (CBuilder*) owner;

	if (c.params.size() == 1 || c.params.size() == 5) {
		// repair unit
		CUnit* unit = uh->GetUnit(c.params[0]);

		if (unit == NULL) {
			FinishCommand();
			return;
		}

		if (tempOrder && owner->moveState == MOVESTATE_MANEUVER) {
			// limit how far away we go
			if (LinePointDist(commandPos1, commandPos2, unit->pos) > 500) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		if (c.params.size() == 5) {
			const float3 pos(c.params[1], c.params[2], c.params[3]);
			const float radius = c.params[4] + 100.0f; // do not walk too far outside repair area

			if (((pos - unit->pos).SqLength2D() > radius * radius ||
				(builder->curBuild == unit && unit->isMoving && !ObjInBuildRange(unit)))) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		// do not consider units under construction irreparable
		// even if they can be repaired
		if ((unit->beingBuilt || unit->unitDef->repairable)
		    && (unit->health < unit->maxHealth) &&
		    ((unit != owner) || owner->unitDef->canSelfRepair) &&
		    (!unit->soloBuilder || (unit->soloBuilder == owner)) &&
			(!(c.options & INTERNAL_ORDER) || (c.options & CONTROL_KEY) || !IsUnitBeingReclaimed(unit, owner)) &&
		    UpdateTargetLostTimer(unit->id)) {

			if (f3SqDist(unit->pos, builder->pos) < Square(builder->buildDistance + unit->radius - 8.0f)) {
				StopMove();
				builder->SetRepairTarget(unit);
				owner->moveType->KeepPointingTo(unit->pos, builder->buildDistance * 0.9f + unit->radius, false);
			} else {
				if (f3SqDist(goalPos, unit->pos) > 1.0f) {
					SetGoal(unit->pos, owner->pos, builder->buildDistance * 0.9f + unit->radius);
				}
			}
		} else {
			StopMove();
			FinishCommand();
		}
	} else if (c.params.size() == 4) {
		// area repair
		const float3 pos(c.params[0], c.params[1], c.params[2]);
		const float radius = c.params[3];

		builder->StopBuild();
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
	return;
}


void CBuilderCAI::ExecuteCapture(Command& c)
{
	// not all builders are capture-capable by default
	if (!owner->unitDef->canCapture)
		return;

	CBuilder* builder = (CBuilder*) owner;

	if (c.params.size() == 1 || c.params.size() == 5) {
		// capture unit
		CUnit* unit = uh->GetUnit(c.params[0]);

		if (unit == NULL) {
			FinishCommand();
			return;
		}

		if (c.params.size() == 5) {
			const float3 pos(c.params[1], c.params[2], c.params[3]);
			const float radius = c.params[4] + 100; // do not walk too far outside capture area

			if (((pos - unit->pos).SqLength2D() > (radius * radius) ||
				(builder->curCapture == unit && unit->isMoving && !ObjInBuildRange(unit)))) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		if (unit->unitDef->capturable && unit->team != owner->team && UpdateTargetLostTimer(unit->id)) {
			if (f3SqDist(unit->pos, builder->pos) < Square(builder->buildDistance + unit->radius - 8.0f)) {
				StopMove();
				builder->SetCaptureTarget(unit);
				owner->moveType->KeepPointingTo(unit->pos, builder->buildDistance * 0.9f + unit->radius, false);
			} else {
				if (f3SqDist(goalPos, unit->pos) > 1) {
					SetGoal(unit->pos, owner->pos, builder->buildDistance * 0.9f + unit->radius);
				}
			}
		} else {
			StopMove();
			FinishCommand();
		}
	} else if (c.params.size() == 4) {
		// area capture
		const float3 pos(c.params[0], c.params[1], c.params[2]);
		const float radius = c.params[3];

		builder->StopBuild();

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
	return;
}


void CBuilderCAI::ExecuteGuard(Command& c)
{
	if (!owner->unitDef->canGuard)
		return;

	CBuilder* builder = (CBuilder*) owner;
	CUnit* guardee = uh->GetUnit(c.params[0]);

	if (guardee == NULL) { FinishCommand(); return; }
	if (guardee == owner) { FinishCommand(); return; }
	if (UpdateTargetLostTimer(guardee->id) == 0) { FinishCommand(); return; }
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) { FinishCommand(); return; }


	if (CBuilder* b = dynamic_cast<CBuilder*>(guardee)) {
		if (b->terraforming) {
			if (f3SqDist(builder->pos, b->terraformCenter) <
					Square((builder->buildDistance * 0.8f) + (b->terraformRadius * 0.7f))) {
				StopMove();
				owner->moveType->KeepPointingTo(b->terraformCenter, builder->buildDistance * 0.9f, false);
				builder->HelpTerraform(b);
			} else {
				StopSlowGuard();
				SetGoal(b->terraformCenter, builder->pos, builder->buildDistance * 0.7f + b->terraformRadius * 0.6f);
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
			builder->StopBuild();
		}

		const bool pushRepairCommand =
			(  b->curBuild != NULL) &&
			(  b->curBuild->soloBuilder == NULL || b->curBuild->soloBuilder == owner) &&
			(( b->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			( !b->curBuild->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			Command nc(CMD_REPAIR, c.options);
			nc.params.push_back(b->curBuild->id);

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

			Command nc(CMD_REPAIR, c.options);
			nc.params.push_back(fac->curBuild->id);

			commandQue.push_front(nc);
			inCommand = false;
			// SlowUpdate();
			return;
		}
	}

	if (!(c.options & CONTROL_KEY) && IsUnitBeingReclaimed(guardee, owner))
		return;

	const float3 curPos = owner->pos;
	const float3 dif = (guardee->pos - curPos).Normalize();
	const float3 goal = guardee->pos - dif * (builder->buildDistance * 0.5f);

	if (f3SqLen(guardee->pos - curPos) >
			(builder->buildDistance * 1.1f + guardee->radius) *
			(builder->buildDistance * 1.1f + guardee->radius)) {
		StopSlowGuard();
	}
	if (f3SqLen(guardee->pos - curPos) <
			(builder->buildDistance * 0.9f + guardee->radius) *
			(builder->buildDistance * 0.9f + guardee->radius)) {
		StartSlowGuard(guardee->moveType->GetMaxSpeed());
		StopMove();

		owner->moveType->KeepPointingTo(guardee->pos,
			builder->buildDistance * 0.9f + guardee->radius, false);

		const bool pushRepairCommand =
			(  guardee->health < guardee->maxHealth) &&
			(  guardee->soloBuilder == NULL || guardee->soloBuilder == owner) &&
			(( guardee->beingBuilt && owner->unitDef->canAssist) ||
			 (!guardee->beingBuilt && owner->unitDef->canRepair));

		if (pushRepairCommand) {
			StopSlowGuard();

			Command nc(CMD_REPAIR, c.options);
			nc.params.push_back(guardee->id);

			commandQue.push_front(nc);
			inCommand = false;
			return;
		} else {
			NonMoving();
		}
	} else {
		const float da = f3SqLen(goalPos - goal);
		const float db = f3SqLen(goalPos - owner->pos);

		if (da > 4000.0f || db < Square(owner->moveType->GetMaxSpeed() * GAME_SPEED + 1 + SQUARE_SIZE * 2)) {
			SetGoal(goal, curPos);
		}
	}
}


void CBuilderCAI::ExecuteReclaim(Command& c)
{
	CBuilder* builder = (CBuilder*) owner;

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

		if (uid >= uh->MaxUnits()) { // reclaim feature
			CFeature* feature = featureHandler->GetFeature(uid - uh->MaxUnits());

			if (feature != NULL) {
				if (((c.options & INTERNAL_ORDER) && !(c.options & CONTROL_KEY) && IsFeatureBeingResurrected(feature->id, owner)) ||
					!ReclaimObject(feature)) {
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
			CUnit* unit = uh->GetUnitUnsafe(uid);

			if (unit != NULL && c.params.size() == 5) {
				const float3 pos(c.params[1], c.params[2], c.params[3]);
				const float radius = c.params[4] + 100.0f; // do not walk too far outside reclaim area

				const bool outOfReclaimRange =
					((pos - unit->pos).SqLength2D() > radius * radius) ||
					(builder->curReclaim == unit && unit->isMoving && !ObjInBuildRange(unit));
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
		const float3 pos(c.params[0], c.params[1], c.params[2]);
		const float radius = c.params[3];
		const bool recUnits = !!(c.options & META_KEY);
		const bool recEnemyOnly = (c.options & META_KEY) && (c.options & CONTROL_KEY);
		const bool recSpecial = !!(c.options & CONTROL_KEY);

		RemoveUnitFromReclaimers(owner);
		RemoveUnitFromFeatureReclaimers(owner);
		builder->StopBuild();

		if (FindReclaimTargetAndReclaim(pos, radius, c.options, true, recUnits, false, false, recEnemyOnly, recSpecial)) {
			inCommand=false;
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
	CBuilder* builder = (CBuilder*) owner;

	if (f3SqDist(feature->pos, builder->pos) < Square(builder->buildDistance + feature->radius - 1.0f)) {
		StopMove();
		owner->moveType->KeepPointingTo(feature->pos, builder->buildDistance * 0.9f + feature->radius, false);
		builder->SetResurrectTarget(feature);
	} else {
		if (f3SqDist(goalPos, feature->pos) > 1) {
			SetGoal(feature->pos,owner->pos, builder->buildDistance * 0.8f + feature->radius);
		} else {
			if (owner->moveType->progressState == AMoveType::Failed) {
				return false;
			}
		}
	}
	return true;
}


void CBuilderCAI::ExecuteResurrect(Command& c)
{
	// not all builders are resurrect-capable by default
	if (!owner->unitDef->canResurrect)
		return;

	CBuilder* builder = (CBuilder*) owner;

	if (c.params.size() == 1) {
		unsigned int id = (unsigned int) c.params[0];

		if (id >= uh->MaxUnits()) { // resurrect feature
			CFeature* feature = featureHandler->GetFeature(id - uh->MaxUnits());

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

				if (builder->lastResurrected && uh->GetUnitUnsafe(builder->lastResurrected) != NULL && owner->unitDef->canRepair) {
					// resurrection finished, start repair (by overwriting the current order)
					c = Command(CMD_REPAIR, c.options | INTERNAL_ORDER);
					c.AddParam(builder->lastResurrected);

					builder->lastResurrected = 0;
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
		const float3 pos(c.params[0], c.params[1], c.params[2]);
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
	return;
}


void CBuilderCAI::ExecutePatrol(Command& c)
{
	if (!owner->unitDef->canPatrol)
		return;

	if (c.params.size() < 3) {
		return;
	}

	Command temp(CMD_FIGHT, c.options|INTERNAL_ORDER);
	temp.params.push_back(c.params[0]);
	temp.params.push_back(c.params[1]);
	temp.params.push_back(c.params[2]);

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
	CBuilder* builder = (CBuilder*) owner;

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
			commandPos1 = float3(c.params[3], c.params[4], c.params[5]);
		}
	} else {
		// Some hackery to make sure the line (commandPos1,commandPos2) is NOT
		// rotated (only shortened) if we reach this because the previous return
		// fight command finished by the 'if((curPos-pos).SqLength2D()<(64*64)){'
		// condition, but is actually updated correctly if you click somewhere
		// outside the area close to the line (for a new command).
		commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		if (f3SqLen(owner->pos - commandPos1) > (96 * 96)) {
			commandPos1 = owner->pos;
		}
	}

	float3 pos(c.params[0], c.params[1], c.params[2]);

	if (!inCommand) {
		inCommand = true;
		commandPos2 = pos;
	}
	if (c.params.size() >= 6) {
		pos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	}
	if (pos != goalPos) {
		SetGoal(pos, owner->pos);
	}
	const bool resurrectMode = !!(c.options & ALT_KEY);
	const bool reclaimEnemyMode = !!(c.options & META_KEY);
	const bool reclaimEnemyOnlyMode = (c.options & CONTROL_KEY) && (c.options & META_KEY);
	float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	if (!reclaimEnemyOnlyMode && (owner->unitDef->canRepair || owner->unitDef->canAssist) && // Priority 1: Repair
	    FindRepairTargetAndRepair(curPosOnLine, 300*owner->moveState+builder->buildDistance-8, c.options, true, resurrectMode)){
		tempOrder = true;
		inCommand = false;
		if (lastPC1 != gs->frameNum) {  //avoid infinite loops
			lastPC1 = gs->frameNum;
			SlowUpdate();
		}
		return;
	}
	if (!reclaimEnemyOnlyMode && resurrectMode && owner->unitDef->canResurrect && // Priority 2: Resurrect (optional)
	    FindResurrectableFeatureAndResurrect(curPosOnLine, 300, c.options, false)) {
		tempOrder = true;
		inCommand = false;
		if (lastPC2 != gs->frameNum) {  //avoid infinite loops
			lastPC2 = gs->frameNum;
			SlowUpdate();
		}
		return;
	}
	if (owner->unitDef->canReclaim && // Priority 3: Reclaim / reclaim non resurrectable (optional) / reclaim enemy units (optional)
	    FindReclaimTargetAndReclaim(curPosOnLine, 300, c.options, false, false, resurrectMode, reclaimEnemyMode, reclaimEnemyOnlyMode, false)) {
		tempOrder = true;
		inCommand = false;
		if (lastPC3 != gs->frameNum) {  //avoid infinite loops
			lastPC3 = gs->frameNum;
			SlowUpdate();
		}
		return;
	}
	if (f3SqLen(owner->pos - pos) < (64*64)) {
		FinishCommand();
		return;
	}
	if(owner->haveTarget && owner->moveType->progressState!=AMoveType::Done){
		StopMove();
	} else if(owner->moveType->progressState!=AMoveType::Active){
		owner->moveType->StartMoving(goalPos, 8);
	}
	return;
}


void CBuilderCAI::ExecuteRestore(Command& c)
{
	if (!owner->unitDef->canRestore)
		return;

	CBuilder* builder = (CBuilder*) owner;

	if (inCommand) {
		if (!builder->terraforming) {
			FinishCommand();
		}
	} else if (owner->unitDef->canRestore) {
		float3 pos(c.params[0], ground->GetHeightReal(c.params[0], c.params[2]), c.params[2]);
		const float radius = std::min(c.params[3], 200.0f);

		if (f3SqDist(builder->pos, pos) < Square(builder->buildDistance - 1.0f)) {
			StopMove();
			builder->StartRestore(pos, radius);
			owner->moveType->KeepPointingTo(pos, builder->buildDistance * 0.9f, false);
			inCommand = true;
		} else {
			SetGoal(pos, owner->pos, builder->buildDistance * 0.9f);
		}
	}
	return;
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
			CTransportCAI* tran = dynamic_cast<CTransportCAI*>(pointed->commandAI);
			if ((pointed->health < pointed->maxHealth) &&
			    (pointed->unitDef->repairable) &&
			    (!pointed->soloBuilder || (pointed->soloBuilder == owner)) &&
			    (( pointed->beingBuilt && owner->unitDef->canAssist) ||
			     (!pointed->beingBuilt && owner->unitDef->canRepair))) {
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
bool CBuilderCAI::IsUnitBeingReclaimed(CUnit* unit, CUnit *friendUnit)
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
		if (cmdFeatureId-uh->MaxUnits() == featureId && (!friendUnit || teamHandler->Ally(friendUnit->allyteam, (*it)->allyteam))) {
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
		if (cmdFeatureId-uh->MaxUnits() == featureId && (!friendUnit || teamHandler->Ally(friendUnit->allyteam, (*it)->allyteam))) {
			retval = true;
			break;
		}
	}
	for (std::list<CUnit*>::iterator it = rm.begin(); it != rm.end(); ++it)
		RemoveUnitFromResurrecters(*it);
	return retval;
}


bool CBuilderCAI::ReclaimObject(CSolidObject* object) {
	CBuilder* builder = (CBuilder*) owner;

	if (f3SqDist(object->pos, builder->pos) < Square(builder->buildDistance + object->radius - 1.0f)) {
		StopMove();
		owner->moveType->KeepPointingTo(object->pos, builder->buildDistance * 0.9f + object->radius, false);
		builder->SetReclaimTarget(object);
	} else {
		if (f3SqDist(goalPos, object->pos) > 1) {
			SetGoal(object->pos, owner->pos, builder->buildDistance * 0.8f + object->radius);
		} else {
			if (owner->moveType->progressState == AMoveType::Failed) {
				return false;
			}
		}
	}
	return true;
}


/******************************************************************************/
//
//  Area searches
//

bool CBuilderCAI::FindReclaimTargetAndReclaim(const float3& pos,
                                                   float radius,
                                                   unsigned char options,
                                                   bool noResCheck,
                                                   bool recUnits,
                                                   bool recNonRez,
                                                   bool recEnemy,
                                                   bool recEnemyOnly,
                                                   bool recSpecial)
{
	const CSolidObject* best = NULL;
	float bestDist = 1.0e30f;
	bool stationary = false;
	bool metal = false;
	int rid = -1;

	if(recUnits || recEnemy || recEnemyOnly) {
		const std::vector<CUnit*> &units = qf->GetUnitsExact(pos, radius);
		for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
			const CUnit* u = *ui;
			if (u != owner
			    && u->unitDef->reclaimable
			    && ((!recEnemy && !recEnemyOnly) || !teamHandler->Ally(owner->allyteam, u->allyteam))
			    && (u->losStatus[owner->allyteam] & (LOS_INRADAR|LOS_INLOS))
			   ) {
				// do not reclaim friendly builders that are busy
				if(!u->unitDef->builder || u->commandAI->commandQue.empty() || !teamHandler->Ally(owner->allyteam, u->allyteam)) {
					if(u->isMoving && stationary) { // reclaim stationary targets first
						continue;
					}
					const float dist = f3SqLen(u->pos - owner->pos);
					if(dist < bestDist || (!stationary && !u->isMoving)) {
						if (!owner->unitDef->canmove && !ObjInBuildRange(u)) {
							continue;
						}
						if(!stationary && !u->isMoving) {
							stationary = true;
						}
						bestDist = dist;
						best = u;
					}
				}
			}
		}
		if(best)
			rid = best->id;
	}

	if((!best || !stationary) && !recEnemyOnly) {
		const CTeam* team = teamHandler->Team(owner->team);
		best = NULL;
		const std::vector<CFeature*> &features = qf->GetFeaturesExact(pos, radius);
		for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
			const CFeature* f = *fi;
			if (f->def->reclaimable && (recSpecial || f->def->autoreclaim) && 
				(!recNonRez || !(f->def->destructable && f->udef != NULL))) {
				if (recSpecial && metal && f->def->metal <= 0.0) {
					continue;
				}
				float dist = f3SqLen(f->pos - owner->pos);
				if ((dist < bestDist || (recSpecial && !metal && f->def->metal > 0.0)) &&
					(noResCheck ||
					((f->def->metal  > 0.0f) && (team->metal  < team->metalStorage)) ||
					((f->def->energy > 0.0f) && (team->energy < team->energyStorage)))) {
					if (!f->IsInLosForAllyTeam(owner->allyteam)) {
						continue;
					}
					if (!owner->unitDef->canmove && !ObjInBuildRange(f)) {
						continue;
					}
					if(!(options & CONTROL_KEY) && IsFeatureBeingResurrected(f->id, owner)) {
						continue;
					}
					if(recSpecial && !metal && f->def->metal > 0.0f) {
						metal = true;
					}
					bestDist = dist;
					best = f;
				}
			}
		}
		if(best)
			rid = uh->MaxUnits() + best->id;
	}

	if (rid >= 0) {
		if (!noResCheck) {
			// FIGHT commands always resource check
			PushOrUpdateReturnFight();
		}

		Command cmd(CMD_RECLAIM, options | INTERNAL_ORDER);
			cmd.params.push_back(rid);
			cmd.params.push_back(pos.x);
			cmd.params.push_back(pos.y);
			cmd.params.push_back(pos.z);
			cmd.params.push_back(radius);
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
	const std::vector<CFeature*> &features = qf->GetFeaturesExact(pos, radius);

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
			float dist = f3SqLen(f->pos - owner->pos);
			if (dist < bestDist) {
				// dont lock-on to units outside of our reach (for immobile builders)
				if (!owner->unitDef->canmove && !ObjInBuildRange(f)) {
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
		Command c2(CMD_RESURRECT, options | INTERNAL_ORDER);
		c2.params.push_back(uh->MaxUnits() + best->id);
		commandQue.push_front(c2);
		return true;
	}

	return false;
}


bool CBuilderCAI::FindCaptureTargetAndCapture(const float3& pos, float radius,
                                              unsigned char options,
											  bool healthyOnly)
{
	const std::vector<CUnit*> &cu = qf->GetUnits(pos, radius);
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
			float dist = f3SqLen(unit->pos - owner->pos);
			if(dist < bestDist || (!stationary && !unit->isMoving)) {
				if (!owner->unitDef->canmove && !ObjInBuildRange(unit)) {
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
		Command nc(CMD_CAPTURE, options | INTERNAL_ORDER);
		nc.params.push_back(best->id);
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
	const std::vector<CUnit*> &cu = qf->GetUnitsExact(pos, radius);

	const CUnit* best = NULL;
	float bestDist = 1.0e30f;
	bool haveEnemy = false;
	bool trySelfRepair = false;
	bool stationary = false;

	for (std::vector<CUnit*>::const_iterator ui = cu.begin(); ui != cu.end(); ++ui) {
		CUnit* unit = *ui;
		if (teamHandler->Ally(owner->allyteam, unit->allyteam)) {
			if (!haveEnemy && (unit->health < unit->maxHealth)) {
				// don't help allies build unless set on roam
				if (unit->beingBuilt && owner->team != unit->team && (owner->moveState != MOVESTATE_ROAM)) {
					continue;
				}                
				// don't help factories produce units when set on hold pos                
				if (unit->beingBuilt && unit->mobility && (owner->moveState == MOVESTATE_HOLDPOS)) {
					continue;
				}
				// don't repair stuff that can't be repaired
				if (!unit->beingBuilt && !unit->unitDef->repairable) {
					continue;
				}
				// don't assist or repair if can't assist or repair
				if (!( unit->beingBuilt && owner->unitDef->canAssist) &&
						!(!unit->beingBuilt && owner->unitDef->canRepair)) {
					continue;
				}
				if (unit->soloBuilder && (unit->soloBuilder != owner)) {
					continue;
				}
				if (unit == owner) {
					trySelfRepair = true;
					continue;
				}
				if(unit->isMoving && stationary) { // repair stationary targets first
					continue;
				}
				if(builtOnly && unit->beingBuilt) {
					continue;
				}
				float dist = f3SqLen(unit->pos - owner->pos);
				if (dist < bestDist || (!stationary && !unit->isMoving)) {
					// dont lock-on to units outside of our reach (for immobile builders)
					if (!owner->unitDef->canmove && !ObjInBuildRange(unit)) {
						continue;
					}
					// don't repair stuff that's being reclaimed
					if (!(options & CONTROL_KEY) && IsUnitBeingReclaimed(unit, owner)) {
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
		else {
			if (attackEnemy && owner->unitDef->canAttack && (owner->maxRange > 0) &&
				(unit->losStatus[owner->allyteam] & (LOS_INRADAR|LOS_INLOS))) {
				const float dist = f3SqLen(unit->pos - owner->pos);
				if ((dist < bestDist) || !haveEnemy) {
					if (!owner->unitDef->canmove &&
					    ((dist - unit->radius) > owner->maxRange)) {
						continue;
					}
					best = unit;
					bestDist = dist;
					haveEnemy = true;
				}
			}
		}
	}

	if (best == NULL) {
		if (trySelfRepair &&
		    owner->unitDef->canSelfRepair &&
		    (owner->health < owner->maxHealth)) {
			best = owner;
		} else {
			return false;
		}
	}

	if (!haveEnemy) {
		if (attackEnemy) {
			PushOrUpdateReturnFight();
		}
		Command cmd(CMD_REPAIR, options | INTERNAL_ORDER);
			cmd.params.push_back(best->id);
			cmd.params.push_back(pos.x);
			cmd.params.push_back(pos.y);
			cmd.params.push_back(pos.z);
			cmd.params.push_back(radius);
		commandQue.push_front(cmd);
	}
	else {
		PushOrUpdateReturnFight(); // attackEnemy must be true
		Command cmd(CMD_ATTACK, options | INTERNAL_ORDER);
		cmd.params.push_back(best->id);
		commandQue.push_front(cmd);
	}

	return true;
}



void CBuilderCAI::BuggerOff(const float3& pos, float radius) {
	if (owner->unitDef->IsStaticBuilderUnit())
		return;

	CMobileCAI::BuggerOff(pos, radius);
}

