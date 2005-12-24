
class aiHandler;
class aiUnit;

// renamed CObject from spring source - to prevent namespace collision
class aiObject
{
public:
	void DeleteDeathDependence(aiObject* o);
	void AddDeathDependence(aiObject* o);
	virtual void DependentDied(aiObject* o);
	inline aiObject(){};
	virtual ~aiObject();

	multiset<aiObject*> listeners,listening;
};

class BuildTask;

class aiHandler : public aiObject
{
public:
	aiHandler (CGlobals *g) : globals(g) {}

	virtual void Update () {}
	virtual void UnitDestroyed (aiUnit *unit) {}
	virtual void UnfinishedUnitDestroyed (aiUnit *unit) {}
	virtual void UnitFinished (aiUnit *unit) {}
	virtual void UnitMoveFailed (aiUnit *unit) {}
	virtual void ChatMsg (const char* msg, int player) {}
	virtual void Initialize (CGlobals *g) {}

	// should not register the unit - since it's not finished yet - this is done by dstHandler
	virtual aiUnit* CreateUnit (int id, BuildTask *task) {return 0;} 

	virtual const char *GetName () { return "aiHandler"; }

	CGlobals *globals;
};

class BuildUnit;

enum BuildTaskType
{
	BTF_Resource = 0,
	BTF_Force	= 1,
	BTF_Defense	= 2,
	BTF_Recon	= 3
};

class aiTask;

class TaskFactory : public aiHandler
{
public:
	TaskFactory(CGlobals *g) : aiHandler(g){}
	virtual BuildTask* GetNewBuildTask () = 0; // non-build tasks have to be created in Update()

	// Automatically called by the build handler, no need to do it in GetNewTask()
	void DependentDied (aiObject *o);
	void AddTask (aiTask *t);

	vector<aiTask *> activeTasks;
};

#define UNIT_FINISHED	1
#define UNIT_ENABLED	2
#define UNIT_IDLE		4

class aiUnit : public aiObject
{
public:
	aiUnit () { 
		def=0; 
		id=0; 
		owner=0; 
		flags=0;
	}

	virtual void UnitFinished () {}

	void DependentDied (aiObject *o);

	const UnitDef *def;
	int id;
	aiHandler *owner;
	unsigned long flags;
};


