#include "StdAfx.h"
#include "CommandAI.h"
#include "LineDrawer.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "ExternalAI/Group.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Units/UnitDef.h"
#include "Game/GameHelper.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "LogOutput.h"
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
	c.action="stop";
	c.type=CMDTYPE_ICON;
	c.name="Stop";
	c.hotkey="s";
	c.tooltip="Stop: Cancel the units current actions";
	possibleCommands.push_back(c);

	if(owner->unitDef->canAttack){
		c.id=CMD_ATTACK;
		c.action="attack";
		c.type=CMDTYPE_ICON_UNIT_OR_MAP;
		c.name="Attack";
		c.hotkey="a";
		c.tooltip="Attack: Attacks an unit or a position on the ground";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canDGun){
		c.id=CMD_DGUN;
		c.action="dgun";
		c.type=CMDTYPE_ICON_UNIT_OR_MAP;
		c.name="DGun";
		c.hotkey="d";
		c.tooltip="DGun: Attacks using the units special weapon";
		possibleCommands.push_back(c);
	}

 	c.id=CMD_WAIT;
	c.action="wait";
 	c.type=CMDTYPE_ICON;
 	c.name="Wait";
 	c.hotkey="w";
 	c.onlyKey=true;
 	c.tooltip="Wait: Tells the unit to wait until another units handles him";
 	possibleCommands.push_back(c);

	c.id=CMD_SELFD;
	c.action="selfd";
	c.type=CMDTYPE_ICON;
	c.name="SelfD";
	c.hotkey="Ctrl+d";
	c.onlyKey=true;
	c.tooltip="SelfD: Tells the unit to self destruct";
	possibleCommands.push_back(c);

	c.onlyKey=false;
	c.hotkey="";
	nonQueingCommands.insert(CMD_SELFD);

	if(!owner->unitDef->noAutoFire){
		if(!owner->unitDef->weapons.empty() || owner->unitDef->type=="Factory"/* || owner->unitDef->canKamikaze*/){
			c.id=CMD_FIRE_STATE;
			c.action="firestate";
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
	} else{
		owner->fireState=0;
	}

	c.params.clear();
	c.id=CMD_MOVE_STATE;
	c.action="movestate";
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
	c.action="repeat";
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
		c.action="trajectory";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Trajectory";
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
		c.action="onoff";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Active state";
		c.hotkey="x";
		if(owner->unitDef->activateWhenBuilt)
			c.params.push_back("1");
		else
			c.params.push_back("0");

		c.params.push_back("Off");
		c.params.push_back("On");

		c.tooltip="Active State: Sets the active state of the unit to on or off";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_ONOFF);
		c.hotkey="";
	}

	if(owner->unitDef->canCloak)
	{
		c.params.clear();
		c.id=CMD_CLOAK;
		c.action="cloak";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Cloak state";
		c.hotkey="k";
		if(owner->unitDef->startCloaked)
			c.params.push_back("1");
		else
			c.params.push_back("0");

		c.params.push_back("UnCloaked");
		c.params.push_back("Cloaked");

		c.tooltip="Cloak State: Sets wheter the unit is cloaked or not";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_CLOAK);
		c.hotkey="";
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
	switch (c.id) {
		case CMD_SET_WANTED_MAX_SPEED: {
			if (!CanSetMaxSpeed()) {
				return;
			}
			break;
		}
		case CMD_FIRE_STATE: {
			if (c.params.empty() || owner->unitDef->noAutoFire) {
				return;
			}
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
		case CMD_MOVE_STATE: {
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
		case CMD_REPEAT: {
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
		case CMD_TRAJECTORY: {
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
		case CMD_ONOFF: {
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
			return;
		}
		case CMD_CLOAK: {
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
			return;
		}
		case CMD_STOCKPILE: {
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
			return;
		}
		case CMD_SELFD: {
			if(owner->selfDCountdown){
				owner->selfDCountdown=0;
			} else {
				owner->selfDCountdown = owner->unitDef->selfDCountdown*2+1;
			}
			return;
		}
	}

	// bail early, do not check for overlaps or queue cancelling 
	if (c.id == CMD_SET_WANTED_MAX_SPEED) {    
		commandQue.push_back(c);
		if(commandQue.size()==1 && !owner->beingBuilt) {
			SlowUpdate();
		}
		return;
	}
	
	if(!(c.options & SHIFT_KEY)) {
		if(!commandQue.empty()){
			if ((commandQue.front().id == CMD_ATTACK)      ||
			    (commandQue.front().id == CMD_AREA_ATTACK) ||
			    (commandQue.front().id == CMD_DGUN)) {
				owner->AttackUnit(0,true);
			}

			if((c.id==CMD_STOP || c.id==CMD_WAIT) && commandQue.front().id==CMD_WAIT) {
				commandQue.pop_front();
			} else {
				commandQue.clear();
			}
		}
		inCommand=false;
		if(orderTarget){
			DeleteDeathDependence(orderTarget);
			orderTarget=0;
		}
	}

	if(c.id == CMD_PATROL){
		if(!owner->unitDef->canPatrol){
			return;
		}
		std::deque<Command>::iterator ci = commandQue.begin();
		for(; ci != commandQue.end() && ci->id!=CMD_PATROL; ci++);
		if(ci==commandQue.end()){
			if(commandQue.empty()){
				Command c2;
				c2.id = CMD_PATROL;
				c2.params.push_back(owner->pos.x);
				c2.params.push_back(owner->pos.y);
				c2.params.push_back(owner->pos.z);
				c2.options = c.options;
				commandQue.push_back(c2);
			} else {
				do{
					ci--;
					if(ci->params.size() >=3){
						Command c2;
						c2.id = CMD_PATROL;
						c2.params = ci->params;
						c2.options = c.options;
						commandQue.push_back(c2);
						ci=commandQue.begin();
					} else if(ci==commandQue.begin()){
						Command c2;
						c2.id = CMD_PATROL;
						c2.params.push_back(owner->pos.x);
						c2.params.push_back(owner->pos.y);
						c2.params.push_back(owner->pos.z);
						c2.options = c.options;
						commandQue.push_back(c2);
					}
				}while(ci!=commandQue.begin());
			}
		}
	}

	std::deque<Command>::iterator ci = CCommandAI::GetCancelQueued(c);
	if(c.id<0 && ci != commandQue.end()){
		do{
			if(ci == commandQue.begin()){
				commandQue.erase(ci);
				Command c2;
				c2.id = CMD_STOP;
				commandQue.push_front(c2);
				SlowUpdate();
			} else {
				commandQue.erase(ci);
			}
			ci = CCommandAI::GetCancelQueued(c);
		}while(ci != commandQue.end());
		return;
	} else if(ci != commandQue.end()){
		if(ci == commandQue.begin()){
			commandQue.erase(ci);
			Command c2;
			c2.id = CMD_STOP;
			commandQue.push_front(c2);
			SlowUpdate();
		} else {
			commandQue.erase(ci);
		}
		ci = CCommandAI::GetCancelQueued(c);
		return;
	}

	if(!this->GetOverlapQueued(c).empty()){
		return;
	}

	//avoid weaponless units moving to 0 distance when given attack order
	if(c.id==CMD_ATTACK && owner->weapons.empty() && owner->unitDef->canKamikaze==false){
		Command c2;
		c2.id=CMD_STOP;
		commandQue.push_back(c2);
		return;
	}

	commandQue.push_back(c);
	if(commandQue.size()==1 && !owner->beingBuilt)
		SlowUpdate();
}


/**
* @brief Determins if c will cancel a queued command
* @return true if c will cancel a queued command
**/
bool CCommandAI::WillCancelQueued(Command &c)
{
	return this->GetCancelQueued(c) != this->commandQue.end();
}


/**
* @brief Finds the queued command that would be canceled by the Command c
* @return An iterator located at the command, or commandQue.end() if no such queued command exsists
**/
std::deque<Command>::iterator CCommandAI::GetCancelQueued(Command &c){
	if(!commandQue.empty()){
		std::deque<Command>::iterator ci=commandQue.end();
		do{
			--ci;			//iterate from the end and dont check the current order
			if((ci->id==c.id || (c.id<0 && ci->id<0)) && ci->params.size()==c.params.size()){
				if(c.params.size()==1){			//we assume the param is a unit of feature id
					if(ci->params[0]==c.params[0]){
						return ci;
					}
				} else if(c.params.size()>=3){		//we assume this means that the first 3 makes a position
					float3 cpos(c.params[0],c.params[1],c.params[2]);
					float3 cipos(ci->params[0],ci->params[1],ci->params[2]);
					if(c.id < 0){
						UnitDef* u1 = unitDefHandler->GetUnitByID(-c.id);
						UnitDef* u2 = unitDefHandler->GetUnitByID(-ci->id);
						if(u1 && u2
							&& fabs(cpos.x-cipos.x)*2 <= max(u1->xsize, u2->xsize)*SQUARE_SIZE
							&& fabs(cpos.z-cipos.z)*2 <= max(u1->ysize, u2->ysize)*SQUARE_SIZE) {
							return ci;
						}
					} else {
						if((cpos-cipos).SqLength2D()<17*17){
							return ci;
						}
					}
				}
			}
		}while(ci!=commandQue.begin());
	}
	return commandQue.end();
}

/**
* @brief Returns commands that overlap c, but will not be canceled by c
* @return a vector containing commands that overlap c
*/
std::vector<Command> CCommandAI::GetOverlapQueued(Command &c){
	std::deque<Command>::iterator ci = commandQue.end();
	std::vector<Command> v;
	BuildInfo buildInfo(c);

	if(ci != commandQue.begin()){
		do{
			--ci;			//iterate from the end and dont check the current order
			if((ci->id==c.id || (c.id<0 && ci->id<0)) && ci->params.size()==c.params.size()){
				if(c.params.size()==1) //we assume the param is a unit or feature id
				{			
					if(ci->params[0]==c.params[0])
						v.push_back(*ci);
				}
				else if(c.params.size()>=3)		//we assume this means that the first 3 makes a position
				{
					BuildInfo other;

					if(other.Parse(*ci)){
						if(buildInfo.def && other.def
							&& (fabs(buildInfo.pos.x-other.pos.x)*2 > max(buildInfo.GetXSize(), other.GetXSize())*SQUARE_SIZE
							|| fabs(buildInfo.pos.z-other.pos.z)*2 > max(buildInfo.GetYSize(), other.GetYSize())*SQUARE_SIZE)
							&& fabs(buildInfo.pos.x-other.pos.x)*2 < (buildInfo.GetXSize() + other.GetXSize())*SQUARE_SIZE
							&& fabs(buildInfo.pos.z-other.pos.z)*2 < (buildInfo.GetYSize() + other.GetYSize())*SQUARE_SIZE)
						{
							v.push_back(*ci);
						}
					} else {
						if((buildInfo.pos-other.pos).SqLength2D()<17*17)
							v.push_back(*ci);
					}
				}
			}
		}while(ci!=commandQue.begin());
	}
	return v;
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
			if(targetDied || (c.params.size() == 1 && uh->units[int(c.params[0])] && !(uh->units[int(c.params[0])]->losStatus[owner->allyteam] & LOS_INRADAR))){
				FinishCommand();
				break;
			}
			if ((c.params.size() == 3) && (owner->commandShotCount > 0) && (commandQue.size() > 1)) {
				FinishCommand();
				break;
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

int CCommandAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!gs->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack) {
				return CMD_ATTACK;
			}
		}
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
	lineDrawer.StartPath(owner->midPos, cmdColors.start);

	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
			case CMD_ATTACK:
			case CMD_DGUN:{
				if(ci->params.size()==1){
					if(uh->units[int(ci->params[0])]!=0){
						const float3 endPos =
							helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
					}
				} else {
					const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				}
				break;
			}
			default:{
				break;
			}
		}
	}

	lineDrawer.FinishPath();
}

void CCommandAI::FinishCommand(void)
{
	int type=commandQue.front().id;
	if(repeatOrders && type!=CMD_STOP && type != CMD_PATROL && !(commandQue.front().options & INTERNAL_ORDER))
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
	c.action="stockpile";
	c.hotkey="";
	c.type=CMDTYPE_ICON;
	c.name="0/0";
	c.tooltip="Stockpile: Queue up ammunition for later use";
	c.iconname="bitmaps/flare.bmp";
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
			sprintf(name,"%i/%i",stockpileWeapon->numStockpiled,
				stockpileWeapon->numStockpiled+stockpileWeapon->numStockpileQued);
			pci->name=name;
			selectedUnits.PossibleCommandChange(owner);
		}
	}
}

void CCommandAI::WeaponFired(CWeapon* weapon)
{
	if(weapon->weaponDef->manualfire && !weapon->weaponDef->dropped && !commandQue.empty()
		&& (commandQue.front().id==CMD_ATTACK || commandQue.front().id==CMD_DGUN) && inCommand)
	{
		owner->AttackUnit(0,true);
		globalAI->WeaponFired(owner,weapon->weaponDef);
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
