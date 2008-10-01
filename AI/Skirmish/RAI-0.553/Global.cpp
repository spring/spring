#include "Global.h"
//#include "Sim/Units/UnitDef.h"
#include "Sim/Features/FeatureDef.h"
#include <time.h>		// time(NULL)

cRAIGlobal::cRAIGlobal(IAICallback* cb)
{
	ClearLogFiles(cb);
	l=new cLogFile("RAIGlobal_LastGame.log",false);
	*l<<"Loading ...";

/*
	int *F = new int[10000];
	int FSize = cb->GetFeatures(F,10000)
	*l<<"\n cb->GetFeatures(F,10000) no longer crashes.";
*/
//	*l<<"\n Team="<<cb->GetMyTeam();
	int seed = time(NULL);
	srand(seed);
//	*l<<"\n Random Seed = "<<seed;

	*l<<"\n Mod = "<<cb->GetModName();
	*l<<"\n Map = "<<cb->GetMapName();

	AIs=0;
	TM = new cTerrainMap(cb,l);

	*l<<"\n Loading cMetalMap ...";
	KMM = new CMetalMap(cb);
	KMM->Init();

//	RMM = new cRMetalMap(cb,l);

	const UnitDef **uds = new const UnitDef*[cb->GetNumUnitDefs()];
	cb->GetUnitDefList(uds);
	int minXSize=-1;
	int minZSize=-1;
	int udSize=cb->GetNumUnitDefs();
	for( int iud=udSize-1; iud>=0; iud-- )
	{
		if( uds[iud] == 0 ) // War Alien VS Human v1.0 workaround
		{
			cb->SendTextMsg("WARNING: Mod Unit Definition Invalid.",0);
			*l<<"\n  WARNING: (unitdef->id="<<iud+1<<") Mod UnitDefList["<<iud<<"] = 0";
			udSize--;
			uds[iud] = uds[udSize];
		}
		else if( uds[iud]->extractsMetal > 0 && (minXSize == -1 || 8*uds[iud]->xsize < minXSize ) )
		{
			minXSize=8*uds[iud]->xsize;
			minZSize=8*uds[iud]->ysize;
		}
	}

	*l<<"\n Validating Metal Spots ...";
	float searchDis=cb->GetExtractorRadius()/2.0f;
	if( searchDis < 16.0f )
		searchDis = 16.0f;
	for(int iM=int(KMM->VectoredSpots.size())-1; iM>=0; iM--)
	{
		KMM->VectoredSpots.at(iM).y=cb->GetElevation(KMM->VectoredSpots.at(iM).x,KMM->VectoredSpots.at(iM).z);
		float3 Location=KMM->VectoredSpots.at(iM);
		bool CanBuilt=false;
		for( int iud=0; iud<udSize; iud++ )
		{
			const UnitDef* ud=uds[iud];
			if( ud->extractsMetal > 0.0 )
			{
				float3 Pos=cb->ClosestBuildSite(ud,KMM->VectoredSpots.at(iM),searchDis,0);
				if( cb->CanBuildAt(ud,Pos) )
				{
					CanBuilt=true;
					iud=udSize; // End Loop
				}
			}
		}
		if( CanBuilt )
		{
			for(int iM2=0; iM2<iM; iM2++)
			{
				if( Location.distance2D(KMM->VectoredSpots.at(iM2)) <= 8.0+minXSize || Location.distance2D(KMM->VectoredSpots.at(iM2)) <= 8.0+minZSize )
				{
					float3 Location2=KMM->VectoredSpots.at(iM2);
					*l<<"\n  Metal Resource located at (x"<<Location2.x<<" z"<<Location2.z<<" y"<<Location2.y<<") is unusable in this mod (due to close distance to other metal spots).";
					KMM->VectoredSpots.erase(KMM->VectoredSpots.begin()+iM2);
					KMM->NumSpotsFound--;
					iM--;
					iM2--;
				}
			}
		}
		if( !CanBuilt )
		{
			*l<<"\n  Metal Resource located at (x"<<Location.x<<" z"<<Location.z<<" y"<<Location.y<<") is unusable in this mod (due to the lack of a required extractor).";
			KMM->VectoredSpots.erase(KMM->VectoredSpots.begin()+iM);
			KMM->NumSpotsFound--;
		}
	}

	*l<<"\n Metal Spots Found: "<<KMM->NumSpotsFound;

	*l<<"\n Finding Geo Spots ...";
	int *F = new int[10000];
	float3 pos;
	int FSize = cb->GetFeatures(F,10000,pos,99999); // Work Around - Spring Version: v0.75b2
	for(int i=0; i<FSize; i++)
	{
		const FeatureDef *fd = cb->GetFeatureDef(F[i]);
		if( fd->geoThermal )
		{
			bool CanBuild=false;
			float3 Location=cb->GetFeaturePos(F[i]);
			for( int iud=0; iud<udSize; iud++ )
			{
				const UnitDef* ud=uds[iud];
				if( ud->needGeo )
				{
					float3 BuildLoc=cb->ClosestBuildSite(ud,Location,48.0f,0);
					if( cb->CanBuildAt(ud,BuildLoc) )
					{
						CanBuild=true;
						iud=cb->GetNumUnitDefs(); // End Loop
					}
				}
			}

			if( !CanBuild )
				*l<<"\n  Energy Resource located at (x"<<Location.x<<" z"<<Location.z<<" y"<<Location.y<<") is unusable in this mod.";
			else
				GeoSpot.insert(F[i]);
		}
	}
	delete [] uds;
	delete [] F;
	if( FSize == 10000 )
	{
		cb->SendTextMsg("ERROR: not all features were searched for Geo Build Options",5);
		*l<<"\nERROR: not all features were searched for Geo Build Options";
	}

	*l<<"\n Geo Spots Found: "<<int(GeoSpot.size());
	*l<<"\nLoading Complete.";
}

cRAIGlobal::~cRAIGlobal()
{
	*l<<"\n\nShuting Down";
	delete KMM;
	delete TM;
	*l<<" ... Complete.";
	delete l;
}

void cRAIGlobal::ClearLogFiles(IAICallback* cb)
{
	char s1[32] = "AI/RAI/"; cb->GetValue(AIVAL_LOCATE_FILE_W, s1);
	char s2[32] = "AI/RAI/Metal/"; cb->GetValue(AIVAL_LOCATE_FILE_W, s2);

	string sDir="AI/RAI/";
	string sFile;

	sFile=sDir+"RAIGlobal_LastGame.log";
	remove(sFile.c_str());
	for( int i=0; i<10; i++ )
	{	
		char c[2];
		sprintf(c, "%i",i);
		sFile=sDir+"RAI"+string(c)+"_LastGame.log";
		remove(sFile.c_str());
	}

	for( int i=0; i<10; i++ ) // RAI v0.30-v0.355 log files
	{	
		char c[2];
		sprintf(c, "%i",i);
		string sFile=sDir+"AI"+string(c)+"_LastGame.log";
		remove(sFile.c_str());
	}
//	sFile=sDir+"Prerequisite.log";
//	remove(sFile.c_str());
//	sFile=sDir+"Debug.log";
//	remove(sFile.c_str());
}
