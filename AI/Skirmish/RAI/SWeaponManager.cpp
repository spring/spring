#include "SWeaponManager.h"
#include "LegacyCpp/WeaponDef.h"

cSWeaponManager::cSWeaponManager(IAICallback* callback, cRAI* global)
{
	G=global;
	l=global->l;
	cb=callback;
}

cSWeaponManager::~cSWeaponManager()
{

}

void cSWeaponManager::UnitFinished(int unit, sRAIUnitDef* udr)
{
	if( udr->SWeapon == 0 )
		return;

	mWeapon.insert(irPair(unit,udr));
}

void cSWeaponManager::UnitDestroyed(int unit)
{
	if( mWeapon.find(unit) != mWeapon.end() )
		mWeapon.erase(unit);
}

void cSWeaponManager::UnitIdle(int unit, sRAIUnitDef* udr)
{
	int stockPile;
	int stockQued;
	cb->GetProperty(unit,AIVAL_STOCKPILED,&stockPile);
	cb->GetProperty(unit,AIVAL_STOCKPILE_QUED,&stockQued);
	if( stockPile > 0 && udr->SWeapon->manualfire && G->Enemies.size() >= (G->UMobile.size()/25)+1 )
	{
		set<int> Targets;
		set<int> ImmobileTargets;
		for( map<int,EnemyInfo>::iterator iE=G->Enemies.begin(); iE!=G->Enemies.end(); ++iE )
		{
			if( cb->GetUnitPos(unit).distance(G->CM->GetEnemyPosition(iE->first,&iE->second)) <= udr->SWeapon->range )
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
					++iE;
			}
			else
			{
				iE = Targets.begin();
				for( int i=0; i < rand()%int(Targets.size()); i++ )
					++iE;
			}

			EnemyInfo *E = &G->Enemies.find(*iE)->second;
			Command c(CMD_ATTACK);
			if( E->inLOS || E->inRadar )
				c.PushParam(*iE);
			else
			{
				c.PushPos(G->CM->GetEnemyPosition(*iE,E));
			}
			cb->GiveOrder(unit,&c);
			return;
		}
	}
	else if( stockPile+stockQued < 5 || (!udr->SWeapon->manualfire && stockPile+stockQued < 10) )
	{
		if( ((G->UDH->BLMetal->UDefActive == 0 && G->UDH->BLMetalL->UDefActive == 0)
				|| 0.66*cb->GetMetalIncome() > udr->SWeapon->metalcost/udr->SWeapon->reload) &&
			((G->UDH->BLEnergy->UDefActive == 0 && G->UDH->BLEnergyL->UDefActive == 0)
				|| 0.66*cb->GetEnergyIncome() > udr->SWeapon->energycost/udr->SWeapon->reload) )
		{
			Command c(CMD_STOCKPILE);
			cb->GiveOrder(unit, &c);
			return;
		}
	}

	UnitInfo *UI = &G->Units.find(unit)->second;
	if( UI->udrBL->task <= 1 )
	{
		Command c(CMD_WAIT);
		cb->GiveOrder(unit, &c);
	}
}

void cSWeaponManager::Update()
{
	for( map<int,sRAIUnitDef*>::iterator iU = mWeapon.begin(); iU!=mWeapon.end(); ++iU )
		if( !G->IsHumanControled(iU->first,&G->Units.find(iU->first)->second) )
			UnitIdle(iU->first,iU->second);
}
