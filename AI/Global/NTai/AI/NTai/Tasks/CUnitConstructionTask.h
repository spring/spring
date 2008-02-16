namespace ntai {
	class CUnitConstructionTask : public IModule {
	public:
		CUnitConstructionTask(Global* GL, int unit, CUnitTypeData* builder, CUnitTypeData* building);
		virtual ~CUnitConstructionTask();
		void RecieveMessage(CMessage &message);
		bool Init();
		void End();

		bool Succeeded();
	protected:
		bool succeed;
		string mymessage;
		int unit;
		CUnitTypeData* builder;
		CUnitTypeData* building;
	};
}
