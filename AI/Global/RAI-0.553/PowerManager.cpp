#include "PowerManager.h"
#include "Sim/Weapons/WeaponDefHandler.h"
//#include "Sim/Units/UnitDef.h"
//#include "LogFile.h"

cPowerManager::cPowerManager(IAICallback* callback, cRAI* global)
{
	G=global;
	l=G->l;
	cb=callback;

	NeededCloakPower=0;
	NeededOnOffPower=0;
	NeededWeaponPower=0;
	MetalProductionEtoM=0;
	MetalProduction=0;
}

cPowerManager::~cPowerManager()
{
	*l<<"\n cPowerManager Debug:";
	*l<<"\n  Needed Cloak Power     = "<<NeededCloakPower;
	*l<<"\n  Needed On/Off Power    = "<<NeededOnOffPower;
	*l<<"\n  Needed Weapon Power    = "<<NeededWeaponPower;
	*l<<"\n  Extra Metal EtoM       = "<<MetalProductionEtoM;
	*l<<"\n  Extra Metal Production = "<<MetalProduction;
//	if( NeededCloakPower != 0.0 || NeededOnOffPower != 0.0 || NeededWeaponPower != 0.0 || MetalProductionEtoM != 0.0 || MetalProduction != 0.0 )
//		*l<<"\n  ERROR!";
}

void cPowerManager::UnitFinished(int unit, UnitInfo *U)
{
	NeededWeaponPower+=U->udr->WeaponCostMax;

	if( U->ud->canCloak )
	{
		mCloakUnits.insert(cRAI::iupPair(unit,U));
		U->CloakUI = new sPowerUnitInfo(cb->IsUnitCloaked(unit));
		if( !U->CloakUI->active )
			NeededCloakPower+=int(U->udr->CloakCostMax);
	}
	if( U->ud->onoffable )
	{
		map<int,UnitInfo*> *pmPI = 0;
		if( U->udr->EnergyDifference < 0 && U->udr->MetalDifference > 0 && G->UDH->EnergyToMetalRatio*U->udr->MetalDifference < -U->udr->EnergyDifference )
		{
			pmPI = &mEtoMConverter;
			if( !cb->IsUnitActivated(unit) )
			{
				MetalProductionEtoM+=U->udr->MetalDifference;
				NeededOnOffPower+=U->ud->energyUpkeep;
			}
		}
		else if( U->ud->energyUpkeep <= 0 && U->udr->EnergyDifference > 0 && U->udr->MetalDifference < 0 && U->udr->EnergyDifference < G->UDH->EnergyToMetalRatio*-U->udr->MetalDifference )
		{
			pmPI = &mMtoEConverter;
		}
		else if( U->ud->energyUpkeep > 0 )
		{
			pmPI = &mOnOffUnits;
			if( !cb->IsUnitActivated(unit) )
			{
				MetalProduction+=U->udr->MetalDifference;
				NeededOnOffPower+=U->ud->energyUpkeep;
			}
		}
		else if( U->ud->activateWhenBuilt && !cb->IsUnitActivated(unit) ) // solar panals are turned off when given, RAI does not manage there power
		{
			Command c;
			c.id=CMD_ONOFF;
			c.params.push_back(1);
			cb->GiveOrder(unit, &c);
		}
		
		if( pmPI != 0 )
		{
			pmPI->insert(cRAI::iupPair(unit,U));
			U->PowerUI = new sPowerUnitInfo(cb->IsUnitActivated(unit));
		}
	}
}

void cPowerManager::UnitDestroyed(int unit, UnitInfo *U)
{
	NeededWeaponPower-=U->udr->WeaponCostMax;

	if( mCloakUnits.find(unit) != mCloakUnits.end() )
	{
		if( !U->CloakUI->active )
			NeededCloakPower-=int(U->udr->CloakCostMax);
		mCloakUnits.erase(unit);
		delete U->CloakUI;
	}

	if( U->ud->onoffable )
	{
		map<int,UnitInfo*> *pmPI = 0;
		if( mEtoMConverter.find(unit) != mEtoMConverter.end() )
		{
			pmPI = &mEtoMConverter;
			if( !U->PowerUI->active )
			{
				MetalProductionEtoM-=U->udr->MetalDifference;
				NeededOnOffPower-=U->ud->energyUpkeep;
			}
		}
		else if( mMtoEConverter.find(unit) != mMtoEConverter.end() )
		{
			pmPI = &mMtoEConverter;
		}
		else if( mOnOffUnits.find(unit) != mOnOffUnits.end() )
		{
			pmPI = &mOnOffUnits;
			if( !U->PowerUI->active )
			{
				MetalProduction-=U->udr->MetalDifference;
				NeededOnOffPower-=U->ud->energyUpkeep;
			}
		}

		if( pmPI != 0 )
		{
			pmPI->erase(unit);
			delete U->PowerUI;
		}
	}
}

void cPowerManager::Update()
{
	float fMetal=cb->GetMetalIncome()-cb->GetMetalUsage();
	float fPower=cb->GetEnergyIncome()-cb->GetEnergyUsage();

	for( map<int,UnitInfo*>::iterator iU = mCloakUnits.begin(); iU!=mCloakUnits.end(); iU++ )
	{
		float fDrain=iU->second->udr->CloakCostMax;
		if( fDrain > fPower )
		{
			if( iU->second->CloakUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn OFF
				NeededCloakPower+=fDrain;
				fPower+=fDrain;
				iU->second->CloakUI->active = false;

				Command c;
				c.id=CMD_CLOAK;
				c.params.push_back(0);
				cb->GiveOrder(iU->first, &c);
			}
		}
		else if( fDrain*4 < fPower )
		{
			if( !iU->second->CloakUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn ON
				NeededCloakPower-=fDrain;
				fPower-=fDrain;
				iU->second->CloakUI->active = true;

				Command c;
				c.id=CMD_CLOAK;
				c.params.push_back(1);
				cb->GiveOrder(iU->first, &c);
			}
		}
	}

	for( map<int,UnitInfo*>::iterator iU = mEtoMConverter.begin(); iU!=mEtoMConverter.end(); iU++ )
	{
		//float fDrain=-iU->second->udr->EnergyDifference; // positive value ( if you include the - )
		float fDrain=iU->second->udr->ud->energyUpkeep; // positive value

		if( cb->GetMetal() > 0.85*cb->GetMetalStorage() || cb->GetEnergy()+(FUPDATE_POWER/30.0)*(fPower-fDrain) < 0.75*cb->GetEnergyStorage() )
		{
			if( iU->second->PowerUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn OFF
				MetalProductionEtoM+=iU->second->udr->MetalDifference;
				NeededOnOffPower+=fDrain;
				fPower+=fDrain;
				iU->second->PowerUI->active = false;

				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(0);
				cb->GiveOrder(iU->first, &c);
			}
		}
		else if( cb->GetEnergy()+2.0*(FUPDATE_POWER/30.0)*(fPower-fDrain) > 0.75*cb->GetEnergyStorage() )
		{
			if( !iU->second->PowerUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn ON
				MetalProductionEtoM-=iU->second->udr->MetalDifference;
				NeededOnOffPower-=fDrain;
				fPower-=fDrain;
				iU->second->PowerUI->active = true;

				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(1);
				cb->GiveOrder(iU->first, &c);
			}
		}
	}

	for( map<int,UnitInfo*>::iterator iU = mMtoEConverter.begin(); iU!=mMtoEConverter.end(); iU++ )
	{
		float fDrain=-iU->second->udr->MetalDifference; // positive value ( if you include the - )

		if( cb->GetEnergy() > 0.85*cb->GetEnergyStorage() || cb->GetMetal()+(FUPDATE_POWER/30.0)*(fMetal-fDrain) < 0.85*cb->GetMetalStorage() )
		{
			if( iU->second->PowerUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn OFF
				fMetal+=fDrain;
				iU->second->PowerUI->active = false;

				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(0);
				cb->GiveOrder(iU->first, &c);
			}
		}
		else if( cb->GetMetal()+2.0*(FUPDATE_POWER/30.0)*(fMetal-fDrain) > 0.85*cb->GetMetalStorage() )
		{
			if( !iU->second->PowerUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn ON
				fMetal-=fDrain;
				iU->second->PowerUI->active = true;

				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(1);
				cb->GiveOrder(iU->first, &c);
			}
		}
	}

	for( map<int,UnitInfo*>::iterator iU = mOnOffUnits.begin(); iU!=mOnOffUnits.end(); iU++ )
	{
		float fDrain=iU->second->udr->ud->energyUpkeep; // positive value
		if( cb->GetEnergy()+(FUPDATE_POWER/30.0)*(fPower-fDrain) < 0 )
		{
			if( iU->second->PowerUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn OFF
				MetalProduction+=iU->second->udr->MetalDifference;
				NeededOnOffPower+=fDrain;
				fPower+=fDrain;
				iU->second->PowerUI->active = false;

				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(0);
				cb->GiveOrder(iU->first, &c);
			}
		}
		else if( cb->GetEnergy()+2.0*(FUPDATE_POWER/30.0)*(fPower-fDrain) > 0 )
		{
			if( !iU->second->PowerUI->active && !G->IsHumanControled(iU->first,iU->second) )
			{	// Turn ON
				MetalProduction-=iU->second->udr->MetalDifference;
				NeededOnOffPower-=fDrain;
				fPower-=fDrain;
				iU->second->PowerUI->active = true;

				Command c;
				c.id=CMD_ONOFF;
				c.params.push_back(1);
				cb->GiveOrder(iU->first, &c);
			}
		}
	}
}
