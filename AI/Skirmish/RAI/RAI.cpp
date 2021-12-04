#include "RAI.h"
#include "LegacyCpp/IGlobalAICallback.h"
#include "LegacyCpp/UnitDef.h"
#include "LegacyCpp/CommandQueue.h"
#include "LegacyCpp/MoveData.h"
#include "CUtils/Util.h"
#include "System/StringUtil.h"
#include "System/SafeCStrings.h"
#include <stdio.h>
//#include <direct.h>	// mkdir function (windows)
//#include <sys/stat.h>	// mkdir function (linux)
#include <time.h>		// time(NULL)

static GlobalResourceMap* GRMap = NULL;
static GlobalTerrainMap* GTMap  = NULL;
static cLogFile* gl = NULL;
static int RAIs=0;

namespace std
{
	void _xlen(){}
}

UnitInfo::UnitInfo(sRAIUnitDef* rUnitDef)
{
	udr=rUnitDef;
	ud=rUnitDef->ud;

	AIDisabled=false;
	humanCommand=false;
	unitBeingBuilt=true;
	inCombat=false;
	lastUnitIdleFrame=0;
	lastUnitDamagedFrame=0;
	enemyID=-1;
	area=0;
	E=0;
	enemyEff=0;
	udrBL=0;
	RS=0;
	BuildQ=0;
	CloakUI=0;
	OnOffUI=0;
	SWeaponUI=0;
	group=0;
	UE = 0;
}

EnemyInfo::EnemyInfo()
{
	inLOS=false;
	inRadar=false;
	baseThreatFrame=-1;
	baseThreatID=-1;
	ud=0;
	udr=0;
	RS=0;
	posLocked=false;
}

cRAI::cRAI()
{
	DebugEnemyEnterLOS=0;
	DebugEnemyLeaveLOS=0;
	DebugEnemyEnterRadar=0;
	DebugEnemyLeaveRadar=0;
	DebugEnemyDestroyedLOS=0;
	DebugEnemyDestroyedRadar=0;
	DebugEnemyEnterLOSError=0;
	DebugEnemyLeaveLOSError=0;
	DebugEnemyEnterRadarError=0;
	DebugEnemyLeaveRadarError=0;
	eventSize = 0;
	SWM=0;
	cb=0;
	frame=0;
	memset(eventList, 0, EVENT_LIST_SIZE);
	TM=0;
	RM=0;
	UM=0;
	UDH=0;
	CM=0;
	B=0;
	l=0;
}

cRAI::~cRAI()
{
	*l<<"\n\nShutting Down ...";
	if( RAIDEBUGGING )
	{
		*l<<"\n cRAI Debug:";
		*l<<"\n  clearing Units size="<<int(Units.size())<<": ";
		*l<<"\n  clearing Enemies size="<<Enemies.size()<<": ";
	}

	while( int(Units.size()) > 0 )
		UnitDestroyed(Units.begin()->first,-1);
	while( int(Enemies.size()) > 0 )
		EnemyDestroyed(Enemies.begin()->first,-1);

	if( RAIDEBUGGING )
	{
		int ERadar,ELOS;
		*l<<"\n Enemys (Enter LOS - Errors)                        = "<<DebugEnemyEnterLOS<<" = "<<DebugEnemyEnterLOS+DebugEnemyEnterLOSError<<"-"<<DebugEnemyEnterLOSError;
		*l<<"\n Enemys (Leave LOS + Destroyed in LOS - Errors)     = "<<DebugEnemyLeaveLOS+DebugEnemyDestroyedLOS<<" = "<<DebugEnemyLeaveLOS+DebugEnemyLeaveLOSError<<"+"<<DebugEnemyDestroyedLOS<<"-"<<DebugEnemyLeaveLOSError;
		*l<<"\n Enemies Remaining in LOS                           = "<<(ELOS=DebugEnemyEnterLOS-DebugEnemyLeaveLOS-DebugEnemyDestroyedLOS);
		*l<<"\n Enemys (Enter Radar - Errors)                      = "<<DebugEnemyEnterRadar<<" = "<<DebugEnemyEnterRadar+DebugEnemyEnterRadarError<<"-"<<DebugEnemyEnterRadarError;
		*l<<"\n Enemys (Leave Radar + Destroyed in Radar - Errors) = "<<DebugEnemyLeaveRadar+DebugEnemyDestroyedRadar<<" = "<<DebugEnemyLeaveRadar+DebugEnemyLeaveRadarError<<"+"<<DebugEnemyDestroyedRadar<<"-"<<DebugEnemyLeaveRadarError;
		*l<<"\n Enemies Remaining in Radar                         = "<<(ERadar=DebugEnemyEnterRadar-DebugEnemyLeaveRadar-DebugEnemyDestroyedRadar);
		*l<<"\n Enemies Remaining                                  = "<<int(Enemies.size());
		*l<<"\n Units Remaining                                    = "<<UM->GroupSize;
		*l<<"\n Groups Remaining                                   = "<<UM->GroupSize;
		if( UM->GroupSize != 0 || ELOS != 0 || ERadar != 0 )
			*l<<"\n  (ERROR)";
	}

	delete UM;
	UM = NULL;
	delete B;
	B = NULL;
	delete SWM;
	SWM = NULL;
	delete CM;
	CM = NULL;
	delete UDH;
	UDH = NULL;

	RAIs--;
	if( RAIs == 0 )
	{
		*gl<<"\n Global RAI Shutting Down";
//		double closingTimer = clock();
		GlobalResourceMap* tmpRM = GRMap;
		GRMap = NULL;
		RM    = NULL;
		delete tmpRM;
		tmpRM = NULL;
//		*gl<<"\n Resource-Map Closing Time: "<<(clock()-closingTimer)/(double)CLOCKS_PER_SEC<<" seconds";
		GlobalTerrainMap* tmpTM = GTMap;
		GTMap = NULL;
		TM    = NULL;
		delete tmpTM;
		tmpTM = NULL;
		*gl<<"\n Global RAI Shutdown Complete.";
		delete gl;
		gl = NULL;
	}

	*l<<"\nShutdown Complete.";
	delete l;
	l = NULL;
}

void cRAI::InitAI(IGlobalAICallback* callback, int team)
{
	cb = callback->GetAICallback();
	if( GRMap == 0 )
		ClearLogFiles();
	l = new cLogFile(cb, GetLogFileSubPath(cb->GetMyTeam()), false);

/*	string test = (char*)cb->GetMyTeam(); // Crashes Spring?  Spring-Version(v0.76b1)
	*l<<"cb->GetMyTeam()="<<test<<"\n";
*/
	if( GRMap == 0 )
	{
		ClearLogFiles();
		gl = new cLogFile(cb, "log/RAIGlobal_LastGame.log", false);
		*gl<<"Loading Global RAI...";
		*gl<<"\n Mod = " << cb->GetModHumanName() << "(" << IntToString(cb->GetModHash(), "%x") << ")";
		*gl<<"\n Map = " << cb->GetMapName()      << "(" << IntToString(cb->GetMapHash(), "%x") << ")";
		int seed = time(NULL);
		srand(seed);
		RAIs=0;
		double loadingTimer = clock();
		GTMap = new GlobalTerrainMap(cb,gl);
		*gl<<"\n  Terrain-Map Loading Time: "<<(clock()-loadingTimer)/(double)CLOCKS_PER_SEC<<" seconds";
		loadingTimer = clock();
		GRMap = new GlobalResourceMap(cb,gl,GTMap);
		*gl<<"\n  Resource-Map Loading Time: "<<(clock()-loadingTimer)/(double)CLOCKS_PER_SEC<<" seconds\n";
/*
		loadingTimer = clock();
		CMetalMap* KMM;
		KMM = new CMetalMap(cb);
		KMM->Init();
		*l<<"  KAI Metal-Map Loading Time: "<<(clock()-loadingTimer)/(double)CLOCKS_PER_SEC<<" seconds";
		*l<<"\n   KAI Metal-Sites Found: "<<KMM->NumSpotsFound;
		delete KMM;
*/
		*gl<<"\nGlobal RAI Loading Complete.\n\n";
	}
	RM = GRMap;
	TM = GTMap;
	RAIs++;

	*l<<"Loading ...";
	*l<<"\n Team = "<<cb->GetMyTeam();
	*l<<"\n Ally Team = "<<cb->GetMyAllyTeam();

	*l<<"\nLoading cRAIUnitDefHandler ...";
	UDH = new cRAIUnitDefHandler(cb,RM,TM,l);

	UM = new cUnitManager(cb,this);
	B = new cBuilder(cb,this);
	UpdateEventAdd(3,cb->GetCurrentFrame()-1);
	SWM = new cSWeaponManager(cb,this);
	CM = new cCombatManager(cb,this);
	*l<<"\nLoading Complete.\n";
/*
	if( cb->GetUnitDef("arm_retaliator") != 0 ) // XTA Cheat Nuke Test
	{
		IAICheats* cheat=callback->GetCheatInterface();
		float3 pos =*cb->GetStartPos();
		float3 pos2 =*cb->GetStartPos();
		float3 pos3 =*cb->GetStartPos();
		if( pos.x < 8*cb->GetMapWidth() - pos.x )
		{
			pos.x = 1;
			pos2.x = 200;
			pos3.x = 1;
		}
		else
		{
			pos.x = 8*cb->GetMapWidth();
			pos2.x = pos.x - 200;
			pos3.x = pos.x;
		}
		if( pos.z < 8*cb->GetMapHeight() - pos.z )
		{
			pos.z = 1;
			pos2.z = 1;
			pos3.z = 200;
		}
		else
		{
			pos.z = 8*cb->GetMapHeight();
			pos2.z = pos.z;
			pos3.z = pos.z - 200;
		}

		pos = cb->ClosestBuildSite(cb->GetUnitDef("arm_retaliator"),pos,1500,0);
		cheat->CreateUnit("arm_retaliator",pos);
		cheat->CreateUnit("arm_moho_metal_maker",cb->ClosestBuildSite(cb->GetUnitDef("arm_moho_metal_maker"),pos,1500,3));
		cheat->CreateUnit("arm_fusion_reactor",cb->ClosestBuildSite(cb->GetUnitDef("arm_fusion_reactor"),pos,1500,3));
		cheat->CreateUnit("arm_energy_storage",cb->ClosestBuildSite(cb->GetUnitDef("arm_energy_storage"),pos,1500,3));
		cheat->CreateUnit("arm_annihilator",cb->ClosestBuildSite(cb->GetUnitDef("arm_annihilator"),pos,1500,10));
		cheat->CreateUnit("arm_annihilator",cb->ClosestBuildSite(cb->GetUnitDef("arm_annihilator"),pos,1500,10));
		cheat->CreateUnit("arm_annihilator",cb->ClosestBuildSite(cb->GetUnitDef("arm_annihilator"),pos,1500,10));
	}
*/
}

void cRAI::UnitCreated(int unit, int builder)
{
	const UnitDef* ud=cb->GetUnitDef(unit);
	if( RAIDEBUGGING ) { *l<<"\nUnitCreated("<<unit; *l<<")["+ud->humanName+"]"; }
	Units.insert(iuPair(unit,UnitInfo(&UDH->UDR.find(ud->id)->second)));
	UnitInfo* U = &Units.find(unit)->second;
	float3 position = cb->GetUnitPos(unit);
	U->area = GetCurrentMapArea(U->udr,position);

	if( ud->speed == 0 )
	{
		for(map<int,UnitInfo*>::iterator i=UImmobile.begin(); i!=UImmobile.end(); ++i )
		{
			if( U->udr->WeaponGuardRange > 0 && i->second->udr->WeaponGuardRange == 0 && position.distance2D(cb->GetUnitPos(i->first)) < U->udr->WeaponGuardRange )
			{
				U->UDefending.insert(cRAI::iupPair(i->first,i->second));
				i->second->UDefences.insert(cRAI::iupPair(unit,U));
			}
			else if( U->udr->WeaponGuardRange == 0 && i->second->udr->WeaponGuardRange > 0 && position.distance2D(cb->GetUnitPos(i->first)) < i->second->udr->WeaponGuardRange )
			{
				U->UDefences.insert(cRAI::iupPair(i->first,i->second));
				i->second->UDefending.insert(cRAI::iupPair(unit,U));
			}
		}
		UImmobile.insert(iupPair(unit,U));
	}
	else
		UMobile.insert(iupPair(unit,U));

	B->UnitCreated(unit,U);
	B->BP->UResourceCreated(unit,U);
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::UnitFinished(int unit)
{
	if( RAIDEBUGGING ) *l<<"\nUnitFinished("<<unit<<")";
	if( Units.find(unit) == Units.end() ) // Occurs if a player canceled a build order with more than one quaried and something still finished
		UnitCreated(unit, -1);

	UnitInfo* U = &Units.find(unit)->second;
	U->unitBeingBuilt = false;
	if( U->AIDisabled )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}

	B->UnitFinished(unit,U);
	if( U->AIDisabled )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}
	B->PM->UnitFinished(unit,U);
	SWM->UnitFinished(unit,U->udr);
	UM->UnitFinished(unit,U);
	if( U->ud->highTrajectoryType == 2 && rand()%4 == 0 )
	{	// Nothing too meanful, just a degree of randomness
		Command c;
		c.id = CMD_TRAJECTORY;
		c.params.push_back(1);
		cb->GiveOrder(unit,&c);
	}
	if( U->ud->speed == 0 )
		UnitIdle(unit);

	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::UnitDestroyed(int unit,int attacker)
{
	if( RAIDEBUGGING ) *l<<"\nUnitDestroyed("<<unit<<","<<attacker<<")";
	if( Units.find(unit) == Units.end() ) // Occurs if a player canceled a build order with more than one quaried and something was still being worked on
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}
	UnitInfo *U = &Units.find(unit)->second;
	if( U->UE != 0 )
		UpdateEventRemove(U->UE);
	if( !U->AIDisabled )
	{
		B->UnitDestroyed(unit,U);
		if( !U->unitBeingBuilt )
		{
			B->PM->UnitDestroyed(unit,U);
			SWM->UnitDestroyed(unit);
			UM->UnitDestroyed(unit,U);
		}
	}
	B->BP->UResourceDestroyed(unit,U);
	if( U->ud->speed == 0 )
	{
		for(map<int,UnitInfo*>::iterator i=U->UDefending.begin(); i!=U->UDefending.end(); ++i )
			i->second->UDefences.erase(unit);
		for(map<int,UnitInfo*>::iterator i=U->UDefences.begin(); i!=U->UDefences.end(); ++i )
			i->second->UDefending.erase(unit);
		UImmobile.erase(unit);
	}
	else
		UMobile.erase(unit);
	Units.erase(unit);
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::EnemyEnterLOS(int enemy)
{
	if( RAIDEBUGGING ) *l<<"\nEnemyEnterLOS("<<enemy<<")";
	if( cb->GetUnitHealth(enemy) <= 0 ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		DebugEnemyEnterLOSError++;
		*l<<"\nWARNING: EnemyEnterLOS("<<enemy<<"): enemy is either dead or not in LOS";
		return;
	}
	DebugEnemyEnterLOS++;
	if( Enemies.find(enemy) == Enemies.end() )
		Enemies.insert(iePair(enemy,EnemyInfo()));
	EnemyInfo *E = &Enemies.find(enemy)->second;
	E->inLOS=true;
	E->ud=cb->GetUnitDef(enemy);
	E->udr=&UDH->UDR.find(E->ud->id)->second;
	if( E->ud->speed == 0 )
	{
		E->position = cb->GetUnitPos(enemy);
		E->posLocked = true;
	}
	else if( E->posLocked )
	{	// just in case the id is being reused
		E->posLocked = false;
	}

	UM->EnemyEnterLOS(enemy,E);
	B->BP->EResourceEnterLOS(enemy,E);
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::EnemyLeaveLOS(int enemy)
{
	if( RAIDEBUGGING ) *l<<"\nEnemyLeaveLOS("<<enemy<<")";
	if( Enemies.find(enemy) == Enemies.end() ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		DebugEnemyLeaveLOSError++;
		*l<<"\nWARNING: EnemyLeaveLOS("<<enemy<<"): unknown unit id";
		return;
	}
	EnemyInfo *E = &Enemies.find(enemy)->second;
	if( !E->inLOS ) // Work Around:  Spring-Version(v0.76b1)
	{
		DebugEnemyLeaveLOSError++;
		*l<<"\nWARNING: EnemyLeaveLOS("<<enemy<<"): not in LOS";
		return;
	}

	DebugEnemyLeaveLOS++;
	E->inLOS=false;
	if( !E->inRadar )
	{
		if( !E->posLocked )
			E->position = cb->GetUnitPos(enemy);
		if( !TM->IsSectorValid(TM->GetSectorIndex(E->position)) )
			EnemyRemove(enemy,E);
	}
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::EnemyEnterRadar(int enemy)
{
	if( RAIDEBUGGING ) *l<<"\nEnemyEnterRadar("<<enemy<<")";
	if( cb->GetUnitPos(enemy).x <= 0 && cb->GetUnitPos(enemy).y <= 0 && cb->GetUnitPos(enemy).z <= 0 ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		DebugEnemyEnterRadarError++;
		*l<<"\nWARNING: EnemyEnterRadar("<<enemy<<"): enemy position is invalid";
		return;
	}
	DebugEnemyEnterRadar++;
	if( Enemies.find(enemy) == Enemies.end() )
		Enemies.insert(iePair(enemy,EnemyInfo()));
	EnemyInfo *E = &Enemies.find(enemy)->second;
	E->inRadar=true;
	UM->EnemyEnterRadar(enemy,E);
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::EnemyLeaveRadar(int enemy)
{
	if( RAIDEBUGGING ) *l<<"\nEnemyLeaveRadar("<<enemy<<")";
	if( Enemies.find(enemy) == Enemies.end() ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		DebugEnemyLeaveRadarError++;
		*l<<"\nWARNING: EnemyLeaveRadar("<<enemy<<"): unknown unit id";
		return;
	}
	EnemyInfo *E = &Enemies.find(enemy)->second;
	if( !E->inRadar ) // Work Around:  Spring-Version(v0.76b1)
	{
		DebugEnemyLeaveRadarError++;
		*l<<"\nWARNING: EnemyLeaveRadar("<<enemy<<"): not in radar";
		return;
	}

	DebugEnemyLeaveRadar++;
	E->inRadar=false;
	if( !E->inLOS )
	{
		if( !E->posLocked )
			E->position = cb->GetUnitPos(enemy);
		if( !TM->IsSectorValid(TM->GetSectorIndex(E->position)) )
			EnemyRemove(enemy,E);
	}
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::EnemyDestroyed(int enemy,int attacker)
{
	if( RAIDEBUGGING ) *l<<"\nEnemyDestroyed("<<enemy<<","<<attacker<<")";
	if( Enemies.find(enemy) == Enemies.end() ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		*l<<"\nWARNING: EnemyDestroyed("<<enemy<<","<<attacker<<"): unknown unit id";
		return;
	}

	EnemyInfo *E = &Enemies.find(enemy)->second;
	if( E->inLOS )
		DebugEnemyDestroyedLOS++;
	if( E->inRadar )
		DebugEnemyDestroyedRadar++;
	EnemyRemove(enemy,E);
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::EnemyRemove(int enemy, EnemyInfo *E)
{
	if( E->RS != 0 && E->RS->unitID == enemy )
	{
		E->RS->unitID=-1;
		E->RS->unitUD=0;
		E->RS->enemy = false;
	}
	if( E->baseThreatID != -1 )
		EThreat.erase(enemy);
	while( int(E->attackGroups.size()) > 0 )
		UM->GroupRemoveEnemy(enemy,E,*E->attackGroups.begin());
	Enemies.erase(enemy);
}

void cRAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{

}

void cRAI::UnitIdle(int unit)
{
	if( RAIDEBUGGING ) *l<<"\nUI("<<unit<<")";
	if( Units.find(unit) == Units.end() ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		*l<<"\nWARNING: UnitIdle("<<unit<<"): unknown unit id";
		return;
	}
	UnitInfo *U = &Units.find(unit)->second;
	if( U->AIDisabled || cb->UnitBeingBuilt(unit) || cb->IsUnitParalyzed(unit) || int(cb->GetCurrentUnitCommands(unit)->size()) > 0 )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}
	U->humanCommand=false;
	if( cb->GetCurrentFrame() <= U->lastUnitIdleFrame+15 ) // !! Occurs if enemy attack order fails/...possibly some other reason
	{
		UpdateEventAdd(1,cb->GetCurrentFrame()+15,unit,U);
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}
	U->lastUnitIdleFrame=cb->GetCurrentFrame();
	if( U->UE != 0 && U->UE->type == 1 )
		UpdateEventRemove(U->UE);
	if( U->inCombat )
	{
		CM->UnitIdle(unit,U);
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}
	UM->UnitIdle(unit,U);
	if( RAIDEBUGGING ) *l<<"#";
}


void cRAI::UnitDamaged(int unit,int attacker,float damage,float3 dir)
{
	if( RAIDEBUGGING ) *l<<"\nUnitDamaged("<<unit<<","<<attacker<<","<<damage<<",x"<<dir.x<<" z"<<dir.z<<" y"<<dir.y<<")";
	if( cb->UnitBeingBuilt(unit) || cb->IsUnitParalyzed(unit) || cb->GetUnitHealth(unit) <= 0.0 )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}

	UnitInfo *U = &Units.find(unit)->second;
	if( cb->GetCurrentFrame() <= U->lastUnitDamagedFrame+15 )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}

	U->lastUnitDamagedFrame=cb->GetCurrentFrame();

	EnemyInfo *E=0;
	if( Enemies.find(attacker) != Enemies.end() )
		E = &Enemies.find(attacker)->second;
	else
		attacker = -1;

	if( U->ud->speed==0 )
	{
		if( E != 0 )
		{
			E->baseThreatFrame=cb->GetCurrentFrame();
			E->baseThreatID=unit;
			if( EThreat.find(attacker) == EThreat.end() )
			{
				EThreat.insert(iepPair(attacker,E));
				for( int i=0; i<UM->GroupSize; i++ )
					if( int(UM->Group[i]->Enemies.size()) == 0 && !UM->Group[i]->Units.begin()->second->ud->canLoopbackAttack )
						CM->GetClosestEnemy(cb->GetUnitPos(UM->Group[i]->Units.begin()->first),UM->Group[i]->Units.begin()->second);
			}
		}
		ValidateUnitList(&U->UGuards);
		for( map<int,UnitInfo*>::iterator i = U->UGuards.begin(); i != U->UGuards.end(); ++i )
		{
			if( int(i->second->URepair.size()) == 0 && !IsHumanControled(i->first,i->second) )
			{
				Command c;
				c.id = CMD_REPAIR;
				c.params.push_back(unit);
				cb->GiveOrder(i->first, &c);
			}
			if( i->second->URepair.find(unit) == i->second->URepair.end() )
				i->second->URepair.insert(iupPair(unit,U));
		}
		if( U->ud->canReclaim && attacker > -1 && U->udr->IsNano() && cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(attacker)) < U->ud->buildDistance && !IsHumanControled(unit,U) )
		{
			if( cb->GetCurrentUnitCommands(unit)->size() == 0 || (cb->GetCurrentUnitCommands(unit)->front().id != CMD_REPAIR && cb->GetCurrentUnitCommands(unit)->front().id != CMD_RECLAIM) )
			{
				Command c;
				c.id = CMD_RECLAIM;
				c.params.push_back(attacker);
				cb->GiveOrder(unit, &c);
			}
		}
	}
	if( U->AIDisabled || U->udrBL->task<=0 || U->ud->speed==0 )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}

	if( E != 0 && U->group != 0 && U->group->Enemies.find(attacker) == U->group->Enemies.end() )
	{
		if( E->baseThreatID != -1 || UM->ActiveAttackOrders() )
			UM->GroupAddEnemy(attacker,E,U->group);
	}

	if( !IsHumanControled(unit,U) )
		CM->UnitDamaged(unit,U,attacker,E,dir);

	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::UnitMoveFailed(int unit)
{
	if( RAIDEBUGGING ) *l<<"\nUnitMoveFailed("<<unit<<")";
	if( UMobile.find(unit) == UMobile.end() ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
	{
		*l<<"\nWARNING: UnitMoveFailed("<<unit<<"): unknown unit id";
		return;
	}

	UnitInfo *U = UMobile.find(unit)->second;
	if( U->AIDisabled || cb->IsUnitParalyzed(unit) ||
		B->UBuilderMoveFailed(unit,U) || UM->UnitMoveFailed(unit,U) ||
		int(cb->GetCurrentUnitCommands(unit)->size()) > 0 )
	{
		if( RAIDEBUGGING ) *l<<"#";
		return;
	}

	Command c;
	c.id=CMD_WAIT;
	//c.timeOut=cb->GetCurrentFrame()+120;
	cb->GiveOrder(unit,&c);
	UpdateEventAdd(1,cb->GetCurrentFrame()+90,unit,U);
	if( RAIDEBUGGING ) *l<<"#";
}

int cRAI::HandleEvent(int msg,const void* data)
{
	if( RAIDEBUGGING ) *l<<"\nHandleEvent("<<msg<<","<<"~"<<")";
	switch (msg)
	{
	case AI_EVENT_UNITGIVEN:
	case AI_EVENT_UNITCAPTURED:
		{
			const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;

			const int myAllyTeamId = cb->GetMyAllyTeam();
			const bool oldEnemy = !cb->IsAllied(myAllyTeamId, cb->GetTeamAllyTeam(cte->oldteam));
			const bool newEnemy = !cb->IsAllied(myAllyTeamId, cb->GetTeamAllyTeam(cte->newteam));

			if ( oldEnemy && !newEnemy ) {
			{
				if( Enemies.find(cte->unit) != Enemies.end() )
					EnemyDestroyed(cte->unit,-1);
				}
			}
			else if( !oldEnemy && newEnemy )
			{
				// unit changed from an ally to an enemy team
				// we lost a friend! :(
				EnemyCreated(cte->unit);
				if (!cb->UnitBeingBuilt(cte->unit)) {
					EnemyFinished(cte->unit);
				}
			}

			if( cte->oldteam == cb->GetMyTeam() )
			{
				UnitDestroyed(cte->unit,-1);
			}
			else if( cte->newteam == cb->GetMyTeam() )
			{
				if( cb->GetUnitHealth(cte->unit) <= 0 ) // ! Work Around:  Spring-Version(v0.74b1-0.75b2)
				{
					*l<<"\nERROR: HandleEvent(AI_EVENT_(UNITGIVEN|UNITCAPTURED)): given unit is dead or does not exist";
					return 0;
				}
				UnitCreated(cte->unit, -1);
				Units.find(cte->unit)->second.AIDisabled=false;
				if( !cb->UnitBeingBuilt(cte->unit) )
				{
					UnitFinished(cte->unit);
					UnitIdle(cte->unit);
				}
			}
		}
		break;
	case AI_EVENT_PLAYER_COMMAND:
		{
			const IGlobalAI::PlayerCommandEvent* pce = (const IGlobalAI::PlayerCommandEvent*) data;
			bool ImportantCommand=false;
			if( pce->command.id < 0 )
				ImportantCommand = true;
			switch( pce->command.id )
			{
			case CMD_MOVE:
			case CMD_PATROL:
			case CMD_FIGHT:
			case CMD_ATTACK:
			case CMD_AREA_ATTACK:
			case CMD_GUARD:
			case CMD_REPAIR:
			case CMD_LOAD_UNITS:
			case CMD_UNLOAD_UNITS:
			case CMD_UNLOAD_UNIT:
			case CMD_RECLAIM:
			case CMD_DGUN:
			case CMD_RESTORE:
			case CMD_RESURRECT:
			case CMD_CAPTURE:
				ImportantCommand = true;
			}

			for( int i=0; i<int(pce->units.size()); i++ )
			{
				if( Units.find(pce->units.at(i)) == Units.end() ) // ! Work Around:  Spring-Version(v0.75b2)
				{
					*l<<"\nERROR: HandleEvent(AI_EVENT_PLAYER_COMMAND): unknown unit id="<<pce->units.at(i);
//					pce->units.erase(pce->units.begin()+i);
//					i--;
				}
				else if( ImportantCommand )
					Units.find(pce->units.at(i))->second.humanCommand = true;
			}
			if( ImportantCommand )
			{
				B->HandleEvent(pce);
			}
			else if( pce->command.id == CMD_SELFD )
			{
				for( vector<int>::const_iterator i=pce->units.begin(); i!=pce->units.end(); ++i )
					UnitDestroyed(*i,-1);
			}
		}
		break;
	}
	if( RAIDEBUGGING ) *l<<"#";
	return 0;
}

void cRAI::Update()
{
	frame=cb->GetCurrentFrame();
	if(frame%FUPDATE_MINIMAL)
		return;

	if( RAIDEBUGGING ) *l<<"\nUpdate("<<frame<<")";

	if(!(frame%FUPDATE_POWER))
	{	// Old Code, ensures a unit won't just go permanently idle, hopefully unnecessary in the future
		ValidateAllUnits();
		for(map<int,UnitInfo>::iterator iU=Units.begin(); iU!=Units.end(); ++iU)
			if( !cb->UnitBeingBuilt(iU->first) && !iU->second.AIDisabled && iU->second.udrBL->task > 1 &&
				frame > iU->second.lastUnitIdleFrame+FUPDATE_UNITS && iU->second.UE == 0 && cb->GetCurrentUnitCommands(iU->first)->size() == 0 )
			{
//				*l<<"\nWARNING: Unit was Idle  Name="<<iU->second.ud->humanName<<"("<<iU->first<<")";
				UnitIdle(iU->first);
			}
	}

	while( eventSize > 0 && eventList[0]->frame <= frame )
	{
		switch( eventList[0]->type )
		{
		case 1: // Checks for idleness
//			*l<<"\n(u1)";
			if( ValidateUnit(eventList[0]->unitID) ) // if false, the event will be removed elsewhere
			{
				if( !IsHumanControled(eventList[0]->unitID,eventList[0]->unitI) )
				{
//					*l<<" Stopping "<<eventList[0]->unitI->ud->humanName<<"("<<eventList[0]->unitID<<")";
					eventList[0]->unitI->lastUnitIdleFrame = -1;
					if( eventList[0]->unitI->ud->speed == 0 )
					{
						int unit = eventList[0]->unitID;
						UpdateEventRemove(eventList[0]);
						UnitIdle(unit);
					}
					else
					{	// doesn't seem to work for factories  Spring-Version(v0.76b1)
						Command c;
						c.id=CMD_STOP;
						cb->GiveOrder(eventList[0]->unitID, &c);
						UpdateEventRemove(eventList[0]);
					}
				}
				else
				{
					eventList[0]->frame = frame;
					UpdateEventReorderFirst();
				}
			}
			break;
		case 2: // Few Unit Monitoring
//			*l<<"\n(u2)";
			if( ValidateUnit(eventList[0]->unitID) ) // if false, the event will be removed elsewhere
			{
				if( !eventList[0]->unitI->inCombat &&
					!IsHumanControled(eventList[0]->unitID,eventList[0]->unitI) &&
					eventList[0]->unitI->BuildQ != 0 &&
					cb->GetCurrentUnitCommands(eventList[0]->unitID)->front().id < 0 )
				{
					if( eventList[0]->lastPosition == 0 )
						eventList[0]->lastPosition = new float3;
					float3 position = cb->GetUnitPos(eventList[0]->unitID);
					if( eventList[0]->unitI->BuildQ->creationID.size() > 0 )
					{
						eventList[0]->lastPosition->x = -1.0;
						float3 conPosition = cb->GetUnitPos(eventList[0]->unitI->BuildQ->creationID.front());
						if( abs(int(position.x-conPosition.x)) + 4.0 < 8.0*eventList[0]->unitI->ud->xsize/2.0 + 8.0*eventList[0]->unitI->BuildQ->creationUD->ud->xsize/2.0 &&
							abs(int(position.z-conPosition.z)) + 4.0 < 8.0*eventList[0]->unitI->ud->zsize/2.0 + 8.0*eventList[0]->unitI->BuildQ->creationUD->ud->zsize/2.0 )
						{	// most likely, the commander built something on top of himself
							Command c;
							c.id = CMD_RECLAIM;
							c.params.push_back(eventList[0]->unitI->BuildQ->creationID.front());
							cb->GiveOrder(eventList[0]->unitID,&c);
							if( B->BP->NeedResourceSite(eventList[0]->unitI->BuildQ->creationUD->ud) )
							{
								c.params.clear();
								c.id = CMD_MOVE;
								c.options = SHIFT_KEY;
								float3 newPos;
								if( position.x < conPosition.x )
									newPos.x = (position.x - ((position.x-conPosition.x)/position.distance2D(conPosition))*(8.0*eventList[0]->unitI->ud->xsize/2.0 +8.0*eventList[0]->unitI->BuildQ->creationUD->ud->xsize/2.0) );
								else
									newPos.x = (position.x + ((position.x-conPosition.x)/position.distance2D(conPosition))*(8.0*eventList[0]->unitI->ud->xsize/2.0 +8.0*eventList[0]->unitI->BuildQ->creationUD->ud->xsize/2.0) );
								if( position.z < conPosition.z )
									newPos.z = (position.z - ((position.z-conPosition.z)/position.distance2D(conPosition))*(8.0*eventList[0]->unitI->ud->zsize/2.0 +8.0*eventList[0]->unitI->BuildQ->creationUD->ud->zsize/2.0) );
								else
									newPos.z = (position.z + ((position.z-conPosition.z)/position.distance2D(conPosition))*(8.0*eventList[0]->unitI->ud->zsize/2.0 +8.0*eventList[0]->unitI->BuildQ->creationUD->ud->zsize/2.0) );
								CorrectPosition(newPos);
								c.params.push_back(newPos.x);
								c.params.push_back(newPos.y);
								c.params.push_back(newPos.z);
								cb->GiveOrder(eventList[0]->unitID,&c);
							}
						}
					}
					else if( position == *eventList[0]->lastPosition )
					{	// most likely, the commander is stuck at the starting point
//						if( eventList[0]->unitI->area == 0 && TM->udMobileType.find(eventList[0]->unitI->ud->id)->second != 0 )
//						{} // trapped forever
						eventList[0]->unitI->lastUnitIdleFrame = -1;
						Command c;
						c.id = CMD_MOVE;
						float f = (40.0+(rand()%401)/10.0);
						float3 newPos = position;
						if( rand()%2 == 0 )
							newPos.x += f;
						else
							newPos.x -= f;
						f = (40.0+(rand()%401)/10.0);
						if( rand()%2 == 0 )
							newPos.z += f;
						else
							newPos.z -= f;

						CorrectPosition(newPos);
						c.params.push_back(newPos.x);
						c.params.push_back(newPos.y);
						c.params.push_back(newPos.z);
						cb->GiveOrder(eventList[0]->unitID,&c);
						*eventList[0]->lastPosition = position;
					}
					else
						*eventList[0]->lastPosition = position;
				}
				if( Units.size() >= 10 )
				{
					if( eventList[0]->lastPosition != 0 )
						delete eventList[0]->lastPosition;
					UpdateEventRemove(eventList[0]);
				}
				else
					UpdateEventReorderFirst();
			}
			break;
		case 3: // Initiatization
//			*l<<"\n(u3)";
			if( frame >= 210 || ( cb->GetMetalIncome()>0 && cb->GetMetalIncome()<0.9*cb->GetMetalStorage() ) || ( cb->GetEnergyIncome()>0 && cb->GetEnergyIncome()<0.9*cb->GetEnergyStorage() ) )
			{
				*l<<"\nInitiated=true  Frame="<<frame<<" Metal-Income="<<cb->GetMetalIncome()<<" Energy-Income="<<cb->GetEnergyIncome()<<"\n";
				UpdateEventRemove(eventList[0]);
				B->UpdateUDRCost();
				for(map<int,UnitInfo>::iterator i=Units.begin(); i!=Units.end(); ++i )
					if( !i->second.AIDisabled  )
					{
						if( Units.size() < 10 && i->second.ud->movedata != 0 )
							UpdateEventAdd(2,frame+FUPDATE_MINIMAL,i->first,&i->second);
						if( cb->GetCurrentUnitCommands(i->first)->size() == 0 )
							UnitIdle(i->first);
					}
				B->bInitiated=true;
			}
			else
				UpdateEventReorderFirst();
			break;
		default:
			*l<<"(ERROR)";
			UpdateEventRemove(eventList[0]);
			break;
		}
	}

	if(!(frame%FUPDATE_POWER))
	{
		B->PM->Update();
		SWM->Update();
		if(!(frame%FUPDATE_BUILDLIST))
			B->UpdateUDRCost();
	}
	if( RAIDEBUGGING ) *l<<"#";
}

void cRAI::UpdateEventAdd(const int &eventType, const int &eventFrame, int uID, UnitInfo* uI)
{
	if( eventSize == 10000 )
	{
		*l<<"\nERROR: Event Maximum Reached.";
		return;
	}

	UpdateEvent* e = new UpdateEvent;
	if( uI != 0 )
	{
		if( uI->UE != 0 ) // The unit already has an event
		{
			if( eventType >= uI->UE->type ) // more or equally important
				UpdateEventRemove(uI->UE);	// remove the old
			else
			{
				delete e;
				return;
			}
		}
		uI->UE = e;
	}

	e->type = eventType;
	e->frame = eventFrame;
	e->unitID = uID;
	e->unitI = uI;
	e->lastPosition = 0;

	for(e->index = eventSize; e->index>0; e->index-- )
		if( e->frame < eventList[e->index-1]->frame )
		{
			eventList[e->index] = eventList[e->index-1];
			eventList[e->index]->index = e->index;
		}
		else
			break;

	eventList[e->index] = e;
	eventSize++;
}

void cRAI::UpdateEventReorderFirst()
{
	UpdateEvent* e = eventList[0];
	e->frame += FUPDATE_MINIMAL;
	while( e->index < eventSize-1 && eventList[e->index+1]->frame < e->frame )
	{
		eventList[e->index] = eventList[e->index+1];
		eventList[e->index]->index = e->index;
		e->index++;
	}
	eventList[e->index] = e;
}

void cRAI::UpdateEventRemove(UpdateEvent* e)
{
	if( e->unitI != 0 )
		e->unitI->UE = 0;
	eventSize--;
	while( e->index < eventSize )
	{
		eventList[e->index] = eventList[e->index+1];
		eventList[e->index]->index = e->index;
		e->index++;
	}
	delete e;
}

void cRAI::CorrectPosition(float3& position)
{
	if( position.x < 1 )
		position.x = 1;
	else if( position.x > 8*cb->GetMapWidth()-2 )
		position.x = 8*cb->GetMapWidth()-2;
	if( position.z < 1 )
		position.z = 1;
	else if( position.z > 8*cb->GetMapHeight()-2 )
		position.z = 8*cb->GetMapHeight()-2;
	position.y = cb->GetElevation(position.x,position.z);
}

TerrainMapArea* cRAI::GetCurrentMapArea(sRAIUnitDef* udr, float3& position)
{
	if( udr->mobileType == 0 ) // flying units & buildings
		return 0;

	// other mobile units & their factories
	int iS = TM->GetSectorIndex(position);
	if( !TM->IsSectorValid(iS) )
	{
		CorrectPosition(position);
		iS = TM->GetSectorIndex(position);
	}
	return udr->mobileType->sector[iS].area;
}

float3 cRAI::GetRandomPosition(TerrainMapArea* area)
{
	float3 Pos;
	if( area == 0 )
	{
		Pos.x=1.0 + rand()%7 + 8.0*(rand()%cb->GetMapWidth());
		Pos.z=1.0 + rand()%7 + 8.0*(rand()%cb->GetMapHeight());
		CorrectPosition(Pos);
		return Pos;
	}

	vector<int> Temp;
	for( map<int,TerrainMapAreaSector*>::iterator iS=area->sector.begin(); iS!=area->sector.end(); ++iS )
		Temp.push_back(iS->first);
	int iS=Temp.at(rand()%int(Temp.size()));
	Pos.x=TM->sector[iS].position.x - TM->convertStoP/2-1.0 + rand()%(TM->convertStoP-1);
	Pos.z=TM->sector[iS].position.z - TM->convertStoP/2-1.0 + rand()%(TM->convertStoP-1);
	CorrectPosition(Pos);
	return Pos;
}

bool cRAI::IsHumanControled(const int& unit,UnitInfo* U)
{
	if( int(cb->GetCurrentUnitCommands(unit)->size()) == 0 )
		return false;
	if( U->humanCommand )
		return true;
	return false;
}

bool cRAI::ValidateUnit(const int& unitID)
{
	if( cb->GetUnitDef(unitID) == 0 ) // ! Work Around:  Spring-Version(v0.74b1-0.75b2)
	{
		*l<<"\nERROR: ValidateUnit(): iU->first="<<unitID;
		UnitDestroyed(unitID,-1);
		return false;
	}
	return true;
}

bool cRAI::ValidateUnitList(map<int,UnitInfo*>* UL)
{
	int ULsize = UL->size();
	for(map<int,UnitInfo*>::iterator iU=UL->begin(); iU!=UL->end(); ++iU)
	{
		if( !ValidateUnit(iU->first) )
		{
			// The iterator has becomes invalid at this point
			if( ULsize == 1 ) // if true then the list is now empty, and may have been deleted (UL is invalid)
				return false;
			else
				return ValidateUnitList(UL);
		}
	}
	return true;
}

void cRAI::ValidateAllUnits()
{
	for(map<int,UnitInfo>::iterator iU=Units.begin(); iU!=Units.end(); ++iU)
	{
		if( !ValidateUnit(iU->first) )
		{
			// The iterator has becomes invalid at this point
			ValidateAllUnits();
			return;
		}
	}
}

void cRAI::DebugDrawLine(float3 StartPos, float distance, int direction, float xposoffset, float zposoffset, float yposoffset, int lifetime, int arrow, float width, int group)
{
	StartPos.x+=xposoffset;
	StartPos.z+=zposoffset;
	StartPos.y+=yposoffset;
	float3 EndPos=StartPos;
	switch( direction )
	{
	case 0:
			EndPos.x+=distance;
		break;
	case 1:
			EndPos.z+=distance;
		break;
	case 2:
			EndPos.x-=distance;
		break;
	case 3:
			EndPos.z-=distance;
		break;
	}
	cb->CreateLineFigure(StartPos, EndPos, width, arrow, lifetime, group);
}

void cRAI::DebugDrawShape(float3 centerPos, float lineLength, float width, int arrow, float yPosOffset, int lifeTime, int sides, int group)
{
	DebugDrawLine(centerPos, lineLength, 0, -lineLength/2,  lineLength/2, yPosOffset, lifeTime, arrow, width, group);
	DebugDrawLine(centerPos, lineLength, 1, -lineLength/2, -lineLength/2, yPosOffset, lifeTime, arrow, width, group);
	DebugDrawLine(centerPos, lineLength, 2,  lineLength/2, -lineLength/2, yPosOffset, lifeTime, arrow, width, group);
	DebugDrawLine(centerPos, lineLength, 3,  lineLength/2,  lineLength/2, yPosOffset, lifeTime, arrow, width, group);
}

bool cRAI::LocateFile(IAICallback* cb, const string& relFileName, string& absFileName, bool forWriting) 
{
	int action = forWriting ? AIVAL_LOCATE_FILE_W : AIVAL_LOCATE_FILE_R;

	char absFN[2048];
	STRCPY_T(absFN, sizeof(absFN), relFileName.c_str());
	const bool located = cb->GetValue(action, absFN);

	if (located) {
		absFileName = absFN;
	} else {
		absFileName = "";
	}

	return located;
}

static bool IsFSGoodChar(const char c) {

	if ((c >= '0') && (c <= '9')) {
		return true;
	} else if ((c >= 'a') && (c <= 'z')) {
		return true;
	} else if ((c >= 'A') && (c <= 'Z')) {
		return true;
	} else if ((c == '.') || (c == '_') || (c == '-')) {
		return true;
	}

	return false;
}
std::string cRAI::MakeFileSystemCompatible(const std::string& str) {

	std::string cleaned = str;

	for (std::string::size_type i=0; i < cleaned.size(); i++) {
		if (!IsFSGoodChar(cleaned[i])) {
			cleaned[i] = '_';
		}
	}

	return cleaned;
}

void cRAI::RemoveLogFile(string relFileName) const {

	string absFileName;
	if (cRAI::LocateFile(cb, relFileName, absFileName, true)) {
		remove(absFileName.c_str());
	}
}

std::string cRAI::GetLogFileSubPath(int teamId) const {

	static const size_t logFileSubPath_sizeMax = 64;
	char logFileSubPath[logFileSubPath_sizeMax];
	SNPRINTF(logFileSubPath, logFileSubPath_sizeMax, "log/RAI%i_LastGame.log", teamId);
	return std::string(logFileSubPath);
}

void cRAI::ClearLogFiles()
{
	for( int t=0; t < 255; t++ )
	{
		RemoveLogFile(GetLogFileSubPath(t));
	}

	RemoveLogFile("log/RAIGlobal_LastGame.log");
	RemoveLogFile("log/TerrainMapDebug.log");
//	RemoveLogFile("log/PathfinderDebug.log");
//	RemoveLogFile("log/PathFinderAPNDebug.log");
//	RemoveLogFile("log/PathFinderNPNDebug.log");
//	RemoveLogFile("log/Prerequisite.log");
//	RemoveLogFile("log/Debug.log");
}
