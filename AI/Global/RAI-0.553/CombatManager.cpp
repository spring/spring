#include "CombatManager.h"
#include "Sim/Weapons/WeaponDefHandler.h"

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
	if( ValidateEnemy(unit,U) )
	{
		if( CanAttack(U,U->E,GetEnemyPosition(U->enemyID,U->E)) == 0 )
		{
			U->enemyID=-1;
		}
	}
	float3 fPos=cb->GetUnitPos(unit);
	if( U->enemyID == -1 )
	{
		while( (U->enemyID=GetClosestEnemy(fPos,U)) > 0 && !ValidateEnemy(unit,U) ) {}
	}
	float fDis = -1;
	if( U->enemyID >= 0 )
	{
		fDis = fPos.distance(GetEnemyPosition(U->enemyID,U->E));
		if( fDis == 0 )
			fDis = 1;
	}
	if( U->enemyID == -1 || (U->pBOL->task != TASK_ASSAULT && fDis > 2.5*8*U->ud->losRadius) )
	{
		U->bInCombat=false;
		U->commandTimeOut=cb->GetCurrentFrame();
		return;
	}
	else if( CommandDGun(unit,U) )
		return;
	else if( U->enemyEff == 0 || (U->pBOL->task != TASK_ASSAULT && fDis > 1.75*U->enemyEff->BestRange ) || (U->ud->isCommander && cb->GetUnitHealth(unit)/U->ud->health <= 0.66 ) ) // || G->Enemies.find(U->enemyID)->second.ud->kamikazeDist > fDis
	{
		float3 EPos = GetEnemyPosition(U->enemyID,U->E);
		CommandRun(unit,U,EPos);
		return;
	}
	else if( CommandCapture(unit,U,fDis) || CommandManeuver(unit,U,fDis) )
		return;
	else
	{
		float3 EPos = GetEnemyPosition(U->enemyID,U->E);
		Command c;
		if( U->ud->canAttack && (U->E->inLOS || U->E->inRadar) )
		{
			c.id = CMD_ATTACK;
			c.params.push_back(U->enemyID);
		}
		else if( U->ud->canAttack && (U->udr->IsBomber && U->E->posLocked) )
		{
			c.id = CMD_ATTACK;
			c.params.push_back(EPos.x);
			c.params.push_back(EPos.y);
			c.params.push_back(EPos.z);
		}
		else // cant see enemy or Mod Workaround: Combat Lords - cant be given attack orders
		{
			c.id = CMD_MOVE;
			c.params.push_back(EPos.x -100.0 +rand()%201 );
			c.params.push_back(EPos.y);
			c.params.push_back(EPos.z -100.0 +rand()%201 );
		}

		cb->GiveOrder(unit, &c);
		U->commandTimeOut=int(GetNextUpdate(fDis,U));
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
		{
			if( CanAttack(U, A, APos) != 0 && (U->Group==0 || U->Group->Enemies.find(attackerID) != U->Group->Enemies.end()) )
			{
				U->enemyID=attackerID;
				U->E = A;
				U->enemyEff = CanAttack(U, A, APos);
			}
		}
	}
	if( U->bInCombat )
	{
		if( U->ud->isCommander )
		{
			if( int(cb->GetCurrentUnitCommands(unitID)->size()) == 0 )
				UnitIdle(unitID,U);
			else if( cb->GetCurrentUnitCommands(unitID)->front().id != CMD_MOVE )
			{
				if( cb->GetUnitHealth(unitID)/U->ud->health <= 0.66 || 
					(cb->GetUnitHealth(unitID)/U->ud->health <= 0.9 && cb->GetCurrentUnitCommands(unitID)->front().id == CMD_CAPTURE) )
					UnitIdle(unitID,U);
			}
		}
		return;
	}
	if( U->BuildQ != 0 && U->BuildQ->RS != 0 )
	{
		U->BuildQ->tryCount = 4; // If the project is destroyed, give up on it
	}
	U->bInCombat=true;
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
	{
		UnitIdle(unitID,U);
	}
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
	Command c;
	c.id=CMD_DGUN;
	c.params.push_back(EPos.x);
	c.params.push_back(EPos.y);
	c.params.push_back(EPos.z);
	cb->GiveOrder(unitID, &c);
	U->commandTimeOut=cb->GetCurrentFrame()+5;
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

	Command c;
	c.id = CMD_CAPTURE;
	c.params.push_back(U->enemyID);
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

	Command c;
	c.id = CMD_LOAD_UNITS;
	c.params.push_back(U->enemyID);
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
		int iS=G->TM->GetSector(EPos);
		if( G->TM->SectorValid(iS) && G->TM->Sector[iS].AltSector != 0 )
		{
			Pos=G->TM->Sector[iS].AltSector->Pos;
			Pos.x+=128-rand()%256;
			Pos.z+=128-rand()%256;
			Command c;
			c.id = CMD_MOVE;
			c.params.push_back(Pos.x);
			c.params.push_back(cb->GetElevation(Pos.x,Pos.z));
			c.params.push_back(Pos.z);
			cb->GiveOrder(unitID, &c);
			U->commandTimeOut=int(GetNextUpdate(EDis,U));
			return true;
		}
	}
	if( EDis < 0.70*U->enemyEff->BestRange || EDis > U->enemyEff->BestRange )
	{
		float fDisAway=(0.87*U->enemyEff->BestRange-EDis);
		Pos.x+=(Pos.x-EPos.x)*(fDisAway/EDis);
		Pos.z+=(Pos.z-EPos.z)*(fDisAway/EDis);

		if( !G->TM->CanMoveToPos(U->mapBody,Pos) )
			return false;

		Command c;
		c.id = CMD_MOVE;
		c.params.push_back(Pos.x);
		c.params.push_back(cb->GetElevation(Pos.x,Pos.z));
		c.params.push_back(Pos.z);
		cb->GiveOrder(unitID, &c);
		U->commandTimeOut=int(GetNextUpdate(EDis,U));
		return true;
	}
	return false;
}

void cCombatManager::CommandRun(const int& unitID, UnitInfo *U, float3& EPos)
{
	float3 Pos=cb->GetUnitPos(unitID);
	Pos.x+=Pos.x-EPos.x;
	Pos.z+=Pos.z-EPos.z;
	G->CorrectPosition(&Pos);
	Command c;
	c.id = CMD_MOVE;
	c.params.push_back(Pos.x);
	c.params.push_back(cb->GetElevation(Pos.x,Pos.z));
	c.params.push_back(Pos.z);
	cb->GiveOrder(unitID, &c);
	U->commandTimeOut=cb->GetCurrentFrame()+210;
}

int cCombatManager::GetClosestEnemy(float3 Pos, UnitInfo* U)
{
	U->enemyID=-1;
	if( !G->UnitM->ActiveAttackOrders() ) // these two function need improvement, for now ill just use a short cut
	{
		return GetClosestThreat(Pos, U);
	}
	sWeaponEfficiency* weTemp;
	float fDis,fTemp;
	float3 fE;
	for( map<int,EnemyInfo>::iterator E=G->Enemies.begin(); E!=G->Enemies.end(); E++ )
	{
		fE=GetEnemyPosition(E->first,&E->second);
		if( (weTemp = CanAttack(U,&E->second,fE)) != 0 )
		{
			fTemp=Pos.distance(fE);
			if( U->enemyID == -1 || fTemp < fDis )
			{
				U->enemyID=E->first;
				U->E = &E->second;
				U->enemyEff = weTemp;
				fDis=fTemp;
			}
		}
	}
	if( U->enemyID != -1 && U->Group != 0 )
	{
		G->UnitM->GroupAddEnemy(U->enemyID,U->E,U->Group);
	}
	return U->enemyID;
}

int cCombatManager::GetClosestThreat(float3 Pos, UnitInfo* U)
{
	sWeaponEfficiency* weTemp;
	float fDis,fTemp;
	float3 fE;
	set<int> del;
	for( map<int,EnemyInfo*>::iterator E=G->EThreat.begin(); E!=G->EThreat.end(); E++ )
	{
		fE=GetEnemyPosition(E->first,E->second);
		if( E->second->baseThreatFrame > cb->GetCurrentFrame()+3600 ||
			(E->second->baseThreatFrame > cb->GetCurrentFrame()+1200 && G->UImmobile.find(E->second->baseThreatID) == G->UImmobile.end() ) ||
			(E->second->ud != 0 && G->UImmobile.find(E->second->baseThreatID) != G->UImmobile.end() && 1.3*E->second->ud->maxWeaponRange < fE.distance(cb->GetUnitPos(E->second->baseThreatID)) ) )
		{
			E->second->baseThreatID = -1;
			E->second->baseThreatFrame = -1;
			del.insert(E->first);
		}
		else if( (weTemp = CanAttack(U,E->second,fE)) != 0 )
		{
			fTemp=Pos.distance(fE);
			if( U->enemyID == -1 || fTemp < fDis )
			{
				U->enemyID=E->first;
				U->E = E->second;
				U->enemyEff = weTemp;
				fDis=fTemp;
			}
		}
	}
	while( int(del.size()) > 0 )
	{
		if( !G->UnitM->ActiveAttackOrders() )
		{
			EnemyInfo* E = G->EThreat.find(*del.begin())->second;
			while( int(E->AttackGroups.size()) > 0 )
				G->UnitM->GroupRemoveEnemy(*del.begin(),E,E->AttackGroups.begin()->second);
		}
		G->EThreat.erase(*del.begin());
		del.erase(*del.begin());
	}
	if( U->enemyID != -1 && U->Group != 0 )
	{
		G->UnitM->GroupAddEnemy(U->enemyID,U->E,U->Group);
	}
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
	if( !G->TM->CanMoveToPos(U->mapBody,EPos) )
	{
		return 0;
	}
	float fElevation=cb->GetElevation(EPos.x,EPos.z);
	if( EPos.y < 0.0 && U->udr->WeaponSeaEff.BestRange > 0 )
	{
		return &U->udr->WeaponSeaEff;
	}
	if( EPos.y-fElevation>50.0 && U->udr->WeaponAirEff.BestRange > 0 )
	{
		return &U->udr->WeaponAirEff;
	}
	if( EPos.y-fElevation<=50.0 && EPos.y >= -15.0 && U->udr->WeaponLandEff.BestRange > 0 )
	{
		return &U->udr->WeaponLandEff;
	}
	return 0;
}

bool cCombatManager::ValidateEnemy(const int& unitID, UnitInfo* U, bool IdleIfInvalid)
{
	if( U->enemyID == -1 || G->Enemies.find(U->enemyID) == G->Enemies.end() )
	{
		U->enemyID=-1;
		if( IdleIfInvalid )
			U->commandTimeOut=cb->GetCurrentFrame()+90;
		return false;
	}
	if( cb->GetUnitDef(U->enemyID) != 0 && cb->GetUnitAllyTeam(unitID) == cb->GetUnitAllyTeam(U->enemyID) ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		*l<<"\nWARNING: ValidateEnemy: ally has captured an enemy U->enemyID="<<U->enemyID;
		G->EnemyDestroyed(U->enemyID,-1);
		U->enemyID=-1;
		return false;
	}
	float3 EPos=cb->GetUnitPos(U->enemyID);
	if( U->Group == 0 )
	{
		U->E = &G->Enemies.find(U->enemyID)->second;
		U->enemyEff = CanAttack(U,U->E,EPos);
	}
	if( EPos.x != 0 || EPos.z != 0 || EPos.y != 0 ) // Position is valid
	{
		if( !U->E->inLOS && !U->E->inRadar && cb->GetUnitPos(unitID).distance2D(GetEnemyPosition(U->enemyID,U->E)) < 100.0f ) // the enemy id was reused, hopefully thats the only time this happens
		{
			*l<<"\nWARNING: ValidateEnemy: Enemy position is valid, but is not in LOS/Radar U->enemyID="<<U->enemyID;
			G->EnemyRemove(U->enemyID,U->E);
			U->enemyID=-1;
			return false;
		}
		return true;
	}
	if( U->E->inLOS || U->E->inRadar ) // ! Work Around   Spring Version: v0.72b1-0.75b2
	{
		*l<<"\nWARNING: ValidateEnemy: Enemy Position Invalid  ID="<<U->enemyID;
		if( U->E->inLOS )
			G->DebugEnemyEnterLOSError++;
		if( U->E->inRadar )
			G->DebugEnemyEnterRadarError++;
	}
	else if( cb->GetUnitPos(unitID).distance2D(U->E->position) > 300.0f )
	{
		return true;
	}
	G->EnemyRemove(U->enemyID,U->E);
	U->enemyID=-1;
	if( IdleIfInvalid )
		U->commandTimeOut=cb->GetCurrentFrame()+90;
	return false;
}
