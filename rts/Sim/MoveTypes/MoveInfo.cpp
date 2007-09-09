#include "StdAfx.h"
#include "MoveInfo.h"
#include "FileSystem/FileHandler.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "LogOutput.h"
#include "Map/ReadMap.h"
#include "MoveMath/MoveMath.h"
#include <boost/lexical_cast.hpp>
#include "creg/STL_Deque.h"
#include "creg/STL_Map.h"
#include "mmgr.h"

CR_BIND(MoveData, );
CR_BIND(CMoveInfo, );

CR_REG_METADATA(MoveData, (
		CR_ENUM_MEMBER(moveType),
		CR_MEMBER(size),

		CR_MEMBER(depth),
		CR_MEMBER(maxSlope),
		CR_MEMBER(slopeMod),
		CR_MEMBER(depthMod),

		CR_MEMBER(pathType),
		CR_MEMBER(moveMath),
		CR_MEMBER(crushStrength),
		CR_MEMBER(moveFamily)));

CR_REG_METADATA(CMoveInfo, (
		CR_MEMBER(moveData),
		CR_MEMBER(name2moveData),
		CR_MEMBER(moveInfoChecksum),
		CR_MEMBER(terrainType2MoveFamilySpeed)));


CMoveInfo* moveinfo;
using namespace std;


static float DegreesToMaxSlope(float degrees)
{
	return (float)(1.0 - cos(degrees * 1.5f * PI / 180.0));
}


CMoveInfo::CMoveInfo()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("MoveDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading movement definitions");
	}

	moveInfoChecksum = 0;

  for (size_t num = 1; /* no test */; num++) {
  	const LuaTable moveTable = rootTable.SubTable(num);
  	if (!moveTable.IsValid()) {
  		break;
		}
	
		MoveData* md = SAFE_NEW MoveData;
		const string name = moveTable.GetString("name", "");

		md->pathType = (num - 1);
		md->maxSlope = 1.0f;
		md->depth = 0.0f;
		md->crushStrength = moveTable.GetFloat("crushStrength", 10.0f);

		if ((name.find("BOAT") != string::npos) ||
		    (name.find("SHIP") != string::npos)) {
			md->moveType = MoveData::Ship_Move;
			md->depth = moveTable.GetFloat("minWaterDepth", 10.0f);
//			logOutput.Print("class %i %s boat", num, name.c_str());
			md->moveFamily = 3;
		}
		else if (name.find("HOVER") != string::npos) {
			md->moveType = MoveData::Hover_Move;
//			md->maxSlope = 1.0 - cos(moveTable.GetFloat("maxSlope", 10.0f) * 1.5f * PI / 180);
			md->maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 15.0f));
			md->moveFamily = 2;
//			logOutput.Print("class %i %s hover", num, name.c_str());
		}
		else {
			md->moveType = MoveData::Ground_Move;	
			md->depthMod = 0.1f;
			md->depth = moveTable.GetFloat("maxWaterDepth", 0.0f);
//			md->maxSlope = 1 - cos(moveTable.GetFloat("maxSlope", 60.0f) * 1.5f * PI / 180);
			md->maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 60.0f));
			
//			logOutput.Print("class %i %s ground", num, name.c_str());
			if (name.find("TANK") != string::npos) {
				md->moveFamily = 0;
			} else {
				md->moveFamily = 1;
			}
		}

		md->slopeMod = 4.0f / (md->maxSlope + 0.001f);

		// TA has only half our res so multiply size with 2
		md->size = max(2, min(8, moveTable.GetInt("footprintX", 1) * 2));

//		logOutput.Print("%f %i", md->slopeMod, md->size);
		moveInfoChecksum += md->size;
		moveInfoChecksum ^= *(unsigned int*)&md->slopeMod;
		moveInfoChecksum += *(unsigned int*)&md->depth;

		moveData.push_back(md);
		name2moveData[name] = md->pathType;
	}

	for (int a = 0; a < 256; ++a) {
		terrainType2MoveFamilySpeed[a][0] = readmap->terrainTypes[a].tankSpeed;
		terrainType2MoveFamilySpeed[a][1] = readmap->terrainTypes[a].kbotSpeed;
		terrainType2MoveFamilySpeed[a][2] = readmap->terrainTypes[a].hoverSpeed;
		terrainType2MoveFamilySpeed[a][3] = readmap->terrainTypes[a].shipSpeed;
	}
}


CMoveInfo::~CMoveInfo()
{
	while (!moveData.empty()) {
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
