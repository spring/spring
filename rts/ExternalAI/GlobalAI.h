#ifndef GLOBALAI_H
#define GLOBALAI_H

#include "Object.h"
#include "Platform/SharedLib.h"

class IGlobalAI;
class CGlobalAICallback;
class CGroupHandler;

class CGlobalAI: public CObject
{
public:
	CR_DECLARE(CGlobalAI);
	CGlobalAI(int team, const char* dll);
	~CGlobalAI(void);

	void LoadAILib(int, const char*, bool);

	void Serialize(creg::ISerializer *s);
	void PostLoad();
	void Load(std::istream *s);
	void Save(std::ostream *s);

	void Update(void);
	void PreDestroy(); // called just before all the units are destroyed

	int team;
	bool cheatevents;

	bool IsCInterface;
	bool IsLoadSupported;

	IGlobalAI* ai;
	CGlobalAICallback* callback;

	SharedLib* lib;
	std::string libName;

	typedef bool (*ISCINTERFACE)();
	typedef int (*GETGLOBALAIVERSION)();
	typedef IGlobalAI* (*GETNEWAI)();
	typedef IGlobalAI* (*GETNEWAIBYNAME)(const char*, int);
	typedef void (*RELEASEAI)(IGlobalAI* i);
	typedef bool (*ISLOADSUPPORTED)();
	
	ISCINTERFACE _IsCInterfaceFunc;
	GETGLOBALAIVERSION _GetGlobalAiVersionFunc;
	GETNEWAI _GetNewAIFunc;
	GETNEWAIBYNAME _GetNewAIByNameFunc;
	RELEASEAI _ReleaseAIFunc;
	ISLOADSUPPORTED _IsLoadSupportedFunc;

private:
	void LoadCPPAI(int, const char*, bool, bool, bool);
	void LoadJavaProxyAI();
};

#endif
