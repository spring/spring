namespace ntai {
	class CBuildingPlacer : public IModule {
	public:
		CBuildingPlacer(Global* GL);
		~CBuildingPlacer();
		void RecieveMessage(CMessage &message);
		bool Init();

		void GetBuildPosMessage(IModule* reciever, int builderID, float3 builderpos, CUnitTypeData* builder, CUnitTypeData* building, float freespacep);

		void Block(float3 pos, CUnitTypeData* utd);
		void Block(float3 pos);
		void Block(float3 pos, float radius);

		void UnBlock(float3 pos, CUnitTypeData* utd);
		void UnBlock(float3 pos);
		void UnBlock(float3 pos, float radius);

		CGridManager blockingmap;
		map<int,float3> tempgeo;
		vector<float3> geolist;

	protected:
	};
}
