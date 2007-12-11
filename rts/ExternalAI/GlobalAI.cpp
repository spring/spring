#include "StdAfx.h"
#include "GlobalAI.h"
#include "IGlobalAI.h"
#include "GlobalAICallback.h"
#include "GlobalAIHandler.h"
#include "GroupHandler.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "ExternalAI/GlobalAICInterface/AbicProxy.h"
#include "LogOutput.h"
#include "mmgr.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include <sstream>

CR_BIND_DERIVED(CGlobalAI,CObject,(0,NULL))

CR_REG_METADATA(CGlobalAI, (
				CR_MEMBER(team),
				CR_MEMBER(cheatevents),
				CR_MEMBER(dllName),
//				CR_MEMBER(gh),
				CR_SERIALIZER(Serialize),
				CR_POSTLOAD(PostLoad)
				));

void AIException(const char *what);

#define HANDLE_EXCEPTION  \
	catch (const std::exception& e) {	\
		if (globalAI->CatchException ()) {		\
			AIException(e.what());		\
			throw;						\
		} else throw;					\
	}									\
	catch (const char *s) {	\
		if (globalAI->CatchException ()) {		\
			AIException(s);				\
			throw;						\
		} else throw;					\
	}									\
	catch (...) {						\
		if (globalAI->CatchException ()) {		\
			AIException(0);				\
			throw;						\
		}else throw;					\
	}

CGlobalAI::CGlobalAI(int team, const char* dll): team(team), cheatevents(false),dllName(dll?dll:"")
{
	ai = 0;
	if (!dll) return;
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

//		gh = SAFE_NEW CGroupHandler(team);
		callback = SAFE_NEW CGlobalAICallback(this);
		try {
			ai->InitAI(dll, callback, team);
		} HANDLE_EXCEPTION;
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
			std::ostringstream tmp;
			tmp << "Incorrect Global AI dll version " << i << ", expected "
					<< GLOBAL_AI_INTERFACE_VERSION;
			handleerror(NULL, dll, tmp.str().c_str(), MBF_OK | MBF_EXCL);
			return;
		}

		GetNewAI = (GETNEWAI) lib->FindAddress("GetNewAI");
		ReleaseAI = (RELEASEAI) lib->FindAddress("ReleaseAI");

		ai = GetNewAI();
//		gh = SAFE_NEW CGroupHandler(team);
		callback = SAFE_NEW CGlobalAICallback(this);
		try {
			ai->InitAI(callback, team);
		} HANDLE_EXCEPTION;

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
			try {
				ReleaseAI(ai);
			} HANDLE_EXCEPTION;
		}

		delete lib;
		delete callback;
//		delete gh;
	}
}

void CGlobalAI::Serialize(creg::ISerializer *s)
{

}

void CGlobalAI::Load(std::istream *s)
{
	try {
		ai->Load(callback,s);
	} HANDLE_EXCEPTION;
}

void CGlobalAI::Save(std::ostream *s)
{
	try {
		ai->Save(s);
	} HANDLE_EXCEPTION;
}

void CGlobalAI::PostLoad()
{
	if (!filesystem.GetFilesize(dllName.c_str())) {
		handleerror(NULL, dllName.c_str(), "Could not find AI lib", MBF_OK | MBF_EXCL);
		return;
	}
	typedef bool (* ISLOADSUPPORTED)();
	ISLOADSUPPORTED _IsLoadSupported;

	lib = SharedLib::Instantiate(dllName.c_str());
	_IsCInterface = (ISCINTERFACE) lib -> FindAddress("IsCInterface");
	_IsLoadSupported = (ISLOADSUPPORTED) lib -> FindAddress("IsLoadSupported");
	bool IsLoadSupported = _IsLoadSupported && _IsLoadSupported();

	// check if IsCInterface function exported and return value true
	if ( _IsCInterface != 0 && _IsCInterface() == 1) {
		// presents C interface
		logOutput << dllName.c_str() <<  " has C interface\n";
		IsCInterface = true;

		// keep as AbicProxy, so InitAI works ok
		AbicProxy* ai = SAFE_NEW AbicProxy;
		this->ai = ai;

		callback = SAFE_NEW CGlobalAICallback(this);
		if (!IsLoadSupported) {
			try {
				ai->InitAI(dllName.c_str(), callback, team);
			} HANDLE_EXCEPTION;
		}
	} else {
		// presents C++ interface
		logOutput << dllName.c_str() <<  " has C++ interface\n";
		IsCInterface = false;

		GetGlobalAiVersion = (GETGLOBALAIVERSION) lib->FindAddress("GetGlobalAiVersion");

		if (GetGlobalAiVersion == 0) {
			handleerror(NULL, dllName.c_str(), "Incorrect Global AI dll", MBF_OK|MBF_EXCL);
			return;
		}

		int i = GetGlobalAiVersion();

		if (i != GLOBAL_AI_INTERFACE_VERSION) {
			handleerror(NULL, dllName.c_str(), "Incorrect Global AI dll version", MBF_OK | MBF_EXCL);
			return;
		}

		GetNewAI = (GETNEWAI) lib->FindAddress("GetNewAI");
		ReleaseAI = (RELEASEAI) lib->FindAddress("ReleaseAI");

		ai = GetNewAI();
		callback = SAFE_NEW CGlobalAICallback(this);
		if (!IsLoadSupported) {
			try {
				ai->InitAI(callback, team);
			} HANDLE_EXCEPTION;
		}
	}
	if (!IsLoadSupported) {
		for (int a = 0; a < MAX_UNITS; a++) {
			if (!uh->units[a]) continue;
			if (uh->units[a]->team==team) {
				try {
					ai->UnitCreated(a);
				} HANDLE_EXCEPTION;
				if (!uh->units[a]->beingBuilt)
					try {
						ai->UnitFinished(a);
					} HANDLE_EXCEPTION;
			} else {
				if ((uh->units[a]->allyteam==gs->AllyTeam(team))||gs->Ally(gs->AllyTeam(team),uh->units[a]->allyteam)) {

				} else {
					if (uh->units[a]->losStatus[gs->AllyTeam(team)] & (LOS_INRADAR|LOS_INLOS)) {
						try {
							ai->EnemyEnterRadar(a);
						} HANDLE_EXCEPTION;
					}
					if (uh->units[a]->losStatus[gs->AllyTeam(team)] & LOS_INLOS) {
						try {
							ai->EnemyEnterLOS(a);
						} HANDLE_EXCEPTION;
					}
				}
			}
		}
	}
}

void CGlobalAI::Update(void)
{
//	gh->Update();
	ai->Update();
}

IMPLEMENT_PURE_VIRTUAL(IGlobalAI::~IGlobalAI())
