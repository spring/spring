#include "StdAfx.h"
#include "CategoryHandler.h"
#include "Game/UI/InfoConsole.h"
#include <algorithm>
#include <cctype>
#include "mmgr.h"

using namespace std;
CCategoryHandler* CCategoryHandler::instance=0;
CCategoryHandler* categoryHandler=0;

CCategoryHandler::CCategoryHandler(void)
:	firstUnused(0)
{
}

CCategoryHandler::~CCategoryHandler(void)
{
}

unsigned int CCategoryHandler::GetCategory(std::string name)
{
	StringToLowerInPlace(name);
	while(!name.empty() && *name.begin()==' ')
		name.erase(name.begin());
	if(name.empty())
		return 0;
	if(categories.find(name)==categories.end()){
		if(name.find("ctrl")!=string::npos
		|| name.find("arm")!=string::npos 
		|| name.find("core")!=string::npos 
		|| name.find("level")!=string::npos 
		|| name.find("energy")!=string::npos 
		|| name.find("storage")!=string::npos 
		|| name.find("defensive")!=string::npos 
		|| name.find("extractor")!=string::npos 
		|| name.find("metal")!=string::npos 
		|| name.find("torp")!=string::npos)		//remove some categories that we dont think we need since we have too few of them
			return 0;
		if(firstUnused>31){
			info->AddLine("Warning to many unit categories %i missed %s",firstUnused+1,name.c_str());
			return 0;
		}
		categories[name]=(1<<(firstUnused++));
//		info->AddLine("New cat %s #%i",name.c_str(),firstUnused);
	}
	return categories[name];
}

unsigned int CCategoryHandler::GetCategories(std::string names)
{
	StringToLowerInPlace(names);
	unsigned int ret=0;

	while(!names.empty()){
		string name=names;

		if(names.find_first_of(' ')!=string::npos)
			name.erase(name.find_first_of(' '),5000);

		if(names.find_first_of(' ')==string::npos)
			names.clear();
		else
			names.erase(0,names.find_first_of(' ')+1);

		ret|=GetCategory(name);
	}
	return ret;
}
