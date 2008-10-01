namespace ntai {
	class CUnitConstructionTask : public IModule {
	public:
		CUnitConstructionTask(Global* GL, int unit, CUnitTypeData* builder, CUnitTypeData* building);
		virtual ~CUnitConstructionTask();
		void RecieveMessage(CMessage &message);
		bool Init();
		void End();
	protected:
		string mymessage;
		int unit;
		CUnitTypeData* builder;
		CUnitTypeData* building;
	};
}
