#include "StdAfx.h"
#include "DamageArrayHandler.h"
#include "DamageArray.h"
#include "TdfParser.h"
#include "Game/UI/InfoConsole.h"
#include <algorithm>
#include <locale>
#include <cctype>
#include "mmgr.h"

CDamageArrayHandler* damageArrayHandler;
int DamageArray::numTypes=1;

CDamageArrayHandler::CDamageArrayHandler(void)
{
	try
	{
		TdfParser p("Armor.txt");

		std::vector<std::string> typelist = p.GetSectionList("");

		DamageArray::numTypes=typelist.size()+1;
		numTypes=typelist.size()+1;

		info->AddLine(1, "Number of damage types: %d", numTypes);
		int a=1;
		for(std::vector<std::string>::iterator ti=typelist.begin();ti!=typelist.end();++a,++ti){
			std::string s = StringToLower(*ti);
			name2type[s]=a;
	//		info->AddLine("%s has type num %i",(*ti).c_str(),a);
			const std::map<std::string, std::string>& units=p.GetAllValues(*ti);

			for(std::map<std::string, std::string>::const_iterator ui=units.begin();ui!=units.end();++ui){
				std::string s = StringToLower(ui->first);
				name2type[s] = a;
	//			info->AddLine("unit %s has type num %i",ui->first.c_str(),a);
			}
		}
	}
	catch(content_error) // If the modrules.tdf isnt found
	{
		DamageArray::numTypes=1;
		numTypes=1;
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

DamageArray::DamageArray() {
	paralyzeDamageTime=0;
	impulseBoost=craterBoost=0.0f;
	impulseFactor=craterMult=1.0f;
	damages=new float[numTypes];
	for(int a=0;a<numTypes;++a)
		damages[a]=1;
}

DamageArray::DamageArray(const DamageArray& other){
	paralyzeDamageTime=other.paralyzeDamageTime;
	impulseBoost=other.impulseBoost;
	craterBoost=other.craterBoost;
	impulseFactor=other.impulseFactor;
	craterMult=other.craterMult;
	damages=new float[numTypes];
	for(int a=0;a<numTypes;++a)
		damages[a]=other.damages[a];
}

DamageArray::~DamageArray(){
	delete[] damages;
}
