#include "BuilderPlacement.h"
#include "LegacyCpp/UnitDef.h"
#include "LegacyCpp/MoveData.h"
#include "LegacyCpp/CommandQueue.h"

#include <set>
#include <time.h>

ResourceSiteExtBO::ResourceSiteExtBO(sRAIUnitDef* UDR )
{
	udr=UDR;
	CanBuild=false;
	RBRanged=true;
	RBBlocked=false;
	RBRanked=false;
}

void ResourceSiteExtBO::CheckBuild()
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

ResourceSiteExt::ResourceSiteExt(ResourceSite *RSite, IAICallback* callback)
{
	cb=callback;
	S=RSite;
	unitID=-1;
	builderID=-1;
	ally=false;
	enemy=false;
	unitUD=0;
	builderUI=0;
	if( S->featureID == -1 ) // Metal-Site
	{
		searchRadius=cb->GetExtractorRadius()/2.0f;
		if( searchRadius < 16.0f )
			searchRadius = 16.0f;
	}
	else // Geo-Site
	{
		searchRadius=48.0f;
	}
	disApart=3;
}

void ResourceSiteExt::CheckBlocked()
{
	for( map<int,ResourceSiteExtBO>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); ++iB )
	{
		float3 fBuildSite=cb->ClosestBuildSite(iB->second.udr->ud,S->position,searchRadius,disApart);
		if( !cb->CanBuildAt(iB->second.udr->ud,fBuildSite) && (unitID == -1 || enemy || ally ) )
			iB->second.RBBlocked=true;
		else
			iB->second.RBBlocked=false;
		iB->second.CheckBuild();
	}
}

void ResourceSiteExt::CheckRanked()
{
	for( map<int,ResourceSiteExtBO>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); ++iB )
	{
		if( unitID == -1 ||
			( S->type == 0 && iB->second.udr->ud->extractsMetal >= 1.5*unitUD->ud->extractsMetal ) ||
			( S->type == 1 && iB->second.udr->ud->metalCost >= 1.85*unitUD->ud->metalCost && (unitUD->ud->techLevel <= 0 || iB->second.udr->ud->techLevel > unitUD->ud->techLevel ) ) )
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

void ResourceSiteExt::SetRanged(bool inRange)
{
	for( map<int,ResourceSiteExtBO>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); ++iB )
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
	*l<<"\nLoading Build-Placement ...";
	const double SecondAlgorithmTimeLimit = 2.5; // seconds per RAI

	//double clockStart = clock();

	if( int(G->Units.size()) == 0 )
		*l<<"\nERROR: G->Units";

	int unitLimit;
	cb->GetValue(AIVAL_UNIT_LIMIT,&unitLimit);
	if( unitLimit > 500 ) // Can't think of a reason to ever have this higher, but resource linking could take really long
		unitLimit = 500;
	float3 StartPosition=cb->GetUnitPos(G->Units.begin()->first);
	int MetalSiteLimit = G->RM->RSize[0];
	if( MetalSiteLimit > unitLimit/6 )
		MetalSiteLimit = unitLimit/6;
	int GeoSiteLimit = G->RM->RSize[1];
	if( GeoSiteLimit > unitLimit/6 )
		GeoSiteLimit = unitLimit/6;
	Resources = new ResourceSiteExt*[MetalSiteLimit+GeoSiteLimit];
	ResourceSize = 0;

	*l<<"\n Metal-Site limit : "<<MetalSiteLimit;
	*l<<"\n Geo-Site limit : "<<GeoSiteLimit;

	// Finding and using the Nearest Metal-Sites
	set<int> Seleted;
	for( int iM=0; iM<MetalSiteLimit && iM<G->RM->RSize[0]; iM++ )
	{
		int iBest=-1;
		float fBest=0.0f;
		for( int iR=0; iR<G->RM->RSize[0]; iR++ )
		{	// Cycles through only metal-sites
			if( Seleted.find(iR) == Seleted.end() )
				if( iBest == -1 || StartPosition.distance2D(G->RM->R[0][iR]->position) < fBest )
				{
					iBest=iR;
					fBest=StartPosition.distance2D(G->RM->R[0][iR]->position);
				}
		}
		Seleted.insert(iBest);
		Resources[ResourceSize++] = new ResourceSiteExt(G->RM->R[0][iBest],cb);
	}

	// Finding and using the Nearest Geo-Sites
	Seleted.clear();
	for( int iG=0; iG<GeoSiteLimit && iG<G->RM->RSize[1]; iG++ )
	{
		int iBest=-1;
		float fBest=0.0f;
		for( int iR=0; iR<G->RM->RSize[1]; iR++ )
		{	// Cycles through only geo-sites
			if( Seleted.find(iR) == Seleted.end() )
				if( iBest == -1 || StartPosition.distance2D(G->RM->R[1][iR]->position) < fBest )
				{
					iBest=iR;
					fBest=StartPosition.distance2D(G->RM->R[1][iR]->position);
				}
		}
		Seleted.insert(iBest);
		Resources[ResourceSize++] = new ResourceSiteExt(G->RM->R[1][iBest],cb);
	}

	// Updating BuildOptions and Resources, some units may have been disabled
	typedef pair<int,ResourceSiteExtBO> irbPair;
	typedef pair<ResourceSite*,ResourceSiteDistance> rrPair;
	for( int iR=0; iR<ResourceSize; iR++ )
	{
		if( Resources[iR]->S->type == 0 )
		{
			sRAIBuildList *BL = G->UDH->BLMetalL;
			for( int i=0; i<BL->UDefSize; i++ )
			{
				sRAIUnitDef* udr=BL->UDef[i]->RUD;
				if( Resources[iR]->S->options.find(udr->ud->id) != Resources[iR]->S->options.end() )
					Resources[iR]->BuildOptions.insert(irbPair(udr->ud->id,ResourceSiteExtBO(udr)));
			}
		}
		else if( Resources[iR]->S->type == 1 )
		{
			// Units using Geo-Sites can be found in any number of build lists
			for( map<int,sRAIUnitDef>::iterator iUD=G->UDH->UDR.begin(); iUD!=G->UDH->UDR.end(); ++iUD )
			{
				if( iUD->second.ud->needGeo && !iUD->second.Disabled && int(iUD->second.PrerequisiteOptions.size()) > 0 )
				{
					if( Resources[iR]->S->options.find(iUD->second.ud->id) != Resources[iR]->S->options.end() )
						Resources[iR]->BuildOptions.insert(irbPair(iUD->second.ud->id,ResourceSiteExtBO(&iUD->second)));
				}
			}
		}

		if( int(Resources[iR]->S->siteDistance.size()) == 0 || int(Resources[iR]->S->siteDistance.size()) < G->RM->RSize[0]+G->RM->RSize[1] )
		{
			// Cheap distance calculations, although this is still too much to calculate for all map resources
			for( int iR2=0; iR2<ResourceSize; iR2++ )
				if( Resources[iR]->S->siteDistance.find(Resources[iR2]->S) == Resources[iR]->S->siteDistance.end() )
					Resources[iR]->S->siteDistance.insert(rrPair(Resources[iR2]->S,ResourceSiteDistance(Resources[iR]->S->position.distance2D(Resources[iR2]->S->position))));
		}

		// Old Code, Unneeded?  Caused by: the old RAI style's handling of a Metal-Map
		if( int(Resources[iR]->BuildOptions.size()) == 0 )
		{
			float3* p = &Resources[iR]->S->position;
			*l<<"\nERROR: "<<(Resources[iR]->S->type==0?"Metal":"Energy")<<" Resource located at (x"<<p->x;
			*l<<" z"<<p->z<<" y"<<p->y<<") can not be built at.";
			*l<<" size="<<Resources[iR]->S->options.size();
			delete Resources[iR];
			Resources[iR] = Resources[ResourceSize-1];
			ResourceSize--;
			iR--;
		}
		else
			RSRemaining.insert(irPair(iR,Resources[iR]));
	}
	*l<<"\n Resources in use : "<<ResourceSize;

	if( ResourceSize == 0 )
		return;

	// Read all movetypes
	int arraySize = G->TM->mobileType.size();
	basicArray<TerrainMapMobileType*> MobileTypes(arraySize);
	for( list<TerrainMapMobileType>::iterator iM=G->TM->mobileType.begin(); iM!=G->TM->mobileType.end(); ++iM )
		MobileTypes.push_back(&*iM);

	// Sort them by worst to best, remove unusable movetypes
	for( int iM=0; iM<MobileTypes.size()-1; iM++)
	{
		if( (*MobileTypes[iM])->areaLargest == 0 )
		{
			MobileTypes.removeE(iM);
			iM--;
		}
		else if( (*MobileTypes[iM+1])->areaLargest == 0 )
		{
			MobileTypes.removeE(iM+1);
			iM--;
		}
		else if( (*MobileTypes[iM])->areaLargest->percentOfMap > (*MobileTypes[iM+1])->areaLargest->percentOfMap ||
				((*MobileTypes[iM])->areaLargest->percentOfMap == (*MobileTypes[iM+1])->areaLargest->percentOfMap && (*MobileTypes[iM])->maxSlope > (*MobileTypes[iM+1])->maxSlope) )
		{
//*l<<"\n  MT("<<(*MobileTypes[iM])->MD->name<<" "<<iM<<"<->"<<iM+1<<" "<<(*MobileTypes[iM+1])->MD->name<<")";
			MobileTypes.swap(iM,iM+1);
			iM--;
			if( iM != -1 )
				iM --;
		}
	}

	TerrainMapMobileType* MT=NULL;
//	for( MobileTypes.begin(); MobileTypes.nextE(MT); )
//		*l<<"\n  MoveType="<<MT->MD->name<<" pathType="<<MT->MD->pathType;

	// First Link Algorithm
	double linkStartClock = clock();
	TerrainMapArea* Area;
	for( int iR1=0; iR1<ResourceSize; iR1++ )
		for( int iR2=iR1+1; iR2<ResourceSize; iR2++ )
			for( MobileTypes.begin(); MobileTypes.nextE(MT); )
				if( (Area = MT->sector[G->TM->GetSectorIndex(Resources[iR1]->S->position)].area) == 0 ||
					Area != MT->sector[G->TM->GetSectorIndex(Resources[iR2]->S->position)].area )
				{
					Resources[iR1]->S->siteDistance.find(Resources[iR2]->S)->second.distance.insert(ifPair(MT->MD->pathType,-2.0));
					Resources[iR2]->S->siteDistance.find(Resources[iR1]->S)->second.distance.insert(ifPair(MT->MD->pathType,-2.0));
				}
	set<int> RSList1; // sites that are so far connected
	set<int> RSList2; // sites that have yet to be connected
	RSList1.insert(0);
	for( int i=1; i<ResourceSize; i++ )
		RSList2.insert(i);
	int iM = 0; // current moveType index
	while( int(RSList2.size()) > 0 )
	{
		if( iM >= MobileTypes.size() )
			iM = -1; // None of the moveTypes worked, just ignore terrain this time around
		int bestI1;
		int bestI2;
		int pathType;
		float dis; // temp
		bestI1 = -1;
		if( iM == -1 )
			pathType = -1;
		else
		{
			MT = (*MobileTypes[iM]);
			pathType = MT->MD->pathType;
		}
//*l<<"\n RSL2size="<<RSList2.size()<<" iM="<<iM;
		for( set<int>::iterator iR1=RSList1.begin(); iR1!=RSList1.end(); ++iR1 )
			if( iM == -1 || MT->sector[G->TM->GetSectorIndex(Resources[*iR1]->S->position)].area != 0 )
			{
//*l<<" r1="<<*iR1;
				for( set<int>::iterator iR2=RSList2.begin(); iR2!=RSList2.end(); ++iR2 )
				{
//*l<<" r2="<<*iR2;
					dis = Resources[*iR1]->S->GetResourceDistance(Resources[*iR2]->S,pathType);
					if( dis > 0 && (bestI1 == -1 || Resources[bestI1]->S->GetResourceDistance(Resources[bestI2]->S,pathType) > dis) )
					{
						bestI1=*iR1;
						bestI2=*iR2;
					}
				}
			}
		if( bestI1 == -1 )
		{	// Nothing selected, try again with a different moveType
//*l<<"m";
			iM++;
		}
		else if( iM >= 0 && Resources[bestI1]->S->siteDistance.find(Resources[bestI2]->S)->second.bestDistance == 0 )
		{	// The best choice distance has yet to be calculated
//*l<<"r";
			FindResourceDistance(Resources[bestI1]->S,Resources[bestI2]->S,MT->MD->pathType);
		}
		else
		{
//*l<<"s";
			RSList1.insert(bestI2);
			RSList2.erase(bestI2);
			Resources[bestI1]->Linked.insert(irPair(bestI2,Resources[bestI2]));
			Resources[bestI2]->Linked.insert(irPair(bestI1,Resources[bestI1]));
			if( iM == -1 )
			{
				Resources[bestI1]->S->siteDistance.find(Resources[bestI2]->S)->second.bestDistance = &Resources[bestI1]->S->siteDistance.find(Resources[bestI2]->S)->second.minDistance;
				Resources[bestI2]->S->siteDistance.find(Resources[bestI1]->S)->second.bestDistance = &Resources[bestI2]->S->siteDistance.find(Resources[bestI1]->S)->second.minDistance;
			}
			iM = 0;
		}
	}
	*l<<"\n First Link Algorithm Loading Time: "<<(clock()-linkStartClock)/CLOCKS_PER_SEC<<"s";

	// Second Link Algorithm
	linkStartClock = clock();
	float *MTPenalty; // the moveData*->pathType is the index to MTPenalty
	int MTPenaltySize = 0;
	for( MobileTypes.begin(); MobileTypes.nextE(MT); )
	{
		if( MTPenaltySize < MT->MD->pathType+1 )
			MTPenaltySize = MT->MD->pathType+1;
	}
	MTPenalty = new float[MTPenaltySize+1]; // the last index is for the flying movetype
	for( int iMP=0; iMP<MTPenaltySize; iMP++)
		MTPenalty[iMP] = -1.0;
	int linkCount = 0;
	for( MobileTypes.begin(); MobileTypes.nextE(MT); )
//	for( vector<TerrainMapMobileType*>::iterator iMT=MobileTypes.begin(); iMT!=MobileTypes.end(); iMT++ )
	{
		MTPenalty[MT->MD->pathType] = linkCount;

		for( int iR=0; iR<ResourceSize; iR++ )
			for(map<int,ResourceSiteExt*>::iterator iRL = Resources[iR]->Linked.begin(); iRL != Resources[iR]->Linked.end(); ++iRL )
				if( iRL->second->S->siteDistance.find(Resources[iR]->S)->second.bestPathType == MT->MD->pathType )
					linkCount++;
	}
	for( int iMP=0; iMP<MTPenaltySize; iMP++)
		if( MTPenalty[iMP] >= 0.0 )
		{
//*l<<"\n "<<MTPenalty[iMP]<<"/"<<linkCount;
			MTPenalty[iMP] = linkCount/(1+linkCount*1.10-MTPenalty[iMP]);
			if( MTPenalty[iMP] < 1.0 )
				MTPenalty[iMP] = 1.0;
			else if( MTPenalty[iMP] > 5.0 )
				MTPenalty[iMP] = 5.0;
//*l<<" MTPenalty["<<iMP<<"]="<<MTPenalty[iMP];
		}
	MTPenalty[MTPenaltySize] = 20.0;

	int **RMT = new int*[ResourceSize]; // Resource MoveType, 2 dimensional array
	float **RD = new float*[ResourceSize]; // Resource Distances, Local Copy, 2 dimensional array
	ResourceSiteDistance* RSD;
	for( int iR1=0; iR1<ResourceSize; iR1++ )
	{
		RMT[iR1] = new int[ResourceSize];
		RD[iR1] = new float[ResourceSize];
		for( int iR2=0; iR2<ResourceSize; iR2++ )
			RD[iR1][iR2] = -1.0;
	}
	for( int iR1=0; iR1<ResourceSize; iR1++ )
		for( int iR2=iR1+1; iR2<ResourceSize; iR2++ )
		{
			RMT[iR1][iR2] = MTPenaltySize;
			RMT[iR2][iR1] = MTPenaltySize;
			RSD = &Resources[iR1]->S->siteDistance.find(Resources[iR2]->S)->second;
			if( RSD->bestDistance != 0 )
			{
				if( RSD->bestPathType >= 0 )
				{
					RMT[iR1][iR2] = RSD->bestPathType;
					RMT[iR2][iR1] = RSD->bestPathType;
				}
			}
			else
				for( MobileTypes.begin(); MobileTypes.nextE(MT); )
					if( RSD->distance.find(MT->MD->pathType) == RSD->distance.end() )
					{
						RMT[iR1][iR2] = MT->MD->pathType;
						RMT[iR2][iR1] = MT->MD->pathType;
						break;
					}
		}

	basicArray<ResourceLinkInfo> RLIList(ResourceSize);
	ResourceLinkInfo *RLI;
	for( int iR=0; iR<ResourceSize; iR++ )
		if( Resources[iR]->Linked.size() < 3 )
		{
			RLI = RLIList.push_back();
			RLI->index = iR;
			RLI->bestI = -1;
			RLI->restrictedR.insert(iR);
			for(map<int,ResourceSiteExt*>::iterator iRL = Resources[iR]->Linked.begin(); iRL != Resources[iR]->Linked.end(); ++iRL )
				RLI->restrictedR.insert(iRL->first);
		}
//	*l<<"\n   Second Link Loading Time 0: "<<(clock()-linkStartClock)/(double)CLOCKS_PER_SEC<<"s";

//	double linkClock1 = 0;
//	double linkClock2 = 0;
//	double linkClock3 = 0;
//	double linkClock4 = 0;
//	double linkClockTemp;
	while( RLIList.elementSize > 0 && (clock()-linkStartClock)/(double)CLOCKS_PER_SEC <= SecondAlgorithmTimeLimit )
	{
//*l<<"\n";
		// Cycle through RLIList to find the closest resource pair available (with consideration to other factors)
		bool resetLink,acceptLink;
		ResourceLinkInfo *bestRLI = 0;
//		linkClockTemp = clock();
		for( RLIList.begin(); RLIList.nextE(RLI); )
		{
			if( RLI->bestI == -2 ) // The best link was found & rejected in latter code
				RLIList.removeE();
			else
			{
				if( RLI->bestI == -1 ) // has not been found yet
				{
					for( int iR2=0; iR2<ResourceSize; iR2++ )
						if( RLI->restrictedR.find(iR2) == RLI->restrictedR.end() )
						{
//*l<<"\n  r1="<<RLI->index<<" r2="<<iR2;
							if( RD[RLI->index][iR2] < 0.0 )
							{
								RSD = &Resources[RLI->index]->S->siteDistance.find(Resources[iR2]->S)->second;
//*l<<" BPT:"<<RSD->bestPathType;
								RD[RLI->index][iR2] = Resources[RLI->index]->S->GetResourceDistance(Resources[iR2]->S,RSD->bestPathType);
//*l<<" *:"<<RD[RLI->index][iR2];
								RD[RLI->index][iR2] *= MTPenalty[RMT[RLI->index][iR2]];
								RD[iR2][RLI->index] = RD[RLI->index][iR2];
							}
//*l<<" d(1-2):"<<RD[RLI->index][iR2];

							if( RLI->bestI < 0 || RD[RLI->index][iR2] < RD[RLI->index][RLI->bestI] )
							{
								acceptLink = true;
								for(map<int,ResourceSiteExt*>::iterator iRL1 = Resources[RLI->index]->Linked.begin(); iRL1 != Resources[RLI->index]->Linked.end(); ++iRL1 )
								{
									RSD = &Resources[iR2]->S->siteDistance.find(iRL1->second->S)->second;
									if( RSD->bestDistance != 0 )
									{
										if( RD[iR2][iRL1->first] < 0.0 )
										{
											RD[iR2][iRL1->first] = Resources[iR2]->S->GetResourceDistance(iRL1->second->S,RSD->bestPathType) * MTPenalty[RMT[iR2][iRL1->first]];
											RD[iRL1->first][iR2] = RD[iR2][iRL1->first];
										}
										if( RD[RLI->index][iRL1->first] < 0.0 )
										{
											RD[RLI->index][iRL1->first] = Resources[RLI->index]->S->GetResourceDistance(iRL1->second->S,Resources[RLI->index]->S->siteDistance.find(iRL1->second->S)->second.bestPathType) * MTPenalty[RMT[RLI->index][iRL1->first]];
											RD[iRL1->first][RLI->index] = RD[RLI->index][iRL1->first];
										}
//*l<<" d(L1-2):"<<RD[iR2][iRL1->first];
//*l<<"+"<<0.35*RD[RLI->index][iRL1->first];
										if( RD[iR2][iRL1->first]+0.35*RD[RLI->index][iRL1->first] < RD[RLI->index][iR2] )
										{
//*l<<" rejected("<<iRL1->first<<")";
											acceptLink = false;
											break;
										}
									}
								}

								if( acceptLink )
								{
//*l<<" *acceptLink*";
									RLI->bestI = iR2;
								}
							}
						}
				}
//if( RLI->bestI >= 0 ) *l<<"\n  dis["<<RLI->index<<"]["<<RLI->bestI<<"]="<<RD[RLI->index][RLI->bestI];
				if( RLI->bestI == -1 )
					RLIList.removeE();
				else if( bestRLI == 0 || RD[RLI->index][RLI->bestI] < RD[bestRLI->index][bestRLI->bestI] )
					bestRLI = RLI;
			}
		}
//		linkClock1 += clock()-linkClockTemp;

		if( bestRLI == 0 )
			break; // the loop would have ended anyway, this just jumps over the next area of code

//		linkClockTemp = clock();
		resetLink = false;
		RSD = &Resources[bestRLI->index]->S->siteDistance.find(Resources[bestRLI->bestI]->S)->second;
		if( RSD->bestDistance == 0 )
		{	// The best choice distance has yet to be calculated
//*l<<" r-1-2";
			for( MobileTypes.begin(); RSD->bestDistance == 0 && MobileTypes.nextE(MT); )
				if( RSD->distance.find(MT->MD->pathType) == RSD->distance.end() )
				{
					FindResourceDistance(Resources[bestRLI->index]->S,Resources[bestRLI->bestI]->S,MT->MD->pathType);
					RMT[bestRLI->index][bestRLI->bestI] = RSD->bestPathType;
					RMT[bestRLI->bestI][bestRLI->index] = RSD->bestPathType;
				}
			if( RSD->bestDistance == 0 )
			{
				RSD->bestDistance = &RSD->minDistance;
				RSD = &Resources[bestRLI->bestI]->S->siteDistance.find(Resources[bestRLI->index]->S)->second;
				RSD->bestDistance = &RSD->minDistance;
				RMT[bestRLI->index][bestRLI->bestI] = MTPenaltySize;
				RMT[bestRLI->bestI][bestRLI->index] = MTPenaltySize;
			}
			RD[bestRLI->index][bestRLI->bestI] = -1.0;
			RD[bestRLI->bestI][bestRLI->index] = -1.0;
			resetLink = true;
		}
//		linkClock2 += clock()-linkClockTemp;
//		linkClockTemp = clock();
		for(map<int,ResourceSiteExt*>::iterator iRL1 = Resources[bestRLI->index]->Linked.begin(); iRL1 != Resources[bestRLI->index]->Linked.end(); ++iRL1 )
		{
			RSD = &iRL1->second->S->siteDistance.find(Resources[bestRLI->bestI]->S)->second;
			if( RSD->bestDistance == 0 )
			{	// An additionally needed set of calculations
//*l<<" r-L1-2";
				for( MobileTypes.begin(); RSD->bestDistance == 0 && MobileTypes.nextE(MT); )
					if( RSD->distance.find(MT->MD->pathType) == RSD->distance.end() )
					{
						FindResourceDistance(iRL1->second->S,Resources[bestRLI->bestI]->S,MT->MD->pathType);
						RMT[iRL1->first][bestRLI->bestI] = RSD->bestPathType;
						RMT[bestRLI->bestI][iRL1->first] = RSD->bestPathType;
					}
				if( RSD->bestDistance == 0 )
				{
					RSD->bestDistance = &RSD->minDistance;
					RSD = &Resources[bestRLI->bestI]->S->siteDistance.find(iRL1->second->S)->second;
					RSD->bestDistance = &RSD->minDistance;
					RMT[iRL1->first][bestRLI->bestI] = MTPenaltySize;
					RMT[bestRLI->bestI][iRL1->first] = MTPenaltySize;
				}
				RD[iRL1->first][bestRLI->bestI] = -1.0;
				RD[bestRLI->bestI][iRL1->first] = -1.0;
				resetLink = true;
			}
		}
//		linkClock3 += clock()-linkClockTemp;
//		linkClockTemp = clock();
		if( resetLink )
		{	// New calculations have been made, distances should be reconsidered
			for( RLIList.begin(); RLIList.nextE(RLI); )
				if( (RLI->index == bestRLI->bestI && bestRLI->restrictedR.find(RLI->bestI) != bestRLI->restrictedR.end() ) ||
					(Resources[bestRLI->index]->Linked.find(RLI->index) != Resources[bestRLI->index]->Linked.end() && RLI->bestI == bestRLI->bestI) )
					RLI->bestI = -1;
			bestRLI->bestI = -1;
			bestRLI = 0;
		}
		else
		{
//*l<<" size1:"<<Resources[bestRLI->index]->Linked.size();
			// These are an additional sets of conditions that are not checked until the best has been found
			for(map<int,ResourceSiteExt*>::iterator iRL1 = Resources[bestRLI->index]->Linked.begin(); iRL1 != Resources[bestRLI->index]->Linked.end(); ++iRL1 )
			{
//*l<<" d(L1-B):"<<RD[bestRLI->bestI][iRL1->first];
				if( RD[bestRLI->bestI][iRL1->first] < RD[bestRLI->bestI][bestRLI->index] )
				{	// The best choose didn't work out
//*l<<" *no good*";
					bestRLI->bestI = -2; // this will end the search for this paticular resource
					bestRLI = 0;
					break;
				}
			}
			if( bestRLI != 0 )
			{
//*l<<" size2:"<<Resources[bestRLI->bestI]->Linked.size();
				for(map<int,ResourceSiteExt*>::iterator iRLB = Resources[bestRLI->bestI]->Linked.begin(); iRLB != Resources[bestRLI->bestI]->Linked.end(); ++iRLB )
				{
					RSD = &Resources[bestRLI->index]->S->siteDistance.find(iRLB->second->S)->second;
					if( RSD->bestDistance == 0 )
					{	// An additionally needed set of calculations
						for( MobileTypes.begin(); RSD->bestDistance == 0 && MobileTypes.nextE(MT); )
							if( RSD->distance.find(MT->MD->pathType) == RSD->distance.end() )
							{
								FindResourceDistance(Resources[bestRLI->index]->S,iRLB->second->S,MT->MD->pathType);
								RMT[bestRLI->index][iRLB->first] = RSD->bestPathType;
								RMT[iRLB->first][bestRLI->index] = RSD->bestPathType;
							}
						if( RSD->bestDistance == 0 )
						{
							RSD->bestDistance = &RSD->minDistance;
							RSD = &iRLB->second->S->siteDistance.find(Resources[bestRLI->index]->S)->second;
							RSD->bestDistance = &RSD->minDistance;
							RMT[bestRLI->index][iRLB->first] = MTPenaltySize;
							RMT[iRLB->first][bestRLI->index] = MTPenaltySize;
						}
						RD[bestRLI->index][iRLB->first] = -1.0;
						RD[iRLB->first][bestRLI->index] = -1.0;
					}
					if( RD[bestRLI->index][iRLB->first] < 0.0 )
					{
						RD[bestRLI->index][iRLB->first] = Resources[bestRLI->index]->S->GetResourceDistance(iRLB->second->S,RSD->bestPathType) * MTPenalty[RMT[bestRLI->index][iRLB->first]];
						RD[iRLB->first][bestRLI->index] = RD[bestRLI->index][iRLB->first];
					}
//*l<<" d(LB-1):"<<RD[bestRLI->index][iRLB->first];
					if( RD[bestRLI->index][iRLB->first] < RD[bestRLI->index][bestRLI->bestI] )
					{
//						float3 Pos1=Resources[bestRLI->index]->S->position;
//						float3 Pos2=Resources[bestRLI->bestI]->S->position;
//						Pos1.y+=515.0;
//						Pos2.y+=755.0;
//						cb->CreateLineFigure(Pos1,Pos2,30,0,900000,0);
//*l<<" *no good*";
						// The best choose didn't work out
						bestRLI->bestI = -2; // this will end the search for this paticular resource
						bestRLI = 0;
						break;
					}
				}
			}
			if( bestRLI != 0 )
			{
				RSD = &Resources[bestRLI->index]->S->siteDistance.find(Resources[bestRLI->bestI]->S)->second;
//*l<<"\n*Best* R("<<bestRLI->index<<"<->"<<bestRLI->bestI<<") dis=";
//*l<<Resources[bestRLI->index]->S->GetResourceDistance(Resources[bestRLI->bestI]->S,RSD->bestPathType);
//*l<<" p="<<MTPenalty[RMT[bestRLI->index][bestRLI->bestI]];
//*l<<" d="<<RD[bestRLI->bestI][bestRLI->index];
//*l<<" d="<<RD[bestRLI->index][bestRLI->bestI];
				if( RAIDEBUGGING && cb->GetMyTeam() == 0 ) // Debug Lines
				{
					vector<float3> *path = &RSD->pathDebug;
					if( path->empty() )
					{
						path->push_back(Resources[bestRLI->index]->S->position);
						path->push_back(Resources[bestRLI->bestI]->S->position);
					}
					for( size_t i=0; i+1 < path->size(); i++ )
					{
						float3 Pos1=path->at(i);
						float3 Pos2=path->at(i+1);
						Pos1.y+=45.0;
						if( i == 0 )
							Pos1.y+=35.0;
						Pos2.y+=45.0;
						if( i == path->size()-2 )
							Pos2.y-=75.0;
						cb->CreateLineFigure(Pos1,Pos2,30,0,900000,0);
					}
				}

				Resources[bestRLI->index]->Linked.insert(irPair(bestRLI->bestI,Resources[bestRLI->bestI]));
				Resources[bestRLI->bestI]->Linked.insert(irPair(bestRLI->index,Resources[bestRLI->index]));
				for( RLIList.begin(); RLIList.nextE(RLI); )
					if( RLI->restrictedR.find(bestRLI->bestI) != RLI->restrictedR.end() )
					{
						RLI->bestI = -1;
						if( RLI->index == bestRLI->bestI )
							RLI->restrictedR.insert(bestRLI->index);
					}
				bestRLI->restrictedR.insert(bestRLI->bestI);
				bestRLI->bestI = -1;
				for( RLIList.begin(); RLIList.nextE(RLI); )
					if( Resources[RLI->index]->Linked.size() >= 3 )
						RLIList.removeE();
				bestRLI = 0;
			}
		}
//		linkClock4 += clock()-linkClockTemp;
	}
	if( RLIList.elementSize > 0 )
		*l<<"\n Second Linking Algorithm Aborted (was running for more than "<<SecondAlgorithmTimeLimit<<" seconds)";

	for( int iR=0; iR<ResourceSize; iR++ )
	{
		delete [] RMT[iR];
		delete [] RD[iR];
	}
	delete [] RMT;
	delete [] RD;
	delete [] MTPenalty;
//	*l<<"\n   Second Link Loading Time 1: "<<linkClock1/(double)CLOCKS_PER_SEC<<"s";
//	*l<<"\n   Second Link Loading Time 2: "<<linkClock2/(double)CLOCKS_PER_SEC<<"s";
//	*l<<"\n   Second Link Loading Time 3: "<<linkClock3/(double)CLOCKS_PER_SEC<<"s";
//	*l<<"\n   Second Link Loading Time 4: "<<linkClock4/(double)CLOCKS_PER_SEC<<"s";
	*l<<"\n   Second Link Algorithm Loading Time: "<<(clock()-linkStartClock)/CLOCKS_PER_SEC<<"s";

	// Setting Linked Distance 2
	for( int iR=0; iR<ResourceSize; iR++ )
	{
		for( map<int,ResourceSiteExt*>::iterator iRL=Resources[iR]->Linked.begin(); iRL!=Resources[iR]->Linked.end(); ++iRL )
		{
			if( Resources[iR]->LinkedD2.find(iRL->first) == Resources[iR]->LinkedD2.end() )
				Resources[iR]->LinkedD2.insert(irPair(iRL->first,Resources[iRL->first]));
			for( map<int,ResourceSiteExt*>::iterator iRL2=Resources[iRL->first]->Linked.begin(); iRL2!=Resources[iRL->first]->Linked.end(); ++iRL2 )
			{
				if( iRL2->first != iR && Resources[iR]->LinkedD2.find(iRL2->first) == Resources[iR]->LinkedD2.end() )
					Resources[iR]->LinkedD2.insert(irPair(iRL2->first,Resources[iRL2->first]));
			}
		}
	}

	for( int iR=0; iR<ResourceSize; iR++ )
		Resources[iR]->SetRanged();

	// Debug Lines
	if( RAIDEBUGGING && cb->GetMyTeam() == 0 )
		for( int iR1=0; iR1<ResourceSize; iR1++ )
		{
			for( int iR2=iR1+1; iR2<ResourceSize; iR2++ )
			{
				if( Resources[iR1]->Linked.find(iR2) != Resources[iR1]->Linked.end() )
				{
					vector<float3> *path = &Resources[iR1]->S->siteDistance.find(Resources[iR2]->S)->second.pathDebug;
					if( path->empty() )
					{
						path->push_back(Resources[iR1]->S->position);
						path->push_back(Resources[iR2]->S->position);
					}
					for( int i=0; i<int(path->size())-1; i++ )
					{
						float3 Pos1=path->at(i);
						float3 Pos2=path->at(i+1);
						Pos1.y+=15.0;
						Pos2.y+=15.0;
						cb->CreateLineFigure(Pos1,Pos2,10,0,900000,0);
					}
				}
			}

			*l<<"\n R("<<iR1<<") type="<<Resources[iR1]->S->type<<" Pos(x"<<Resources[iR1]->S->position.x<<",z"<<Resources[iR1]->S->position.z<<") L("<<Resources[iR1]->Linked.size()<<"):";
			for( map<int,ResourceSiteExt*>::iterator RL=Resources[iR1]->Linked.begin(); RL!=Resources[iR1]->Linked.end(); ++RL )
				*l<<" "<<RL->first;
			*l<<"\t B("<<Resources[iR1]->BuildOptions.size()<<"):";
			for( map<int,ResourceSiteExtBO>::iterator RS=Resources[iR1]->BuildOptions.begin(); RS!=Resources[iR1]->BuildOptions.end(); ++RS )
				*l<<" "<<RS->first;
		}
}

cBuilderPlacement::~cBuilderPlacement()
{
	for( int i=0; i<ResourceSize; i++ )
		delete Resources[i];
//	delete [] Sector;
}

void cBuilderPlacement::UResourceCreated(int unit, UnitInfo *U)
{
	if( !NeedResourceSite(U->ud) )
		return;

	if( U->ud->extractsMetal > 0.0f )
		UExtractor.insert(cRAI::iupPair(unit,U));
	else if( U->ud->needGeo )
		UGeoPlant.insert(cRAI::iupPair(unit,U));

	int iR=GetResourceIndex(unit,U->ud);
	if( iR == -1 )
	{
//		cb->SendTextMsg("TEST: Decomission: iR = -1",0);
//		G->B->Decomission.insert(unit); // broken
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
		G->B->Decomission.insert(unit);
	}
}

void cBuilderPlacement::UResourceDestroyed(int unit, UnitInfo *U)
{
	if( !NeedResourceSite(U->ud) )
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
	if( !NeedResourceSite(E->ud) )
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

		if( Resources[iR]->builderID > -1 && G->ValidateUnit(Resources[iR]->builderID) && int(cb->GetCurrentUnitCommands(Resources[iR]->builderID)->size())>0 && Resources[iR]->BuildOptions.find(-cb->GetCurrentUnitCommands(Resources[iR]->builderID)->begin()->GetID()) != Resources[iR]->BuildOptions.end() )
		{
			if( Resources[iR]->builderUI->ud->canCapture )
			{
				Command c(CMD_CAPTURE);
				c.PushParam(enemy);
				cb->GiveOrder(Resources[iR]->builderID, &c);
			}
			else if ( Resources[iR]->builderUI->ud->canReclaim )
			{
				Command c(CMD_RECLAIM);
				c.PushParam(enemy);
				cb->GiveOrder(Resources[iR]->builderID, &c);
			}
		}
	}
}

bool cBuilderPlacement::NeedResourceSite(const UnitDef* bd) const
{
	if( !bd->needGeo && bd->extractsMetal==0 )
		return false;
	if( bd->extractsMetal>0 && G->RM->isMetalMap )
		return false;
	return true;
}

ResourceSiteExt* cBuilderPlacement::FindResourceSite(float3& pos, const UnitDef* bd, TerrainMapArea* BuilderMA)
{
//	*l<<"\nFindResourceSite(x"<<pos.x<<" z"<<pos.z<<" y"<<pos.y<<","<<bd->humanName<<","<<(BuilderMA==0?"-":"*")<<")";
	if( !NeedResourceSite(bd) )
		return 0;
	UpdateAllyResources();
	map<int,ResourceSiteExt*>* RL;
	if( int(UExtractor.size()) == 0 && int(UGeoPlant.size()) == 0 && int(RSRemaining.size()) > 0 )
	{
//		*l<<"\n\n(No Resources) - Alternative Search";
		RL=&RSRemaining;
		CheckBlockedRList(RL);
	}
	else
		RL=&RSAvailable;
	float fBest=0;
	int iBest=-1;
	float distance;
	for( map<int,ResourceSiteExt*>::iterator iR=RL->begin(); iR!=RL->end(); ++iR )
	{
//*l<<" "<<iR->first;
		if( iR->second->builderID == -1 &&
			iR->second->BuildOptions.find(bd->id) != iR->second->BuildOptions.end() &&
			!iR->second->BuildOptions.find(bd->id)->second.RBBlocked &&
			!iR->second->BuildOptions.find(bd->id)->second.RBRanked)
		{
			if( G->TM->CanMoveToPos(BuilderMA,iR->second->S->position) )
			{
				distance = pos.distance(iR->second->S->position);
				if( iBest==-1 || distance < fBest )
				{
					iBest=iR->first;
					fBest=distance;
				}
			}
		}
	}
	if( iBest == -1 )
	{
		*l<<"\nWARNING: FindResourceSite() has failed: builder = "<<bd->humanName;
		return 0;
	}

	return Resources[iBest];
}

float3 cBuilderPlacement::FindBuildPosition(sBuildQuarry *BQ)
{
	const UnitDef* bd = BQ->creationUD->ud;
	if( BQ->RS != 0 ) // Resource is set
		return cb->ClosestBuildSite(bd,BQ->RS->S->position,BQ->RS->searchRadius,BQ->RS->disApart);

	float3 cPosition = cb->GetUnitPos(BQ->builderID); // Construction Position
	if( NeedResourceSite(BQ->creationUD->ud) )
	{
		cPosition.x = -2;
		cPosition.z = -1;
		cPosition.y = -1;
		return cPosition;
	}

	G->CorrectPosition(cPosition);
	float3 bPosition = cPosition; // Builder Position
	if( FindWeaponPlacement(BQ->builderUI,cPosition) )
	{
		cPosition.x+=rand()%81-40;
		cPosition.z+=rand()%81-40;
		G->CorrectPosition(cPosition);
	}
	else if( BQ->builderUI->ud->speed == 0.0 )
	{
		cPosition.x+=rand()%int(1.8*BQ->builderUI->ud->buildDistance)-0.9*BQ->builderUI->ud->buildDistance;
		cPosition.z+=rand()%int(1.8*BQ->builderUI->ud->buildDistance)-0.9*BQ->builderUI->ud->buildDistance;
		G->CorrectPosition(cPosition);
	}
	else
	{
		cPosition.x+=rand()%int(0.9*BQ->builderUI->ud->buildDistance)-0.45*BQ->builderUI->ud->buildDistance;
		cPosition.z+=rand()%int(0.9*BQ->builderUI->ud->buildDistance)-0.45*BQ->builderUI->ud->buildDistance;
		G->CorrectPosition(cPosition);
	}

	int iS;
	if( !CanBuildAt(BQ->builderUI,bPosition,cPosition) || !CanBeBuiltAt(BQ->creationUD,cPosition) )
	{
		iS = G->TM->GetSectorIndex(cPosition);
		if( BQ->creationUD->mobileType != 0 ) // a factory or mobile unit
		{
			TerrainMapAreaSector *AS = G->TM->GetAlternativeSector(BQ->builderUI->area,iS,BQ->creationUD->mobileType);
			if( BQ->creationUD->immobileType != 0 ) // a factory
			{
				TerrainMapSector *S = G->TM->GetAlternativeSector(AS->area,iS,BQ->creationUD->immobileType);
				if( S != 0 )
					cPosition = S->position;
				else
				{
					cPosition.x = -3;
					cPosition.z = -1;
					cPosition.y = -1;
					return cPosition;
				}
			}
			else
				cPosition = AS->S->position;
		}
		else if( BQ->creationUD->immobileType != 0 ) // buildings
			cPosition = G->TM->GetClosestSector(BQ->creationUD->immobileType,iS)->position;
	}
	if( BQ->builderUI->ud->speed == 0.0 )
	{
		float3 cPosition2 = cb->ClosestBuildSite(bd,cPosition,BQ->builderUI->ud->buildDistance,5);
		if( cPosition2.x <= 0 && cPosition2.y <= 0 && cPosition2.z <= 0 )
			cPosition2 = cb->ClosestBuildSite(bd,cPosition,BQ->builderUI->ud->buildDistance+25,1);
		return cPosition2;
	}

	iS = G->TM->GetSectorIndex(cPosition);
	float fSearchRadius=1000.0f;
	int iDisApart=10;
	if( G->TM->sector[iS].isWater )
		iDisApart=15;
	else if( bd->speed > 0 )
		iDisApart=5;
	else if( int(bd->buildOptions.size())>0 )
		iDisApart=15;
	if( iDisApart < int(bd->kamikazeDist/8.0) )
		iDisApart = int(bd->kamikazeDist/8.0);

	float3 cPosition2 = cb->ClosestBuildSite(bd,cPosition,fSearchRadius,iDisApart);
	if( cPosition2.x == -1 )
	{
		cPosition2 = cb->ClosestBuildSite(bd,cPosition,2500.0f,iDisApart);
		if( cPosition2.x == -1 )
			return bPosition;
	}
	return cPosition2;
}

bool cBuilderPlacement::CanBeBuiltAt(sRAIUnitDef *udr, float3& position, const float& range)
{
	int iS = G->TM->GetSectorIndex(position);
	TerrainMapSector* sector;
	if( udr->mobileType != 0 ) // a factory or mobile unit
	{
		TerrainMapAreaSector* AS = G->TM->GetAlternativeSector(0,iS,udr->mobileType);
		if( udr->immobileType != 0 ) // a factory
		{
			sector = G->TM->GetAlternativeSector(AS->area,iS,udr->immobileType);
			if( sector == 0 )
				return false;
		}
		else
			sector = AS->S;
	}
	else if( udr->immobileType != 0 ) // buildings
		sector = G->TM->GetClosestSector(udr->immobileType,iS);
	else
		return true; // flying units

	if( sector == &G->TM->sector[iS] ) // the current sector is the best sector
		return true;
	return sector->position.distance2D(G->TM->sector[iS].position) < range;
}

bool cBuilderPlacement::CanBuildAt(UnitInfo *U, const float3& position, const float3& destination)
{
	if( U->udr->immobileType != 0 ) // A hub or factory
		return position.distance2D(destination) < U->ud->buildDistance;
	if( U->area == 0 ) // A flying unit
		return true;
	int iS = G->TM->GetSectorIndex(destination);
	if( U->area->sector.find(iS) != U->area->sector.end() )
		return true;
	if( G->TM->GetClosestSector(U->area,iS)->S->position.distance2D(destination) < U->ud->buildDistance - G->TM->convertStoP )
		return true;
	return false;
}
/*
// Callback Clone Functions
const int SQUARE_SIZE = 8;
#include <algorithm>
// Stolen directly from ALCallback.cpp
struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};
// Stolen directly from ALCallback.cpp
bool SearchOffsetComparator (const SearchOffset& a, const SearchOffset& b)
{	return a.qdist < b.qdist;	}
// Stolen directly from ALCallback.cpp
const vector<SearchOffset>& GetSearchOffsetTable (int radius)
{	static vector <SearchOffset> searchOffsets;
	int size = radius*radius*4;
	if (size > searchOffsets.size()) {
		searchOffsets.resize (size);
		for (int y=0;y<radius*2;y++)
			for (int x=0;x<radius*2;x++)
			{	SearchOffset& i = searchOffsets[y*radius*2+x];
				i.dx = x-radius;
				i.dy = y-radius;
				i.qdist = i.dx*i.dx+i.dy*i.dy;
			}
		sort (searchOffsets.begin(), searchOffsets.end(), SearchOffsetComparator);
	}
	return searchOffsets;
}
float3 cBuilderPlacement::ClosestBuildSite(const UnitDef* ud, float3 p, float sRadius, int facing)
{
	if (!ud) return float3(-1.0f,0.0f,0.0f);
	int endr = (int)(sRadius/(SQUARE_SIZE*2));
	const vector<SearchOffset>& ofs = GetSearchOffsetTable (endr);
	for(int so=0;so<endr*endr*4;so++)
	{
		float x = p.x+ofs[so].dx*SQUARE_SIZE*2;
		float z = p.z+ofs[so].dy*SQUARE_SIZE*2;
		if( cb->CanBuildAt(ud,float3(x,0,z),facing) )
		{
			return float3(x,0,z);
		}
	}
	return float3(-1.0f,0.0f,0.0f);
}
*/
bool cBuilderPlacement::FindWeaponPlacement(UnitInfo *U, float3& position)
{
	if( U->BuildQ->creationUD->WeaponGuardRange == 0 )
		return false;

	if( U->BuildQ->creationUD->ud->minWaterDepth < 0 && U->BuildQ->creationUD->WeaponSeaEff.BestRange > 0 )
	{
		int iS = G->TM->GetSectorIndex(position);
		if( !G->TM->sector[iS].isWater )
			position = G->TM->GetClosestSector(G->TM->waterSectorType,iS)->position;
		return true;
	}

	int BID = -1;
	float3 buildPosition;
	for(map<int,UnitInfo*>::iterator i=G->UImmobile.begin(); i!=G->UImmobile.end(); ++i )
	{
		buildPosition = cb->GetUnitPos(i->first);
		if( i->second->udr->WeaponGuardRange == 0 && int(i->second->UDefences.size()) == 0 &&
			CanBuildAt(U,position,buildPosition) && CanBeBuiltAt(U->BuildQ->creationUD,buildPosition,i->second->udr->WeaponGuardRange) )
			if( BID == -1 || position.distance2D(buildPosition) < position.distance2D(cb->GetUnitPos(BID)) )
				BID = i->first;
	}

	if( BID > 0 )
	{
		position = cb->GetUnitPos(BID);
		return true;
	}

	return false;
}

void cBuilderPlacement::CheckBlockedRList( map<int,ResourceSiteExt*> *RL )
{
	if( RL == 0 )
		RL = &RSAvailable;
	set<int> deletion;
	for( map<int,ResourceSiteExt*>::iterator iR=RL->begin(); iR!=RL->end(); ++iR )
	{
		if( iR->second->unitID == -1 )
			iR->second->CheckBlocked();
		else if( cb->GetUnitHealth(iR->second->unitID) <= 0 )
		{
			deletion.insert(iR->first);
		}
	}
	while( int(deletion.size()) > 0 )
	{
		if( RL->find(*deletion.begin()) != RL->end() )
		{
			ResourceSiteExt* RS = RL->find(*deletion.begin())->second;
			SetResourceOwner(*deletion.begin(),RS,-1);
			RS->CheckBlocked();
		}
		deletion.erase(deletion.begin());
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
		if( !NeedResourceSite(ud) || G->Units.find(Units[iU]) != G->Units.end() )
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
	float fBest=0.0f;
	for( int iR=0; iR<ResourceSize; iR++ )
	{
		if(	Resources[iR]->unitID == unit )
			return iR;
		if( (Resources[iR]->S->type == 0 && ud->extractsMetal>0.0 ) ||
			(Resources[iR]->S->type == 1 && ud->needGeo ) )
		{
			float dis=Resources[iR]->S->position.distance2D(cb->GetUnitPos(unit));
			if( dis <= Resources[iR]->searchRadius && Resources[iR]->BuildOptions.find(ud->id) != Resources[iR]->BuildOptions.end() && (iBest==-1 || dis < fBest) )
			{
				iBest = iR;
				fBest = dis;
			}
		}
	}
	return iBest;
}

void cBuilderPlacement::SetResourceOwner(int RSindex, ResourceSiteExt* RS, int unit, sRAIUnitDef* udr)
{
	if( int(RSAvailable.size()) == 0 && unit >= 0 )
		for( int iR=0; iR<ResourceSize; iR++ )
			Resources[iR]->SetRanged(false);

/*	*l<<"\n\nSetResourceOwner("<<RSindex<<",_,"<<unit<<",{"<<(udr==0?"0":udr->ud->humanName)<<"})";
	*l<<"\n - Resources Available ("<<Resource.size()<<"):";
	for( map<int,ResourceSiteExt*>::iterator iR=Resource.begin(); iR!=Resource.end(); iR++ )
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
/*
	sRAIBuildList *BL;
	if( RS->S->type == 0 )
		BL=G->UDH->BLMetalL;
	else
		BL=G->UDH->BLEnergyL;
*/
	if( unit >= 0 && oldunit == -1 )
	{
		if( RSAvailable.find( RSindex ) == RSAvailable.end() )
		{
			RS->SetRanged();
			RSAvailable.insert(irPair(RSindex,RS));
			RSRemaining.erase(RSindex);
		}
		for( map<int,ResourceSiteExt*>::iterator iR=RS->Linked.begin(); iR!=RS->Linked.end(); ++iR )
		{
			if( RSAvailable.find(iR->first) == RSAvailable.end() )
			{
				iR->second->SetRanged();
				RSAvailable.insert(irPair(iR->first,iR->second));
				RSRemaining.erase(iR->first);
			}
			for( map<int,ResourceSiteExt*>::iterator iR2=iR->second->Linked.begin(); iR2!=iR->second->Linked.end(); ++iR2 )
			{
				if( RSAvailable.find(iR2->first) == RSAvailable.end() )
				{
					iR2->second->SetRanged();
					RSAvailable.insert(irPair(iR2->first,iR2->second));
					RSRemaining.erase(iR2->first);
				}
			}
		}
	}
	else if( unit == -1 )
	{
		set<int> RSL;
		RSL.insert(RSindex);
		for( map<int,ResourceSiteExt*>::iterator iR=RS->LinkedD2.begin(); iR!=RS->LinkedD2.end(); ++iR )
		{
			if( RSAvailable.find(iR->first) != RSAvailable.end() && ( iR->second->unitID == -1 || iR->second->enemy ) )
				RSL.insert(iR->first);
		}
		bool found;
		while( int(RSL.size()) > 0 )
		{
			found=false;
			for( map<int,ResourceSiteExt*>::iterator iR=Resources[*RSL.begin()]->LinkedD2.begin(); iR!=Resources[*RSL.begin()]->LinkedD2.end(); ++iR )
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
				RSRemaining.insert(irPair(*RSL.begin(),Resources[*RSL.begin()]));
				RSAvailable.erase(*RSL.begin());
			}
			RSL.erase(RSL.begin());
		}
	}
	RS->CheckRanked();
	if( int(RSAvailable.size()) == 0 )
		for( int iR=0; iR<ResourceSize; iR++ )
			Resources[iR]->SetRanged(true);
/*
	*l<<"\n - Resources Available ("<<Resource.size()<<"):";
	for( map<int,ResourceSiteExt*>::iterator iR=Resource.begin(); iR!=Resource.end(); iR++ )
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
	for( map<int,ResourceSiteExt*>::iterator iR=Resource.begin(); iR!=Resource.end(); iR++ )
		if( iR->second->unitID == -1 || iR->second->enemy )
			*l<<" "<<iR->first;
	*l<<"\n\n";
*/
}

float cBuilderPlacement::FindResourceDistance(ResourceSite* R1, ResourceSite* R2, const int &pathType)
{
	ResourceSiteDistance* RSD = &R1->siteDistance.find(R2)->second;
	if( RSD->distance.find(pathType) != RSD->distance.end() )
		return RSD->distance.find(pathType)->second;
	if( RSD->bestDistance != 0 )
		*l<<"\nERROR: RSD->bestDistance has already been set.";
	RSD->distance.insert(ifPair(pathType,0.0));
	float *distance = &RSD->distance.find(pathType)->second;
	int pathID = cb->InitPath(R1->position,R2->position,pathType);
	RSD->pathDebug.push_back(R1->position);
	do
	{
		RSD->pathDebug.push_back(cb->GetNextWaypoint(pathID));
		*distance += RSD->pathDebug.back().distance(RSD->pathDebug.at(RSD->pathDebug.size()-2));
//		*l<<" x"<<RSD->pathDebug.back().x<<" z"<<RSD->pathDebug.back().z<<" "<<*distance<<" ";
		if( RSD->pathDebug.back() == RSD->pathDebug.at(RSD->pathDebug.size()-2) )
		{	// The last two points were identical, the pathfinder has failed
			*distance = -1.0;
			break;
		}
	}
	while( RSD->pathDebug.back() != R2->position ); // the end position has been reached
	cb->FreePath(pathID);
	ResourceSiteDistance* RSD2 = &R2->siteDistance.find(R1)->second;
	RSD2->distance.insert(ifPair(pathType,*distance));
	if( *distance > 0 && RSD->bestDistance == 0 )
	{
		RSD->bestPathType = pathType;
		RSD->bestDistance = distance;
		RSD2->bestPathType = pathType;
		RSD2->bestDistance = &RSD2->distance.find(pathType)->second;
		if( RAIDEBUGGING )
			RSD2->pathDebug = RSD->pathDebug;
		else
			RSD->pathDebug.clear();
	}
	else
		RSD->pathDebug.clear();
	return *distance;
}
