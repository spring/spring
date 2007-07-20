class CUnitConstructionTask : public IModule {
public:
	CUnitConstructionTask(Global* GL, int unit, const UnitDef* builder, const UnitDef* building);
	virtual ~CUnitConstructionTask();
	void RecieveMessage(CMessage &message);
	bool Init(boost::shared_ptr<IModule> me);
	void End();
protected:
	string mymessage;
	int unit;
	const UnitDef* builder;
	const UnitDef* building;
};
