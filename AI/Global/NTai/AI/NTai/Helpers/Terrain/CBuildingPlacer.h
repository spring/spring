class CBuildingPlacer : public IModule {
public:
	CBuildingPlacer(Global* GL);
	~CBuildingPlacer();
	void RecieveMessage(CMessage &message);
	bool Init(boost::shared_ptr<IModule> me);

	//float3 GetBuildPos(float3 builderpos, const UnitDef* builder, const UnitDef* building, float freespace);
	void GetBuildPosMessage(boost::shared_ptr<IModule> reciever, int builderID, float3 builderpos, const UnitDef* builder, const UnitDef* building, float freespacep);
	//float3 findfreespace(const UnitDef* building,float3 MapPos, float buildingradius, float searchradius);

	void Block(float3 pos, const UnitDef* ud);
	void Block(float3 pos);
	void Block(float3 pos, float radius);

	void UnBlock(float3 pos, const UnitDef* ud);
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
