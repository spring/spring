#ifndef GLOBALAI_H
#define GLOBALAI_H

#include "Object.h"
#include "Platform/SharedLib.h"

class IGlobalAI;
class CGlobalAICallback;
class CGroupHandler;

class CGlobalAI :
	public CObject
{
public:
	CR_DECLARE(CGlobalAI);
	CGlobalAI(int team, const char* dll);
	~CGlobalAI(void);
	void Serialize(creg::ISerializer *s);
	void PostLoad();
	void Load(std::istream *s);
	void Save(std::ostream *s);

	void Update(void);
	void PreDestroy (); // called just before all the units are destroyed

	int team;
	bool cheatevents;

	bool IsCInterface;

	IGlobalAI* ai;
	CGlobalAICallback* callback;
//	CGroupHandler* gh;

	SharedLib *lib;
	std::string dllName;

	typedef bool (* ISCINTERFACE)();
	typedef int (* GETGLOBALAIVERSION)();
	typedef IGlobalAI* (* GETNEWAI)();
	typedef void (* RELEASEAI)(IGlobalAI* i);
	
	ISCINTERFACE _IsCInterface;
	GETGLOBALAIVERSION GetGlobalAiVersion;
	GETNEWAI GetNewAI;
	RELEASEAI ReleaseAI;
};

#endif
