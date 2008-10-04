// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "GroupAI.h"
#include "Sim/Units/UnitDef.h"
#include "System/Util.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}

CGroupAI::CGroupAI()
{
	frontDir=float3(1,0,0);
	sideDir=float3(0,0,1);
	numColumns=5;
	columnDist=64;
	lineDist=64;

	unitsChanged=false;
}

CGroupAI::~CGroupAI()
{

}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();

	CommandDescription cd;

	cd.id=CMD_STOP;
	cd.type=CMDTYPE_ICON;
	cd.name="Stop";
	cd.action="stop";
	commands.push_back(cd);

	cd.id=CMD_MOVE;
	cd.type=CMDTYPE_ICON_FRONT;
	cd.name="Move";
	cd.params.push_back("2000");
	cd.action="move";
	cd.tooltip="Move: Click on the goal and hold mouse button while drawing out a front to form behind";
	char c[40];
	SNPRINTF(c,10,"%d",(int)columnDist);
	cd.params.push_back(c);
	commands.push_back(cd);
};

bool CGroupAI::AddUnit(int unit)
{
	myUnits.insert(unit);
	unitsChanged=true;
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
	myUnits.erase(unit);
	unitsChanged=true;
}

void CGroupAI::GiveCommand(Command* c)
{
	switch(c->id){
	case CMD_STOP:
		for(set<int>::iterator si=myUnits.begin();si!=myUnits.end();++si){
			aicb->GiveOrder(*si,c);
		}
		break;
	case CMD_MOVE:
		MakeFormationMove(c);
		break;
	default:
		aicb->SendTextMsg("Unknown cmd to simple formation ai",0);
	}
}

int CGroupAI::GetDefaultCmd(int unitid)
{
	return CMD_MOVE;
}

void CGroupAI::Update()
{
	if(callback->IsSelected()){
		const Command* c=callback->GetOrderPreview();
		if(c->id==CMD_MOVE && c->params.size()==6){			//draw a preview of how the units will be ordered
			float3 centerPos(c->params[0],c->params[1],c->params[2]);
			float3 rightPos(c->params[3],c->params[4],c->params[5]);

			float frontLength=centerPos.distance(rightPos)*2;
			if(frontLength>80){
				float3 tempSideDir=centerPos-rightPos;
				tempSideDir.y=0;
				tempSideDir.Normalize();
				float3 tempFrontDir=tempSideDir.cross(float3(0,1,0));

				int tempNumColumns=(int)(frontLength/columnDist);

				int posNum=0;
				float rot=GetRotationFromVector(tempFrontDir);
				std::multimap<float,int> orderedUnits;
				CreateUnitOrder(orderedUnits);

				for(multimap<float,int>::iterator oi=orderedUnits.begin();oi!=orderedUnits.end();++oi){
					int lineNum=posNum/tempNumColumns;
					int colNum=posNum-lineNum*tempNumColumns;
					float side=(0.25f+colNum*0.5f)*columnDist*(colNum&1 ? -1:1);

					float3 pos=centerPos-tempFrontDir*(float)lineNum*lineDist+tempSideDir*side;
					pos.y=aicb->GetElevation(pos.x,pos.z);
					const UnitDef* ud=aicb->GetUnitDef(oi->second);
					aicb->DrawUnit(ud->name.c_str(),pos,rot,1,aicb->GetMyTeam(),true,false);
					++posNum;
				}
			}
		}
	}
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	return commands;
}

void CGroupAI::MakeFormationMove(Command* c)
{
	float3 centerPos(c->params[0],c->params[1],c->params[2]);
	float3 rightPos(centerPos);
	if(c->params.size()==6)
		rightPos=float3(c->params[3],c->params[4],c->params[5]);

	float frontLength=centerPos.distance(rightPos)*2;
	if(frontLength>80){
		sideDir=centerPos-rightPos;
		sideDir.y=0;
		sideDir.Normalize();
		frontDir=sideDir.cross(float3(0,1,0));

		numColumns=(int)(frontLength/columnDist);
	}

	int positionsUsed=0;

	std::multimap<float,int> orderedUnits;
	CreateUnitOrder(orderedUnits);

	for(multimap<float,int>::iterator oi=orderedUnits.begin();oi!=orderedUnits.end();++oi){
		MoveToPos(oi->second,centerPos,positionsUsed++,c->options);		
	}
}

void CGroupAI::MoveToPos(int unit, float3& basePos, int posNum,unsigned char options)
{
	int lineNum=posNum/numColumns;
	int colNum=posNum-lineNum*numColumns;
	float side=(0.25f+colNum*0.5f)*columnDist*(colNum&1 ? -1:1);

	float3 pos=basePos-frontDir*(float)lineNum*lineDist+sideDir*side;
	GiveMoveOrder(unit,pos,options);
}

void CGroupAI::GiveMoveOrder(int unit, const float3& pos,unsigned char options)
{
	Command nc;
	nc.id=CMD_MOVE;
	nc.options=options;
	nc.params.push_back(pos.x);
	nc.params.push_back(pos.y);
	nc.params.push_back(pos.z);
	aicb->GiveOrder(unit,&nc);
}


#define PI 3.141592654f
float CGroupAI::GetRotationFromVector(float3 vector)
{
	float h;
	float dx=vector.x;
	float dz=vector.z;
	if(dz!=0){
		float d=dx/dz;
		h=atan(d);
		if(dz<0)
			h+=PI;
	} else {
		if(dx>0)
			h=PI/2;
		else
			h=-PI/2;
	}

	return h;
}

void CGroupAI::CreateUnitOrder(std::multimap<float,int>& out)
{
	for(set<int>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){
		const UnitDef* ud=aicb->GetUnitDef(*ui);
		float range=aicb->GetUnitMaxRange(*ui);
		if(range<1)
			range=2000;		//give weaponless units a long range to make them go to the back
		float value=(ud->metalCost*60+ud->energyCost)/aicb->GetUnitMaxHealth(*ui)*range;
		out.insert(pair<float,int>(value,*ui));
	}
}

