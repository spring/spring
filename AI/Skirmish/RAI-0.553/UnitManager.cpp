#include "UnitManager.h"
//#include "LogFile.h"

sRAIGroup::sRAIGroup(int Index)
{
	index=Index;
}

sRAIGroup::~sRAIGroup()
{
	for( map<int,EnemyInfo*>::iterator i=Enemies.begin(); i!=Enemies.end(); i++ )
	{
		i->second->AttackGroups.erase(this);
	}
}

cUnitManager::cUnitManager(IAICallback* callback, cRAI* Global)
{
	cb=callback;
	G=Global;
	l=G->l;
	GroupSize=0;
	AttackOrders=false;
	SLSize=0;
}

void cUnitManager::UnitFinished(int unit,UnitInfo *U)
{
//	*l<<" (t="<<U->pBOL->task<<")";
	switch( U->pBOL->task )
	{
	case TASK_CONSTRUCT:
		G->Build->UBuilderFinished(unit,U);
		break;
	case TASK_ASSAULT:
		{
			UAssault.insert(cRAI::iupPair(unit,U));
			UpdateGroupSize();
			Assign(unit,U);
			if( ActiveAttackOrders() )
				SendAttackGroups();
		}
		break;
	case TASK_SCOUT:
		{
			UScout.insert(isPair(unit,sScoutUnitInfo()));
		}
		break;
	case TASK_SUICIDE:
		{
			USuicide.insert(cRAI::iupPair(unit,U));
		}
		break;
	case TASK_SUPPORT:
		{
			USupport.insert(unit);
		}
		break;
	case TASK_TRANSPORT:
		{
			UTrans.insert(itPair(unit,sTransportUnitInfo(U->ud)));
		}
		break;
	}
}

void cUnitManager::UnitDestroyed(int unit,UnitInfo *U)
{
//	*l<<" (t="<<U->pBOL->task<<")";
	switch( U->pBOL->task )
	{
	case TASK_CONSTRUCT:
		G->Build->UBuilderDestroyed(unit,U);
		break;
	case TASK_ASSAULT:
		{
			UAssault.erase(unit);
			UpdateGroupSize();

			U->Group->Units.erase(unit);
			if( int(U->Group->Units.size()) == 0 )
			{
				GroupSize--;
				sRAIGroup* RGroup=Group[U->Group->index];
				Group[U->Group->index]=Group[GroupSize];
				Group[U->Group->index]->index = U->Group->index;
				delete RGroup;
			}
		}
		break;
	case TASK_SCOUT:
		{
			UScout.erase(unit);
		}
		break;
	case TASK_SUICIDE:
		{
			USuicide.erase(unit);
		}
		break;
	case TASK_SUPPORT:
		{
			USupport.erase(unit);
		}
		break;
	case TASK_TRANSPORT:
		{
			UTrans.erase(unit);
		}
		break;
	}
}

void cUnitManager::UnitIdle(int unit,UnitInfo *U)
{
//	*l<<" (t="<<U->pBOL->task<<")";
	switch( U->pBOL->task )
	{
	case TASK_CONSTRUCT:
		G->Build->UBuilderIdle(unit,U);
		break;
	case TASK_ASSAULT:
		{
			if( int(U->Group->Enemies.size()) > 0 )
			{
				U->bInCombat = true;
				U->commandTimeOut = 0;
				return;
			}
			if( G->UDH->BLMobileRadar->UDefActive == 0 && G->Enemies.size() < 15 && int(G->Enemies.size()) <= int(UAssault.size())/2 )
			{
				int num=0;
				for( map<int,UnitInfo*>::iterator iU = U->Group->Units.begin(); iU != U->Group->Units.end(); iU++ )
				{
					if( cb->GetUnitPos(iU->first).distance2D(U->Group->ScoutPoint) < 350.0f )
						num++;
				}
				if( num >= 1+int(U->Group->Units.size())/2 )
				{
					U->Group->ScoutPoint = G->GetRandomPosition(U->mapBody);
					for( map<int,UnitInfo*>::iterator iU = U->Group->Units.begin(); iU != U->Group->Units.end(); iU++ )
						iU->second->commandTimeOut = 0;
				}

				Command c;
				if( cb->GetUnitPos(unit).distance2D(U->Group->ScoutPoint) < 350.0f )
				{
					c.id = CMD_WAIT;
					U->commandTimeOut = cb->GetCurrentFrame() + 300;
					//c.timeOut = cb->GetCurrentFrame() + 300;

				}
				else
				{
					c.id = CMD_MOVE;
					c.params.push_back(U->Group->ScoutPoint.x);
					c.params.push_back(U->Group->ScoutPoint.y);
					c.params.push_back(U->Group->ScoutPoint.z);
				}
				cb->GiveOrder(unit, &c);
			}
			else
			{
				Command c;
				if( cb->GetUnitPos(unit).distance2D(U->Group->RallyPoint) < 400 )
				{
					c.id = CMD_WAIT;
					U->commandTimeOut = cb->GetCurrentFrame() + 900;
					//c.timeOut = cb->GetCurrentFrame() + 300;

				}
				else
				{
					c.id = CMD_MOVE;
					c.params.push_back(U->Group->RallyPoint.x);
					c.params.push_back(U->Group->RallyPoint.y);
					c.params.push_back(U->Group->RallyPoint.z);
				}
				cb->GiveOrder(unit, &c);
			}
		}
		break;
	case TASK_SCOUT:
		{
			sScoutUnitInfo *S = &UScout.find(unit)->second;
			if( S->ScoutLocAssigned )
			{
				float3 Pos=cb->GetUnitPos(unit);
				const UnitDef* ud = cb->GetUnitDef(unit);

				if( Pos.x - S->SL->location.x > 100 || Pos.x - S->SL->location.x < -100 || Pos.z - S->SL->location.z > 100 || Pos.z - S->SL->location.z < -100 )
				{
					Command c;
					c.id = CMD_MOVE;
					c.params.push_back(S->SL->location.x);
					c.params.push_back(S->SL->location.y);
					c.params.push_back(S->SL->location.z);
					cb->GiveOrder(unit, &c);
				}
				return;
			}
			if( SLSize>0 )
			{
				S->SL= SL[0];
				SL[0]=SL[--SLSize];
				UnitIdle(unit,U);
				return;
			}
			Command c;
			c.id = CMD_MOVE;
			float3 movePos=G->GetRandomPosition(U->mapBody);
			c.params.push_back(movePos.x);
			c.params.push_back(movePos.y);
			c.params.push_back(movePos.z);
			cb->GiveOrder(unit, &c);
		}
		break;
	case TASK_SUICIDE:
		{
			Command c;
			if( int(G->Enemies.size()) > 0 && G->CombatM->GetClosestEnemy(cb->GetUnitPos(unit), U) >= 0 )
			{
				int iRan=rand()%int(G->Enemies.size());
				map<int,EnemyInfo>::iterator iE=G->Enemies.begin();
				for( int i=0; i<iRan; i++,iE++ ) {}

				U->bInCombat=true;
				U->enemyID=iE->first;
				U->E = &iE->second;
				U->enemyEff = G->CombatM->CanAttack(U,U->E,G->CombatM->GetEnemyPosition(U->enemyID,U->E));

				c.id = CMD_ATTACK;
				c.params.push_back(iE->first);
				cb->GiveOrder(unit,&c);
				return;
			}

			c.id = CMD_MOVE;
			float3 movePos=G->GetRandomPosition(U->mapBody);
			c.params.push_back(movePos.x);
			c.params.push_back(movePos.y);
			c.params.push_back(movePos.z);
			cb->GiveOrder(unit, &c);
		}
		break;
	case TASK_SUPPORT:
		{
			Command c;
			c.id = CMD_WAIT;
			cb->GiveOrder(unit, &c);
		}
		break;
	case TASK_TRANSPORT:
		{
			sTransportUnitInfo *T = &UTrans.find(unit)->second;
			if( T->AssistID == -1 )
			{
				Command c;
				c.id = CMD_WAIT;
				cb->GiveOrder(unit, &c);

				U->commandTimeOut=cb->GetCurrentFrame()+450;
			}
			else
			{

			}
		}
		break;
	}
}

void cUnitManager::EnemyEnterLOS(int enemy,EnemyInfo *E)
{
	EnemyEnterRadar(enemy,E);
}

void cUnitManager::EnemyEnterRadar(int enemy,EnemyInfo *E)
{
	if( ActiveAttackOrders() )
		SendAttackGroups();
}

bool cUnitManager::UnitMoveFailed(int unit, UnitInfo *U)
{
	if( int(UTrans.size()) == 0 )
		return false;
	for( map<int,sTransportUnitInfo>::iterator iT=UTrans.begin(); iT!=UTrans.end(); iT++ )
	{
		if( iT->second.AssistID == -1 && iT->second.ud->transportMass >= U->ud->mass )
		{
			iT->second.AssistID=unit;
			//return true;
		}
	}
	return false;
}

void cUnitManager::UpdateGroupSize()
{
	MaxGroupMSize=5+int(UAssault.size())/4;
}

bool cUnitManager::ActiveAttackOrders()
{
	if( int(G->Enemies.size()) == 0 )
	{
//		if( AttackOrders ) cb->SendTextMsg("Defending",5);
		AttackOrders=false;
	}
	else if( int(UAssault.size()) >= 66 || (int(UAssault.size()) >= 6 && int(UAssault.size()) > 0.8*G->Enemies.size()) || G->UDH->BLBuilder->UDefActive == 0 )
	{
//		if( !AttackOrders ) cb->SendTextMsg("Attacking",5);
		AttackOrders=true;
	}
	else if( int(UAssault.size()) <= 4 || (int(UAssault.size()) <= 44 && int(UAssault.size()) < 0.533*G->Enemies.size()) )
	{
		if( AttackOrders )
		{
//			cb->SendTextMsg("Defending",5);
			AttackOrders=false;
			for( int i=0; i<GroupSize; i++ ) // TEMP
			{
				set<int> del;
				for( map<int,EnemyInfo*>::iterator iE = Group[i]->Enemies.begin(); iE != Group[i]->Enemies.end(); iE++ )
				{
					if( iE->second->baseThreatID == -1 )
					{
						del.insert(iE->first);
					}
				}
				while( int(del.size()) > 0 )
				{
					GroupRemoveEnemy(*del.begin(),Group[i]->Enemies.find(*del.begin())->second,Group[i]);
					del.erase(*del.begin());
				}
			}
		}
	}

	return AttackOrders;
}

void cUnitManager::GroupAddUnit(int unit, UnitInfo* U, sRAIGroup* Gr)
{
	Gr->Units.insert(cRAI::iupPair(unit,U));
	U->Group = Gr;
	if( int(Gr->Enemies.size()) > 0 )
		U->bInCombat = true;
}

void cUnitManager::GroupAddEnemy(int enemy, EnemyInfo *E, sRAIGroup* Gr)
{
	if( !G->ValidateUnitList(&Gr->Units) )
		return;

	Gr->Enemies.insert(cRAI::iepPair(enemy,E));
	E->AttackGroups.insert(ggPair(Gr,Gr));
	if( Gr->Enemies.size() == 1 )
	{
		for( map<int,UnitInfo*>::iterator iU = Gr->Units.begin(); iU != Gr->Units.end(); iU++ )
		{
			iU->second->bInCombat=true;
			if( !G->IsHumanControled(iU->first,iU->second) )
			{
				Command c;
				c.id=CMD_STOP;
				cb->GiveOrder(iU->first, &c);
			}
		}
	}
}

void cUnitManager::GroupRemoveEnemy(int enemy, EnemyInfo *E, sRAIGroup* Gr)
{
	if( !G->ValidateUnitList(&Gr->Units) )
		return;

	Gr->Enemies.erase(enemy);
	E->AttackGroups.erase(Gr);
	for( map<int,UnitInfo*>::iterator iU = Gr->Units.begin(); iU != Gr->Units.end(); iU++ )
	{
		if( iU->second->enemyID == enemy )
		{
			iU->second->enemyID = -1;
			if( !G->IsHumanControled(iU->first,iU->second) )
			{
				iU->second->commandTimeOut=1;
			}
		}
	}
	if( int(Gr->Enemies.size()) == 0 && int(G->EThreat.size()) == 0 && !ActiveAttackOrders() )
	{
		GroupResetRallyPoint(Gr);
		Gr->ScoutPoint = G->GetRandomPosition(Gr->Units.begin()->second->mapBody);
	}
}

void cUnitManager::GroupResetRallyPoint(sRAIGroup* Gr)
{
	float3 GPos = cb->GetUnitPos(Gr->Units.begin()->first);
	UnitInfo* GU = Gr->Units.begin()->second;
	int iBest=-1;
	UnitInfo* uBest;

	G->ValidateUnitList(&G->UImmobile);
	for(map<int,UnitInfo*>::iterator iU=G->UImmobile.begin(); iU!=G->UImmobile.end(); iU++)
	{
		if( G->TM->CanMoveToPos(GU->mapBody,cb->GetUnitPos(iU->first)) && GPos.distance2D(cb->GetUnitPos(iU->first)) < GPos.distance2D(cb->GetUnitPos(iBest)) )
		{
			iBest = iU->first;
			uBest = iU->second;
		}
	}

	if( iBest != -1 )
		Gr->RallyPoint = cb->GetUnitPos(iBest);
	else
		Gr->RallyPoint = GPos;

//	G->DebugDrawShape(Gr->RallyPoint,100.0f,50,0,50,900);

	if( Gr->RallyPoint.x < 8*cb->GetMapWidth() - Gr->RallyPoint.x )
		Gr->RallyPoint.x -= 300;
	else
		Gr->RallyPoint.x += 300;
	if( Gr->RallyPoint.z < 8*cb->GetMapHeight() - Gr->RallyPoint.z )
		Gr->RallyPoint.z -= 300;
	else
		Gr->RallyPoint.z += 300;

	Gr->RallyPoint.x += -250.0f + rand()%501;
	Gr->RallyPoint.z += -250.0f + rand()%501;
	G->CorrectPosition(&Gr->RallyPoint);

//	G->DebugDrawShape(Gr->RallyPoint,100.0f,25,0,50,900);

	float3 pos = cb->ClosestBuildSite(GU->ud,Gr->RallyPoint,800,15);
	if( pos.x <= 0 && pos.y <= 0 && pos.z <= 0 )
		pos = cb->ClosestBuildSite(GU->ud,Gr->RallyPoint,800,3);
	if( pos.x <= 0 && pos.y <= 0 && pos.z <= 0 )
		Gr->RallyPoint = GPos;
	else
		Gr->RallyPoint = pos;

//	G->DebugDrawShape(Gr->RallyPoint,100.0f,5,0,50,900);
}

void cUnitManager::Assign(int unit,UnitInfo *U)
{
	set<int> Grs;
	for( int i=0; i<GroupSize; i++ )
	{
		if( U->mapBody == Group[i]->Units.begin()->second->mapBody &&
			U->ud->canLoopbackAttack == Group[i]->Units.begin()->second->udr->ud->canLoopbackAttack &&
			int(Group[i]->Units.size()) < MaxGroupMSize )
		{
			Grs.insert(i);
			for(map<int,UnitInfo*>::iterator GM=Group[i]->Units.begin(); GM!=Group[i]->Units.end(); GM++)
			{
				sRAIUnitDef* udr = GM->second->udr;
				if( U->ud->speed > 1.5*udr->ud->speed || 1.5*U->ud->speed < udr->ud->speed )
				{
					Grs.erase(i);
					break;
				}
			}
		}
	}

	if( GroupSize == 25 && int(Grs.size()) == 0 )
	{
		*l<<"\nWARNING: Maximum number of groups reached";
		Grs.insert(24);
	}

	if( int(Grs.size()) > 0 )
	{
		GroupAddUnit(unit,U,Group[*Grs.begin()]);
	}
	else
	{
		Group[GroupSize] = new sRAIGroup(GroupSize);
		sRAIGroup* Gr = Group[GroupSize];
		GroupSize++;

		GroupAddUnit(unit,U,Gr);
		Gr->ScoutPoint = G->GetRandomPosition(U->mapBody);
		GroupResetRallyPoint(Gr);
	}
}

void cUnitManager::SendAttackGroups()
{
	for( int i=0; i<GroupSize; i++ )
	{
		if( int(Group[i]->Enemies.size()) == 0 && (int(Group[i]->Units.size()) >= 4 || G->UDH->BLBuilder->UDefActive == 0) )
		{
			int enemyID = G->CombatM->GetClosestEnemy(cb->GetUnitPos(Group[i]->Units.begin()->first),Group[i]->Units.begin()->second);
//			EnemyInfo* E = Group[i]->Units.begin()->second->E;
		}
	}
}
