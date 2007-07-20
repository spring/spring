class IBehaviour : public IModule{
	IBehaviour(Global* GL, boost::shared_ptr<IModule> unit);
	virtual ~IBehaviour();
	virtual bool Init(boost::shared_ptr<IModule> me);
	virtual void RecieveMessage(CMessage &message);
	virtual bool IsActive();
	virtual bool SetActive(bool active);
protected:
	virtual void FinishBehaviour();
	boost::shared_ptr<IModule> unit;
	bool active;
};
