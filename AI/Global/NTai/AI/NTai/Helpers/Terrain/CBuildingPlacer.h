class CBuildingPlacer : public IModule {
public:
	CBuildingPlacer(Global* GL);
	~CBuildingPlacer();
	void RecieveMessage(CMessage &message);
	bool Init();

	//float3 GetBuildPos(float3 builderpos, const UnitDef* builder, const UnitDef* building, float freespace);
	void GetBuildPosMessage(IModule* reciever, int builderID, float3 builderpos, weak_ptr<CUnitTypeData> wbuilder, weak_ptr<CUnitTypeData> wbuilding, float freespacep);
	//float3 findfreespace(const UnitDef* building,float3 MapPos, float buildingradius, float searchradius);

	void Block(float3 pos, weak_ptr<CUnitTypeData> utd);
	void Block(float3 pos);
	void Block(float3 pos, float radius);

	void UnBlock(float3 pos, weak_ptr<CUnitTypeData> utd);
	void UnBlock(float3 pos);
	void UnBlock(float3 pos, float radius);

	CGridManager blockingmap;
	//CGridManager slopemap;
	//CGridManager lowheightmap;
	//CGridManager highheightmap;
	map<int,float3> tempgeo;
	vector<float3> geolist;
	//ThreadPool* pool;
	//CWorkerThread* c;
//	CGridManager mexmap;
//	CGridManager geomap;
protected:
};
