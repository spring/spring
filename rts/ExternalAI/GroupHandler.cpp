#include "StdAfx.h"
// GroupHandler.cpp: implementation of the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "GroupHandler.h"
#include "Group.h"
#include "IGroupAI.h"
#include "Game/UI/InfoConsole.h"
#include "Game/SelectedUnits.h"
#include "TimeProfiler.h"
#include "Sim/Units/Unit.h"
#include "Game/UI/MouseHandler.h"
#include "Game/CameraController.h"
#include "Platform/SharedLib.h"
#include "Platform/errorhandler.h"
#include "Platform/FileSystem.h"
#include "SDL_types.h"
#include "SDL_keysym.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupHandler* grouphandler;
extern Uint8 *keys;

CGroupHandler::CGroupHandler(int team)
: firstUnusedGroup(10),
	team(team)
{
	defaultKey.dllName="default";
	defaultKey.aiNumber=0;

	FindDlls();

	for(int a=0;a<10;++a){
		groups.push_back(new CGroup(defaultKey,a,this));
	}
}

CGroupHandler::~CGroupHandler()
{
	for(int a=0;a<firstUnusedGroup;++a)
		delete groups[a];
}

void CGroupHandler::Update()
{
START_TIME_PROFILE;
	for(std::vector<CGroup*>::iterator ai=groups.begin();ai!=groups.end();++ai)
		if((*ai)!=0)
			(*ai)->Update();
END_TIME_PROFILE("Group AI");
}

void CGroupHandler::TestDll(string name)
{
	typedef int (* GETGROUPAIVERSION)();
	typedef const char ** (* GETAINAMELIST)();
	typedef bool (* ISUNITSUITED)(unsigned aiNumber,const UnitDef* unitDef);
	
	SharedLib *lib;
	GETGROUPAIVERSION GetGroupAiVersion;
	GETAINAMELIST GetAiNameList;
	ISUNITSUITED IsUnitSuited;

	lib = SharedLib::instantiate(name);
	if (!lib){
		info->AddLine ("Cant load dll: %s",name.c_str());
		return;
	}

	GetGroupAiVersion = (GETGROUPAIVERSION)lib->FindAddress("GetGroupAiVersion");
	if (!GetGroupAiVersion){
		info->AddLine("Incorrect AI dll(%s): No GetGroupAiVersion function found", name.c_str());
		delete lib;
		return;
	}

	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION){
		info->AddLine("AI dll %s has incorrect version", name.c_str());
		delete lib;
		return;
	}

	IsUnitSuited = (ISUNITSUITED)lib->FindAddress("IsUnitSuited");
	if (!IsUnitSuited){
		info->AddLine ("No IsUnitSuited function found in AI dll %s", name.c_str());
		delete lib;
		return;
	}
	
	GetAiNameList = (GETAINAMELIST)lib->FindAddress("GetAiNameList");
	if (!GetAiNameList){
		info->AddLine("No GetAiNameList function found in AI dll %s",name.c_str());
		delete lib;
		return;
	}

	const char ** aiNameList=GetAiNameList();

	for(unsigned i=0;aiNameList[i]!=NULL;i++){
		AIKey key;
		key.dllName=name;
		key.aiNumber=i;
		availableAI[key]=string(aiNameList[i]);
	}
//	(*info) << name.c_str() << " " << c << "\n";
	delete lib;
}

void CGroupHandler::GroupCommand(int num)
{
	if(keys[SDLK_LCTRL]){
		if(!keys[SDLK_LSHIFT])
			groups[num]->ClearUnits();
		set<CUnit*>::iterator ui;
		for(ui=selectedUnits.selectedUnits.begin();ui!=selectedUnits.selectedUnits.end();++ui){
			(*ui)->SetGroup(groups[num]);
		}
	}
	if(selectedUnits.selectedGroup==num && !groups[num]->units.empty()){
		float3 p(0,0,0);
		for(std::set<CUnit*>::iterator gi=groups[num]->units.begin();gi!=groups[num]->units.end();++gi){
			p+=(*gi)->pos;
		}
		p/=groups[num]->units.size();
		mouse->currentCamController->SetPos(p);
	}
	selectedUnits.SelectGroup(num);
}

void CGroupHandler::FindDlls(void)
{
	std::vector<std::string> match;
	std::string dir("AI/Helper-libs");
#ifdef _WIN32
	match = filesystem.FindFiles(dir,"*.dll");
#elif defined(__APPLE__)
	match = filesystem.FindFiles(dir,"*.dylib");
#else
	match = filesystem.FindFiles(dir,"*.so");
#endif
	for (std::vector<std::string>::iterator it = match.begin(); it != match.end(); it++) {
		TestDll(*it);
	}
}

CGroup* CGroupHandler::CreateNewGroup(AIKey aiKey)
{
	if(freeGroups.empty()){
		CGroup* group=new CGroup(aiKey,firstUnusedGroup++,this);
		groups.push_back(group);
		if(group!=groups[group->id]){
			handleerror(0,"Id error when creating group","Error",0);
		}
		return group;
	} else {
		int id=freeGroups.back();
		freeGroups.pop_back();
		CGroup* group=new CGroup(aiKey,id,this);
		groups[id]=group;
		return group;
	}
}

void CGroupHandler::RemoveGroup(CGroup* group)
{
	if(group->id<10){
		info->AddLine("Warning trying to remove hotkey group %i",group->id);
		return;
	}
	if(selectedUnits.selectedGroup==group->id)
		selectedUnits.ClearSelected();
	groups[group->id]=0;
	freeGroups.push_back(group->id);
	delete group;
}

map<AIKey,string> CGroupHandler::GetSuitedAis(set<CUnit*> units)
{
	typedef bool (* ISUNITSUITED)(unsigned aiNumber,const UnitDef* unitDef);
	ISUNITSUITED IsUnitSuited;

	map<AIKey,string> suitedAis;
	suitedAis[defaultKey]="default";

	map<AIKey,string>::iterator aai;
	for(aai=availableAI.begin();aai!=availableAI.end();++aai)
	{
		SharedLib *lib;
		const AIKey& aiKey = aai->first;
		lib = SharedLib::instantiate(aiKey.dllName);
		IsUnitSuited = (ISUNITSUITED)lib->FindAddress("IsUnitSuited");
		bool suited = false;
		set<CUnit*>::iterator ui;
		for(ui=units.begin();ui!=units.end();++ui)
		{
			const UnitDef* ud = (*ui)->unitDef;
			if(IsUnitSuited(aiKey.aiNumber,ud))
			{
				suited = true;
				break;
			}
		}
		if(suited)
			suitedAis[aiKey]=aai->second;
		delete lib;
	}

	lastSuitedAis = suitedAis;
	return suitedAis;
}
