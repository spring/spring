#include "GlobalAI.h"
#include "IGlobalAI.h"
#include "GlobalAICallback.h"
#include "GroupHandler.h"
#include <boost/filesystem/operations.hpp>
#include "errorhandler.h"
#include "SharedLib.h"
//#include "mmgr.h"

CGlobalAI::CGlobalAI(int team, const char* dll)
: team(team)
{
	ai=0;
	boost::filesystem::path l(dll,boost::filesystem::native);

	if (!boost::filesystem::exists(l)) {
		handleerror(NULL,dll,"Could not find AI lib",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	lib = SharedLib::instantiate(l.native_file_string());
	
	GetGlobalAiVersion = (GETGLOBALAIVERSION)lib->FindAddress("GetGlobalAiVersion");
	if (GetGlobalAiVersion==0){
		handleerror(NULL,dll,"Incorrect Global AI dll",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	
	int i=GetGlobalAiVersion();

	if (i!=GLOBAL_AI_INTERFACE_VERSION){
		handleerror(NULL,dll,"Incorrect Global AI dll version",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	GetNewAI = (GETNEWAI)lib->FindAddress("GetNewAI");
	ReleaseAI = (RELEASEAI)lib->FindAddress("ReleaseAI");

	ai=GetNewAI();
	callback=new CGlobalAICallback(this);
	gh=new CGroupHandler(team);
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
