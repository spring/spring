//New construction system

struct CBPlan{ // This is not used for factories building units.
	int builder; // the builder
	int subject; // the thing being built
	bool mobile_mobile; // is this a unit building a unit? Needed to issue unitIdles, else scouters built in nanoblobz wont work...
	bool started; // has construction started?
	float3 pos; // where is the unit being built? Used to tie the new unit to the plan 
};

class CManufacturer{
public:
	CManufacturer(Global* GL);
	virtual ~CManufacturer();
	void Init();
	void UnitCreated(int uid);
	void UnitFinished(int uid);
	void UnitIdle(int uid);
	void UnitDestroyed(int uid);
	void Update();
	void EnemyDestroyed(int eid);
	float3 GetBuildPos(int builder, const UnitDef* target, float3 unitpos);
	float3 GetBuildPlacement(int unit,float3 unitpos,const UnitDef* ud,int spacing);
	bool TaskCycle(CBuilder* i);
	bool CBuild(string name, int unit, int spacing);
	//bool FBuild(string name,int unit,int quantity);
	bool LoadBuildTree(CBuilder* ui);
	void LoadGlobalTree();
	vector<Task> Global_queue;
	map<string,btype> types;
	map<btype,string> typenames;
	btype GetTaskType(string s);
	string GetTaskName(btype type);
	void RegisterTaskPair(string name, btype type);
	void RegisterTaskTypes();
	string GetBuild(int uid, string tag, bool efficiency=true);
	bool CanBuild(int uid,const UnitDef* ud, string name);
private:
	map<string, vector<string> > metatags;
	vector<CBPlan> BPlans;
	vector<CBuilder> builders;
	Global* G;
	bool initialized;
};
