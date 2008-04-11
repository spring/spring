#include "Builder.h"
//#include "Sim/Units/UnitDef.h"
//struct WeaponDef;
//#include <set>
#include "Sim/Features/FeatureDef.h"
#include "Sim/Weapons/WeaponDefHandler.h"

sBuildQuarry::sBuildQuarry(sRAIBuildList *buildlist)
{
	BL=buildlist;
	if( BL != 0 )
		BL->unitsActive++;
	builderID=-1;
	RS=0;
	tryCount=0;
};

sBuildQuarry::~sBuildQuarry()
{
	if( BL != 0 )
		BL->unitsActive--;
};

bool sBuildQuarry::IsValid(int frame)
{
	if( int(creationID.size()) == 0 && builderID == -1 && frame >= deletionFrame )
		return false;
	return true;
};

void sBuildQuarry::SetRS(sResourceSpot* rs)
{
	if( RS != 0 )
		RS->builderID=-1;
	RS = rs;
	if( RS != 0 )
		RS->builderID=builderID;
};
// ------------------------------------------------------------------------------------------------

cBuilder::cBuilder(IAICallback* callback, cRAI* global)
{
	cb = callback;
	G = global;
	UDR = G->UDH;
	for( map<int,sRAIUnitDef>::iterator iU=UDR->UDR.begin(); iU!=UDR->UDR.end(); iU++ )
		if( int(iU->second.ud->wreckName.size()) > 0 )
			UDRResurrect.insert(srPair(iU->second.ud->wreckName,&iU->second));

	l=G->l;
	bInitiated=false;
	ConDrainEnergyDebug=0;
	ConDrainMetalDebug=0;
	ConDrainFrameDebug=0;
	ConEnergyLost=0;
	ConMetalLost=0;
	ConEnergyDrain=0;
	ConMetalDrain=0;
	ConEnergyRate=0;
	ConMetalRate=0;
	ConEnergyStorage=0;
	ConMetalStorage=0;

	for( int i=0; i<8; i++ )
		BQSize[i]=0;
	Prerequisite=0;

	*l<<"\n Loading cPowerManager ...";
	PM = new cPowerManager(cb,G);
	BP = 0;
}

cBuilder::~cBuilder()
{
	if( int(G->Units.size()) == 0 )
	{
		*l<<"\n cBuilder Debug:";
		*l<<"\n  clearing Build Quarry ...";
		while( BQSize[0] > 0 )
			BQRemove(0);

		*l<<"\n  Builders Active      = "<<int(UBuilder.size());
		*l<<"\n  Constructions Active = "<<int(UConstruction.size());
		*l<<"\n  Metal Cost Limit     = "<<MCostLimit;
		*l<<"\n  Energy Cost Limit    = "<<ECostLimit;
		*l<<"\n  Const Energy Drain   = "<<ConEnergyDrain;
		*l<<"\n  Const Metal Drain    = "<<ConMetalDrain;
		*l<<"\n  Const Energy Rate    = "<<ConEnergyRate;
		*l<<"\n  Const Metal Rate     = "<<ConMetalRate;
		*l<<"\n  Const Energy Storage = "<<ConEnergyStorage;
		*l<<"\n  Const Metal Storage  = "<<ConMetalStorage;
//		if( ConEnergyRate != 0 || ConMetalRate != 0 || ConEnergyStorage != 0 || ConMetalStorage != 0 )
//			*l<<"\n ERROR!";
	}

	if( BP != 0 )
		delete BP;
	delete PM;
}

void cBuilder::UnitCreated(const int& unit, UnitInfo *U)
{
	if( BP == 0 )
	{
		*l<<"\n Loading cBuilderPlacement ...";
		BP = new cBuilderPlacement(cb,G);
	}
	// Attempts to identify which Build Quarry this new unit belongs to.
	int iBQIndex=-1;
	for(int iBQ=0; iBQ<BQSize[0]; iBQ++)
	{
		if(	BQ[iBQ]->creationUDID==U->ud->id && BQ[iBQ]->builderID > -1 && G->ValidateUnit(BQ[iBQ]->builderID) && int(cb->GetCurrentUnitCommands(BQ[iBQ]->builderID)->size())>0 )
		{
			const Command *c=&cb->GetCurrentUnitCommands(BQ[iBQ]->builderID)->front();
			if( c->id == -U->ud->id )
			{
				if( int(c->params.size()) >= 3 ) // (0.73b1): for some reason human issued build commands do not have params
				{
					float fx = cb->GetUnitPos(unit).x - c->params.at(0);
					float fz = cb->GetUnitPos(unit).z - c->params.at(2);
					float fx2 = cb->GetUnitPos(unit).x - cb->GetUnitPos(BQ[iBQ]->builderID).x;
					float fz2 = cb->GetUnitPos(unit).z - cb->GetUnitPos(BQ[iBQ]->builderID).z;
					float fMarginOfError=BQ[iBQ]->builderUI->ud->buildDistance + 50;
					if( fMarginOfError < 300 )
						fMarginOfError = 300;

					// The margin of error in position can be quite large, the size of units can distort their intended build positions
					// Thus constructers, which are set to build at a certain position, can build at a distance that even exceeds their build distance (or at least, that is what these variables have told me during testing)
					if( (fx < fMarginOfError && fx > -fMarginOfError && fz < fMarginOfError && fz > -fMarginOfError) ||
						(fx2 < fMarginOfError && fx2 > -fMarginOfError && fz2 < fMarginOfError && fz2 > -fMarginOfError) )
					{
//						*l<<"\n\nUnit Position:  x="<<cb->GetUnitPos(unit).x<<" z="<<cb->GetUnitPos(unit).z;
//						*l<<"\nBuilder Position: x="<<cb->GetUnitPos(BQ[iBQ]->builderID).x<<" z="<<cb->GetUnitPos(BQ[iBQ]->builderID).z;
//						*l<<"\nBOrder Position:  x="<<c->params.at(0)<<" z="<<c->params.at(2);
//						*l<<"\nMarginOfError: "<<fMarginOfError<<"\n\n";
						if( iBQIndex > -1 ) // It found more than 1 possible Build Quarry match
						{
							*l<<"\nWARNING: Creation Def Conflict, code needs works.";
						}
						iBQIndex=iBQ;
					}
				}
			}
		}
	}
	int humanCreated=-1;
	if( iBQIndex == -1 ) // This is a bit more thorough check for a human builder
	{
		G->ValidateAllUnits();
		for(map<int,UnitInfo>::iterator iU=G->Units.begin(); iU!=G->Units.end(); iU++)
		{
			if(	cb->GetCurrentUnitCommands(iU->first)->size() > 0 )
			{
				const Command *c=&cb->GetCurrentUnitCommands(iU->first)->front();
				if( c->id == -U->ud->id && int(c->params.size()) == 0 )
				{
					float fx2 = cb->GetUnitPos(unit).x - cb->GetUnitPos(iU->first).x;
					float fz2 = cb->GetUnitPos(unit).z - cb->GetUnitPos(iU->first).z;
					float fMarginOfError=iU->second.ud->buildDistance + 150;
					if( fMarginOfError < 300 )
						fMarginOfError = 300;

					if( fx2 < fMarginOfError && fx2 > -fMarginOfError && fz2 < fMarginOfError && fz2 > -fMarginOfError )
					{
						humanCreated = iU->first;
					}
				}
			}
		}
	}
	if( iBQIndex >= 0 )
	{
		typedef pair<int,UnitConstructionInfo> iuPair;
		UConstruction.insert(iuPair(unit,UnitConstructionInfo(BQ[iBQIndex],unit,U)));
		U->AIDisabled=false;
	}
	else if( humanCreated >= 0 && (U->ud->speed > 0 || int(U->udr->BuildOptions.size()) == 0) )
	{
		U->AIDisabled=true;
	}
	else
	{
		// This case can occur 4 ways: 1) starting units 2) resurrected units 3) RAI failed to identify it's builder 4) factory built by human player
		U->AIDisabled=false;
	}
	if( !U->AIDisabled )
		U->udr->UnitConstructsActive.insert(unit);
}

void cBuilder::UnitFinished(const int& unit, UnitInfo *U)
{
	if( bInitiated && UConstruction.find(unit) != UConstruction.end() )
	{
		UnitConstructionInfo *pUCQ=&UConstruction.find(unit)->second;
		if( pUCQ->BQAbandoned )
		{
			UConstruction.erase(unit); // Remove from list of units being built
		}
		else
		{
			if( int(cb->GetCurrentUnitCommands(unit)->size()) == 0 && UBuilder.find(pUCQ->BQ->builderID) != UBuilder.end() )
			{
				if( pUCQ->BQ->builderUI->ud->speed == 0 )
				{
					Command c;
					c.id=CMD_MOVE;
					float3 fPos=cb->GetUnitPos(unit);
					fPos.x+=-100+rand()%201;
					if( cb->GetBuildingFacing(pUCQ->BQ->builderID) == 2 )
						fPos.z-=150+rand()%201;
					else
						fPos.z+=150+rand()%201;
					fPos.y=cb->GetElevation(fPos.x,fPos.z);
					c.params.push_back(fPos.x);
					c.params.push_back(fPos.y);
					c.params.push_back(fPos.z);
					cb->GiveOrder(unit,&c);
					U->commandTimeOut=cb->GetCurrentFrame()+150;
				}
			}
			BQRemove(pUCQ->BQ->index); // Remove from global build instructions
		}
	}
	U->udr->UnitsActive.insert(unit);
	U->udr->UnitConstructsActive.erase(unit);
	UnitAssignBuildList(unit,U);
	U->udr->CheckUnitLimit();
	U->udr->CheckBuildOptions();
	if( U->ud->speed == 0 )
	{
		for(map<int,UnitInfo*>::iterator i=UNanos.begin(); i!=UNanos.end(); i++ )
		{
			if( cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(i->first)) < i->second->ud->buildDistance )
			{
				i->second->UGuarding.insert(cRAI::iupPair(unit,U));
				U->UGuards.insert(cRAI::iupPair(i->first,i->second));
				if( U->pBOL != 0 && U->pBOL->task == TASK_CONSTRUCT && int(U->udr->BuildOptions.size()) > 0 )
					i->second->UAssist.insert(cRAI::iupPair(unit,U));
			}
		}
		if( U->udr->IsNano() )
		{
			UNanos.insert(cRAI::iupPair(unit,U));
			for(map<int,UnitInfo*>::iterator i=G->UImmobile.begin(); i!=G->UImmobile.end(); i++ )
			{
				if( i->first != unit && i->second->UnitFinished && cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(i->first)) < U->ud->buildDistance )
				{
					U->UGuarding.insert(cRAI::iupPair(i->first,i->second));
					i->second->UGuards.insert(cRAI::iupPair(unit,U));
					if( i->second->pBOL->task == TASK_CONSTRUCT && int(i->second->udr->BuildOptions.size()) > 0 )
						U->UAssist.insert(cRAI::iupPair(i->first,i->second));
				}
			}
		}
	}
}

void cBuilder::UnitDestroyed(const int& unit, UnitInfo* U)
{
	if( !U->UnitFinished )
	{
		if( UConstruction.find(unit) != UConstruction.end() )
		{
			if( !UConstruction.find(unit)->second.BQAbandoned )
			{
				UConstruction.find(unit)->second.BQ->creationID.remove(unit);
			}
			UConstruction.erase(unit);
		}
		U->udr->UnitConstructsActive.erase(unit);
	}
	else
	{
		U->udr->UnitsActive.erase(unit);
		U->pBOL->RBL->unitsActive--;
		U->udr->CheckUnitLimit();
		U->udr->CheckBuildOptions();
		if( U->ud->speed == 0 )
		{
			if( U->udr->IsNano() )
			{
				UNanos.erase(unit);
				for(map<int,UnitInfo*>::iterator i=U->UGuarding.begin(); i!=U->UGuarding.end(); i++ )
					i->second->UGuards.erase(unit);
			}
			for(map<int,UnitInfo*>::iterator i=U->UGuards.begin(); i!=U->UGuards.end(); i++ )
			{
				i->second->UGuarding.erase(unit);
				i->second->UAssist.erase(unit);
			}
		}
	}
}

void cBuilder::UnitAssignBuildList(const int& unit, UnitInfo *U, bool bInitialized)
{
	if( bInitialized )
		U->pBOL->RBL->unitsActive--;
	float fBestV=-1;	// Value
	int iBestI=-1;		// Index
	for(int i=0; i<U->udr->ListSize; i++ )
	{
		sRAIBuildList* BL=U->udr->List[i]->RBL;
		if( BL->unitsActive < BL->minUnits && (iBestI == -1 || U->udr->List[iBestI]->RBL->minUnits == 0 || float(BL->unitsActive)/float(BL->minUnits) < fBestV) )
		{
			iBestI = i;
			fBestV = float(BL->unitsActive)/float(BL->minUnits);
		}
		else if( fBestV == -1 || ( BL->priority > 0 && U->udr->List[iBestI]->RBL->unitsActive >= U->udr->List[iBestI]->RBL->minUnits && float(1+BL->unitsActive)/float(BL->priority) < fBestV ) )
		{
			iBestI = i;
			fBestV = float(1+BL->unitsActive)/float(BL->priority);
		}
	}

	if( iBestI == -1 )
	{
		*l<<"\n\nWARNING: unknown unit type in use: ud->id="<<U->ud->id;
//		cb->SendTextMsg("unknown unit type in use",0);
		U->udr->UnitsActive.erase(unit);
		U->AIDisabled=true;
		return;
	}
	U->pBOL=U->udr->List[iBestI];
	U->pBOL->RBL->unitsActive++;
}

void cBuilder::UBuilderFinished(const int& unit, UnitInfo *U)
{
	UBuilder.insert(cRAI::iupPair(unit,U));
	if( int(U->udr->UnitsActive.size()) == 1 )
		UpdateUDRCost();
}

void cBuilder::UBuilderDestroyed(const int& unit, UnitInfo *U)
{
	if( U->BuildQ != 0 )
		BQAssignBuilder(U->BuildQ->index,-1,0);

	UBuilder.erase(unit);
	if( int(U->udr->UnitsActive.size()) == 0 )
		UpdateUDRCost();
}

void cBuilder::UBuilderIdle(const int& unit,UnitInfo *U)
{
	int iFrame = cb->GetCurrentFrame();
	float fOnOffPower = PM->NeededOnOffPower;
	float fCloakPower = PM->NeededCloakPower;
	if( fCloakPower > 0.10*cb->GetEnergyIncome() )
		fCloakPower = 0.10*cb->GetEnergyIncome();
	float fWeaponPower = PM->NeededWeaponPower;
	if( fWeaponPower > 0.25*cb->GetEnergyIncome() )
		fWeaponPower = 0.25*cb->GetEnergyIncome();
	float MRatio;
	if( cb->GetMetalIncome() == 0 || cb->GetMetalUsage() == 0 )
		MRatio=1;
	else
		MRatio=0.5*(cb->GetMetalUsage()/cb->GetMetalIncome());
	float ERatio;
	if( cb->GetEnergyIncome() == 0 || cb->GetEnergyUsage() == 0 )
		ERatio=1;
	else
		ERatio=0.5*(cb->GetEnergyUsage()/cb->GetEnergyIncome());
	float MetalRate=cb->GetMetalIncome()-cb->GetMetalUsage()+0.75*PM->MetalProduction+0.1*PM->MetalProductionEtoM+0.75*ConMetalRate;
	float EnergyRate=cb->GetEnergyIncome()-cb->GetEnergyUsage()-fOnOffPower-fCloakPower-fWeaponPower+0.75*ConEnergyRate;

	// This is a very poor attempt to correct the values reported by callback, however having this makes a big difference when the commander is building it's first few units
	if( ConDrainFrameDebug+15 >= cb->GetCurrentFrame() )
	{
		MetalRate+=ConDrainMetalDebug;
		EnergyRate+=ConDrainEnergyDebug;
/*
		*l<<"\n\n CF:"<<cb->GetCurrentFrame()<<" DF:"<<ConDrainFrameDebug<<" Dif:"<<cb->GetCurrentFrame()- ConDrainFrameDebug;
		*l<<"\n Metal/Energy Usage: "<<cb->GetMetalUsage()<<"/"<<cb->GetEnergyUsage();
		*l<<"\n Debugging Metal/Energy Rates: "<<ConDrainMetalDebug<<"/"<<ConDrainEnergyDebug;
*/
	}
	else
	{
		ConDrainMetalDebug=0;
		ConDrainEnergyDebug=0;
	}
	Command c;
	bool HaveOrders;
	if( U->BuildQ != 0 )
	{
		if( int(U->BuildQ->creationID.size())>0 && cb->GetUnitHealth(U->BuildQ->creationID.front()) <= 0 )
			U->BuildQ->creationID.pop_front();
		if( int(U->BuildQ->creationID.size()) == 0 )
			U->BuildQ->tryCount++;

		if( !U->BuildQ->IsValid(iFrame) || U->BuildQ->tryCount > 4 )
		{
			BQRemove(U->BuildQ->index);
			HaveOrders=false;
		}
		else
		{
			c.id=-U->BuildQ->creationUDID;
			HaveOrders=true;
		}
	}
	else
	{
		HaveOrders=false;
	}
	if( !HaveOrders && int(U->udr->BuildOptions.size()) > 0 )
	{
		for(int iBQ=0; iBQ<BQSize[0]; iBQ++) // Checking old build orders
		{
			if( !BQ[iBQ]->IsValid(iFrame) || 
				(iFrame > BQ[iBQ]->deletionFrame+9000 && // 9000f = 5mins
				 int(BQ[iBQ]->creationID.size()) > 0 &&
				 cb->GetUnitHealth(BQ[iBQ]->creationID.front()) < 0.02*cb->GetUnitMaxHealth(BQ[iBQ]->creationID.front()) ) )
			{
				if( BQ[iBQ]->builderID > -1 )
				{
					BQ[iBQ]->builderUI->commandTimeOut=1;
				}
				BQRemove(iBQ--);
			}
		}
		if(    BQSize[0] < 30 && BQSize[0] < int(UBuilder.size())+1 && UDR->BLActive > 0 )
		{
			G->ValidateUnitList(&UBuilder);
			BP->CheckBlockedRList();
		}
		while( BQSize[0] < 30 && BQSize[0] < int(UBuilder.size())+1 && UDR->BLActive > 0 )
		{
//			*l<<"\n\n NBO";
//			if( Prerequisite != 0 ) *l<<"(Prerequisite: "<<Prerequisite->creationUD->ud->humanName<<")";

			float fBestV=-1;	// Value
			int iBestI=-1;		// Index
			for(int i=0; i<UDR->BLSize; i++ )
			{
				UDR->BL[i]->UDefActiveTemp = UDR->BL[i]->UDefActive;
			}
			for(int i=0; i<UDR->BLActive; i++ )
			{
				// If a prerequisite is being built or this list is unimportant, then make sure this list can build something that already has a prerequisite
				if( Prerequisite != 0 || UDR->BL[i]->minUnits == 0 )
				{
					for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
					{
						if( !UDR->BL[i]->UDef[iud]->RUD->HasPrerequisite )
						{
							UDR->BL[i]->UDefActiveTemp--;
							UDR->BL[i]->UDefSwitch(iud,UDR->BL[i]->UDefActiveTemp);
						}
					}
				}
				// make sure the first few extractors are built at the closest metal spots available by restricting the wrong types
				if( UDR->BL[i] == UDR->BLMetalL )
				{
					if( UDR->BL[i]->unitsActive < UDR->BL[i]->minUnits )
					{
						float3 Pos = cb->GetUnitPos(unit);
						set<int> best;
						sResourceSpot* RSbest;
						for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
						{
							int mapbody=G->GetCurrentMapBody(UDR->BL[i]->UDef[iud]->RUD->ud,Pos);
							sResourceSpot* RS = BP->FindResourceSpot(Pos,UDR->BL[i]->UDef[iud]->RUD->ud,mapbody);
							if( RS != 0 )
							{
								if( int(best.size()) == 0 || RS == RSbest )
								{
									best.insert(iud);
									RSbest = RS;
								}
								else if( Pos.distance2D(RS->location) < Pos.distance2D(RSbest->location) )
								{
									best.clear();
									best.insert(iud);
									RSbest = RS;
								}
							}
						}
						if( int(best.size()) > 0 )
						{
							for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
							{
								if( best.find(iud) == best.end() )
								{
									UDR->BL[i]->UDefActiveTemp--;
									UDR->BL[i]->UDefSwitch(iud,UDR->BL[i]->UDefActiveTemp);
								}
							}
						}
					}
				}
				else if( int(G->UImmobileWater.size()) == 0 ) // restrict the building of most water units until 1)something important(extractor) has been built in it or 2) the active builder is close to the water
				{
					float3 pos = cb->GetUnitPos(unit);
					G->CorrectPosition(&pos);
					MapSector *MS = &G->TM->Sector[G->TM->GetSector(pos)];
					if( MS->Land && (MS->AltSector == 0 || MS->Pos.distance2D(MS->AltSector->Pos) > 750.0f ) )
					{
						for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
						{
							sRAIUnitDef *udr = UDR->BL[i]->UDef[iud]->RUD;
							if( udr->ud->minWaterDepth >= 0 || (udr->WeaponGuardRange > 0 && udr->WeaponSeaEff.BestRange > 0) )
							{
								UDR->BL[i]->UDefActiveTemp--;
								UDR->BL[i]->UDefSwitch(iud,UDR->BL[i]->UDefActiveTemp);
							}
						}
					}
				}
//				*l<<"\n"<<UDR->BL[i]->Name<<" BuildList "<<UDR->BL[i]->unitsActive<<"/"<<UDR->BL[i]->priority<<": Min="<<UDR->BL[i]->minUnits<<" UDefActiveTemp="<<UDR->BL[i]->UDefActiveTemp;
				if( UDR->BL[i]->UDefActiveTemp == 0 ) {}
				else if( UDR->BL[i]->unitsActive < UDR->BL[i]->minUnits && (fBestV<0 || UDR->BL[iBestI]->minUnits == 0 || float(UDR->BL[i]->unitsActive)/float(UDR->BL[i]->minUnits) < fBestV) )
				{
					iBestI = i;
					fBestV = float(UDR->BL[iBestI]->unitsActive)/float(UDR->BL[iBestI]->minUnits);
				}
				else if( fBestV<0 || (UDR->BL[i]->priority > 0 && UDR->BL[iBestI]->unitsActive >= UDR->BL[iBestI]->minUnits && float(1+UDR->BL[i]->unitsActive)/float(UDR->BL[i]->priority) < fBestV ) )
				{
					iBestI = i;
					fBestV = float(1+UDR->BL[i]->unitsActive)/float(UDR->BL[i]->priority);
				}
			}
			if( iBestI == -1 ) // ? hopefully won't happen
			{
				*l<<"\n\nERROR: No BuildList Choice Available.";
				cb->SendTextMsg("ERROR: No BuildList Choice Available.",5);
				break;
			}

			sRAIBuildList *BL = UDR->BL[iBestI];
			int iBuildType=1;

			if( BL == UDR->BLEnergy || BL == UDR->BLEnergyL )
				iBuildType=2;
			else if( BL == UDR->BLMetal || BL == UDR->BLMetalL )
				iBuildType=3;
			else if( BL == UDR->BLBuilder )
				iBuildType=4;

			if( BL->unitsActive < BL->minUnits )
			{
				for( int iud=BL->UDefActiveTemp-1; iud>=0; iud-- )
				{
					if( !BL->UDef[iud]->RUD->HasPrerequisite )
					{
						BL->UDefActiveTemp--;
						BL->UDefSwitch(iud,BL->UDefActiveTemp);
					}
				}
				if( BL->UDefActiveTemp == 0 )
					BL->UDefActiveTemp = BL->UDefActive;
			}
			iBestI = rand()%BL->UDefActiveTemp;
			sRAIUnitDef *udr = BL->UDef[iBestI]->RUD;
//			*l<<"\n Option Seleted: "<<udr->ud->humanName;

			if( !udr->HasPrerequisite && BQSize[4] < 5 )
			{
				int oldID=udr->ud->id;
				udr=&UDR->UDR.find(udr->GetPrerequisite())->second;
				if( udr->ud->id != oldID )
				{
					iBuildType=7;
//					*l<<"\n Prerequisite Seleted: "<<udr->ud->humanName;
				}
			}
			float fBuildTime=udr->ud->buildTime/U->ud->buildSpeed;
			float EnergyDemand = 0;
			float MetalDemand = 0;
			float ConstructerDemand=0;
			float EnergyStorageDemand=0;
			float MetalStorageDemand=0;

			if( BL->unitsActive >= BL->minUnits && BL != UDR->BLEnergy && BL != UDR->BLMetal && BL != UDR->BLMetalL )
			{
				if( (UDR->BLEnergyL->UDefActiveTemp > 0 || UDR->BLEnergy->UDefActiveTemp > 0) && BQSize[2] < 5 && BQSize[2] < 0.4*BQSize[0] )
					EnergyDemand = -(fBuildTime*(EnergyRate+udr->EnergyDifference)+cb->GetEnergy()-0.25*ConEnergyLost-udr->ud->energyCost)*ERatio/UDR->EnergyToMetalRatio;

				if( (UDR->BLMetalL->UDefActiveTemp > 0 || UDR->BLMetal->UDefActiveTemp > 0) && BQSize[3] < 5 && BQSize[3] < 0.4*BQSize[0] && !( EnergyDemand>0 && udr->HighEnergyDemand ) )
				{
					if( UDR->BLMetalL->UDefActiveTemp == 0 )
						MetalRate=cb->GetMetalIncome()-cb->GetMetalUsage()+PM->MetalProduction+PM->MetalProductionEtoM+ConMetalRate;

					MetalDemand = -(fBuildTime*(MetalRate+udr->MetalDifference)+cb->GetMetal()-0.25*ConMetalLost-udr->ud->metalCost)*MRatio;
					if( MetalDemand <= 0 && UDR->BLMetalL->UDefActiveTemp > 0 && BQSize[3] == 0 )
						MetalDemand=2;
				}

				if( UDR->BLBuilder->UDefActiveTemp > 0 && BQSize[4] < 2 && BQSize[4] < 0.3*BQSize[0] )
				{
					if( EnergyRate > 0 && MetalRate > 0 && 30.0*EnergyRate+cb->GetEnergy()-(0.6+0.1*BQSize[4])*cb->GetEnergyStorage() > 0 && 30.0*MetalRate+cb->GetMetal()-(0.6+0.1*BQSize[4])*cb->GetMetalStorage() > 0 )
						ConstructerDemand=1;
				}

				if( UDR->BLEnergyStorage->UDefActiveTemp > 0 && BQSize[5] < 1 )
					EnergyStorageDemand=-(cb->GetEnergyStorage()+ConEnergyStorage-0.20*udr->ud->energyCost);

				if( UDR->BLMetalStorage->UDefActiveTemp > 0 && BQSize[6] < 1 )
					MetalStorageDemand=-(cb->GetMetalStorage()+ConMetalStorage-0.20*udr->ud->metalCost);
			}
/*
*l<<"\n CB";
*l<<"\n   Metal  ="<<cb->GetMetal();
*l<<"   Metal Income  ="<<cb->GetMetalIncome();
*l<<"   Metal Usage  ="<<cb->GetMetalUsage();
*l<<"   Metal Storage  ="<<cb->GetMetalStorage();
*l<<"\n   Energy ="<<cb->GetEnergy();
*l<<"   Energy Income ="<<cb->GetEnergyIncome();
*l<<"   Energy Usage ="<<cb->GetEnergyUsage();
*l<<"   Energy Storage ="<<cb->GetEnergyStorage();
*l<<"\n Con Energy Lost ="<<ConEnergyLost;
*l<<"\n Metal Energy Lost ="<<ConMetalLost;
*l<<"\n MetalProduction="<<PM->MetalProduction;
*l<<"\n MetalProductionEtoM="<<PM->MetalProductionEtoM;
*l<<"\n fOnOffPower="<<fOnOffPower;
*l<<"\n fWeaponPower="<<fWeaponPower;
*l<<"\n fCloakPower="<<fCloakPower;
*l<<"\n Build Time="<<fBuildTime;
*l<<"\n Name="<<udr->ud->humanName;
*l<<"\n   metalCost="<<udr->ud->metalCost;
*l<<"\n   energyCost="<<udr->ud->energyCost;
*l<<"\n   metalUpkeep="<<udr->ud->metalUpkeep;
*l<<"\n   metalMake="<<udr->ud->metalMake;
*l<<"\n   energyUpkeep="<<udr->ud->energyUpkeep;
*l<<"\n   energyMake="<<udr->ud->energyMake;
*l<<"\n   HighEnergyDemand="<<udr->HighEnergyDemand;
*l<<"\n   extractsMetal="<<udr->ud->extractsMetal;
*l<<"\n BQSize"; for( int i=0; i<8; i++ ) *l<<" ["<<i<<"]="<<BQSize[i];
*l<<"\n EnergyDemand="<<EnergyDemand;
*l<<"\n MetalDemand="<<MetalDemand;
*l<<"\n EnergyStorageDemand="<<EnergyStorageDemand;
*l<<"\n MetalStorageDemand="<<MetalStorageDemand;
*l<<"\n ConstructerDemand="<<ConstructerDemand;
*/
			float fHigh=0;
			if( EnergyDemand > fHigh )
			{
				fHigh=EnergyDemand;
				iBuildType=2;
				if( UDR->BLEnergyL->UDefActiveTemp > 0 )
					BL=UDR->BLEnergyL;
				else
					BL=UDR->BLEnergy;
			}
			if( MetalDemand > fHigh )
			{
				fHigh=MetalDemand;
				iBuildType=3;

				if( UDR->BLMetalL->UDefActiveTemp > 0 )
					BL=UDR->BLMetalL;
				else
				{
					BL=UDR->BLMetal;
				}
			}
			if( ConstructerDemand > fHigh )
			{
				fHigh=ConstructerDemand;
				iBuildType=4;
				BL=UDR->BLBuilder;
			}
			if( EnergyStorageDemand > fHigh )
			{
				fHigh=EnergyStorageDemand;
				iBuildType=5;
				BL=UDR->BLEnergyStorage;
			}
			if( MetalStorageDemand > fHigh )
			{
				fHigh=MetalStorageDemand;
				iBuildType=6;
				BL=UDR->BLMetalStorage;
			}
			if( fHigh > 0 )
			{
//				*l<<"\n "+BL->Name+"(a="<<BL->UDefActiveTemp<<") Build Type="<<iBuildType;
				int iIndex=rand()%BL->UDefActiveTemp;
//				*l<<" BL Index="<<iIndex;
				udr = BL->UDef[iIndex]->RUD;
//				*l<<"\n  2-6 Demand Alternative Selected: "<<udr->ud->humanName;
				if( !udr->HasPrerequisite && BQSize[4] < 5 && BQSize[4] <= 0.5*BQSize[0] )
				{
					int oldID=udr->ud->id;
					udr=&UDR->UDR.find(udr->GetPrerequisite())->second;
					if( udr->ud->id != oldID )
					{
						iBuildType=7;
//						*l<<"\n Prerequisite Seleted: "<<udr->ud->humanName;
					}
				}
			}
			bool bPrerequisiteOptionsChecked=false;
			int iBest=-1;
			int Count=0;
			for(map<int,sRAIUnitDef*>::iterator iP=udr->PrerequisiteOptions.begin(); iP!=udr->PrerequisiteOptions.end() && !bPrerequisiteOptionsChecked; iP++)
			{
				for(set<int>::iterator iU=iP->second->UnitsActive.begin(); iU!=iP->second->UnitsActive.end() && !bPrerequisiteOptionsChecked; iU++ )
				{
					if( UBuilder.find(*iU) != UBuilder.end() && UBuilder.find(*iU)->second->BuildQ == 0 && *iU != unit )
					{
						if( int(cb->GetCurrentUnitCommands(*iU)->size()) == 0 || cb->GetCurrentUnitCommands(*iU)->front().id == CMD_WAIT )
						{
							UBuilder.find(*iU)->second->commandTimeOut=cb->GetCurrentFrame();
						}
						bPrerequisiteOptionsChecked=true;
					}
				}
			}
			if( !bPrerequisiteOptionsChecked && BQSize[4] < 3 && BQSize[4] <= 0.3*BQSize[0] && Prerequisite == 0 && rand()%5 == 0 )
			{
				int oldID=udr->ud->id;
				udr=&UDR->UDR.find(udr->GetPrerequisiteNewBuilder())->second;
				if( udr->ud->id != oldID )
				{
					if( udr->UnitConstructs == 0 &&
						((UDR->BLMetalL->UDefSize == 0 && UDR->BLMetal->UDefSize == 0) || 0.25*cb->GetMetalIncome() > udr->ud->metalCost/UDR->AverageConstructSpeed ) &&
						((UDR->BLEnergyL->UDefSize == 0 && UDR->BLEnergy->UDefSize == 0) || 0.25*cb->GetEnergyIncome() > udr->ud->energyCost/UDR->AverageConstructSpeed) )
					{
//						*l<<"\n New Builder Selected: "<<udr->ud->humanName;
						iBuildType=4;
					}
					else
						udr=&UDR->UDR.find(oldID)->second;
				}
			}
			BQAdd(udr,BL,iBuildType);
			if( Prerequisite == 0 && iBuildType == 7 && int(udr->UnitsActive.size()) == 0 )
			{
				Prerequisite = BQ[BQSize[0]-1];
			}
			MetalRate=cb->GetMetalIncome()-cb->GetMetalUsage()+0.75*PM->MetalProduction+0.1*PM->MetalProductionEtoM+0.75*ConMetalRate;
			EnergyRate=cb->GetEnergyIncome()-cb->GetEnergyUsage()-fOnOffPower-fCloakPower-fWeaponPower+0.75*ConEnergyRate;
		}
		// Search existing build orders
		float Demand[8]; // index = type, higher value mean that type should be built first
		Demand[1] = 0;
		Demand[2] = 3;
		Demand[3] = 3;
		if( cb->GetEnergy()*(cb->GetEnergyIncome()/cb->GetEnergyUsage()) < cb->GetMetal()*(cb->GetMetalIncome()/cb->GetMetalUsage()) )
			Demand[2]++;
		else
			Demand[3]++;
		Demand[4] = 2;
		if( MetalIsFavorable(0.75f) && EnergyIsFavorable(0.75f) && int(G->Units.size()) >= 5 )
			Demand[4]+=3;
		Demand[5] = 1;
		Demand[6] = 1;
		Demand[7] = 6;
		int iBest=-1;
		for(int iBQ=0; iBQ<BQSize[0]; iBQ++)
		{
			if( BQ[iBQ]->builderID == -1 && U->udr->BuildOptions.find(BQ[iBQ]->creationUDID) != U->udr->BuildOptions.end() )
			{
				if( iBest == -1 || BQ[iBQ]==Prerequisite || (BQ[iBest]!=Prerequisite && (Demand[BQ[iBQ]->type] > Demand[BQ[iBest]->type] || (BQ[iBest]->type==BQ[iBQ]->type && BQ[iBest]->creationUD->HighEnergyDemand && !BQ[iBQ]->creationUD->HighEnergyDemand) ) ) )
				{
					iBest=iBQ;
				}
			}
		}
		if( iBest >= 0 )
		{
			BQAssignBuilder(iBest,unit,U);
			HaveOrders=true;
			c.id=-U->BuildQ->creationUDID;
		}
	}
/*
	for(int iBQ=0; iBQ<BQSize[0]; iBQ++) // Build Quarry Debug
	{
		*l<<"\nBQ #"<<iBQ+1;
		*l<<":  BID="<<BQ[iBQ]->builderID;
		if( int(BQ[iBQ]->creationID.size()) > 0 )
		{
			*l<<" CID="<<BQ[iBQ]->creationID.front();
		}
		*l<<" CUDID="<<BQ[iBQ]->creationUDID;
	}
*/
	if( int(U->URepair.size()) > 0 )
	{
		while( int(U->URepair.size()) > 0 && ( cb->GetUnitDef(U->URepair.begin()->first) == 0 || cb->GetUnitHealth(U->URepair.begin()->first) == cb->GetUnitMaxHealth(U->URepair.begin()->first) ) )
			U->URepair.erase(U->URepair.begin()->first);

		if( int(U->URepair.size()) > 0 )
		{
			c.id = CMD_REPAIR;
			c.params.push_back(U->URepair.begin()->first);
			cb->GiveOrder(unit, &c);
			return;
		}
	}
	if( !HaveOrders && U->ud->canResurrect && U->ud->speed > 0 )
	{
		UpdateKnownFeatures(unit,U);
		if( int(ResDebris.size()) > 0 )
		{
			int iBest=-1;
			float3 Pos = cb->GetUnitPos(unit);
			float3 debPos;
			for( map<int,float3>::iterator iR=ResDebris.begin(); iR!=ResDebris.end(); iR++ )
			{
				if( (iBest == -1 || Pos.distance(debPos) > Pos.distance(iR->second)) && G->TM->CanMoveToPos(U->mapBody,iR->second) )
				{
					iBest = iR->first;
					debPos= ResDebris.find(iBest)->second;
				}
			}
			if( iBest != -1 )
			{
//				*l<<"\n "+U->ud->humanName+" is resurrecting.";
				c.id=CMD_RESURRECT;
//				c.params.push_back(iBest);
				c.params.push_back(debPos.x); // ! Work Around   Spring Version: v0.74b3
				c.params.push_back(debPos.y);
				c.params.push_back(debPos.z);
				c.params.push_back(25.0f);
				cb->GiveOrder(unit, &c);
				float fTime=Pos.distance(debPos)/(U->ud->speed/3.0);
				U->commandTimeOut=cb->GetCurrentFrame()+1500+30*int(fTime);
				ResDebris.erase(iBest);
				return;
			}
		}
	}
	if( !HaveOrders && U->ud->canReclaim && U->ud->speed > 0 && !U->ud->isCommander )
	{
		while( int(Decomission.size()) > 0 && cb->GetUnitDef(*Decomission.begin()) == 0 )
		{
			Decomission.erase(*Decomission.begin());
		}
		if( int(Decomission.size()) > 0 && G->TM->CanMoveToPos(U->mapBody,cb->GetUnitPos(*Decomission.begin())) )
		{
//			*l<<"\n "+U->ud->humanName+" is Reclaim/Decomissioning.";
			c.id=CMD_RECLAIM;
			c.params.push_back(*Decomission.begin());
			cb->GiveOrder(unit, &c);
			return;
		}
		if( int(FeatureDebris.size()) > 0 && G->TM->CanMoveToPos(U->mapBody,FeatureDebris.begin()->second) )
		{
			map<int,float3>::iterator i=FeatureDebris.begin();
			float3 fPos = i->second;

			int *F = new int[1];
			if( cb->GetFeatures(F,1,i->second,20.0) == 1 )
			{
//				*l<<"\n "+U->ud->humanName+" is Reclaim/Clearing.";
				c.id=CMD_RECLAIM;
				c.params.push_back(fPos.x); // ! Work Around   Spring Version: v0.74b3
				c.params.push_back(fPos.y);
				c.params.push_back(fPos.z);
				c.params.push_back(80.0);
				cb->GiveOrder(unit, &c);

				float fTime=cb->GetUnitPos(unit).distance(fPos)/(U->ud->speed/3.0);
				U->commandTimeOut=cb->GetCurrentFrame()+150+30*int(fTime);
				FeatureDebris.erase(i->first);
				delete [] F;
				return;
			}
			delete [] F;
			FeatureDebris.erase(i->first);
		}
		if( cb->GetMetal() < 0.15*cb->GetMetalStorage() || !MetalIsFavorable(0.5f) )
		{
			UpdateKnownFeatures(unit,U);
			if( int(MetalDebris.size()) > 0 )
			{
				int iBest=-1;
				float3 Pos = cb->GetUnitPos(unit);
				float3 debPos;
				for( map<int,float3>::iterator iM=MetalDebris.begin(); iM!=MetalDebris.end(); iM++ )
				{
					if( (iBest == -1 || Pos.distance(debPos) > Pos.distance(iM->second)) && G->TM->CanMoveToPos(U->mapBody,iM->second) )
					{
						iBest = iM->first;
						debPos = iM->second;
					}
				}
				if( iBest != -1 )
				{
//					*l<<"\n "+U->ud->humanName+" is Reclaim/E Gathering.";
					c.id=CMD_RECLAIM;
//					c.params.push_back(iBest);
					c.params.push_back(debPos.x); // ! Work Around   Spring Version: v0.74b3
					c.params.push_back(debPos.y);
					c.params.push_back(debPos.z);
					c.params.push_back(25.0f);
					cb->GiveOrder(unit, &c);

					float fTime=Pos.distance(debPos)/(U->ud->speed/3.0);
					U->commandTimeOut=cb->GetCurrentFrame()+1500+30*int(fTime);
					MetalDebris.erase(iBest);
					return;
				}
			}
		}
		if( cb->GetEnergy() < 0.15*cb->GetEnergyStorage() || !EnergyIsFavorable(0.5f) )
		{
			UpdateKnownFeatures(unit,U);
			if( int(EnergyDebris.size()) > 0 )
			{
				int iBest=-1;
				float3 Pos = cb->GetUnitPos(unit);
				float3 debPos;
				for( map<int,float3>::iterator iE=EnergyDebris.begin(); iE!=EnergyDebris.end(); iE++ )
				{
					if( (iBest == -1 || Pos.distance(debPos) > Pos.distance(iE->second)) && G->TM->CanMoveToPos(U->mapBody,iE->second) )
					{
						iBest = iE->first;
						debPos = iE->second;
					}
				}
				if( iBest != -1 )
				{
//					*l<<"\n "+U->ud->humanName+" is Reclaim/M Gathering.";
					c.id=CMD_RECLAIM;
					c.params.push_back(debPos.x); // ! Work Around   Spring Version: v0.74b3
					c.params.push_back(debPos.y);
					c.params.push_back(debPos.z);
					c.params.push_back(25.0f);
					cb->GiveOrder(unit, &c);
					float fTime=Pos.distance(debPos)/(U->ud->speed/3.0);
					U->commandTimeOut=cb->GetCurrentFrame()+1500+30*int(fTime);
					EnergyDebris.erase(iBest);
					return;
				}
			}
		}
	}
	// If this unit has nothing else to do, and resources are favorable, continuously build military units
	if( !HaveOrders && BQSize[0]<40 && MetalIsFavorable(0.35f,0.94f) && EnergyIsFavorable(0.70f,0.94f) )
	{
		vector<int> build;
		for( map<int,sRAIUnitDef*>::iterator iB=U->udr->BuildOptions.begin(); iB!=U->udr->BuildOptions.end(); iB++ )
		{
			if( iB->second->CanBeBuilt )
			{
				for( int iBL=0; iBL<iB->second->ListSize; iBL++ )
				{
					if( iB->second->List[iBL]->RBL->Name == "Mobile Anti-Land/Air" ||
						(iB->second->List[iBL]->RBL->Name == "Mobile Anti-Naval" && iB->second->List[iBL]->RBL->priority > 0 ) )
					{
						build.push_back(iB->first);
						break;
					}
				}
			}
		}

		if( int(build.size()) > 0 )
		{
			int iRan=rand()%int(build.size());
				BQAdd(&UDR->UDR.find(U->udr->BuildOptions.find(build.at(iRan))->first)->second,0,1);
			BQAssignBuilder(BQSize[0]-1,unit,U);
			c.id=-U->BuildQ->creationUDID;
			HaveOrders=true;
		}
	}
	if( HaveOrders && c.id < 0 ) // Has Build Orders
	{
		if( int(U->BuildQ->creationID.size()) > 0 )
		{
//			*l<<"\n "+U->ud->humanName+" is Repair/Building";
			c.id = CMD_REPAIR;
			c.params.push_back(U->BuildQ->creationID.front());
			cb->GiveOrder(unit, &c);
			return;
		}

		const UnitDef *bd=U->BuildQ->creationUD->ud;
		if( U->BuildQ->RS != 0 )
		{
			if( U->BuildQ->RS->unitID > -1 && U->BuildQ->RS->enemy )
			{
				if( U->ud->canCapture )
				{
//					*l<<"\n "+U->ud->humanName+" is Capture/Building.";
					c.id = CMD_CAPTURE;
					c.params.push_back(U->BuildQ->RS->unitID);
					cb->GiveOrder(unit, &c);
				}
				else if( U->ud->canReclaim )
				{
//					*l<<"\n "+U->ud->humanName+" is Reclaim/Building.";
					c.id = CMD_RECLAIM;
					c.params.push_back(U->BuildQ->RS->unitID);
					cb->GiveOrder(unit, &c);
				}
				else
				{
					*l<<"\nWARNING: Can Not Reclaim/Capture Enemy";
					BQAssignBuilder(U->BuildQ->index,-1,0); // unassign builder
				}
				return;
			}
			if( U->BuildQ->RS->BuildOptions.find(bd->id)->second.RBRanked )
			{
				BQRemove(U->BuildQ->index);
				return;
			}
		}
		float3 fBuildPos = BP->FindBuildPosition(U->BuildQ);
		if( !cb->CanBuildAt(bd,fBuildPos) )
		{
			if( U->ud->speed==0.0 && bd->speed>0.0 ) { fBuildPos=cb->GetUnitPos(unit); } // Fix for gundam v1.1 mod(CanBuildAt almost always fails).  Unfortunately this condition assumes that all immobile constructers build mobile units at their center
			else if( U->BuildQ->RS != 0 && U->BuildQ->RS->unitID > -1 && cb->GetUnitTeam(U->BuildQ->RS->builderID) == cb->GetUnitTeam(U->BuildQ->RS->unitID) && U->ud->canReclaim )
			{
//				*l<<"\n "+U->ud->humanName+" is Reclaim/Upgrade/Building.";
				c.id = CMD_RECLAIM;
				c.params.push_back(U->BuildQ->RS->unitID);
				cb->GiveOrder(unit, &c);
				return;
			}
			else
			{
				if( !U->BuildQ->creationUD->CanBeBuilt ) // usually this means an intended limited resource is no longer available
				{
					BQRemove(U->BuildQ->index);
					U->commandTimeOut=1;
				}
				else
				{
					*l<<"\nWARNING: ("<<unit<<")"<<U->ud->humanName<<" can not build a '"<<bd->humanName<<"' at selected position: x="<<fBuildPos.x<<" y="<<fBuildPos.y<<" z="<<fBuildPos.z;
					BQAssignBuilder(U->BuildQ->index,-1,0); // unassign builder
				}
				return;
			}
		}

//		*l<<"\n "+U->ud->humanName+" is Building a "+bd->humanName+"("<<-c.id<<")";
		c.params.push_back(fBuildPos.x);
		c.params.push_back(fBuildPos.y);
		c.params.push_back(fBuildPos.z);

		if( int(U->BuildQ->creationUD->BuildOptions.size()) > 0 )
		{
			float3 fBuildPosH = fBuildPos;
			fBuildPosH.z+=48.0f;
//			fBuildPosH.y=cb->GetElevation(fBuildPosH.x,fBuildPosH.z);
			float3 fBuildPosL = fBuildPos;
			fBuildPosL.z-=48.0f;
//			fBuildPosL.y=cb->GetElevation(fBuildPosL.x,fBuildPosL.z);
			if( fBuildPos.z/8 >= cb->GetMapHeight()-5-(U->BuildQ->creationUD->ud->ysize/2) ||
				(cb->CanBuildAt(bd,fBuildPosL) && !cb->CanBuildAt(bd,fBuildPosH) ) )
				c.params.push_back(2);
		}
		cb->GiveOrder(unit, &c);
		return;
	}
	if( !HaveOrders && U->ud->canAssist ) // Assist Build
	{
		if( U->ud->speed > 0 )
		{
			int BestIndex=-1;
			float3 fPos=cb->GetUnitPos(unit);
			float BestDis;
			for(int iBQ=0; iBQ<BQSize[0]; iBQ++)
			{
				if( BQ[iBQ]->builderID >= 0 && G->TM->CanMoveToPos(U->mapBody,cb->GetUnitPos(BQ[iBQ]->builderID)) )
				{	
					float Dis=fPos.distance(cb->GetUnitPos(BQ[iBQ]->builderID));
					if( BestIndex == -1 || Dis < BestDis )
					{
						BestIndex=iBQ;
						BestDis=Dis;
					}
				}
			}

			if( BestIndex > 0 )
			{
//				*l<<"\n "+U->ud->humanName+" is Assisting "<<BQ[BestIndex]->builderUI->ud->humanName;
				c.id = CMD_GUARD;
				c.params.push_back(BQ[BestIndex]->builderID);
				//c.timeOut=cb->GetCurrentFrame()+360;
				U->commandTimeOut=cb->GetCurrentFrame()+600;
				cb->GiveOrder(unit, &c);
				return;
			}
		}
		else
		{
			for( map<int,UnitInfo*>::iterator i = U->UAssist.begin(); i != U->UAssist.end(); i++ )
			{
				if( i->second->BuildQ != 0 )
				{
					c.id = CMD_GUARD;
					c.params.push_back(i->first);
					U->commandTimeOut=cb->GetCurrentFrame()+600;
					cb->GiveOrder(unit, &c);
					return;
				}
			}
		}
	}
//	*l<<"\n "+U->ud->humanName+" is Waiting";
	U->commandTimeOut=cb->GetCurrentFrame()+600;

	if( U->ud->speed == 0.0 ) // Work Around  Spring Version: v0.74.b1
	{
		return;
	}

	c.id = CMD_WAIT;
	//c.timeOut=cb->GetCurrentFrame()+600;
	cb->GiveOrder(unit, &c);
}
/*
void cBuilder::UBuilderDamaged(const int& unit, int attacker, float3 dir)
{
}
*/
bool cBuilder::UBuilderMoveFailed(const int& unit, UnitInfo *U)
{
	int iF;
	int *F = new int[10];
	int FSize = cb->GetFeatures(F,10,cb->GetUnitPos(unit),90);
	for( iF=0; iF < FSize; iF++ )
		if( cb->GetFeatureDef(F[iF])->destructable )
			break;
	if( iF < FSize )
	{
		if( U->ud->canReclaim )
		{
//			*l<<" "+UBuilder.find(unit)->second.ud->name+" is Reclaiming ";
			Command c;
			c.id=CMD_RECLAIM;
			float3 fPos = cb->GetUnitPos(unit);
			c.params.push_back(fPos.x); // ! Work Around   Spring Version: v0.74b3
			c.params.push_back(fPos.y);
			c.params.push_back(fPos.z);
			c.params.push_back(90.0);
			//c.params.push_back(F[0]);

			U->commandTimeOut=cb->GetCurrentFrame()+1200;
			cb->GiveOrder(unit, &c);

			fPos = cb->GetFeaturePos(F[iF]);
			delete [] F;
			return true;
		}
		else
		{
			if( FeatureDebris.find(F[iF]) == FeatureDebris.end() ) // Add to reclaim list
			{
				FeatureDebris.insert(ifPair(F[iF],cb->GetUnitPos(unit)));
			}
		}
	}
	delete [] F;
	return false;
}

void cBuilder::HandleEvent(const IGlobalAI::PlayerCommandEvent *pce)
{
	for( vector<int>::const_iterator i=pce->units.begin(); i!=pce->units.end(); i++ )
	{
		if( UBuilder.find(*i) != UBuilder.end() )
		{
			UnitInfo* U = UBuilder.find(*i)->second;
			if( U->BuildQ != 0 )
			{
				if( pce->command.options == RIGHT_MOUSE_KEY && pce->command.id < 0 )
				{
					if( int(U->BuildQ->creationID.size())>0 && U->BuildQ->creationUDID == -pce->command.id )
					{
						G->UnitDestroyed(U->BuildQ->creationID.front(),-1);
					}
				}
				else if( pce->command.options != SHIFT_KEY )
					BQAssignBuilder(U->BuildQ->index,-1,0);
			}
		}
	}
}

void cBuilder::UpdateUDRCost()
{
//	if( MCostUpdate < 0.2*MCostLimit && MCostUpdate > -0.2*MCostLimit &&
//		ECostUpdate < 0.2*ECostLimit && ECostUpdate > -0.2*ECostLimit )
//		return;
	MCostLimit = cb->GetMetalIncome()+PM->MetalProduction+0.15*PM->MetalProductionEtoM;
	ECostLimit = cb->GetEnergyIncome();
	if( MCostLimit > 110.0 || (UDR->BLMetal->UDefSize == 0 && UDR->BLMetalL->UDefSize == 0) )
		MCostLimit=9.9e8;
	if( ECostLimit > 110.0*UDR->EnergyToMetalRatio || (UDR->BLEnergy->UDefSize == 0 && UDR->BLEnergyL->UDefSize == 0) )
		ECostLimit=9.9e8;

//	*l<<"\n Rechecking Unit Costs and Determining Active BL Options ...";
	for( map<int,sRAIUnitDef>::iterator i=UDR->UDR.begin(); i!=UDR->UDR.end(); i++ )
	{
		sRAIUnitDef *udr = &i->second;
		if( udr->MetalPCost < MCostLimit && udr->EnergyPCost < ECostLimit )
		{
			if( udr->RBCost )
			{	// enabling
				udr->RBCost = false;
				udr->CheckBuildOptions();
			}
		}
		else if( udr->MetalPCost > 1.5*MCostLimit || udr->EnergyPCost > 1.5*ECostLimit )
		{
			if( !udr->RBCost )
			{	// disabling
				udr->RBCost = true;
				udr->CheckBuildOptions();
			}
		}
	}
	for(int iBL=0; iBL<UDR->BLSize; iBL++)
	{
		if( UDR->BL[iBL]->minUnits > 0 && UDR->BL[iBL]->UDefSize > 0 && UDR->BL[iBL]->UDefSize > UDR->BL[iBL]->UDefActive )
		{
//			*l<<"\n Determining Cheapest for '"<<UDR->BL[iBL]->Name<<"' BuildList ...";
			sRAIUnitDef *BestLandudr=0,*BestWaterudr=0; // NOTE: the same udr may be selected for both, may also already be enabled
			float BestLandCost,BestWaterCost;
			bool BestLandCanBuildConstructers,BestWaterCanBuildConstructers; // only used in determining the cheapest constructer

			for(int iU=0; iU<UDR->BL[iBL]->UDefSize; iU++)
			{
				sRAIUnitDef *udr=UDR->BL[iBL]->UDef[iU]->RUD;
				if( !udr->Disabled && udr->HasPrerequisite )
				{
					bool CanBuildConstructers = false; // only used in determining the cheapest constructer
					for( map<int,sRAIUnitDef*>::iterator iB=udr->BuildOptions.begin(); iB!=udr->BuildOptions.end(); iB++)
					{
						if( !iB->second->Disabled && int(iB->second->BuildOptions.size()) > 0 )
						{
							CanBuildConstructers = true;
							break;
						}
					}
					float Cost = udr->MetalPCost + udr->EnergyPCost*UDR->EnergyToMetalRatio;
					if( udr->ud->minWaterDepth < 0 )
					{
						if( BestLandudr == 0 ||
							(CanBuildConstructers && !BestLandCanBuildConstructers) ||
							(Cost < BestLandCost && (CanBuildConstructers || !BestLandCanBuildConstructers) ) )
						{
							BestLandudr = udr;
							BestLandCost = Cost;
							BestLandCanBuildConstructers = CanBuildConstructers;
						}
					}
					if( udr->ud->maxWaterDepth > -G->TM->MaxWaterDepth || udr->ud->floater )
					{
						if( BestWaterudr == 0 ||
							(CanBuildConstructers && !BestWaterCanBuildConstructers) ||
							(Cost < BestWaterCost && (CanBuildConstructers || !BestWaterCanBuildConstructers) ) )
						{
							BestWaterudr = udr;
							BestWaterCost = Cost;
							BestWaterCanBuildConstructers = CanBuildConstructers;
						}
					}
				}
			}
			if( BestLandudr != 0 && (BestWaterudr == 0 || BestLandCost < 3*BestWaterCost) )
			{
//				*l<<"\nBestLandudr="<<BestLandudr->ud->humanName;
				if( BestLandudr->RBCost )
				{
					BestLandudr->RBCost = false;
					BestLandudr->CheckBuildOptions();
				}
			}
			if( BestWaterudr != 0 && (BestLandudr == 0 || BestWaterCost < 3*BestLandCost) )
			{
//				*l<<"\nBestWaterudr="<<BestWaterudr->ud->humanName;
				if( BestWaterudr->RBCost )
				{
					BestWaterudr->RBCost = false;
					BestWaterudr->CheckBuildOptions();
				}
			}
		}
	}
	for(int iBQ=0; iBQ<BQSize[0]; iBQ++)
	{
		if( BQ[iBQ]->creationUD->RBCost )
		{
			*l<<"\n (Low Resources) Abandoning Construction: "<<BQ[iBQ]->creationUD->ud->humanName;
			BQRemove(iBQ--);
		}
	}
/*
	*l<<"\n\n Displaying Active Build List Options ...";
	for(int iBL=0; iBL<UDR->BLActive; iBL++ )
	{
		*l<<"\n  "<<UDR->BL[iBL]->Name<<" BuildList: ";
		for(int iBO=0; iBO<UDR->BL[iBL]->UDefActive; iBO++)
			*l<<" "+UDR->BL[iBL]->UDef[iBO]->RUD->ud->humanName+" ";
	}
	*l<<"\n\n";
*/
}

void cBuilder::UpdateKnownFeatures(const int& unit, UnitInfo *U)
{
/*
	*l<<"\nUpdate: M";
	set<int> deletion;
	for( map<int,FeatureDef*>::iterator iM=MetalDebris.begin(); iM!=MetalDebris.end(); iM++ )
	{
		if( cb->GetFeatureDef(iM->first) == 0 )
			deletion.insert(iM->first);
	}
	*l<<"s="<<deletion.size();
	for( set<int>::iterator iM=deletion.begin(); iM!=deletion.end(); iM++ )
	{
		MetalDebris.erase(*iM);
	}
	deletion.clear();
	*l<<"\nUpdate: E";
	for( map<int,FeatureDef*>::iterator iE=EnergyDebris.begin(); iE!=EnergyDebris.end(); iE++ )
	{
		if( cb->GetFeatureHealth(iE->first) == 0.0f )
			deletion.insert(iE->first);
	}
	*l<<"s="<<deletion.size();
	for( set<int>::iterator iE=deletion.begin(); iE!=deletion.end(); iE++ )
	{
		EnergyDebris.erase(*iE);
	}
*/
	int *F = new int[15];
	int FeatureSize = cb->GetFeatures(F,15,cb->GetUnitPos(unit),750);
	for( int iF=0; iF<FeatureSize; iF++ )
	{
		const FeatureDef* fd = cb->GetFeatureDef(F[iF]);
		if( fd->reclaimable )
		{
			if( fd->metal >= 40 )
			{
				if( MetalDebris.find(F[iF]) == MetalDebris.end() )
					MetalDebris.insert(ifPair(F[iF],cb->GetFeaturePos(F[iF])));
			}
			if( fd->energy >= 40 )
			{
				if( EnergyDebris.find(F[iF]) == EnergyDebris.end() )
					EnergyDebris.insert(ifPair(F[iF],cb->GetFeaturePos(F[iF])));
			}
			if( UDRResurrect.find(fd->myName) != UDRResurrect.end() )
			{
				if( ResDebris.find(F[iF]) == ResDebris.end() )
					ResDebris.insert(ifPair(F[iF],cb->GetFeaturePos(F[iF])));
			}
		}
	}
	delete [] F;
}

bool cBuilder::MetalIsFavorable(float storage,float production)
{
	if( UDR->BLMetalL->UDefSize == 0 && UDR->BLMetal->UDefSize == 0 )
		return true;
	if( cb->GetMetal() > storage*cb->GetMetalStorage() && cb->GetMetalIncome() > production*(cb->GetMetalUsage()-ConDrainMetalDebug) )
		return true;
	return false;
}

bool cBuilder::EnergyIsFavorable(float storage,float production)
{
	if( UDR->BLEnergyL->UDefSize == 0 && UDR->BLEnergy->UDefSize == 0 )
		return true;
	if( cb->GetEnergy() > storage*cb->GetEnergyStorage() && cb->GetEnergyIncome() > production*(cb->GetEnergyUsage()-ConDrainEnergyDebug) )
		return true;
	return false;
}

void cBuilder::BQAssignBuilder(int index, const int& unit, UnitInfo* U)
{
//*l<<"\nBQAssignBuilder("<<index<<","<<unit<<",-)";
	if( BQ[index]->builderID >= 0 )
	{
		const UnitDef* ud=BQ[index]->builderUI->ud;
		ConEnergyLost+=int(ud->energyCost);
		ConMetalLost+=int(ud->metalCost);
		ConEnergyDrain+=int(ud->energyCost/(ud->buildTime/UDR->AverageConstructSpeed));
		ConMetalDrain+=int(ud->metalCost/(ud->buildTime/UDR->AverageConstructSpeed));

		BQ[index]->builderUI->BuildQ=0;
		if( U!= 0 && U->BuildQ != 0 )
			U->BuildQ->builderID=-1;
		if( BQ[index]->RS != 0 )
		{
			BQ[index]->RS->builderID = -1;
			BQ[index]->SetRS(0);
		}
	}
	BQ[index]->builderID=unit;
	BQ[index]->builderUI=U;
	if( unit >= 0 )
	{
		BQ[index]->deletionFrame=1200+cb->GetCurrentFrame();
		ConEnergyLost-=int(U->ud->energyCost);
		ConMetalLost-=int(U->ud->metalCost);
		ConEnergyDrain-=int(U->ud->energyCost/(U->ud->buildTime/UDR->AverageConstructSpeed));
		ConMetalDrain-=int(U->ud->metalCost/(U->ud->buildTime/UDR->AverageConstructSpeed));
		BQ[index]->builderUI->BuildQ=BQ[index];
		float3 pos = cb->GetUnitPos(unit);
		int mapbody=G->GetCurrentMapBody(BQ[index]->creationUD->ud,pos);
		if( BP->NeedsResourceSpot(BQ[index]->creationUD->ud,mapbody) )
		{
			sResourceSpot* RS = BP->FindResourceSpot(pos,BQ[index]->creationUD->ud,U->mapBody);
			BQ[index]->SetRS(RS);
			if( RS != 0 )
			{
				BQ[index]->RS->builderID = unit;
				BQ[index]->RS->builderUI = U;
			}
		}
	}
}
/*
void cBuilder::BQAssignConstruct(int index, const int& unit, sRAIUnitDef *udr)
{

}
*/
void cBuilder::BQAdd(sRAIUnitDef *udr, sRAIBuildList *BL, int type)
{
//	*l<<"\n (New Build Order Added): "+udr->ud->humanName;
	BQ[BQSize[0]] = new sBuildQuarry(BL);
	BQ[BQSize[0]]->index = BQSize[0];
	BQ[BQSize[0]]->creationUD=udr;
	BQ[BQSize[0]]->creationUDID=udr->ud->id;
	BQ[BQSize[0]]->type = type;
	BQ[BQSize[0]]->deletionFrame=1200+cb->GetCurrentFrame();
	BQSize[0]++;
	BQSize[type]++;
	udr->UnitConstructs++;
	udr->CheckUnitLimit();
	ConEnergyLost+=int(udr->ud->energyCost);
	ConMetalLost+=int(udr->ud->metalCost);
	ConEnergyDrain+=int(udr->ud->energyCost/(udr->ud->buildTime/UDR->AverageConstructSpeed));
	ConMetalDrain+=int(udr->ud->metalCost/(udr->ud->buildTime/UDR->AverageConstructSpeed));
	ConEnergyRate+=int(udr->EnergyDifference);
	ConMetalRate+=int(udr->MetalDifference);
	ConEnergyStorage+=int(udr->ud->energyStorage);
	ConMetalStorage+=int(udr->ud->metalStorage);
}

void cBuilder::BQRemove(int index)
{
	if( BQ[index] == Prerequisite )
	{
		Prerequisite = 0;
	}

	sRAIUnitDef *udr=BQ[index]->creationUD;
	if( BQ[index]->builderID >= 0 )
	{
		ConDrainEnergyDebug=udr->ud->energyCost/(udr->ud->buildTime/BQ[index]->builderUI->ud->buildSpeed);
		ConDrainMetalDebug=udr->ud->metalCost/(udr->ud->buildTime/BQ[index]->builderUI->ud->buildSpeed);
		ConDrainFrameDebug=cb->GetCurrentFrame();
	}

	BQAssignBuilder(index,-1,0);
	BQSize[0]--;
	BQSize[BQ[index]->type]--;
	udr->UnitConstructs--;
	udr->CheckUnitLimit();

	ConEnergyLost-=int(udr->ud->energyCost);
	ConMetalLost-=int(udr->ud->metalCost);
	ConEnergyDrain-=int(udr->ud->energyCost/(udr->ud->buildTime/UDR->AverageConstructSpeed));
	ConMetalDrain-=int(udr->ud->metalCost/(udr->ud->buildTime/UDR->AverageConstructSpeed));

	ConEnergyRate-=int(udr->EnergyDifference);
	ConMetalRate-=int(udr->MetalDifference);
	ConEnergyStorage-=int(udr->ud->energyStorage);
	ConMetalStorage-=int(udr->ud->metalStorage);

	sBuildQuarry *sTemp=BQ[index];
	BQ[index]=BQ[BQSize[0]];
	BQ[BQSize[0]]=sTemp;
	BQ[index]->index=index;

	for( list<int>::iterator i=BQ[BQSize[0]]->creationID.begin(); i!=BQ[BQSize[0]]->creationID.end(); i++ )
		if( UConstruction.find(*i) != UConstruction.end() )
		{
			if( cb->UnitBeingBuilt(*i) )
				UConstruction.find(*i)->second.BQAbandoned=true;
			else
				UConstruction.erase(*i); // Remove from construction list
		}

	delete BQ[BQSize[0]];
}
