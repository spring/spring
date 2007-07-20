class CLeaveBuildSpotTask : public IModule {
public:
	//CLeaveBuildSpotTask(Global* GL);
	CLeaveBuildSpotTask(Global* GL, int unit, const UnitDef* ud);
	void RecieveMessage(CMessage &message);
	bool Init(boost::shared_ptr<IModule> me);
	void End();
protected:
	string mymessage;
	int unit;
	const UnitDef* ud;
};
