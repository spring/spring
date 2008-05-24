#include "StdAfx.h"
#include "GlobalAI.h"
#include "IGlobalAI.h"
#include "GlobalAICallback.h"
#include "GlobalAIHandler.h"
#include "GroupHandler.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "LogOutput.h"
#include "mmgr.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include <sstream>

CR_BIND_DERIVED(CGlobalAI, CObject, (0, NULL))
CR_REG_METADATA(CGlobalAI, (
	CR_MEMBER(team),
	CR_MEMBER(cheatevents),
	CR_MEMBER(libName),
	CR_MEMBER(IsCInterface),
	CR_MEMBER(IsLoadSupported),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

void AIException(const char *what);

#define HANDLE_EXCEPTION					\
	catch (const std::exception& e) {		\
		if (globalAI->CatchException()) {	\
			AIException(e.what());			\
			throw;							\
		} else throw;						\
	}										\
	catch (const char *s) {	\
		if (globalAI->CatchException()) {	\
			AIException(s);					\
			throw;							\
		} else throw;						\
	}										\
	catch (...) {							\
		if (globalAI->CatchException()) {	\
			AIException(0);					\
			throw;							\
		} else throw;						\
	}

CGlobalAI::CGlobalAI(int team, const char* botLibName): team(team), cheatevents(false), libName(botLibName? botLibName: "")
{
	LoadAILib(team, botLibName, false);
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
				_ReleaseAIFunc(ai);
			} HANDLE_EXCEPTION;
		}

		delete lib;
		delete callback;
	}
}

void CGlobalAI::Serialize(creg::ISerializer* s)
{
}



void CGlobalAI::Load(std::istream* s)
{
	try {
		ai->Load(callback, s);
	} HANDLE_EXCEPTION;
}

void CGlobalAI::Save(std::ostream* s)
{
	try {
		ai->Save(s);
	} HANDLE_EXCEPTION;
}



void CGlobalAI::PostLoad()
{
	LoadAILib(team, libName.c_str(), true);
}



void CGlobalAI::LoadAILib(int team, const char* botLibName, bool postLoad)
{
	ai = 0;

	if (!botLibName) {
		// no AI for this team
		return;
	}

	if (!filesystem.GetFilesize(botLibName)) {
		char msg[512];
		SNPRINTF(msg, 511, "Could not find GlobalAI library \"%s\"", botLibName);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}

	bool isJavaAI = (strstr(botLibName, ".jar") == (botLibName + strlen(botLibName) - 4));

	if (isJavaAI) {
		// Java AI, need to load the JAI proxy first
		LoadJavaProxyAI();
	} else {
		lib = SharedLib::Instantiate(botLibName);
	}

	// the JAI proxy is C++ anyway
	_IsCInterfaceFunc = (isJavaAI)? 0: (ISCINTERFACE) lib->FindAddress("IsCInterface");
	_IsLoadSupportedFunc = (ISLOADSUPPORTED) lib->FindAddress("IsLoadSupported");

	IsCInterface = (_IsCInterfaceFunc != 0 && _IsCInterfaceFunc() == 1);
	IsLoadSupported = (_IsLoadSupportedFunc != 0 && _IsLoadSupportedFunc());

	//if (IsCInterface) {
	//	LoadABICAI(team, botLibName, postLoad, IsLoadSupported);
	//} else {
		LoadCPPAI(team, botLibName, postLoad, IsLoadSupported, isJavaAI);
	//}


	if (postLoad && !IsLoadSupported) {
		// fallback code to help the AI if it
		// doesn't implement load/save support
		for (int a = 0; a < MAX_UNITS; a++) {
			if (!uh->units[a])
				continue;

			if (uh->units[a]->team == team) {
				try {
					ai->UnitCreated(a);
				} HANDLE_EXCEPTION;
				if (!uh->units[a]->beingBuilt)
					try {
						ai->UnitFinished(a);
					} HANDLE_EXCEPTION;
			} else {
				if ((uh->units[a]->allyteam == gs->AllyTeam(team)) || gs->Ally(gs->AllyTeam(team), uh->units[a]->allyteam)) {
					/* do nothing */
				} else {
					if (uh->units[a]->losStatus[gs->AllyTeam(team)] & (LOS_INRADAR | LOS_INLOS)) {
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

void CGlobalAI::LoadCPPAI(int team, const char* botLibName, bool postLoad, bool loadSupported, bool isJavaAI)
{
	if (isJavaAI) {
		logOutput << botLibName << " is a Java archive\n";
	} else {
		logOutput << botLibName << " has a C++ interface\n";
	}

	_GetGlobalAiVersionFunc = (GETGLOBALAIVERSION) lib->FindAddress("GetGlobalAiVersion");

	if (_GetGlobalAiVersionFunc == 0) {
		char msg[512];
		SNPRINTF(msg, 511, "Incorrect GlobalAI library \"%s\" (no \"GetGlobalAiVersion\" function exported)", botLibName);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}

	const int botInterfaceVersion = _GetGlobalAiVersionFunc();

	if (botInterfaceVersion != GLOBAL_AI_INTERFACE_VERSION) {
		char msg[1024];
		SNPRINTF(msg, 1023,
			"Incorrect GlobalAI library \"%s\"\n"
			"(lib interface version %d, engine interface version %d)",
			botLibName, botInterfaceVersion, GLOBAL_AI_INTERFACE_VERSION);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}


	if (isJavaAI) {
		// we want to load a Java AI inside a jar,
		// pass the name of the actual .jar to the
		// proxy library so it can spawn a JVM for
		// that AI
		_GetNewAIByNameFunc = (GETNEWAIBYNAME) lib->FindAddress("GetNewAIByName");

		if (_GetNewAIByNameFunc == 0) {
			throw std::runtime_error("JAI proxy does not export \"GetNewAIByName\"");
		}

		// note: team parameter is unnecessary
		ai = _GetNewAIByNameFunc(botLibName, team);
	} else {
		_GetNewAIFunc = (GETNEWAI) lib->FindAddress("GetNewAI");

		if (_GetNewAIFunc == 0) {
			char msg[512];
			SNPRINTF(msg, 511, "GlobalAI library \"%s\" does not export \"GetNewAI\"", botLibName);
			throw std::runtime_error(msg);
		}

		ai = _GetNewAIFunc();
	}

	// note: verify that this is really exported too?
	_ReleaseAIFunc = (RELEASEAI) lib->FindAddress("ReleaseAI");
	callback = SAFE_NEW CGlobalAICallback(this);

	if (!postLoad || (postLoad && !loadSupported)) {
		try {
			ai->InitAI(callback, team);
		} HANDLE_EXCEPTION;
	}
}


void CGlobalAI::LoadJavaProxyAI()
{
	// TODO: Mac support? non-hardcoded proxy?
	#ifdef WIN32
	const char* javaProxyAI = "AI\\Bot-libs\\JAI\\JAI.dll";
	#else
	const char* javaProxyAI = "AI/Bot-libs/JAI/JAI.so";
	#endif

	if (!filesystem.GetFilesize(javaProxyAI)) {
		char msg[512];
		SNPRINTF(msg, 511, "Could not find Java GlobalAI proxy library \"%s\"", javaProxyAI);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}

	lib = SharedLib::Instantiate(javaProxyAI);
}


void CGlobalAI::Update(void)
{
	ai->Update();
}

IMPLEMENT_PURE_VIRTUAL(IGlobalAI::~IGlobalAI())
