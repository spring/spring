#include "StdAfx.h"
#include "MoveInfo.h"
#include "FileHandler.h"
#include "SunParser.h"
#include "InfoConsole.h"
//#include "mmgr.h"

CMoveInfo* moveinfo;
using namespace std;

CMoveInfo::CMoveInfo(void)
{
	sunparser=new CSunParser;
#ifdef _WIN32
	sunparser->LoadFile("gamedata\\moveinfo.tdf");
#else
        sunparser->LoadFile("gamedata/MOVIEINFO.TDF");
#endif

	moveInfoChecksum=0;

	for(int num=0;ClassExists(num);++num){
		char c[100];
		sprintf(c,"class%i",num);
		string className=c;
	
		MoveData* md=new MoveData;
		string name=sunparser->SGetValueDef("",className+"\\name");

		md->maxSlope=1;
		md->depth=0;
		md->pathType=num;
		md->crushStrength = atof(sunparser->SGetValueDef("10",className+"\\CrushStrength").c_str());
		if(name.find("BOAT")!=string::npos || name.find("SHIP")!=string::npos){
			md->moveType=MoveData::Ship_Move;
			md->depth=atof(sunparser->SGetValueDef("10",className+"\\MinWaterDepth").c_str());
//			info->AddLine("class %i %s boat",num,name.c_str());

		} else if(name.find("HOVER")!=string::npos){
			md->moveType=MoveData::Hover_Move;
			md->maxSlope=cos(atof(sunparser->SGetValueDef("10",className+"\\MaxSlope").c_str())*1.2*PI/180);

//			info->AddLine("class %i %s hover",num,name.c_str());
		} else {
			md->moveType=MoveData::Ground_Move;	
			md->depthMod=0.1;
			md->depth=atof(sunparser->SGetValueDef("0",className+"\\MaxWaterDepth").c_str());
			md->maxSlope=cos(atof(sunparser->SGetValueDef("60",className+"\\MaxSlope").c_str())*1.2*PI/180);
//			info->AddLine("class %i %s ground",num,name.c_str());
		}
		md->slopeMod=3/(1-md->maxSlope+0.001);
		md->size=max(2,min(8,atoi(sunparser->SGetValueDef("1", className+"\\FootprintX").c_str())*2));//ta has only half our res so multiply size with 2
//		info->AddLine("%f %i",md->slopeMod,md->size);
		moveInfoChecksum+=md->size;
		moveInfoChecksum^=*(unsigned int*)&md->slopeMod;
		moveInfoChecksum+=*(unsigned int*)&md->depth;

		moveData.push_back(md);
		name2moveData[name]=md->pathType;
	}

	delete sunparser;
}

CMoveInfo::~CMoveInfo(void)
{
	while(!moveData.empty()){
		delete moveData.back();
		moveData.pop_back();
	}
}

MoveData* CMoveInfo::GetMoveDataFromName(std::string name)
{
	return moveData[name2moveData[name]];
}

bool CMoveInfo::ClassExists(int num)
{
	char className[100];
	sprintf(className,"class%i",num);
	
	std::vector<string> sections=sunparser->GetSectionList("");
	for(vector<string>::iterator si=sections.begin();si!=sections.end();++si){
		if((*si)==className)
			return true;
	}
	return false;
}
