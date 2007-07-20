class CKeywordConstructionTask : public IModule {
public:
	CKeywordConstructionTask(Global* GL, int unit, btype type);
	void RecieveMessage(CMessage &message);
	bool Init(boost::shared_ptr<IModule> me);
	void Build();
	void End();
protected:
	string mymessage;
	int unit;
	btype type;
	const UnitDef* building;
};
