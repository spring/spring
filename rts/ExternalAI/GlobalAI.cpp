#include "StdAfx.h"
#include "GlobalAI.h"
#include "IGlobalAI.h"
#include "GlobalAICallback.h"
#include "GroupHandler.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "mmgr.h"

CGlobalAI::CGlobalAI(int team, const char* dll)
: team(team)
{
	ai=0;

	if (!filesystem.GetFilesize(dll)) {
		handleerror(NULL,dll,"Could not find AI lib",MBF_OK|MBF_EXCL);
		return;
	}

	lib = SharedLib::instantiate(dll);
	
	GetGlobalAiVersion = (GETGLOBALAIVERSION)lib->FindAddress("GetGlobalAiVersion");
	if (GetGlobalAiVersion==0){
		handleerror(NULL,dll,"Incorrect Global AI dll",MBF_OK|MBF_EXCL);
		return;
	}
	
	int i=GetGlobalAiVersion();

	if (i!=GLOBAL_AI_INTERFACE_VERSION){
		handleerror(NULL,dll,"Incorrect Global AI dll version",MBF_OK|MBF_EXCL);
		return;
	}

	GetNewAI = (GETNEWAI)lib->FindAddress("GetNewAI");
	ReleaseAI = (RELEASEAI)lib->FindAddress("ReleaseAI");

	ai=GetNewAI();
	gh=new CGroupHandler(team);
	callback=new CGlobalAICallback(this);
	ai->InitAI(callback,team);
}

void CGlobalAI::PreDestroy ()
{
	callback->noMessages = true;
}

CGlobalAI::~CGlobalAI(void)
{
	if(ai){
		ReleaseAI(ai);
		delete lib;
		delete callback;
		delete gh;
	}
}

void CGlobalAI::Update(void)
{
	gh->Update();
	ai->Update();
}

IMPLEMENT_PURE_VIRTUAL(IGlobalAI::~IGlobalAI())
