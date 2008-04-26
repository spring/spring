#include "StdAfx.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "Helper.h"
#include "GroupAI.h"
#include <stdarg.h>

CR_BIND(CHelper, (NULL,NULL));

CR_REG_METADATA(CHelper,(
				//errorPos
				//metalMap
				//friendlyUnits
				//myTeam
				//extractorRadius
				CR_MEMBER(mmkrME),
				CR_MEMBER(maxPartitionRadius),
				CR_MEMBER(locations),
				CR_MEMBER(metalMakerAIid),
				//geoDef
				//drawColor
				CR_MEMBER(owner)
				//CR_POSTLOAD(PostLoad)//Will be called from CGroupAI::PostLoad
				));

CR_BIND(CHelper::partition, );

CR_REG_METADATA_SUB(CHelper, partition,(
					CR_MEMBER(pos),
					CR_MEMBER(name),
					CR_MEMBER(taken),
					CR_MEMBER(empty)
					));

CR_BIND(CHelper::location, );

CR_REG_METADATA_SUB(CHelper, location,(
					CR_MEMBER(centerPos),
					CR_MEMBER(radius),
					CR_MEMBER(partitionRadius),
					CR_MEMBER(numPartitions),
					CR_MEMBER(squarePartitions),
					CR_MEMBER(mexSpots),
					CR_MEMBER(partitions)
					));

CHelper::CHelper(IAICallback* aicb,CGroupAI *owner)
{
	this->aicb = aicb;
	this->owner = owner;

	errorPos		= float3(-1.0f,0.0f,0.0f);
//	friendlyUnits	= new int[MAX_UNITS];
	friendlyUnits.resize(MAX_UNITS);
	extractorRadius	= aicb?aicb->GetExtractorRadius():0;
	myTeam			= aicb?aicb->GetMyTeam():-1;
	metalMakerAIid	= 0;
	maxPartitionRadius = 100;
	drawColor[0] = 1.0f;
	drawColor[1] = 1.0f;
	drawColor[2] = 1.0f;
	drawColor[3] = 0.7f;
	if (!aicb) return;
	metalMap = new CMetalMap(aicb,false);
	metalMap->Init();

	// get the best M / E ratio for metalmakers and get a unitdef for a geo
	mmkrME	= 0;
	geoDef	= 0;
	std::map<std::string,const UnitDef*> targetBO;
	int size = aicb->GetFriendlyUnits(&(friendlyUnits.front()));

	for (int i = 0; i < size; i++) {
		if(myTeam != aicb->GetUnitTeam(friendlyUnits[i]))
			continue;
		ParseBuildOptions(targetBO,aicb->GetUnitDef(friendlyUnits[i]),true);
	}
	for (std::map<std::string, const UnitDef*>::iterator boi = targetBO.begin(); boi != targetBO.end(); ++boi) {
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

void CHelper::PostLoad()
{
	extractorRadius	= aicb->GetExtractorRadius();
	myTeam			= aicb->GetMyTeam();

	metalMap = new CMetalMap(aicb,false);
	metalMap->Init();

	// get the best M / E ratio for metalmakers and get a unitdef for a geo
	mmkrME	= 0;
	geoDef	= 0;
	std::map<std::string, const UnitDef*> targetBO;
	int size = aicb->GetFriendlyUnits(&(friendlyUnits.front()));
	for (int i = 0; i < size; i++) {
		if(myTeam != aicb->GetUnitTeam(friendlyUnits[i]))
			continue;
		ParseBuildOptions(targetBO,aicb->GetUnitDef(friendlyUnits[i]),true);
	}
	for (std::map<std::string, const UnitDef*>::iterator boi = targetBO.begin(); boi != targetBO.end(); ++boi) {
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

pair<int,int> CHelper::BuildNameToId(std::string name, int unit)
{
	std::pair<int, int> commandPair;
	commandPair.first	= 0;
	commandPair.second	= 0;
	const std::vector<CommandDescription>* cd=aicb->GetUnitCommands(unit);
	for (std::vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi) {
		if(cdi->id<0 && cdi->name==name) {
			commandPair.first	= cdi->id;
			commandPair.second	= cdi->type;
			break;
		}
	}
	return commandPair;
}

std::string CHelper::BuildIdToName(int id, int unit)
{
	std::string name = "";
	const std::vector<CommandDescription>* cd=aicb->GetUnitCommands(unit);
	for (std::vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi) {
		if(cdi->id<0 && cdi->id==id) {
			name = cdi->name;
			break;
		}
	}
	return name;
}

void CHelper::ParseBuildOptions(std::map<std::string,const UnitDef*> &targetBO, const UnitDef *unitDef, bool recursive)
{
	if(unitDef==0)
		return;
	if(unitDef->buildOptions.empty())
		return;
	if(targetBO.find(unitDef->name)!=targetBO.end())
		return;
	for (std::map<int, std::string>::const_iterator boi=unitDef->buildOptions.begin();boi!=unitDef->buildOptions.end();++boi) {
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

float3 CHelper::FindBuildPos(std::string name, bool isMex, bool isGeo, float distance, int builder)
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
						float3 buildPos2 = aicb->ClosestBuildSite(ud,lpi->pos,loc->partitionRadius,distance);
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
				float3 buildPos1 = aicb->ClosestBuildSite(ud,loc->centerPos,loc->radius,distance);
				if(buildPos1!=errorPos)
					return buildPos1;
			}
		}
	}
	return errorPos;
}

bool CHelper::IsMetalSpotAvailable(float3 spot,float extraction)
{
	int size = aicb->GetFriendlyUnits(&friendlyUnits.front(),spot,extractorRadius);
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

void CHelper::NewLocation(float3 centerPos, float radius)
{
	location* loc = new location;
	loc->centerPos	= centerPos;
	loc->radius		= radius;
	FindMetalSpots(centerPos,radius,&(loc->mexSpots));
	// calculate the partitions (if any)
	int squarePartitions	= (int) floor(radius / maxPartitionRadius);
	if(squarePartitions <= 1)
	{
		loc->numPartitions		= 0;
		loc->squarePartitions	= 0;
	}
	else
	{
		loc->numPartitions		= squarePartitions * squarePartitions;
		loc->squarePartitions	= squarePartitions;
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

void CHelper::ResetLocations()
{
	for(vector<location*>::iterator li=locations.begin();li!=locations.end();++li)
		delete *li;
	locations.clear();
}

int CHelper::FindMetalSpots(float3 pos, float radius, vector<float3>* mexSpots)
{
	int numSpotsFound = 0;
	if(!metalMap->IsMetalMap)
	{
		float xMin = pos.x - radius;
		float zMin = pos.z - radius;
		float xMax = pos.x + radius;
		float zMax = pos.z + radius;
		for(vector<float3>::const_iterator mi=metalMap->VectoredSpots.begin();mi!=metalMap->VectoredSpots.end();++mi)
		{
			if((*mi).x < xMax && (*mi).x > xMin && (*mi).z < zMax && (*mi).z > zMin)
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
	int size = aicb->GetFriendlyUnits(&friendlyUnits.front());
	for(int i=0;i<size;i++)
	{
		int unit = friendlyUnits[i];
		if(myTeam != aicb->GetUnitTeam(unit))
			continue;
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
#ifdef _WIN32
			metalMakerAIid = aicb->CreateGroup("AI/Helper-libs/MetalMakerAI.dll",99);
#else
			metalMakerAIid = aicb->CreateGroup("AI/Helper-libs/MetalMakerAI.so",99);
#endif
			aicb->AddUnitToGroup(unit,metalMakerAIid);
		}
	}
}

void CHelper::DrawBuildArea()
{
	float3 pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8;
	for(vector<location*>::iterator li=locations.begin();li!=locations.end();++li)
	{
		location* loc = *li;

		pos1.x = loc->centerPos.x - loc->radius;
		pos1.z = loc->centerPos.z - loc->radius;
		pos1.y = loc->centerPos.y;

		pos2.x = loc->centerPos.x - loc->radius;
		pos2.z = loc->centerPos.z + loc->radius;
		pos2.y = loc->centerPos.y;

		pos3.x = loc->centerPos.x + loc->radius;
		pos3.z = loc->centerPos.z + loc->radius;
		pos3.y = loc->centerPos.y;

		pos4.x = loc->centerPos.x + loc->radius;
		pos4.z = loc->centerPos.z - loc->radius;
		pos4.y = loc->centerPos.y;

		pos5.z = pos1.z;
		pos5.y = pos1.y;

		pos6.z = pos2.z;
		pos6.y = pos2.y;

		pos7.x = pos1.x;
		pos7.y = pos1.y;

		pos8.x = pos4.x;
		pos8.y = pos4.y;

		aicb->LineDrawerStartPath(pos1,drawColor);
		aicb->LineDrawerDrawLine(pos2,drawColor);
		aicb->LineDrawerDrawLine(pos3,drawColor);
		aicb->LineDrawerDrawLine(pos4,drawColor);
		aicb->LineDrawerDrawLine(pos1,drawColor);
		aicb->LineDrawerFinishPath();

		for(int i=1;i<loc->squarePartitions;i++)
		{
			float distance = 2*loc->radius/loc->squarePartitions;
			pos5.x = i*distance + pos1.x;
			pos6.x = i*distance + pos2.x;
		
			aicb->LineDrawerStartPath(pos5,drawColor);
			aicb->LineDrawerDrawLine(pos6,drawColor);
			aicb->LineDrawerFinishPath();

			pos7.z = i*distance + pos1.z;
			pos8.z = i*distance + pos4.z;
		
			aicb->LineDrawerStartPath(pos7,drawColor);
			aicb->LineDrawerDrawLine(pos8,drawColor);
			aicb->LineDrawerFinishPath();
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
