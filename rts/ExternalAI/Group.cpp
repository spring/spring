// Group.cpp: implementation of the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Group.h"
#include "IGroupAI.h"
#include "Sim/Units/Unit.h"
#include "GroupAiCallback.h"
#include "GroupHandler.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/InfoConsole.h"
#include <boost/filesystem/path.hpp>
#include "Platform/errorhandler.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroup::CGroup(string dllName,int id,CGroupHandler* grouphandler)
: lastCommandPage(0),
	id(id),
	ai(0),
	currentAiNum(0),
	handler(grouphandler),
	lib(0)
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
	if(ai)
		ReleaseAI(ai);
	if(lib)
		delete lib;
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
	if(ai)
		ReleaseAI(ai);
	if(lib)
		delete lib;

	ai=0;
	if(dllName=="default"){
		return;
	}

	boost::filesystem::path p(dllName,boost::filesystem::native);
	lib = SharedLib::instantiate(dllName.c_str());
	if (lib==0) 
		handleerror(NULL,dllName.c_str(),"Could not find AI dll",MBF_OK|MBF_EXCL);
	
	GetGroupAiVersion = (GETGROUPAIVERSION)lib->FindAddress("GetGroupAiVersion");
	if (GetGroupAiVersion==0)
		handleerror(NULL,dllName.c_str(),"Incorrect AI dll",MBF_OK|MBF_EXCL);
	
	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION)
		handleerror(NULL,dllName.c_str(),"Incorrect AI dll version",MBF_OK|MBF_EXCL);
	
	GetNewAI = (GETNEWAI)lib->FindAddress("GetNewAI");
	ReleaseAI = (RELEASEAI)lib->FindAddress("ReleaseAI");

	ai=GetNewAI();
	ai->InitAi(callback);
	
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
