#include "StdAfx.h"
#include "MoveInfo.h"
#include "FileSystem/FileHandler.h"
#include "TdfParser.h"
#include "Game/UI/InfoConsole.h"
#include "Map/ReadMap.h"
#include <boost/lexical_cast.hpp>
#include "mmgr.h"

CMoveInfo* moveinfo;
using namespace std;

CMoveInfo::CMoveInfo()
{
  TdfParser parser("gamedata/MOVEINFO.TDF");

	moveInfoChecksum=0;

  for(size_t num = 0;;++num) {
    string class_name = "class" + boost::lexical_cast<std::string>(num);
    if( ! parser.SectionExist( class_name ) )
      break;
	
		MoveData* md=new MoveData;
		string name=parser.SGetValueDef("",class_name+"\\name");

		md->maxSlope=1;
		md->depth=0;
		md->pathType=num;
		md->crushStrength = atof(parser.SGetValueDef("10",class_name+"\\CrushStrength").c_str());
		if(name.find("BOAT")!=string::npos || name.find("SHIP")!=string::npos){
			md->moveType=MoveData::Ship_Move;
			md->depth=atof(parser.SGetValueDef("10",class_name+"\\MinWaterDepth").c_str());
//			info->AddLine("class %i %s boat",num,name.c_str());
			md->moveFamily=3;
		} else if(name.find("HOVER")!=string::npos){
			md->moveType=MoveData::Hover_Move;
			md->maxSlope=1-cos((float)atof(parser.SGetValueDef("10",class_name+"\\MaxSlope").c_str())*1.5f*PI/180);
			md->moveFamily=2;
//			info->AddLine("class %i %s hover",num,name.c_str());
		} else {
			md->moveType=MoveData::Ground_Move;	
			md->depthMod=0.1;
			md->depth=atof(parser.SGetValueDef("0",class_name+"\\MaxWaterDepth").c_str());
			md->maxSlope=1-cos((float)atof(parser.SGetValueDef("60",class_name+"\\MaxSlope").c_str())*1.5f*PI/180);
//			info->AddLine("class %i %s ground",num,name.c_str());
			if(name.find("TANK")!=string::npos)
				md->moveFamily=0;
			else
				md->moveFamily=1;
		}
		md->slopeMod=4/(md->maxSlope+0.001);
		md->size=max(2,min(8,atoi(parser.SGetValueDef("1", class_name+"\\FootprintX").c_str())*2));//ta has only half our res so multiply size with 2
//		info->AddLine("%f %i",md->slopeMod,md->size);
		moveInfoChecksum+=md->size;
		moveInfoChecksum^=*(unsigned int*)&md->slopeMod;
		moveInfoChecksum+=*(unsigned int*)&md->depth;

		moveData.push_back(md);
		name2moveData[name]=md->pathType;
	}

	for(int a=0;a<256;++a){
		terrainType2MoveFamilySpeed[a][0]=readmap->terrainTypes[a].tankSpeed;
		terrainType2MoveFamilySpeed[a][1]=readmap->terrainTypes[a].kbotSpeed;
		terrainType2MoveFamilySpeed[a][2]=readmap->terrainTypes[a].hoverSpeed;
		terrainType2MoveFamilySpeed[a][3]=readmap->terrainTypes[a].shipSpeed;
	}
}

CMoveInfo::~CMoveInfo()
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

/*bool CMoveInfo::ClassExists(int num)
{
	char class_name[100];
	sprintf(class_name,"class%i",num);
	
	std::vector<string> sections=parser.GetSectionList("");
	for(vector<string>::iterator si=sections.begin();si!=sections.end();++si){
		if((*si)==class_name)
			return true;
	}
	return false;
}
*/
