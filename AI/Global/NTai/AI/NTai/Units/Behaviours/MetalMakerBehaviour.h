class CMetalMakerBehaviour : public IBehaviour{
public:
	CMetalMakerBehaviour(Global* GL, boost::shared_ptr<IModule> unit);
	virtual ~CMetalMakerBehaviour();
	bool Init(boost::shared_ptr<IModule> me);
	void RecieveMessage(CMessage &message);
private:
	bool turnedOn;
	float energyUse;
};
