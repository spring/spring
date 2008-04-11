#include "BuilderPlacement.h"
#include "Sim/Units/UnitDef.h"
//#include "ExternalAI/IAICallback.h"
#include <set>

sResourceSpotBO::sResourceSpotBO(sRAIUnitDef* UDR )
{
	udr=UDR;
	CanBuild=false;
	RBRanged=true;
	RBBlocked=false;
	RBRanked=false;
}

void sResourceSpotBO::CheckBuild()
{
	if( RBBlocked || RBRanked || RBRanged )
	{
		if( CanBuild )
		{
			CanBuild=false;
			udr->SetULConstructs(udr->UnitLimit[1]-1);
		}
	}
	else
	{
		if( !CanBuild )
		{
			CanBuild=true;
			udr->SetULConstructs(udr->UnitLimit[1]+1);
		}
	}
}

sResourceSpot::sResourceSpot(IAICallback* callback, float3 position, int ID, const FeatureDef* featureUD)
{
	cb=callback;
	featureID=ID;
	featureD=featureUD;
	unitID=-1;
	builderID=-1;
	location=position;
	ally=false;
	enemy=false;
	if( featureID == -1 ) // metal spot
	{
		type=1;
		searchRadius=cb->GetExtractorRadius()/2.0f;
		if( searchRadius < 16.0f )
			searchRadius = 16.0f;
	}
	else // geo spot
	{
		type=2;
		searchRadius=48.0f;
	}
	disApart=3;
//	unitLostFrame=-99999;
};

void sResourceSpot::CheckBlocked()
{
	for( map<int,sResourceSpotBO>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
	{
		float3 fBuildSite=cb->ClosestBuildSite(iB->second.udr->ud,location,searchRadius,disApart);
		if( !cb->CanBuildAt(iB->second.udr->ud,fBuildSite) && (unitID == -1 || enemy || ally ) )
			iB->second.RBBlocked=true;
		else
			iB->second.RBBlocked=false;
		iB->second.CheckBuild();
	}
}

void sResourceSpot::CheckRanked()
{
	for( map<int,sResourceSpotBO>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
	{
		if( unitID == -1 ||
			( type == 1 && iB->second.udr->ud->extractsMetal >= 1.5*unitUD->ud->extractsMetal ) ||
			( type == 2 && iB->second.udr->ud->metalCost >= 1.85*unitUD->ud->metalCost && (unitUD->ud->techLevel <= 0 || iB->second.udr->ud->techLevel > unitUD->ud->techLevel ) ) )
		{
			iB->second.RBRanked = false;
		}
		else
		{
			iB->second.RBRanked = true;
		}
		iB->second.CheckBuild();
	}
}

void sResourceSpot::SetRanged(bool inRange)
{
	for( map<int,sResourceSpotBO>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
	{
		iB->second.RBRanged = !inRange;
		iB->second.CheckBuild();
	}
}

cBuilderPlacement::cBuilderPlacement(IAICallback* callback, cRAI* global)
{
	G=global;
	l=global->l;
	cb=callback;

//	MapW=8.0*cb->GetMapWidth();
//	MapH=8.0*cb->GetMapHeight();
//	SectorXSize = cb->GetMapWidth()/32;
//	SectorZSize = cb->GetMapHeight()/32;
//	Sector = new BuildSector[SectorXSize*SectorZSize];

	if( int(G->Units.size()) == 0 )
		*l<<"\nERROR: G->Units";

	int UnitLimit = 500; //cb->GetValue();
	float3 StartLocation=cb->GetUnitPos(G->Units.begin()->first);
	int MetalSpotLimit = G->RG->KMM->NumSpotsFound;
	if( MetalSpotLimit > UnitLimit/4 )
		MetalSpotLimit = UnitLimit/4;
	if( MetalSpotLimit > 350 )
		MetalSpotLimit = 350;
	int GeoSpotLimit = int(G->RG->GeoSpot.size());
	if( GeoSpotLimit > UnitLimit/25 )
		GeoSpotLimit = UnitLimit/25;
	if( GeoSpotLimit > 50 )
		GeoSpotLimit = 50;
	ResourceSize=0;

	*l<<"\n    Metal Spots : "<<MetalSpotLimit;
	*l<<"\n      Geo Spots : "<<GeoSpotLimit;

	set<int> Seleted;
	for( int iM=0; iM < MetalSpotLimit; iM++ ) // Finding Nearest Metal Spots
	{
		int iBest=-1;
		float fBest=0;
		for( int iS=0; iS<int(G->RG->KMM->VectoredSpots.size()); iS++ )
		{
			if( Seleted.find(iS) == Seleted.end() )
			{
				if( iBest == -1 || StartLocation.distance2D(G->RG->KMM->VectoredSpots.at(iS)) < fBest )
				{
					iBest=iS;
					fBest=StartLocation.distance2D(G->RG->KMM->VectoredSpots.at(iS));
				}
			}
		}
		Seleted.insert(iBest);
		Resources[ResourceSize++] = new sResourceSpot(cb,G->RG->KMM->VectoredSpots.at(iBest));
	}

	Seleted.clear();
	for( int iG=MetalSpotLimit; iG < MetalSpotLimit+GeoSpotLimit; iG++ ) // Finding Nearest Geo Spots
	{
		int iBest=-1;
		float fBest=0;
		for(set<int>::iterator iS=G->RG->GeoSpot.begin(); iS!=G->RG->GeoSpot.end(); iS++)
		{
			if( Seleted.find(*iS) == Seleted.end() )
			{
				if( iBest == -1 || StartLocation.distance2D(cb->GetFeaturePos(*iS)) < fBest )
				{
					iBest=*iS;
					fBest=StartLocation.distance2D(cb->GetFeaturePos(*iS));
				}
			}
		}
		Seleted.insert(iBest);
		Resources[ResourceSize++] = new sResourceSpot(cb,cb->GetFeaturePos(iBest),iBest,cb->GetFeatureDef(iBest));
	}

	for( int iR=0; iR<ResourceSize; iR++ ) // Updating BuildOptions, Validating Resources
	{
		sRAIBuildList *BL;
		if( Resources[iR]->type==1 )
		{
			BL=G->UDH->BLMetalL;
			for( int i=0; i<BL->UDefSize; i++ )
			{
				sRAIUnitDef* udr=BL->UDef[i]->RUD;
				float3 BuildLoc=cb->ClosestBuildSite(udr->ud,Resources[iR]->location,Resources[iR]->searchRadius,0);
				if( cb->CanBuildAt(udr->ud,BuildLoc) )
				{
					Resources[iR]->BuildOptions.insert(irbPair(udr->ud->id,sResourceSpotBO(udr)));
				}
			}
		}
		else if( Resources[iR]->type==2 )
		{
			BL=G->UDH->BLEnergyL;
			for( map<int,sRAIUnitDef>::iterator iUD=G->UDH->UDR.begin(); iUD!=G->UDH->UDR.end(); iUD++ )
			{
				if( iUD->second.ud->needGeo && !iUD->second.Disabled && int(iUD->second.PrerequisiteOptions.size()) > 0 )
				{
					float3 BuildLoc=cb->ClosestBuildSite(iUD->second.ud,Resources[iR]->location,Resources[iR]->searchRadius,0);
					if( cb->CanBuildAt(iUD->second.ud,BuildLoc) )
					{
						Resources[iR]->BuildOptions.insert(irbPair(iUD->second.ud->id,sResourceSpotBO(&iUD->second)));
					}
				}
			}
		}

		if( int(Resources[iR]->BuildOptions.size()) == 0 ) // NOTE: This unfortunately occurs when the starting unit is a factory on top of a resource
		{
			*l<<"\nWARNING: "<<(Resources[iR]->type==1?"Metal":"Energy")<<" Resource located at (x"<<Resources[iR]->location.x;
			*l<<" z"<<Resources[iR]->location.z<<" y"<<Resources[iR]->location.y<<") can not be built at.";
			delete Resources[iR];
			Resources[iR] = Resources[ResourceSize-1];
			ResourceSize--;
			iR--;
		}
		else
			ResRemaining.insert(irPair(iR,Resources[iR]));
	}

	float Distance[400][400]; // required for Visual Studios
//	float Distance[ResourceSize][ResourceSize];

	for( int iR1=0; iR1<ResourceSize; iR1++ )
		for( int iR2=iR1+1; iR2<ResourceSize; iR2++ )
		{
			Distance[iR1][iR2] = 0;
			
//			*l<<"\n\n P1.x"<<Resources[iR1]->location.x<<" P1.y"<<Resources[iR1]->location.y<<" P1.z"<<Resources[iR1]->location.z;
//			*l<<"\n P2.x"<<Resources[iR2]->location.x<<" P2.y"<<Resources[iR2]->location.y<<" P2.z"<<Resources[iR2]->location.z;

			// distance is calculated in 50 segments to take slope in consideration
			for( int i=0; i<50; i++ )
			{
				float x = -0.02*(Resources[iR1]->location.x-Resources[iR2]->location.x);
				float z = -0.02*(Resources[iR1]->location.z-Resources[iR2]->location.z);
				float3 Pos1 = Resources[iR1]->location;
				Pos1.x += i*x;
				Pos1.z += i*z;
				float3 Pos2 = Resources[iR1]->location;
				Pos2.x += (i+1)*x;
				Pos2.z += (i+1)*z;

				Pos1.y = cb->GetElevation(Pos1.x,Pos1.z)*0.125*Pos1.distance2D(Pos2);
				Pos2.y = cb->GetElevation(Pos2.x,Pos2.z)*0.125*Pos1.distance2D(Pos2);

//				*l<<"\n x*)"<<Pos1.x<<" z*)"<<Pos1.z<<" y*)"<<Pos1.y;
//				*l<<"\n x)"<<Pos1.x-Pos2.x<<" z)"<<Pos1.z-Pos2.z<<" y)"<<Pos1.y-Pos2.y<<" Pos1.distance(Pos2)="<<Pos1.distance(Pos2);

				Distance[iR1][iR2] += Pos1.distance(Pos2);
			}
			Distance[iR2][iR1] = Distance[iR1][iR2];

//			*l<<"\n  Distance["<<iR1<<"]["<<iR2<<"] "<<Distance[iR1][iR2];
		}

	if( ResourceSize > 0 ) // First Link Algorithm
	{
		set<int> RSList1;
		set<int> RSList2;
		RSList1.insert(0);
		for( int i=1; i<ResourceSize; i++ )
			RSList2.insert(i);

		int R1;
		int R2;
		while( int(RSList2.size()) > 0 )
		{
			R1 = -1;
			for( set<int>::iterator iR1=RSList1.begin(); iR1!=RSList1.end(); iR1++ )
			{
				for( set<int>::iterator iR2=RSList2.begin(); iR2!=RSList2.end(); iR2++ )
				{
					if( R1 == -1 || Distance[R1][R2] > Distance[*iR1][*iR2] )
					{
						R1=*iR1;
						R2=*iR2;
					}
				}
			}
			RSList1.insert(R2);
			RSList2.erase(R2);
			Resources[R1]->Linked.insert(irPair(R2,Resources[R2]));
			Resources[R2]->Linked.insert(irPair(R1,Resources[R1]));
//			*l<<"\nDistance["<<R1<<"]["<<R2<<"] "<<Distance[R1][R2];
		}
	}

	for( int iR1=0; iR1<ResourceSize; iR1++ ) // Second Link Algorithm
	{
		if( (Resources[iR1]->Linked.size()) == 1 )
		{
			int bestI=-1;
			for( int iR2=0; iR2<ResourceSize; iR2++ )
			{
				if( iR2 != iR1 && Distance[iR1][iR2] < Distance[iR2][Resources[iR1]->Linked.begin()->first] && Distance[iR1][iR2] < 3*Distance[iR1][Resources[iR1]->Linked.begin()->first] && ( bestI == -1 || Distance[iR1][iR2] < Distance[iR1][bestI] ) )
				{
					bestI = iR2;
				}
			}
			if( bestI > -1 )
			{
				Resources[iR1]->Linked.insert(irPair(bestI,Resources[bestI]));
				Resources[bestI]->Linked.insert(irPair(iR1,Resources[iR1]));
/*
				if( cb->GetMyTeam() == 0 ) // Debug Lines
				{
				float3 Pos1=Resources[iR1]->location;
				float3 Pos2=Resources[bestI]->location;
				Pos1.y+=50.0;
				Pos2.y+=50.0;
				cb->CreateLineFigure(Pos1,Pos2,40,0,900000,0);
				}
*/
			}
		}
	}

	for( int iR=0; iR<ResourceSize; iR++ ) // Setting Linked Distance 2
	{
		for( map<int,sResourceSpot*>::iterator iRL=Resources[iR]->Linked.begin(); iRL!=Resources[iR]->Linked.end(); iRL++ )
		{
			if( Resources[iR]->LinkedD2.find(iRL->first) == Resources[iR]->LinkedD2.end() )
				Resources[iR]->LinkedD2.insert(irPair(iRL->first,Resources[iRL->first]));
			for( map<int,sResourceSpot*>::iterator iRL2=Resources[iRL->first]->Linked.begin(); iRL2!=Resources[iRL->first]->Linked.end(); iRL2++ )
			{
				if( iRL2->first != iR && Resources[iR]->LinkedD2.find(iRL2->first) == Resources[iR]->LinkedD2.end() )
					Resources[iR]->LinkedD2.insert(irPair(iRL2->first,Resources[iRL2->first]));
			}
		}
	}

	for( int iR=0; iR<ResourceSize; iR++ )
		Resources[iR]->SetRanged();
/*
	if( cb->GetMyTeam() == 0 )
	for( int iR=0; iR<ResourceSize; iR++ ) // Debug Lines
	{
		for( int iR2=iR+1; iR2<ResourceSize; iR2++ )
		{
			if( Resources[iR]->Linked.find(iR2) != Resources[iR]->Linked.end() )
			{
				float3 Pos1=Resources[iR]->location;
				float3 Pos2=Resources[iR2]->location;
				Pos1.y+=50.0;
				Pos2.y+=50.0;
				cb->CreateLineFigure(Pos1,Pos2,10,0,900000,0);
			}
		}

		*l<<"\n R("<<iR<<") type="<<Resources[iR]->type<<" L("<<Resources[iR]->Linked.size()<<"):";
		for( map<int,sResourceSpot*>::iterator RL=Resources[iR]->Linked.begin(); RL!=Resources[iR]->Linked.end(); RL++ )
			*l<<" "<<RL->first;
		*l<<"\t B("<<Resources[iR]->BuildOptions.size()<<"):";
		for( map<int,sResourceSpotBO>::iterator RS=Resources[iR]->BuildOptions.begin(); RS!=Resources[iR]->BuildOptions.end(); RS++ )
			*l<<" "<<RS->first;
	}
*/
}

cBuilderPlacement::~cBuilderPlacement()
{
	for( int i=0; i<ResourceSize; i++ )
		delete Resources[i];
//	delete [] Sector;
}

void cBuilderPlacement::UResourceCreated(int unit, UnitInfo *U)
{
	if( !NeedsResourceSpot(U->ud,U->mapBody) )
		return;

	if( U->ud->extractsMetal > 0.0f )
		UExtractor.insert(cRAI::iupPair(unit,U));
	else if( U->ud->needGeo )
		UGeoPlant.insert(cRAI::iupPair(unit,U));

	int iR=GetResourceIndex(unit,U->ud);
	if( iR == -1 )
	{
//		cb->SendTextMsg("TEST: Decomission: iR = -1",0);
//		G->Build->Decomission.insert(unit); // broken
		return;
	}

	if( Resources[iR]->unitID == -1 || !Resources[iR]->BuildOptions.find(U->ud->id)->second.RBRanked )
	{
		SetResourceOwner(iR,Resources[iR],unit,U->udr);
		U->RS = Resources[iR];
	}
	else if( !U->AIDisabled )
	{
//		cb->SendTextMsg("TEST: Decomission: ranked",0);
		G->Build->Decomission.insert(unit);
	}
}

void cBuilderPlacement::UResourceDestroyed(int unit, UnitInfo *U)
{
	if( !NeedsResourceSpot(U->ud,U->mapBody) )
		return;

	if( U->ud->extractsMetal > 0.0f )
		UExtractor.erase(unit);
	else if( U->ud->needGeo )
		UGeoPlant.erase(unit);

	if( U->RS != 0 && U->RS->unitID == unit )
	{
		for( int iR=0; iR<ResourceSize; iR++ )
		{
			if(	Resources[iR]->unitID == unit )
			{
				SetResourceOwner(iR,U->RS,-1);
				break;
			}
		}
	}
}

void cBuilderPlacement::EResourceEnterLOS(int enemy, EnemyInfo *E)
{
	if( !NeedsResourceSpot(E->ud,-1) )
		return;

	int iR=GetResourceIndex(enemy,E->ud);
	if( iR == -1 )
		return;

	if( Resources[iR]->unitID == -1 || !Resources[iR]->BuildOptions.find(E->ud->id)->second.RBRanked )
	{
		Resources[iR]->unitID=enemy;
		Resources[iR]->unitUD=E->udr;
		Resources[iR]->enemy = true;
		E->RS = Resources[iR];

		if( Resources[iR]->builderID > -1 && G->ValidateUnit(Resources[iR]->builderID) && int(cb->GetCurrentUnitCommands(Resources[iR]->builderID)->size())>0 && Resources[iR]->BuildOptions.find(-cb->GetCurrentUnitCommands(Resources[iR]->builderID)->begin()->id) != Resources[iR]->BuildOptions.end() )
		{
			if( Resources[iR]->builderUI->ud->canCapture )
			{
				Command c;
				c.id = CMD_CAPTURE;
				c.params.push_back(enemy);
				cb->GiveOrder(Resources[iR]->builderID, &c);
			}
			else if ( Resources[iR]->builderUI->ud->canReclaim )
			{
				Command c;
				c.id = CMD_RECLAIM;
				c.params.push_back(enemy);
				cb->GiveOrder(Resources[iR]->builderID, &c);
			}
		}
	}
}

bool cBuilderPlacement::CanBuildAtMapBody(float3 fBuildPos, const UnitDef* bd, int BuilderMapBody)
{
	int ConstructMapBody=G->GetCurrentMapBody(bd,fBuildPos);
	if( ConstructMapBody == -2 )
		return false;
	if( BuilderMapBody == -1 || ConstructMapBody == BuilderMapBody || BuilderMapBody == -2 ) // MapBody == -2 really should return false, but the only time i've known this to happen is TA mods - commander starts were they shouldnt
		return true;
	if( G->TM->MapB[BuilderMapBody]->Sector.find(G->TM->GetSector(fBuildPos)) == G->TM->MapB[BuilderMapBody]->Sector.end() )
		return false;
	return true;
}

bool cBuilderPlacement::NeedsResourceSpot(const UnitDef* bd, int ConstructMapBody)
{
	if( !bd->needGeo && bd->extractsMetal==0 )
		return false;
	if( ConstructMapBody < 0 ) // assume it should return true, it either can't be built here or it can be built anywhere
		return true;
	if( bd->extractsMetal>0 && G->TM->MapB[ConstructMapBody]->MetalMap )
		return false;
	return true;
}

sResourceSpot* cBuilderPlacement::FindResourceSpot(float3& pos, const UnitDef* bd, int BuilderMapBody)
{
//	*l<<"\nFindResourceSpot("<<bd->humanName<<","<<BuilderMapBody<<", x"<<pos.x<<" z"<<pos.z<<" y"<<pos.y<<")";
	if( !NeedsResourceSpot(bd,G->GetCurrentMapBody(bd,pos)) )
		return 0;

	UpdateAllyResources();
	map<int,sResourceSpot*>* RL;
	if( int(UExtractor.size()) == 0 && int(UGeoPlant.size()) == 0 && int(ResRemaining.size()) > 0 )
	{
//		*l<<"\n\n(No Resources) - Alternative Search";
		RL=&ResRemaining;
		CheckBlockedRList(RL);
	}
	else
		RL=&Resource;
	float fBest=0;
	int iBest=-1;
	for( map<int,sResourceSpot*>::iterator iR=RL->begin(); iR!=RL->end(); iR++ )
	{
		if( iR->second->builderID == -1 &&
			iR->second->BuildOptions.find(bd->id) != iR->second->BuildOptions.end() &&
			!iR->second->BuildOptions.find(bd->id)->second.RBBlocked &&
			!iR->second->BuildOptions.find(bd->id)->second.RBRanked)
		{
			int CMB = G->GetCurrentMapBody(bd,pos);
			if( G->TM->CanMoveToPos(BuilderMapBody,iR->second->location) || (!G->TM->MapB[BuilderMapBody]->Water && CMB >= 0 && G->TM->MapB[CMB]->Water) )
			{
				float fDis=pos.distance(iR->second->location);
				if( iBest==-1 || fDis < fBest )
				{
					iBest=iR->first;
					fBest=fDis;
				}
			}
		}
	}
	if( iBest == -1 )
	{
		*l<<"\nWARNING: Search for resource has failed: "<<bd->humanName;
		return 0;
	}

	return Resources[iBest];
}

float3 cBuilderPlacement::FindBuildPosition(sBuildQuarry *BQ)
{
	const UnitDef* bd = BQ->creationUD->ud;
	if( BQ->RS != 0 ) // Resource is set
		return cb->ClosestBuildSite(bd,BQ->RS->location,BQ->RS->searchRadius,BQ->RS->disApart);

	float3 Pos=cb->GetUnitPos(BQ->builderID);
	int mapBody = G->GetCurrentMapBody(BQ->creationUD->ud,Pos);
	if( NeedsResourceSpot(BQ->creationUD->ud,mapBody) )
	{
		Pos.x = -2;
		Pos.z = -2;
		Pos.y = -2;
		return Pos;
	}
	else if( FindWeaponPlacement(BQ->builderUI,Pos) )
	{
		Pos.x+=rand()%81-40;
		Pos.z+=rand()%81-40;
		G->CorrectPosition(&Pos);
	}
	else if( BQ->builderUI->ud->speed == 0.0 )
	{
		Pos.x+=rand()%int(1.8*BQ->builderUI->ud->buildDistance)-0.9*BQ->builderUI->ud->buildDistance;
		Pos.z+=rand()%int(1.8*BQ->builderUI->ud->buildDistance)-0.9*BQ->builderUI->ud->buildDistance;
		G->CorrectPosition(&Pos);
		float3 BuildPos = cb->ClosestBuildSite(bd,Pos,BQ->builderUI->ud->buildDistance,5);
		if( BuildPos.x <= 0 && BuildPos.y <= 0 && BuildPos.z <= 0 )
			BuildPos = cb->ClosestBuildSite(bd,cb->GetUnitPos(BQ->builderID),BQ->builderUI->ud->buildDistance+25,1);

//		if( cb->CanBuildAt(BQ->builderUI->ud,cb->GetUnitPos(BQ->builderID),cb->GetBuildingFacing(BQ->builderID)) )
		return BuildPos;
	}
	else
	{
		Pos.x+=rand()%int(0.9*BQ->builderUI->ud->buildDistance)-0.45*BQ->builderUI->ud->buildDistance;
		Pos.z+=rand()%int(0.9*BQ->builderUI->ud->buildDistance)-0.45*BQ->builderUI->ud->buildDistance;
		G->CorrectPosition(&Pos);
	}
	if( !CanBuildAtMapBody(Pos,bd,BQ->builderUI->mapBody) )//&& bd->extractsMetal == 0.0 && !bd->needGeo )
	{
		Pos=G->TM->Sector[G->TM->GetSector(Pos)].AltSector->Pos;
	}

	float fSearchRadius=1000.0f;
	int iDisApart=10;
	int iMB=G->GetCurrentMapBody(bd,Pos);
	if( iMB>=0 && G->TM->MapB[iMB]->Water )
		iDisApart=15;
	else if( bd->speed > 0 )
		iDisApart=5;
	else if( int(bd->buildOptions.size())>0 )
		iDisApart=15;
	if( iDisApart < int(bd->kamikazeDist/8.0) )
		iDisApart = int(bd->kamikazeDist/8.0);

	float3 bpos = cb->ClosestBuildSite(bd,Pos,fSearchRadius,iDisApart);
	if( bpos.x == -1 )
	{
		bpos = cb->ClosestBuildSite(bd,Pos,2500.0f,iDisApart);
		if( bpos.x == -1 )
			return Pos;
	}
	return bpos;
}

bool cBuilderPlacement::FindWeaponPlacement(UnitInfo *U, float3& pos)
{
	if( U->BuildQ->creationUD->WeaponGuardRange == 0 )
		return false;

	if( U->BuildQ->creationUD->ud->minWaterDepth < 0 && U->BuildQ->creationUD->WeaponSeaEff.BestRange > 0 )
	{
		G->CorrectPosition(&pos);
		MapSector* MS = &G->TM->Sector[G->TM->GetSector(pos)];
		if( MS->Land && MS->AltSector != 0 )
			pos = MS->AltSector->Pos;
		return true;
	}

	int BID = -1;
	for(map<int,UnitInfo*>::iterator i=G->UImmobile.begin(); i!=G->UImmobile.end(); i++ )
	{
		if( i->second->udr->WeaponGuardRange == 0 && int(i->second->UDefences.size()) == 0 && CanBuildAtMapBody(cb->GetUnitPos(i->first),U->BuildQ->creationUD->ud,U->mapBody) )
		{
			if( BID == -1 || pos.distance2D(cb->GetUnitPos(i->first)) < pos.distance2D(cb->GetUnitPos(BID)) )
				BID = i->first;
		}
	}

	if( BID > 0 )
	{
		pos = cb->GetUnitPos(BID);
		return true;
	}

	return false;
}

void cBuilderPlacement::CheckBlockedRList( map<int,sResourceSpot*> *RL )
{
	if( RL == 0 )
		RL = &Resource;
	set<int> del;
	for( map<int,sResourceSpot*>::iterator iR=RL->begin(); iR!=RL->end(); iR++ )
	{
		if( iR->second->unitID == -1 )
			iR->second->CheckBlocked();
		else if( cb->GetUnitHealth(iR->second->unitID) <= 0 )
		{
			del.insert(iR->first);
		}
	}
	while( int(del.size()) > 0 )
	{
		if( RL->find(*del.begin()) != RL->end() )
		{
			sResourceSpot* RS = RL->find(*del.begin())->second;
			SetResourceOwner(*del.begin(),RS,-1);
			RS->CheckBlocked();
		}
		del.erase(del.begin());
	}
}

void cBuilderPlacement::UpdateAllyResources()
{
	int *Units = new int[5000];
	int UnitsSize;
	try
	{
		UnitsSize = cb->GetFriendlyUnits(Units);
	}
	catch(...)
	{
		cb->SendTextMsg("ERROR: more than 5000 Friendly Units, increase limit.",0);
		*l<<"\n\nERROR: more than 5000 Friendly Units, increase limit.\n";
		delete [] Units;
		return;
	}

	for( int iU=0; iU<UnitsSize; iU++ )
	{
		const UnitDef* ud=cb->GetUnitDef(Units[iU]);
		if( !NeedsResourceSpot(ud,-1) || G->Units.find(Units[iU]) != G->Units.end() )
		{
			Units[iU--]=Units[--UnitsSize];
		}
		else
		{
			int iR=GetResourceIndex(Units[iU],ud);
			if( iR > -1 )
			{
				if( Resources[iR]->unitID == -1 || !Resources[iR]->BuildOptions.find(ud->id)->second.RBRanked )
				{
					SetResourceOwner(iR,Resources[iR],Units[iU],&G->UDH->UDR.find(ud->id)->second);
				}
			}
		}
	}

	delete [] Units;
}

int cBuilderPlacement::GetResourceIndex(const int &unit, const UnitDef* ud)
{
	int iBest=-1;
	float fBest;
	for( int iR=0; iR<ResourceSize; iR++ )
	{
		if(	Resources[iR]->unitID == unit )
			return iR;
		if( (Resources[iR]->type == 1 && ud->extractsMetal>0.0 ) ||
			(Resources[iR]->type == 2 && ud->needGeo ) )
		{
			float dis=Resources[iR]->location.distance2D(cb->GetUnitPos(unit));
			if( dis <= Resources[iR]->searchRadius && Resources[iR]->BuildOptions.find(ud->id) != Resources[iR]->BuildOptions.end() && (iBest==-1 || dis < fBest) )
			{
				iBest = iR;
				fBest = dis;
			}
		}
	}
	return iBest;
}

void cBuilderPlacement::SetResourceOwner(int RSindex, sResourceSpot* RS, int unit, sRAIUnitDef* udr)
{
	if( int(Resource.size()) == 0 && unit >= 0 )
		for( int iR=0; iR<ResourceSize; iR++ )
			Resources[iR]->SetRanged(false);

/*	*l<<"\n\nSetResourceOwner("<<RSindex<<",_,"<<unit<<",{"<<(udr==0?"0":udr->ud->humanName)<<"})";
	*l<<"\n - Resources Available ("<<Resource.size()<<"):";
	for( map<int,sResourceSpot*>::iterator iR=Resource.begin(); iR!=Resource.end(); iR++ )
		*l<<" "<<iR->first;
*/
	int oldunit = RS->unitID;
	RS->unitID=unit;
	RS->unitUD=udr;
	RS->enemy = false;
	if( unit >= 0 && cb->GetUnitAllyTeam(unit) != cb->GetMyAllyTeam() )
		RS->ally = true;
	else
		RS->ally = false;
	sRAIBuildList *BL;
	if( RS->type == 1 )
		BL=G->UDH->BLMetalL;
	else
		BL=G->UDH->BLEnergyL;
	if( unit >= 0 && oldunit == -1 )
	{
		if( Resource.find( RSindex ) == Resource.end() )
		{
			RS->SetRanged();
			Resource.insert(irPair(RSindex,RS));
			ResRemaining.erase(RSindex);
		}
		for( map<int,sResourceSpot*>::iterator iR=RS->Linked.begin(); iR!=RS->Linked.end(); iR++ )
		{
			if( Resource.find(iR->first) == Resource.end() )
			{
				iR->second->SetRanged();
				Resource.insert(irPair(iR->first,iR->second));
				ResRemaining.erase(iR->first);
			}
			for( map<int,sResourceSpot*>::iterator iR2=iR->second->Linked.begin(); iR2!=iR->second->Linked.end(); iR2++ )
			{
				if( Resource.find(iR2->first) == Resource.end() )
				{
					iR2->second->SetRanged();
					Resource.insert(irPair(iR2->first,iR2->second));
					ResRemaining.erase(iR2->first);
				}
			}
		}
	}
	else if( unit == -1 )
	{
		set<int> RSL;
		RSL.insert(RSindex);
		for( map<int,sResourceSpot*>::iterator iR=RS->LinkedD2.begin(); iR!=RS->LinkedD2.end(); iR++ )
		{
			if( Resource.find(iR->first) != Resource.end() && ( iR->second->unitID == -1 || iR->second->enemy ) )
				RSL.insert(iR->first);
		}
		bool found;
		while( int(RSL.size()) > 0 )
		{
			found=false;
			for( map<int,sResourceSpot*>::iterator iR=Resources[*RSL.begin()]->LinkedD2.begin(); iR!=Resources[*RSL.begin()]->LinkedD2.end(); iR++ )
			{
				if( Resources[iR->first]->unitID > -1 && !Resources[iR->first]->enemy )
				{
					found = true;
					break;
				}
			}

			if( !found )
			{
				Resources[*RSL.begin()]->SetRanged(false);
				ResRemaining.insert(irPair(*RSL.begin(),Resources[*RSL.begin()]));
				Resource.erase(*RSL.begin());
			}
			RSL.erase(RSL.begin());
		}
	}
	RS->CheckRanked();
	if( int(Resource.size()) == 0 )
		for( int iR=0; iR<ResourceSize; iR++ )
			Resources[iR]->SetRanged(true);
/*
	*l<<"\n - Resources Available ("<<Resource.size()<<"):";
	for( map<int,sResourceSpot*>::iterator iR=Resource.begin(); iR!=Resource.end(); iR++ )
		*l<<" "<<iR->first;

	BL=G->UDH->BLMetalL;
	for( int i=0; i<BL->UDefSize; i++ )
	{
		sRAIUnitDef* udrTemp=BL->UDef[i]->RUD;
		if( !udrTemp->Disabled )
		{
			*l<<"\n"+udrTemp->ud->humanName;
			if( udrTemp->RBPrereq )
				*l<<"(RBPrereq)";
			if( udrTemp->RBCost )
				*l<<"(RBCost)";
			if( udrTemp->RBUnitLimit )
				*l<<"(RBUnitLimit)";
			*l<<": "<<udrTemp->UnitsActive.size()<<"+"<<udrTemp->UnitConstructs<<"/"<<udrTemp->UnitLimit[0];
		}
	}
	BL=G->UDH->BLEnergyL;
	for( int i=0; i<BL->UDefSize; i++ )
	{
		sRAIUnitDef* udrTemp=BL->UDef[i]->RUD;
		if( !udrTemp->Disabled )
		{
			*l<<"\n"+udrTemp->ud->humanName;
			if( udrTemp->RBPrereq )
				*l<<"(RBPrereq)";
			if( udrTemp->RBCost )
				*l<<"(RBCost)";
			if( udrTemp->RBUnitLimit )
				*l<<"(RBUnitLimit)";
			*l<<": "<<udrTemp->UnitsActive.size()<<"+"<<udrTemp->UnitConstructs<<"/"<<udrTemp->UnitLimit[0];
		}
	}

	*l<<"\n Empty Resources Available:";
	for( map<int,sResourceSpot*>::iterator iR=Resource.begin(); iR!=Resource.end(); iR++ )
		if( iR->second->unitID == -1 || iR->second->enemy )
			*l<<" "<<iR->first;
	*l<<"\n\n";
*/
}
