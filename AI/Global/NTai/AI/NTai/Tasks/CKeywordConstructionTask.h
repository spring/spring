class CKeywordConstructionTask : public IModule {
public:
	CKeywordConstructionTask(Global* GL, int unit, btype type);
	void RecieveMessage(CMessage &message);
	bool Init();
	void Build();
	void End();
protected:
	string mymessage;
	int unit;
	btype type;
	CUnitTypeData* building;
	CUnitTypeData* utd;
};
