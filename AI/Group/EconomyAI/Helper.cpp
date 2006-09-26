#include "StdAfx.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "Helper.h"
#include <stdarg.h>

CHelper::CHelper(IAICallback* aicb)
{
	this->aicb = aicb;

	errorPos		= float3(-1.0f,0.0f,0.0f);
	friendlyUnits	= new int[MAX_UNITS];
	extractorRadius	= aicb->GetExtractorRadius();
	metalMakerAIid	= 0;
	maxPartitionRadius = 100;

	metalMap = new CMetalMap(aicb,false);
	metalMap->Init();

	AssignMetalMakerAI();


	// get the best M / E ratio for metalmakers and get a unitdef for a geo
	mmkrME	= 0;
	geoDef	= 0;
	map<string,const UnitDef*> targetBO;
	int size = aicb->GetFriendlyUnits(friendlyUnits);
	for(int i=0;i<size;i++)
	{
		ParseBuildOptions(targetBO,aicb->GetUnitDef(friendlyUnits[i]),true);
	}
	for(map<string,const UnitDef*>::iterator boi=targetBO.begin();boi!=targetBO.end();++boi)
	{
		const UnitDef* ud = boi->second;
		if(ud->isMetalMaker)
		{
			float tempMmkrME = ud->makesMetal / max(1.0f,ud->energyUpkeep);
			if(tempMmkrME > mmkrME)
				mmkrME = tempMmkrME;
		}
		if(ud->needGeo && geoDef==0)
		{
			geoDef = ud;
		}
	}
	if(mmkrME==0)
		mmkrME = 1/100;
}

CHelper::~CHelper()
{
	for(vector<location*>::iterator li=locations.begin();li!=locations.end();++li)
	{
		delete *li;
	}
	locations.clear();
	delete metalMap;
}

pair<int,int> CHelper::BuildNameToId(string name, int unit)
{
	pair<int,int> commandPair;
	commandPair.first	= 0;
	commandPair.second	= 0;
	const vector<CommandDescription>* cd=aicb->GetUnitCommands(unit);
	for(vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi)
	{
		if(cdi->id<0 && cdi->name==name)
		{
			commandPair.first	= cdi->id;
			commandPair.second	= cdi->type;
			break;
		}
	}
	return commandPair;
}

string CHelper::BuildIdToName(int id, int unit)
{
	string name = "";
	const vector<CommandDescription>* cd=aicb->GetUnitCommands(unit);
	for(vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi)
	{
		if(cdi->id<0 && cdi->id==id)
		{
			name = cdi->name;
			break;
		}
	}
	return name;
}

void CHelper::ParseBuildOptions(map<string,const UnitDef*> &targetBO, const UnitDef *unitDef, bool recursive)
{
	if(unitDef==0)
		return;
	if(unitDef->buildOptions.empty())
		return;
	if(targetBO.find(unitDef->name)!=targetBO.end())
		return;
	for(map<int,string>::const_iterator boi=unitDef->buildOptions.begin();boi!=unitDef->buildOptions.end();++boi)
	{
		if(targetBO.find(boi->second)!=targetBO.end())
			continue;
		const UnitDef* ud=aicb->GetUnitDef(boi->second.c_str());
		if(ud==0)
			continue;
		targetBO[ud->name] = ud;
		if(recursive)
			ParseBuildOptions(targetBO,ud,true);
	}
}

float3 CHelper::FindBuildPos(string name, bool isMex, bool isGeo, int builder)
{
	const UnitDef* ud = aicb->GetUnitDef(name.c_str());
	if(ud==0)
	{
		return errorPos;
	}
	// iterate through every buildinglocation
	for(vector<location*>::iterator li=locations.begin();li!=locations.end();++li)
	{
		location* loc = *li;
		// do we want to build a mex on a non-metal map?
		if(isMex && !metalMap->IsMetalMap && !isGeo)
		{
			float3 builderPos	= aicb->GetUnitPos(builder);
			float3 returnPos	= errorPos;
			float minDist		= 0.0f;
			for(vector<float3>::iterator mi=loc->mexSpots.begin();mi!=loc->mexSpots.end();++mi)
			{
				if(IsMetalSpotAvailable(*mi,ud->extractsMetal))
				{
					float3 buildPos= aicb->ClosestBuildSite(ud,*mi,extractorRadius,0);
					if(buildPos!=errorPos)
					{
						float dist = (builderPos-buildPos).SqLength2D();
						if(dist < minDist || minDist == 0.0f)
						{
							minDist		= dist;
							returnPos	= buildPos;
						}
					}
				}
			}
			return returnPos;

		}
		else
		{
			if(loc->numPartitions > 0 && !isGeo)
			{
				// the building location is divided into partitions
				for(vector<partition>::iterator lpi=loc->partitions.begin();lpi!=loc->partitions.end();++lpi)
				{
					if(!lpi->taken && !(!lpi->empty && lpi->name != name))
					{
						float3 buildPos2 = aicb->ClosestBuildSite(ud,lpi->pos,loc->partitionRadius,0);
						if(buildPos2!=errorPos)
						{
							lpi->name = name;
							lpi->empty = false;
							return buildPos2;
						}
					}
				}
			}
			else
			{
				// the buildinglocation is not divided into partitions or we have a geo
				float3 buildPos1 = aicb->ClosestBuildSite(ud,loc->centerPos,loc->radius,0);
				if(buildPos1!=errorPos)
					return buildPos1;
			}
		}
	}
	return errorPos;
}

bool CHelper::IsMetalSpotAvailable(float3 spot,float extraction)
{
	int size = aicb->GetFriendlyUnits(friendlyUnits,spot,extractorRadius);
	for(int i=0;i<size;i++)
	{
		const UnitDef* ud=aicb->GetUnitDef(friendlyUnits[i]);
		if(ud==0)
			continue;
		if(ud->extractsMetal >= extraction)
		{
			return false;
		}
	}
	return true;
}

void CHelper::NewLocation(float3 centerPos, float radius, bool reset)
{
	if(reset)
	{
		for(vector<location*>::iterator li=locations.begin();li!=locations.end();++li)
		{
			delete *li;
		}
		locations.clear();
	}
	location* loc = new location;
	loc->centerPos	= centerPos;
	loc->radius		= radius;
	FindMetalSpots(centerPos,radius,&(loc->mexSpots));
	// calculate the partitions (if any)
	int squarePartitions	= (int) floor(radius / maxPartitionRadius);
	if(squarePartitions <= 1)
	{
		loc->numPartitions = 0;
	}
	else
	{
		loc->numPartitions		= squarePartitions * squarePartitions;
		loc->partitionRadius	= radius / squarePartitions;
		float xOffset		= centerPos.x - radius;
		float zOffset		= centerPos.z - radius;
		float negzOffset	= centerPos.z + radius;
		for(int i=0;i<squarePartitions;i++)
		{
			for(int j=0;j<squarePartitions;j++)
			{
				partition part;
				part.empty	= true;
				part.pos.x	= xOffset + ((i+0.5) * 2 * loc->partitionRadius);
				if((i+1) & 2)
				{
					part.pos.z	= negzOffset - ((j+0.5) * 2 * loc->partitionRadius);
				}
				else
				{
					part.pos.z	= zOffset + ((j+0.5) * 2 * loc->partitionRadius);
				}
				part.pos.y	= aicb->GetElevation(part.pos.x, part.pos.z);
				part.name	= "still open spot";
				if(!metalMap->IsMetalMap && FindMetalSpots(part.pos,loc->partitionRadius,0) > 0)
				{
					// prevent metal spots from being built over
					part.taken = true;
				}
				else
				{
					if(geoDef!=0 && aicb->ClosestBuildSite(geoDef,part.pos,loc->partitionRadius,0)!=errorPos)
					{
						// prevent geo spots from being built over
						part.taken = true;
					}
					else
						part.taken = false;
				}
				loc->partitions.push_back(part);
			}
		}
	}
	locations.push_back(loc);
}

int CHelper::FindMetalSpots(float3 pos, float radius, vector<float3>* mexSpots)
{
	int numSpotsFound = 0;
	if(!metalMap->IsMetalMap)
	{
		float sqRadius	= radius*radius;
		for(vector<float3>::const_iterator mi=metalMap->VectoredSpots.begin();mi!=metalMap->VectoredSpots.end();++mi)
		{
			if((pos-(*mi)).SqLength2D() < sqRadius)
			{
				float3 spot = *mi;
				spot.y = aicb->GetElevation(spot.x,spot.z);
				if(mexSpots != 0) mexSpots->push_back(spot);
				numSpotsFound++;
			}
		}
	}
	return numSpotsFound;
}
void CHelper::AssignMetalMakerAI()
{
	int size = aicb->GetFriendlyUnits(friendlyUnits);
	for(int i=0;i<size;i++)
	{
		int unit = friendlyUnits[i];
		const UnitDef* ud=aicb->GetUnitDef(unit);
		if(ud==0)
			continue;
		if(ud->isMetalMaker)
		{
			if(metalMakerAIid!=0)
			{
				if(aicb->AddUnitToGroup(unit,metalMakerAIid))
				{
					continue;
				}
			}
			metalMakerAIid = aicb->CreateGroup("AI/Helper-libs/MetalMakerAI.dll",99);
			aicb->AddUnitToGroup(unit,metalMakerAIid);
		}
	}
}

void CHelper::DrawBuildArea()
{
	if(!(aicb->GetCurrentFrame() & 3))
	{
		int g = 0;
		float3 pos1,pos2;
		for(vector<location*>::iterator li=locations.begin();li!=locations.end();++li)
		{
			location* loc = *li;
			for(int a=0;a<=20;++a)
			{
				pos1=float3(loc->centerPos.x+sin(a*PI*2/20)*loc->radius,0,loc->centerPos.z+cos(a*PI*2/20)*loc->radius);
				pos1.y=aicb->GetElevation(pos1.x,pos1.z)+5;
				if(a>0)
					g = aicb->CreateLineFigure(pos1,pos2,2.0f,0,4,g);
				pos2 = pos1;
			}
			for(int i=0;i<loc->numPartitions;i++)
			{
				for(int a=0;a<=20;++a)
				{
					pos1=float3(loc->partitions[i].pos.x+sin(a*PI*2/20)*loc->partitionRadius,0,loc->partitions[i].pos.z+cos(a*PI*2/20)*loc->partitionRadius);
					pos1.y=aicb->GetElevation(pos1.x,pos1.z)+5;
					if(a>0)
						g = aicb->CreateLineFigure(pos1,pos2,2.0f,0,4,g);
					pos2 = pos1;
				}
			}
			aicb->SetFigureColor(g,1.0f,1.0f,1.0f,1.0f);
		}
	}
}

void CHelper::SendTxt(const char *fmt, ...)
{
	char text[500];
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	aicb->SendTextMsg(text,0);
}
