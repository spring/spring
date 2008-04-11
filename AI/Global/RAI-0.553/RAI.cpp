#include "RAI.h"
#include "ExternalAI/IGlobalAICallback.h"
//#include "ExternalAI/IAICheats.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/MoveTypes/MoveInfo.h"
//#include "LogFile.h"
//#include <vector>
//#include <iostream>

namespace std
{
	void _xlen(){};
}

UnitInfo::UnitInfo(sRAIUnitDef *runitdef)
{
	udr=runitdef;
	ud=runitdef->ud;

	AIDisabled=false;
	HumanOrder=false;
	UnitFinished=false;
	bInCombat=false;
	lastUnitIdleFrame=0;
	lastUnitDamagedFrame=0;
	commandTimeOut=-1;
	enemyID=-1;
	E=0;
	enemyEff=0;
	pBOL=0;
	RS=0;
	BuildQ=0;
	CloakUI=0;
	PowerUI=0;
	SWeaponUI=0;
	Group=0;
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
}

cRAI::~cRAI()
{
	*l<<"\n\nSHUTING DOWN ...";
	*l<<"\n cRAI Debug:";

	*l<<"\n  clearing Units size="<<int(Units.size())<<": ";
	while( int(Units.size()) > 0 )
		UnitDestroyed(Units.begin()->first,-1);

	*l<<"\n  clearing Enemies size="<<Enemies.size()<<": ";
	while( int(Enemies.size()) > 0 )
		EnemyDestroyed(Enemies.begin()->first,-1);

	int ERadar,ELOS;
	*l<<"\n Enemys (Enter LOS - Errors)                        = "<<DebugEnemyEnterLOS<<" = "<<DebugEnemyEnterLOS+DebugEnemyEnterLOSError<<"-"<<DebugEnemyEnterLOSError;
	*l<<"\n Enemys (Leave LOS + Destroyed in LOS - Errors)     = "<<DebugEnemyLeaveLOS+DebugEnemyDestroyedLOS<<" = "<<DebugEnemyLeaveLOS+DebugEnemyLeaveLOSError<<"+"<<DebugEnemyDestroyedLOS<<"-"<<DebugEnemyLeaveLOSError;
	*l<<"\n Enemies Remaining in LOS                           = "<<(ELOS=DebugEnemyEnterLOS-DebugEnemyLeaveLOS-DebugEnemyDestroyedLOS);
	*l<<"\n Enemys (Enter Radar - Errors)                      = "<<DebugEnemyEnterRadar<<" = "<<DebugEnemyEnterRadar+DebugEnemyEnterRadarError<<"-"<<DebugEnemyEnterRadarError;
	*l<<"\n Enemys (Leave Radar + Destroyed in Radar - Errors) = "<<DebugEnemyLeaveRadar+DebugEnemyDestroyedRadar<<" = "<<DebugEnemyLeaveRadar+DebugEnemyLeaveRadarError<<"+"<<DebugEnemyDestroyedRadar<<"-"<<DebugEnemyLeaveRadarError;
	*l<<"\n Enemies Remaining in Radar                         = "<<(ERadar=DebugEnemyEnterRadar-DebugEnemyLeaveRadar-DebugEnemyDestroyedRadar);
	*l<<"\n Enemies Remaining                                  = "<<int(Enemies.size());
	*l<<"\n Units Remaining                                    = "<<UnitM->GroupSize;
	*l<<"\n Groups Remaining                                   = "<<UnitM->GroupSize;
	if( UnitM->GroupSize != 0 || ELOS != 0 || ERadar != 0 )
		*l<<"\n ERROR!";

	delete UnitM;
	delete Build;
	delete SWM;
	delete CombatM;

	RG->AIs--;
	if( RG->AIs == 0 )
	{
	*l<<"\n deleting RAIGlobal ...";
		delete RAIGlobal;
		RAIGlobal=0;
	}

	*l<<"\n deleting UDH ...";
	delete UDH;

	*l<<"\nSHUTDOWN Complete.";

	delete l;
}

void cRAI::InitAI(IGlobalAICallback* callback, int team)
{
	cb=callback->GetAICallback();

	if( RAIGlobal == 0 )
		RAIGlobal = new cRAIGlobal(cb);

	char c[2];
	sprintf(c, "%i", cb->GetMyTeam());
	l=new cLogFile("RAI"+string(c)+"_LastGame.log",false);

	*l<<"Loading ...";
	*l<<"\n Team = "<<cb->GetMyTeam();
	*l<<"\n Ally Team = "<<cb->GetMyAllyTeam();
	if( RAIGlobal == 0 )
	{
		*l<<"\nLoading RAIGlobal ...";
		RAIGlobal = new cRAIGlobal(cb);
	}
	RG = RAIGlobal;
	TM = RG->TM;
	RG->AIs++;

	*l<<"\nLoading cRAIUnitDefHandler ...";
	UDH = new cRAIUnitDefHandler(cb,RG,l);

	UnitM = new cUnitManager(cb,this);
	Build = new cBuilder(cb,this);
	SWM = new cSWeaponManager(cb,this);
	CombatM = new cCombatManager(cb,this);
	*l<<"\nLoading Complete.";
/*
	if( cb->GetUnitDef("arm_retaliator") != 0 ) // XTA Cheat Nuke Test
	{
		IAICheats *cheat=callback->GetCheatInterface();
		float3 pos = *cb->GetStartPos();
		float3 pos2 = *cb->GetStartPos();
		float3 pos3 = *cb->GetStartPos();
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

void cRAI::UnitCreated(int unit)
{
	const UnitDef* ud=cb->GetUnitDef(unit);
//	*l<<"\nUnitCreated("; *l<<unit<<")["+ud->humanName+"]";
	Units.insert(iuPair(unit,UnitInfo(&UDH->UDR.find(ud->id)->second)));
	UnitInfo* U = &Units.find(unit)->second;
	U->mapBody = GetCurrentMapBody(ud,cb->GetUnitPos(unit));

	if( ud->speed == 0 )
	{
		for(map<int,UnitInfo*>::iterator i=UImmobile.begin(); i!=UImmobile.end(); i++ )
		{
			if( U->udr->WeaponGuardRange > 0 && i->second->udr->WeaponGuardRange == 0 && cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(i->first)) < U->udr->WeaponGuardRange )
			{
				U->UDefending.insert(cRAI::iupPair(i->first,i->second));
				i->second->UDefences.insert(cRAI::iupPair(unit,U));
			}
			else if( U->udr->WeaponGuardRange == 0 && i->second->udr->WeaponGuardRange > 0 && cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(i->first)) < i->second->udr->WeaponGuardRange )
			{
				U->UDefences.insert(cRAI::iupPair(i->first,i->second));
				i->second->UDefending.insert(cRAI::iupPair(unit,U));
			}
		}
		UImmobile.insert(iupPair(unit,U));
		if( U->ud->minWaterDepth >= 0 )
			UImmobileWater.insert(unit);
	}
	else
		UMobile.insert(iupPair(unit,U));

	Build->UnitCreated(unit,U);
	Build->BP->UResourceCreated(unit,U);
//	*l<<" UC(end)";
}

void cRAI::UnitFinished(int unit)
{
//	*l<<"\nUnitFinished("<<unit<<")";
	if( Units.find(unit) == Units.end() ) // Occurs if a player canceled a build order with more than one quaried and something still finished
	{
		UnitCreated(unit);
	}

	UnitInfo *U = &Units.find(unit)->second;
	U->UnitFinished = true;
	if( U->AIDisabled )
	{
//		*l<<" UF(end)";
		return;
	}

	Build->UnitFinished(unit,U);
	if( U->AIDisabled )
	{
//		*l<<" UF(end)";
		return;
	}

	Build->PM->UnitFinished(unit,U);
	SWM->UnitFinished(unit,U->udr);
	UnitM->UnitFinished(unit,U);

	if( U->ud->highTrajectoryType == 2 && rand()%4 == 0 )
	{
		Command c;
		c.id = CMD_TRAJECTORY;
		c.params.push_back(1);
		cb->GiveOrder(unit,&c);
	}
//	*l<<" UF(end)";
}

void cRAI::UnitDestroyed(int unit,int attacker)
{
//	*l<<"\nUnitDestroyed("<<unit<<","<<attacker<<")";
	if( Units.find(unit) == Units.end() ) // Occurs if a player canceled a build order with more than one quaried and something was still being worked on
	{
//		*l<<"(PC)UDe(end)";
		return;
	}
	UnitInfo *U = &Units.find(unit)->second;

	if( !U->AIDisabled )
	{
		Build->UnitDestroyed(unit,U);
		if( U->UnitFinished )
		{
			Build->PM->UnitDestroyed(unit,U);
			SWM->UnitDestroyed(unit);
			UnitM->UnitDestroyed(unit,U);
		}
	}
	Build->BP->UResourceDestroyed(unit,U);
	if( U->ud->speed == 0 )
	{
		for(map<int,UnitInfo*>::iterator i=U->UDefending.begin(); i!=U->UDefending.end(); i++ )
			i->second->UDefences.erase(unit);
		for(map<int,UnitInfo*>::iterator i=U->UDefences.begin(); i!=U->UDefences.end(); i++ )
			i->second->UDefending.erase(unit);
		UImmobile.erase(unit);
		if( U->ud->minWaterDepth >= 0 )
			UImmobileWater.erase(unit);
	}
	else
		UMobile.erase(unit);
	Units.erase(unit);
//	*l<<" UDe(end)";
}

void cRAI::EnemyEnterLOS(int enemy)
{
//	*l<<"\nEnemyEnterLOS("<<enemy<<")";
	if( cb->GetUnitHealth(enemy) <= 0 ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		DebugEnemyEnterLOSError++;
		*l<<"\nWARNING: EnemyEnterLOS(): enemy is either dead or not in LOS";
//		*l<<" EEL(end)";
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

	UnitM->EnemyEnterLOS(enemy,E);
	Build->BP->EResourceEnterLOS(enemy,E);
//	*l<<" EEL(end)";
}

void cRAI::EnemyLeaveLOS(int enemy)
{
//	*l<<"\nEnemyLeaveLOS("<<enemy<<")";
	if( Enemies.find(enemy) == Enemies.end() ) // ! Work Around  Spring Version: v0.72b1-0.75b2
	{
		DebugEnemyLeaveLOSError++;
		*l<<"\nWARNING: EnemyLeaveLOS(): unknown unit id";
//		*l<<" ELL(end)";
		return;
	}
	EnemyInfo *E = &Enemies.find(enemy)->second;
	if( !E->inLOS ) // Spring Version: v0.76b1
	{
		DebugEnemyLeaveLOSError++;
		*l<<"\nWARNING: EnemyLeaveLOS(): not in LOS";
//		*l<<" ELL(end)";
		return;
	}

	DebugEnemyLeaveLOS++;
	E->inLOS=false;
	if( !E->inRadar )
	{
		if( !E->posLocked )
			E->position = cb->GetUnitPos(enemy);
		if( !TM->SectorValid(TM->GetSector(E->position)) )
			EnemyRemove(enemy,E);
	}
//	*l<<" ELL(end)";
}

void cRAI::EnemyEnterRadar(int enemy)
{
//	*l<<"\nEnemyEnterRadar("<<enemy<<")";
	if( cb->GetUnitPos(enemy).x <= 0 && cb->GetUnitPos(enemy).y <= 0 && cb->GetUnitPos(enemy).z <= 0 ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		DebugEnemyEnterRadarError++;
		*l<<"\nWARNING: EnemyEnterRadar(): enemy position is invalid";
//		*l<<" EER(end)";
		return;
	}
	DebugEnemyEnterRadar++;

	if( Enemies.find(enemy) == Enemies.end() )
		Enemies.insert(iePair(enemy,EnemyInfo()));
	EnemyInfo *E = &Enemies.find(enemy)->second;
	E->inRadar=true;

	UnitM->EnemyEnterRadar(enemy,E);
//	*l<<" EER(end)";
}

void cRAI::EnemyLeaveRadar(int enemy)
{
//	*l<<"\nEnemyLeaveRadar("<<enemy<<")";
	if( Enemies.find(enemy) == Enemies.end() ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		DebugEnemyLeaveRadarError++;
		*l<<"\nWARNING: EnemyLeaveRadar(): unknown unit id";
//		*l<<" ELR(end)";
		return;
	}
	EnemyInfo *E = &Enemies.find(enemy)->second;
	if( !E->inRadar ) // Spring Version: v0.76b1
	{
		DebugEnemyLeaveRadarError++;
		*l<<"\nWARNING: EnemyLeaveRadar(): not in radar";
//		*l<<" ELR(end)";
		return;
	}

	DebugEnemyLeaveRadar++;
	E->inRadar=false;
	if( !E->inLOS )
	{
		if( !E->posLocked )
			E->position = cb->GetUnitPos(enemy);
		if( !TM->SectorValid(TM->GetSector(E->position)) )
			EnemyRemove(enemy,E);
	}
//	*l<<" ELR(end)";
}

void cRAI::EnemyDestroyed(int enemy,int attacker)
{
//	*l<<"\nEnemyDestroyed("<<enemy<<","<<attacker<<")";
	if( Enemies.find(enemy) == Enemies.end() ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		*l<<"\nWARNING: EnemyDestroyed(): unknown unit id";
//		*l<<" ED(end)";
		return;
	}

	EnemyInfo *E = &Enemies.find(enemy)->second;
	if( E->inLOS )
		DebugEnemyDestroyedLOS++;
	if( E->inRadar )
		DebugEnemyDestroyedRadar++;
	EnemyRemove(enemy,E);
//	*l<<" ED(end)";
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
	while( int(E->AttackGroups.size()) > 0 )
		UnitM->GroupRemoveEnemy(enemy,E,E->AttackGroups.begin()->second);
	Enemies.erase(enemy);
}

void cRAI::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{

}

void cRAI::UnitIdle(int unit)
{
//	*l<<"\nUI("<<unit<<")";
	if( Units.find(unit) == Units.end() ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		*l<<"\nWARNING: UnitIdle(): unknown unit id";
//		*l<<" UI(end)";
		return;
	}
	UnitInfo *U = &Units.find(unit)->second;
	if( U->AIDisabled || cb->UnitBeingBuilt(unit) || cb->IsUnitParalyzed(unit) )
	{
//		*l<<" UI(end)";
		return;
	}
	if( int(cb->GetCurrentUnitCommands(unit)->size()) > 0 )
	{
//		*l<<" UI(end)";
		return;
	}
	U->HumanOrder=false;

	if( cb->GetCurrentFrame() <= U->lastUnitIdleFrame+15 ) // !! Occurs if enemy attack order fails/...possibly some other reason
	{
		U->commandTimeOut=cb->GetCurrentFrame()+15;
//		*l<<" UI(end)";
		return;
	}
	U->lastUnitIdleFrame=cb->GetCurrentFrame();
	U->commandTimeOut=-1;
	if( U->bInCombat )
	{
		CombatM->UnitIdle(unit,U);
//		*l<<" UI(end)";
		return;
	}
	UnitM->UnitIdle(unit,U);
//	*l<<" UI(end)";
}

void cRAI::GotChatMsg(const char* msg,int player)
{

}

void cRAI::UnitDamaged(int unit,int attacker,float damage,float3 dir)
{
//	*l<<"\nUnitDamaged("<<unit<<","<<attacker<<","<<damage<<",x"<<dir.x<<" z"<<dir.z<<" y"<<dir.y<<")";
	if( cb->UnitBeingBuilt(unit) || cb->IsUnitParalyzed(unit) || cb->GetUnitHealth(unit) <= 0.0 )
	{
//		*l<<" UDa(end)";
		return;
	}

	UnitInfo *U = &Units.find(unit)->second;
	if( cb->GetCurrentFrame() <= U->lastUnitDamagedFrame+15 )
	{
//		*l<<" UDa(end)";
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
				for( int i=0; i<UnitM->GroupSize; i++ )
				{
					if( int(UnitM->Group[i]->Enemies.size()) == 0 && !UnitM->Group[i]->Units.begin()->second->ud->canLoopbackAttack )
					{
						int enemyID = CombatM->GetClosestEnemy(cb->GetUnitPos(UnitM->Group[i]->Units.begin()->first),UnitM->Group[i]->Units.begin()->second);
					}
				}
			}
		}
		ValidateUnitList(&U->UGuards);
		for( map<int,UnitInfo*>::iterator i = U->UGuards.begin(); i != U->UGuards.end(); i++ )
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
	if( U->AIDisabled || U->pBOL->task<=0 || U->ud->speed==0 )
	{
//		*l<<" UDa(end)";
		return;
	}

	if( E != 0 && U->Group != 0 && U->Group->Enemies.find(attacker) == U->Group->Enemies.end() )
	{
		if( E->baseThreatID != -1 || UnitM->ActiveAttackOrders() )
			UnitM->GroupAddEnemy(attacker,E,U->Group);
	}

	if( !IsHumanControled(unit,U) )
		CombatM->UnitDamaged(unit,U,attacker,E,dir);
//	*l<<" UDa(end)";
}

void cRAI::UnitMoveFailed(int unit)
{
//	*l<<"\nUnitMoveFailed("<<unit<<")";
	if( UMobile.find(unit) == UMobile.end() ) // ! Work Around   Spring Version: v0.72b1-0.74b1
	{
//		*l<<" UMF(end)";
		return;
	}

	UnitInfo *U = UMobile.find(unit)->second;
	if( U->AIDisabled || cb->IsUnitParalyzed(unit) ||
		Build->UBuilderMoveFailed(unit,U) || UnitM->UnitMoveFailed(unit,U) ||
		int(cb->GetCurrentUnitCommands(unit)->size()) > 0 )
	{
//		*l<<" UMF(end)";
		return;
	}

//	*l<<U->ud->humanName+" is waiting.";
	Command c;
	c.id=CMD_WAIT;
	//c.timeOut=cb->GetCurrentFrame()+120;
	cb->GiveOrder(unit,&c);
	U->commandTimeOut=cb->GetCurrentFrame()+90;
//	*l<<" UMF(end)";
}

int cRAI::HandleEvent(int msg,const void* data)
{
//	*l<<"\nHandleEvent("<<msg<<","<<"~"<<")";
	switch (msg)
	{
	case AI_EVENT_UNITGIVEN:
		{
			const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;
			if( cte->newteam != cb->GetMyTeam() )
			{
				cb->SendTextMsg("cRAI::HandleEvent-AI_EVENT_UNITGIVEN: This AI is out of date, check for a more recent one.",0);
				*l<<"\nERROR: cRAI::HandleEvent-AI_EVENT_UNITGIVEN: This AI is out of date, check for a more recent one.\n";
			}

			if( Enemies.find(cte->unit) != Enemies.end() )
				EnemyDestroyed(cte->unit,-1);

			if( cb->GetUnitHealth(cte->unit) <= 0 ) // ! Work Around   Spring Version: v0.74b1-0.75b2
			{
				*l<<"\nWARNING: HandleEvent-AI_EVENT_UNITGIVEN: Given Unit is Dead";
				return 0;
			}

			UnitCreated(cte->unit);
			Units.find(cte->unit)->second.AIDisabled=false;
			Units.find(cte->unit)->second.commandTimeOut = 0;
			if( !cb->UnitBeingBuilt(cte->unit) )
				UnitFinished(cte->unit);
		}
		break;
	case AI_EVENT_UNITCAPTURED:
		{
			const IGlobalAI::ChangeTeamEvent* cte = (const IGlobalAI::ChangeTeamEvent*) data;
			if( cte->oldteam != cb->GetMyTeam() )
			{
				cb->SendTextMsg("cRAI::HandleEvent-AI_EVENT_UNITCAPTURED: This AI is out of date, check for a more recent one.",0);
				*l<<"\nERROR: cRAI::HandleEvent-AI_EVENT_UNITCAPTURED: This AI is out of date, check for a more recent one.\n";
			}

			UnitDestroyed(cte->unit,-1);
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
				if( Units.find(pce->units.at(i)) == Units.end() ) // ! Work Around   Spring Version: v0.75b2
				{
					*l<<"\nWARNING: HandleEvent-AI_EVENT_PLAYER_COMMAND: unknown unit id="<<pce->units.at(i);
//					pce->units.erase(pce->units.begin()+i);
//					i--;
				}
				else if( ImportantCommand )
					Units.find(pce->units.at(i))->second.HumanOrder = true;
			}
			if( ImportantCommand )
			{
				Build->HandleEvent(pce);
			}
			else if( pce->command.id == CMD_SELFD )
			{
				for( vector<int>::const_iterator i=pce->units.begin(); i!=pce->units.end(); i++ )
					UnitDestroyed(*i,-1);
			}
		}
		break;
	}
//	*l<<" HE(end)";
	return 0;
}

void cRAI::Update()
{
	frame=cb->GetCurrentFrame();
	if(!Build->bInitiated)
	{
		if( frame > 210 || ( cb->GetMetalIncome()>0 && cb->GetMetalIncome()<0.9*cb->GetMetalStorage() ) || ( cb->GetEnergyIncome()>0 && cb->GetEnergyIncome()<0.9*cb->GetEnergyStorage() ) )
		{
			*l<<"\nbInitiated=true - Frame="<<frame<<" Metal Income="<<cb->GetMetalIncome()<<" Energy Income="<<cb->GetEnergyIncome()<<"\n";
			Build->UpdateUDRCost();
				for(map<int,UnitInfo>::iterator i=Units.begin(); i!=Units.end(); i++ )
					if( cb->GetCurrentUnitCommands(i->first)->size() == 0 && !i->second.AIDisabled )
						UnitIdle(i->first);
			Build->bInitiated=true;
		}
	}
	if(frame%FUPDATE_MINIMAL)
	{
		return;
	}

//	*l<<"\nUpdate()";
	ValidateAllUnits();
	for(map<int,UnitInfo>::iterator iU=Units.begin(); iU!=Units.end(); iU++)
	{
		if( !iU->second.AIDisabled && !cb->UnitBeingBuilt(iU->first) && iU->second.pBOL->task > 0 )
		{
			if( iU->second.commandTimeOut >= 0 && frame >= iU->second.commandTimeOut && !IsHumanControled(iU->first,&iU->second) )
			{
//				*l<<"\nStopping "<<iU->second.ud->humanName<<".";
				Command c;
				c.id=CMD_STOP;
				cb->GiveOrder(iU->first, &c);
				iU->second.commandTimeOut=-1;
			}
			else if( frame > iU->second.lastUnitIdleFrame+FUPDATE_UNITS && cb->GetCurrentUnitCommands(iU->first)->size() == 0 )
			{
//				*l<<"\nUnit was Idle  Name="<<iU->second.ud->name;
				UnitIdle(iU->first);
			}
		}
	}

	if(!(frame%FUPDATE_POWER))
	{
		Build->PM->Update();
		SWM->Update();
	}
	if(!(frame%FUPDATE_BUILDLIST))	// Currently every 1 minute
	{
		Build->UpdateUDRCost();
	}
//	*l<<" U(end)";
}

void cRAI::CorrectPosition(float3* Position)
{
	if( Position->x < 1 )
		Position->x = 1;
	else if( Position->x > 8*cb->GetMapWidth()-1 )
		Position->x = 8*cb->GetMapWidth()-1;
	if( Position->z < 1 )
		Position->z = 1;
	else if( Position->z > 8*cb->GetMapHeight()-1 )
		Position->z = 8*cb->GetMapHeight()-1;
	Position->y = cb->GetElevation(Position->x,Position->z);
}

int cRAI::GetCurrentMapBody(const UnitDef* ud, float3 Position)
{
	if( ud->canfly || ud->canhover )
		return -1;

	if( ud->movedata == 0 && ud->minWaterDepth < 0 && -ud->maxWaterDepth <= TM->MaxWaterDepth )
		return -1;

	if( ud->movedata != 0 && ud->minWaterDepth < 0 && -ud->movedata->depth <= TM->MaxWaterDepth )
		return -1;

	int iS = TM->GetSector(Position);
	if( !TM->SectorValid(iS) )
	{
		CorrectPosition(&Position);
		iS = TM->GetSector(Position);
	}
	for( map<int,MapBody*>::iterator iB=TM->Sector[iS].mb.begin(); iB!=TM->Sector[iS].mb.end(); iB++ )
	{
		if( iB->second->Water && ud->minWaterDepth >= 0 )
			return iB->first;
		else if( !iB->second->Water && ud->minWaterDepth < 0 )
			return iB->first;
	}
	return -2;
}

float3 cRAI::GetRandomPosition(int MapBody)
{
	float3 Pos;
	if( MapBody < 0 )
	{
		Pos.x=1.0 + rand()%7 + 8.0*(rand()%cb->GetMapWidth());
		Pos.z=1.0 + rand()%7 + 8.0*(rand()%cb->GetMapHeight());
		Pos.y=cb->GetElevation(Pos.x,Pos.z);
		return Pos;
	}

	vector<int> Temp;
	for( map<int,MapSector*>::iterator iS=TM->MapB[MapBody]->Sector.begin(); iS!=TM->MapB[MapBody]->Sector.end(); iS++ )
		Temp.push_back(iS->first);
	int iS=Temp.at(rand()%int(Temp.size()));
	Pos.x=TM->Sector[iS].Pos.x - 127.0 + rand()%255;
	Pos.z=TM->Sector[iS].Pos.z - 127.0 + rand()%255;
	Pos.y=cb->GetElevation(Pos.x,Pos.z);
	return Pos;
}

bool cRAI::ValidateUnit(const int& unitID)
{
	if( cb->GetUnitDef(unitID) == 0 ) // ! Work Around   Spring Version: v0.74b1-0.75b2
	{
		*l<<"\nWARNING: ValidateUnit(): iU->first="<<unitID;
		UnitDestroyed(unitID,-1);
		return false;
	}
	return true;
}

bool cRAI::ValidateUnitList(map<int,UnitInfo*> *UL)
{
	int ULsize = UL->size();
	for(map<int,UnitInfo*>::iterator iU=UL->begin(); iU!=UL->end(); iU++)
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
	for(map<int,UnitInfo>::iterator iU=Units.begin(); iU!=Units.end(); iU++)
	{
		if( !ValidateUnit(iU->first) )
		{
			// The iterator has becomes invalid at this point
			ValidateAllUnits();
			return;
		}
	}
}

bool cRAI::IsHumanControled(const int& unit,UnitInfo *U)
{
	if( int(cb->GetCurrentUnitCommands(unit)->size()) == 0 )
		return false;
	if( U->HumanOrder )
		return true;
	return false;
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
	cb->CreateLineFigure(StartPos,EndPos,width,arrow,lifetime,group);
}

void cRAI::DebugDrawShape(float3 CenterPos, float linelength, float width, int arrow, float yposoffset, int lifetime, int sides, int group)
{
	DebugDrawLine(CenterPos,linelength,0,-linelength/2,linelength/2,yposoffset,lifetime,arrow,width,group);
	DebugDrawLine(CenterPos,linelength,1,-linelength/2,-linelength/2,yposoffset,lifetime,arrow,width,group);
	DebugDrawLine(CenterPos,linelength,2,linelength/2,-linelength/2,yposoffset,lifetime,arrow,width,group);
	DebugDrawLine(CenterPos,linelength,3,linelength/2,linelength/2,yposoffset,lifetime,arrow,width,group);
}
