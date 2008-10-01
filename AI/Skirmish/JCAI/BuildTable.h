//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#define NUM_BUILD_ASSIST 1

enum CachedDefFlags
{
	CUD_Building		= 0x01,
	CUD_Builder			= 0x02,
	CUD_WindGen			= 0x04,
	CUD_AADefense		= 0x08,
	CUD_GroundDefense	= 0x10,
	CUD_MoveType		= 0x20, // unit has a movetype
};

class BuildTable
{
public:
	typedef std::vector<int> BuilderList;

	BuildTable();
	~BuildTable();

	struct ResInfo
	{
		ResInfo() { metal=energy=buildtime=0.0f; }
		float metal, energy,buildtime;
	};

	struct Table
	{
		// memory use is about 900k for 284 unit defs - nothing compared to springs usage...
		struct ent
		{
			ent() { id=-1;depth=0; } 
			ResInfo res;
			short id, depth;
			void set(short ID, short DEPTH) { id=ID; depth=DEPTH; }
		};
		
		Table() { data=0; }
		~Table() { delete[] data; }

		ent& get(int builder, int target) { return data[builder*size + target]; }
		void alloc(int numdef) { data=new ent[numdef*numdef]; size=numdef; }
		int datasize() { return sizeof(ent) * size * size; }

		ent* data;
		int size;
	};

	struct UDef	{
		UDef( ) { def=0; flags=0;}
		string name;
		ResourceInfo storage, cost, make, use;
		float metalExtractDepth, buildTime, energyUse, buildSpeed;
		const UnitDef* def;
		int numBuildOptions;
		ulong flags;
		MoveData::MoveType moveType;
		float weaponRange; // highest range of unit weapons
		float weaponDamage; // all weapon damages summed
		vector<int> *buildby;

		bool IsBuilder() { return (flags & CUD_Builder) != 0; }
		bool IsBuilding() { return (flags & CUD_Building) != 0; }
		bool IsShip() { return (flags & CUD_MoveType) && moveType == MoveData::Ship_Move; }
	};

	int numDefs;
	Table table;

	int buildAssistTypes[NUM_BUILD_ASSIST];

	UDef* deflist;
	vector<int> *buildby;
	vector<int> builders;
	IAICallback *cb;

	const UnitDef* GetDef (UnitDefID id);
	const UnitDef* GetDef (Table::ent* ent);
	int GetDefID (const char *name);
	UDef* GetCachedDef(UnitDefID i) { return &deflist[i-1]; }

	void CalcBuildTable(int cur, int endbuild, int depth, ResInfo res);
	void Init (IAICallback *cb, bool bLoadFromCache);

	bool UnitCanBuild (const UnitDef *builder, const UnitDef *constr);
	BuilderList* GetBuilderList(const UnitDef* b);
	Table::ent& GetBuildPath(const UnitDef* builder, const UnitDef* target) 
	{
		return table.get(builder->id-1, target->id-1);
	}
	Table::ent& GetBuildPath(int builder, int target)
	{
		return table.get(builder-1,target-1);
	}

	// score calculation for table generating heuristic
	inline float CalcScore(int depth, ResInfo& res)
	{
		return res.buildtime + res.metal * 20 + res.energy;
	}

	// loading&saving
	bool LoadCache (const char *fn);
	void SaveCache (const char *fn);
};


typedef BuildTable::Table::ent BuildPathNode;

extern BuildTable buildTable;

