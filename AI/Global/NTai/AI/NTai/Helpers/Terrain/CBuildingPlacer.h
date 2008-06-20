/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

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

	class CBuildAlgorithm : public IModule{
	public:
		CBuildAlgorithm(CBuildingPlacer* buildalgorithm, IModule* reciever, float3 builderpos, CUnitTypeData* wbuilder, CUnitTypeData* wbuilding, float freespace, CGridManager* blockingmap, const float* heightmap, float3 mapdim, Global* G);

		void RecieveMessage(CMessage &message);
		bool Init();
		void operator()();

		Global* G;
		const float* heightmap;
		float bestDistance;
		float3 mapdim;

		bool valid;
		CGridManager* blockingmap;
		CBuildingPlacer* buildalgorithm;
		IModule* reciever;
		float3 builderpos;
		CUnitTypeData* builder;
		CUnitTypeData* building;
		float freespace;
	};
}
