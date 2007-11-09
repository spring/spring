class CUnitConstructionTask : public IModule {
public:
	CUnitConstructionTask(Global* GL, int unit, weak_ptr<CUnitTypeData> builder, weak_ptr<CUnitTypeData> building);
	virtual ~CUnitConstructionTask();
	void RecieveMessage(CMessage &message);
	bool Init();
	void End();
protected:
	string mymessage;
	int unit;
	shared_ptr<CUnitTypeData> builder;
	shared_ptr<CUnitTypeData> building;
};
