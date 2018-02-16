#include "UnitManager.h"
//#include "LogFile.h"

sRAIGroup::sRAIGroup(int Index)
{
	index = Index;
	M = 0;
	C = 0;
}

sRAIGroup::~sRAIGroup()
{
	for( map<int,EnemyInfo*>::iterator i=Enemies.begin(); i!=Enemies.end(); ++i )
	{
		i->second->attackGroups.erase(this);
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
	MaxGroupMSize=0;
	memset(SL, 0, SCOUT_POSITON_LIST_SIZE * sizeof(SL[0]));
	memset(Group, 0, RAI_GROUP_SIZE * sizeof(Group[0]));
}

void cUnitManager::UnitFinished(int unit,UnitInfo *U)
{
//	*l<<" (t="<<U->udrBL->task<<")";
	switch( U->udrBL->task )
	{
	case TASK_CONSTRUCT:
		G->B->UBuilderFinished(unit,U);
		break;
	case TASK_ASSAULT:
		{
			UAssault.insert(cRAI::iupPair(unit,U));
			UpdateGroupSize();
			Assign(unit,U);
			if( ActiveAttackOrders() )
				SendattackGroups();
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
//	*l<<" (t="<<U->udrBL->task<<")";
	switch( U->udrBL->task )
	{
	case TASK_CONSTRUCT:
		G->B->UBuilderDestroyed(unit,U);
		break;
	case TASK_ASSAULT:
		{
			UAssault.erase(unit);
			GroupRemoveUnit(unit,U);
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
//	*l<<" (t="<<U->udrBL->task<<")";
	switch( U->udrBL->task )
	{
	case TASK_CONSTRUCT:
		G->B->UBuilderIdle(unit,U);
		break;
	case TASK_ASSAULT:
		{
			if( int(U->group->Enemies.size()) > 0 )
			{
				U->inCombat = true;
				G->UpdateEventAdd(1,cb->GetCurrentFrame()+90,unit,U);
			}
			if( G->Enemies.empty() && (UAssault.size() > 50 || (G->UDH->BLMobileRadar->UDefActiveTemp == 0)) )
			{
				int num=0;
				for( map<int,UnitInfo*>::iterator iU = U->group->Units.begin(); iU != U->group->Units.end(); ++iU )
				{
					if( cb->GetUnitPos(iU->first).distance2D(U->group->M->ScoutPoint) < 350.0f )
						num++;
				}
				if( num >= 1+int(U->group->Units.size())/2 )
				{
					U->group->M->ScoutPoint = G->GetRandomPosition(U->area);
					for( map<int,UnitInfo*>::iterator iU = U->group->Units.begin(); iU != U->group->Units.end(); ++iU )
						G->UpdateEventAdd(1,0,iU->first,iU->second);
				}

				Command c;
				if( cb->GetUnitPos(unit).distance2D(U->group->M->ScoutPoint) < 400.0f )
				{
					c.id = CMD_WAIT;
					c.timeOut = cb->GetCurrentFrame()+900;
//					G->UpdateEventAdd(1,cb->GetCurrentFrame()+300,unit,U);
				}
				else
				{
					c.id = CMD_MOVE;
					c.params.push_back(U->group->M->ScoutPoint.x);
					c.params.push_back(U->group->M->ScoutPoint.y);
					c.params.push_back(U->group->M->ScoutPoint.z);
				}
				cb->GiveOrder(unit, &c);
			}
			else
			{
				Command c;
				if( cb->GetUnitPos(unit).distance2D(U->group->M->RallyPoint) < 400.0f )
				{
					c.id = CMD_WAIT;
					c.timeOut = cb->GetCurrentFrame()+900;
//					G->UpdateEventAdd(1,cb->GetCurrentFrame()+900,unit,U);
				}
				else
				{
					c.id = CMD_MOVE;
					c.params.push_back(U->group->M->RallyPoint.x);
					c.params.push_back(U->group->M->RallyPoint.y);
					c.params.push_back(U->group->M->RallyPoint.z);
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
				//const UnitDef* ud = cb->GetUnitDef(unit);

				if( Pos.x - S->SL->position.x > 100 || Pos.x - S->SL->position.x < -100 || Pos.z - S->SL->position.z > 100 || Pos.z - S->SL->position.z < -100 )
				{
					Command c;
					c.id = CMD_MOVE;
					c.params.push_back(S->SL->position.x);
					c.params.push_back(S->SL->position.y);
					c.params.push_back(S->SL->position.z);
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
			float3 movePos=G->GetRandomPosition(U->area);
			c.params.push_back(movePos.x);
			c.params.push_back(movePos.y);
			c.params.push_back(movePos.z);
			cb->GiveOrder(unit, &c);
		}
		break;
	case TASK_SUICIDE:
		{
			Command c;
			if( int(G->Enemies.size()) > 0 && G->CM->GetClosestEnemy(cb->GetUnitPos(unit), U) >= 0 )
			{
				set<int> Targets;
				for( map<int,EnemyInfo>::iterator iE=G->Enemies.begin(); iE!=G->Enemies.end(); ++iE )
					if( G->TM->CanMoveToPos(U->area,G->CM->GetEnemyPosition(iE->first,&iE->second)) )
						Targets.insert(iE->first);

				int iT=rand()%int(Targets.size());
				set<int>::iterator enemyID = Targets.begin();
				for( int i=0; i<iT; i++,enemyID++ ) {}

				U->inCombat=true;
				U->enemyID = *enemyID;
				U->E = &G->Enemies.find(U->enemyID)->second;
				U->enemyEff = G->CM->CanAttack(U,U->E,G->CM->GetEnemyPosition(U->enemyID,U->E));

				c.id = CMD_ATTACK;
				c.params.push_back(U->enemyID);
				cb->GiveOrder(unit,&c);
				return;
			}

			c.id = CMD_MOVE;
			float3 movePos=G->GetRandomPosition(U->area);
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

				G->UpdateEventAdd(1,cb->GetCurrentFrame()+450,unit,U);
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
		SendattackGroups();
}

bool cUnitManager::UnitMoveFailed(int unit, UnitInfo *U)
{
	if( int(UTrans.size()) == 0 )
		return false;
	for( map<int,sTransportUnitInfo>::iterator iT=UTrans.begin(); iT!=UTrans.end(); ++iT )
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
	else if( int(UAssault.size()) >= 60 || (int(UAssault.size()) >= 6 && int(UAssault.size()) > 0.8*G->Enemies.size()) || G->UDH->BLBuilder->UDefActive == 0 )
	{
//		if( !AttackOrders ) cb->SendTextMsg("Attacking",5);
		AttackOrders=true;
	}
	else if( int(UAssault.size()) <= 4 || (int(UAssault.size()) <= 40 && int(UAssault.size()) < 0.533*G->Enemies.size()) )
	{
		if( AttackOrders )
		{
//			cb->SendTextMsg("Defending",5);
			AttackOrders=false;
			for( int i=0; i<GroupSize; i++ ) // TEMP
			{
				set<int> deletion;
				for( map<int,EnemyInfo*>::iterator iE = Group[i]->Enemies.begin(); iE != Group[i]->Enemies.end(); ++iE )
					if( iE->second->baseThreatID == -1 )
						deletion.insert(iE->first);
				while( int(deletion.size()) > 0 )
				{
					GroupRemoveEnemy(*deletion.begin(),Group[i]->Enemies.find(*deletion.begin())->second,Group[i]);
					deletion.erase(*deletion.begin());
				}
			}
		}
	}

	return AttackOrders;
}

void cUnitManager::GroupAddUnit(int unit, UnitInfo* U, sRAIGroup* group)
{
	group->Units.insert(cRAI::iupPair(unit,U));
	U->group = group;
	if( int(group->Enemies.size()) > 0 )
		U->inCombat = true;

	if( U->udrBL->task == TASK_ASSAULT )
	{
		if( group->M == 0 )
			group->M = new sRAIGroupMilitary;
		group->M->count++;
	}
	else if( U->udrBL->task == TASK_CONSTRUCT )
	{
		if( group->C == 0 )
			group->C = new sRAIGroupConstruct;
		group->C->count++;
	}
}

void cUnitManager::GroupRemoveUnit(int unit, UnitInfo* U)
{
	UpdateGroupSize();
	U->group->Units.erase(unit);

	if( U->udrBL->task == TASK_ASSAULT )
	{
		U->group->M->count--;
		if( U->group->M->count == 0 )
		{
			delete U->group->M;
			U->group->M = 0;
		}
	}
	else if( U->udrBL->task == TASK_CONSTRUCT )
	{
		U->group->C->count--;
		if( U->group->C->count == 0 )
		{
			delete U->group->C;
			U->group->C = 0;
		}
	}

	if( int(U->group->Units.size()) == 0 )
	{
		GroupSize--;
		sRAIGroup* RGroup=Group[U->group->index];
		Group[U->group->index]=Group[GroupSize];
		Group[U->group->index]->index = U->group->index;
		delete RGroup;
	}
}

void cUnitManager::GroupAddEnemy(int enemy, EnemyInfo *E, sRAIGroup* group)
{
	if( !G->ValidateUnitList(&group->Units) )
		return;

	group->Enemies.insert(cRAI::iepPair(enemy,E));
	E->attackGroups.insert(group);
	if( group->Enemies.size() == 1 )
	{
		for( map<int,UnitInfo*>::iterator iU = group->Units.begin(); iU != group->Units.end(); ++iU )
		{
			iU->second->inCombat=true;
			if( !G->IsHumanControled(iU->first,iU->second) )
				G->UpdateEventAdd(1,-1,iU->first,iU->second);
		}
	}
}

void cUnitManager::GroupRemoveEnemy(int enemy, EnemyInfo *E, sRAIGroup* group)
{
	if( !G->ValidateUnitList(&group->Units) )
		return;

	group->Enemies.erase(enemy);
	E->attackGroups.erase(group);
	for( map<int,UnitInfo*>::iterator iU = group->Units.begin(); iU != group->Units.end(); ++iU )
	{
		if( iU->second->enemyID == enemy )
		{
			iU->second->enemyID = -1;
			if( !G->IsHumanControled(iU->first,iU->second) )
				G->UpdateEventAdd(1,0,iU->first,iU->second);
		}
	}
	if( int(group->Enemies.size()) == 0 && int(G->EThreat.size()) == 0 && !ActiveAttackOrders() )
	{
		GroupResetRallyPoint(group);
		group->M->ScoutPoint = G->GetRandomPosition(group->Units.begin()->second->area);
	}
}

void cUnitManager::GroupResetRallyPoint(sRAIGroup* group)
{
	float3 GPos = cb->GetUnitPos(group->Units.begin()->first);
	UnitInfo* GU = group->Units.begin()->second;
	int iBest=-1;
//	UnitInfo* uBest;

	G->ValidateUnitList(&G->UImmobile);
	for(map<int,UnitInfo*>::iterator iU=G->UImmobile.begin(); iU!=G->UImmobile.end(); ++iU)
	{
		if( G->TM->CanMoveToPos(GU->area,cb->GetUnitPos(iU->first)) && GPos.distance2D(cb->GetUnitPos(iU->first)) < GPos.distance2D(cb->GetUnitPos(iBest)) )
		{
			iBest = iU->first;
//			uBest = iU->second;
		}
	}

	if( iBest != -1 )
		group->M->RallyPoint = cb->GetUnitPos(iBest);
	else
		group->M->RallyPoint = GPos;
//	G->DebugDrawShape(group->M->RallyPoint,100.0f,50,0,50,900);

	if( group->M->RallyPoint.x < 8*cb->GetMapWidth() - group->M->RallyPoint.x )
		group->M->RallyPoint.x -= 300;
	else
		group->M->RallyPoint.x += 300;
	if( group->M->RallyPoint.z < 8*cb->GetMapHeight() - group->M->RallyPoint.z )
		group->M->RallyPoint.z -= 300;
	else
		group->M->RallyPoint.z += 300;

	group->M->RallyPoint.x += -250.0f + rand()%501;
	group->M->RallyPoint.z += -250.0f + rand()%501;
	G->CorrectPosition(group->M->RallyPoint);
//	G->DebugDrawShape(group->M->RallyPoint,100.0f,25,0,50,900);

	float3 pos = cb->ClosestBuildSite(GU->ud,group->M->RallyPoint,800,15);
	if( pos.x <= 0 && pos.y <= 0 && pos.z <= 0 )
		pos = cb->ClosestBuildSite(GU->ud,group->M->RallyPoint,800,3);
	if( pos.x <= 0 && pos.y <= 0 && pos.z <= 0 )
		group->M->RallyPoint = GPos;
	else
		group->M->RallyPoint = pos;
	G->CorrectPosition(group->M->RallyPoint);

//	G->DebugDrawShape(group->M->RallyPoint,100.0f,5,0,50,900);
}

void cUnitManager::Assign(int unit,UnitInfo *U)
{
	set<int> Grs;
	for( int i=0; i<GroupSize; i++ )
	{
		if( U->area == Group[i]->Units.begin()->second->area &&
			U->ud->canLoopbackAttack == Group[i]->Units.begin()->second->udr->ud->canLoopbackAttack &&
			int(Group[i]->Units.size()) < MaxGroupMSize )
		{
			Grs.insert(i);
			for(map<int,UnitInfo*>::iterator GM=Group[i]->Units.begin(); GM!=Group[i]->Units.end(); ++GM)
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
		sRAIGroup* group = Group[GroupSize];
		GroupSize++;

		GroupAddUnit(unit,U,group);
		group->M->ScoutPoint = G->GetRandomPosition(U->area);
		GroupResetRallyPoint(group);
	}
}

void cUnitManager::SendattackGroups()
{
	for( int i=0; i<GroupSize; i++ ) {
		if( (Group[i]->Enemies.size() == (size_t)0) &&
		    ( (Group[i]->Units.size() >= (size_t)4) ||
		      (G->UDH->BLBuilder->UDefActive == 0) ) ) {
//			int enemyID = G->CM->GetClosestEnemy(cb->GetUnitPos(Group[i]->Units.begin()->first), Group[i]->Units.begin()->second);
			// TODO: FIXME: implement me
		}
	}
}
