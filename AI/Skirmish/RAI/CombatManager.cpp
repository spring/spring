#include "CombatManager.h"
#include "LegacyCpp/WeaponDef.h"
#include "LegacyCpp/CommandQueue.h"

cCombatManager::cCombatManager(IAICallback* callback, cRAI* Global)
{
	cb=callback;
	G=Global;
	l=G->l;
}

cCombatManager::~cCombatManager()
{
}

void cCombatManager::UnitIdle(const int& unit, UnitInfo *U)
{
//*l<<"(cui) -eID="<<U->enemyID;
	if( ValidateEnemy(unit,U) && CanAttack(U,U->E,GetEnemyPosition(U->enemyID,U->E)) == 0 )
		U->enemyID=-1;
	float3 fPos=cb->GetUnitPos(unit);
	if( U->enemyID == -1 )
		while( (U->enemyID=GetClosestEnemy(fPos,U)) > 0 && !ValidateEnemy(unit,U) ) {}
	float distance = -1;
	if( U->enemyID >= 0 )
	{
		distance = fPos.distance(GetEnemyPosition(U->enemyID,U->E));
		if( distance == 0 )
			distance = 1;
	}
	if( U->enemyID == -1 || (U->udrBL->task != TASK_ASSAULT && distance > 2.5*8*U->ud->losRadius) )
	{
		U->inCombat=false;
		G->UpdateEventAdd(1,0,unit,U);
		return;
	}
	else if( CommandDGun(unit,U) )
		return;
	else if( U->enemyEff == 0 || (U->udrBL->task != TASK_ASSAULT && distance > 1.75*U->enemyEff->BestRange ) || (U->ud->isCommander && cb->GetUnitHealth(unit)/U->ud->health <= 0.66 ) ) // || G->Enemies.find(U->enemyID)->second.ud->kamikazeDist > distance
	{
		float3 EPos = GetEnemyPosition(U->enemyID,U->E);
		CommandRun(unit,U,EPos);
		return;
	}
	else if( CommandCapture(unit,U,distance) || CommandManeuver(unit,U,distance) )
		return;
	else
	{
		float3 EPos = GetEnemyPosition(U->enemyID,U->E);
		Command c;
		if( U->ud->canAttack && (U->E->inLOS || U->E->inRadar) )
		{
			c = Command(CMD_ATTACK);
			c.PushParam(U->enemyID);
		}
		else if( U->ud->canAttack && (U->udr->IsBomber && U->E->posLocked) )
		{
			c = Command(CMD_ATTACK);
			c.PushPos(EPos);
		}
		else // cant see enemy or Mod Workaround: Combat Lords - cant be given attack orders
		{
			c = Command(CMD_MOVE);
			EPos.x += -100.0 +rand()%201;
			EPos.z += -100.0 +rand()%201;
			G->CorrectPosition(EPos);
			c.PushPos(EPos);
		}

		cb->GiveOrder(unit, &c);
		G->UpdateEventAdd(1,int(GetNextUpdate(distance,U)),unit,U);
	}
}

void cCombatManager::UnitDamaged(const int& unitID, UnitInfo* U, const int& attackerID, EnemyInfo* A, float3& dir)
{
	ValidateEnemy(unitID,U,false);
	if( attackerID >= 0 && attackerID != U->enemyID )
	{
		float3 Pos = cb->GetUnitPos(unitID);
		float3 APos = GetEnemyPosition(attackerID,A);
		if( U->enemyID == -1 || Pos.distance(APos) < Pos.distance(GetEnemyPosition(U->enemyID,U->E)) )
			if( CanAttack(U, A, APos) != 0 && (U->group == 0 || U->group->Enemies.find(attackerID) != U->group->Enemies.end()) )
			{
				U->enemyID=attackerID;
				U->E = A;
				U->enemyEff = CanAttack(U, A, APos);
			}
	}
	if( U->inCombat )
	{
		if( U->ud->isCommander )
		{
			if( int(cb->GetCurrentUnitCommands(unitID)->size()) == 0 )
				UnitIdle(unitID,U);
			else if( cb->GetCurrentUnitCommands(unitID)->front().GetID() != CMD_MOVE )
			{
				if( cb->GetUnitHealth(unitID)/U->ud->health <= 0.66 ||
					(cb->GetUnitHealth(unitID)/U->ud->health <= 0.9 && cb->GetCurrentUnitCommands(unitID)->front().GetID() == CMD_CAPTURE) )
					UnitIdle(unitID,U);
			}
		}
		return;
	}
	if( U->BuildQ != 0 && U->BuildQ->RS != 0 )
		U->BuildQ->tryCount = 4; // If the project is destroyed too many times, give up on it
	U->inCombat=true;
	if( U->enemyID == -1 )
	{
		if( attackerID >= 0 )
		{
			float3 APos = GetEnemyPosition(attackerID,A);
			CommandRun(unitID,U,APos);
		}
		else
		{
			float3 EPos = cb->GetUnitPos(unitID);
			EPos.x += dir.x*700;
			EPos.z += dir.z*700;
			EPos.y = cb->GetElevation(EPos.x,EPos.z);
			CommandRun(unitID,U,EPos);
		}
	}
	else
		UnitIdle(unitID,U);
}

bool cCombatManager::CommandDGun(const int& unitID, UnitInfo *U)
{
	if( U->udr->DGun == 0 || cb->GetEnergy() < U->udr->DGun->energycost )
		return false;

	float3 EPos = GetEnemyPosition(U->enemyID,U->E);
	float EDis = EPos.distance(cb->GetUnitPos(unitID));
	if( EDis > 1.05*U->udr->DGun->range )
		return false;

	if( U->ud->isCommander )
	{
		if( U->E->ud != 0 && U->E->ud->isCommander )
		{
			CommandRun(unitID,U,EPos);
			return true;
		}
	}
	Command c(CMD_DGUN);
	c.PushPos(EPos);
	cb->GiveOrder(unitID, &c);
	G->UpdateEventAdd(1,cb->GetCurrentFrame()+5,unitID,U);
	return true;
}

bool cCombatManager::CommandCapture(const int& unitID, UnitInfo* U, const float& EDis)
{
	if( !U->ud->canCapture ) //|| EDis > 1.5*U->ud->buildDistance )
		return false;

	if( U->ud->isCommander && cb->GetUnitHealth(unitID)/U->ud->health <= 0.9 )
		return false;

	if( !U->E->inLOS || (!cb->IsUnitParalyzed(U->enemyID) && 1.5*U->ud->speed < U->E->ud->speed) )
		return false;

	Command c(CMD_CAPTURE);
	c.PushParam(U->enemyID);
	cb->GiveOrder(unitID, &c);
	return true;
}
/*
bool cCombatManager::CommandTrap(const int& unitID, UnitInfo* U, const float& EDis)
{
	if( !U->E->inLOS || U->ud->transportMass < U->E->ud->mass )
		return false;
	if( U->ud->transportCapacity == 0 )
		return false;

	Command c(CMD_LOAD_UNITS);
	c.PushParam(U->enemyID);
	cb->GiveOrder(unitID, &c);
	return true;
}
*/
bool cCombatManager::CommandManeuver(const int& unitID, UnitInfo *U, const float& EDis)
{
	if( U->ud->canfly || U->E->ud == 0 || !U->E->inLOS || U->enemyEff->BestRange <= 1.15*cb->GetUnitMaxRange(U->enemyID) || EDis > 3500.0 || int(G->UMobile.size()) > 60 )
		return false;

	float3 Pos=cb->GetUnitPos(unitID);
	float3 EPos=GetEnemyPosition(U->enemyID,U->E);

	if( U->ud->minWaterDepth < 0 && Pos.y <= 0 && U->udr->WeaponSeaEff.BestRange == 0 )
	{
		int iS=G->TM->GetSectorIndex(EPos);
		if( G->TM->IsSectorValid(iS) )
		{
			Pos = G->TM->GetClosestSector(G->TM->landSectorType,iS)->position;
			Pos.x+=128-rand()%256;
			Pos.z+=128-rand()%256;
			G->CorrectPosition(Pos);
			Command c(CMD_MOVE);
			c.PushPos(Pos);
			cb->GiveOrder(unitID, &c);
			G->UpdateEventAdd(1,int(GetNextUpdate(EDis,U)),unitID,U);
			return true;
		}
	}
	if( EDis < 0.70*U->enemyEff->BestRange || EDis > U->enemyEff->BestRange )
	{
		float distanceAway=(0.87*U->enemyEff->BestRange-EDis);
		Pos.x+=(Pos.x-EPos.x)*(distanceAway/EDis);
		Pos.z+=(Pos.z-EPos.z)*(distanceAway/EDis);
		G->CorrectPosition(Pos);

		if( !G->TM->CanMoveToPos(U->area,Pos) )
			return false;

		Command c(CMD_MOVE);
		c.PushParam(Pos.x);
		c.PushParam(cb->GetElevation(Pos.x,Pos.z));
		c.PushParam(Pos.z);
		cb->GiveOrder(unitID, &c);
		G->UpdateEventAdd(1,int(GetNextUpdate(EDis,U)),unitID,U);
		return true;
	}
	return false;
}

void cCombatManager::CommandRun(const int& unitID, UnitInfo *U, float3& EPos)
{
	float3 Pos=cb->GetUnitPos(unitID);
	Pos.x+=Pos.x-EPos.x;
	Pos.z+=Pos.z-EPos.z;
	G->CorrectPosition(Pos);
	Command c(CMD_MOVE);
	c.PushPos(Pos);
	cb->GiveOrder(unitID, &c);
	G->UpdateEventAdd(1,cb->GetCurrentFrame()+210,unitID,U);
}

int cCombatManager::GetClosestEnemy(float3 Pos, UnitInfo* U)
{
	U->enemyID=-1;
	// these two function need improvement, for now I'll just use a short cut
	if( !G->UM->ActiveAttackOrders() && U->udrBL->task != TASK_SUICIDE )
		return GetClosestThreat(Pos, U);
	sWeaponEfficiency* weTemp;
	float distance,fTemp;
	float3 fE;
	distance=0.0f;
	for( map<int,EnemyInfo>::iterator E=G->Enemies.begin(); E!=G->Enemies.end(); ++E )
	{
		fE=GetEnemyPosition(E->first,&E->second);
		if( (weTemp = CanAttack(U,&E->second,fE)) != 0 )
		{
			fTemp=Pos.distance(fE);
			if( U->enemyID == -1 || fTemp < distance )
			{
				U->enemyID=E->first;
				U->E = &E->second;
				U->enemyEff = weTemp;
				distance=fTemp;
			}
		}
	}
	if( U->enemyID != -1 && U->group != 0 )
		G->UM->GroupAddEnemy(U->enemyID,U->E,U->group);
	return U->enemyID;
}

int cCombatManager::GetClosestThreat(float3 Pos, UnitInfo* U)
{
	sWeaponEfficiency* weTemp;
	float distance,fTemp;
	distance=0.0f;
	float3 fE;
	set<int> deletion;
	for( map<int,EnemyInfo*>::iterator E=G->EThreat.begin(); E!=G->EThreat.end(); ++E )
	{
		fE=GetEnemyPosition(E->first,E->second);
		if( E->second->baseThreatFrame > cb->GetCurrentFrame()+3600 ||
			(E->second->baseThreatFrame > cb->GetCurrentFrame()+1200 && G->UImmobile.find(E->second->baseThreatID) == G->UImmobile.end() ) ||
			(E->second->ud != 0 && G->UImmobile.find(E->second->baseThreatID) != G->UImmobile.end() && 1.3*E->second->ud->maxWeaponRange < fE.distance(cb->GetUnitPos(E->second->baseThreatID)) ) )
		{
			E->second->baseThreatID = -1;
			E->second->baseThreatFrame = -1;
			deletion.insert(E->first);
		}
		else if( (weTemp = CanAttack(U,E->second,fE)) != 0 )
		{
			fTemp=Pos.distance(fE);
			if( U->enemyID == -1 || fTemp < distance )
			{
				U->enemyID=E->first;
				U->E = E->second;
				U->enemyEff = weTemp;
				distance=fTemp;
			}
		}
	}
	while( int(deletion.size()) > 0 )
	{
		if( !G->UM->ActiveAttackOrders() )
		{
			EnemyInfo* E = G->EThreat.find(*deletion.begin())->second;
			while( int(E->attackGroups.size()) > 0 )
				G->UM->GroupRemoveEnemy(*deletion.begin(),E,*E->attackGroups.begin());
		}
		G->EThreat.erase(*deletion.begin());
		deletion.erase(*deletion.begin());
	}
	if( U->enemyID != -1 && U->group != 0 )
		G->UM->GroupAddEnemy(U->enemyID,U->E,U->group);
	return U->enemyID;
}

float3 cCombatManager::GetEnemyPosition(const int& enemyID, EnemyInfo* E)
{
	if( E->posLocked || (!E->inLOS && !E->inRadar) )
		return E->position;
	return cb->GetUnitPos(enemyID);
}

float cCombatManager::GetNextUpdate(const float &Distance, UnitInfo* U)
{
	if( U->ud->speed == 0.0 )
		return cb->GetCurrentFrame()+90.0;
	float fFrame=30.0*((Distance-U->enemyEff->BestRange)/(5.0*U->ud->speed));
	if( int(G->UMobile.size()) > 45 )
		fFrame*=3;
	if( fFrame > 90.0 )
		return cb->GetCurrentFrame()+fFrame;
	return cb->GetCurrentFrame()+90.0;
}

sWeaponEfficiency* cCombatManager::CanAttack(UnitInfo* U, EnemyInfo *E, const float3& EPos)
{
	if( !G->TM->CanMoveToPos(U->area,EPos) )
		return 0;
	float fElevation=cb->GetElevation(EPos.x,EPos.z);
	if( EPos.y < 0.0 && U->udr->WeaponSeaEff.BestRange > 0 )
		return &U->udr->WeaponSeaEff;
	if( EPos.y-fElevation>50.0 && U->udr->WeaponAirEff.BestRange > 0 )
		return &U->udr->WeaponAirEff;
	if( EPos.y-fElevation<=50.0 && EPos.y >= -15.0 && U->udr->WeaponLandEff.BestRange > 0 )
		return &U->udr->WeaponLandEff;
	return 0;
}

bool cCombatManager::ValidateEnemy(const int& unitID, UnitInfo* U, bool IdleIfInvalid)
{
	if( U->enemyID == -1 || G->Enemies.find(U->enemyID) == G->Enemies.end() )
	{	// old enemy target that doesn't exist
		U->enemyID=-1;
		if( IdleIfInvalid )
			G->UpdateEventAdd(1,cb->GetCurrentFrame()+90,unitID,U);
		return false;
	}
	float3 EPos = cb->GetUnitPos(U->enemyID);
	if( U->group == 0 )
	{	// keeping variables up-to-date, this is event-driven for groups
		U->E = &G->Enemies.find(U->enemyID)->second;
		U->enemyEff = CanAttack(U,U->E,EPos);
	}
	if( cb->GetUnitDef(U->enemyID) != 0 && cb->GetUnitAllyTeam(unitID) == cb->GetUnitAllyTeam(U->enemyID) )
	{	// an enemy ID was reused by an ally team
		if( U->E->inLOS || U->E->inRadar ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
		{	// OR an ally captures an enemy unit & no events were sent
			*l<<"\nWARNING: ValidateEnemy(eID="<<U->enemyID<<"): an ally has captured an enemy unit";
		}
		G->EnemyDestroyed(U->enemyID,-1);
		U->enemyID=-1;
		return false;
	}
	if( EPos.x > 0 || EPos.z > 0 || EPos.y > 0 ) // Position is valid
	{
		if( !U->E->inLOS && !U->E->inRadar ) // ! Work Around:  Spring-Version(v0.72b1-0.76b1)
		{
			if( cb->GetUnitDef(U->enemyID) != 0 )
			{
				*l<<"\nWARNING: ValidateEnemy(eID="<<U->enemyID<<"): incorrect LOS status";
				G->EnemyEnterLOS(U->enemyID);
			}
			else
			{
				*l<<"\nWARNING: ValidateEnemy(eID="<<U->enemyID<<"): incorrect radar status";
				G->EnemyEnterRadar(U->enemyID);
			}
		}
		return true;
	}
	if( !U->E->inLOS && !U->E->inRadar && cb->GetUnitPos(unitID).distance2D(U->E->position) > 300.0f )
		return true;
	G->EnemyRemove(U->enemyID,U->E);
	U->enemyID=-1;
	if( IdleIfInvalid )
		G->UpdateEventAdd(1,cb->GetCurrentFrame()+90,unitID,U);
	return false;
}
