#include "StdAfx.h"
#include "DamageArrayHandler.h"
#include "DamageArray.h"
#include "TdfParser.h"
#include "LogOutput.h"
#include <algorithm>
#include <locale>
#include <cctype>
#include "creg/STL_Map.h"
#include "mmgr.h"

CR_BIND(CDamageArrayHandler, );

CR_REG_METADATA(CDamageArrayHandler, (
		CR_MEMBER(numTypes),
		CR_MEMBER(name2type),
		CR_MEMBER(typeList)));


CDamageArrayHandler* damageArrayHandler;


CDamageArrayHandler::CDamageArrayHandler(void)
{
	try
	{
		TdfParser p("Armor.txt");

		typeList = p.GetSectionList("");

		numTypes = typeList.size() + 1;

		logOutput.Print(1, "Number of damage types: %d", numTypes);
		int a=1;
		for(std::vector<std::string>::iterator ti=typeList.begin();ti!=typeList.end();++a,++ti){
			std::string s = StringToLower(*ti);
			name2type[s]=a;
	//		logOutput.Print("%s has type num %i",(*ti).c_str(),a);
			const std::map<std::string, std::string>& units=p.GetAllValues(*ti);

			for(std::map<std::string, std::string>::const_iterator ui=units.begin();ui!=units.end();++ui){
				std::string s = StringToLower(ui->first);
				name2type[s] = a;
	//			logOutput.Print("unit %s has type num %i",ui->first.c_str(),a);
			}
		}
	}
	catch(content_error) // If the modrules.tdf isnt found
	{
		numTypes = 1;
	}
}


CDamageArrayHandler::~CDamageArrayHandler(void)
{
}


int CDamageArrayHandler::GetTypeFromName(std::string name)
{
	StringToLowerInPlace(name);
	if(name2type.find(name)!=name2type.end())
		return name2type[name];
	return 0;
}
