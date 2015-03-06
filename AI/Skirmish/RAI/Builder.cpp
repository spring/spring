#include "Builder.h"
#include "LegacyCpp/FeatureDef.h"
#include "LegacyCpp/WeaponDef.h"
#include "LegacyCpp/CommandQueue.h"

sBuildQuarry::sBuildQuarry(sRAIBuildList *buildlist)
{
	BL=buildlist;
	if( BL != 0 )
		BL->unitsActive++;
	builderID=-1;
	RS=0;
	tryCount=0;
	builderUI=0;
	creationUDID=0;
	creationUD=0;
	index=0;
	type=0;
	deletionFrame=0;
}

sBuildQuarry::~sBuildQuarry()
{
	if( BL != 0 )
		BL->unitsActive--;
}

bool sBuildQuarry::IsValid(int frame)
{
	if( int(creationID.size()) == 0 && builderID == -1 && frame >= deletionFrame )
		return false;
	return true;
}

void sBuildQuarry::SetRS(ResourceSiteExt* rs)
{
	if( RS != 0 )
		RS->builderID=-1;
	RS = rs;
	if( RS != 0 )
		RS->builderID=builderID;
}
// ------------------------------------------------------------------------------------------------

cBuilder::cBuilder(IAICallback* callback, cRAI* global)
{
	cb = callback;
	G = global;
	UDR = G->UDH;
	for( map<int,sRAIUnitDef>::iterator iU=UDR->UDR.begin(); iU!=UDR->UDR.end(); ++iU )
		if( int(iU->second.ud->wreckName.size()) > 0 )
			UDRResurrect.insert(srPair(iU->second.ud->wreckName,&iU->second));

	l=G->l;
	bInitiated=false;
	LastBuildOrder=0;
	BuilderIDDebug=0;
	BuilderFrameDebug=0;
	ConEnergyLost=0;
	ConMetalLost=0;
	ConEnergyDrain=0;
	ConMetalDrain=0;
	ConEnergyRate=0;
	ConMetalRate=0;
	ConEnergyStorage=0;
	ConMetalStorage=0;
	memset(BQ, 0, BUILD_QUARRY_SIZE);
	BuilderMetalDebug = 0;
	BuilderEnergyDebug = 0;
	ECostLimit = 0;
	MCostLimit = 0;

	for( int i=0; i<8; i++ )
		BQSize[i]=0;
	Prerequisite=0;

	PM = new cPowerManager(cb,G);
	BP = 0;
}

cBuilder::~cBuilder()
{
	if( RAIDEBUGGING )
	{
		*l<<"\n cBuilder Debug:";
		*l<<"\n  clearing Build Quarry ...";
	}
	while( BQSize[0] > 0 )
		BQRemove(0);
	if( RAIDEBUGGING )
	{
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

	delete BP;
	delete PM;
}

void cBuilder::UnitCreated(const int& unit, UnitInfo *U)
{
	if( BP == 0 )
		BP = new cBuilderPlacement(cb,G);

	// Attempts to identify which Build Quarry this new unit belongs to.
	int iBQIndex=-1;
	for(int iBQ=0; iBQ<BQSize[0]; ++iBQ)
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
					// Thus constructors, which are set to build at a certain position, can build at a distance that even exceeds their build distance (or at least, that is what these variables have told me during testing)
					if( (fx < fMarginOfError && fx > -fMarginOfError && fz < fMarginOfError && fz > -fMarginOfError) ||
						(fx2 < fMarginOfError && fx2 > -fMarginOfError && fz2 < fMarginOfError && fz2 > -fMarginOfError) )
					{
//						*l<<"\n\nUnit position:  x="<<cb->GetUnitPos(unit).x<<" z="<<cb->GetUnitPos(unit).z;
//						*l<<"\nBuilder position: x="<<cb->GetUnitPos(BQ[iBQ]->builderID).x<<" z="<<cb->GetUnitPos(BQ[iBQ]->builderID).z;
//						*l<<"\nBOrder position:  x="<<c->params.at(0)<<" z="<<c->params.at(2);
//						*l<<"\nMarginOfError: "<<fMarginOfError<<"\n\n";
						if( iBQIndex > -1 ) // It found more than 1 possible Build Quarry match
						{
							if( RAIDEBUGGING ) *l<<"\nWARNING: Creation Def Conflict, code needs works";
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
		for(map<int,UnitInfo>::iterator iU=G->Units.begin(); iU!=G->Units.end(); ++iU)
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
		if( RAIDEBUGGING ) *l<<"(b="<<BQ[iBQIndex]->builderID<<")";
		typedef pair<int,UnitConstructionInfo> iuPair;
		UConstruction.insert(iuPair(unit,UnitConstructionInfo(BQ[iBQIndex],unit,U)));
		U->AIDisabled=false;
	}
	else if( humanCreated >= 0 && (U->ud->speed > 0 || int(U->udr->BuildOptions.size()) == 0) )
	{
		U->AIDisabled = true;
	}
	else
	{
		// This case can occur 4 ways: 1) starting units 2) resurrected units 3) RAI failed to identify it's builder 4) factory built by human player
		U->AIDisabled = false;
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
					G->CorrectPosition(fPos);
					c.params.push_back(fPos.x);
					c.params.push_back(fPos.y);
					c.params.push_back(fPos.z);
					cb->GiveOrder(unit,&c);
					G->UpdateEventAdd(1,cb->GetCurrentFrame()+150,unit,U);
				}
			}
			if( RAIDEBUGGING ) *l<<"(b="<<pUCQ->BQ->builderID<<")";
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
		for(map<int,UnitInfo*>::iterator i=UNanos.begin(); i!=UNanos.end(); ++i )
		{
			if( cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(i->first)) < i->second->ud->buildDistance )
			{
				i->second->UGuarding.insert(cRAI::iupPair(unit,U));
				U->UGuards.insert(cRAI::iupPair(i->first,i->second));
//				if( U->pBOL != 0 && U->udrBL->task == TASK_CONSTRUCT && int(U->udr->BuildOptions.size()) > 0 )
//					i->second->UAssist.insert(cRAI::iupPair(unit,U));
			}
		}
		if( U->udr->IsNano() )
		{
			UNanos.insert(cRAI::iupPair(unit,U));
			for(map<int,UnitInfo*>::iterator i=G->UImmobile.begin(); i!=G->UImmobile.end(); ++i )
			{
				if( i->first != unit && !i->second->unitBeingBuilt && cb->GetUnitPos(unit).distance2D(cb->GetUnitPos(i->first)) < U->ud->buildDistance )
				{
					U->UGuarding.insert(cRAI::iupPair(i->first,i->second));
					i->second->UGuards.insert(cRAI::iupPair(unit,U));
//					if( i->second->udrBL->task == TASK_CONSTRUCT && int(i->second->udr->BuildOptions.size()) > 0 )
//						U->UAssist.insert(cRAI::iupPair(i->first,i->second));
				}
			}
		}
	}
}

void cBuilder::UnitDestroyed(const int& unit, UnitInfo* U)
{
	if( U->unitBeingBuilt )
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
		U->udrBL->RBL->unitsActive--;
		U->udr->CheckUnitLimit();
		U->udr->CheckBuildOptions();
		if( U->ud->speed == 0 )
		{
			if( U->udr->IsNano() )
			{
				UNanos.erase(unit);
				for(map<int,UnitInfo*>::iterator i=U->UGuarding.begin(); i!=U->UGuarding.end(); ++i )
					i->second->UGuards.erase(unit);
			}
			for(map<int,UnitInfo*>::iterator i=U->UGuards.begin(); i!=U->UGuards.end(); ++i )
			{
				i->second->UGuarding.erase(unit);
//				i->second->UAssist.erase(unit);
			}
		}
	}
}

void cBuilder::UnitAssignBuildList(const int& unit, UnitInfo *U, bool bInitialized)
{
	if( bInitialized )
		U->udrBL->RBL->unitsActive--;
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
		cb->SendTextMsg("unknown unit type in use",0);
		*l<<"\n\nWARNING: unknown unit type in use: ("<<U->ud->id<<")"<<U->ud->humanName;
		U->udr->UnitsActive.erase(unit);
		U->AIDisabled = true;
		return;
	}
	U->udrBL=U->udr->List[iBestI];
	U->udrBL->RBL->unitsActive++;
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
//	*l<<" UBuilderIdle:";
	int iFrame = cb->GetCurrentFrame();
	float OnOffPower = -PM->OffEnergyDifference;
	if( OnOffPower > 0.25*cb->GetEnergyIncome() )
		OnOffPower = 0.25*cb->GetEnergyIncome();
	float fWeaponPower = -PM->WeaponEnergyNeeded;
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
	float MetalRate=cb->GetMetalIncome()-cb->GetMetalUsage()+PM->EtoMIncome+0.75*ConMetalRate;
	float EnergyRate=cb->GetEnergyIncome()-cb->GetEnergyUsage()-OnOffPower-fWeaponPower+0.75*ConEnergyRate;

	// This is a small attempt to correct the values reported by callback, however having this makes a big difference when the commander is building it's first few units
	if( BuilderFrameDebug+30 >= cb->GetCurrentFrame() && G->Units.find(BuilderIDDebug) != G->Units.end() && G->ValidateUnit(BuilderIDDebug) )
	{
//		*l<<"\n\n CF:"<<cb->GetCurrentFrame()<<" DF:"<<BuilderFrameDebug<<" Dif:"<<cb->GetCurrentFrame()- BuilderFrameDebug;
		UnitInfo *B = &G->Units.find(BuilderIDDebug)->second;
		UnitResourceInfo URI;
		cb->GetUnitResourceInfo(BuilderIDDebug,&URI);
		BuilderEnergyDebug = URI.energyUse;
		BuilderMetalDebug = URI.metalUse;
		if( B->ud->onoffable && cb->IsUnitActivated(BuilderIDDebug) )
		{
			BuilderEnergyDebug -= B->ud->energyUpkeep;
			BuilderMetalDebug -= B->ud->metalUpkeep;
		}
		if( B->ud->canCloak && cb->IsUnitCloaked(BuilderIDDebug) )
			BuilderEnergyDebug += B->udr->CloakMaxEnergyDifference;
		// Testing on Spring-Version(0.76b1) XTA V9.44 has shown GetUnitResourceInfo to be about 90%-100% accurate depending on the frame
//		*l<<"\n Metal/Energy Usage: "<<cb->GetMetalUsage()<<"/"<<cb->GetEnergyUsage();
//		*l<<"\n Debugging Metal/Energy Rates: "<<BuilderMetalDebug<<"/"<<BuilderEnergyDebug;
		MetalRate+=BuilderMetalDebug;
		EnergyRate+=BuilderEnergyDebug;
	}
	else
	{
		BuilderMetalDebug=0;
		BuilderEnergyDebug=0;
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
		HaveOrders=false;
	if( !HaveOrders && int(U->udr->BuildOptions.size()) > 0 )
	{
		for(int iBQ=0; iBQ<BQSize[0]; iBQ++) // Checking old build orders
		{
			if( !BQ[iBQ]->IsValid(iFrame) ||
				(iFrame > BQ[iBQ]->deletionFrame+5400 && // 5400f = 3mins
				 int(BQ[iBQ]->creationID.size()) > 0 &&
				 cb->GetUnitHealth(BQ[iBQ]->creationID.front()) < 0.02*cb->GetUnitMaxHealth(BQ[iBQ]->creationID.front()) ) )
			{
				if( BQ[iBQ]->builderID > -1 )
					G->UpdateEventAdd(1,0,BQ[iBQ]->builderID,BQ[iBQ]->builderUI);
				BQRemove(iBQ--);
			}
		}
		if( BQSize[0] < 30 && BQSize[0] < int(UBuilder.size())+1 && UDR->BLActive > 0 )
		{
			G->ValidateUnitList(&UBuilder);
			BP->CheckBlockedRList();
			if( !UDR->RBMobile.empty() )
			{
				float3 position = cb->GetUnitPos(unit);
				G->CorrectPosition(position);
				int iS = G->TM->GetSectorIndex(position);
				set<TerrainMapMobileType*> deletion;
				for(set<TerrainMapMobileType*>::iterator iM=UDR->RBMobile.begin(); iM!=UDR->RBMobile.end(); ++iM)
					if( G->TM->GetAlternativeSector(U->area,iS,*iM)->S->position.distance2D(G->TM->sector[iS].position) < 700.0 )
						deletion.insert(*iM);
				while( !deletion.empty() )
				{
					UDR->RBMobile.erase(*deletion.begin());
					deletion.erase(*deletion.begin());
				}
			}
			if( !(UDR->RBImmobile.empty()) )
			{
				float3 position = cb->GetUnitPos(unit);
				G->CorrectPosition(position);
				int iS = G->TM->GetSectorIndex(position);
				set<TerrainMapImmobileType*> deletion;
				for(set<TerrainMapImmobileType*>::iterator iM=UDR->RBImmobile.begin(); iM!=UDR->RBImmobile.end(); ++iM)
					if( G->TM->GetClosestSector(*iM,iS)->position.distance2D(G->TM->sector[iS].position) < 700.0 )
						deletion.insert(*iM);
				while( !deletion.empty() )
				{
					UDR->RBImmobile.erase(*deletion.begin());
					deletion.erase(*deletion.begin());
				}
			}
			for(int i=0; i<UDR->BLSize; i++ )
				UDR->BL[i]->UDefActiveTemp = UDR->BL[i]->UDefActive;
			for(int i=0; i<UDR->BLActive; i++ )
			{	// If a list is unimportant, then make sure this list can build something that already has a prerequisite
				if( UDR->BL[i]->minUnits == 0 )
					for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
						if( !UDR->BL[i]->UDef[iud]->RUD->HasPrerequisite )
							UDR->BL[i]->UDefSwitch(iud,--UDR->BL[i]->UDefActiveTemp);
				// Restrict unit types that would require moving too far away
				if( !UDR->RBMobile.empty() )
					for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
						if( UDR->BL[i]->UDef[iud]->RUD->mobileType != 0 && UDR->RBMobile.find(UDR->BL[i]->UDef[iud]->RUD->mobileType) != UDR->RBMobile.end() )
							UDR->BL[i]->UDefSwitch(iud,--UDR->BL[i]->UDefActiveTemp);
				if( !UDR->RBImmobile.empty() )
					for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
						if( UDR->BL[i]->UDef[iud]->RUD->immobileType != 0 )
							if( UDR->RBImmobile.find(UDR->BL[i]->UDef[iud]->RUD->immobileType) != UDR->RBImmobile.end() ||
								(UDR->BL[i]->UDef[iud]->RUD->mobileType != 0 && UDR->RBMobile.find(UDR->BL[i]->UDef[iud]->RUD->mobileType) != UDR->RBMobile.end()) )
								UDR->BL[i]->UDefSwitch(iud,--UDR->BL[i]->UDefActiveTemp);
				// make sure the first few extractors are built at the closest Metal-Sites available by restricting the wrong types
				if( UDR->BL[i] == UDR->BLMetalL && UDR->BL[i]->unitsActive < UDR->BL[i]->minUnits )
				{
					float3 Pos = cb->GetUnitPos(unit);
					set<int> best;
					ResourceSiteExt* RSbest=NULL;
					for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
					{
						ResourceSiteExt* RS = BP->FindResourceSite(Pos,UDR->BL[i]->UDef[iud]->RUD->ud,U->area);
						if( RS != 0 )
						{
							if( int(best.size()) == 0 || RS == RSbest )
							{
								best.insert(iud);
								RSbest = RS;
							}
							else if( Pos.distance2D(RS->S->position) < Pos.distance2D(RSbest->S->position) )
							{
								best.clear();
								best.insert(iud);
								RSbest = RS;
							}
						}
					}
					if( int(best.size()) > 0 )
						for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
							if( best.find(iud) == best.end() )
							{
								UDR->BL[i]->UDefActiveTemp--;
								UDR->BL[i]->UDefSwitch(iud,UDR->BL[i]->UDefActiveTemp);
							}
				}
			}
		}
		while( BQSize[0] < 30 && BQSize[0] < int(UBuilder.size())+1 && UDR->BLActive > 0 )
		{
//			*l<<"\n\n(NBO)"; if( Prerequisite != 0 ) *l<<"(Prerequisite: "<<Prerequisite->creationUD->ud->humanName<<") ";
			CreateBuildOrders();// UNFINISHED
			float fBestV=-1;	// Value
			int iBestI=-1;		// Index
			for(int i=0; i<UDR->BLActive; i++ )
			{	// If a prerequisite is being built, then make sure this list can build something that already has a prerequisite
				if( Prerequisite != 0 )
					for( int iud=UDR->BL[i]->UDefActiveTemp-1; iud>=0; iud-- )
						if( !UDR->BL[i]->UDef[iud]->RUD->HasPrerequisite )
							UDR->BL[i]->UDefSwitch(iud,--UDR->BL[i]->UDefActiveTemp);
//				*l<<"\n"<<UDR->BL[i]->Name<<" Build-List "<<UDR->BL[i]->unitsActive<<"/"<<UDR->BL[i]->priority<<": Min="<<UDR->BL[i]->minUnits<<" UDefActiveTemp="<<UDR->BL[i]->UDefActiveTemp;
				if( UDR->BL[i]->UDefActiveTemp > 0 )
				{
					if( UDR->BL[i]->unitsActive < UDR->BL[i]->minUnits && (fBestV<0 || UDR->BL[iBestI]->minUnits == 0 || float(UDR->BL[i]->unitsActive)/float(UDR->BL[i]->minUnits) < fBestV) )
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
			}
			if( iBestI == -1 ) // ? hopefully won't ever happen again
			{
				cb->SendTextMsg("ERROR: No Build-List Choice Available.",5);
				*l<<"\n\nERROR: No Build-List Choice Available.";
				break;
			}

			sRAIBuildList *BL = UDR->BL[iBestI];
			if( BL->unitsActive < BL->minUnits )
			{
				int temp = BL->UDefActiveTemp;
				for( int iud=BL->UDefActiveTemp-1; iud>=0; iud-- )
					if( !BL->UDef[iud]->RUD->HasPrerequisite )
						BL->UDefSwitch(iud,--BL->UDefActiveTemp);
				if( BL->UDefActiveTemp == 0 )
					BL->UDefActiveTemp = temp;
			}

			int iBuildType=1;
			if( BL == UDR->BLEnergy || BL == UDR->BLEnergyL )
				iBuildType=2;
			else if( BL == UDR->BLMetal || BL == UDR->BLMetalL )
				iBuildType=3;
			else if( BL == UDR->BLBuilder )
				iBuildType=4;

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

			float fBuildTime=udr->ud->buildTime/UDR->AverageConstructSpeed;
			float EnergyDemand = -1;
			float MetalDemand = -1;
			float ConstructorDemand = -1;
			float EnergyStorageDemand = -1;
			float MetalStorageDemand = -1;

			if( BL->unitsActive >= BL->minUnits && BL != UDR->BLEnergy && BL != UDR->BLMetal && BL != UDR->BLMetalL )
			{
				if( (UDR->BLEnergyL->UDefActiveTemp > 0 || UDR->BLEnergy->UDefActiveTemp > 0) && BQSize[2] < 5 && BQSize[2] < 0.4*BQSize[0] )
					EnergyDemand = -(fBuildTime*EnergyRate+cb->GetEnergy()-0.1*ConEnergyLost-udr->ud->energyCost)*ERatio/UDR->EnergyToMetalRatio;
				if( (UDR->BLMetalL->UDefActiveTemp > 0 || UDR->BLMetal->UDefActiveTemp > 0) && BQSize[3] < 5 && BQSize[3] < 0.4*BQSize[0] && !( EnergyDemand>0 && udr->HighEnergyDemand ) )
				{
					MetalDemand = -(fBuildTime*MetalRate+cb->GetMetal()-0.1*ConMetalLost-udr->ud->metalCost)*MRatio;
					if( MetalDemand <= 0 && UDR->BLMetalL->UDefActiveTemp > 0 && BQSize[3] == 0 )
						MetalDemand = 3;
				}
				if( UDR->BLBuilder->UDefActiveTemp > 0 && BQSize[4] < 2 && BQSize[4] < 0.3*BQSize[0] )
				{
					if( EnergyRate > 0 && MetalRate > 0 && 30.0*EnergyRate+cb->GetEnergy()-(0.6+0.1*BQSize[4])*cb->GetEnergyStorage() > 0 && 30.0*MetalRate+cb->GetMetal()-(0.6+0.1*BQSize[4])*cb->GetMetalStorage() > 0 )
						ConstructorDemand = 2;
				}
				if( UDR->BLEnergyStorage->UDefActiveTemp > 0 && BQSize[5] < 1 )
				{
					if( (cb->GetEnergyStorage()+ConEnergyStorage < 2.5*cb->GetEnergyUsage()) ||
						(cb->GetEnergyStorage()+ConEnergyStorage < udr->WeaponMaxEnergyCost) )
						EnergyStorageDemand = 1;
				}
				if( UDR->BLMetalStorage->UDefActiveTemp > 0 && BQSize[6] < 1 )
				{
					if( cb->GetMetalStorage()+ConMetalStorage < 2.5*cb->GetMetalUsage() )
						MetalStorageDemand = 1;
				}
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
*l<<"\n EtoMEnergyNeeded="<<PM->EtoMEnergyNeeded;
*l<<"\n EtoMMetalIncome="<<PM->EtoMIncome;
*l<<"\n OnOffPower="<<OnOffPower;
*l<<"\n fWeaponPower="<<fWeaponPower;
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
*l<<"\n ConstructorDemand="<<ConstructorDemand;
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
			if( ConstructorDemand > fHigh )
			{
				fHigh=ConstructorDemand;
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
				if( iBuildType == 3 && udr->HighEnergyDemand && PM->EtoMNeeded < 0 )
				{
					iBuildType=2;
					if( UDR->BLEnergyL->UDefActiveTemp > 0 )
						BL=UDR->BLEnergyL;
					else
						BL=UDR->BLEnergy;
					iIndex=rand()%BL->UDefActiveTemp;
//					*l<<" BL Index="<<iIndex;
					udr = BL->UDef[iIndex]->RUD;
				}
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
			//int iBest=-1;
			//int Count=0;
			for(map<int,sRAIUnitDef*>::iterator iP=udr->PrerequisiteOptions.begin(); iP!=udr->PrerequisiteOptions.end() && !bPrerequisiteOptionsChecked; ++iP)
			{
				for(set<int>::iterator iU=iP->second->UnitsActive.begin(); iU!=iP->second->UnitsActive.end() && !bPrerequisiteOptionsChecked; ++iU )
					if( UBuilder.find(*iU) != UBuilder.end() && UBuilder.find(*iU)->second->BuildQ == 0 && *iU != unit )
					{
						if( int(cb->GetCurrentUnitCommands(*iU)->size()) == 0 || cb->GetCurrentUnitCommands(*iU)->front().id == CMD_WAIT )
							G->UpdateEventAdd(1,0,*iU,UBuilder.find(*iU)->second);
						bPrerequisiteOptionsChecked=true;
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
			MetalRate=cb->GetMetalIncome()-cb->GetMetalUsage()+0.1*PM->EtoMIncome+0.75*ConMetalRate; //+0.75*PM->MetalProduction
			EnergyRate=cb->GetEnergyIncome()-cb->GetEnergyUsage()-OnOffPower-fWeaponPower+0.75*ConEnergyRate;
		}
		// Search existing build orders
		float Demand[8]; // index = type, higher value mean that type should be built first
		Demand[1] = 0;
		Demand[2] = 3;
		Demand[3] = 3;
		if( cb->GetEnergy()*(cb->GetEnergyIncome()/std::max(0.1f, cb->GetEnergyUsage())) <
			cb->GetMetal()*(cb->GetMetalIncome()/std::max(0.1f, cb->GetMetalUsage())) )
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
			if( BQ[iBQ]->builderID == -1 && U->udr->BuildOptions.find(BQ[iBQ]->creationUDID) != U->udr->BuildOptions.end() )
				if( iBest == -1 || BQ[iBQ]==Prerequisite || (BQ[iBest]!=Prerequisite && (Demand[BQ[iBQ]->type] > Demand[BQ[iBest]->type] || (BQ[iBest]->type==BQ[iBQ]->type && BQ[iBest]->creationUD->HighEnergyDemand && !BQ[iBQ]->creationUD->HighEnergyDemand) ) ) )
					iBest=iBQ;
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
			for( map<int,float3>::iterator iR=ResDebris.begin(); iR!=ResDebris.end(); ++iR )
			{
				if( (iBest == -1 || Pos.distance(debPos) > Pos.distance(iR->second)) && G->TM->CanMoveToPos(U->area,iR->second) )
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
				c.params.push_back(debPos.x); // ! Work Around:  Spring-Version(v0.74b3)
				c.params.push_back(debPos.y);
				c.params.push_back(debPos.z);
				c.params.push_back(25.0f);
				cb->GiveOrder(unit, &c);
				float fTime=Pos.distance(debPos)/(U->ud->speed/3.0);
				G->UpdateEventAdd(1,cb->GetCurrentFrame()+1500+30*int(fTime),unit,U);
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
		if( int(Decomission.size()) > 0 && G->TM->CanMoveToPos(U->area,cb->GetUnitPos(*Decomission.begin())) )
		{
//			*l<<"\n "+U->ud->humanName+" is Reclaim/Decomissioning.";
			c.id=CMD_RECLAIM;
			c.params.push_back(*Decomission.begin());
			cb->GiveOrder(unit, &c);
			return;
		}
		if( int(FeatureDebris.size()) > 0 && G->TM->CanMoveToPos(U->area,FeatureDebris.begin()->second) )
		{
			map<int,float3>::iterator i=FeatureDebris.begin();
			float3 fPos = i->second;

			int *F = new int[1];
			if( cb->GetFeatures(F,1,i->second,20.0) == 1 )
			{
//				*l<<"\n "+U->ud->humanName+" is Reclaim/Clearing.";
				c.id=CMD_RECLAIM;
				c.params.push_back(fPos.x); // ! Work Around:  Spring-Version(v0.74b3)
				c.params.push_back(fPos.y);
				c.params.push_back(fPos.z);
				c.params.push_back(80.0);
				cb->GiveOrder(unit, &c);

				float fTime=cb->GetUnitPos(unit).distance(fPos)/(U->ud->speed/3.0);
				G->UpdateEventAdd(1,cb->GetCurrentFrame()+150+30*int(fTime),unit,U);
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
				for( map<int,float3>::iterator iM=MetalDebris.begin(); iM!=MetalDebris.end(); ++iM )
				{
					if( (iBest == -1 || Pos.distance(debPos) > Pos.distance(iM->second)) && G->TM->CanMoveToPos(U->area,iM->second) )
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
					c.params.push_back(debPos.x); // ! Work Around:  Spring-Version(v0.74b3)
					c.params.push_back(debPos.y);
					c.params.push_back(debPos.z);
					c.params.push_back(25.0f);
					cb->GiveOrder(unit, &c);

					float fTime=Pos.distance(debPos)/(U->ud->speed/3.0);
					G->UpdateEventAdd(1,cb->GetCurrentFrame()+1500+30*int(fTime),unit,U);
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
				for( map<int,float3>::iterator iE=EnergyDebris.begin(); iE!=EnergyDebris.end(); ++iE )
				{
					if( (iBest == -1 || Pos.distance(debPos) > Pos.distance(iE->second)) && G->TM->CanMoveToPos(U->area,iE->second) )
					{
						iBest = iE->first;
						debPos = iE->second;
					}
				}
				if( iBest != -1 )
				{
//					*l<<"\n "+U->ud->humanName+" is Reclaim/M Gathering.";
					c.id=CMD_RECLAIM;
					c.params.push_back(debPos.x); // ! Work Around:  Spring-Version(v0.74b3)
					c.params.push_back(debPos.y);
					c.params.push_back(debPos.z);
					c.params.push_back(25.0f);
					cb->GiveOrder(unit, &c);
					float fTime=Pos.distance(debPos)/(U->ud->speed/3.0);
					G->UpdateEventAdd(1,cb->GetCurrentFrame()+1500+30*int(fTime),unit,U);
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
		float3 position = cb->GetUnitPos(unit);
		G->CorrectPosition(position);
		for( map<int,sRAIUnitDef*>::iterator iB=U->udr->BuildOptions.begin(); iB!=U->udr->BuildOptions.end(); ++iB )
			if( iB->second->CanBeBuilt && BP->CanBeBuiltAt(iB->second,position) )
				for( int iL=0; iL<iB->second->ListSize; iL++ )
					if( iB->second->List[iL]->RBL->Name == "Mobile Anti-Land/Air" ||
						(iB->second->List[iL]->RBL->Name == "Mobile Anti-Naval" && iB->second->List[iL]->RBL->priority > 0 ) )
					{
						build.push_back(iB->first);
						break;
					}
		if( int(build.size()) > 0 )
		{
			int iRan=rand()%int(build.size());
			BQAdd(U->udr->BuildOptions.find(build.at(iRan))->second,0,1);
			BQAssignBuilder(BQSize[0]-1,unit,U);
			c.id=-U->BuildQ->creationUDID;
			HaveOrders=true;
		}
	}
	if( HaveOrders && c.id < 0 ) // Has Build Orders
	{
		if( int(U->BuildQ->creationID.size()) > 0 )
		{	// Resuming a construction
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
		float3 position = BP->FindBuildPosition(U->BuildQ);
		if( !cb->CanBuildAt(bd,position) )
		{	// FindBuildPosition() most likely returned an error position (-[1-3],-1,-1)
//			*l<<"\n FindBuildPosition() failed:"<<position.x;
			if( U->ud->speed==0.0 && bd->speed>0.0 )
			{	// Fix for the mod gundam v1.1-v1.11 ( cb->CanBuildAt() & cb->ClosestBuildSite() will almost always fails).
				// In addition to this, cb->GiveOrder() will fail if the position is invalid, even though the position doesn't matter
				// this condition unfortunately assumes that all immobile constructors build mobile units at their center
				position=cb->GetUnitPos(unit);
			}
			else if( U->BuildQ->RS != 0 && U->BuildQ->RS->unitID > -1 && cb->GetUnitTeam(U->BuildQ->RS->builderID) == cb->GetUnitTeam(U->BuildQ->RS->unitID) && U->ud->canReclaim )
			{	// removing an old extractor while upgrading it
//				*l<<"\n "+U->ud->humanName+" is Reclaim/Upgrade/Building.";
				c.id = CMD_RECLAIM;
				c.params.push_back(U->BuildQ->RS->unitID);
				cb->GiveOrder(unit, &c);
				return;
			}
			else
			{
				if( !U->BuildQ->creationUD->CanBeBuilt )
				{	// usually this means an intended resource is no longer available
					BQRemove(U->BuildQ->index);
				}
				else
				{
					*l<<"\nWARNING: ("<<unit<<")"<<U->ud->humanName<<" can not build a '"<<bd->humanName<<"' at selected position: x="<<position.x<<" y="<<position.y<<" z="<<position.z;
					BQAssignBuilder(U->BuildQ->index,-1,0); // unassign builder
				}
				G->UpdateEventAdd(1,0,unit,U);
				return;
			}
		}
//		*l<<"\n "+U->ud->humanName+" is Building a "+bd->humanName+"("<<-c.id<<") at (x"<<position.x<<",z"<<position.z<<",y"<<position.y<<")";
		c.params.push_back(position.x);
		c.params.push_back(position.y);
		c.params.push_back(position.z);

		if( U->BuildQ->creationUD->ud->movedata == 0 && int(U->BuildQ->creationUD->BuildOptions.size()) > 0 )
		{	// decides if a factory should face the opposite direction due to bad terrain
			float3 positionH = position;
			positionH.z+=48.0f;
//			positionH.y=cb->GetElevation(positionH.x,positionH.z);
			float3 positionL = position;
			positionL.z-=48.0f;
//			positionL.y=cb->GetElevation(positionL.x,positionL.z);
			if( position.z/8 >= cb->GetMapHeight()-5-(U->BuildQ->creationUD->ud->zsize/2) ||
				(cb->CanBuildAt(bd,positionL) && !cb->CanBuildAt(bd,positionH) ) )
				c.params.push_back(2);
		}
		if( U->ud->buildSpeed == 0 )
		{	// Work-Around for the mod: fibre v13.1
			// This is more experimental than anything meaningful
			if( c.params.size() == 3 )
				c.params.push_back(0);
			G->UpdateEventAdd(1,cb->GetCurrentFrame()+300,unit,U);
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
			float BestDis=0.0f;
			for(int iBQ=0; iBQ<BQSize[0]; iBQ++)
			{
				if( BQ[iBQ]->builderID >= 0 && G->TM->CanMoveToPos(U->area,cb->GetUnitPos(BQ[iBQ]->builderID)) )
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
				G->UpdateEventAdd(1,cb->GetCurrentFrame()+600,unit,U);
				cb->GiveOrder(unit, &c);
				return;
			}
		}
		else if( U->group != 0 )
		{
			for( map<int,UnitInfo*>::iterator i = U->group->Units.begin(); i != U->group->Units.end(); ++i )
			{
				if( i->second->BuildQ != 0 )
				{
					c.id = CMD_GUARD;
					c.params.push_back(i->first);
					G->UpdateEventAdd(1,cb->GetCurrentFrame()+600,unit,U);
					cb->GiveOrder(unit, &c);
					return;
				}
			}
		}
	}

//	*l<<"\n "+U->ud->humanName+" is Waiting";
//	G->UpdateEventAdd(1,cb->GetCurrentFrame()+600,unit,U);
	c.id = CMD_WAIT;
	c.timeOut=cb->GetCurrentFrame()+600;

	// Wait commands are usually issued so that I know a unit that is doing nothing
	// is suppose to be doing nothing, but eventually this command broke the factories.
	// If factories are given wait orders they will stop responding to all future commands
	// cb->cb->GetCurrentUnitCommands()->size() will always = 0 from that point on
	if( U->ud->movedata == 0 ) // Work Around:  Spring-Version(v0.74b1-0.76b1)
	{
		G->UpdateEventAdd(1,cb->GetCurrentFrame()+600,unit,U);
		return;
	}
	cb->GiveOrder(unit, &c);
}

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
			c.params.push_back(fPos.x); // ! Work Around:  Spring-Version(v0.74b3)
			c.params.push_back(fPos.y);
			c.params.push_back(fPos.z);
			c.params.push_back(90.0);
			//c.params.push_back(F[0]);

			G->UpdateEventAdd(1,cb->GetCurrentFrame()+1200,unit,U);
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
	for( vector<int>::const_iterator i=pce->units.begin(); i!=pce->units.end(); ++i )
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
	MCostLimit = cb->GetMetalIncome()+PM->EtoMIncome;
	ECostLimit = cb->GetEnergyIncome();
	if( MCostLimit > 110.0 || (UDR->BLMetal->UDefSize == 0 && UDR->BLMetalL->UDefSize == 0) )
		MCostLimit=9.9e8;
	if( ECostLimit > 110.0*UDR->EnergyToMetalRatio || (UDR->BLEnergy->UDefSize == 0 && UDR->BLEnergyL->UDefSize == 0) )
		ECostLimit=9.9e8;

//	*l<<"\n Rechecking Unit Costs and Determining Active BL Options ...";
	for( map<int,sRAIUnitDef>::iterator i=UDR->UDR.begin(); i!=UDR->UDR.end(); ++i )
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
		if( UDR->BL[iBL]->minUnits > 0 && UDR->BL[iBL]->UDefSize > 0 && UDR->BL[iBL]->UDefSize > UDR->BL[iBL]->UDefActive )
		{
//			*l<<"\n Determining Cheapest for '"<<UDR->BL[iBL]->Name<<"' Build-List ...";
			sRAIUnitDef *BestLandudr=0,*BestWaterudr=0; // NOTE: the same udr may be selected for both, may also already be enabled
			float BestLandCost=0.0f,BestWaterCost=0.0f;
			bool BestLandCanBuildConstructors=false,BestWaterCanBuildConstructors=false; // only used in determining the cheapest constructor
			for(int iU=0; iU<UDR->BL[iBL]->UDefSize; iU++)
			{
				sRAIUnitDef *udr = UDR->BL[iBL]->UDef[iU]->RUD;
				if( !udr->Disabled && udr->HasPrerequisite && !udr->RBUnitLimit )
				{
					bool CanBuildConstructors = false; // only used in determining the cheapest constructor
					for( map<int,sRAIUnitDef*>::iterator iB=udr->BuildOptions.begin(); iB!=udr->BuildOptions.end(); ++iB)
						if( !iB->second->Disabled && int(iB->second->BuildOptions.size()) > 0 )
						{
							CanBuildConstructors = true;
							break;
						}
					float Cost = udr->MetalPCost + udr->EnergyPCost*UDR->EnergyToMetalRatio;
					if( udr->ud->minWaterDepth < 0 )
					{
						if( BestLandudr == 0 ||
							(CanBuildConstructors && !BestLandCanBuildConstructors) ||
							(Cost < BestLandCost && (CanBuildConstructors || !BestLandCanBuildConstructors) ) )
						{
							BestLandudr = udr;
							BestLandCost = Cost;
							BestLandCanBuildConstructors = CanBuildConstructors;
						}
					}
					if( udr->ud->maxWaterDepth > -G->TM->minElevation || udr->ud->floater )
					{
						if( BestWaterudr == 0 ||
							(CanBuildConstructors && !BestWaterCanBuildConstructors) ||
							(Cost < BestWaterCost && (CanBuildConstructors || !BestWaterCanBuildConstructors) ) )
						{
							BestWaterudr = udr;
							BestWaterCost = Cost;
							BestWaterCanBuildConstructors = CanBuildConstructors;
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
	for(int iBQ=0; iBQ<BQSize[0]; iBQ++)
	{
		if( BQ[iBQ]->creationUD->RBCost )
		{
			*l<<"\n(Low Resources) Abandoning Construction: "<<BQ[iBQ]->creationUD->ud->humanName;
			BQRemove(iBQ--);
		}
	}
/*
	*l<<"\n\n Displaying Active Build-List Options ...";
	for(int iBL=0; iBL<UDR->BLActive; iBL++ )
	{
		*l<<"\n  "<<UDR->BL[iBL]->Name<<" Build-List(a="<<UDR->BL[iBL]->UDefActiveTemp<<"): ";
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
		// fd should never be NULL, but it seems to be sometimes
		// therefore, play safe
		if( fd != NULL && fd->reclaimable )
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

void cBuilder::CreateBuildOrders() const
{

}
/*
void cBuilder::UpdateResourceUsage()
{
	MetalUsage=0;
	EnergyUsage=0;
	for( int i=0; i < BQSize[0]; i++ )
	{
		if( BQ[i]->builderID > 0 && BQ[i]->creationID.size() > 0 )
		{
			if( BQ[i]->creationUD->ud->metalCost > 0 )
				MetalUsage+=BQ[i]->builderUI->ud->buildSpeed/BQ[i]->creationUD->ud->metalCost;
			if( BQ[i]->creationUD->ud->energyCost > 0 )
				EnergyUsage+=BQ[i]->builderUI->ud->buildSpeed/BQ[i]->creationUD->ud->energyCost;
		}
	}
}
*/
bool cBuilder::MetalIsFavorable(float storage,float production)
{
	if( UDR->BLMetalL->UDefSize == 0 && UDR->BLMetal->UDefSize == 0 )
		return true;
	if( cb->GetMetalIncome() > 5.0*(cb->GetMetalUsage()-BuilderMetalDebug) )
		return true;
	if( (cb->GetMetal() > storage*cb->GetMetalStorage() || cb->GetMetalIncome() > 0.33*cb->GetMetalStorage() ) && cb->GetMetalIncome() > production*(cb->GetMetalUsage()-BuilderMetalDebug) )
		return true;
	return false;
}

bool cBuilder::EnergyIsFavorable(float storage,float production)
{
	if( UDR->BLEnergyL->UDefSize == 0 && UDR->BLEnergy->UDefSize == 0 )
		return true;
	if( (cb->GetEnergy() > storage*cb->GetEnergyStorage() || cb->GetEnergyIncome() > 0.33*cb->GetEnergyStorage() ) && cb->GetEnergyIncome() > production*(cb->GetEnergyUsage()-BuilderEnergyDebug) )
		return true;
	return false;
}

void cBuilder::BQAssignBuilder(int index, const int& unit, UnitInfo* U)
{
//	*l<<"\nBQAssignBuilder("<<BQ[index]->creationUD->ud->humanName<<","<<unit<<","<<(U==0?"-":U->ud->humanName)<<")";
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
		if( BP->NeedResourceSite(BQ[index]->creationUD->ud) )
		{
			ResourceSiteExt* RS = BP->FindResourceSite(pos,BQ[index]->creationUD->ud,U->area);
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
		BuilderIDDebug = BQ[index]->builderID;
		BuilderFrameDebug = cb->GetCurrentFrame();
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

	for( list<int>::iterator i=BQ[BQSize[0]]->creationID.begin(); i!=BQ[BQSize[0]]->creationID.end(); ++i )
		if( UConstruction.find(*i) != UConstruction.end() )
		{
			if( cb->UnitBeingBuilt(*i) )
				UConstruction.find(*i)->second.BQAbandoned=true;
			else
				UConstruction.erase(*i); // Remove from construction list
		}

	delete BQ[BQSize[0]];
}
