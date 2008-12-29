#include "StdAfx.h"
// GroupHandler.cpp: implementation of the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "GroupHandler.h"
#include "Group.h"
#include "IGroupAI.h"
#include "LogOutput.h"
#include "Game/SelectedUnits.h"
#include "TimeProfiler.h"
#include "Sim/Units/Unit.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/CameraHandler.h"
#include "Platform/SharedLib.h"
#include "Platform/errorhandler.h"
#include "FileSystem/FileSystem.h"
#include "SDL_types.h"
#include "SDL_keysym.h"
#include "mmgr.h"

//CGroupHandler* grouphandler;
CGroupHandler* grouphandlers[MAX_TEAMS];
extern Uint8 *keys;

CR_BIND(CGroupHandler, (0))

CR_REG_METADATA(CGroupHandler, (
				CR_MEMBER(groups),
				CR_MEMBER(team),
				CR_MEMBER(freeGroups),
				CR_MEMBER(firstUnusedGroup)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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

void CGroupHandler::PostLoad()
{
}

void CGroupHandler::Load(std::istream *s)
{
	for(std::vector<CGroup*>::iterator ai=groups.begin();ai!=groups.end();++ai)
		if((*ai)&&((*ai)->ai)) {
			(*ai)->ai->Load((IGroupAICallback*)(*ai)->callback,s);
		}
}

void CGroupHandler::Save(std::ostream *s)
{
	for(std::vector<CGroup*>::iterator ai=groups.begin();ai!=groups.end();++ai)
		if((*ai)&&((*ai)->ai)) {
			(*ai)->ai->Save(s);
		}
}

void CGroupHandler::Update()
{
	SCOPED_TIMER("Group AI");
	for(std::vector<CGroup*>::iterator ai=groups.begin();ai!=groups.end();++ai)
		if((*ai)!=0)
			(*ai)->Update();
}

void CGroupHandler::DrawCommands()
{
//	GML_STDMUTEX_LOCK(cai); // not needed, protected via SelectedUnits::DrawCommands
	for(std::vector<CGroup*>::iterator ai=groups.begin();ai!=groups.end();++ai)
		if((*ai)!=0)
			(*ai)->DrawCommands();
}

void CGroupHandler::TestDll(std::string name)
{
	typedef int (* GETGROUPAIVERSION)();
	typedef const char ** (* GETAINAMELIST)();
	typedef bool (* ISUNITSUITED)(unsigned aiNumber,const UnitDef* unitDef);

	SharedLib *lib;
	GETGROUPAIVERSION GetGroupAiVersion;
	GETAINAMELIST GetAiNameList;
	ISUNITSUITED IsUnitSuited;

	lib = SharedLib::Instantiate(name);
	if (!lib){
		logOutput.Print ("Cant load dll: %s",name.c_str());
		return;
	}

	GetGroupAiVersion = (GETGROUPAIVERSION)lib->FindAddress("GetGroupAiVersion");
	if (!GetGroupAiVersion){
		logOutput.Print("Incorrect AI dll(%s): No GetGroupAiVersion function found", name.c_str());
		delete lib;
		return;
	}

	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION){
		logOutput.Print("AI dll %s has incorrect version", name.c_str());
		delete lib;
		return;
	}

	IsUnitSuited = (ISUNITSUITED)lib->FindAddress("IsUnitSuited");
	if (!IsUnitSuited){
		logOutput.Print ("No IsUnitSuited function found in AI dll %s", name.c_str());
		delete lib;
		return;
	}

	GetAiNameList = (GETAINAMELIST)lib->FindAddress("GetAiNameList");
	if (!GetAiNameList){
		logOutput.Print("No GetAiNameList function found in AI dll %s",name.c_str());
		delete lib;
		return;
	}

	const char ** aiNameList=GetAiNameList();

	for(unsigned i=0;aiNameList[i]!=NULL;i++){
		AIKey key;
		key.dllName=name;
		key.aiNumber=i;
		availableAI[key] = std::string(aiNameList[i]);
	}
//	logOutput << name.c_str() << " " << c << "\n";
	delete lib;
}

void CGroupHandler::GroupCommand(int num)
{
	GML_RECMUTEX_LOCK(sel); // GroupCommand
	GML_STDMUTEX_LOCK(group); // GroupCommand

	if (keys[SDLK_LCTRL]) {
		if (!keys[SDLK_LSHIFT]) {
			groups[num]->ClearUnits();
		}
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		CUnitSet::const_iterator ui;
		for(ui = selUnits.begin(); ui != selUnits.end(); ++ui) {
			(*ui)->SetGroup(groups[num]);
		}
	}
	else if (keys[SDLK_LSHIFT])  {
		// do not select the group, just add its members to the current selection
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			selectedUnits.AddUnit(*gi);
		}
		return;
	}
	else if (keys[SDLK_LALT] || keys[SDLK_LMETA])  {
		// do not select the group, just toggle its members with the current selection
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			if (selUnits.find(*gi) == selUnits.end()) {
				selectedUnits.AddUnit(*gi);
			} else {
				selectedUnits.RemoveUnit(*gi);
			}
		}
		return;
	}

	if(selectedUnits.selectedGroup==num && !groups[num]->units.empty()){
		float3 p(0,0,0);
		for(CUnitSet::iterator gi=groups[num]->units.begin();gi!=groups[num]->units.end();++gi){
			p+=(*gi)->pos;
		}
		p/=groups[num]->units.size();
		camHandler->GetCurrentController().SetPos(p);
	}

	selectedUnits.SelectGroup(num);
}

void CGroupHandler::GroupCommand(int num, const std::string& cmd)
{
	GML_RECMUTEX_LOCK(sel); // GroupCommand
	GML_STDMUTEX_LOCK(group); // GroupCommand

	if ((cmd == "set") || (cmd == "add")) {
		if (cmd == "set") {
			groups[num]->ClearUnits();
		}
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		CUnitSet::const_iterator ui;
		for(ui = selUnits.begin(); ui != selUnits.end(); ++ui) {
			(*ui)->SetGroup(groups[num]);
		}
	}
	else if (cmd == "selectadd")  {
		// do not select the group, just add its members to the current selection
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			selectedUnits.AddUnit(*gi);
		}
		return;
	}
	else if (cmd == "selectclear")  {
		// do not select the group, just remove its members from the current selection
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			selectedUnits.RemoveUnit(*gi);
		}
		return;
	}
	else if (cmd == "selecttoggle")  {
		// do not select the group, just toggle its members with the current selection
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		CUnitSet::const_iterator gi;
		for (gi = groups[num]->units.begin(); gi != groups[num]->units.end(); ++gi) {
			if (selUnits.find(*gi) == selUnits.end()) {
				selectedUnits.AddUnit(*gi);
			} else {
				selectedUnits.RemoveUnit(*gi);
			}
		}
		return;
	}

	if(selectedUnits.selectedGroup==num && !groups[num]->units.empty()){
		float3 p(0,0,0);
		for(CUnitSet::iterator gi=groups[num]->units.begin();gi!=groups[num]->units.end();++gi){
			p+=(*gi)->pos;
		}
		p/=groups[num]->units.size();
		camHandler->GetCurrentController().SetPos(p);
	}

	selectedUnits.SelectGroup(num);
}

void CGroupHandler::FindDlls(void)
{
	std::vector<std::string> match;
	std::string dir("AI/Helper-libs");
	match = filesystem.FindFiles(dir, std::string("*.") + SharedLib::GetLibExtension());

	for (std::vector<std::string>::iterator it = match.begin(); it != match.end(); it++)
		TestDll(*it);
}

CGroup* CGroupHandler::CreateNewGroup(AIKey aiKey)
{
	GML_STDMUTEX_LOCK(group); // GroupCommand

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
	GML_RECMUTEX_LOCK(sel); // RemoveGroup
	GML_STDMUTEX_LOCK(group); // RemoveGroup

	if(group->id<10){
		logOutput.Print("Warning trying to remove hotkey group %i",group->id);
		return;
	}
	if(selectedUnits.selectedGroup==group->id)
		selectedUnits.ClearSelected();
	groups[group->id]=0;
	freeGroups.push_back(group->id);
	delete group;
}

std::map<AIKey, std::string> CGroupHandler::GetSuitedAis(const CUnitSet& units)
{
	GML_RECMUTEX_LOCK(sel); // GetSuitedAis

	typedef bool (* ISUNITSUITED)(unsigned aiNumber,const UnitDef* unitDef);
	ISUNITSUITED IsUnitSuited;

	std::map<AIKey, std::string> suitedAis;
	suitedAis[defaultKey]="default";

	std::map<AIKey, std::string>::iterator aai;
	for(aai=availableAI.begin();aai!=availableAI.end();++aai)
	{
		SharedLib *lib;
		const AIKey& aiKey = aai->first;
		lib = SharedLib::Instantiate(aiKey.dllName);
		IsUnitSuited = (ISUNITSUITED)lib->FindAddress("IsUnitSuited");
		bool suited = false;
		CUnitSet::const_iterator ui;
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
