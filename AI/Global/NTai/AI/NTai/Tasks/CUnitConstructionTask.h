class CUnitConstructionTask : public IModule {
public:
	CUnitConstructionTask(Global* GL, int unit, const UnitDef* builder, const UnitDef* building);
	virtual ~CUnitConstructionTask();
	void RecieveMessage(CMessage &message);
	bool Init();
	void End();
protected:
	string mymessage;
	int unit;
	const UnitDef* builder;
	const UnitDef* building;
};
