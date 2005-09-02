// Group.cpp: implementation of the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Group.h"
#include "IGroupAI.h"
#include "Unit.h"
#include "GroupAiCallback.h"
#include "GroupHandler.h"
#include "SelectedUnits.h"
#include "InfoConsole.h"
#include "Group.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroup::CGroup(string dllName,int id,CGroupHandler* grouphandler)
: lastCommandPage(0),
	id(id),
	ai(0),
#ifndef NO_DLL
	m_hDLL(0),
#endif
	currentAiNum(0),
	handler(grouphandler)
{
	callback=new CGroupAICallback(this);
	SetNewAI(dllName);

	int a=0;
	for(map<string,string>::iterator aai=handler->availableAI.begin();aai!=handler->availableAI.end() && dllName!=aai->first;++aai){
		a++;
	}
	currentAiNum=a;
}

CGroup::~CGroup()
{
	ClearUnits();			//shouldnt have any units left but just to be sure
#ifndef NO_DLL
	if(ai)
		ReleaseAI(ai);
	if(m_hDLL)
		FreeLibrary(m_hDLL);
#endif
	delete callback;
}

bool CGroup::AddUnit(CUnit *unit)
{
	units.insert(unit);
	if(ai && !ai->AddUnit(unit->id)){
		units.erase(unit);
		return false;
	}
	return true;
}

void CGroup::RemoveUnit(CUnit *unit)
{
	if(ai)
		ai->RemoveUnit(unit->id);
	units.erase(unit);
}

void CGroup::SetNewAI(string dllName)
{
#ifndef NO_DLL
	if(ai)
		ReleaseAI(ai);
	if(m_hDLL)
		FreeLibrary(m_hDLL);

	ai=0;
	m_hDLL=0;
	if(dllName=="default"){
		return;
	}

	m_hDLL=LoadLibrary(dllName.c_str());
	if (m_hDLL==0) 
		MessageBox(NULL,dllName.c_str(),"Could not find AI dll",MB_OK|MB_ICONEXCLAMATION);
	
	GetGroupAiVersion = (GETGROUPAIVERSION)GetProcAddress(m_hDLL,"GetGroupAiVersion");
	if (GetGroupAiVersion==0)
		MessageBox(NULL,dllName.c_str(),"Incorrect AI dll",MB_OK|MB_ICONEXCLAMATION);
	
	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION)
		MessageBox(NULL,dllName.c_str(),"Incorrect AI dll version",MB_OK|MB_ICONEXCLAMATION);
	
	GetNewAI = (GETNEWAI)GetProcAddress(m_hDLL,"GetNewAI");
	ReleaseAI = (RELEASEAI)GetProcAddress(m_hDLL,"ReleaseAI");

	ai=GetNewAI();
	ai->InitAi(callback);
#endif
	
	set<CUnit*> unitBackup=units;

	for(set<CUnit*>::iterator ui=unitBackup.begin();ui!=unitBackup.end();++ui){
		if(!ai->AddUnit((*ui)->id)){
			units.erase(*ui);
			(*ui)->group=0;
		}
	}
}

void CGroup::Update()
{
	if(units.empty() && id>=10 && handler==grouphandler){		//last check is a hack so globalai groups dont get erased
		handler->RemoveGroup(this);
		grouphandler->RemoveGroup(this);
		return;
	}
	if(ai)
		ai->Update();
}

const vector<CommandDescription>& CGroup::GetPossibleCommands()
{
	if(ai){
		myCommands=ai->GetPossibleCommands();
	} else {
		myCommands.clear();
	}

	CommandDescription c;
	c.id=CMD_AISELECT;
	c.type=CMDTYPE_COMBO_BOX;
	c.name="Select AI";
	c.tooltip="Select the AI to use for this group from the available AIs";
	c.showUnique=true;
	c.key='Q';
	c.switchKeys=CONTROL_KEY;

	char t[50];
	sprintf(t,"%i",currentAiNum);
	c.params.push_back(t);
	map<string,string>::iterator aai;
	for(aai=handler->availableAI.begin();aai!=handler->availableAI.end();++aai){
		c.params.push_back(aai->second.c_str());
	}
	myCommands.push_back(c);
	return myCommands;
}

int CGroup::GetDefaultCmd(CUnit *unit,CFeature* feature)
{
	if(!ai)
		return CMD_STOP;

	if(unit==0)
		return ai->GetDefaultCmd(0);
	return ai->GetDefaultCmd(unit->id);
}

void CGroup::GiveCommand(Command c)
{
	if(c.id==CMD_AISELECT){
		map<string,string>::iterator aai;
		int a=0;
		for(aai=handler->availableAI.begin();aai!=handler->availableAI.end() && a<c.params[0];++aai){
			a++;
		}
		currentAiNum=(int)c.params[0];
		SetNewAI(aai->first);
		selectedUnits.PossibleCommandChange(0);
	} else {
		if(ai)
			ai->GiveCommand(&c);
	}
}

void CGroup::CommandFinished(int unit,int type)
{
	if(ai)
		ai->CommandFinished(unit,type);
}

void CGroup::ClearUnits(void)
{
	while(!units.empty()){	
		(*units.begin())->SetGroup(0);
	}
}
