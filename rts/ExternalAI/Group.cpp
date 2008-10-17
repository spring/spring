// Group.cpp: implementation of the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Group.h"
#include "IGroupAI.h"
#include "GroupAiCallback.h"
#include "GroupHandler.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/Unit.h"
#include "System/EventHandler.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "mmgr.h"
#include "creg/STL_List.h"

AIKey defaultKey(std::string("default"),0);

CR_BIND_DERIVED(CGroup,CObject,(defaultKey,0,NULL))

CR_REG_METADATA(CGroup, (
				CR_MEMBER(id),
				CR_MEMBER(units),
				CR_MEMBER(myCommands),
				CR_MEMBER(lastCommandPage),
				CR_MEMBER(currentAiNum),
				CR_MEMBER(currentAiKey),
				CR_MEMBER(handler),
				CR_SERIALIZER(Serialize),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(AIKey,)

CR_REG_METADATA(AIKey, (
				CR_MEMBER(dllName),
				CR_MEMBER(aiNumber)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroup::CGroup(AIKey aiKey,int id,CGroupHandler* grouphandler)
: lastCommandPage(0),
	id(id),
	ai(0),
	currentAiNum(0),
	handler(grouphandler),
	lib(0)
{
	if (grouphandler) callback=SAFE_NEW CGroupAICallback(this); else callback=0;
	SetNewAI(aiKey);

	int a=0;
	if (grouphandler) for(map<AIKey,string>::iterator aai=handler->lastSuitedAis.begin();aai!=handler->lastSuitedAis.end() && aiKey!=aai->first;++aai){
		a++;
	}
	currentAiNum=a;
}

CGroup::~CGroup()
{
	ClearUnits();			//shouldnt have any units left but just to be sure
	if(ai)
		ReleaseAI(currentAiKey.aiNumber,ai);
	if(lib)
		delete lib;
	delete callback;
}

void CGroup::Serialize(creg::ISerializer *s)
{
}

void CGroup::PostLoad()
{
	callback=SAFE_NEW CGroupAICallback(this);
	int a=0;
	for(map<AIKey,string>::iterator aai=handler->lastSuitedAis.begin();aai!=handler->lastSuitedAis.end() && currentAiKey!=aai->first;++aai){
		a++;
	}
	currentAiNum=a;
/*
	if(ai) {
		ReleaseAI(currentAiKey.aiNumber,ai);
		ai = 0;
	}
*/
	if(lib) {
		delete lib;
		lib = 0;
	}

	if(currentAiKey.dllName=="default"){
		return;
	}

	lib = SharedLib::Instantiate(currentAiKey.dllName);
	if (lib==0)
		handleerror(NULL,currentAiKey.dllName.c_str(),"Could not find AI dll",MBF_OK|MBF_EXCL);

	GetGroupAiVersion = (GETGROUPAIVERSION)lib->FindAddress("GetGroupAiVersion");
	if (GetGroupAiVersion==0)
		handleerror(NULL,currentAiKey.dllName.c_str(),"Incorrect AI dll",MBF_OK|MBF_EXCL);

	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION)
		handleerror(NULL,currentAiKey.dllName.c_str(),"Incorrect AI dll version",MBF_OK|MBF_EXCL);

	GetNewAI = (GETNEWAI)lib->FindAddress("GetNewAI");
	ReleaseAI = (RELEASEAI)lib->FindAddress("ReleaseAI");
	IsUnitSuited = (ISUNITSUITED)lib->FindAddress("IsUnitSuited");

	typedef bool (* ISLOADSUPPORTED)(unsigned aiNumber);
	ISLOADSUPPORTED IsLoadSupported;
	IsLoadSupported = (ISLOADSUPPORTED)lib->FindAddress("IsLoadSupported");

	ai=GetNewAI(currentAiKey.aiNumber);
	if (IsLoadSupported&&IsLoadSupported(currentAiKey.aiNumber)) {
		return;
	}
	ai->InitAi(callback);

	CUnitSet unitBackup=units;

	for(CUnitSet::iterator ui=unitBackup.begin();ui!=unitBackup.end();++ui)
	{
		if(IsUnitSuited(currentAiKey.aiNumber,(*ui)->unitDef))
		{
			if(ai->AddUnit((*ui)->id))
			{
				continue;
			}
		}
		units.erase(*ui);
		(*ui)->group=0;
	}
}

bool CGroup::AddUnit(CUnit *unit)
{
	GML_STDMUTEX_LOCK(group); // AddUnit

	eventHandler.GroupChanged(id);

	units.insert(unit);
	if(ai)
	{
		if(IsUnitSuited(currentAiKey.aiNumber,unit->unitDef))
		{
			if(ai->AddUnit(unit->id))
			{
				return true;
			}
		}
		units.erase(unit);
		return false;
	}
	return true;
}

void CGroup::RemoveUnit(CUnit *unit)
{
	GML_STDMUTEX_LOCK(group); // RemoveUnit

	eventHandler.GroupChanged(id);
	if(ai)
		ai->RemoveUnit(unit->id);
	units.erase(unit);
}

void CGroup::SetNewAI(AIKey aiKey)
{
	eventHandler.GroupChanged(id);
	if(ai) {
		ReleaseAI(currentAiKey.aiNumber,ai);
		ai = 0;
	}
	if(lib) {
		delete lib;
		lib = 0;
	}

	currentAiKey=aiKey;
	if(aiKey.dllName=="default"){
		return;
	}

	lib = SharedLib::Instantiate(aiKey.dllName);
	if (lib==0)
		handleerror(NULL,aiKey.dllName.c_str(),"Could not find AI dll",MBF_OK|MBF_EXCL);

	GetGroupAiVersion = (GETGROUPAIVERSION)lib->FindAddress("GetGroupAiVersion");
	if (GetGroupAiVersion==0)
		handleerror(NULL,aiKey.dllName.c_str(),"Incorrect AI dll",MBF_OK|MBF_EXCL);

	int i=GetGroupAiVersion();

	if (i!=AI_INTERFACE_VERSION)
		handleerror(NULL,aiKey.dllName.c_str(),"Incorrect AI dll version",MBF_OK|MBF_EXCL);

	GetNewAI = (GETNEWAI)lib->FindAddress("GetNewAI");
	ReleaseAI = (RELEASEAI)lib->FindAddress("ReleaseAI");
	IsUnitSuited = (ISUNITSUITED)lib->FindAddress("IsUnitSuited");

	ai=GetNewAI(currentAiKey.aiNumber);
	ai->InitAi(callback);

	CUnitSet unitBackup=units;

	for(CUnitSet::iterator ui=unitBackup.begin();ui!=unitBackup.end();++ui)
	{
		if(IsUnitSuited(currentAiKey.aiNumber,(*ui)->unitDef))
		{
			if(ai->AddUnit((*ui)->id))
			{
				continue;
			}
		}
		units.erase(*ui);
		(*ui)->group=0;
	}
}

void CGroup::Update()
{
	if(units.empty() && id>=10 && /*handler==grouphandler*/handler->team==gu->myTeam){		//last check is a hack so globalai groups dont get erased
		handler->RemoveGroup(this);
		return;
	}
	if(ai)
		ai->Update();
}

void CGroup::DrawCommands()
{
//	GML_STDMUTEX_LOCK(cai); // DrawCommands. Not needed, protected via CGroupHandler
	if(units.empty() && id>=10 && /*handler==grouphandler*/handler->team==gu->myTeam){		//last check is a hack so globalai groups dont get erased
		handler->RemoveGroup(this);
		return;
	}
	if(ai)
		ai->DrawCommands();
}

const vector<CommandDescription>& CGroup::GetPossibleCommands()
{
	CommandDescription c;

	myCommands.clear();

	if(ai){
		/*
		We deepcopy the vector member-by-member, because relying on copy
		constructors, as in 'myCommands=ai->GetPossibleCommands();', would mean
		marshalling of container objects (std::string particularly) over DLL
		boundaries.

		On MinGW (at least) this caused a crash when clearing the last Group AI:
		When the code here was executed, the std::string implementation did not
		copy the strings, but it incremented a reference counter and stored
		a pointer to the data in the DLL. When the last group AI was cleared
		the DLL was unloaded from memory.
		Then, after CGroup::~CGroup() execution finished, the myCommands destructor
		would be called, which would call all the std::string destructors, which
		would try to decrement a reference counter in inaccesible (DLL) memory ->
		segfault
		*/
		const vector<CommandDescription>& aipc=ai->GetPossibleCommands();
		for (vector<CommandDescription>::const_iterator i = aipc.begin(); i != aipc.end(); ++i) {
			c.id   = i->id;
			c.type = i->type;
			c.action    = i->action.c_str();
			c.name      = i->name.c_str();
			c.iconname  = i->iconname.c_str();
			c.mouseicon = i->mouseicon.c_str();
			c.tooltip   = i->tooltip.c_str();
			c.showUnique  = i->showUnique;
			c.hidden      = i->hidden;
			c.disabled    = i->disabled;
			c.onlyTexture = i->onlyTexture;
			for (vector<string>::const_iterator j = i->params.begin(); j != i->params.end(); ++j) {
				c.params.push_back(j->c_str());
			}
			myCommands.push_back(c);
			c.params.clear();
		}
	}

	c.id=CMD_AISELECT;
	c.type=CMDTYPE_COMBO_BOX;
	c.name="Select AI";
	c.tooltip="Select the AI to use for this group from the available AIs";
	c.showUnique=true;

	char t[50];
	sprintf(t,"%i",currentAiNum+1);
	c.params.push_back(t);
	c.params.push_back("Cancel");
	map<AIKey,string>::iterator aai;
	map<AIKey,string> suitedAis = handler->GetSuitedAis(units);
	for(aai=suitedAis.begin();aai!=suitedAis.end();++aai){
		c.params.push_back((aai->second).c_str());
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
		if (c.params[0]!=0) {
			map<AIKey,string>::iterator aai;
			int a=0;
			for(aai=handler->lastSuitedAis.begin();aai!=handler->lastSuitedAis.end() && a<c.params[0]-1;++aai){
				a++;
			}
			currentAiNum=a;
			SetNewAI(aai->first);
			selectedUnits.PossibleCommandChange(0);
		}
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
	GML_STDMUTEX_LOCK(group); // ClearUnits

	eventHandler.GroupChanged(id);
	while(!units.empty()){
		(*units.begin())->SetGroup(0);
	}
}
