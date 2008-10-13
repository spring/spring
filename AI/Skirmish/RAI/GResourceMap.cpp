#include "GResourceMap.h"
#include <set>
#include "Sim/Features/FeatureDef.h"
//#include <time.h>
#include <stdio.h>

// a metal map block is 0-255 * cb->GetMaxMetal(), extractors are normally built on several
const int RAI_MinimalMetalSite = 500; // (this * ud->extractsMetal = the predicted income)

ResourceSite::ResourceSite(float3& rsPosition, int rsFeatureID, const FeatureDef* fd)
{
	featureID = rsFeatureID;
	featureD = fd;
	if( featureID >= 0 )
	{
		type = 1;
		amount = 1.0;
	}
	else
	{
		type = 0;
		amount = 0.0;
	}
	position = rsPosition;
}

float ResourceSite::GetResourceDistance(ResourceSite* RS, const int& pathType)
{
	ResourceSiteDistance* RSD = &siteDistance.find(RS)->second;
	if( RSD->distance.find(pathType) != RSD->distance.end() )
		return RSD->distance.find(pathType)->second;
	if( RSD->bestDistance != 0 )
		return *RSD->bestDistance;
	return RSD->minDistance;
}

GlobalResourceMap::GlobalResourceMap(IAICallback* cb, cLogFile* l, GlobalTerrainMap* TM)
{
//	l = logfile;
	*l<<"\n Loading the Resource-Map ...";

	const int RAI_MaxMetalSites = 1500;
	const int RAI_MaxGeoSites = 500;

	for( int iT=0; iT<2; iT++ )
		RSize[iT] = 0;
	R[0] = new ResourceSite*[RAI_MaxMetalSites];
	R[1] = new ResourceSite*[RAI_MaxGeoSites];
	sector = 0;

	bool UDMetalResources = false;
	bool UDGeoResource = false;
	int udSize=cb->GetNumUnitDefs();
	const UnitDef **udList = new const UnitDef*[udSize];
	cb->GetUnitDefList(udList);
	for( int iud=udSize-1; iud>=0; iud-- )
	{
		if( udList[iud] == 0 ) // Work-Around(Several Mods):  Spring-Version(v0.72b1-0.76b1)
		{
			cb->SendTextMsg("WARNING: Mod Unit Definition Invalid.",0);
			*l<<"\n  WARNING: (unitdef->id="<<iud+1<<") Mod UnitDefList["<<iud<<"] = 0";
			udSize--;
			udList[iud] = udList[udSize];
		}
		else if( udList[iud]->extractsMetal > 0 )
			UDMetalResources = true;
		else if( udList[iud]->needGeo )
			UDGeoResource = true;
		else
		{
			udSize--;
			udList[iud] = udList[udSize];
		}
	}

	if( !UDGeoResource && !UDMetalResources )
	{
		saveResourceFile = false;
		delete [] udList;
		*l<<"\n  No resource units were detected for this mod.";
		return;
	}

	int *fList;
	int fSize = 0;
	if( UDGeoResource )
	{
		fList = new int[10000]; // Feature List
//		int fSize = cb->GetFeatures(fList,10000); // Crashes:  Spring-Version(v0.75b2-v0.76b1)
		fSize = cb->GetFeatures(fList,10000,float3(1,1,1),999999); // Crash Work-Around
		if( fSize == 10000 )
		{
			cb->SendTextMsg("WARNING: not all features will be searched for Geo Build Options",0);
			*l<<"\nWARNING: not all features will be searched for Geo Build Options";
		}
		for(int i=0; i<fSize; i++)
			if( !cb->GetFeatureDef(fList[i])->geoThermal )
				fList[i] = fList[--fSize];
	}

	resourceFileName = RAIDirectory + "cache/" + string(cb->GetModName());
	resourceFileName.resize(resourceFileName.size()-4);
	resourceFileName += "-" + string(cb->GetMapName());
	resourceFileName.resize(resourceFileName.size()-3);
	resourceFileName += "res";
	FILE *resourceFile = fopen(resourceFileName.c_str(),"rb");
	bool useResourceFile = false;
	if( resourceFile )
	{
		useResourceFile = true;
		*l<<"\n  Loading Resource-Site Data ...";
		int udL2Size;
		fread(&udL2Size, sizeof(int), 1, resourceFile);
		if( udSize != udL2Size )
			useResourceFile = false;
		else
		{	// Checks if the unit-Def list have changed
			int ID;
			for( int i=0; i<udSize; i++ )
			{
				fread(&ID, sizeof(int), 1, resourceFile);
				if( udList[i]->id != ID )
				{	// The order or types of definitions have changed
					useResourceFile = false;
					break;
				}
			}
			if( useResourceFile )
			{
				int featureSites;
				fread(&featureSites, sizeof(int), 1, resourceFile);
				if( fSize != featureSites )
					useResourceFile = false;
				else
				{	// Checks if the feature resource list has changed
					for( int i=0; i<fSize; i++ )
					{
						fread(&ID, sizeof(int), 1, resourceFile);
						if( fList[i] != ID )
						{	// The order or types of features have changed
							useResourceFile = false;
							break;
						}
					}
					if( useResourceFile )
					{	// The actual loading starts here
						typedef pair<ResourceSite*,ResourceSiteDistance> rrPair;
						typedef pair<int,float> ifPair;
						float3 position;
						float distance;
						int featureID;
						int size,dSize;
						int iT2,iR2;
						int optionID;
						for( int iT=0; iT<2; iT++ )
						{
							fread(&RSize[iT], sizeof(int), 1, resourceFile);
							for( int iR=0; iR<RSize[iT]; iR++ )
							{
								fread(&featureID, sizeof(int), 1, resourceFile);
								fread(&position, sizeof(float3), 1, resourceFile);
								if( featureID >= 0 )
									R[iT][iR] = new ResourceSite(position,featureID,cb->GetFeatureDef(featureID));
								else
									R[iT][iR] = new ResourceSite(position);
								fread(&R[iT][iR]->amount, sizeof(float), 1, resourceFile);
								fread(&size, sizeof(int), 1, resourceFile);
								for( int i=0; i<size; i++ )
								{
									fread(&optionID, sizeof(int), 1, resourceFile);
									R[iT][iR]->options.insert(optionID);
								}
							}
						}
						for( int iT=0; iT<2; iT++ )
							for( int iR=0; iR<RSize[iT]; iR++ )
							{
								fread(&size, sizeof(int), 1, resourceFile);
								for( int i=0; i<size; i++ )
								{
									fread(&iT2, sizeof(int), 1, resourceFile);
									fread(&iR2, sizeof(int), 1, resourceFile);
									R[iT][iR]->siteDistance.insert(rrPair(R[iT2][iR2],ResourceSiteDistance(0.0)));
									ResourceSiteDistance* RSD = &R[iT][iR]->siteDistance.find(R[iT2][iR2])->second;
									fread(&RSD->minDistance, sizeof(float), 1, resourceFile);
									fread(&RSD->bestPathType, sizeof(int), 1, resourceFile);
									fread(&dSize, sizeof(int), 1, resourceFile);
									for( int i=0; i<dSize; i++ )
									{
										fread(&optionID, sizeof(int), 1, resourceFile);
										fread(&distance, sizeof(float), 1, resourceFile);
										RSD->distance.insert(ifPair(optionID,distance));
									}
									fread(&dSize, sizeof(int), 1, resourceFile);
									for( int i=0; i<dSize; i++ )
									{
										fread(&position, sizeof(float3), 1, resourceFile);
										RSD->pathDebug.push_back(position);
									}
									if( RSD->bestPathType == -2 )
										RSD->bestDistance = &RSD->minDistance;
									else if( RSD->bestPathType >= 0 )
										RSD->bestDistance = &RSD->distance.find(RSD->bestPathType)->second;
								}
							}
						fread(&averageMetalSite, sizeof(float), 1, resourceFile);
						fread(&isMetalMap, sizeof(bool), 1, resourceFile);
						if( isMetalMap )
						{
							sector = new MetalMapSector[TM->sectorXSize*TM->sectorZSize];
							for( int iS=0; iS<TM->sectorXSize*TM->sectorZSize; iS++ )
							{
								fread(&sector[iS].isMetalSector, sizeof(bool), 1, resourceFile);
								sector[iS].S = &TM->sector[iS];
							}
						}
					}
				}
			}
		}
		fclose(resourceFile);
		if( !useResourceFile )
			*l<<"\n  A change has been detected in the map/mod, the resource data will be reloaded.";
	}

	if( useResourceFile || cb->GetCurrentFrame() == 0 )
	{	// only save the data if it was created at the beginning of a game
		saveResourceFile = true;
		for( int i=0; i<udSize; i++ )
			saveUD.push_back(udList[i]->id);
		for( int i=0; i<fSize; i++ )
			saveF.push_back(fList[i]);
		saveSectorSize = TM->sectorXSize*TM->sectorZSize;
	}
	else
		saveResourceFile = false;

	if( !UDGeoResource )
		*l<<"\n  No geo-resource units were detected for this mod.";
	else if( !useResourceFile )
	{
		*l<<"\n  Finding Geo-Sites ...";
		float3 position;
		float3 buildPosition;
		for(int i=0; i<fSize; i++)
		{
			const FeatureDef *fd = cb->GetFeatureDef(fList[i]);
			if( fd->geoThermal )
			{
				position = cb->GetFeaturePos(fList[i]);
				if( !TM->waterIsHarmful || cb->GetElevation(position.x,position.z) >= 0 )
				{
					ResourceSite *RS = new ResourceSite(position,fList[i],fd);
					for( int iud=0; iud<udSize; iud++ )
					{
						const UnitDef* ud = udList[iud];
						if( ud->needGeo )
						{
							buildPosition = cb->ClosestBuildSite(ud,RS->position,48.0f,0);
							if( cb->CanBuildAt(ud,buildPosition) )
								RS->options.insert(ud->id);
						}
					}
					if( RS->options.size() == 0 )
					{
						*l<<"\n  Energy Resource located at (x"<<RS->position.x<<" z"<<RS->position.z<<" y"<<RS->position.y<<") is unusable in this mod.";
						delete RS;
					}
					else
						R[1][RSize[1]++] = RS;
				}
			}
		}
		delete [] fList;
		*l<<"\n   Geo-Sites Found: "<<RSize[1];
	}
	else
	{
		delete [] fList;
		*l<<"\n   Geo-Sites Loaded: "<<RSize[1];
	}

	if( !UDMetalResources )
	{
		*l<<"\n  No metal-resource units were detected for this mod.";
		isMetalMap = false;
		averageMetalSite = 0.0;
	}
	else if( !useResourceFile )
	{
		*l<<"\n  Determining the available amount of metal ...";
		*l<<"\n   GetMaxMetal(): "<<cb->GetMaxMetal();
		// Calculate the radius distances & ranges
		const float MMExtractorRadius = cb->GetExtractorRadius()/16.0;
		MMExtractorRadiusI = MMExtractorRadius;
		*l<<"\n   Metal-Map Extractor Radius: "<<MMExtractorRadius;
		const int MMRSSize = MMExtractorRadiusI*2+1;
		MMRS = new sMMRadiusSquare*[MMRSSize];
		for( int x=0; x<MMRSSize; x++ )
		{
			MMRS[x] = new sMMRadiusSquare[MMRSSize];
			for(int z=0; z<MMRSSize; z++)
			{
				MMRS[x][z].distance = float3(x,0,z).distance2D(float3(MMExtractorRadiusI,0,MMExtractorRadiusI));
				if( MMRS[x][z].distance <= MMExtractorRadius )
					MMRS[x][z].inRange = true;
				else
					MMRS[x][z].inRange = false;
			}
		}

		if( cb->GetExtractorRadius() <= 70.0 )
		{
			int inRange = 0;
			for(int z=0; z<MMRSSize; z++)
				for(int x=0; x<MMRSSize; x++)
					if( MMRS[x][z].inRange )
						inRange++;
			const float minimalMetalSquare = RAI_MinimalMetalSite/(inRange*cb->GetMaxMetal());
			*l<<"\n   Metal-Squares per Site: "<<inRange;
			*l<<"\n   Minimal Metal-Square Value: "<<minimalMetalSquare;

			sector = new MetalMapSector[TM->sectorXSize*TM->sectorZSize];
			const unsigned char *standardMetalMap = cb->GetMetalMap();
			const int convertStoMM = TM->convertStoP/16; // * for conversion, / for reverse conversion
			const int metalMapXSize = TM->sectorXSize*convertStoMM;
			int uselessSectors = 0;
			int metalSectors = 0;
			float totalMetal = 0.0;
//			*l<<"\n   Sector to Metal-Map Conversion: "<<convertStoMM;
			for(int z=0; z < TM->sectorZSize; z++)
				for(int x=0; x < TM->sectorXSize; x++)
				{
//if( x == 0 ) *l<<"\n ";
					float sectorMetal = 0.0;
					int i=(z*TM->sectorXSize)+x;
					sector[i].S = &TM->sector[i];
					if( sector[i].S->maxElevation < 0 && TM->waterIsHarmful )
						uselessSectors++;
					else
					{
						int iMap = ((z*convertStoMM)*metalMapXSize)+x*convertStoMM;
//*l<<"\t"<<standardMetalMap[iMap];
						for(int zM=0; zM<convertStoMM; zM++)
							for(int xM=0,iM=iMap+zM*metalMapXSize+xM; xM<convertStoMM; xM++,iM=iMap+zM*metalMapXSize+xM )
								if( (float)standardMetalMap[iM] > 2.0*minimalMetalSquare )
								{
									sector[i].percentMetal++;
									sectorMetal += standardMetalMap[iM];
								}
						sector[i].percentMetal *= 100.0/(convertStoMM*convertStoMM);
						if( sector[i].percentMetal >= 75.0 )//&& sector[i].totalMetal > 2*RAI_MinimalMetalSite )
						{
							sector[i].isMetalSector = true;
							metalSectors++;
							totalMetal += sectorMetal;
						}
//*l<<"\t("<<sector[i].totalMetal<<"/"<<sector[i].percentMetal<<"%)";
					}
				}
			*l<<"\n   Metal-Sector Percent: "<<(100.0*metalSectors)/(TM->sectorXSize*TM->sectorZSize-uselessSectors)<<"%";
			if( (100.0*metalSectors)/(TM->sectorXSize*TM->sectorZSize-uselessSectors) > 40.0 ) // 40% of the sectors are metal
			{
				isMetalMap = true;
				*l<<" (Metal-Map Detected)";
				averageMetalSite = (inRange*cb->GetMaxMetal()*totalMetal)/(metalSectors*convertStoMM*convertStoMM);
			}
			else
			{
				isMetalMap = false;
				delete [] sector;
				sector = 0;
			}
		}
		else
			isMetalMap = false;

		if( !isMetalMap )
		{
			*l<<"\n  Finding Metal-Sites ...";
			averageMetalSite = 0.0;
			MMZSize = cb->GetMapHeight()/2;
			MMXSize = cb->GetMapWidth()/2;
			*l<<"\n   Metal-Map Size: "<<MMXSize*MMZSize<<" (x"<<MMXSize<<",z"<<MMZSize<<")";
			const float MBtoBB = 2.0; // Metal-Block to Build-Block, * for Conversion, / for the reverse
			const float MMMinExtractorRadius = sqrt(pow(float(udList[0]->xsize)/MBtoBB,2)+pow(float(udList[0]->ysize)/MBtoBB,2)); // If less then this value then sites could overlap
			*l<<"\n   Minimal Metal-Map Extractor Radius: "<<MMMinExtractorRadius;

			// sorts the list so that the most unique size extractors are first
			int uniqueExtractors = 0;
			for( int iudU=0; iudU<=uniqueExtractors; iudU++ )
			{
//*l<<" "<<iudU;
				int uniqueExtractorIndex=-1;
				for( int iud=iudU; iud<udSize && uniqueExtractorIndex==-1; iud++ )
					if( udList[iud]->extractsMetal > 0 )
					{
						uniqueExtractorIndex = iud;
						for( int iud2=0; iud2<uniqueExtractors; iud2++ )
							if( udList[iud]->xsize == udList[iud2]->xsize && udList[iud]->ysize == udList[iud2]->ysize &&
								udList[iud]->minWaterDepth == udList[iud2]->minWaterDepth && udList[iud]->maxWaterDepth == udList[iud2]->maxWaterDepth )
							{
								uniqueExtractorIndex = -1;
								break;
							}
					}
				if( uniqueExtractorIndex >=0 )
				{
//*l<<" +"<<uniqueExtractorIndex;
					const UnitDef* ud = udList[uniqueExtractors];
					udList[uniqueExtractors] = udList[uniqueExtractorIndex];
					udList[uniqueExtractorIndex] = ud;
					uniqueExtractors++;

					// make udList[0] = the smallest extractor
					for( int i=uniqueExtractors-1; i>0 && udList[i]->xsize*udList[i]->ysize < udList[i-1]->xsize*udList[i-1]->ysize; i-- )
					{
						const UnitDef* ud = udList[i-1];
						udList[i-1] = udList[i];
						udList[i] = ud;
					}
				}
			}
			*l<<"\n   Minimal Unit-Definition Size: (x"<<udList[0]->xsize<<",z"<<udList[0]->ysize<<")";
			*l<<"\n   Unique Extractor Unit-Definitions: "<<uniqueExtractors;

			// Calculate the offsets
			edgeOffset = new int[MMRSSize];
			for( int x=0; x<MMRSSize; x++ )
				for(int z=0; z<MMRSSize; z++)
					if( MMRS[x][z].inRange )
					{
						edgeOffset[x] = MMExtractorRadiusI-z;
						break;
					}

			// Initailizing MMS(.metal .assessing .x .z)
			int SMindex;	// temp variable
			float percentMetal = 0.0;
			const unsigned char *StandardMetalMap = cb->GetMetalMap();
			MMS = new sMetalMapSquare*[MMXSize];
			for( int x=0; x<MMXSize; x++ )
			{
//*l<<"\n";
				MMS[x] = new sMetalMapSquare[MMZSize];
				for(int z=0; z<MMZSize; z++)
				{
					SMindex = z*MMXSize + x;
					if( (int)StandardMetalMap[SMindex] > 0 )
					{
						MMS[x][z].metal = (float)StandardMetalMap[SMindex]*cb->GetMaxMetal();
						percentMetal++;
					}
					else
						MMS[x][z].metal = 0.0;
//*l<<MMS[x][z].metal<<"\t";
					MMS[x][z].x = x;
					MMS[x][z].z = z;
					MMS[x][z].assessing = true;
					MMS[x][z].inaccuracy = -1.0;
				}
			}
			percentMetal *= 100.0/(MMXSize*MMZSize);
			*l<<"\n   Percent Metal: "<<percentMetal<<"%";
			bool valueAccuracy = false;
			if( percentMetal < 1.0 )
			{
				valueAccuracy = true;
				*l<<" (metal-site accuracy will be considered important)";
			}

//			double MSTimerTemp = clock();
			// Updates MMS(.totalMetal .assessing)
			FindMMSTotalMetal(0,MMXSize-1,0,MMZSize-1);
//			*l<<"\n    Metal-Site Init FindMMSTotalMetal Loading:\t"<<(clock()-MSTimerTemp)/(double)CLOCKS_PER_SEC<<" seconds";

			// Updates MMS(.assessing), checks for area occupied by Geo-Sites
			if( UDGeoResource )
			{
				// Find the smallest Geo UnitDef
				const UnitDef *ud = 0;
				for( int iud=0; iud<udSize; iud++ )
					if( udList[iud]->needGeo )
						if( ud == 0 || udList[iud]->xsize*udList[iud]->ysize < ud->xsize*ud->ysize )
							ud = udList[iud];

				const int MMtoMP = 16; // Metal-Map to Map-Position, * for conversion, / for the reverse
				int xMin,xMax,zMin,zMax; // temp variables
				for( int iR=0; iR<RSize[1]; iR++ )
				{
					xMin = int(R[1][iR]->position.x)/MMtoMP;
					zMin = int(R[1][iR]->position.z)/MMtoMP;
					SetLimitBoundary(xMin,xMax,ud->xsize/MBtoBB-1,zMin,zMax,ud->ysize/MBtoBB-1);
					for(int z=zMin; z<=zMax; z++)
						for(int x=xMin; x<=xMax; x++)
							if( MMS[x][z].assessing )
								MMS[x][z].assessing = false;
				}
			}

			// Updates MMS(.assessing), checks the elevation and water type
			const float *StandardHeightMap = cb->GetHeightMap();
			const int HeightMapXSize = cb->GetMapWidth();
			const int MMapToHMap = HeightMapXSize/MMXSize;
			MMSAssessingSize = 0;
			float3 position;// temp variable
			for( int x=0; x<MMXSize; x++ )
				for(int z=0; z<MMZSize; z++)
					if( MMS[x][z].assessing )
						if( TM->waterIsHarmful && StandardHeightMap[(z*MMapToHMap+MMapToHMap/2)*HeightMapXSize+(x*MMapToHMap+MMapToHMap/2)] < 0 )
							MMS[x][z].assessing = false;
						else
							MMSAssessingSize++;

			// try cutting it down a little more, if needed
			if( MMSAssessingSize > 250000 )
			{
				*l<<"\n   Assessing "<<MMSAssessingSize<<" possible metal-sites.";
				*l<<"\n    Reducing Assessment ... ";
				for( int x=0; x<MMXSize; x++ )
					for(int z=0; z<MMZSize; z++)
						if( MMS[x][z].assessing && MMS[x][z].totalMetal < 1.75*RAI_MinimalMetalSite )
							MMS[x][z].assessing = false;
			}

//			MSTimerTemp = clock();
			// Updates MMS(.assessing), checks if the positions can be built at
			MMSAssessingSize = 0;
			MMSAssessing = new sMetalMapSquare*[MMXSize*MMZSize];
			for( int x=0; x<MMXSize; x++ )
				for(int z=0; z<MMZSize; z++)
					if( MMS[x][z].assessing )
					{	// Long Calculation: (cause: cb->CanBuildAt - probably can't be improved)
						MMS[x][z].assessing = false;
						position = float3(x*16.0+8.0, 0.0, z*16.0+8.0);
						for( int iud=0; iud<uniqueExtractors; iud++ )
							if( udList[iud]->extractsMetal > 0 && cb->CanBuildAt(udList[iud],position) )
							{
								MMS[x][z].assessing = true;
								MMSAssessing[MMSAssessingSize++] = &MMS[x][z];
								break;
							}
					}
//			*l<<"\n    Metal-Site Init CanBuildAt Loading:\t"<<(clock()-MSTimerTemp)/(double)CLOCKS_PER_SEC<<" seconds";
			*l<<"\n   Assessing "<<MMSAssessingSize<<" possible metal-sites.";
//			double MSTimer1 = 0;
//			double MSTimer2 = 0;
//			double MSTimer3 = 0;
			float searchDis = cb->GetExtractorRadius()/2.0;
			if( searchDis < 16.0 )
				searchDis = 16.0; // Needs to be at least this high to work with ClosestBuildSite
			sMetalMapSquare *mms;	// temp variable
			int xMin,xMax,zMin,zMax,xOffset,zOffset; // temp variables
			while( MMSAssessingSize > 0 && RSize[0] < RAI_MaxMetalSites )
			{
//*l<<"\nMMSRemaining.size()="<<MMSRemaining.size();
//				MSTimerTemp = clock();
				// Setting mms as the best metal-site available
				// Sorted by high totalMetal and then by low inaccuracy
				mms = MMSAssessing[0]; // [0] always has assessing = true
				if( valueAccuracy )
				{
					float bestMetal = 0;
					for( int i=0; i<MMSAssessingSize; i++ )
						if( !MMSAssessing[i]->assessing )
							MMSAssessing[i--] = MMSAssessing[--MMSAssessingSize];
						else if( MMSAssessing[i]->totalMetal < 0.5*bestMetal )
						{}	// nothing
						else
						{	// slow calculations
							if( MMSAssessing[i]->totalMetal > bestMetal )
							{
								bestMetal = MMSAssessing[i]->totalMetal;
								if( mms->totalMetal < 0.51*bestMetal )
									mms = MMSAssessing[i];
							}
							if( MMSAssessing[i]->inaccuracy <= 0.0 )
								FindMMSInaccuracy(MMSAssessing[i]->x,MMSAssessing[i]->z);
							if( mms->inaccuracy <= 0.0 )
								FindMMSInaccuracy(mms->x,mms->z);
//							*l<<"\n m="<<MMSAssessing[i]->totalMetal<<" ia="<<MMSAssessing[i]->inaccuracy<<" r="<<MMSAssessing[i]->totalMetal*(MMSAssessing[i]->totalMetal/MMSAssessing[i]->inaccuracy);
							if( MMSAssessing[i]->totalMetal*(MMSAssessing[i]->totalMetal/MMSAssessing[i]->inaccuracy) > mms->totalMetal*(mms->totalMetal/mms->inaccuracy) )
								mms = MMSAssessing[i];
						}
				}
				else
				{
					for( int i=0; i<MMSAssessingSize; i++ )
						if( !MMSAssessing[i]->assessing )
							MMSAssessing[i--] = MMSAssessing[--MMSAssessingSize];
						else if( MMSAssessing[i]->totalMetal > 1.001*mms->totalMetal )
							mms = MMSAssessing[i];
						else if( MMSAssessing[i]->totalMetal < 0.999*mms->totalMetal )
						{}	// nothing
						else
						{	// 0.999-1.001: fixes float rounding errors, cb->GetMaxMetal() was not a whole number
							// inaccuracy can take a long time to calculate so it won't be until it's needed
							if( MMSAssessing[i]->inaccuracy <= 0.0 )
								FindMMSInaccuracy(MMSAssessing[i]->x,MMSAssessing[i]->z);
							if( mms->inaccuracy <= 0.0 )
								FindMMSInaccuracy(mms->x,mms->z);
							if( MMSAssessing[i]->inaccuracy < mms->inaccuracy )//0.999*mms->inaccuracy )
								mms = MMSAssessing[i];
						}
				}
				if( mms->totalMetal < RAI_MinimalMetalSite )
					break;
//				MSTimer1 += clock()-MSTimerTemp;
//				MSTimerTemp = clock();

				// Create the metal-site
				position = float3(mms->x*16.0+8.0, 0.0, mms->z*16.0+8.0);
				ResourceSite *RS = new ResourceSite(position);
				RS->position.y = StandardHeightMap[(mms->z*MMapToHMap+MMapToHMap/2)*HeightMapXSize+(mms->x*MMapToHMap+MMapToHMap/2)];
				RS->amount = mms->totalMetal;
				averageMetalSite += RS->amount;
				for( int iud=0; iud<udSize; iud++ )
					if( udList[iud]->extractsMetal > 0.0 )
					{
						position = cb->ClosestBuildSite(udList[iud],RS->position,searchDis,0);
						if( cb->CanBuildAt(udList[iud],position) && (!TM->waterIsHarmful || position.y >= 0) )
							RS->options.insert(udList[iud]->id);
					}
				R[0][RSize[0]++] = RS;
//				MSTimer2 += clock()-MSTimerTemp;
//				*l<<"\n mms->(x"<<mms->x<<",z"<<mms->z<<")";

				// The Extractor Radius is small enough that Sites will overlap, remove the nearby indexes being assessed
//				if( MMExtractorRadius < MMMinExtractorRadius )
//				{
				SetLimitBoundary((xMin=mms->x),xMax,udList[0]->xsize/MBtoBB -1,(zMin=mms->z),zMax,udList[0]->ysize/MBtoBB -1);
				for(int z=zMin; z<=zMax; z++)
					for(int x=xMin; x<=xMax; x++)
						if( MMS[x][z].assessing )
							MMS[x][z].assessing = false;
//				}

				// Update nearby Metal-Map positions
				SetLimitBoundary((xMin=mms->x),xMax,xOffset,(zMin=mms->z),zMax,zOffset,MMExtractorRadiusI);
				for(int z=zMin,zMMRS=zOffset; z<=zMax; z++,zMMRS++)
					for(int x=xMin,xMMRS=xOffset; x<=xMax; x++,xMMRS++)
						if( MMRS[xMMRS][zMMRS].inRange )
						{
//							*l<<"\n  (x"<<x<<",z"<<z<<") i="<<index<<" MMSRsize="<<MMSRemaining.size();
							MMS[x][z].metal = 0.0;
						}

//				MSTimerTemp = clock();
				// Recalculate the affected MetalMapSquares
				SetLimitBoundary((xMin=mms->x),xMax,xOffset,(zMin=mms->z),zMax,zOffset,2*MMExtractorRadiusI);
				FindMMSTotalMetal(xMin,xMax,zMin,zMax);
//				MSTimer3 += clock()-MSTimerTemp;

				// Ensures that the first element is being assessed
				while( MMSAssessingSize > 0 && !MMSAssessing[0]->assessing )
					MMSAssessing[0] = MMSAssessing[--MMSAssessingSize];
			}
//			*l<<"\n    Metal-Site Search Loading:            "<<MSTimer1/CLOCKS_PER_SEC<<" seconds";
//			*l<<"\n    Metal-Site CanBuildAt Loading:        "<<MSTimer2/CLOCKS_PER_SEC<<" seconds";
//			*l<<"\n    Metal-Site FindMMSTotalMetal Loading: "<<MSTimer3/CLOCKS_PER_SEC<<" seconds";
			delete [] edgeOffset;
			delete [] MMSAssessing;
			for( int x=0; x<MMXSize; x++ )
				delete [] MMS[x];
			delete [] MMS;
			if( RSize[0] > 0 )
			{
				averageMetalSite /= RSize[0];
				*l<<"\n    Minimal Metal-Site Value: "<<RAI_MinimalMetalSite;
			}
		}
		for( int x=0; x<MMRSSize; x++ )
			delete [] MMRS[x];
		delete [] MMRS;
		*l<<"\n    Average Metal-Site Value: "<<averageMetalSite;
		*l<<"\n    Metal-Sites Found: "<<RSize[0];
	}
	else
		*l<<"\n    Metal-Sites Loaded: "<<RSize[0];
	delete [] udList;
/*
	// debugging
	for( int iT=0; iT<2; iT++ )
		for( int iR=0; iR<RSize[iT]; iR++ )
		{
			*l<<"\n    R["<<iT<<"]["<<iR<<"] \tamount: "<<R[iT][iR]->amount<<" \tlocation(x"<<R[iT][iR]->position.x<<",z"<<R[iT][iR]->position.z<<"):";
			*l<<"\t B("<<R[iT][iR]->options.size()<<"):";
			for( set<int>::iterator RS=R[iT][iR]->options.begin(); RS!=R[iT][iR]->options.end(); RS++ )
				*l<<" "<<*RS;
		}
*/
}

GlobalResourceMap::~GlobalResourceMap()
{
	if( saveResourceFile )
	{
		FILE *resourceFile = fopen(resourceFileName.c_str(),"wb");
		int size;
		fwrite(&(size=saveUD.size()), sizeof(int), 1, resourceFile);
		for(vector<int>::iterator i=saveUD.begin(); i!=saveUD.end(); i++)
			fwrite(&*i, sizeof(int), 1, resourceFile);
		fwrite(&(size=saveF.size()), sizeof(int), 1, resourceFile);
		for(vector<int>::iterator i=saveF.begin(); i!=saveF.end(); i++)
			fwrite(&*i, sizeof(int), 1, resourceFile);
		for( int iT=0; iT<2; iT++ )
		{
			fwrite(&RSize[iT], sizeof(int), 1, resourceFile);
			for( int iR=0; iR<RSize[iT]; iR++ )
			{
				fwrite(&R[iT][iR]->featureID, sizeof(int), 1, resourceFile);
				fwrite(&R[iT][iR]->position, sizeof(float3), 1, resourceFile);
				fwrite(&R[iT][iR]->amount, sizeof(float), 1, resourceFile);
				fwrite(&(size=R[iT][iR]->options.size()), sizeof(int), 1, resourceFile);
				for( set<int>::iterator i=R[iT][iR]->options.begin(); i!=R[iT][iR]->options.end(); i++ )
					fwrite(&*i, sizeof(int), 1, resourceFile);
			}
		}
		for( int iT=0; iT<2; iT++ )
			for( int iR=0; iR<RSize[iT]; iR++ )
			{
				fwrite(&(size=R[iT][iR]->siteDistance.size()), sizeof(int), 1, resourceFile);
				for( map<ResourceSite*,ResourceSiteDistance>::iterator iRS=R[iT][iR]->siteDistance.begin(); iRS!=R[iT][iR]->siteDistance.end(); iRS++ )
				{
					fwrite(&iRS->first->type, sizeof(int), 1, resourceFile);
					ResourceSiteDistance* RSD = &iRS->second;
					for(int i=0; i<RSize[iRS->first->type]; i++)
						if( R[iRS->first->type][i] == iRS->first )
						{
							fwrite(&i, sizeof(int), 1, resourceFile);
							break;
						}
					fwrite(&RSD->minDistance, sizeof(float), 1, resourceFile);
					if( RSD->bestPathType == -1 && RSD->bestDistance != 0 )
						RSD->bestPathType = -2;
					fwrite(&RSD->bestPathType, sizeof(int), 1, resourceFile);
					fwrite(&(size=RSD->distance.size()), sizeof(int), 1, resourceFile);
					for( map<int,float>::iterator i=RSD->distance.begin(); i!=RSD->distance.end(); i++ )
					{
						fwrite(&i->first, sizeof(int), 1, resourceFile);
						fwrite(&i->second, sizeof(float), 1, resourceFile);
					}
					fwrite(&(size=RSD->pathDebug.size()), sizeof(int), 1, resourceFile);
					for( vector<float3>::iterator i=RSD->pathDebug.begin(); i!=RSD->pathDebug.end(); i++ )
						fwrite(&*i, sizeof(float3), 1, resourceFile);
				}
			}
		fwrite(&averageMetalSite, sizeof(float), 1, resourceFile);
		fwrite(&isMetalMap, sizeof(bool), 1, resourceFile);
		if( isMetalMap )
			for( int iS=0; iS<saveSectorSize; iS++ )
				fwrite(&sector[iS].isMetalSector, sizeof(bool), 1, resourceFile);
		fclose(resourceFile);
	}

	if( sector != 0 )
		delete [] sector;
	for( int iT=0; iT<2; iT++ )
	{
		for(int iR=0; iR<RSize[iT]; iR++)
			delete R[iT][iR];
		delete [] R[iT];
	}
}

float3 GlobalResourceMap::GetMetalMapPosition(const float3& position)
{	// UNFINISHED
	return position;
}

void GlobalResourceMap::SetLimitBoundary(int &xMin, int &xMax, int &xMMRS, int &zMin, int &zMax, int &zMMRS, const int &increment)
{
	xMax = xMin + increment;
	if( xMax > MMXSize-1 )
		xMax = MMXSize-1;
	xMin -= increment;
	if( xMin < 0 )
	{
		xMMRS = -xMin;
		xMin = 0;
	}
	else
		xMMRS = 0;

	zMax = zMin + increment;
	if( zMax > MMZSize-1 )
		zMax = MMZSize-1;
	zMin -= increment;
	if( zMin < 0 )
	{
		zMMRS = -zMin;
		zMin = 0;
	}
	else
		zMMRS = 0;
}

void GlobalResourceMap::SetLimitBoundary(int &xMin, int &xMax, const int &xIncrement, int &zMin, int &zMax, const int &zIncrement)
{
	xMax = xMin + xIncrement;
	if( xMax > MMXSize-1 )
		xMax = MMXSize-1;
	xMin -= xIncrement;
	if( xMin < 0 )
		xMin = 0;

	zMax = zMin + zIncrement;
	if( zMax > MMZSize-1 )
		zMax = MMZSize-1;
	zMin -= zIncrement;
	if( zMin < 0 )
		zMin = 0;
}

void GlobalResourceMap::FindMMSTotalMetal(const int &xMMin, const int &xMMax, const int &zMMin, const int &zMMax)
{	// Updating MMS (.totalMetal .assessing)
	int xMin,xMax,zMin,zMax,xOffset,zOffset,xMMRS,zMMRS,x,z; // temp variables
	for(int xM=xMMin; xM<=xMMax; xM++)
		for(int zM=zMMin; zM<=zMMax; zM++)
			if( MMS[xM][zM].assessing )
			{
				MMS[xM][zM].totalMetal = 0.0;
				MMS[xM][zM].inaccuracy = -1.0; // reset
				SetLimitBoundary((xMin=xM),xMax,xOffset,(zMin=zM),zMax,zOffset,MMExtractorRadiusI);
				if( xM > xMMin && MMS[xM-1][zM].assessing )
				{	// Shifting right from a previous calculation
					MMS[xM][zM].totalMetal = MMS[xM-1][zM].totalMetal;
					for(z=zMin; z<=zMax; z++,zOffset++)
					{
						xMax = xM+edgeOffset[zOffset];
						if( xMax < MMXSize )
							MMS[xM][zM].totalMetal += MMS[xMax][z].metal;
						xMin = xM-edgeOffset[zOffset]-1;
						if( xMin >= 0 )
							MMS[xM][zM].totalMetal -= MMS[xMin][z].metal;
					}
				}
				else if( zM > zMMin && MMS[xM][zM-1].assessing )
				{	// Shifting down from a previous calculation
					MMS[xM][zM].totalMetal = MMS[xM][zM-1].totalMetal;
					for(x=xMin; x<=xMax; x++,xOffset++)
					{
						zMax = zM+edgeOffset[xOffset];
						if( zMax < MMZSize )
							MMS[xM][zM].totalMetal += MMS[x][zMax].metal;
						zMin = zM-edgeOffset[xOffset]-1;
						if( zMin >= 0 )
							MMS[xM][zM].totalMetal -= MMS[x][zMin].metal;
					}
				}
				else
				{	// Default Calculation, depending on the extractor radius this can be 30x slower
					for(z=zMin,zMMRS=zOffset; z<=zMax; z++,zMMRS++)
						for(x=xMin,xMMRS=xOffset; x<=xMax; x++,xMMRS++)
							if( MMRS[xMMRS][zMMRS].inRange )
								MMS[xM][zM].totalMetal += MMS[x][z].metal;
				}
//*l<<" MMS[xM][zM].totalMetal="<<MMS[xM][zM].totalMetal;
			}
	// .assessing is checked afterwards because assessing=true can help with it's calculations
	for(int xM=xMMin; xM<=xMMax; xM++)
		for(int zM=zMMin; zM<=zMMax; zM++)
			if( MMS[xM][zM].assessing && MMS[xM][zM].totalMetal < RAI_MinimalMetalSite )
				MMS[xM][zM].assessing = false;
}

void GlobalResourceMap::FindMMSInaccuracy(const int &xM, const int &zM)
{
	// Very Long Calculation: (cause: a map with a large extractor radius - needs some improvement in the future)
	int xMin,xMax,zMin,zMax,xOffset,zOffset; // temp variables
	SetLimitBoundary((xMin=xM),xMax,xOffset,(zMin=zM),zMax,zOffset,MMExtractorRadiusI);
	for(int z=zMin,zMMRS=zOffset; z<=zMax; z++,zMMRS++)
		for(int x=xMin,xMMRS=xOffset; x<=xMax; x++,xMMRS++)
			if( MMRS[xMMRS][zMMRS].inRange )
				MMS[xM][zM].inaccuracy += MMS[x][z].metal * (1.0+MMRS[xMMRS][zMMRS].distance/3.0);
}
