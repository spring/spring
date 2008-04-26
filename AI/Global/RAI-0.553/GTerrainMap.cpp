#include "GTerrainMap.h"
#include <set>
//#include "Sim/MoveTypes/MoveInfo.h"
//#include "StdAfx.h"
//#include "CommanderScript.h"
//#include "Map/ReadMap.h"
//#include "TdfParser.h"
//#include "Other/SunParser.h"
//#include "OTAI/SunParser.h"

using std::set;
using std::deque;
using std::pair;

cTerrainMap::cTerrainMap(IAICallback* cb, cLogFile* logfile)
{
	l=logfile;
	*l<<"\n Loading cTerrainMap ...";
	PercentMetal=0;
	PercentLand=0;
	// cb->GetMapWidth(),cb->GetMapHeight()  8/8 Map Unit
	SectorZSize = cb->GetMapHeight()/32; // 32*8/32*8 = 512/512 Map Display Unit
	SectorXSize = cb->GetMapWidth()/(cb->GetMapHeight()/SectorZSize);
	*l<<"\n  Sector Size = x"<<SectorXSize<<" z"<<SectorZSize;
	Sector = new MapSector[SectorXSize*SectorZSize];
	int i;
	for(int z=0; z < SectorZSize; z++)
	{
		for(int x=0; x < SectorXSize; x++)
		{
			i=(z*SectorXSize)+x;
			Sector[i].Pos.x=x*256+128; // Center Position of Block
			Sector[i].Pos.z=z*256+128; //
			Sector[i].Pos.y=cb->GetElevation(Sector[i].Pos.x,Sector[i].Pos.z);
		}
	}

	const int SearchHeight=cb->GetMapHeight()/4; // 4*8/4*8 = 32/32 Search Block Unit
	const int SearchWidth =cb->GetMapWidth()/(cb->GetMapHeight()/SearchHeight);
	const float DtoS=8.0*(cb->GetMapHeight()/SearchHeight); // 32, Distance index to Search index conversion
	const int StoS=SearchHeight/SectorZSize;				// 8, Search index to Sector index conversion
	const int StoM=(cb->GetMapHeight()/SearchHeight)/2;		// 2, Search index to MetalMap index conversion
	*l<<"\n  Distance to Search Index = "<<DtoS;
	*l<<"\n  Search Index to Sector Index = "<<StoS;
	*l<<"\n  Search Index to MetalMap Index = "<<StoM;
	*l<<"\n  Search Size = x"<<SearchWidth<<" z"<<SearchHeight;
	*l<<"\n  Search Width * Search Height = "<<SearchWidth*SearchHeight;

	set<int> m;
	for(int z=0; z < SearchHeight; z++)
	{
		for(int x=0; x < SearchWidth; x++)
		{
			i=z*SearchWidth+x;
			m.insert(i);
		}
	}

	*l<<"\n  Searching Map for Land Masses ...";
	const unsigned char *MetalMapArray = cb->GetMetalMap();
	typedef pair<int,MapSector*> imPair;
	MapBodySize=0;
	int MBIndex;
	deque<int> Search;
	while( int(m.size())>0 || int(Search.size())>0 )
	{
		if( int(Search.size()) > 0 )
		{
			int x=Search.front()%SearchWidth;
			int z=Search.front()/SearchWidth;

			if( (cb->GetElevation(DtoS*x+DtoS/2,DtoS*z+DtoS/2) <= 0.0 && MapB[MBIndex]->Water) ||
				(cb->GetElevation(DtoS*x+DtoS/2,DtoS*z+DtoS/2) > 0.0 && !MapB[MBIndex]->Water) )
			{
				MapB[MBIndex]->PercentOfMap+=1.0;
				if( MetalMapArray[StoM*z*StoM*SearchWidth + StoM*x] > 0 )
					MapB[MBIndex]->PercentMetal+=1.0;

				i=SectorXSize*(z/StoS)+(x/StoS);
				if( MapB[MBIndex]->Sector.find(i) == MapB[MBIndex]->Sector.end() )
					MapB[MBIndex]->Sector.insert(imPair(i,&Sector[i]));

				i=z*SearchWidth+x;
				if( m.find(i-1)!=m.end() && x > 0 ) // Search left
				{
					Search.push_back(i-1);
					m.erase(i-1);
				}
				if( m.find(i+1)!=m.end() && x < SearchWidth-1 ) // Search right
				{
					Search.push_back(i+1);
					m.erase(i+1);
				}				
				if( m.find(i-SearchWidth)!=m.end() && z > 0 ) // Search up
				{
					Search.push_back(i-SearchWidth);
					m.erase(i-SearchWidth);
				}				
				if( m.find(i+SearchWidth)!=m.end() && z < SearchHeight-1 ) // Search down
				{
					Search.push_back(i+SearchWidth);
					m.erase(i+SearchWidth);
				}		
			}
			else
				m.insert(Search.front()); // not the same land mass, add back to global list
			Search.pop_front();
		}
		else
		{
			if( MapBodySize == 50 || (MapBodySize>0 && (MapB[MapBodySize-1]->PercentOfMap <= 16.0 || MapB[MapBodySize-1]->PercentOfMap/(SearchWidth*SearchHeight) <= 0.005 ) ) ) // Too mand landmasses detected.  find, erase & ignore the smallest landmass
			{
				if( MapBodySize == 50 )
				{
					*l<<"\nWARNING: Land Mass Limit Reached (possible error).";
				}

				MBIndex=-1;
				for( int iMB=0; iMB<MapBodySize; iMB++ )
				{
					if( MBIndex == -1 || MapB[MBIndex]->PercentOfMap > MapB[iMB]->PercentOfMap )
						MBIndex = iMB;
				}
				delete MapB[MBIndex];
				MapBodySize--;
			}
			else
				MBIndex=MapBodySize;

			i=*m.begin();
			int x=i%SearchWidth;
			int z=i/SearchWidth;

			Search.push_back(i);
			m.erase(i);
			MapB[MBIndex]=new MapBody(MBIndex);
			if( cb->GetElevation(DtoS*x+DtoS/2,DtoS*z+DtoS/2) <= 0 )
			{
				MapB[MBIndex]->Water=true;
			}
			else
			{
				MapB[MBIndex]->Water=false;
			}
			MapBodySize++;
		}
	}
	if( MapBodySize>0 && MapB[MapBodySize-1]->PercentOfMap <= 32.0 )
	{
		delete MapB[MapBodySize-1];
		MapBodySize--;
	}

	*l<<"\n  Calculating PercentOfMap & Determining Sector Types ...";
	MetalMapWater=false;
	MetalMapLand=false;
	typedef pair<int,MapBody*> ibPair;
	for( int iMB=0; iMB<MapBodySize; iMB++ )
	{
		MapB[iMB]->PercentMetal = 100.0*(MapB[iMB]->PercentMetal/MapB[iMB]->PercentOfMap);
		MapB[iMB]->PercentOfMap *= 100.0/(SearchWidth*SearchHeight);
		for( map<int,MapSector*>::iterator iS=MapB[iMB]->Sector.begin(); iS!=MapB[iMB]->Sector.end(); iS++ )
		{
			iS->second->mb.insert(ibPair(iMB,MapB[iMB]));
			if( MapB[iMB]->Water )
				iS->second->Water=true;
			else
				iS->second->Land=true;
		}

		if( MapB[iMB]->PercentOfMap >= 10.0 && MapB[iMB]->PercentMetal >= 90.0 && cb->GetExtractorRadius() <= 45.0 )
		{
			MapB[iMB]->MetalMap = true;
			if( MapB[iMB]->Water )
				MetalMapWater=true;
			else
				MetalMapLand=true;
		}
		else
		{
			MapB[iMB]->MetalMap = false;
		}
	}

	MapBMainLand=0;
	MapBMainWater=0;
	for( int i=0; i<MapBodySize; i++ )
	{
		if( !MapB[i]->Water && (MapBMainLand == 0 || MapBMainLand->PercentOfMap < MapB[i]->PercentOfMap) )
			MapBMainLand=MapB[i];
		if( MapB[i]->Water && (MapBMainWater == 0 || MapBMainWater->PercentOfMap < MapB[i]->PercentOfMap) )
			MapBMainWater=MapB[i];
	}

	*l<<"\n  Determining Closest Sectors for each Sector by Land Mass ...";
	for( int iS=0; iS<SectorXSize*SectorZSize; iS++ )
	{
		for( int iMB=0; iMB<MapBodySize; iMB++ )
		{
			float fClosest=-1.0;
			int iBest=-1;
			for( map<int,MapSector*>::iterator iMS=MapB[iMB]->Sector.begin(); iMS!=MapB[iMB]->Sector.end(); iMS++ )
			{
				if( iBest == -1 || Sector[iS].Pos.distance(iMS->second->Pos) < fClosest )
				{
					fClosest = Sector[iS].Pos.distance(iMS->second->Pos);
					iBest = iMS->first;
				}
			}
			Sector[iS].ms.insert(imPair(iMB,&Sector[iBest]));
		}

		int BestBody;
		for( int iMB=0; iMB<MapBodySize; iMB++ )
		{
			if( (Sector[iS].Land && Sector[iS].ms.find(iMB)->second->Water) ||
				(Sector[iS].Water && Sector[iS].ms.find(iMB)->second->Land) )
			{
				if( Sector[iS].AltSector == 0 || MapB[iMB]->PercentOfMap > 1.75*MapB[BestBody]->PercentOfMap ||
					(1.75*MapB[iMB]->PercentOfMap > MapB[BestBody]->PercentOfMap &&
					2.25*Sector[iS].Pos.distance(Sector[iS].ms.find(iMB)->second->Pos) < Sector[iS].Pos.distance(Sector[iS].ms.find(BestBody)->second->Pos)) )
				{
					BestBody=iMB;
					Sector[iS].AltSector=Sector[iS].ms.find(iMB)->second;
				}
			}
		}
		if( !Sector[iS].Land && !Sector[iS].Water )
			Sector[iS].AltSector = &Sector[iS];
	}

	*l<<"\n  Displaying Land/Water Masses ...";
	for( int i=0; i<MapBodySize; i++ )
	{
		*l<<"\n  ";
		if( MapB[i]->Water )
			*l<<"Water Mass ";
		else
			*l<<"Land Mass  ";

		*l<<"("<<i+1<<"), occuping "<<MapB[i]->PercentOfMap<<"% of the map ("<<MapB[i]->PercentMetal<<"% Metal) ";
		if( MapB[i]->MetalMap )
			*l<<"(MetalMap)";
		*l<<", has been detected in Sectors:";
		for( map<int,MapSector*>::iterator iM=MapB[i]->Sector.begin(); iM!=MapB[i]->Sector.end(); iM++ )
		{
			*l<<" ("<<int(iM->second->Pos.x/256)<<","<<int(iM->second->Pos.z/256)<<")";
		}
	}

	if( MetalMapLand )
		*l<<"\n  (Land Metal Map Detected)";   
	if( MetalMapWater )
		*l<<"\n  (Water Metal Map Detected)";   

	int MapHeight=(cb->GetMapHeight())/2;
	int MapWidth=(cb->GetMapWidth())/2;
	for(int z=0; z < MapHeight; z++)
		for(int x=0; x < MapWidth; x++)
		{
			i=(z*MapWidth)+x;
			if( MetalMapArray[i] > 0 )
				PercentMetal=((PercentMetal*i)+1)/(i+1);
			else
				PercentMetal=((PercentMetal*i)+0)/(i+1);
		}
	PercentMetal*=100.0;
	*l<<"\n  Map Metal Percent: "<<PercentMetal<<"%";

	MaxWaterDepth=0;
	MapHeight=cb->GetMapHeight();
	MapWidth =cb->GetMapWidth();
	const float *HeightMapArray = cb->GetHeightMap();
	for(int z=0; z < MapHeight; z++)
		for(int x=0; x < MapWidth; x++)
		{
			i=(z*MapWidth)+x;
			if( HeightMapArray[i] > 0 )
				PercentLand=((PercentLand*i)+1)/(i+1);
			else
				PercentLand=((PercentLand*i)+0)/(i+1);
			if( HeightMapArray[i] < MaxWaterDepth )
				MaxWaterDepth = HeightMapArray[i];
		}
	PercentLand*=100.0;
	*l<<"\n  Map Land Percent: "<<PercentLand<<"%";
	*l<<"\n  Max Water Depth: "<<MaxWaterDepth;
	if( MaxWaterDepth == 0 )
		MaxWaterDepth = -1; // important for calculations that use this
/*
	*l<<"\n  Loading Map ";
	string s = "maps/";
	s += cb->GetMapName();
	int size = cb->GetFileSize(s.c_str());
    if(size == -1)
    {
		*l<<"\nERROR: File not found: "<<s;
        return;
    }
	*l<<"\nFile: '"<<s<<"' size="<<size;
	char *buf = new char[size];
	*l<<".";
	cb->ReadFile(s.c_str(),buf, size);

	*l<<"\nreading it";
	for( int i=0; i<size; i++ )
	{
		if(*buf == '[') //sectionname
		{
			string thissection = "";
			while(*(++buf)!=']')
			{
				thissection += *buf;
			}
			*l<<"\nSection="<<thissection;
		}
	}
*l<<"\ndone?";
*/
}

cTerrainMap::~cTerrainMap()
{
	for( int i=0; i<MapBodySize; i++ )
		delete MapB[i];
	delete [] Sector;
}

int cTerrainMap::GetSector(const float3& Position)
{
	return SectorXSize*(int(Position.z)/256) + int(Position.x)/256;
}

bool cTerrainMap::CanMoveToPos(int MapBody, float3 Destination)
{
	int iS = GetSector(Destination);
	if( !SectorValid(iS) )
		return false;
	if( MapBody == -1 || Sector[iS].mb.find(MapBody) != Sector[iS].mb.end() || MapBody == -2 ) // MapBody == -2 mean the unit probably cant move 'at all', but go ahead and let it try
		return true;
	return false;
}

bool cTerrainMap::SectorValid(const int& SectorI)
{
	if( SectorI < 0 || SectorI > SectorXSize*SectorZSize -1 )
		return false;
	return true;
}
