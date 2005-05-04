// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "GroupAI.h"
#include <stdarg.h>	
#include "igroupaicallback.h"
#include "unitdef.h"
#include ".\groupai.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}

CGroupAI::CGroupAI()
{
	CommandDescription cd;

	unitsChanged=false;
	nextBuildingId=1;
}

CGroupAI::~CGroupAI()
{
	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		delete ui->second;
	myUnits.clear();

	for(map<int,QuedBuilding*>::iterator qbi=quedBuildngs.begin();qbi!=quedBuildngs.end();++qbi)
		delete qbi->second;
	quedBuildngs.clear();

	for(map<int,BuildOption*>::iterator boi=buildOptions.begin();boi!=buildOptions.end();++boi)
		delete boi->second;
	buildOptions.clear();
}

void CGroupAI::InitAi(IGroupAiCallback* callback)
{
	this->callback=callback;
	UpdateAvailableCommands();
};

bool CGroupAI::AddUnit(int unit)
{
	const UnitDef* ud=callback->GetUnitDef(unit);
	if(ud->buildSpeed==0){								//can only use builder units
		callback->SendTextMsg("Cant use non builders",0);
		return false;
	}

	UnitInfo* info=new UnitInfo;
	info->lastGivenOrder=0;
	info->buildSpeed=ud->buildSpeed;
	info->totalGuardSpeed=0;
	info->moveSpeed=callback->GetUnitSpeed(unit);

	const vector<CommandDescription>* cd=callback->GetUnitCommands(unit);
	for(vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi){		//check if this unit brings some new build options
		if(cdi->id<0){			//id<0 = build option
			info->possibleBuildOrders.insert(cdi->id);
			info->orderedBuildOrders.push_back(cdi->id);
			if(buildOptions.find(cdi->id)==buildOptions.end()){				//if we havent seen it before then add it
				BuildOption* bo=new BuildOption;
				bo->type=cdi->type;
				bo->name=cdi->name;
				bo->numQued=0;
				const UnitDef* ud=callback->GetUnitDef(bo->name.c_str());
				bo->buildTime=ud->buildTime;
				if(bo->buildTime==0){
					char c[6000];
					sprintf(c,"Zero build time unit? %f %s %s",ud->buildTime,ud->name.c_str(),bo->name.c_str());
					callback->SendTextMsg(c,0);
					bo->buildTime=1;
				}
				buildOptions[cdi->id]=bo;
			}
		}	
	}
	myUnits[unit]=info;
	unitsChanged=true;
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
	UnitInfo* info=myUnits[unit];
	if(info->lastGivenOrder){
		if(info->lastGivenOrder>0){			//set to build something
			QuedBuilding* qb=quedBuildngs[info->lastGivenOrder];
			qb->unitsOnThis.erase(unit);
			qb->totalBuildSpeed-=info->buildSpeed+info->totalGuardSpeed;
		} else {												//set to guard something
			UnitInfo* guardInfo=myUnits[-info->lastGivenOrder];
			guardInfo->unitsGuardingMe.erase(unit);
			guardInfo->totalGuardSpeed-=info->buildSpeed;
		}
	}
	for(std::set<int>::iterator gi=info->unitsGuardingMe.begin();gi!=info->unitsGuardingMe.end();++gi){
		UnitInfo* guardInfo=myUnits[*gi];
		guardInfo->lastGivenOrder=0;		
	}
	info->unitsGuardingMe.clear();
	delete myUnits[unit];
	myUnits.erase(unit);
	unitsChanged=true;
}

void CGroupAI::GiveCommand(Command* c)
{
	if(c->id==CMD_STOP)
		return;

	for(std::vector<CommandDescription>::iterator cdi=commands.begin();cdi!=commands.end();++cdi){
		if(cdi->id==c->id){
			if(cdi->type==CMDTYPE_ICON){			//factory build unit
				int prevNum=buildOptions[c->id]->numQued;
				int numItems=1;
				if(c->options& SHIFT_KEY)
					numItems*=5;
				if(c->options & CONTROL_KEY)
					numItems*=20;

				if(c->options & RIGHT_MOUSE_KEY){
					buildOptions[c->id]->numQued=max(0,buildOptions[c->id]->numQued-numItems);
				} else {
					buildOptions[c->id]->numQued+=numItems;
				}
				UpdateFactoryIcon(&*cdi,buildOptions[c->id]->numQued);
				callback->UpdateIcons();
			} else {													//built on map
				float3 pos(c->params[0],c->params[1],c->params[2]);
				int close=FindCloseQuedBuilding(pos,15);
				if(close>=0){
					QuedBuilding* qb=quedBuildngs[close];
					if(!qb->unitsOnThis.empty()){
						FinishBuilderTask(*qb->unitsOnThis.begin(),false);
					}
					delete qb;
					quedBuildngs.erase(close);
					return;
				}
				QuedBuilding* qb=new QuedBuilding;
				qb->type=c->id;
				qb->pos=pos;
				qb->buildTimeLeft=buildOptions[c->id]->buildTime;
				qb->totalBuildSpeed=0;
				qb->failedTries=0;
				qb->startFrame=frameNum;
				quedBuildngs[nextBuildingId++]=qb;
			}
		}
	}
}

void CGroupAI::CommandFinished(int unit,int type)
{
	UnitInfo* info=myUnits[unit];
	bool isFactory=info->moveSpeed==0;
	if(isFactory){
		for(std::set<int>::iterator gi=info->unitsGuardingMe.begin();gi!=info->unitsGuardingMe.end();++gi){
			UnitInfo* guardInfo=myUnits[*gi];
			guardInfo->lastGivenOrder=0;
		}
		info->unitsGuardingMe.clear();
		info->totalGuardSpeed=0;
		info->lastGivenOrder=0;
		FindNewJob(unit);
	} else {
		if(info->lastGivenOrder>0 && type==quedBuildngs[info->lastGivenOrder]->type){
//			callback->SendTextMsg("Command finsihed for builder",0);

			QuedBuilding* qb=quedBuildngs[info->lastGivenOrder];
			qb->unitsOnThis.erase(unit);
			qb->totalBuildSpeed-=info->buildSpeed+info->totalGuardSpeed;
			
			bool found=false;
			int foundUnits[1000];
			int num=callback->GetFriendlyUnits(foundUnits,qb->pos,10);
			for(int a=0;a<num;++a){
				const UnitDef* ud=callback->GetUnitDef(foundUnits[a]);
				if(ud->id==-qb->type){			//ok found the right unit
//					callback->SendTextMsg("Building finished ok",0);
					
					delete qb;
					quedBuildngs.erase(info->lastGivenOrder);
					found=true;
					break;
				}
			}
			if(!found){
				qb->failedTries++;
				if(qb->failedTries>3){
					callback->SendTextMsg("Building failed",0);
					delete qb;
					quedBuildngs.erase(info->lastGivenOrder);
				}
			}

			for(std::set<int>::iterator gi=info->unitsGuardingMe.begin();gi!=info->unitsGuardingMe.end();++gi){
				UnitInfo* guardInfo=myUnits[*gi];
				guardInfo->lastGivenOrder=0;		
			}
			info->unitsGuardingMe.clear();
			info->totalGuardSpeed=0;
			info->lastGivenOrder=0;
			FindNewJob(unit);
		}
	}
}

int CGroupAI::GetDefaultCmd(int unitid)
{
	return CMD_STOP;
}

void CGroupAI::Update()
{
	frameNum=callback->GetCurrentFrame();
	if(unitsChanged){
		UpdateAvailableCommands();
		updateUnit=myUnits.begin();
		unitsChanged=false;
	}

	if(!myUnits.empty()){
		FindNewJob(updateUnit->first);

		++updateUnit;
		if(updateUnit==myUnits.end())
			updateUnit=myUnits.begin();
	}

	if(!(frameNum & 3) && callback->IsSelected()){
		int team=callback->GetMyTeam();
		for(map<int,QuedBuilding*>::iterator qbi=quedBuildngs.begin();qbi!=quedBuildngs.end();++qbi){
			callback->DrawUnit(buildOptions[qbi->second->type]->name.c_str(),qbi->second->pos,0,4,team,true,true);
		}

		for(map<int,QuedBuilding*>::iterator qbi=quedBuildngs.begin();qbi!=quedBuildngs.end();++qbi){
			std::set<int>* uot=&qbi->second->unitsOnThis;
			for(std::set<int>::iterator ui=uot->begin();ui!=uot->end();++ui){
				callback->CreateLineFigure(qbi->second->pos+float3(0,10,0),callback->GetUnitPos(*ui)+float3(0,10,0),3,1,4,0);
			}
		}/**/
	}
}

void CGroupAI::UpdateAvailableCommands(void)
{
	commands.clear();

	CommandDescription cd;
	cd.id=CMD_STOP;
	cd.name="Stop";
	commands.push_back(cd);			//should always have a stop command since it is the default

	set<int> alreadyFound;
	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){
		for(vector<int>::iterator oi=ui->second->orderedBuildOrders.begin();oi!=ui->second->orderedBuildOrders.end();++oi){
			if(alreadyFound.find(*oi)==alreadyFound.end()){		//skip if this command has already been added
				CommandDescription cd;
				cd.id=*oi;
				cd.name=buildOptions[*oi]->name;
				cd.type=buildOptions[*oi]->type;
				if(cd.type==CMDTYPE_ICON){
					UpdateFactoryIcon(&cd,buildOptions[*oi]->numQued);
				}
				commands.push_back(cd);
				alreadyFound.insert(*oi);
			}
		}
	}
	callback->UpdateIcons();
}

void CGroupAI::UpdateFactoryIcon(CommandDescription* cd, int numQued)
{
	cd->params.clear();
	if(numQued){
		char t[32];
		itoa(numQued,t,10);
		cd->params.push_back(t);
	}
}

void CGroupAI::FindNewJob(int unit)
{
	UnitInfo* info=myUnits[unit];

	bool isFactory=info->moveSpeed==0;
	const deque<Command>* curCommands=callback->GetCurrentUnitCommands(unit);

	if(!isFactory && info->lastGivenOrder && !curCommands->empty()){
		if(info->lastGivenOrder>0 && curCommands->front().id!=CMD_GUARD){
			QuedBuilding* qb=quedBuildngs[info->lastGivenOrder];
			int foundUnits[1000];
			int num=callback->GetFriendlyUnits(foundUnits,qb->pos,10);
			for(int a=0;a<num;++a){
				const UnitDef* ud=callback->GetUnitDef(foundUnits[a]);
				if(ud->id==-qb->type){			//ok found the right unit
					float health=callback->GetUnitHealth(foundUnits[a]);
					float maxHealth=callback->GetUnitMaxHealth(foundUnits[a]);
					qb->buildTimeLeft=buildOptions[qb->type]->buildTime*((maxHealth-health)/maxHealth);		//we have no direct access to the current build status of a unit so we assumes that the health is a good indicator
					break;
				}
			}
			qb->buildTimeLeft=buildOptions[qb->type]->buildTime;		//didnt find a unit so assume we havent started building
			return;
		}
		if(info->lastGivenOrder<0)
			return;	
	}

	if(isFactory && curCommands->size()){
		return;
	}

	if(info->lastGivenOrder){
//		SendTxt("Free unit with order %i %i",unit,info->lastGivenOrder);
		if(info->lastGivenOrder>0){
			FinishBuilderTask(unit,true);
		} else {
			SendTxt("Free unit with guard order? %i %i",unit,info->lastGivenOrder);
			if(myUnits.find(-info->lastGivenOrder)!=myUnits.end()){
				myUnits[-info->lastGivenOrder]->unitsGuardingMe.erase(unit);
			}
			info->lastGivenOrder=0;
		}
	}

	float bestValue=0;
	int bestJob=0;

	if(!isFactory){		//mobile builder
		float3 myPos=callback->GetUnitPos(unit);
		for(map<int,QuedBuilding*>::iterator qbi=quedBuildngs.begin();qbi!=quedBuildngs.end();++qbi){		//check for building to build
			QuedBuilding* qb=qbi->second;
			bool canBuildThis=info->possibleBuildOrders.find(qb->type)!=info->possibleBuildOrders.end();
			if(canBuildThis || !qb->unitsOnThis.empty()){			//can we build this directly or is there someone who can start if for us
				float buildTime=qb->buildTimeLeft/(qb->totalBuildSpeed+info->buildSpeed);
				float moveTime=max(0.01f,((qb->pos-myPos).Length()-150)/info->moveSpeed*10);
				float travelMod=buildTime/(buildTime+moveTime);			//units prefer stuff with low travel time compared to build time
				float finishMod=buildOptions[qb->type]->buildTime/(qb->buildTimeLeft+buildOptions[qb->type]->buildTime*0.1f);			//units prefer stuff that is nearly finished
				float canBuildThisMod=canBuildThis?1.5f:1;								//units prefer to do stuff they have in their build options (less risk of guarded unit dying etc)
				float ageMod=20+sqrtf((float)(frameNum+1)-qb->startFrame);
//				float buildSpeedMod=info->buildSpeed/(qb->totalBuildSpeed+info->buildSpeed);
				float value=finishMod*canBuildThisMod*travelMod*ageMod;
/*				char c[6000];
				sprintf(c,"value %f %f",moveTime,buildTime);
				callback->SendTextMsg(c,0);
*/				if(value>bestValue){
					bestValue=value;
					if(!qb->unitsOnThis.empty())
						bestJob=-*qb->unitsOnThis.begin();				//negative number=pick a unit to guard
					else
						bestJob=qbi->first;				//positive number=do this build project
				}
			}
		}
		for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find factories to guard
			if(ui->second->moveSpeed==0 && !callback->GetCurrentUnitCommands(ui->first)->empty()){
				float moveTime=max(1.0f,((callback->GetUnitPos(ui->first)-myPos).Length()-150)/info->moveSpeed*2);
				float value=3.0f*(ui->second->buildSpeed/(ui->second->totalGuardSpeed+ui->second->buildSpeed))/moveTime;
				if(value>bestValue && ui->second->unitsGuardingMe.size()<5){
					bestValue=value;
					bestJob=-ui->first;
				}
			}
		}
		if(bestJob){										//we have found something to do
//			SendTxt("Best job found %i %i %.2f",unit,bestJob,bestValue);
			info->lastGivenOrder=bestJob;
			Command c;
			if(bestJob>0){		//build building
				QuedBuilding* qb=quedBuildngs[bestJob];
				c.id=qb->type;
				c.params.push_back(qb->pos.x);
				c.params.push_back(qb->pos.y);
				c.params.push_back(qb->pos.z);

				qb->totalBuildSpeed+=info->buildSpeed;
				qb->unitsOnThis.insert(unit);
			} else {		//guard unit
				c.id=CMD_GUARD;
				c.params.push_back((float)-bestJob);

				UnitInfo* guardInfo=myUnits[-bestJob];
				guardInfo->unitsGuardingMe.insert(unit);
				guardInfo->totalGuardSpeed+=info->buildSpeed;

				if(guardInfo->moveSpeed!=0){
					QuedBuilding* qb=quedBuildngs[guardInfo->lastGivenOrder];
					qb->totalBuildSpeed+=info->buildSpeed;
				}
			}
			callback->GiveOrder(unit,&c);
		}
	} else {									//factory
		CommandDescription* bestCommandDescription;
	
		for(std::vector<CommandDescription>::iterator cdi=commands.begin();cdi!=commands.end();++cdi){
			if(cdi->type==CMDTYPE_ICON && info->possibleBuildOrders.find(cdi->id)!=info->possibleBuildOrders.end()){		//we can build this stuff
				float value=(float)rand()*buildOptions[cdi->id]->numQued;				//the probability to pick a certain unit to build is linear to the number of qued, maybe use some better choose method ?
				if(value>bestValue){
					bestValue=value;
					bestCommandDescription=&*cdi;
					bestJob=cdi->id;
				}
			}
		}

		if(bestJob){
			Command c;
			c.id=bestJob;
			c.options=0;
			callback->GiveOrder(unit,&c);

//			info->lastGivenOrder=bestJob;
			UpdateFactoryIcon(bestCommandDescription,--buildOptions[bestJob]->numQued);
			callback->UpdateIcons();
		}
	}
}

void CGroupAI::SendTxt(const char *fmt, ...)
{
	char text[500];
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	callback->SendTextMsg(text,0);
}

int CGroupAI::FindCloseQuedBuilding(float3 pos, float radius)
{
	for(map<int,QuedBuilding*>::iterator qbi=quedBuildngs.begin();qbi!=quedBuildngs.end();++qbi){
		if(qbi->second->pos.distance2D(pos)<radius)
			return qbi->first;
	}
	return -1;
}

void CGroupAI::FinishBuilderTask(int unit,bool failure)
{
	UnitInfo* info=myUnits[unit];
	if(quedBuildngs.find(info->lastGivenOrder)!=quedBuildngs.end()){
		QuedBuilding* qb=quedBuildngs[info->lastGivenOrder];
		qb->unitsOnThis.erase(unit);
		qb->totalBuildSpeed-=info->buildSpeed+info->totalGuardSpeed;
		if(failure){
			qb->failedTries++;
			qb->startFrame=frameNum;
			if(qb->failedTries>1){
				callback->SendTextMsg("Building failed",0);
				delete qb;
				quedBuildngs.erase(info->lastGivenOrder);
			}
		}
	}
	for(std::set<int>::iterator gi=info->unitsGuardingMe.begin();gi!=info->unitsGuardingMe.end();++gi){
		UnitInfo* guardInfo=myUnits[*gi];
		guardInfo->lastGivenOrder=0;		
	}
	info->unitsGuardingMe.clear();
	info->totalGuardSpeed=0;
	info->lastGivenOrder=0;
	Command c;
	c.id=CMD_STOP;
	callback->GiveOrder(unit,&c);
}
