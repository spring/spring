#include "SWeaponManager.h"
#include "Sim/Weapons/WeaponDefHandler.h"

cSWeaponManager::cSWeaponManager(IAICallback* callback, cRAI* global)
{
	G=global;
	l=global->l;
	cb=callback;
}

cSWeaponManager::~cSWeaponManager()
{

}

void cSWeaponManager::UnitFinished(int unit, sRAIUnitDef* udRAI)
{
	if( udRAI->SWeapon == 0 )
		return;

	typedef pair<int,sSWeaponUnitInfo> isPair;
	mWeapon.insert(isPair(unit,sSWeaponUnitInfo(udRAI)));
}

void cSWeaponManager::UnitDestroyed(int unit)
{
	if( mWeapon.find(unit) != mWeapon.end() )
		mWeapon.erase(unit);
}

void cSWeaponManager::UnitIdle(int unit, sSWeaponUnitInfo* U)
{
	if( U->StockPile > 0 && U->udr->SWeapon->manualfire && int(G->Enemies.size()) >= 1+(G->UMobile.size()/20) )
	{
		set<int> Targets;
		set<int> ImmobileTargets;
		for( map<int,EnemyInfo>::iterator iE=G->Enemies.begin(); iE!=G->Enemies.end(); iE++ )
		{
			if( cb->GetUnitPos(unit).distance(G->CombatM->GetEnemyPosition(iE->first,&iE->second)) <= U->udr->SWeapon->range )
			{
				if( iE->second.ud != 0 && iE->second.ud->speed == 0 )
					ImmobileTargets.insert(iE->first);
				else if( iE->second.inLOS || iE->second.inRadar )
					Targets.insert(iE->first);
			}
		}

		if( int(ImmobileTargets.size()) > 0 || int(Targets.size()) > 0 )
		{
			set<int>::iterator iE;
			if( int(ImmobileTargets.size()) > 0 )
			{
				iE = ImmobileTargets.begin();
				for( int i=0; i < rand()%int(ImmobileTargets.size()); i++ )
					iE++;
			}
			else
			{
				iE = Targets.begin();
				for( int i=0; i < rand()%int(Targets.size()); i++ )
					iE++;
			}

			EnemyInfo *E = &G->Enemies.find(*iE)->second;
			U->StockPile--;
			Command c;
			c.id = CMD_ATTACK;
			if( E->inLOS || E->inRadar )
				c.params.push_back(*iE);
			else
			{
				float3 Pos = G->CombatM->GetEnemyPosition(*iE,E);
				c.params.push_back(Pos.x);
				c.params.push_back(Pos.y);
				c.params.push_back(Pos.z);
			}
			cb->GiveOrder(unit,&c);
			return;
		}
	}
	else if( U->StockPile < 5 || !U->udr->SWeapon->manualfire )
	{
		if( ((G->UDH->BLMetal->UDefActive == 0 && G->UDH->BLMetalL->UDefActive == 0) || 0.66*cb->GetMetalIncome() > U->udr->SWeapon->metalcost/U->udr->SWeapon->reload) &&
			((G->UDH->BLEnergy->UDefActive == 0 && G->UDH->BLEnergyL->UDefActive == 0) || 0.66*cb->GetEnergyIncome() > U->udr->SWeapon->energycost/U->udr->SWeapon->reload) )
		{
			U->StockPile++;
			Command c;
			c.id = CMD_STOCKPILE;
			cb->GiveOrder(unit, &c);
			return;
		}
	}

	UnitInfo *UI = &G->Units.find(unit)->second;
	if( UI->pBOL->task <= 1 )
	{
		Command c;
		c.id = CMD_WAIT;
		cb->GiveOrder(unit, &c);
		UI->commandTimeOut=cb->GetCurrentFrame()+300;
	}
}

void cSWeaponManager::Update()
{
	for( map<int,sSWeaponUnitInfo>::iterator iU = mWeapon.begin(); iU!=mWeapon.end(); iU++ )
		if( !G->IsHumanControled(iU->first,&G->Units.find(iU->first)->second) )
			UnitIdle(iU->first,&iU->second);
}
