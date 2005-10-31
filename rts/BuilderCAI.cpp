#include "StdAfx.h"
#include "BuilderCAI.h"
#include "Builder.h"
#include "Group.h"
#include "myGL.h"
#include "UnitHandler.h"
#include "TextureHandler.h"
#include "UnitLoader.h"
#include "SelectedUnits.h"
#include "Building.h"
#include "QuadField.h"
#include "3DModelParser.h"
#include "Factory.h"
#include "MoveType.h"
#include "Ground.h"
#include "UnitDefHandler.h"
#include "GameHelper.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "InfoConsole.h"
#include "Team.h"
//#include "mmgr.h"

CBuilderCAI::CBuilderCAI(CUnit* owner)
: CMobileCAI(owner),
	building(false),
	commandFromPatrol(false),
	buildPos(0,0,0),
	cachedRadius(0),
	cachedRadiusId(0),
	buildRetries(0)
{
	CommandDescription c;
	c.id=CMD_REPAIR;
	c.type=CMDTYPE_ICON_UNIT_OR_AREA;
	c.name="Repair";
	c.tooltip="Repair: Repairs another unit";
	c.key='R';
	possibleCommands.push_back(c);

	c.id=CMD_RECLAIM;
	c.type=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
	c.name="Reclaim";
	c.tooltip="Reclaim: Sucks in the metal/energy content of a unit/feature and add it to your storage";
	c.key='E';
	possibleCommands.push_back(c);

	c.id=CMD_RESTORE;
	c.type=CMDTYPE_ICON_AREA;
	c.name="Restore";
	c.tooltip="Restore: Restores an area of the map to its original height";
	c.key=0;
	c.params.push_back("200");
	possibleCommands.push_back(c);
	c.params.clear();

	if(owner->unitDef->canResurrect){
		c.id=CMD_RESURRECT;
		c.type=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
		c.name="Resurrect";
		c.tooltip="Resurrect: Resurrects a unit from a feature";
		c.key=0;
		possibleCommands.push_back(c);
	}
	if(owner->unitDef->canCapture){
		c.id=CMD_CAPTURE;
		c.type=CMDTYPE_ICON_UNIT_OR_AREA;
		c.name="Capture";
		c.tooltip="Capture: Captures a unit from the enemy";
		c.key=0;
		possibleCommands.push_back(c);
	}
	CBuilder* fac=(CBuilder*)owner;

	map<int,string>::iterator bi;
	for(bi=fac->unitDef->buildOptions.begin();bi!=fac->unitDef->buildOptions.end();++bi){
		string name=bi->second;
		UnitDef* ud= unitDefHandler->GetUnitByName(name);
		CommandDescription c;
		c.id=-ud->id;						//build options are always negative
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

	map<int,string>::iterator boi;
	if((boi=buildOptions.find(c.id))!=buildOptions.end()){
		float radius;
		if(cachedRadiusId==c.id){
			radius=cachedRadius;
		} else {
			radius=modelParser->Load3DO(unitDefHandler->GetUnitByName(boi->second)->model.modelpath,1,owner->team)->radius;
			cachedRadiusId=c.id;
			cachedRadius=radius;
		}
		if(inCommand){
			if(building){
				if(buildPos.distance2D(fac->pos)>fac->buildDistance+radius-8){
					owner->moveType->StartMoving(buildPos, fac->buildDistance*0.5+radius);
				} else {
					StopMove();
					owner->moveType->KeepPointingTo(buildPos, (fac->buildDistance+radius)*0.6, false);	//needed since above startmoving cancels this
				}
				if(!fac->curBuild && !fac->terraforming){
					building=false;
					StopMove();				//cancel the effect of KeepPointingTo
					FinishCommand();
				}
			} else {
				buildPos.x=c.params[0];
				buildPos.y=c.params[1];
				buildPos.z=c.params[2];
				if(buildPos.distance2D(fac->pos)<fac->buildDistance*0.6+radius){
					StopMove();
					if(uh->maxUnits>(int)gs->teams[owner->team]->units.size()){
						buildRetries++;
						owner->moveType->KeepPointingTo(buildPos, fac->buildDistance*0.7+radius, false);
						if(fac->StartBuild(boi->second,buildPos) || buildRetries>20){
							building=true;
						} else {
							ENTER_MIXED;
							if(owner->team==gu->myTeam && !(buildRetries&7)){
								info->AddLine("%s: Build pos blocked",owner->unitDef->humanName.c_str());
								info->SetLastMsgPos(owner->pos);
							}
							ENTER_SYNCED;
							helper->BuggerOff(buildPos,radius);
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
					SetGoal(buildPos,owner->pos, fac->buildDistance*0.4+radius);
				}
			}
		} else {		//!inCommand
			float3 pos;
			pos.x=floor(c.params[0]/SQUARE_SIZE+0.5)*SQUARE_SIZE;
			pos.z=floor(c.params[2]/SQUARE_SIZE+0.5)*SQUARE_SIZE;
			pos.y=c.params[1];
			CFeature* f=0;
			uh->TestUnitBuildSquare(pos,boi->second,f);
			if(f){
				Command c2;
				c2.id=CMD_RECLAIM;
				c2.options=0;
				c2.params.push_back(f->id+MAX_UNITS);
				commandQue.push_front(c2);
				SlowUpdate();										//this assumes that the reclaim command can never return directly without having reclaimed the target
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
		if(c.params.size()==1){		//repair unit
			CUnit* unit=uh->units[(int)c.params[0]];
			if(unit && unit->health<unit->maxHealth && unit!=owner){
				if(unit->pos.distance2D(fac->pos)<fac->buildDistance+unit->radius-8){
					StopMove();
					fac->SetRepairTarget(unit);
					owner->moveType->KeepPointingTo(unit->pos, fac->buildDistance*0.9+unit->radius, false);
				} else {
					if(goalPos.distance2D(unit->pos)>1)
						SetGoal(unit->pos,owner->pos, fac->buildDistance*0.9+unit->radius);
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
		if(c.params.size()==1){		//capture unit
			CUnit* unit=uh->units[(int)c.params[0]];
			if(unit && unit->team!=owner->team){
				if(unit->pos.distance2D(fac->pos)<fac->buildDistance+unit->radius-8){
					StopMove();
					fac->SetCaptureTarget(unit);
					owner->moveType->KeepPointingTo(unit->pos, fac->buildDistance*0.9+unit->radius, false);
				} else {
					if(goalPos.distance2D(unit->pos)>1)
						SetGoal(unit->pos,owner->pos, fac->buildDistance*0.9+unit->radius);
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
		CUnit* guarded=uh->units[(int)c.params[0]];
		if(guarded && guarded!=owner){
			if(CBuilder* b=dynamic_cast<CBuilder*>(guarded)){
				if(b->terraforming){
					if(fac->pos.distance2D(b->terraformCenter)<fac->buildDistance*0.8+b->terraformRadius*0.7){
						StopMove();
						owner->moveType->KeepPointingTo(b->terraformCenter, fac->buildDistance*0.9, false);
						fac->HelpTerraform(b);
					} else {
						SetGoal(b->terraformCenter,fac->pos,fac->buildDistance*0.7+b->terraformRadius*0.6);
					}
					return;
				}
				if(b->curBuild){
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
				if(f->curBuild){
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
			if((guarded->pos-curPos).SqLength2D()<(fac->buildDistance*0.9+guarded->radius)*(fac->buildDistance*0.9+guarded->radius)){
				StopMove();
//				info->AddLine("should point with type 3?");
				owner->moveType->KeepPointingTo(guarded->pos, fac->buildDistance*0.9+guarded->radius, false);
				if(guarded->health<guarded->maxHealth)
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
		if(c.params.size()==1){
			int id=(int) c.params[0];
			if(id>=MAX_UNITS){		//reclaim feature
				CFeature* feature=featureHandler->features[id-MAX_UNITS];
				if(feature){
					if(feature->pos.distance2D(fac->pos)<fac->buildDistance*0.9+feature->radius){
						StopMove();
						owner->moveType->KeepPointingTo(feature->pos, fac->buildDistance*0.9+feature->radius, false);
						fac->SetReclaimTarget(feature);
					} else {
						if(goalPos.distance2D(feature->pos)>1){
							SetGoal(feature->pos,owner->pos, fac->buildDistance*0.8+feature->radius);
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
				if(unit && unit!=owner){
					if(unit->pos.distance2D(fac->pos)<fac->buildDistance-1+unit->radius){
						StopMove();
						owner->moveType->KeepPointingTo(unit->pos, fac->buildDistance*0.9+unit->radius, false);
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
			if(FindReclaimableFeatureAndReclaim(pos,radius,c.options)){
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
		if(c.params.size()==1){
			int id=(int)c.params[0];
			if(id>=MAX_UNITS){		//resurrect feature
				CFeature* feature=featureHandler->features[id-MAX_UNITS];
				if(feature && feature->createdFromUnit!=""){
					if(feature->pos.distance2D(fac->pos)<fac->buildDistance*0.9+feature->radius){
						StopMove();
						owner->moveType->KeepPointingTo(feature->pos, fac->buildDistance*0.9+feature->radius, false);
						fac->SetResurrectTarget(feature);
					} else {
						if(goalPos.distance2D(feature->pos)>1){
							SetGoal(feature->pos,owner->pos, fac->buildDistance*0.8+feature->radius);
						} else {
							if(owner->moveType->progressState==CMoveType::Failed){
								StopMove();
								FinishCommand();			
							}
						}
					}
				} else {
					if(fac->lastResurrected && uh->units[fac->lastResurrected]){	//resurrection finished, start repair
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
		float3 curPos=owner->pos;
		if(commandFromPatrol){
			commandFromPatrol=false;
			inCommand=true;
			--patrolTime;		//prevent searching reclaimtargets etc directly if one of those call returned directly (infinite loop)
		}
		if(!inCommand){
			float3 pos(c.params[0],c.params[1],c.params[2]);
			inCommand=true;
			Command co;
			co.id=CMD_PATROL;
			co.params.push_back(curPos.x);
			co.params.push_back(curPos.y);
			co.params.push_back(curPos.z);
			commandQue.push_front(co);
			activeCommand=1;
			patrolTime=0;
		}
		Command& c=commandQue[activeCommand];

		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			info->AddLine("Error: got patrol cmd with less than 3 params on %s",owner->unitDef->humanName.c_str());
			return;
		}

		if(!(patrolTime&3) && owner->fireState==2){
			if(FindRepairTargetAndRepair(owner->pos,300*owner->moveState,c.options,true)){
				commandFromPatrol=true;
				inCommand=false;
				SlowUpdate();
				return;
			}
			if(FindReclaimableFeatureAndReclaim(owner->pos,300*owner->moveState,c.options)){
				commandFromPatrol=true;
				inCommand=false;
				SlowUpdate();
				return;
			}
			patrolGoal=float3(c.params[0],c.params[1],c.params[2]);
		}
		patrolTime++;
		if(!(patrolGoal==goalPos)){
			SetGoal(patrolGoal,curPos);
		}
		if((curPos-float3(c.params[0],c.params[1],c.params[2])).SqLength2D()<64){
			if(owner->group)
				owner->group->CommandFinished(owner->id,CMD_PATROL);

			if((int)commandQue.size()<=activeCommand+1)
				activeCommand=0;
			else {
				if(commandQue[activeCommand+1].id!=CMD_PATROL)
					activeCommand=0;
				else
					activeCommand++;
			}
		}
		if(owner->haveTarget)
			StopMove();
		else
			owner->moveType->StartMoving(patrolGoal, 1000);					
		return;}
	case CMD_RESTORE:{
		if(inCommand){
			if(!fac->terraforming){
				FinishCommand();
			}
		} else {
			float3 pos(c.params[0],c.params[1],c.params[2]);
			float radius(c.params[3]);
			if(fac->pos.distance2D(pos)<fac->buildDistance-1){
				StopMove();
				fac->StartRestore(pos,radius);
				owner->moveType->KeepPointingTo(pos, fac->buildDistance*0.9, false);
				inCommand=true;
			} else {
				SetGoal(pos,owner->pos, fac->buildDistance*0.9);
			}
		}
		return;}
	default:
		break;
	}
	CMobileCAI::SlowUpdate();
}

int CBuilderCAI::GetDefaultCmd(CUnit *pointed,CFeature* feature)
{
	if(pointed){
		if(!gs->allies[gu->myAllyTeam][pointed->allyteam]){
			if(owner->maxRange>0)
				return CMD_ATTACK;
			else
				return CMD_RECLAIM;
		} else {
			if(pointed->health<pointed->maxHealth)
				return CMD_REPAIR;
			else
				return CMD_GUARD;
		}
	}
	if(feature){
		if(owner->unitDef->canResurrect && !feature->createdFromUnit.empty())
			return CMD_RESURRECT;
		else
			return CMD_RECLAIM;
	}
	return CMD_MOVE;
}

void CBuilderCAI::DrawCommands(void)
{
	float3 pos=owner->midPos;
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glEnable(GL_BLEND);

	if(uh->limitDgun && owner->unitDef->isCommander){
		glColor4f(1,1,1,0.6);
		float3 p=gs->teams[owner->team]->startPos;
		glBegin(GL_LINE_STRIP);
		for(int a=0;a<=40;++a){
			float3 pos2=float3(p.x+sin(a*PI*2/40)*uh->dgunRadius,0,p.z+cos(a*PI*2/40)*uh->dgunRadius);
			pos2.y=ground->GetHeight(pos2.x,pos2.z)+5;
			glVertexf3(pos2);
		}
		glEnd();
	}
	glColor4f(1,1,1,0.4);
	glBegin(GL_LINE_STRIP);
	glVertexf3(pos);
	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		bool draw=false;
		switch(ci->id){
		case CMD_MOVE:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(0.5,1,0.5,0.4);
			draw=true;
			break;
		case CMD_RECLAIM:
		case CMD_RESURRECT:
			if(ci->id==CMD_RECLAIM)
				glColor4f(1,0.2,1,0.4);
			else
				glColor4f(0.2,0.6,1,0.4);

			if(ci->params.size()==4){
				pos=float3(ci->params[0],ci->params[1],ci->params[2]);
				float radius=ci->params[3];
				glVertexf3(pos);
				glEnd();
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=20;++a){
					float3 pos2=float3(pos.x+sin(a*PI*2/20)*radius,0,pos.z+cos(a*PI*2/20)*radius);
					pos2.y=ground->GetHeight(pos2.x,pos2.z)+5;
					glVertexf3(pos2);
				}
				glEnd();
				glBegin(GL_LINE_STRIP);
			} else {
				int id=(int) ci->params[0];
				if(id>=MAX_UNITS){
					if(featureHandler->features[id-MAX_UNITS])
						pos=featureHandler->features[id-MAX_UNITS]->midPos;
				} else {
					if(uh->units[id]!=0 && uh->units[id]!=owner)
						pos=helper->GetUnitErrorPos(uh->units[id],owner->allyteam);
				}
			}
			draw=true;
			break;
		case CMD_PATROL:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(0.5,0.5,1,0.4);
			draw=true;
			break;
		case CMD_REPAIR:
		case CMD_CAPTURE:
			if(ci->id==CMD_REPAIR)
				glColor4f(0.3,1,1,0.4);
			else
				glColor4f(1,1,0.3,0.4);
			if(ci->params.size()==4){
				pos=float3(ci->params[0],ci->params[1],ci->params[2]);
				float radius=ci->params[3];
				glVertexf3(pos);
				glEnd();
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=20;++a){
					float3 pos2=float3(pos.x+sin(a*PI*2/20)*radius,0,pos.z+cos(a*PI*2/20)*radius);
					pos2.y=ground->GetHeight(pos2.x,pos2.z);
					glVertexf3(pos2);
				}
				glEnd();
				glBegin(GL_LINE_STRIP);
			} else {
				int id=(int) ci->params[0];
				if(uh->units[id]!=0)
					pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			}
			draw=true;
			break;
		case CMD_RESTORE:{
			glColor4f(0.3,1,0.5,0.4);
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			float radius=ci->params[3];
			glVertexf3(pos);
			glEnd();
			glBegin(GL_LINE_STRIP);
			for(int a=0;a<=20;++a){
				float3 pos2=float3(pos.x+sin(a*PI*2/20)*radius,0,pos.z+cos(a*PI*2/20)*radius);
				pos2.y=ground->GetHeight(pos2.x,pos2.z);
				glVertexf3(pos2);
			}
			glEnd();
			glBegin(GL_LINE_STRIP);
			draw=true;
			break;}
		case CMD_GUARD:
			if(uh->units[int(ci->params[0])]!=0)
				pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			glColor4f(0.3,0.3,1,0.4);
			draw=true;
			break;
		case CMD_ATTACK:
		case CMD_DGUN:
			if(ci->params.size()==1){
				if(uh->units[int(ci->params[0])]!=0)
					pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			} else {
				pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			}
			glColor4f(1,0.5,0.5,0.4);
			draw=true;
			break;
		}
		map<int,string>::iterator boi;
		if((boi=buildOptions.find(ci->id))!=buildOptions.end()){
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			UnitDef *unitdef = unitDefHandler->GetUnitByName(boi->second);

			glColor4f(1,1,1,0.4);
			glVertexf3(pos);
			glEnd();

			pos=helper->Pos2BuildPos(pos,unitdef);

			if(unitdef->extractRange>0){	//draw range
				glDisable(GL_TEXTURE_2D);
				glColor4f(1,0.3,0.3,0.7);
				glBegin(GL_LINE_STRIP);
				for(int a=0;a<=40;++a){
					float3 wpos(cos(a*2*PI/40)*unitdef->extractRange,0,sin(a*2*PI/40)*unitdef->extractRange);
					wpos+=pos;
					wpos.y=ground->GetHeight(wpos.x,wpos.z)+8;
					glVertexf3(wpos);
				}
				glEnd();
			}

			glEnable(GL_TEXTURE_2D);
			texturehandler->SetTATexture();
			glDepthMask(0);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glColor4f(1,1,1,0.3);
			S3DOModel* model=modelParser->Load3DO(unitdef->model.modelpath.c_str() ,1, owner->team);
			glPushMatrix();
			glTranslatef3(pos);
			//glCallList(model->displist);
			model->DrawStatic();
			glDisable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
			glDepthMask(1);
			glPopMatrix();
			glColor4f(1,1,1,0.4);
			glBegin(GL_LINE_STRIP);
			draw=true;
		}
		if(draw){
			glVertexf3(pos);	
		}
	}
	glEnd();
}

void CBuilderCAI::GiveCommand(Command& c)
{
	if(c.id==CMD_RESURRECT && !owner->unitDef->canResurrect)
		return;

	if(c.id==CMD_CAPTURE && !owner->unitDef->canCapture)
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
		float3 pos(c.params[0],c.params[1],c.params[2]);
		UnitDef *unitdef = unitDefHandler->GetUnitByName(boi->second);
		pos=helper->Pos2BuildPos(pos,unitdef);
		CFeature* feature;
		if(!uh->TestUnitBuildSquare(pos,unitdef,feature)){
			return;
		}
	}
	CMobileCAI::GiveCommand(c);
}


void CBuilderCAI::DrawQuedBuildingSquares(void)
{
	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		map<int,string>::iterator boi;
		if((boi=buildOptions.find(ci->id))!=buildOptions.end()){
			float3 pos=float3(ci->params[0],ci->params[1]+3,ci->params[2]);
			UnitDef *unitdef = unitDefHandler->GetUnitByName(boi->second);

			pos=helper->Pos2BuildPos(pos,unitdef);

			float xsize=unitdef->xsize*4;
			float ysize=unitdef->ysize*4;
			glColor4f(0.2,1,0.2,1);
			glBegin(GL_LINE_STRIP);
			glVertexf3(pos+float3(xsize,1,ysize));
			glVertexf3(pos+float3(-xsize,1,ysize));
			glVertexf3(pos+float3(-xsize,1,-ysize));
			glVertexf3(pos+float3(xsize,1,-ysize));
			glVertexf3(pos+float3(xsize,1,ysize));
			glEnd();
		}
	}
}

bool CBuilderCAI::FindReclaimableFeatureAndReclaim(float3 pos, float radius,unsigned char options)
{
	CFeature* best=0;
	float bestDist=10000000;
	std::vector<CFeature*> features=qf->GetFeaturesExact(pos,radius);
	for(std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
		if((*fi)->def->destructable && (*fi)->allyteam!=owner->allyteam){
			float dist=(*fi)->pos.distance2D(owner->pos);
			if(dist<bestDist){
				bestDist=dist;
				best=*fi;
			}
		}
	}
	if(best){
		Command c2;
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
			if(dist<bestDist){
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

bool CBuilderCAI::FindRepairTargetAndRepair(float3 pos, float radius,unsigned char options,bool attackEnemy)
{
	std::vector<CUnit*> cu=qf->GetUnits(pos,radius);
	int myAllyteam=owner->allyteam;
	for(std::vector<CUnit*>::iterator ui=cu.begin();ui!=cu.end();++ui){
		if(gs->allies[owner->allyteam][(*ui)->allyteam] && (*ui)->health<(*ui)->maxHealth && (*ui)!=owner){
			if((*ui)->mobility && (*ui)->beingBuilt && owner->moveState<2)		//dont help factories produce units unless set on roam
				continue;
			Command nc;
			nc.id=CMD_REPAIR;
			nc.options=options | INTERNAL_ORDER;
			nc.params.push_back((*ui)->id);
			commandQue.push_front(nc);
			return true;
		} else if(attackEnemy && owner->maxRange>0 && !gs->allies[owner->allyteam][(*ui)->allyteam]){
			Command nc;
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
		if(!gs->allies[myAllyteam][(*ui)->allyteam] && (*ui)!=owner && !(*ui)->beingBuilt){
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
