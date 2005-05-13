#include "StdAfx.h"
// GroupHandler.cpp: implementation of the CGroupHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "GroupHandler.h"
#include <windows.h>
#ifndef NO_IO
#include <io.h>
#endif
#include "Group.h"
#include "IGroupAI.h"
#include "InfoConsole.h"
#include "SelectedUnits.h"
#include "TimeProfiler.h"
#include "Unit.h"
#include "GroupHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupHandler* grouphandler;
extern bool	keys[256];

CGroupHandler::CGroupHandler()
: firstUnusedGroup(10)
{
	availableAI["default"]="default";
	FindDlls();

	grouphandler=this;		//grouphandler is used in group constructors
	for(int a=0;a<10;++a){
		armies.push_back(new CGroup("default",a));
	}
}

CGroupHandler::~CGroupHandler()
{
	for(int a=0;a<10;++a)
		delete armies[a];
}

void CGroupHandler::Update()
{
START_TIME_PROFILE;
	for(std::vector<CGroup*>::iterator ai=armies.begin();ai!=armies.end();++ai)
		if((*ai)!=0)
			(*ai)->Update();
END_TIME_PROFILE("Group AI");
}

void CGroupHandler::TestDll(string name)
{
	typedef int (WINAPI* GETGROUPAIVERSION)();
	typedef void (WINAPI* GETAINAME)(char* c);
	
	HINSTANCE m_hDLL;
	GETGROUPAIVERSION GetGroupAiVersion;
	GETAINAME GetAiName;

	m_hDLL=LoadLibrary(name.c_str());
	if (m_hDLL==0){
		MessageBox(NULL,name.c_str(),"Cant load dll",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	GetGroupAiVersion = (GETGROUPAIVERSION)GetProcAddress(m_hDLL,"GetGroupAiVersion");
	if (GetGroupAiVersion==0){
		MessageBox(NULL,name.c_str(),"Incorrect AI dll",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION){
		MessageBox(NULL,name.c_str(),"Incorrect AI dll version",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	
	GetAiName = (GETAINAME)GetProcAddress(m_hDLL,"GetAiName");
	if (GetAiName==0){
		MessageBox(NULL,name.c_str(),"Incorrect AI dll",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	char c[500];
	GetAiName(c);

	availableAI[name]=c;
//	(*info) << name.c_str() << " " << c << "\n";
	FreeLibrary(m_hDLL);
}

void CGroupHandler::GroupCommand(int num)
{
	if(keys[VK_CONTROL]){
		if(!keys[VK_SHIFT])
			armies[num]->ClearUnits();
		set<CUnit*>::iterator ui;
		for(ui=selectedUnits.selectedUnits.begin();ui!=selectedUnits.selectedUnits.end();++ui){
			(*ui)->SetGroup(armies[num]);
		}
	}
	selectedUnits.SelectGroup(num);
}

void CGroupHandler::FindDlls(void)
{
#ifndef NO_DLL
	struct _finddata_t files;    
	long hFile;
	int morefiles=0;

	if( (hFile = _findfirst( "aidll\\*.dll", &files )) == -1L ){
		morefiles=-1;
	}

	int numfiles=0;
	while(morefiles==0){
//		info->AddLine("Testing %s",files.name);
		TestDll(string("aidll\\")+files.name);
		
		morefiles=_findnext( hFile, &files ); 
	}

	if(availableAI.empty()){
		MessageBox(0,"Fatal error","Need at least one valid ai dll in ./aidll",0);
		exit(0);
	}
#endif //NO_DLL
}

CGroup* CGroupHandler::CreateNewGroup(string ainame)
{
	if(freeArmies.empty()){
		CGroup* group=new CGroup(ainame,firstUnusedGroup++);
		armies.push_back(group);
		if(group!=armies[group->id]){
			MessageBox(0,"Id error when creating group","Error",0);
		}
		return group;
	} else {
		int id=freeArmies.back();
		freeArmies.pop_back();
		CGroup* group=new CGroup(ainame,id);
		armies[id]=group;
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
	armies[group->id]=0;
	freeArmies.push_back(group->id);
	delete group;
}
