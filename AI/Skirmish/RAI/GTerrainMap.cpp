#include "GTerrainMap.h"
#include "RAI.h"
#include <set>
#include "LegacyCpp/MoveData.h"
#include "System/StringUtil.h"
#include "CUtils/Util.h"
#include <stdexcept>
//#include <time.h>
using std::deque;
using std::set;


template<typename T>
static inline void file_read(T* value, FILE* file)
{
	const size_t readCount = fread(value, sizeof(T), 1, file);
	if (readCount != 1) {
		throw std::runtime_error("failed reading from file");
	}
}

template<typename T>
static inline void file_write(const T* value, FILE* file)
{
	const size_t writeCount = fwrite(value, sizeof(T), 1, file);
	if (writeCount != 1) {
		throw std::runtime_error("failed writing to file");
	}
}

GlobalTerrainMap::GlobalTerrainMap(IAICallback* cb, cLogFile* l)
{
	const int mapFileVersion = 3;

	*l<<"\n Loading the Terrain-Map ...";

	// Reading the WaterDamage entry from the map file
	
	waterIsHarmful = false;
	waterIsAVoid = false;

	string relMapFileName = "cache/";
	relMapFileName += cRAI::MakeFileSystemCompatible(cb->GetMapName());
	relMapFileName += "-" + IntToString(cb->GetMapHash(), "%x");
	relMapFileName += ".res";

	string mapFileName_r;
	FILE* mapFile_r = NULL;
	// NOTE: cRAI::LocateFile() returns TRUE for _absent_ file under WIN32
	if (cRAI::LocateFile(cb, relMapFileName, mapFileName_r, false)) {
		mapFile_r = fopen(mapFileName_r.c_str(), "rb");
	}
	
	if (mapFile_r == NULL) {
		*l<<"\n  Could not find cache file for reading: " << relMapFileName;
	}

	bool mapFileLoaded = false;
	if( mapFile_r )
	{
		int version;

		try
		{
			file_read(&version, mapFile_r);

			if( version == mapFileVersion )
			{
				file_read(&waterIsHarmful, mapFile_r);
				file_read(&waterIsAVoid,   mapFile_r);
				mapFileLoaded = true;
			}
			else
			{
				*l<<"\n  The cached map-file is using a different version format, reloading...";
			}
		}
		catch (const std::exception& ex)
		{
			*l<<"\nERROR: failed reading the terrain map: " << ex.what();
		}

		fclose(mapFile_r);
	}
	
	if( !mapFileLoaded )
	{
//		double mapArchiveTimer = clock();
		string mapArchiveFileName = "maps/";
		mapArchiveFileName += cRAI::MakeFileSystemCompatible(cb->GetMapName());
		mapArchiveFileName += ".smd";

		int mapArchiveFileSize = cb->GetFileSize(mapArchiveFileName.c_str());
		if( mapArchiveFileSize > 0 )
		{
			*l<<"\n  Searching the Map-Archive File: '" << mapArchiveFileName << "'  File Size: "<<mapArchiveFileSize;
			char *archiveFile = new char[mapArchiveFileSize];
			cb->ReadFile(mapArchiveFileName.c_str(),archiveFile,mapArchiveFileSize);
			int waterDamage = GetFileValue(mapArchiveFileSize,archiveFile,"WaterDamage");
			waterIsAVoid = GetFileValue(mapArchiveFileSize,archiveFile,"VoidWater") > 0;
			*l<<"\n   Void Water: "<<waterIsAVoid;
			if( waterIsAVoid )
				*l<<" (This map has no water)";
			*l<<"\n   Water Damage: "<<waterDamage;
			if( waterDamage > 0 )
			{
				waterIsHarmful = true;
				*l<<" (This map's water is harmful to land units";
				if( waterDamage > 10000 )
				{
					waterIsAVoid = true; // UNTESTED
					*l<<" as well as hovercraft";
				}
				*l<<")";
			}
			delete [] archiveFile;
		}
		else
		{
			*l<<"\n  Could not find Map-Archive file for reading additional map info: " << mapArchiveFileName;
		}

//		*l<<"\n  Map-Archive Timer: "<<(clock()-mapArchiveTimer)/CLOCKS_PER_SEC<<" seconds";

		string mapFileName_w;
		FILE* mapFile_w = NULL;
		if (cRAI::LocateFile(cb, relMapFileName, mapFileName_w, true)) {
			mapFile_w = fopen(mapFileName_w.c_str(), "wb");
		}

		if (mapFile_w) {
			try
			{
				file_write(&mapFileVersion, mapFile_w);
				file_write(&waterIsHarmful, mapFile_w);
				file_write(&waterIsAVoid,   mapFile_w);
			}
			catch (const std::exception& ex)
			{
				*l<<"\nERROR: failed writing the terrain map: " << ex.what();
			}
			fclose(mapFile_w);
		} else {
			*l<<"\nERROR: Could not write to file: "<<relMapFileName;
		}
	}

	convertStoP = 64; // = 2^x, should not be less than 16
	if( (cb->GetMapWidth()/64)*(cb->GetMapHeight()/64) < 8*8 )
		convertStoP /= 2; // Smaller Sectors, more detailed analysis
//	else if( (cb->GetMapWidth()/16)*(cb->GetMapHeight()/16) > 20*20 )
//		convertStoP *= 2; // Larger Sectors, less detailed analysis
	sectorXSize = (8*cb->GetMapWidth())/convertStoP;
	sectorZSize = (8*cb->GetMapHeight())/convertStoP;
	
	sectorAirType = new TerrainMapAreaSector[sectorXSize*sectorZSize];
	
	*l<<"\n  Sector-Map Block Size: "<<convertStoP;
	*l<<"\n  Sector-Map Size: "<<sectorXSize*sectorZSize<<" (x"<<sectorXSize<<",z"<<sectorZSize<<")";

	// MoveType Detection and TerrainMapMobileType Initialization
	const UnitDef **udList = new const UnitDef*[cb->GetNumUnitDefs()];
	cb->GetUnitDefList(udList);
	int udSize=cb->GetNumUnitDefs();
	typedef pair<int,TerrainMapMobileType*> itmPair; // used to access mobileType
	typedef pair<int,TerrainMapImmobileType*> itiPair; // used to access immobileType

	for( int iud=0; iud<udSize; iud++ )
		if( udList[iud] != 0 )
		{
			if( udList[iud]->canfly )
				udMobileType.insert(itmPair(udList[iud]->id,0));
			else if( udList[iud]->movedata == 0 )
			{
				TerrainMapImmobileType* IT = 0;
				for(list<TerrainMapImmobileType>::iterator iIT = immobileType.begin(); iIT != immobileType.end(); ++iIT )
					if( iIT->maxElevation == -udList[iud]->minWaterDepth && iIT->canHover == udList[iud]->canhover && iIT->canFloat == udList[iud]->floater )
						if( iIT->minElevation == -udList[iud]->maxWaterDepth ||
							((iIT->canHover || iIT->canFloat) && iIT->minElevation <= 0 && -udList[iud]->maxWaterDepth <= 0) )
						{
							IT = &*iIT;
							break;
						}
				if( IT == 0 )
				{
					TerrainMapImmobileType IT2;
					immobileType.push_back(IT2);
					IT = &immobileType.back();
					IT->maxElevation = -udList[iud]->minWaterDepth;
					IT->minElevation = -udList[iud]->maxWaterDepth;
					IT->canHover = udList[iud]->canhover;
					IT->canFloat = udList[iud]->floater;
				}
				IT->udSize++;
				udImmobileType.insert(itiPair(udList[iud]->id,IT));
			}
			else
			{
				MoveData* md = udList[iud]->movedata;
				TerrainMapMobileType* MT = 0;
				for( list<TerrainMapMobileType>::iterator iMT=mobileType.begin(); iMT!=mobileType.end(); ++iMT )
					if( iMT->maxElevation == -udList[iud]->minWaterDepth && iMT->maxSlope == md->maxSlope &&
						iMT->canHover == udList[iud]->canhover && iMT->canFloat == udList[iud]->floater )
						if( iMT->minElevation == -md->depth ||
							((iMT->canHover || iMT->canFloat) && iMT->minElevation <= 0 && -udList[iud]->maxWaterDepth <= 0) )
						{
							MT = &*iMT;
							break;
						}
				if( MT == 0 )
				{
					TerrainMapMobileType MT2;
					mobileType.push_back(MT2);
					MT = &mobileType.back();
					MT->maxSlope = md->maxSlope;
					MT->maxElevation = -udList[iud]->minWaterDepth;
					MT->minElevation = -md->depth;
					MT->canHover = udList[iud]->canhover;
					MT->canFloat = udList[iud]->floater;
					MT->sector = new TerrainMapAreaSector[sectorXSize*sectorZSize];
					MT->MD = md;
//					MT->PF = new GlobalPathfinder();
//					MT->PF->Init(cb,l,MT->maxElevation,-md->depth,md->maxSlope,MT->canHover,MT->canFloat,waterIsHarmful);
//					vector<float3> path;
//					float3 start = float3(20.0,0,20.0);
//					float3 end = float3(80.0,0,80.0);
//					*l<<" distance="; *l<<MT->PF->FindPath(path,start,end);
				}
				MT->udSize++;
				udMobileType.insert(itmPair(udList[iud]->id,MT));
				if( MT->MD->crushStrength < udList[iud]->movedata->crushStrength )
					MT->MD = udList[iud]->movedata; // figured it would be easier on the pathfinder
			}
		}
	delete [] udList;

	if( waterIsAVoid )
	{
		if( mobileType.empty() ) // Work-Around(Mod FF): buildings use canFloat instead of canFly to represent their ability to fly in space
			waterIsAVoid = false;
		else
			waterIsHarmful = true; // If there is no water then this will prevent water units from being used
	}

	// Special types
	landSectorType = 0;
	waterSectorType = 0;
	for(list<TerrainMapImmobileType>::iterator iMT = immobileType.begin(); iMT != immobileType.end(); ++iMT )
		if( !iMT->canFloat && !iMT->canHover )
		{
			if( iMT->minElevation == 0 && (landSectorType == 0 || iMT->maxElevation > landSectorType->maxElevation ) )
				landSectorType = &*iMT;
			if( iMT->maxElevation == 0 && (waterSectorType == 0 || iMT->minElevation < waterSectorType->minElevation ) )
				waterSectorType = &*iMT;
		}
	if( landSectorType == 0 )
	{
		immobileType.push_back(TerrainMapImmobileType());
		landSectorType = &immobileType.back();
		immobileType.back().maxElevation = 1e7;
		immobileType.back().minElevation = 0;
		immobileType.back().canFloat = false;
		immobileType.back().canHover = false;
	}
	if( waterSectorType == 0 )
	{
		immobileType.push_back(TerrainMapImmobileType());
		waterSectorType = &immobileType.back();
		immobileType.back().maxElevation = 0;
		immobileType.back().minElevation = -1e7;
		immobileType.back().canFloat = false;
		immobileType.back().canHover = false;
	}

	*l<<"\n  Determining Usable Terrain for all units ...";
	// Setting sector & determining sectors for immobileType
	sector = new TerrainMapSector[sectorXSize*sectorZSize];
	const float *standardSlopeMap = cb->GetSlopeMap();
	const float *standardHeightMap = cb->GetHeightMap();
	const int convertStoSM = convertStoP/16; // * for conversion, / for reverse conversion
	const int convertStoHM = convertStoP/8; // * for conversion, / for reverse conversion
	const int slopeMapXSize = sectorXSize*convertStoSM;
	const int heightMapXSize = sectorXSize*convertStoHM;
	
	typedef pair<int,TerrainMapSector*> itdPair;
	
	minElevation=0;
	percentLand=0.0;

	for(int z=0; z < sectorZSize; z++)
		for(int x=0; x < sectorXSize; x++)
		{
			int i=(z*sectorXSize)+x;

			sector[i].position.x = x*convertStoP+convertStoP/2; // Center position of the Block
			sector[i].position.z = z*convertStoP+convertStoP/2; //
			sector[i].position.y = cb->GetElevation(sector[i].position.x,sector[i].position.z);
			
			sectorAirType[i].S = &sector[i];
			
			for(list<TerrainMapMobileType>::iterator iMT = mobileType.begin(); iMT != mobileType.end(); ++iMT )
				iMT->sector[i].S = &sector[i];

			int iMap = ((z*convertStoSM)*slopeMapXSize)+x*convertStoSM;
			for(int zS=0; zS<convertStoSM; zS++)
				for(int xS=0,iS=iMap+zS*slopeMapXSize+xS; xS<convertStoSM; xS++,iS=iMap+zS*slopeMapXSize+xS )
					if( sector[i].maxSlope < standardSlopeMap[iS] )
						sector[i].maxSlope = standardSlopeMap[iS];

			iMap = ((z*convertStoHM)*heightMapXSize)+x*convertStoHM;
			sector[i].minElevation = standardHeightMap[iMap];
			sector[i].maxElevation = standardHeightMap[iMap];
			
			for(int zH=0; zH<convertStoHM; zH++)
			{
				for(int xH=0,iH=iMap+zH*heightMapXSize+xH; xH<convertStoHM; xH++,iH=iMap+zH*heightMapXSize+xH )
				{
					if( standardHeightMap[iH] >= 0 )
					{
						sector[i].percentLand++;
						percentLand++;
					}
					
					if( sector[i].minElevation > standardHeightMap[iH] )
					{
						sector[i].minElevation = standardHeightMap[iH];
						if( minElevation > standardHeightMap[i] )
							minElevation = standardHeightMap[i];
					}
					else if( sector[i].maxElevation < standardHeightMap[iH] )
						sector[i].maxElevation = standardHeightMap[iH];
				}
			}

			sector[i].percentLand *= 100.0 / (convertStoHM*convertStoHM);
			
			if( sector[i].percentLand > 50.0 )
				sector[i].isWater = false;
			else
				sector[i].isWater = true;

			for(list<TerrainMapImmobileType>::iterator iMT = immobileType.begin(); iMT != immobileType.end(); ++iMT )
				if( (iMT->canHover && iMT->maxElevation >= sector[i].maxElevation && !waterIsAVoid ) ||
					(iMT->canFloat && iMT->maxElevation >= sector[i].maxElevation && !waterIsHarmful ) ||
					(iMT->minElevation <= sector[i].minElevation && iMT->maxElevation >= sector[i].maxElevation && (!waterIsHarmful || sector[i].minElevation >=0 ) ) )
					iMT->sector.insert(itdPair(i,&sector[i]));
		}
	
	percentLand *= 100.0/(sectorXSize*convertStoHM*sectorZSize*convertStoHM);

	for(list<TerrainMapImmobileType>::iterator iMT = immobileType.begin(); iMT != immobileType.end(); ++iMT )
		if( (100.0*iMT->sector.size())/float(sectorXSize*sectorZSize) >= 20.0 || (double)convertStoP*convertStoP*iMT->sector.size() >= 1.8e7 )
			iMT->typeUsable = true;
		else
			iMT->typeUsable = false;

	*l<<"\n   Map Land Percent: " << percentLand<<"%";
	if (percentLand < 85.0f) {
		*l<<"\n   Water is a void: " << waterIsAVoid;
		*l<<"\n   Water is harmful: " << waterIsHarmful;
	}
	*l<<"\n   Minimum Elevation: " << minElevation;

	for(list<TerrainMapImmobileType>::iterator iMT = immobileType.begin(); iMT != immobileType.end(); ++iMT )
	{
		*l<<"\n   Immobile-Type: Min/Max Elevation=(";
		if( iMT->canHover )
			*l<<"hover";
		else if( iMT->canFloat || iMT->minElevation < -10000 )
			*l<<"any";
		else
			*l<<iMT->minElevation;
		*l<<" / ";
		if( iMT->maxElevation < 10000 )
			*l<<iMT->maxElevation;
		else
			*l<<"any";
		*l<<")   \tIs buildable across "<<(100.0*iMT->sector.size())/(sectorXSize*sectorZSize)<<"% of the map. (used by "<<iMT->udSize<<" unit-defs)"; //<<" "<<iMT->sector.size()<<" sector(s)";
	}
/*
	*l<<"\n  Displaying the Standard Land/Water Area(s) ...";
	for( int i=0; i<areaIUSize; i++ )
	{
		*l<<"\n   ";
		*l<<"("<<i+1<<"), occupying "<<areaIU[i]->percentOfMap<<"% of the map ("<<areaIU[i]->percentMetal<<"% Metal) ";
		if( areaIU[i]->isMetalMap ) *l<<"(MetalMap)";
		*l<<", has been detected with "<<areaIU[i]->sector.size()<<" sectors.";
	}
*/
	const size_t MAMinimalSectors = 8;          // Minimal # of sector for a valid MapArea
	const float MAMinimalSectorPercent = 0.5;   // Minimal % of map for a valid MapArea
	for( list<TerrainMapMobileType>::iterator iMT=mobileType.begin(); iMT!=mobileType.end(); ++iMT )
	{
		*l<<"\n   Mobile-Type: Min/Max Elevation=(";
		if( iMT->canFloat )
			*l<<"any";
		else if( iMT->canHover )
			*l<<"hover";
		else
			*l<<iMT->minElevation;
		*l<<" / ";
		if( iMT->maxElevation < 10000 )
			*l<<iMT->maxElevation;
		else
			*l<<"any";
		*l<<")\tMax Slope=("<<iMT->maxSlope;
		*l<<")   \tMove-Data used:'"<<iMT->MD->name<<"'";

		deque<int> sectorSearch;
		set<int> sectorsRemaining;
		for(int iS=0; iS<sectorZSize*sectorXSize; iS++)
			if( (iMT->canHover && iMT->maxElevation >= sector[iS].maxElevation && !waterIsAVoid && (sector[iS].maxElevation <= 0 || iMT->maxSlope >= sector[iS].maxSlope) ) ||
				(iMT->canFloat && iMT->maxElevation >= sector[iS].maxElevation && !waterIsHarmful && (sector[iS].maxElevation <= 0 || iMT->maxSlope >= sector[iS].maxSlope) ) ||
				(iMT->maxSlope >= sector[iS].maxSlope && iMT->minElevation <= sector[iS].minElevation && iMT->maxElevation >= sector[iS].maxElevation && (!waterIsHarmful || sector[iS].minElevation >=0 ) ) )
				sectorsRemaining.insert(iS);
		int i,iX,iZ,aIndex = 0; // Temp Var.
		while( int(sectorsRemaining.size())>0 || int(sectorSearch.size())>0 )
		{
			if( int(sectorSearch.size()) > 0 )
			{
				i = sectorSearch.front();
				iMT->area[aIndex]->sector.insert(iasPair(i,&iMT->sector[i]));
				iX = i%sectorXSize;
				iZ = i/sectorXSize;
				if( sectorsRemaining.find(i-1)!=sectorsRemaining.end() && iX > 0 ) // Search left
				{
					sectorSearch.push_back(i-1);
					sectorsRemaining.erase(i-1);
				}
				if( sectorsRemaining.find(i+1)!=sectorsRemaining.end() && iX < sectorXSize-1 ) // Search right
				{
					sectorSearch.push_back(i+1);
					sectorsRemaining.erase(i+1);
				}
				if( sectorsRemaining.find(i-sectorXSize)!=sectorsRemaining.end() && iZ > 0 ) // Search up
				{
					sectorSearch.push_back(i-sectorXSize);
					sectorsRemaining.erase(i-sectorXSize);
				}
				if( sectorsRemaining.find(i+sectorXSize)!=sectorsRemaining.end() && iZ < sectorZSize-1 ) // Search down
				{
					sectorSearch.push_back(i+sectorXSize);
					sectorsRemaining.erase(i+sectorXSize);
				}
				sectorSearch.pop_front();
			}
			else
			{
				if( iMT->areaSize > 0 &&
					( iMT->areaSize == 50 ||
					  iMT->area[iMT->areaSize-1]->sector.size() <= MAMinimalSectors ||
					  100.*float(iMT->area[iMT->areaSize-1]->sector.size())/float(sectorXSize*sectorZSize) <= MAMinimalSectorPercent ) )
				{
					// Too mand areas detected.  Find, erase & ignore the smallest one that was found so far
					if( iMT->areaSize == 50 )
						*l<<"\nWARNING: The MapArea limit has been reached (possible error).";
					aIndex=0;
					for( int iA=1; iA<iMT->areaSize; iA++ )
						if( iMT->area[iA]->sector.size() < iMT->area[aIndex]->sector.size() )
							aIndex = iA;
					delete iMT->area[aIndex];
					iMT->areaSize--;
				}
				else
					aIndex=iMT->areaSize;

				i=*sectorsRemaining.begin();
				sectorSearch.push_back(i);
				sectorsRemaining.erase(i);
				iMT->area[aIndex] = new TerrainMapArea(aIndex,&*iMT);
				iMT->areaSize++;
			}
		}
		if( iMT->areaSize > 0 &&
			( iMT->area[iMT->areaSize-1]->sector.size() <= MAMinimalSectors ||
			  100.0*float(iMT->area[iMT->areaSize-1]->sector.size())/float(sectorXSize*sectorZSize) <= MAMinimalSectorPercent ) )
		{
			iMT->areaSize--;
			delete iMT->area[iMT->areaSize];
		}

		// Calculations
		float percentOfMap = 0.0;
		for( int iA=0; iA<iMT->areaSize; iA++ )
		{
			for( map<int,TerrainMapAreaSector*>::iterator iS=iMT->area[iA]->sector.begin(); iS!=iMT->area[iA]->sector.end(); ++iS )
				iS->second->area = iMT->area[iA];
			iMT->area[iA]->percentOfMap = (100.0*iMT->area[iA]->sector.size())/(sectorXSize*sectorZSize);
			if( iMT->area[iA]->percentOfMap >= 20.0 ) // A map area occupying 20% of the map
			{
				iMT->area[iA]->areaUsable = true;
				iMT->typeUsable = true;
			}
			else
				iMT->area[iA]->areaUsable = false;
			if( iMT->areaLargest == 0 || iMT->areaLargest->percentOfMap < iMT->area[iA]->percentOfMap )
				iMT->areaLargest = iMT->area[iA];

			percentOfMap += iMT->area[iA]->percentOfMap;
		}
		*l<<"  \tHas "<<iMT->areaSize<<" Map-Area(s) occupying "<<percentOfMap<<"% of the map. (used by "<<iMT->udSize<<" unit-defs)";
	}
/*
	// Debugging
	l = new cLogFile("TerrainMapDebug.log",false);
	for( int iS=0; iS<sectorXSize*sectorZSize; iS++ )
	{
		if( iS % sectorXSize == 0 ) *l<<"\n";
		if( sector[iS].maxElevation < 0.0 ) *l<<"~";
		else if( sector[iS].maxSlope > 0.5 ) *l<<"^";
		else if( sector[iS].maxSlope > 0.25 ) *l<<"#";
		else *l<<"x";
	}
	for( list<TerrainMapMobileType>::iterator iM=mobileType.begin(); iM!=mobileType.end(); iM++ )
	{
		*l<<"\n\n "<<iM->MD->name<<" h="<<iM->canHover<<" f="<<iM->canFloat<<" mb="<<iM->areaSize;
		for( int iS=0; iS<sectorXSize*sectorZSize; iS++ )
		{
			if( iS % sectorXSize == 0 ) *l<<"\n";
			if( iM->sector[iS].area != 0 ) *l<<"*";
			else if( sector[iS].maxElevation < 0.0 ) *l<<"~";
			else if( sector[iS].maxSlope > 0.5 ) *l<<"^";
			else *l<<"x";
		}
	}
	*l<<"\n";
*/
}

GlobalTerrainMap::~GlobalTerrainMap()
{
	delete [] sector;
	delete [] sectorAirType;
}

bool GlobalTerrainMap::CanMoveToPos(TerrainMapArea *area, const float3& destination)
{
	int iS = GetSectorIndex(destination);
	if( !IsSectorValid(iS) )
		return false;
	if( area == 0 ) // either a flying unit or a unit was somehow created at an impossible position
		return true;
	if( area == GetSectorList(area)[iS].area )
		return true;
	return false;
}

TerrainMapAreaSector* GlobalTerrainMap::GetSectorList(TerrainMapArea* sourceArea)
{
	if( sourceArea == 0 || sourceArea->mobileType == 0 ) // It flies or it's immobile
		return sectorAirType;
	return sourceArea->mobileType->sector;
}

TerrainMapAreaSector* GlobalTerrainMap::GetClosestSector(TerrainMapArea* sourceArea, const int& destinationSIndex)
{
	map<int,TerrainMapAreaSector*>::iterator iAS = sourceArea->sectorClosest.find(destinationSIndex);
	if( iAS != sourceArea->sectorClosest.end() ) // It's already been determined
		return iAS->second;
//*l<<"\n GCAS";
	TerrainMapAreaSector* TMSectors = GetSectorList(sourceArea);
	if( sourceArea == TMSectors[destinationSIndex].area )
	{
		sourceArea->sectorClosest.insert(iasPair(destinationSIndex,&TMSectors[destinationSIndex]));
//*l<<"1(#)";
		return &TMSectors[destinationSIndex];
	}

	float3 *destination = &TMSectors[destinationSIndex].S->position;
	TerrainMapAreaSector* SClosest = 0;
	float DisClosest = 0.0f;
	for( map<int,TerrainMapAreaSector*>::iterator iS=sourceArea->sector.begin(); iS!=sourceArea->sector.end(); ++iS )
		if( SClosest == 0 || iS->second->S->position.distance(*destination) < DisClosest )
		{
			SClosest = iS->second;
			DisClosest = iS->second->S->position.distance(*destination);
		}
	sourceArea->sectorClosest.insert(iasPair(destinationSIndex,SClosest));
//*l<<"2(#)";
	return SClosest;
}

TerrainMapSector* GlobalTerrainMap::GetClosestSector(TerrainMapImmobileType* sourceIT, const int& destinationSIndex)
{
	map<int,TerrainMapSector*>::iterator iS = sourceIT->sectorClosest.find(destinationSIndex);
	if( iS != sourceIT->sectorClosest.end() ) // It's already been determined
		return iS->second;
//*l<<"\n GCS";
	if( sourceIT->sector.find(destinationSIndex) != sourceIT->sector.end() )
	{
		sourceIT->sectorClosest.insert(isdPair(destinationSIndex,&sector[destinationSIndex]));
//*l<<"1(#)";
		return &sector[destinationSIndex];
	}

	const float3 *destination = &sector[destinationSIndex].position;
	TerrainMapSector* SClosest = 0;
	float DisClosest = 0.0f;
	for( map<int,TerrainMapSector*>::iterator iS=sourceIT->sector.begin(); iS!=sourceIT->sector.end(); ++iS )
		if( SClosest == 0 || iS->second->position.distance(*destination) < DisClosest )
		{
			SClosest = iS->second;
			DisClosest = iS->second->position.distance(*destination);
		}
	sourceIT->sectorClosest.insert(isdPair(destinationSIndex,SClosest));
//*l<<"2(#)";
	return SClosest;
}

TerrainMapAreaSector* GlobalTerrainMap::GetAlternativeSector(TerrainMapArea* sourceArea, const int& sourceSIndex, TerrainMapMobileType* destinationMT)
{
	TerrainMapAreaSector* TMSectors = GetSectorList(sourceArea);
	map<TerrainMapMobileType*,TerrainMapAreaSector*>::iterator iMS = TMSectors[sourceSIndex].sectorAlternativeM.find(destinationMT);
	if( iMS != TMSectors[sourceSIndex].sectorAlternativeM.end() ) // It's already been determined
		return iMS->second;
//*l<<"\nGSAM";
	if( destinationMT == 0 ) // flying unit movetype
	{
//		*l<<"(2#)";
		return &TMSectors[sourceSIndex];
	}

	if( sourceArea != 0 && sourceArea != TMSectors[sourceSIndex].area )
	{
//		*l<<"(3#)";
		return GetAlternativeSector(sourceArea, GetSectorIndex(GetClosestSector(sourceArea,sourceSIndex)->S->position), destinationMT);
	}

	const float3* position = &TMSectors[sourceSIndex].S->position;
	TerrainMapAreaSector* bestAS = 0;
	TerrainMapArea* largestArea = 0;
	float bestDistance = -1.0;
	float bestMidDistance = -1.0;
	TerrainMapArea **TMAreas = destinationMT->area;
	const int *TMAreaSize = &destinationMT->areaSize;
	for( int iA=0; iA<*TMAreaSize; iA++ )
		if( largestArea == 0 || largestArea->percentOfMap < TMAreas[iA]->percentOfMap )
			largestArea = TMAreas[iA];
	for( int iA=0; iA<*TMAreaSize; iA++ )
		if( TMAreas[iA]->areaUsable || !largestArea->areaUsable )
		{
			TerrainMapAreaSector* CAS = GetClosestSector(TMAreas[iA],sourceSIndex);
			float midDistance; // how much of a gap exists between the two areas (source & destination)
			if( sourceArea == 0 || sourceArea == TMSectors[GetSectorIndex(CAS->S->position)].area )
				midDistance = 0.0;
			else
				midDistance = CAS->S->position.distance2D(GetClosestSector(sourceArea,GetSectorIndex(CAS->S->position))->S->position);
			if( bestMidDistance < 0 || midDistance < bestMidDistance )
			{
				bestMidDistance = midDistance;
				bestAS = 0;
				bestDistance = -1.0;
			}
			if( midDistance == bestMidDistance )
			{
				float distance = position->distance2D(CAS->S->position);
				if( bestAS == 0 || distance*TMAreas[iA]->percentOfMap < bestDistance*bestAS->area->percentOfMap )
				{
					bestAS = CAS;
					bestDistance = distance;
				}
			}
		}
	TMSectors[sourceSIndex].sectorAlternativeM.insert(msPair(destinationMT,bestAS));
//	*l<<"(!#)";
	return bestAS;
}

TerrainMapSector* GlobalTerrainMap::GetAlternativeSector(TerrainMapArea* destinationArea, const int& sourceSIndex, TerrainMapImmobileType* destinationIT)
{
	TerrainMapAreaSector* TMSectors = GetSectorList(destinationArea);
	map<TerrainMapImmobileType*,TerrainMapSector*>::iterator iMS = TMSectors[sourceSIndex].sectorAlternativeI.find(destinationIT);
	if( iMS != TMSectors[sourceSIndex].sectorAlternativeI.end() ) // It's already been determined
		return iMS->second;

//*l<<"\nGSAI";
	TerrainMapSector* closestS = 0;
	if( destinationArea != 0 && destinationArea != TMSectors[sourceSIndex].area )
	{
//		*l<<"(3#)";
		closestS = GetAlternativeSector(destinationArea, GetSectorIndex(GetClosestSector(destinationArea,sourceSIndex)->S->position), destinationIT);
		TMSectors[sourceSIndex].sectorAlternativeI.insert(isPair(destinationIT,closestS));
		return closestS;
	}

	const float3 *position = &sector[sourceSIndex].position;
	float closestDistance = -1.0;
	for( map<int,TerrainMapAreaSector*>::iterator iS=destinationArea->sector.begin(); iS!=destinationArea->sector.end(); ++iS )
		if( closestS == 0 || iS->second->S->position.distance(*position) < closestDistance )
		{
			closestS = iS->second->S;
			closestDistance = iS->second->S->position.distance(*position);
		}

	TMSectors[sourceSIndex].sectorAlternativeI.insert(isPair(destinationIT,closestS));
//	*l<<"(!#)";
	return closestS;
}

int GlobalTerrainMap::GetSectorIndex(const float3& position)
{
	return sectorXSize*(int(position.z)/convertStoP) + int(position.x)/convertStoP;
}

bool GlobalTerrainMap::IsSectorValid(const int& sIndex)
{
	if( sIndex < 0 || sIndex >= sectorXSize*sectorZSize )
		return false;
	return true;
}

int GlobalTerrainMap::GetFileValue(int &fileSize, char *&file, string entry)
{
	for(size_t i=0; i<entry.size(); i++) {
		if( !islower(entry[i]) ) {
			entry[i] = tolower(entry[i]);
		}
	}
	size_t entryIndex = 0;
	string entryValue = "";
	for(int i=0; i<fileSize; i++)
	{
		if( entryIndex >= entry.size() )
		{	// Entry Found: Reading the value
			if( file[i] >= '0' && file[i] <= '9' )
				entryValue += file[i];
			else if( file[i] == ';' )
				return atoi(entryValue.c_str());
		}
		else if( entry[entryIndex] == file[i] || (!islower(file[i]) && entry[entryIndex] == tolower(file[i]) ) ) // the current letter matches
			entryIndex++;
		else
			entryIndex = 0;
	}
	return 0;
}
