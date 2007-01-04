#include "StdAfx.h"
#include "GlobalAI.h"
#include "IGlobalAI.h"
#include "GlobalAICallback.h"
#include "GroupHandler.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "mmgr.h"
#include "ExternalAI/GlobalAICInterface/AbicProxy.h"
#include "LogOutput.h"


CGlobalAI::CGlobalAI(int team, const char* dll): team(team), cheatevents(false)
{
	ai = 0;

	if (!filesystem.GetFilesize(dll)) {
		handleerror(NULL, dll, "Could not find AI lib", MBF_OK | MBF_EXCL);
		return;
	}

	lib = SharedLib::Instantiate(dll);
	_IsCInterface = (ISCINTERFACE) lib -> FindAddress("IsCInterface");

	// check if IsCInterface function exported and return value true
	if ( _IsCInterface != 0 && _IsCInterface() == 1) {
		// presents C interface
		logOutput << dll <<  " has C interface\n";
		IsCInterface = true;

		// keep as AbicProxy, so InitAI works ok
		AbicProxy* ai = SAFE_NEW AbicProxy;
		this->ai = ai;

		gh = SAFE_NEW CGroupHandler(team);
		callback = SAFE_NEW CGlobalAICallback(this);
		ai->InitAI(dll, callback, team);
	} else {
		// presents C++ interface
		logOutput << dll <<  " has C++ interface\n";
		IsCInterface = false;
	
		GetGlobalAiVersion = (GETGLOBALAIVERSION) lib->FindAddress("GetGlobalAiVersion");

		if (GetGlobalAiVersion == 0) {
			handleerror(NULL, dll, "Incorrect Global AI dll", MBF_OK|MBF_EXCL);
			return;
		}
		
		int i = GetGlobalAiVersion();

		if (i != GLOBAL_AI_INTERFACE_VERSION) {
			handleerror(NULL, dll, "Incorrect Global AI dll version", MBF_OK | MBF_EXCL);
			return;
		}

		GetNewAI = (GETNEWAI) lib->FindAddress("GetNewAI");
		ReleaseAI = (RELEASEAI) lib->FindAddress("ReleaseAI");

		ai = GetNewAI();
		gh = SAFE_NEW CGroupHandler(team);
		callback = SAFE_NEW CGlobalAICallback(this);
		ai->InitAI(callback, team);
	}
}

void CGlobalAI::PreDestroy()
{
	callback->noMessages = true;
}

CGlobalAI::~CGlobalAI(void)
{
	if (ai) {
		if (!IsCInterface) {
			ReleaseAI(ai);
		}

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
