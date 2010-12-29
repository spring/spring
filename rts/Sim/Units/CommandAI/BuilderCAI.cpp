/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BuilderCAI.h"

#include <assert.h>

#include "TransportCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "LineDrawer.h"
#include "Sim/Units/Groups/Group.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/Team.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/3DModel.h"
#include "Lua/LuaRules.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitSet.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "myMath.h"
#include "creg/STL_Map.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Exceptions.h"


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
		float3 curPos = owner->pos;

		map<int, string>::iterator boi = buildOptions.find(c.id);
		if (boi != buildOptions.end()) {
			build.Parse(c);
			build.pos = helper->Pos2BuildPos(build);
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

	switch (cmd.id) {
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


float CBuilderCAI::GetUnitDefRadius(const UnitDef* ud, int cmdId)
{
	float radius;
	if (cachedRadiusId == cmdId) {
		radius = cachedRadius;
	} else {
		radius = (ud->LoadModel())->radius;
		cachedRadius = radius;
		cachedRadiusId = cmdId;
	}
	return radius;
}


void CBuilderCAI::CancelRestrictedUnit(const std::string& buildOption)
{
	if (owner->team == gu->myTeam) {
		logOutput.Print("%s: Build failed, unit type limit reached", owner->unitDef->humanName.c_str());
		logOutput.SetLastMsgPos(owner->pos);
	}
	FinishCommand();
}


void CBuilderCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced))
		return;

	if ((c.id == CMD_GUARD) &&
	    (c.params.size() == 1) && ((int)c.params[0] == owner->id)) {
		return;
	}

	if (!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.id) == nonQueingCommands.end()
			&& c.id != CMD_WAIT) {
		building = false;
		((CBuilder*) owner)->StopBuild();
	}

	map<int,string>::iterator boi = buildOptions.find(c.id);
	if (boi != buildOptions.end()) {
		if (c.params.size() < 3) {
			return;
		}

		BuildInfo bi;
		bi.pos = float3(c.params[0], c.params[1], c.params[2]);

		if (c.params.size() == 4)
			bi.buildFacing = int(abs(c.params[3])) % 4;

		bi.def = unitDefHandler->GetUnitDefByName(boi->second);
		bi.pos = helper->Pos2BuildPos(bi);

		if (!owner->unitDef->canmove) {
			const CBuilder* builder = (CBuilder*)owner;
			const float dist = f3Len(builder->pos - bi.pos);
			const float radius = GetUnitDefRadius(bi.def, c.id);
			if (dist > (builder->buildDistance + radius - 8.0f)) {
				return;
			}
		}
		CFeature* feature;
		if(!uh->TestUnitBuildSquare(bi,feature,owner->allyteam)) {
			if (!feature && owner->unitDef->canAssist) {
				int yardxpos=int(bi.pos.x+4)/SQUARE_SIZE;
				int yardypos=int(bi.pos.z+4)/SQUARE_SIZE;
				CSolidObject* s;
				CUnit* u;
				if((s=groundBlockingObjectMap->GroundBlocked(yardypos*gs->mapx+yardxpos)) &&
				   (u=dynamic_cast<CUnit*>(s)) &&
				   u->beingBuilt && (u->buildProgress == 0.0f) &&
				   (!u->soloBuilder || (u->soloBuilder == owner))) {
					Command c2;
					c2.id = CMD_REPAIR;
					c2.params.push_back(u->id);
					c2.options = c.options | INTERNAL_ORDER;
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

	map<int, string>::iterator boi = buildOptions.find(c.id);
	if (!owner->beingBuilt && boi != buildOptions.end()) {
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(boi->second);
		const float radius = GetUnitDefRadius(ud, c.id);

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
					build.pos = helper->Pos2BuildPos(build);
					const float sqdist = f3SqDist(build.pos, builder->pos);

					if ((sqdist < Square(builder->buildDistance * 0.6f + radius)) ||
						(!owner->unitDef->canmove && (sqdist <= Square(builder->buildDistance + radius - 8.0f)))) {
						StopMove();

						if (luaRules && !luaRules->AllowUnitCreation(build.def, owner, &build.pos)) {
							FinishCommand();
						}
						else if (uh->MaxUnitsPerTeam() > (int) teamHandler->Team(owner->team)->units.size()) {
							// unit-limit not yet reached
							CFeature* f = NULL;
							buildRetries++;
							owner->moveType->KeepPointingTo(build.pos, builder->buildDistance * 0.7f + radius, false);

							if (builder->StartBuild(build, f) || (buildRetries > 20)) {
								building = true;
							}
							else if (f) {
								inCommand = false;
								ReclaimFeature(f);
							}
							else {
								if ((owner->team == gu->myTeam) && !(buildRetries & 7) && gu->buildWarnings) {
									logOutput.Print(
										"%s: build-position <%.2f, %.2f, %.2f> blocked after %d attempts",
										owner->unitDef->humanName.c_str(),
										build.pos.x, build.pos.y, build.pos.z,
										buildRetries
									);
									logOutput.SetLastMsgPos(owner->pos);
								}

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
				bi.buildFacing = int(abs(c.params[3])) % 4;

			bi.def = unitDefHandler->GetUnitDefByName(boi->second);

			CFeature* f = 0;
			uh->TestUnitBuildSquare(bi, f, owner->allyteam);

			if (f) {
				ReclaimFeature(f);
			} else {
				inCommand=true;
				SlowUpdate();
			}
		}
		return;
	}

	switch (c.id) {
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


/// add a command to reclaim a feature that is blocking our buildsite
void CBuilderCAI::ReclaimFeature(CFeature* f)
{
	if (!owner->unitDef->canReclaim || !f->def->reclaimable) {
		// FIXME user shouldn't be able to queue buildings on top of features
		// in the first place (in this case).
		StopMove();
		FinishCommand();
	} else {
		Command c2;
		c2.id=CMD_RECLAIM;
		c2.options=0;
		c2.params.push_back(f->id + uh->MaxUnits());
		commandQue.push_front(c2);
		SlowUpdate(); //this assumes that the reclaim command can never return directly without having reclaimed the target
	}
}


void CBuilderCAI::FinishCommand(void)
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
	CBuilder* builder = (CBuilder*) owner;
	assert(owner->unitDef->canRepair || owner->unitDef->canAssist);

	if (c.params.size() == 1 || c.params.size() == 5) {
		// repair unit
		CUnit* unit = uh->GetUnit(c.params[0]);

		if (unit == NULL) {
			FinishCommand();
			return;
		}

		if (tempOrder && owner->moveState == 1) {
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

		// don't consider units under construction irreparable
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
	assert(owner->unitDef->canCapture);
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
			if (f3SqDist(unit->pos, builder->pos) < Square(builder->buildDistance + unit->radius - 8)) {
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
	assert(owner->unitDef->canGuard);
	CBuilder* builder = (CBuilder*) owner;
	CUnit* guarded = uh->GetUnit(c.params[0]);

	if (guarded != NULL && guarded != owner && UpdateTargetLostTimer(guarded->id)) {
		if (CBuilder* b = dynamic_cast<CBuilder*>(guarded)) {
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

			if (b->curBuild &&
			    (!b->curBuild->soloBuilder || (b->curBuild->soloBuilder == owner)) &&
			    (( b->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			     (!b->curBuild->beingBuilt && owner->unitDef->canRepair))) {
				StopSlowGuard();

				Command nc;
					nc.id = CMD_REPAIR;
					nc.options = c.options;
					nc.params.push_back(b->curBuild->id);

				commandQue.push_front(nc);
				inCommand = false;
				SlowUpdate();
				return;
			}
		}

		if (CFactory* fac = dynamic_cast<CFactory*>(guarded)) {
			if (fac->curBuild &&
			    (!fac->curBuild->soloBuilder || (fac->curBuild->soloBuilder == owner)) &&
			    (( fac->curBuild->beingBuilt && owner->unitDef->canAssist) ||
			     (!fac->curBuild->beingBuilt && owner->unitDef->canRepair))) {
				StopSlowGuard();

				Command nc;
					nc.id = CMD_REPAIR;
					nc.options = c.options;
					nc.params.push_back(fac->curBuild->id);

				commandQue.push_front(nc);
				inCommand = false;
				// SlowUpdate();
				return;
			}
		}

		if (!(c.options & CONTROL_KEY) && IsUnitBeingReclaimed(guarded, owner))
			return;

		float3 curPos = owner->pos;
		float3 dif = guarded->pos - curPos;
		dif.Normalize();
		float3 goal = guarded->pos - dif * (builder->buildDistance * 0.5f);

		if (f3SqLen(guarded->pos - curPos) >
				(builder->buildDistance * 1.1f + guarded->radius) *
				(builder->buildDistance * 1.1f + guarded->radius)) {
			StopSlowGuard();
		}
		if (f3SqLen(guarded->pos - curPos) <
				(builder->buildDistance * 0.9f + guarded->radius) *
				(builder->buildDistance * 0.9f + guarded->radius)) {
			StartSlowGuard(guarded->maxSpeed);
			StopMove();

			owner->moveType->KeepPointingTo(guarded->pos,
				builder->buildDistance * 0.9f + guarded->radius, false);

			if ((guarded->health < guarded->maxHealth) &&
			    (guarded->unitDef->repairable) &&
			    (!guarded->soloBuilder || (guarded->soloBuilder == owner)) &&
			    (( guarded->beingBuilt && owner->unitDef->canAssist) ||
			     (!guarded->beingBuilt && owner->unitDef->canRepair))) {
				StopSlowGuard();

				Command nc;
					nc.id = CMD_REPAIR;
					nc.options = c.options;
					nc.params.push_back(guarded->id);

				commandQue.push_front(nc);
				inCommand = false;
				return;
			} else {
				NonMoving();
			}
		} else {
			if (f3SqLen(goalPos - goal) > 4000
					|| f3SqLen(goalPos - owner->pos) <
					   (owner->maxSpeed*30 + 1 + SQUARE_SIZE*2) *
					   (owner->maxSpeed*30 + 1 + SQUARE_SIZE*2)){
				SetGoal(goal,curPos);
			}
		}
	} else {
		FinishCommand();
	}
	return;
}


void CBuilderCAI::ExecuteReclaim(Command& c)
{
	CBuilder* builder = (CBuilder*) owner;
	assert(owner->unitDef->canReclaim);

	if (c.params.size() == 1 || c.params.size() == 5) {
		const int signedId = (int) c.params[0];

		if (signedId < 0) {
			logOutput.Print("Trying to reclaim unit or feature with id < 0 (%i), aborting.", signedId);
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
	return;
}


bool CBuilderCAI::ResurrectObject(CFeature *feature) {
	CBuilder* builder = (CBuilder*) owner;

	if (f3SqDist(feature->pos, builder->pos) < Square(builder->buildDistance * 0.9f + feature->radius)) {
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
	assert(owner->unitDef->canResurrect);
	CBuilder* builder = (CBuilder*) owner;

	if (c.params.size() == 1) {
		unsigned int id = (unsigned int) c.params[0];

		if (id >= uh->MaxUnits()) { // resurrect feature
			CFeature* feature = featureHandler->GetFeature(id - uh->MaxUnits());

			if (feature && feature->createdFromUnit != "") {
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
					// resurrection finished, start repair
					c.id = CMD_REPAIR; // kind of hackery to overwrite the current order i suppose
					c.params.clear();
					c.params.push_back(builder->lastResurrected);
					c.options |= INTERNAL_ORDER;
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
	assert(owner->unitDef->canPatrol);
	if (c.params.size() < 3) {
		// this shouldnt happen but anyway ...
		logOutput.Print("Error: got patrol cmd with less than 3 params on %s in buildercai",
			owner->unitDef->humanName.c_str());
		return;
	}

	Command temp;
		temp.id = CMD_FIGHT;
		temp.params.push_back(c.params[0]);
		temp.params.push_back(c.params[1]);
		temp.params.push_back(c.params[2]);
		temp.options = c.options|INTERNAL_ORDER;

	commandQue.push_back(c);
	commandQue.pop_front();
	commandQue.push_front(temp);
	Command tmpC;
		tmpC.id = CMD_PATROL;
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
	if (c.params.size() < 3) {		//this shouldnt happen but anyway ...
		logOutput.Print("Error: got fight cmd with less than 3 params on %s in BuilderCAI",owner->unitDef->humanName.c_str());
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
	assert(owner->unitDef->canRestore);
	CBuilder* builder = (CBuilder*) owner;

	if (inCommand) {
		if (!builder->terraforming) {
			FinishCommand();
		}
	} else if (owner->unitDef->canRestore) {
		float3 pos(c.params[0], c.params[1], c.params[2]);
			pos.y = ground->GetHeight2(pos.x, pos.y);
		const float radius = std::min(c.params[3], 200.0f);

		if (f3SqDist(builder->pos, pos) < Square(builder->buildDistance - 1)) {
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
		if (owner->unitDef->canResurrect && !feature->createdFromUnit.empty()) {
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


/** check if a unit is being reclaimed by a friendly con.

we assume that there won't be a lot of reclaimers because performance would suck
if there were. ideally reclaimers should be assigned on a per-unit basis, but
this requires tracking of deaths, which albeit already done isn't exactly simple
to follow.

TODO easy: store reclaiming units per allyteam
TODO harder: update reclaimers as they start/finish reclaims and/or die */
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
		if (c.id != CMD_RECLAIM || (c.params.size() != 1 && c.params.size() != 5)) {
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
		if (c.id != CMD_RECLAIM || (c.params.size() != 1 && c.params.size() != 5)) {
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
		if (c.id != CMD_RESURRECT || c.params.size() != 1) {
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

	if (f3SqDist(object->pos, builder->pos) < Square(builder->buildDistance - 1 + object->radius)) {
		StopMove();
		owner->moveType->KeepPointingTo(object->pos, builder->buildDistance * 0.9f + object->radius, false);
		builder->SetReclaimTarget(object);
	} else {
		if (f3SqDist(goalPos, object->pos) > 1) {
			SetGoal(object->pos, owner->pos);
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
		const std::vector<CUnit*> units = qf->GetUnitsExact(pos, radius);
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
		const std::vector<CFeature*> features = qf->GetFeaturesExact(pos, radius);
		for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
			const CFeature* f = *fi;
			if (f->def->reclaimable && (recSpecial || f->def->autoreclaim) && 
				(!recNonRez || !(f->def->destructable && f->createdFromUnit != ""))) {
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

		Command cmd;
			cmd.options = options | INTERNAL_ORDER;
			cmd.id = CMD_RECLAIM;
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
	const std::vector<CFeature*> features = qf->GetFeaturesExact(pos, radius);
	std::vector<CFeature*>::const_iterator fi;

	const CFeature* best = NULL;
	float bestDist = 1.0e30f;

	for (fi = features.begin(); fi != features.end(); ++fi) {
		const CFeature* f = *fi;
		if (f->def->destructable && f->createdFromUnit != "") {
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
		Command c2;
			c2.options = options | INTERNAL_ORDER;
			c2.id = CMD_RESURRECT;
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
	const std::vector<CUnit*> cu = qf->GetUnits(pos, radius);
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
		Command nc;
			nc.id = CMD_CAPTURE;
			nc.options = options | INTERNAL_ORDER;
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
	const std::vector<CUnit*> cu = qf->GetUnitsExact(pos, radius);
	std::vector<CUnit*>::const_iterator ui;

	const CUnit* best = NULL;
	float bestDist = 1.0e30f;
	bool haveEnemy = false;
	bool trySelfRepair = false;
	bool stationary = false;

	for (ui = cu.begin(); ui != cu.end(); ++ui) {
		CUnit* unit = *ui;
		if (teamHandler->Ally(owner->allyteam, unit->allyteam)) {
			if (!haveEnemy && (unit->health < unit->maxHealth)) {
				// don't help allies build unless set on roam
				if (unit->beingBuilt && owner->team != unit->team && (owner->moveState != 2)) {
					continue;
				}                
				// don't help factories produce units when set on hold pos                
				if (unit->beingBuilt && unit->mobility && (owner->moveState == 0)) {
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
		Command cmd;
			cmd.id = CMD_REPAIR;
			cmd.options = options | INTERNAL_ORDER;
			cmd.params.push_back(best->id);
			cmd.params.push_back(pos.x);
			cmd.params.push_back(pos.y);
			cmd.params.push_back(pos.z);
			cmd.params.push_back(radius);
		commandQue.push_front(cmd);
	}
	else {
		PushOrUpdateReturnFight(); // attackEnemy must be true
		Command cmd;
			cmd.id = CMD_ATTACK;
			cmd.options = options | INTERNAL_ORDER;
			cmd.params.push_back(best->id);
		commandQue.push_front(cmd);
	}

	return true;
}


/******************************************************************************/
//
//  Drawing routines
//

void CBuilderCAI::DrawCommands(void)
{
	if (uh->limitDgun && owner->unitDef->isCommander) {
		glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
		glSurfaceCircle(teamHandler->Team(owner->team)->startPos, uh->dgunRadius, 40);
	}

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (ci->id < 0) {
			map<int, string>::const_iterator boi = buildOptions.find(ci->id);

			if (boi != buildOptions.end()) {
				BuildInfo bi;
				bi.def = unitDefHandler->GetUnitDefByID(-(ci->id));

				if (ci->params.size() == 4) {
					bi.buildFacing = int(abs(ci->params[3])) % 4;
				}

				bi.pos = float3(ci->params[0], ci->params[1], ci->params[2]);
				bi.pos = helper->Pos2BuildPos(bi);

				cursorIcons.AddBuildIcon(ci->id, bi.pos, owner->team, bi.buildFacing);
				lineDrawer.DrawLine(bi.pos, cmdColors.build);

				// draw metal extraction range
				if (bi.def->extractRange > 0) {
					lineDrawer.Break(bi.pos, cmdColors.build);
					glColor4fv(cmdColors.rangeExtract);

					if (bi.def->extractSquare) {
						glSurfaceSquare(bi.pos, bi.def->extractRange, bi.def->extractRange);
					} else {
						glSurfaceCircle(bi.pos, bi.def->extractRange, 40);
					}

					lineDrawer.Restart();
				}
			}
			continue;
		}

		switch(ci->id) {
			case CMD_MOVE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.patrol);
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = uh->GetUnit(ci->params[0]);

				if ((unit != NULL) && isTrackable(unit)) {
					const float3 endPos = helper->GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_RESTORE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.restore);
				lineDrawer.Break(endPos, cmdColors.restore);
				glColor4fv(cmdColors.restore);
				glSurfaceCircle(endPos, ci->params[3], 20);
				lineDrawer.RestartWithColor(cmdColors.restore);
				break;
			}
			case CMD_ATTACK:
			case CMD_DGUN: {
				if (ci->params.size() == 1) {
					const CUnit* unit = uh->GetUnit(ci->params[0]);

					if ((unit != NULL) && isTrackable(unit)) {
						const float3 endPos = helper->GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
					}
				} else {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				}
				break;
			}
			case CMD_RECLAIM:
			case CMD_RESURRECT: {
				const float* color = (ci->id == CMD_RECLAIM) ? cmdColors.reclaim
				                                             : cmdColors.resurrect;
				if (ci->params.size() == 4) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
					lineDrawer.Break(endPos, color);
					glColor4fv(color);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartWithColor(color);
				} else {
					const int signedId = (int)ci->params[0];
					if (signedId < 0) {
						logOutput.Print("Trying to %s a feature or unit with id < 0 (%i), aborting.",
								(ci->id == CMD_RECLAIM) ? "reclaim" : "resurrect",
								signedId);
						break;
					}

					const unsigned int id = signedId;

					if (id >= uh->MaxUnits()) {
						GML_RECMUTEX_LOCK(feat); // DrawCommands

						CFeature* feature = featureHandler->GetFeature(id - uh->MaxUnits());
						if (feature) {
							const float3 endPos = feature->midPos;
							lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
						}
					} else {
						const CUnit* unit = uh->GetUnitUnsafe(id);

						if ((unit != NULL) && (unit != owner) && isTrackable(unit)) {
							const float3 endPos = helper->GetUnitErrorPos(unit, owner->allyteam);
							lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
						}
					}
				}
				break;
			}
			case CMD_REPAIR:
			case CMD_CAPTURE: {
				const float* color = (ci->id == CMD_REPAIR) ? cmdColors.repair
				                                            : cmdColors.capture;
				if (ci->params.size() == 4) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
					lineDrawer.Break(endPos, color);
					glColor4fv(color);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartWithColor(color);
				} else {
					if (ci->params.size() >= 1) {
						const CUnit* unit = uh->GetUnit(ci->params[0]);

						if ((unit != NULL) && isTrackable(unit)) {
							const float3 endPos = helper->GetUnitErrorPos(unit, owner->allyteam);
							lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
						}
					}
				}
				break;
			}
			case CMD_LOAD_ONTO: {
				const CUnit* unit = uh->GetUnitUnsafe(ci->params[0]);
				lineDrawer.DrawLineAndIcon(ci->id, unit->pos, cmdColors.load);
				break;
			}
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
			default: {
				DrawDefaultCommand(*ci);
				break;
			}
		}

	}
	lineDrawer.FinishPath();
}



// XXX move away from this class
void CBuilderCAI::DrawQuedBuildingSquares(void)
{
	CCommandQueue::const_iterator ci;
	// worst case - 2 squares per building (when underwater) - 8 vertices * 3 floats

	int buildCommands = 0;
	int underwaterCommands = 0;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (buildOptions.find(ci->id) != buildOptions.end()) {
			++buildCommands;
			BuildInfo bi(*ci);
			bi.pos = helper->Pos2BuildPos(bi);
			if (bi.pos.y < 0.f)
				++underwaterCommands;
		}
	}
	std::vector<GLfloat> vertices_quads(buildCommands * 12);
	std::vector<GLfloat> vertices_quads_uw(buildCommands * 12); // underwater
	// 4 vertical lines
	std::vector<GLfloat> vertices_lines(underwaterCommands * 24);
	// colors for lines
	std::vector<GLfloat> colors_lines(underwaterCommands * 48);

	int quadcounter = 0;
	int uwqcounter = 0;
	int linecounter = 0;

	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (buildOptions.find(ci->id) != buildOptions.end()) {
			BuildInfo bi(*ci);
			bi.pos = helper->Pos2BuildPos(bi);
			const float xsize = bi.GetXSize()*4;
			const float zsize = bi.GetZSize()*4;

			const float h = bi.pos.y;
			const float x1 = bi.pos.x - xsize;
			const float z1 = bi.pos.z - zsize;
			const float x2 = bi.pos.x + xsize;
			const float z2 = bi.pos.z + zsize;

			vertices_quads[quadcounter + 0] = x1;
			vertices_quads[quadcounter + 1] = h + 1;
			vertices_quads[quadcounter + 2] = z1;
			vertices_quads[quadcounter + 3] = x1;
			vertices_quads[quadcounter + 4] = h + 1;
			vertices_quads[quadcounter + 5] = z2;
			vertices_quads[quadcounter + 6] = x2;
			vertices_quads[quadcounter + 7] = h + 1;
			vertices_quads[quadcounter + 8] = z2;
			vertices_quads[quadcounter + 9] = x2;
			vertices_quads[quadcounter +10] = h + 1;
			vertices_quads[quadcounter +11] = z1;

			quadcounter += 12;

			if (bi.pos.y < 0.0f) {
				const float col[8] = { 0.0f, 0.0f, 1.0f, 0.5f, // start color
						       0.0f, 0.5f, 1.0f, 1.0f }; // end color

				vertices_quads_uw[uwqcounter + 0] = x1;
				vertices_quads_uw[uwqcounter + 1] = 0.f;
				vertices_quads_uw[uwqcounter + 2] = z1;
				vertices_quads_uw[uwqcounter + 3] = x1;
				vertices_quads_uw[uwqcounter + 4] = 0.f;
				vertices_quads_uw[uwqcounter + 5] = z2;
				vertices_quads_uw[uwqcounter + 6] = x2;
				vertices_quads_uw[uwqcounter + 7] = 0.f;
				vertices_quads_uw[uwqcounter + 8] = z2;
				vertices_quads_uw[uwqcounter + 9] = x2;
				vertices_quads_uw[uwqcounter +10] = 0.f;
				vertices_quads_uw[uwqcounter +11] = z1;

				uwqcounter += 12;

				for (int i = 0; i<4; ++i) {
					std::copy(col, col + 8, colors_lines.begin() + linecounter * 2 + i * 8);
				}

				vertices_lines[linecounter + 0] = x1;
				vertices_lines[linecounter + 1] = h;
				vertices_lines[linecounter + 2] = z1;
				vertices_lines[linecounter + 3] = x1;
				vertices_lines[linecounter + 4] = 0;
				vertices_lines[linecounter + 5] = z1;

				vertices_lines[linecounter + 6] = x2;
				vertices_lines[linecounter + 7] = h;
				vertices_lines[linecounter + 8] = z1;
				vertices_lines[linecounter + 9] = x2;
				vertices_lines[linecounter +10] = 0;
				vertices_lines[linecounter +11] = z1;

				vertices_lines[linecounter +12] = x2;
				vertices_lines[linecounter +13] = h;
				vertices_lines[linecounter +14] = z2;
				vertices_lines[linecounter +15] = x2;
				vertices_lines[linecounter +16] = 0;
				vertices_lines[linecounter +17] = z2;

				vertices_lines[linecounter +18] = x1;
				vertices_lines[linecounter +19] = h;
				vertices_lines[linecounter +20] = z2;
				vertices_lines[linecounter +21] = x1;
				vertices_lines[linecounter +22] = 0;
				vertices_lines[linecounter +23] = z2;

				linecounter += 24;
			}
		}
	}
	if (quadcounter) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
		glVertexPointer(3, GL_FLOAT, 0, &vertices_quads[0]);
		glDrawArrays(GL_QUADS, 0, quadcounter/3);

		if (linecounter) {
			glPushAttrib(GL_CURRENT_BIT);
			glColor4f(0.0f, 0.5f, 1.0f, 1.0f); // same as end color of lines
			glVertexPointer(3, GL_FLOAT, 0, &vertices_quads_uw[0]);
			glDrawArrays(GL_QUADS, 0, uwqcounter/3);
			glPopAttrib();

			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, &colors_lines[0]);
			glVertexPointer(3, GL_FLOAT, 0, &vertices_lines[0]);
			glDrawArrays(GL_LINES, 0, linecounter/3);
			glDisableClientState(GL_COLOR_ARRAY);
		}
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}


/******************************************************************************/
/******************************************************************************/
