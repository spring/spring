#include "StdAfx.h"
#include "BuilderCAI.h"
#include "LineDrawer.h"
#include "ExternalAI/Group.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "myMath.h"
#include "mmgr.h"

CBuilderCAI::CBuilderCAI(CUnit* owner)
: CMobileCAI(owner),
	building(false),
	cachedRadius(0),
	cachedRadiusId(0),
	buildRetries(0),
	lastPC1(-1),
	lastPC2(-1)
{
	CommandDescription c;
	if(owner->unitDef->canRepair){
		c.id=CMD_REPAIR;
		c.action="repair";
		c.hotkey="r";
		c.type=CMDTYPE_ICON_UNIT_OR_AREA;
		c.name="Repair";
		c.tooltip="Repair: Repairs another unit";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canReclaim){
		c.id=CMD_RECLAIM;
		c.action="reclaim";
		c.hotkey="e";
		c.type=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
		c.name="Reclaim";
		c.tooltip="Reclaim: Sucks in the metal/energy content of a unit/feature and add it to your storage";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canRestore){
		c.id=CMD_RESTORE;
		c.action="restore";
		c.hotkey="";
		c.type=CMDTYPE_ICON_AREA;
		c.name="Restore";
		c.tooltip="Restore: Restores an area of the map to its original height";
		c.params.push_back("200");
		possibleCommands.push_back(c);
	}
	c.params.clear();

	if(owner->unitDef->canResurrect){
		c.id=CMD_RESURRECT;
		c.action="resurrect";
		c.hotkey="";
		c.type=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
		c.name="Resurrect";
		c.tooltip="Resurrect: Resurrects a unit from a feature";
		possibleCommands.push_back(c);
	}
	if(owner->unitDef->canCapture){
		c.id=CMD_CAPTURE;
		c.action="capture";
		c.hotkey="";
		c.type=CMDTYPE_ICON_UNIT_OR_AREA;
		c.name="Capture";
		c.tooltip="Capture: Captures a unit from the enemy";
		possibleCommands.push_back(c);
	}
	CBuilder* fac=(CBuilder*)owner;

	map<int,string>::iterator bi;
	for(bi=fac->unitDef->buildOptions.begin();bi!=fac->unitDef->buildOptions.end();++bi){
		string name=bi->second;
		UnitDef* ud= unitDefHandler->GetUnitByName(name);
		CommandDescription c;
		c.id=-ud->id; //build options are always negative
		c.action="buildunit_" + StringToLower(ud->name);
		c.type=CMDTYPE_ICON_BUILDING;
		c.name=name;

		char tmp[500];
		sprintf(tmp,"\nHealth %.0f\nMetal cost %.0f\nEnergy cost %.0f Build time %.0f",ud->health,ud->metalCost,ud->energyCost,ud->buildTime);
		c.tooltip=string("Build: ")+ud->humanName + " " + ud->tooltip+tmp;

		possibleCommands.push_back(c);
		buildOptions[c.id]=name;
	}
	uh->AddBuilderCAI(this);
}

CBuilderCAI::~CBuilderCAI()
{
	uh->RemoveBuilderCAI(this);
}

void CBuilderCAI::SlowUpdate()
{
	if(commandQue.empty()){
		CMobileCAI::SlowUpdate();
		return;
	}

	Command& c=commandQue.front();
	CBuilder* fac=(CBuilder*)owner;
	float3 curPos=owner->pos;

	map<int,string>::iterator boi;
	if((boi=buildOptions.find(c.id))!=buildOptions.end()){
		float radius;
		if(cachedRadiusId==c.id){
			radius=cachedRadius;
		} else {
			radius=modelParser->Load3DO(
				unitDefHandler->GetUnitByName(boi->second)->model.modelpath,1,owner->team)->radius;
			cachedRadiusId=c.id;
			cachedRadius=radius;
		}
		if(inCommand){
			if(building){
				if(build.pos.distance2D(fac->pos)>fac->buildDistance+radius-8){
					owner->moveType->StartMoving(build.pos, fac->buildDistance*0.5f+radius);
				} else {
					StopMove();
					owner->moveType->KeepPointingTo(build.pos, (fac->buildDistance+radius)*0.6f, false);	//needed since above startmoving cancels this
				}
				if(!fac->curBuild && !fac->terraforming){
					building=false;
					StopMove();				//cancel the effect of KeepPointingTo
					FinishCommand();
				}
			} else {
				build.Parse(c);
				if(build.pos.distance2D(fac->pos)<fac->buildDistance*0.6f+radius){
					StopMove();
					if(uh->maxUnits>(int)gs->Team(owner->team)->units.size()){
						buildRetries++;
						owner->moveType->KeepPointingTo(build.pos, fac->buildDistance*0.7f+radius, false);
						if(fac->StartBuild(build) || buildRetries>20){
							building=true;
						} else {
							ENTER_MIXED;
							if(owner->team==gu->myTeam && !(buildRetries&7)){
								logOutput.Print("%s: Build pos blocked",owner->unitDef->humanName.c_str());
								logOutput.SetLastMsgPos(owner->pos);
							}
							ENTER_SYNCED;
							helper->BuggerOff(build.pos,radius);
							NonMoving();
						}
					}
				} else {
					if(owner->moveType->progressState==CMoveType::Failed){
						if(++buildRetries>5){
							StopMove();
							FinishCommand();
						}
					}
					SetGoal(build.pos,owner->pos, fac->buildDistance*0.4f+radius);
				}
			}
		} else {		//!inCommand
			BuildInfo bi;
			bi.pos.x=floor(c.params[0]/SQUARE_SIZE+0.5f)*SQUARE_SIZE;
			bi.pos.z=floor(c.params[2]/SQUARE_SIZE+0.5f)*SQUARE_SIZE;
			bi.pos.y=c.params[1];
			CFeature* f=0;
			if (c.params.size()==4)
				bi.buildFacing = int(c.params[3]);
			bi.def = unitDefHandler->GetUnitByName(boi->second);

			uh->TestUnitBuildSquare(bi,f,owner->allyteam);
			if(f){
				if (!owner->unitDef->canReclaim || !f->def->destructable) {
					// FIXME user shouldn't be able to queue buildings on top of features
					// in the first place (in this case).
					StopMove();
					FinishCommand();
				} else {
					Command c2;
					c2.id=CMD_RECLAIM;
					c2.options=0;
					c2.params.push_back(f->id+MAX_UNITS);
					commandQue.push_front(c2);
					SlowUpdate(); //this assumes that the reclaim command can never return directly without having reclaimed the target
				}
			} else {
				inCommand=true;
				SlowUpdate();
			}
		}
		return;
	}
	switch(c.id){
	case CMD_STOP:
		building=false;
		fac->StopBuild();
		break;
	case CMD_REPAIR:{
		assert(owner->unitDef->canRepair || owner->unitDef->canAssist);
		if(c.params.size()==1){		//repair unit
			CUnit* unit=uh->units[(int)c.params[0]];
			if(tempOrder && owner->moveState==1){		//limit how far away we go
				if(unit && LinePointDist(commandPos1,commandPos2,unit->pos) > 500){
					StopMove();
					FinishCommand();
					break;
				}
			}
			if(unit && unit->health<unit->maxHealth && unit!=owner && UpdateTargetLostTimer((int)c.params[0])){
				if(unit->pos.distance2D(fac->pos)<fac->buildDistance+unit->radius-8){
					StopMove();
					fac->SetRepairTarget(unit);
					owner->moveType->KeepPointingTo(unit->pos, fac->buildDistance*0.9f+unit->radius, false);
				} else {
					if(goalPos.distance2D(unit->pos)>1)
						SetGoal(unit->pos,owner->pos, fac->buildDistance*0.9f+unit->radius);
				}
			} else {
				StopMove();
				FinishCommand();
			}
		} else {			//repair area
			float3 pos(c.params[0],c.params[1],c.params[2]);
			float radius=c.params[3];
			if(FindRepairTargetAndRepair(pos,radius,c.options,false)){
				inCommand=false;
				SlowUpdate();
				return;
			}
			if(!(c.options & ALT_KEY)){
				FinishCommand();
			}
		}
		return;}
	case CMD_CAPTURE:{
		assert(owner->unitDef->canCapture);
		if(c.params.size()==1){		//capture unit
			CUnit* unit=uh->units[(int)c.params[0]];
			if(unit && unit->team!=owner->team && UpdateTargetLostTimer((int)c.params[0])){
				if(unit->pos.distance2D(fac->pos)<fac->buildDistance+unit->radius-8){
					StopMove();
					fac->SetCaptureTarget(unit);
					owner->moveType->KeepPointingTo(unit->pos, fac->buildDistance*0.9f+unit->radius, false);
				} else {
					if(goalPos.distance2D(unit->pos)>1)
						SetGoal(unit->pos,owner->pos, fac->buildDistance*0.9f+unit->radius);
				}
			} else {
				StopMove();
				FinishCommand();
			}
		} else {			//capture area
			float3 pos(c.params[0],c.params[1],c.params[2]);
			float radius=c.params[3];
			if(FindCaptureTargetAndCapture(pos,radius,c.options)){
				inCommand=false;
				SlowUpdate();
				return;
			}
			if(!(c.options & ALT_KEY)){
				FinishCommand();
			}
		}
		return;}
	case CMD_GUARD:{
		assert(owner->unitDef->canGuard);
		CUnit* guarded=uh->units[(int)c.params[0]];
		if(guarded && guarded!=owner && UpdateTargetLostTimer((int)c.params[0])){
			if(CBuilder* b=dynamic_cast<CBuilder*>(guarded)){
				if(b->terraforming){
					if(fac->pos.distance2D(b->terraformCenter)<fac->buildDistance*0.8f+b->terraformRadius*0.7f){
						StopMove();
						owner->moveType->KeepPointingTo(b->terraformCenter, fac->buildDistance*0.9f, false);
						fac->HelpTerraform(b);
					} else {
						SetGoal(b->terraformCenter,fac->pos,fac->buildDistance*0.7f+b->terraformRadius*0.6f);
					}
					return;
				}
				if (b->curBuild && ((b->curBuild->beingBuilt && owner->unitDef->canAssist) || (!b->curBuild->beingBuilt && owner->unitDef->canRepair))) {
					Command nc;
					nc.id=CMD_REPAIR;
					nc.options=c.options;
					nc.params.push_back(b->curBuild->id);
					commandQue.push_front(nc);
					inCommand=false;
//					SlowUpdate();
					return;
				}
			}
			if(CFactory* f=dynamic_cast<CFactory*>(guarded)){
				if (f->curBuild && ((f->curBuild->beingBuilt && owner->unitDef->canAssist) || (!f->curBuild->beingBuilt && owner->unitDef->canRepair))) {
					Command nc;
					nc.id=CMD_REPAIR;
					nc.options=c.options;
					nc.params.push_back(f->curBuild->id);
					commandQue.push_front(nc);
					inCommand=false;
//					SlowUpdate();
					return;
				}
			}
			float3 curPos=owner->pos;
			float3 dif=guarded->pos-curPos;
			dif.Normalize();
			float3 goal=guarded->pos-dif*(fac->buildDistance-5);
			if((guarded->pos-curPos).SqLength2D()<(fac->buildDistance*0.9f+guarded->radius)*(fac->buildDistance*0.9f+guarded->radius)){
				StopMove();
//				logOutput.Print("should point with type 3?");
				owner->moveType->KeepPointingTo(guarded->pos, fac->buildDistance*0.9f+guarded->radius, false);
				if(guarded->health<guarded->maxHealth && ((guarded->beingBuilt && owner->unitDef->canAssist) || (!guarded->beingBuilt && owner->unitDef->canRepair)))
					fac->SetRepairTarget(guarded);
				else
					NonMoving();
			}else{
				if((goalPos-goal).SqLength2D()>4000)
					SetGoal(goal,curPos);
			}
		} else {
			FinishCommand();
		}
		return;}
	case CMD_RECLAIM:{
		assert(owner->unitDef->canReclaim);
		if(c.params.size()==1){
			int id=(int) c.params[0];
			if(id>=MAX_UNITS){		//reclaim feature
				CFeature* feature=featureHandler->features[id-MAX_UNITS];
				if(feature){
					if(feature->pos.distance2D(fac->pos)<fac->buildDistance*0.9f+feature->radius){
						StopMove();
						owner->moveType->KeepPointingTo(feature->pos, fac->buildDistance*0.9f+feature->radius, false);
						fac->SetReclaimTarget(feature);
					} else {
						if(goalPos.distance2D(feature->pos)>1){
							SetGoal(feature->pos,owner->pos, fac->buildDistance*0.8f+feature->radius);
						} else {
							if(owner->moveType->progressState==CMoveType::Failed){
								StopMove();
								FinishCommand();
							}
						}
					}
				} else {
					StopMove();
					FinishCommand();
				}

			} else {							//reclaim unit
				CUnit* unit=uh->units[id];
				if(unit && unit!=owner && unit->unitDef->reclaimable && UpdateTargetLostTimer(id)){
					if(unit->pos.distance2D(fac->pos)<fac->buildDistance-1+unit->radius){
						StopMove();
						owner->moveType->KeepPointingTo(unit->pos, fac->buildDistance*0.9f+unit->radius, false);
						fac->SetReclaimTarget(unit);
					} else {
						if(goalPos.distance2D(unit->pos)>1){
							SetGoal(unit->pos,owner->pos);
						}else{
							if(owner->moveType->progressState==CMoveType::Failed){
								StopMove();
								FinishCommand();
							}
						}
					}
				} else {
					FinishCommand();
				}
			}
		} else if(c.params.size()==4){//area reclaim
			float3 pos(c.params[0],c.params[1],c.params[2]);
			float radius=c.params[3];
			if(FindReclaimableFeatureAndReclaim(pos,radius,c.options,true)){
				inCommand=false;
				SlowUpdate();
				return;
			}
			if(!(c.options & ALT_KEY)){
				FinishCommand();
			}
		} else {	//wrong number of parameters
			FinishCommand();
		}
		return;}
	case CMD_RESURRECT:{
		assert(owner->unitDef->canResurrect);
		if(c.params.size()==1){
			int id=(int)c.params[0];
			if(id>=MAX_UNITS){		//resurrect feature
				CFeature* feature=featureHandler->features[id-MAX_UNITS];
				if(feature && feature->createdFromUnit!=""){
					if(feature->pos.distance2D(fac->pos)<fac->buildDistance*0.9f+feature->radius){
						StopMove();
						owner->moveType->KeepPointingTo(feature->pos, fac->buildDistance*0.9f+feature->radius, false);
						fac->SetResurrectTarget(feature);
					} else {
						if(goalPos.distance2D(feature->pos)>1){
							SetGoal(feature->pos,owner->pos, fac->buildDistance*0.8f+feature->radius);
						} else {
							if(owner->moveType->progressState==CMoveType::Failed){
								StopMove();
								FinishCommand();
							}
						}
					}
				} else {
					if(fac->lastResurrected && uh->units[fac->lastResurrected] && owner->unitDef->canRepair){	//resurrection finished, start repair
						c.id=CMD_REPAIR;		//kind of hackery to overwrite the current order i suppose
						c.params.clear();
						c.params.push_back(fac->lastResurrected);
						c.options|=INTERNAL_ORDER;
						fac->lastResurrected=0;
						inCommand=false;
						SlowUpdate();
						return;
					}
					StopMove();
					FinishCommand();
				}
			} else {							//resurrect unit
				FinishCommand();
			}
		} else if(c.params.size()==4){//area resurect
			float3 pos(c.params[0],c.params[1],c.params[2]);
			float radius=c.params[3];
			if(FindResurrectableFeatureAndResurrect(pos,radius,c.options)){
				inCommand=false;
				SlowUpdate();
				return;
			}
			if(!(c.options & ALT_KEY)){
				FinishCommand();
			}
		} else {	//wrong number of parameters
			FinishCommand();
		}
		return;}
	case CMD_PATROL:{
		assert(owner->unitDef->canPatrol);
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
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
		if(owner->group){
			owner->group->CommandFinished(owner->id,CMD_PATROL);
		}
		SlowUpdate();
		return;}
	case CMD_FIGHT:{
		assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);
		if(tempOrder){
			tempOrder=false;
			inCommand=true;
		}
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			logOutput.Print("Error: got fight cmd with less than 3 params on %s in BuilderCAI",owner->unitDef->humanName.c_str());
			return;
		}
		if(c.params.size() >= 6){
			if(!inCommand){
				commandPos1 = float3(c.params[3],c.params[4],c.params[5]);
			}
		} else {
			// Some hackery to make sure the line (commandPos1,commandPos2) is NOT
			// rotated (only shortened) if we reach this because the previous return
			// fight command finished by the 'if((curPos-pos).SqLength2D()<(64*64)){'
			// condition, but is actually updated correctly if you click somewhere
			// outside the area close to the line (for a new command).
			commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, curPos);
			if ((curPos-commandPos1).SqLength2D()>(96*96)) {
				commandPos1 = curPos;
			}
		}
		float3 pos(c.params[0],c.params[1],c.params[2]);
		if(!inCommand){
			inCommand = true;
			commandPos2=pos;
		}
		if(!(pos==goalPos)){
			SetGoal(pos,curPos);
		}
		float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, curPos);
		if((owner->unitDef->canRepair || owner->unitDef->canAssist) && FindRepairTargetAndRepair(curPosOnLine,300*owner->moveState+fac->buildDistance-8,c.options,true)){
			CUnit* target=uh->units[(int)commandQue.front().params[0]];
			tempOrder=true;
			inCommand=false;
			if(lastPC1!=gs->frameNum){	//avoid infinite loops
				lastPC1=gs->frameNum;
				SlowUpdate();
			}
			return;
		}
		if(owner->unitDef->canReclaim && FindReclaimableFeatureAndReclaim(curPosOnLine,300,c.options,false)){
			tempOrder=true;
			inCommand=false;
			if(lastPC2!=gs->frameNum){	//avoid infinite loops
				lastPC2=gs->frameNum;
				SlowUpdate();
			}
			return;
		}
		if((curPos-pos).SqLength2D()<(64*64)){
			FinishCommand();
			return;
		}
		if(owner->haveTarget && owner->moveType->progressState!=CMoveType::Done){
			StopMove();
		} else if(owner->moveType->progressState!=CMoveType::Active){
			owner->moveType->StartMoving(goalPos, 8);
		}
		return;}
	case CMD_RESTORE:{
		assert(owner->unitDef->canRestore);
		if(inCommand){
			if(!fac->terraforming){
				FinishCommand();
			}
		} else if(owner->unitDef->canRestore){
			float3 pos(c.params[0],c.params[1],c.params[2]);
			float radius(c.params[3]);
			if(fac->pos.distance2D(pos)<fac->buildDistance-1){
				StopMove();
				fac->StartRestore(pos,radius);
				owner->moveType->KeepPointingTo(pos, fac->buildDistance*0.9f, false);
				inCommand=true;
			} else {
				SetGoal(pos,owner->pos, fac->buildDistance*0.9f);
			}
		}
		return;}
	default:
		break;
	}
	CMobileCAI::SlowUpdate();
}

int CBuilderCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!gs->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack && (owner->maxRange > 0)) {
				return CMD_ATTACK;
			} else if (owner->unitDef->canReclaim && pointed->unitDef->reclaimable) {
				return CMD_RECLAIM;
			}
		} else {
			if ((pointed->health < pointed->maxHealth) && ((pointed->beingBuilt && owner->unitDef->canAssist) || (!pointed->beingBuilt && owner->unitDef->canRepair))) {
				return CMD_REPAIR;
			} else if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	if (feature) {
		if (owner->unitDef->canResurrect && !feature->createdFromUnit.empty()) {
			return CMD_RESURRECT;
		} else if(owner->unitDef->canReclaim && feature->def->destructable) {
			return CMD_RECLAIM;
		}
	}
	return CMD_MOVE;
}


void CBuilderCAI::DrawCommands(void)
{
	if(uh->limitDgun && owner->unitDef->isCommander) {
		glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
		glSurfaceCircle(gs->Team(owner->team)->startPos, uh->dgunRadius, 40);
	}

	lineDrawer.StartPath(owner->midPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	deque<Command>::iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		switch(ci->id) {
			case CMD_WAIT:
 			case CMD_SELFD:{
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
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
				const CUnit* unit = uh->units[int(ci->params[0])];
				if((unit != NULL) && isTrackable(unit)) {
					const float3 endPos =
						helper->GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_RESTORE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.restore);
				lineDrawer.Break(endPos, cmdColors.restore);
				glSurfaceCircle(endPos, ci->params[3], 20);
				lineDrawer.RestartSameColor();
				break;
			}
			case CMD_ATTACK:
			case CMD_DGUN: {
				if (ci->params.size() == 1) {
					const CUnit* unit = uh->units[int(ci->params[0])];
					if((unit != NULL) && isTrackable(unit)) {
						const float3 endPos =
						  helper->GetUnitErrorPos(unit, owner->allyteam);
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
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartSameColor();
				} else {
					int id = (int)ci->params[0];
					if (id >= MAX_UNITS) {
						if (featureHandler->features[id - MAX_UNITS]) {
							const float3 endPos =
								featureHandler->features[id - MAX_UNITS]->midPos;
							lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
						}
					} else {
						const CUnit* unit = uh->units[id];
						if((unit != NULL) && (unit != owner) && isTrackable(unit)) {
							const float3 endPos =
								helper->GetUnitErrorPos(unit, owner->allyteam);
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
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartSameColor();
				} else {
					const CUnit* unit = uh->units[int(ci->params[0])];
					if((unit != NULL) && isTrackable(unit)) {
						const float3 endPos =
							helper->GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, color);
					}
				}
				break;
			}
		}

		map<int, string>::iterator boi;
		if ((boi = buildOptions.find(ci->id)) != buildOptions.end()) {
			BuildInfo bi;
			bi.pos = float3(ci->params[0], ci->params[1], ci->params[2]);
			if (ci->params.size() == 4) {
				bi.buildFacing = int(ci->params[3]);
			}
			bi.def = unitDefHandler->GetUnitByName(boi->second);
			bi.pos = helper->Pos2BuildPos(bi);

			// draw the line, and break the path
			lineDrawer.DrawLine(bi.pos, cmdColors.build); // no icon
			lineDrawer.Break(bi.pos, cmdColors.build);

			//draw extraction range
			if (bi.def->extractRange > 0) {
				glColor4f(1.0f, 0.3f, 0.3f, 0.7f);
				glSurfaceCircle(bi.pos, bi.def->extractRange, 40);
			}

			// draw the building (but not its base square)
			glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
			glEnable(GL_DEPTH_TEST);
			unitDrawer->DrawBuildingSample(bi.def, owner->team, bi.pos, bi.buildFacing);
			glDisable(GL_DEPTH_TEST);
			
			// restore the blending function
			glBlendFunc((GLenum)cmdColors.QueuedBlendSrc(),
			            (GLenum)cmdColors.QueuedBlendDst());

			// restart the line path
			lineDrawer.Restart();
		}
	}
	lineDrawer.FinishPath();
}


void CBuilderCAI::GiveCommand(const Command& c)
{
	if (!AllowedCommand(c))
		return;

	if(!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.id)==nonQueingCommands.end()){
		building=false;
		CBuilder* fac=(CBuilder*)owner;
		fac->StopBuild();
	}

	map<int,string>::iterator boi;
	if((boi=buildOptions.find(c.id))!=buildOptions.end()){
		if(c.params.size()<3)
			return;
		BuildInfo bi;
		bi.pos = float3(c.params[0],c.params[1],c.params[2]);
		if(c.params.size()==4) bi.buildFacing=int(c.params[3]);
		bi.def = unitDefHandler->GetUnitByName(boi->second);
		bi.pos=helper->Pos2BuildPos(bi);
		CFeature* feature;
		if(!uh->TestUnitBuildSquare(bi,feature,owner->allyteam)) {
			if (!feature && owner->unitDef->canAssist) {
				int yardxpos=int(bi.pos.x+4)/SQUARE_SIZE;
				int yardypos=int(bi.pos.z+4)/SQUARE_SIZE;
				CSolidObject* s;
				CUnit* u;
				if((s=readmap->GroundBlocked(yardypos*gs->mapx+yardxpos)) && (u=dynamic_cast<CUnit*>(s)) && u->beingBuilt && u->buildProgress == 0.0f) {
					Command c2;
					c2.id = CMD_REPAIR;
					c2.params.push_back(u->id);
					c2.options = c.options | INTERNAL_ORDER;
					CMobileCAI::GiveCommand(c2);
					CMobileCAI::GiveCommand(c);
				}
			}
			return;
		}
	}
	CMobileCAI::GiveCommand(c);
}


void CBuilderCAI::DrawQuedBuildingSquares(void)
{
	deque<Command>::const_iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (buildOptions.find(ci->id) != buildOptions.end()) {
			BuildInfo bi(*ci);
			bi.pos = helper->Pos2BuildPos(bi);
			const float xsize = bi.GetXSize()*4;
			const float ysize = bi.GetYSize()*4;
			glBegin(GL_LINE_LOOP);
			glVertexf3(bi.pos + float3(+xsize, 1, +ysize));
			glVertexf3(bi.pos + float3(-xsize, 1, +ysize));
			glVertexf3(bi.pos + float3(-xsize, 1, -ysize));
			glVertexf3(bi.pos + float3(+xsize, 1, -ysize));
			glEnd();
		}
	}
}


bool CBuilderCAI::FindReclaimableFeatureAndReclaim(float3 pos, float radius,unsigned char options, bool recAny)
{
	CFeature* best=0;
	float bestDist=10000000;
	std::vector<CFeature*> features=qf->GetFeaturesExact(pos,radius);
	for(std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
		if((*fi)->def->destructable && (*fi)->allyteam!=owner->allyteam){
			float dist=(*fi)->pos.distance2D(owner->pos);
			if(dist<bestDist && (recAny
			  ||((*fi)->def->energy > 0 && gs->Team(owner->team)->energy < gs->Team(owner->team)->energyStorage)
			  ||((*fi)->def->metal > 0 && gs->Team(owner->team)->metal < gs->Team(owner->team)->metalStorage))){
				bestDist=dist;
				best=*fi;
			}
		}
	}
	if(best){
		Command c2;
		if(!recAny){
			PushOrUpdateReturnFight();
		}
		c2.options=options | INTERNAL_ORDER;
		c2.id=CMD_RECLAIM;
		c2.params.push_back(MAX_UNITS+best->id);
		commandQue.push_front(c2);
		return true;
	}
	return false;
}

bool CBuilderCAI::FindResurrectableFeatureAndResurrect(float3 pos, float radius,unsigned char options)
{
	CFeature* best=0;
	float bestDist=10000000;
	std::vector<CFeature*> features=qf->GetFeaturesExact(pos,radius);
	for(std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
		if((*fi)->def->destructable && (*fi)->createdFromUnit!=""){
			float dist=(*fi)->pos.distance2D(owner->pos);
			if(dist<bestDist)
			{
				bestDist=dist;
				best=*fi;
			}
		}
	}
	if(best){
		Command c2;
		c2.options=options | INTERNAL_ORDER;
		c2.id=CMD_RESURRECT;
		c2.params.push_back(MAX_UNITS+best->id);
		commandQue.push_front(c2);
		return true;
	}
	return false;
}

void CBuilderCAI::FinishCommand(void)
{
	if(commandQue.front().timeOut==INT_MAX)
		buildRetries=0;
	CMobileCAI::FinishCommand();
}

bool CBuilderCAI::FindRepairTargetAndRepair(float3 pos, float radius,unsigned char options, bool attackEnemy)
{
	std::vector<CUnit*> cu=qf->GetUnits(pos,radius);
	int myAllyteam=owner->allyteam;
	for(std::vector<CUnit*>::iterator ui=cu.begin();ui!=cu.end();++ui){
		if(gs->Ally(owner->allyteam,(*ui)->allyteam) && (*ui)->health<(*ui)->maxHealth && (*ui)!=owner){
			if((*ui)->mobility && (*ui)->beingBuilt && owner->moveState<2)	//dont help factories produce units unless set on roam
				continue;
			if (!((*ui)->beingBuilt && owner->unitDef->canAssist) && !(!(*ui)->beingBuilt && owner->unitDef->canRepair))
				continue;
			Command nc;
			if(attackEnemy){
				PushOrUpdateReturnFight();
			}
			nc.id=CMD_REPAIR;
			nc.options=options | INTERNAL_ORDER;
			nc.params.push_back((*ui)->id);
			commandQue.push_front(nc);
			return true;
		} else if(attackEnemy && owner->unitDef->canAttack && owner->maxRange>0 && !gs->Ally(owner->allyteam,(*ui)->allyteam)){
			Command nc;
			PushOrUpdateReturnFight();
			nc.id=CMD_ATTACK;
			nc.options=options | INTERNAL_ORDER;
			nc.params.push_back((*ui)->id);
			commandQue.push_front(nc);
			return true;
		}
	}
	return false;
}

bool CBuilderCAI::FindCaptureTargetAndCapture(float3 pos, float radius,unsigned char options)
{
	std::vector<CUnit*> cu=qf->GetUnits(pos,radius);
	int myAllyteam=owner->allyteam;
	for(std::vector<CUnit*>::iterator ui=cu.begin();ui!=cu.end();++ui){
		if(!gs->Ally(myAllyteam,(*ui)->allyteam) && (*ui)!=owner && !(*ui)->beingBuilt){
			Command nc;
			nc.id=CMD_CAPTURE;
			nc.options=options | INTERNAL_ORDER;
			nc.params.push_back((*ui)->id);
			commandQue.push_front(nc);
			return true;
		}
	}
	return false;
}
