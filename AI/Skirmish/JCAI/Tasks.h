#ifndef JC_TASKS_H
#define JC_TASKS_H

class BuildUnit;

#define BT_BUILDER 1	// should this be added to the buildhandler?
#define BT_BUILDING 4

class aiTask : public aiObject
{
public:
	aiTask() { startFrame=0; depends=0; flags=0; buildSpeed=0.0f; retries=0; }
	virtual ~aiTask() {}

	void DependentDied (aiObject *obj);
	// Helper for the aiTask implementations
	void GiveConstructorCommand (IAICallback *cb, Command *cmd);

	// Inline Helpers
	bool IsBuilder() { return (flags&BT_BUILDER)!=0; }
	void MarkBuilder() { flags |= BT_BUILDER; }

	// Returns false when task should be deleted
	virtual bool Update (CGlobals *gbl) = 0;
	virtual bool CalculateScore (BuildUnit *unit, CGlobals *gbl, float& score) = 0;
	virtual bool IsTaskFailed (BuildUnit *unit, CGlobals *g) { return false; }
	// Orders the builder to perform the task - the builder is already in the constructors list
	virtual void CommandBuilder (BuildUnit *builder, CGlobals *g) = 0;
	virtual void BuilderDestroyed (BuildUnit *builder) {}
	virtual void Initialize (CGlobals *g) {}
	virtual string GetDebugName(CGlobals *g) {return string();}
	virtual void BuilderMoveFailed (BuildUnit *builder,CGlobals *g) {}


	int flags;
	aiTask *depends; // depends task will be executed before this will
	int startFrame; // frame of when the task was added
	float buildSpeed; // buildspeed of all constructors

	int retries; // UnitMoveFailed triggers a task retry
	enum { MaxRetries=3 };

	vector <BuildUnit *> constructors;

	int index; // index for ptrvec
};

class ReclaimUnitTask : public aiTask
{
public:
	ReclaimUnitTask() { target=0; }

	// Set the unit that will be reclaimed
	void SetReclaimTarget(aiUnit *u);

	bool Update(CGlobals *g);
	bool CalculateScore (BuildUnit *unit, CGlobals *gbl, float& score);
	void DependentDied (aiObject *obj);
	void CommandBuilder(BuildUnit *builder, CGlobals *g);
	string GetDebugName (CGlobals *g);

protected:
	aiUnit *target;// original extractor unit
};

class ReclaimFeaturesTask : public aiTask
{
public:
	float3 pos;
	float range;

	bool Update(CGlobals *g);
	bool CalculateScore (BuildUnit *unit, CGlobals *g, float&score);
	void CommandBuilder (BuildUnit *builder, CGlobals *g);
	string GetDebugName (CGlobals *g);
};


class RepairTask : public aiTask
{
public:
	RepairTask(){target=0;}

	void SetRepairTarget (aiUnit *u);

	bool CalculateScore (BuildUnit *unit, CGlobals *gbl, float& score);
	bool Update(CGlobals *g);
	void DependentDied (aiObject *obj);
	void CommandBuilder (BuildUnit *u, CGlobals *g);
	string GetDebugName (CGlobals *g);

protected:
	aiUnit *target;
};



class BuildTask : public aiTask
{
public:
	BuildTask(const UnitDef *unitDef);

	bool IsBuilding() { return (flags&BT_BUILDING)!=0; }

	// aiTask interface
	bool CalculateScore (BuildUnit *unit, CGlobals *gbl, float& score);
	void DependentDied(aiObject *obj);
	bool Update(CGlobals *g);
	void CommandBuilder (BuildUnit *builder, CGlobals *g);
	void BuilderDestroyed (BuildUnit *builder, CGlobals *g);
	void Initialize (CGlobals *g);
	string GetDebugName(CGlobals *g);
	void BuilderMoveFailed (BuildUnit *builder,CGlobals *g);

	// These have to be implemented by classes derived from BuildTask
	virtual bool InitBuildPos (CGlobals *g);
	virtual int GetBuildSpace (CGlobals *g);

	const UnitDef *def;
	bool isAllocated;
	BuildUnit *lead; // lead builder
	int spotID; // extractor spot ID
	aiUnit *unit;// the unit that is being constructed
	float3 pos;
	int resourceType;
	aiHandler *destHandler;

	bool SetupBuildLocation (CGlobals *g);
	void InitializeLeadBuilder (CGlobals *g);

	friend class ResourceManager;
};

class DbgWindow;
class BuildTasksConfig;


// Tracks resources and provides an "allocation" system for BuildTask's
class ResourceManager
{
public:
	struct Config
	{
		float BuildWeights[NUM_TASK_TYPES];
		int MaxTasks[NUM_TASK_TYPES];

		bool Load (CfgList *sidecfg);
	} config;

	static const char *handlerStr[];

	ResourceManager(CGlobals *g);

	ResourceInfo totalBuildPower;
	ResourceInfo buildMultiplier;
	ResourceInfo averageProd, averageUse;

	// What reserves are avaiable per task type?
	float taskResources [NUM_TASK_TYPES];
	CGlobals *globals;

	void TakeResources (float v);
	void DistributeResources ();
	bool AllocateForTask (float energyCost, float metalCost, int resourceType); // allocates the resources required for this task
	void Update();
	void UpdateBuildMultiplier();
	void SpillResources (const ResourceInfo& res, int type);
	
	void ShowDebugInfo (DbgWindow *wnd);

	inline float ResourceValue (float energy, float metal) {
		return energy * 0.05f + metal;
	}
	inline float ResourceValue (const ResourceInfo& res) {
		return res.energy * 0.05f + res.metal;
	}
};

#endif

