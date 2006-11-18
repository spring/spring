#include "StdAfx.h"
// Factory.cpp: implementation of the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "Factory.h"
#include "Game/Team.h"
#include "Sim/Units/UnitLoader.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include "Game/GameHelper.h"
#include "Game/Camera.h"
#include "Game/WaitCommandsAI.h"
#include "Game/UI/GuiHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Projectiles/GfxProjectile.h"
#include "Sim/Units/UnitHandler.h"
#include "Matrix44f.h"
#include "myMath.h"
#include "Sim/Units/COB/CobFile.h"
#include "SyncTracer.h"
#include "Map/ReadMap.h"
#include "Sound.h"
#include "Sim/Misc/QuadField.h"
#include "Map/Ground.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Game/Team.h"
#include "mmgr.h"

CR_BIND_DERIVED(CFactory, CBuilding);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFactory::CFactory()
:	buildSpeed(100),
	curBuild(0),
	quedBuild(false),
	lastBuild(-1000),
	opening(false)
{
}

CFactory::~CFactory()
{
	if(curBuild){
		curBuild->KillUnit(false,true,0);
		curBuild=0;
	}
}

void CFactory::UnitInit (UnitDef* def, int team, const float3& position)
{
	buildSpeed=def->buildSpeed/32.0f;
	CBuilding::UnitInit (def, team, position);
}

int CFactory::GetBuildPiece()
{
	std::vector<int> args;
	args.push_back(0);
	cob->Call("QueryBuildInfo",args);
	return args[0];
}

// GetBuildPiece() is called if piece < 0
float3 CFactory::CalcBuildPos(int buildPiece)
{
	float3 relBuildPos=localmodel->GetPiecePos(buildPiece < 0 ? GetBuildPiece() : buildPiece);
	float3 buildPos=pos + frontdir*relBuildPos.z + updir*relBuildPos.y + rightdir*relBuildPos.x;
	return buildPos;
}

void CFactory::Update()
{
	if(beingBuilt){
		CUnit::Update();
		return;
	}

	if(quedBuild && inBuildStance){
		float3 buildPos = CalcBuildPos();

		bool canBuild=true;
		std::vector<CUnit*> units=qf->GetUnitsExact(buildPos,16);
		for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
			if((*ui)!=this)
				canBuild=false;
		}
		if(canBuild){
			quedBuild=false;
			CUnit* b=unitLoader.LoadUnit(nextBuild,buildPos+float3(0.01f,0.01f,0.01f),team,true,buildFacing);
			AddDeathDependence(b);
			curBuild=b;

			cob->Call("StartBuilding");

			if(unitDef->sounds.build.id)
				sound->PlaySample(unitDef->sounds.build.id, pos, unitDef->sounds.build.volume);
		} else {
			helper->BuggerOff(buildPos-float3(0.01f,0,0.02f),radius+8);
		}
	}

	if(curBuild && !beingBuilt){
		lastBuild=gs->frameNum;

		int buildPiece = GetBuildPiece();
		CMatrix44f mat=localmodel->GetPieceMatrix(buildPiece);
		int h=GetHeadingFromVector(mat[2],mat[10]);
		curBuild->heading = (h + buildFacing*16*1024) & 65535;

		float3 buildPos = curBuild->pos = CalcBuildPos(buildPiece);
		if(curBuild->floatOnWater)
			curBuild->pos.y=ground->GetHeight(buildPos.x,buildPos.z)-curBuild->unitDef->waterline;
		curBuild->midPos=curBuild->pos+UpVector*curBuild->relMidPos.y;

		if(curBuild->AddBuildPower(buildSpeed,this)){
			std::vector<int> args;
			args.push_back(0);
			cob->Call("QueryNanoPiece",args);

			if(unitDef->showNanoSpray){
				float3 relWeaponFirePos=localmodel->GetPiecePos(args[0]);
				float3 weaponPos=pos + frontdir*relWeaponFirePos.z + updir*relWeaponFirePos.y + rightdir*relWeaponFirePos.x;

				float3 dif=curBuild->midPos-weaponPos;
				float l=dif.Length();
				dif/=l;
				dif+=gs->randVector()*0.15f;
				float3 color= unitDef->nanoColor;
				if(gu->teamNanospray){
					unsigned char* tcol=gs->Team(team)->color;
					color = float3(tcol[0]*(1.f/255.f),tcol[1]*(1.f/255.f),tcol[2]*(1.f/255.f));
				}
				new CGfxProjectile(weaponPos,dif,(int)l,color);
			}
		} else {
			if(!curBuild->beingBuilt){
				if(group && curBuild->group==0)
					curBuild->SetGroup(group);
				Command c;
				c.id=CMD_MOVE_STATE;
				c.options=0;
				c.params.push_back(moveState);
				curBuild->commandAI->GiveCommand(c);
				c.params.clear();
				c.id=CMD_FIRE_STATE;
				c.params.push_back(fireState);
				curBuild->commandAI->GiveCommand(c);
				if(curBuild->commandAI->commandQue.empty() || (dynamic_cast<CMobileCAI*>(curBuild->commandAI) && ((CMobileCAI*)curBuild->commandAI)->unimportantMove)){
					if(((CFactoryCAI*)commandAI)->newUnitCommands.empty()){
						SendToEmptySpot(curBuild);
					} else {
						for(std::deque<Command>::iterator ci=((CFactoryCAI*)commandAI)->newUnitCommands.begin();ci!=((CFactoryCAI*)commandAI)->newUnitCommands.end();++ci)
							curBuild->commandAI->GiveCommand(*ci);
					}
					waitCommandsAI.AddLocalUnit(curBuild, this);
					if (guihandler) {
						guihandler->UnitReady(curBuild, this);
					}
				}
				StopBuild();
			}
		}
	}

	if(lastBuild+200 < gs->frameNum && !quedBuild && opening && uh->CanCloseYard(this)){
		readmap->CloseBlockingYard(this);
		opening=false;
		cob->Call(COBFN_Deactivate);
	}
	CBuilding::Update();
}

void CFactory::StartBuild(string type)
{
	if(beingBuilt)
		return;

#ifdef TRACE_SYNC
	tracefile << "Start build: ";
	tracefile << type.c_str() << "\n";
#endif

	if(curBuild)
		StopBuild();

	quedBuild=true;
	nextBuild=type;

	if(!opening){
		cob->Call(COBFN_Activate);
		readmap->OpenBlockingYard(this, yardMap);
		opening=true;
	}
}

void CFactory::StopBuild()
{
	cob->Call("StopBuilding");
	if(curBuild){
		if(curBuild->beingBuilt){
			AddMetal(curBuild->metalCost*curBuild->buildProgress);
			uh->DeleteUnit(curBuild);
		}
		DeleteDeathDependence(curBuild);
	}
	curBuild=0;
	quedBuild=false;
}

void CFactory::DependentDied(CObject* o)
{
	if(o==curBuild){
		curBuild=0;
		StopBuild();
	}
	CUnit::DependentDied(o);
}

void CFactory::FinishedBuilding(void)
{
	CBuilding::FinishedBuilding();
}

void CFactory::SendToEmptySpot(CUnit* unit)
{
	float r=radius*1.7f+unit->radius*4;

	float3 foundPos=pos+frontdir*r;

	for(int a=0;a<20;++a){
		float3 testPos=pos+frontdir*r*cos(a*PI/10)+rightdir*r*sin(a*PI/10);
		testPos.y=ground->GetHeight(testPos.x,testPos.z);
		if(qf->GetSolidsExact(testPos,unit->radius*1.5f).empty()){
			foundPos=testPos;
			break;
		}
	}
	Command c;
	c.id=CMD_MOVE;
	c.options=0;
	c.params.push_back(foundPos.x);
	c.params.push_back(foundPos.y);
	c.params.push_back(foundPos.z);
	unit->commandAI->GiveCommand(c);
}

void CFactory::SlowUpdate(void)
{
	helper->BuggerOff(pos-float3(0.01f,0,0.02f),radius);

	CBuilding::SlowUpdate();
}

void CFactory::ChangeTeam(int newTeam,ChangeType type)
{
	StopBuild();
	CBuilding::ChangeTeam(newTeam,type);
}
