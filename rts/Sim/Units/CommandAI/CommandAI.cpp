#include "StdAfx.h"
#include "CommandAI.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/Weapon.h"
#include "ExternalAI/Group.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Units/UnitDef.h"
#include "Game/UI/InfoConsole.h"
#include "Game/GameHelper.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Game/SelectedUnits.h"
#include "LoadSaveInterface.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "mmgr.h"

CCommandAI::CCommandAI(CUnit* owner)
:	lastUserCommand(-1000),
	orderTarget(0),
	targetDied(false),
	inCommand(false),
	selected(false),
	owner(owner),
	stockpileWeapon(0),
	lastSelectedCommandPage(0),
	repeatOrders(false),
	lastFinishCommand(0),
	unimportantMove(false)
{
	owner->commandAI=this;
	CommandDescription c;
	c.id=CMD_STOP;
	c.type=CMDTYPE_ICON;
	c.name="Stop";
	c.key='S';
	c.tooltip="Stop: Cancel the units current actions";
	possibleCommands.push_back(c);

	c.id=CMD_ATTACK;
	c.type=CMDTYPE_ICON_UNIT_OR_MAP;
	c.name="Attack";
	c.key='A';
	c.tooltip="Attack: Attacks an unit or a position on the ground";
	possibleCommands.push_back(c);

	if(owner->unitDef->canDGun){
		c.id=CMD_DGUN;
		c.type=CMDTYPE_ICON_UNIT_OR_MAP;
		c.name="DGun";
		c.key='D';
		c.tooltip="DGun: Attacks using the units special weapon";
		possibleCommands.push_back(c);
	}

 	c.id=CMD_WAIT;
 	c.type=CMDTYPE_ICON;
 	c.name="Wait";
 	c.key='W';
 	c.onlyKey=true;
 	c.tooltip="Wait: Tells the unit to wait until another units handles him";
 	possibleCommands.push_back(c);
 
	c.id=CMD_SELFD;
	c.type=CMDTYPE_ICON;
	c.name="SelfD";
	c.key='D';
	c.switchKeys=CONTROL_KEY;
	c.onlyKey=true;
	c.tooltip="SelfD: Tells the unit to self destruct";
	possibleCommands.push_back(c);

	c.onlyKey=false;
	c.key=0;
	c.switchKeys=0;
	nonQueingCommands.insert(CMD_SELFD);

	if(!owner->unitDef->weapons.empty() || owner->unitDef->type=="Factory"/* || owner->unitDef->canKamikaze*/){
		c.id=CMD_FIRE_STATE;
		c.type=CMDTYPE_ICON_MODE;
		c.name="Fire state";
		c.params.push_back("2");
		c.params.push_back("Hold fire");
		c.params.push_back("Return fire");
		c.params.push_back("Fire at will");
		c.tooltip="Fire State: Sets under what conditions an\n unit will start to fire at enemy units\n without an explicit attack order";
		possibleCommands.push_back(c);	
		nonQueingCommands.insert(CMD_FIRE_STATE);
	}

	c.params.clear();
	c.id=CMD_MOVE_STATE;
	c.type=CMDTYPE_ICON_MODE;
	c.name="Move state";
	c.params.push_back("1");
	c.params.push_back("Hold pos");
	c.params.push_back("Maneuver");
	c.params.push_back("Roam");
	c.tooltip="Move State: Sets how far out of its way\n an unit will move to attack enemies";
	possibleCommands.push_back(c);
	owner->moveState=1;
	nonQueingCommands.insert(CMD_MOVE_STATE);

	c.params.clear();
	c.id=CMD_REPEAT;
	c.type=CMDTYPE_ICON_MODE;
	c.name="Repeat";
	c.params.push_back("0");
	c.params.push_back("Repeat off");
	c.params.push_back("Repeat on");
	c.tooltip="Repeat: If on the unit will continously\n push finished orders to the end of its\n order que";
	possibleCommands.push_back(c);
	nonQueingCommands.insert(CMD_REPEAT);

	if(owner->unitDef->highTrajectoryType>1){
		c.params.clear();
		c.id=CMD_TRAJECTORY;
		c.type=CMDTYPE_ICON_MODE;
		c.name="Move state";
		c.params.push_back("0");
		c.params.push_back("Low traj");
		c.params.push_back("High traj");
		c.tooltip="Trajectory: If set to high, weapons that\n support it will try to fire in a higher\n trajectory than usual (experimental)";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_TRAJECTORY);
	}

	if(owner->unitDef->onoffable)
	{
		c.params.clear();
		c.id=CMD_ONOFF;
		c.type=CMDTYPE_ICON_MODE;
		c.name="Active state";
		c.key='X';
		if(owner->unitDef->activateWhenBuilt)
			c.params.push_back("1");
		else
			c.params.push_back("0");

		c.params.push_back("Off");
		c.params.push_back("On");

		c.tooltip="Active State: Sets the active state of the unit to on or off";
		possibleCommands.push_back(c);	
		nonQueingCommands.insert(CMD_ONOFF);
		c.key=0;
	}

	if(owner->unitDef->canCloak)
	{
		c.params.clear();
		c.id=CMD_CLOAK;
		c.type=CMDTYPE_ICON_MODE;
		c.name="Cloak state";
		c.key='K';
		if(owner->unitDef->startCloaked)
			c.params.push_back("1");
		else
			c.params.push_back("0");

		c.params.push_back("UnCloaked");
		c.params.push_back("Cloaked");

		c.tooltip="Cloak State: Sets wheter the unit is cloaked or not";
		possibleCommands.push_back(c);	
		nonQueingCommands.insert(CMD_CLOAK);
		c.key=0;
	}
}

CCommandAI::~CCommandAI()
{
	if(orderTarget){
		DeleteDeathDependence(orderTarget);
		orderTarget=0;
	}
}

vector<CommandDescription>& CCommandAI::GetPossibleCommands()
{
	return possibleCommands;
}

void CCommandAI::GiveCommand(Command& c)
{
	switch(c.id)
	{
	case CMD_FIRE_STATE:
		{
			if(c.params.empty())
				return;
			owner->fireState=(int)c.params[0];
			for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
				if(cdi->id==CMD_FIRE_STATE){
					char t[10];
					SNPRINTF(t, 10, "%d", (int)c.params[0]);
					cdi->params[0]=t;
					break;
				}
			}
			return;
		}
	case CMD_MOVE_STATE:
		{
			if(c.params.empty())
				return;
			owner->moveState=(int)c.params[0];
			for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
				if(cdi->id==CMD_MOVE_STATE){
					char t[10];
					SNPRINTF(t, 10, "%d", (int)c.params[0]);
					cdi->params[0]=t;
					break;
				}
			}
			return;
		}
	case CMD_REPEAT:
		{
			if(c.params.empty())
				return;
			repeatOrders=!!c.params[0];
			for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
				if(cdi->id==CMD_REPEAT){
					char t[10];
					SNPRINTF(t, 10, "%d", (int)c.params[0]);
					cdi->params[0]=t;
					break;
				}
			}
			return;
		}
	case CMD_TRAJECTORY:
		{
			if(c.params.empty() || owner->unitDef->highTrajectoryType<2)
				return;
			owner->useHighTrajectory=!!c.params[0];
			for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
				if(cdi->id==CMD_TRAJECTORY){
					char t[10];
					SNPRINTF(t, 10, "%d", (int)c.params[0]);
					cdi->params[0]=t;
					break;
				}
			}
			return;
		}
	case CMD_ONOFF:{
		if(c.params.empty() || !owner->unitDef->onoffable || owner->beingBuilt)
			return;
		if(c.params[0]==1){
			owner->Activate();
		} else if(c.params[0]==0) {
			owner->Deactivate();
		}
		for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
			if(cdi->id==CMD_ONOFF){
				char t[10];
				SNPRINTF(t, 10, "%d", (int)c.params[0]);
				cdi->params[0]=t;
				break;
			}
		}
		return; }
	case CMD_CLOAK:{
		if(c.params.empty() || !owner->unitDef->canCloak)
			return;
		if(c.params[0]==1){
			owner->wantCloak=true;
		} else if(c.params[0]==0) {
			owner->wantCloak=false;
			owner->curCloakTimeout=gs->frameNum+owner->cloakTimeout;
		}
		for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
			if(cdi->id==CMD_CLOAK){
				char t[10];
				SNPRINTF(t, 10, "%d", (int)c.params[0]);
				cdi->params[0]=t;
				break;
			}
		}
		return; }
	case CMD_STOCKPILE:{
		if(!stockpileWeapon)
			return;
		int change=1;
		if(c.options & RIGHT_MOUSE_KEY)
			change*=-1;
		if(c.options & CONTROL_KEY)
			change*=20;
		if(c.options & SHIFT_KEY)
			change*=5;
		stockpileWeapon->numStockpileQued+=change;
		if(stockpileWeapon->numStockpileQued<0)
			stockpileWeapon->numStockpileQued=0;
		UpdateStockpileIcon();
		return; }
	case CMD_SELFD:
		if(owner->selfDCountdown){
			owner->selfDCountdown=0;
		} else {
			owner->selfDCountdown = owner->unitDef->selfDCountdown*2+1;
		}
		return;
	}

	if(!(c.options & SHIFT_KEY)){
		if(!commandQue.empty()){
			if(commandQue.front().id==CMD_ATTACK || commandQue.front().id==CMD_AREA_ATTACK || commandQue.front().id==CMD_DGUN){
				owner->AttackUnit(0,true);
			}

			if((c.id==CMD_STOP || c.id==CMD_WAIT) && commandQue.front().id==CMD_WAIT)
 				commandQue.pop_front();
 			else
				commandQue.clear();
 		}
		inCommand=false;
		if(orderTarget){
			DeleteDeathDependence(orderTarget);
			orderTarget=0;
		}
	}
	if(!commandQue.empty()){
		std::deque<Command>::iterator ci=commandQue.end();
		for(--ci;ci!=commandQue.begin();--ci){							//iterate from the end and dont check the current order
			if((ci->id==c.id || (c.id<0 && ci->id<0)) && ci->params.size()==c.params.size()){
				if(c.params.size()==1){			//we assume the param is a unit of feature id
					if(ci->params[0]==c.params[0]){
						commandQue.erase(ci);
						return;
					}
				} else if(c.params.size()>=3){		//we assume this means that the first 3 makes a position
					float3 cpos(c.params[0],c.params[1],c.params[2]);
					float3 cipos(ci->params[0],ci->params[1],ci->params[2]);

					if((cpos-cipos).SqLength2D()<17*17){
						commandQue.erase(ci);
						return;				
					}
				}
			}
		}
	}
	if(c.id==CMD_ATTACK && owner->weapons.empty() && owner->unitDef->canKamikaze==false)		//avoid weaponless units moving to 0 distance when given attack order
		c.id=CMD_STOP;

	commandQue.push_back(c);
	if(commandQue.size()==1 && !owner->beingBuilt)
		SlowUpdate();
}

void CCommandAI::SlowUpdate()
{
	if(commandQue.empty())
		return;
	
	Command& c=commandQue.front();
	
	switch(c.id){
	case CMD_WAIT:
		break;
	case CMD_STOP:{
		owner->AttackUnit(0,true);
		std::vector<CWeapon*>::iterator wi;
		for(wi=owner->weapons.begin();wi!=owner->weapons.end();++wi)
			(*wi)->HoldFire();			
		FinishCommand();
		break;};
	case CMD_ATTACK:
	case CMD_DGUN:
		if(inCommand){
			if(targetDied){
				FinishCommand();
			}
		} else {
			if(c.params.size()==1){
				if(uh->units[int(c.params[0])]!=0 && uh->units[int(c.params[0])]!=owner){
					owner->AttackUnit(uh->units[int(c.params[0])], c.id==CMD_DGUN);
					if(orderTarget)
						DeleteDeathDependence(orderTarget);
					orderTarget=uh->units[int(c.params[0])];
					AddDeathDependence(orderTarget);
					inCommand=true;
				} else {
					FinishCommand();
				}
			} else {
				float3 pos(c.params[0],c.params[1],c.params[2]);
				owner->AttackGround(pos, c.id==CMD_DGUN);
				inCommand=true;
			}
		}
		break;
	default:
		FinishCommand();
		break;
	}
}

int CCommandAI::GetDefaultCmd(CUnit *pointed,CFeature* feature)
{
	if(pointed){
		if(!gs->Ally(gu->myAllyTeam,pointed->allyteam))
			return CMD_ATTACK;
	}
	return CMD_STOP;
}

void CCommandAI::DependentDied(CObject *o)
{
	if(o==orderTarget){
		targetDied=true;
		orderTarget=0;
	}
}

void CCommandAI::DrawCommands(void)
{
	float3 pos=owner->midPos;
	glColor4f(1,1,1,0.4);
	glBegin(GL_LINE_STRIP);
	glVertexf3(pos);
	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		bool draw=false;
		switch(ci->id){
		case CMD_ATTACK:
		case CMD_DGUN:
			if(ci->params.size()==1){
				if(uh->units[int(ci->params[0])]!=0)
					pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			} else {
				pos=float3(ci->params[0],ci->params[1]+3,ci->params[2]);
			}
			glColor4f(1,0.5,0.5,0.4);
			draw=true;
			break;
		default:
			break;
		}
		if(draw){
			glVertexf3(pos);	
		}
	}
	glEnd();
}

void CCommandAI::FinishCommand(void)
{
	int type=commandQue.front().id;
	if(repeatOrders && type!=CMD_STOP && !(commandQue.front().options & INTERNAL_ORDER))
		commandQue.push_back(commandQue.front());
	commandQue.pop_front();
	inCommand=false;
	targetDied=false;
	unimportantMove=false;
	orderTarget=0;
	if(owner->group)
		owner->group->CommandFinished(owner->id,type);
	
	if(!owner->group && commandQue.empty())
		globalAI->UnitIdle(owner);

	if(lastFinishCommand!=gs->frameNum){	//avoid infinite loops
		lastFinishCommand=gs->frameNum;
		SlowUpdate();			
	}
}

void CCommandAI::AddStockpileWeapon(CWeapon* weapon)
{
	stockpileWeapon=weapon;
	CommandDescription c;
	c.id=CMD_STOCKPILE;
	c.type=CMDTYPE_ICON;
	c.name="0/0";
	c.iconname="bitmaps/flare.bmp";
	c.tooltip="Stockpile: Que up ammunition for later use";
	possibleCommands.push_back(c);
}

void CCommandAI::StockpileChanged(CWeapon* weapon)
{
	UpdateStockpileIcon();
}

void CCommandAI::UpdateStockpileIcon(void)
{
	vector<CommandDescription>::iterator pci;
	for(pci=possibleCommands.begin();pci!=possibleCommands.end();++pci){
		if(pci->id==CMD_STOCKPILE){
			char name[50];
			sprintf(name,"%i/%i",stockpileWeapon->numStockpiled,stockpileWeapon->numStockpiled+stockpileWeapon->numStockpileQued);
			pci->name=name;
			selectedUnits.PossibleCommandChange(owner);
		}
	}
}

void CCommandAI::WeaponFired(CWeapon* weapon)
{
	if(weapon->weaponDef->manualfire && !weapon->weaponDef->dropped && !commandQue.empty() && (commandQue.front().id==CMD_ATTACK || commandQue.front().id==CMD_DGUN) && inCommand){
		owner->AttackUnit(0,true);
		FinishCommand();
	}
}

void CCommandAI::BuggerOff(float3 pos, float radius)
{
}

void CCommandAI::LoadSave(CLoadSaveInterface* file, bool loading)
{
	int queSize=commandQue.size();
	file->lsInt(queSize);
	for(int a=0;a<queSize;++a){
		if(loading){
			commandQue.push_back(Command());
		}
		Command& c=commandQue[a];
		file->lsInt(c.id);
		file->lsUChar(c.options);
		file->lsInt(c.timeOut);

		int paramSize=c.params.size();
		file->lsInt(paramSize);
		for(int b=0;b<paramSize;++b){
			if(loading){
				c.params.push_back(0);
			}
			file->lsFloat(c.params[b]);
		}
	}
}
