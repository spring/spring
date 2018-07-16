#include "PowerManager.h"
#include "LegacyCpp/WeaponDef.h"

UnitInfoPower::UnitInfoPower(int UID, UnitInfo *UI, bool isActive, int listType)
{
	unitID = UID;
	U = UI;
	active = isActive;
	importance = 1.0;
	type = listType;
	index = 0;
}

cPowerManager::cPowerManager(IAICallback* callback, cRAI* global)
{
	G=global;
	l=G->l;
	cb=callback;

	*l<<"\n Loading cPowerManager ...";

	MetalDifference = 0;
	EnergyDifference = 0;
	WeaponEnergyNeeded = 0;

	OffEnergyDifference = 0;
	OnEnergyDifference = 0;

	EtoMNeeded = 0.0;
	MtoENeeded = 0.0;
	EtoMIncome = 0.0;
	MtoEIncome = 0.0;
	ExMMetalDifference = 0;
	ExMEnergyDifference = 0;

	EDrainSize = 0;
	EDrainActive = 0;
	EtoMSize = 0;
	EtoMActive = 0;
	MtoESize = 0;
	MtoEActive = 0;

	cb->GetValue(AIVAL_UNIT_LIMIT,&UIPLimit);
	EDrain = new UnitInfoPower*[UIPLimit];
	EtoM = new UnitInfoPower*[UIPLimit];
	MtoE = new UnitInfoPower*[UIPLimit];

	DebugUnitFinished = 0;
	DebugUnitDestroyed = 0;
	UIPLimit = 0;
}

cPowerManager::~cPowerManager()
{
	if( RAIDEBUGGING )
	{
		*l<<"\n cPowerManager Debug:";
		*l<<"\n  MetalDifference = "<<MetalDifference;
		*l<<"\n  EnergyDifference = "<<EnergyDifference;
		*l<<"\n  WeaponEnergyNeeded = "<<WeaponEnergyNeeded;
		*l<<"\n  OffEnergyDifference = "<<OffEnergyDifference;
		*l<<"\n  OnEnergyDifference = "<<OnEnergyDifference;
		*l<<"\n  ExMMetalDifference = "<<ExMMetalDifference;
		*l<<"\n  ExMEnergyDifference = "<<ExMEnergyDifference;
		*l<<"\n  EtoMNeeded = "<<EtoMNeeded;
		*l<<"\n  EtoMIncome = "<<EtoMIncome;
		*l<<"\n  MtoEIncome = "<<MtoEIncome;
		*l<<"\n  MtoENeeded = "<<MtoENeeded;
		*l<<"\n  EDrain/EtoM/MtoE Size = "<<EDrainSize<<"/"<<EtoMSize<<"/"<<MtoESize;
		*l<<"\n  Debug-Units: Finished("<<DebugUnitFinished<<") - Destroyed("<<DebugUnitDestroyed<<") = "<<DebugUnitFinished-DebugUnitDestroyed;

		if( (int)MetalDifference != 0 || (int)EnergyDifference != 0 || (int)WeaponEnergyNeeded != 0
		 || (int)OffEnergyDifference != 0 || (int)OnEnergyDifference != 0 || (int)ExMMetalDifference != 0 || (int)ExMEnergyDifference != 0
		 || (int)EtoMNeeded != 0 || (int)EtoMIncome != 0 || (int)MtoEIncome != 0 || (int)MtoENeeded != 0
		 || EDrainSize != 0 || EtoMSize != 0 || MtoESize != 0 )
			*l<<"\n  (ERROR)";
	}

	// These should be empty at this point
	delete [] EDrain;
	delete [] EtoM;
	delete [] MtoE;
}

void cPowerManager::UnitFinished(int unit, UnitInfo *U)
{
	DebugUnitFinished++;
	EnergyDifference += U->ud->energyMake;
	MetalDifference += U->ud->metalMake;
	WeaponEnergyNeeded += U->udr->WeaponEnergyDifference;
	if( U->ud->canCloak && EDrainSize < UIPLimit )
	{
		if( U->udr->CloakMaxEnergyDifference < 0 )
		{
			U->CloakUI = new UnitInfoPower(unit,U,cb->IsUnitCloaked(unit),0);
			U->CloakUI->importance = 100/-U->udr->CloakMaxEnergyDifference;
			if( U->ud->isCommander )
				U->CloakUI->importance *= 3;
			InsertPI(EDrain,EDrainSize,U->CloakUI);
			if( U->CloakUI->index < EDrainActive )
			{
				EDrainActive++;
				if( !U->CloakUI->active )
					GiveCloakOrder(unit,U,true);
				OnEnergyDifference += U->udr->CloakMaxEnergyDifference;
			}
			else
			{
				if( U->CloakUI->active )
					GiveCloakOrder(unit,U,false);
				OffEnergyDifference += U->udr->CloakMaxEnergyDifference;
			}
		}
		else if( !cb->IsUnitCloaked(unit) ) // 0 cost cloak? always use it
			GiveCloakOrder(unit);
	}
	if( U->ud->onoffable )
	{
		if( U->udr->OnOffEnergyDifference < 0 && U->udr->OnOffMetalDifference > 0 && U->udr->MetalDifference < -U->udr->EnergyDifference*G->UDH->EnergyToMetalRatio && EtoMSize < UIPLimit )
		{	// Metal Maker
			U->OnOffUI = new UnitInfoPower(unit,U,cb->IsUnitActivated(unit),3);
			U->OnOffUI->importance = U->udr->OnOffMetalDifference/-U->udr->OnOffEnergyDifference;
			InsertPI(EtoM,EtoMSize,U->OnOffUI);
			if( U->OnOffUI->index <= EtoMActive )
			{
				EtoMActive++;
				if( !U->OnOffUI->active )
					GiveOnOffOrder(unit,U,true);
				ExMMetalDifference += U->udr->OnOffMetalDifference;
				ExMEnergyDifference += U->udr->OnOffEnergyDifference;
			}
			else
			{
				if( U->OnOffUI->active )
					GiveOnOffOrder(unit,U,false);
				EtoMIncome += U->udr->OnOffMetalDifference;
				EtoMNeeded += U->udr->OnOffEnergyDifference;
			}
		}
		else if( U->udr->OnOffEnergyDifference > 0 && U->udr->OnOffMetalDifference < 0 && U->udr->EnergyDifference*G->UDH->EnergyToMetalRatio < -U->udr->MetalDifference && EtoMSize < UIPLimit )
		{	// Energy Maker
			U->OnOffUI = new UnitInfoPower(unit,U,cb->IsUnitActivated(unit),4);
			U->OnOffUI->importance = U->udr->OnOffEnergyDifference/-U->udr->OnOffMetalDifference;
			InsertPI(MtoE,MtoESize,U->OnOffUI);
			if( U->OnOffUI->index <= MtoEActive )
			{
				MtoEActive++;
				if( !U->OnOffUI->active )
					GiveOnOffOrder(unit,U,true);
				ExMMetalDifference += U->udr->OnOffMetalDifference;
				ExMEnergyDifference += U->udr->OnOffEnergyDifference;
			}
			else
			{
				if( U->OnOffUI->active )
					GiveOnOffOrder(unit,U,false);
				MtoEIncome += U->udr->OnOffEnergyDifference;
				MtoENeeded += U->udr->OnOffMetalDifference;
			}
		}
		else if( U->ud->energyUpkeep > 0 && EDrainSize < UIPLimit )
		{	// On/Off units that require energy
			U->OnOffUI = new UnitInfoPower(unit,U,cb->IsUnitActivated(unit),1);
			U->OnOffUI->importance = 100/-U->udr->OnOffEnergyDifference;
			if( U->ud->isCommander || U->ud->extractsMetal > 0 )
				U->OnOffUI->importance *= 3;
			InsertPI(EDrain,EDrainSize,U->OnOffUI);
			if( U->OnOffUI->index <= EDrainActive )
			{
				EDrainActive++;
				if( !U->OnOffUI->active )
					GiveOnOffOrder(unit,U,true);
				OnEnergyDifference += U->udr->OnOffEnergyDifference;
			}
			else
			{
				if( U->OnOffUI->active )
					GiveOnOffOrder(unit,U,false);
				OffEnergyDifference += U->udr->OnOffEnergyDifference;
			}
		}
		else
		{	// On/Off units that do not require energy
			EnergyDifference += U->udr->OnOffEnergyDifference;
			MetalDifference += U->udr->OnOffMetalDifference;
			if( !cb->IsUnitActivated(unit) ) // solar panals are turned off when given, RAI does not manage their states
				GiveOnOffOrder(unit);
		}
	}
}

void cPowerManager::UnitDestroyed(int unit, UnitInfo *U)
{
	DebugUnitDestroyed++;
	EnergyDifference -= U->ud->energyMake;
	MetalDifference -= U->ud->metalMake;
	WeaponEnergyNeeded -= U->udr->WeaponEnergyDifference;

	if( U->CloakUI != 0 )
	{
		if( U->CloakUI->active )
		{
			OnEnergyDifference -= U->udr->CloakMaxEnergyDifference;
			EDrainActive--;
		}
		else
			OffEnergyDifference -= U->udr->CloakMaxEnergyDifference;
		RemovePI(EDrain,EDrainSize,U->CloakUI);
		delete U->CloakUI;
	}

	if( U->OnOffUI != 0 )
	{
		if( U->OnOffUI->type == 3 )
		{
			if( U->OnOffUI->active )
			{
				ExMMetalDifference -= U->udr->OnOffMetalDifference;
				ExMEnergyDifference -= U->udr->OnOffEnergyDifference;
				EtoMActive--;
			}
			else
			{
				EtoMIncome -= U->udr->OnOffMetalDifference;
				EtoMNeeded -= U->udr->OnOffEnergyDifference;
			}
			RemovePI(EtoM,EtoMSize,U->OnOffUI);
		}
		else if( U->OnOffUI->type == 4 )
		{
			if( U->OnOffUI->active )
			{
				ExMMetalDifference -= U->udr->OnOffMetalDifference;
				ExMEnergyDifference -= U->udr->OnOffEnergyDifference;
				MtoEActive--;
			}
			else
			{
				MtoEIncome -= U->udr->OnOffEnergyDifference;
				MtoENeeded -= U->udr->OnOffMetalDifference;
			}
			RemovePI(MtoE,MtoESize,U->OnOffUI);
		}
		else if( U->OnOffUI->type == 1 )
		{
			if( U->OnOffUI->active )
			{
				OnEnergyDifference -= U->udr->OnOffEnergyDifference;
				EDrainActive--;
			}
			else
				OffEnergyDifference -= U->udr->OnOffEnergyDifference;
			RemovePI(EDrain,EDrainSize,U->OnOffUI);
		}
		delete U->OnOffUI;
	}
	else if( U->ud->onoffable )
	{
		EnergyDifference -= U->udr->OnOffEnergyDifference;
		MetalDifference -= U->udr->OnOffMetalDifference;
	}
}

void cPowerManager::Update()
{
	sRAIUnitDef* udr;
	float factor = (FUPDATE_POWER/30.0);
	float energyDif = cb->GetEnergyIncome()-cb->GetEnergyUsage();
	float metalDif = cb->GetMetalIncome()-cb->GetMetalUsage();
	float energyCurrent = cb->GetEnergy() + energyDif*2*factor;
	float metalCurrent = cb->GetMetal() + metalDif*2*factor;
	float metalNext,energyNext;
	bool ConvertEtoM;
	bool ShiftEtoM = G->UDH->EnergyToMetalRatio*metalDif < energyDif;
	if( EtoMActive > 0 )
		ConvertEtoM = true;
	else if( MtoEActive > 0 )
		ConvertEtoM = false;
	else
		ConvertEtoM = ShiftEtoM;
	float energyLimit = 0.8*cb->GetEnergyStorage();
	float metalLimit = 0.8*cb->GetMetalStorage();

	energyDif -= ExMEnergyDifference;
	bool ShiftPowerON = energyDif > 0 && cb->GetEnergy() > 0;
//	*l<<"\n ShiftPowerON="<<ShiftPowerON;
//	*l<<"\n ShiftEtoM="<<ShiftEtoM;
//	*l<<"\n energy="<<cb->GetEnergyIncome()<<"-"<<cb->GetEnergyUsage()<<"-"<<ExMEnergyDifference<<"="<<energyDif;
//	*l<<"\n energyCurrent = "<<energyCurrent<<" of "<<energyLimit;
//	*l<<"\n cb->GetEnergy()="<<cb->GetEnergy();
//	*l<<"\n metalDif="<<metalDif;
//	*l<<"\n metalCurrent = "<<metalCurrent<<" of "<<metalLimit;
	if( ShiftPowerON )
	{
//*l<<" (P-ON)="<<EDrainActive<<" of "<<EDrainSize<<":";
		while( EDrainActive < EDrainSize && energyCurrent > 0 )
		{
//*l<<" "<<EDrainActive;
			udr = EDrain[EDrainActive]->U->udr;
			if( EDrain[EDrainActive]->type == 0 )
				energyNext = energyCurrent + udr->CloakMaxEnergyDifference*2*factor;
			else
				energyNext = energyCurrent + udr->OnOffEnergyDifference*2*factor;
			if( energyNext > 0 )
			{	// Turn ON
				if( EDrain[EDrainActive]->type == 0 )
				{
					GiveCloakOrder(EDrain[EDrainActive]->unitID,EDrain[EDrainActive]->U,true);
					OffEnergyDifference -= udr->CloakMaxEnergyDifference;
					OnEnergyDifference += udr->CloakMaxEnergyDifference;
					energyCurrent += udr->CloakMaxEnergyDifference*factor;
				}
				else
				{
					GiveOnOffOrder(EDrain[EDrainActive]->unitID,EDrain[EDrainActive]->U,true);
					OffEnergyDifference -= udr->OnOffEnergyDifference;
					OnEnergyDifference += udr->OnOffEnergyDifference;
					energyCurrent += udr->OnOffEnergyDifference*factor;
					metalCurrent += udr->OnOffMetalDifference*factor;
				}
				EDrainActive++;
			}
			else
				break;
		}
	}
	else
	{
//*l<<" (P-OFF)="<<EDrainActive<<" of "<<EDrainSize<<":";
		while( EDrainActive > 0 && energyCurrent < 0 )
		{
//*l<<" "<<EDrainActive;
			udr = EDrain[EDrainActive-1]->U->udr;
			if( EDrain[EDrainActive-1]->type != 0 || udr->OnOffMetalDifference <= 0 || metalCurrent-udr->OnOffMetalDifference*factor > 0.5*cb->GetMetalStorage() )
			{	// Turn OFF
				EDrainActive--;
				if( EDrain[EDrainActive]->type == 0 )
				{
					GiveCloakOrder(EDrain[EDrainActive]->unitID,EDrain[EDrainActive]->U,false);
					OffEnergyDifference += udr->CloakMaxEnergyDifference;
					OnEnergyDifference -= udr->CloakMaxEnergyDifference;
					energyCurrent -= udr->CloakMaxEnergyDifference*factor;
				}
				else
				{
					GiveOnOffOrder(EDrain[EDrainActive]->unitID,EDrain[EDrainActive]->U,false);
					OffEnergyDifference += udr->OnOffEnergyDifference;
					OnEnergyDifference -= udr->OnOffEnergyDifference;
					energyCurrent -= udr->OnOffEnergyDifference*factor;
					metalCurrent -= udr->OnOffMetalDifference*factor;
				}
			}
			else
				break;
		}
	}
	if( ConvertEtoM )
	{
		if( ShiftEtoM )
		{
//*l<<" (EM-ON)="<<EtoMActive<<" of "<<EtoMSize<<":";
			while( EtoMActive < EtoMSize && energyCurrent > energyLimit )
			{
//*l<<" "<<EtoMActive;
				udr = EtoM[EtoMActive]->U->udr;
				energyNext = energyCurrent + udr->OnOffEnergyDifference*factor;
				if( energyNext > energyLimit )
					energyNext = energyLimit;
				metalNext = metalCurrent + udr->OnOffMetalDifference*factor;
				if( metalNext > metalLimit )
					metalNext = metalLimit;
//*l<<"\n  energyNext="<<energyNext;
//*l<<"\n  metalNext="<<metalNext;
				if( G->UDH->EnergyToMetalRatio*metalNext+energyNext > G->UDH->EnergyToMetalRatio*metalCurrent+energyLimit )
				{	// Turn ON
					GiveOnOffOrder(EtoM[EtoMActive]->unitID,EtoM[EtoMActive]->U,true);
					EtoMActive++;

					EtoMIncome -= udr->OnOffMetalDifference;
					EtoMNeeded -= udr->OnOffEnergyDifference;
					ExMMetalDifference += udr->OnOffMetalDifference;
					ExMEnergyDifference += udr->OnOffEnergyDifference;

					metalCurrent += udr->OnOffMetalDifference*factor;
					energyCurrent += udr->OnOffEnergyDifference*factor;
				}
				else
					break;
			}
		}
		else
		{
//*l<<" (EM-OFF)="<<EtoMActive<<" of "<<EtoMSize<<":";
			while( EtoMActive > 0 && energyCurrent < 0.1*cb->GetEnergyStorage() )
			{
//*l<<" "<<EtoMActive;
				udr = EtoM[EtoMActive-1]->U->udr;
				energyNext = energyCurrent - udr->OnOffEnergyDifference*factor;
				if( energyNext > energyLimit )
					energyNext = energyLimit;
				metalNext = metalCurrent - udr->OnOffMetalDifference*factor;
				if( metalNext > metalLimit )
					metalNext = metalLimit;
				if( G->UDH->EnergyToMetalRatio*metalNext+energyNext > G->UDH->EnergyToMetalRatio*metalCurrent+energyCurrent )
				{	// Turn OFF
					EtoMActive--;
					GiveOnOffOrder(EtoM[EtoMActive]->unitID,EtoM[EtoMActive]->U,false);

					EtoMIncome += udr->OnOffMetalDifference;
					EtoMNeeded += udr->OnOffEnergyDifference;
					ExMMetalDifference -= udr->OnOffMetalDifference;
					ExMEnergyDifference -= udr->OnOffEnergyDifference;

					metalCurrent -= udr->OnOffMetalDifference*factor;
					energyCurrent -= udr->OnOffEnergyDifference*factor;
				}
				else
					break;
			}
		}
	}
	else
	{
		if( !ShiftEtoM )
		{
//*l<<" (ME-ON)="<<MtoEActive<<" of "<<MtoESize<<":";
			while( MtoEActive < MtoESize && metalCurrent > metalLimit )
			{
//*l<<" "<<MtoEActive;
				udr = MtoE[MtoEActive]->U->udr;
				energyNext = energyCurrent + udr->OnOffEnergyDifference*factor;
				if( energyNext > energyLimit )
					energyNext = energyLimit;
				metalNext = metalCurrent + udr->OnOffMetalDifference*factor;
				if( metalNext > metalLimit )
					metalNext = metalLimit;
//*l<<"\n  energyNext="<<energyNext;
//*l<<"\n  metalNext="<<metalNext;
				if( G->UDH->EnergyToMetalRatio*metalNext+energyNext > G->UDH->EnergyToMetalRatio*metalLimit+energyCurrent )
				{	// Turn ON
					GiveOnOffOrder(MtoE[MtoEActive]->unitID,MtoE[MtoEActive]->U,true);
					MtoEActive++;

					MtoEIncome -= udr->OnOffEnergyDifference;
					MtoENeeded -= udr->OnOffMetalDifference;
					ExMMetalDifference += udr->OnOffMetalDifference;
					ExMEnergyDifference += udr->OnOffEnergyDifference;

					metalCurrent += udr->OnOffMetalDifference*factor;
					energyCurrent += udr->OnOffEnergyDifference*factor;
				}
				else
					break;
			}
		}
		else
		{
//*l<<" (ME-OFF)="<<MtoEActive<<" of "<<MtoESize<<":";
			while( MtoEActive > 0 && metalCurrent < 0.1*cb->GetMetalStorage() )
			{
//*l<<" "<<MtoEActive;
				udr = MtoE[MtoEActive-1]->U->udr;
				energyNext = energyCurrent - udr->OnOffEnergyDifference*factor;
				if( energyNext > energyLimit )
					energyNext = energyLimit;
				metalNext = metalCurrent - udr->OnOffMetalDifference*factor;
				if( metalNext > metalLimit )
					metalNext = metalLimit;
				if( G->UDH->EnergyToMetalRatio*metalNext+energyNext > G->UDH->EnergyToMetalRatio*metalCurrent+energyCurrent )
				{	// Turn OFF
					MtoEActive--;
					GiveOnOffOrder(MtoE[MtoEActive]->unitID,MtoE[MtoEActive]->U,false);

					MtoEIncome += udr->OnOffEnergyDifference;
					MtoENeeded += udr->OnOffMetalDifference;
					ExMMetalDifference -= udr->OnOffMetalDifference;
					ExMEnergyDifference -= udr->OnOffEnergyDifference;

					metalCurrent -= udr->OnOffMetalDifference*factor;
					energyCurrent -= udr->OnOffEnergyDifference*factor;
				}
				else
					break;
			}
		}
	}
}

void cPowerManager::InsertPI(UnitInfoPower **PIA, int &PIASize, UnitInfoPower *P)
{
	for(P->index = PIASize; P->index>0; P->index-- )
		if( P->importance > PIA[P->index-1]->importance )
		{
			PIA[P->index] = PIA[P->index-1];
			PIA[P->index]->index = P->index;
		}
		else
			break;

	PIA[P->index] = P;
	PIASize++;
}

void cPowerManager::RemovePI(UnitInfoPower **PIA, int &PIASize, UnitInfoPower *P)
{
	for(int i=P->index; i<PIASize-1; i++ )
	{
		PIA[i] = PIA[i+1];
		PIA[i]->index = i;
	}
	PIASize--;
}

void cPowerManager::GiveCloakOrder(const int &unitID, UnitInfo *U, bool state )
{
	if( U != 0 )
		U->CloakUI->active = state;
	Command c(CMD_CLOAK);
	c.PushParam(state);
	cb->GiveOrder(unitID, &c);
}

void cPowerManager::GiveOnOffOrder(const int &unitID, UnitInfo *U, bool state )
{
	if( U != 0 )
		U->OnOffUI->active = state;
	Command c(CMD_ONOFF);
	c.PushParam(state);
	cb->GiveOrder(unitID, &c);
}
