class CLeaveBuildSpotTask : public IModule {
public:
	//CLeaveBuildSpotTask(Global* GL);
	CLeaveBuildSpotTask(Global* GL, int unit, weak_ptr<CUnitTypeData> ud);
	void RecieveMessage(CMessage &message);
	bool Init();
	void End();
protected:
	string mymessage;
	int unit;
	shared_ptr<CUnitTypeData> utd;
};
