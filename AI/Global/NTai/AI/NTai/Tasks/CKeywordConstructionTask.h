namespace ntai {
	class CKeywordConstructionTask : public IModule {
	public:
		CKeywordConstructionTask(Global* GL, int unit, btype type);
		void RecieveMessage(CMessage &message);
		bool Init();
		void Build();
		void End();
		bool Succeeded();
	protected:
		bool succeed;
		int unit;
		btype type;
		CUnitTypeData* building;
		CUnitTypeData* utd;
	};
}
