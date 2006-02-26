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
#include "Platform/filefunctions.h"
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
	availableAI["default"]="default";
	FindDlls();

	for(int a=0;a<10;++a){
		groups.push_back(new CGroup("default",a,this));
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
	typedef void (* GETAINAME)(char* c);
	
	SharedLib *lib;
	GETGROUPAIVERSION GetGroupAiVersion;
	GETAINAME GetAiName;

	fs::path p(name,fs::native);
	lib = SharedLib::instantiate(p.native_file_string());
	if (!lib){
		handleerror(NULL,name.c_str(),"Cant load dll",MBF_OK|MBF_EXCL);
		return;
	}

	GetGroupAiVersion = (GETGROUPAIVERSION)lib->FindAddress("GetGroupAiVersion");
	if (!GetGroupAiVersion){
		handleerror(NULL,name.c_str(),"Incorrect AI dll",MBF_OK|MBF_EXCL);
		return;
	}

	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION){
		handleerror(NULL,name.c_str(),"Incorrect AI dll version",MBF_OK|MBF_EXCL);
		return;
	}
	
	GetAiName = (GETAINAME)lib->FindAddress("GetAiName");
	if (!GetAiName){
		handleerror(NULL,name.c_str(),"Incorrect AI dll",MBF_OK|MBF_EXCL);
		return;
	}

	char c[500];
	GetAiName(c);

	availableAI[name]=c;
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
	std::vector<fs::path> match;
	fs::path dir("aidll",fs::native);
#ifdef _WIN32
	match = find_files(dir,"*.dll");
#elif defined(__APPLE__)
	match = find_files(dir,"*.dylib");
#else
	match = find_files(dir,"*.so");
#endif
	for (std::vector<fs::path>::iterator it = match.begin(); it != match.end(); it++) {
		TestDll(it->string());
	}
}

CGroup* CGroupHandler::CreateNewGroup(string ainame)
{
	if(freeGroups.empty()){
		CGroup* group=new CGroup(ainame,firstUnusedGroup++,this);
		groups.push_back(group);
		if(group!=groups[group->id]){
			handleerror(0,"Id error when creating group","Error",0);
		}
		return group;
	} else {
		int id=freeGroups.back();
		freeGroups.pop_back();
		CGroup* group=new CGroup(ainame,id,this);
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
