
namespace ntai {
	class CLeaveBuildSpotTask : public IModule {
	public:
		CLeaveBuildSpotTask(Global* GL, int unit, CUnitTypeData* ud);
		void RecieveMessage(CMessage &message);
		bool Init();
		void End();
		bool Succeeded();
	protected:
		bool succeed;
		string mymessage;
		int unit;
		CUnitTypeData* utd;
	};
}
