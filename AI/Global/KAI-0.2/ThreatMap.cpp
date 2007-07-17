#include "ThreatMap.h"

CR_BIND(CThreatMap ,(NULL))

CR_REG_METADATA(CThreatMap,(
				CR_MEMBER(ThreatArray),
				CR_MEMBER(EmptyThreatArray),
				CR_MEMBER(AverageThreat),
				//CR_MEMBER(xend),
				CR_MEMBER(ai),
				CR_RESERVED(128),
				CR_POSTLOAD(PostLoad)
				));

CThreatMap::CThreatMap(AIClasses *ai)
{
	this->ai=ai;
	ThreatResolution = THREATRES;										// Divide map resolution by this much (8x8 standard spring resolution)
	ThreatMapWidth = ai?ai->cb->GetMapWidth() / ThreatResolution:0;
	ThreatMapHeight = ai?ai->cb->GetMapHeight() / ThreatResolution:0;
	TotalCells = ThreatMapWidth * ThreatMapHeight;
	ThreatArray.resize(TotalCells);
	EmptyThreatArray.resize(TotalCells);
//	xend.resize(1280);
}	
CThreatMap::~CThreatMap()
{
//	delete [] ThreatArray;
//	delete [] xend;
}

void CThreatMap::PostLoad()
{
	ThreatMapWidth = ai->cb->GetMapWidth() / ThreatResolution;
	ThreatMapHeight = ai->cb->GetMapHeight() / ThreatResolution;
	TotalCells = ThreatMapWidth * ThreatMapHeight;
//	ThreatArray.resize(TotalCells);
//	EmptyThreatArray.resize(TotalCells);
}

void CThreatMap::Create()
{
	//L("Creating threat Array");
	Clear();
	const int* Enemies  = ai->sh->GetEnemiesList();
	double totalthreat = 0;
	int NumOfEnemies = ai->sh->GetNumberOfEnemies();
	for (int i = 0; i < NumOfEnemies; i++){
		//L("adding enemy unit");
		AddEnemyUnit(Enemies[i]);
	}
	
	for (int i = 0; i < TotalCells; i++){
		totalthreat += ThreatArray[i];
	}
	AverageThreat = totalthreat / TotalCells;
	// This is for making shure that there are no problems when the threat is too low.
	if(AverageThreat < 1.0)
		AverageThreat = 1;
	for (int i = 0; i < TotalCells; i++){
		ThreatArray[i] += AverageThreat;
	}
	//L("Threat array created!");
	//ai->debug->MakeBWTGA(ThreatArray,ThreatMapWidth,ThreatMapHeight,"ThreatMap",0.5);
	

}

void CThreatMap::AddEnemyUnit (int unitid)
{
	if(!ai->cb->UnitBeingBuilt(unitid) && ai->cheat->GetUnitDef(unitid)->weapons.size()){
		float3 pos = ai->cheat->GetUnitPos(unitid);
		int posx = int(pos.x / (8*ThreatResolution));
		int posy = int(pos.z / (8*ThreatResolution));
		const UnitDef* Def = ai->cheat->GetUnitDef(unitid);
		float Range = (ai->ut->GetMaxRange(Def))/(8*ThreatResolution)+2.5; //edited for slack
		float SQRange = Range * Range;
		float DPS = ai->ut->unittypearray[Def->id].AverageDPS;
		if(DPS > 2000)
			DPS = 2000;
		//ai->math->TimerStart();
		for (int myx = int(posx - Range); myx < posx + Range; myx++){
			if (myx >= 0 && myx < ThreatMapWidth){
				for (int myy = int(posy - Range); myy < posy + Range; myy++){
					if (myy >= 0 && myy < ThreatMapHeight && ((posx - myx)*(posx - myx) + (posy - myy)*(posy - myy) - 0.5) <= SQRange){
						ThreatArray[myy * ThreatMapWidth + myx] += DPS;
					}
				}
			}
		}
		/*
		// This code is only faster on units with long range, so ignore it atm.
		L("AddEnemyUnit old: " << ai->math->TimerSecs());
		ai->math->TimerStart();
		int DoubleRadius = Range*2;
		for (int a=0;a<DoubleRadius+1;a++){ 
			float z=a-Range;
			xend[a]=sqrt(SQRange-z*z);
		}
		
		for (int sy=posy-Range,a=0;sy<=posy+Range;sy++,a++){
			if (sy >= 0 && sy < ThreatMapHeight){
				int clearXStart = posx-xend[a];
				int clearXEnd = posx+xend[a];
				if(clearXStart < 0)
					clearXStart = 0;
				if(clearXEnd >= ThreatMapWidth)
					clearXEnd = ThreatMapWidth -1;
				for(int xClear = clearXStart; xClear <= clearXEnd; xClear++){
					ThreatArray[sy * ThreatMapWidth + xClear] += DPS;		
				}
			}
		}
		L("AddEnemyUnit new: " << ai->math->TimerSecs());
		*/
	}
}

void CThreatMap::RemoveEnemyUnit (int unitid)
{
	float3 pos = ai->cheat->GetUnitPos(unitid);
	int posx = int(pos.x / (8*ThreatResolution));
	int posy = int(pos.z / (8*ThreatResolution));
	const UnitDef* Def = ai->cheat->GetUnitDef(unitid);
	float Range = ai->ut->GetMaxRange(Def)/(8*ThreatResolution) ;
	float SQRange = Range * Range;
	float DPS = ai->ut->unittypearray[Def->id].AverageDPS;
	for (int myx = int(posx - Range); myx < posx + Range; myx++){
		if (myx >= 0 && myx < ThreatMapWidth){
			for (int myy = int(posy - Range); myy < posy + Range; myy++){
				if (myy >= 0 && myy < ThreatMapHeight && ((posx - myx)*(posx - myx) + (posy - myy)*(posy - myy)) <= SQRange){
					ThreatArray[myy * ThreatMapWidth + myx] -= DPS;
				}
			}
		}
	}
}

void CThreatMap::Clear()
{
	for(int i = 0; i < TotalCells; i++)
		ThreatArray[i] = 0;
}


float CThreatMap::GetAverageThreat()
{
	float modthreat = AverageThreat + 1;
	return modthreat;
}

float CThreatMap::GetUnmodifiedAverageThreat()
{
	return AverageThreat;
}


float CThreatMap::ThreatAtThisPoint(const float3 &pos)
{
	return ThreatArray[int(pos.z / (8*ThreatResolution)) * ThreatMapWidth + int(pos.x/(8*ThreatResolution))] ;
}
