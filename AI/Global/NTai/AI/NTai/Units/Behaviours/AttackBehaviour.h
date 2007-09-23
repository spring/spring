class CAttackBehaviour : public IBehaviour{
public:
	CAttackBehaviour(Global* GL, boost::shared_ptr<IModule> unit);
	virtual ~CAttackBehaviour();
	bool Init(boost::shared_ptr<IModule> me);
	void RecieveMessage(CMessage &message);
private:
	bool engaged;
	int uid;
};
