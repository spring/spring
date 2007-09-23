
class IBehaviour : public IModule{
public:
	IBehaviour(){}
	IBehaviour(Global* GL, boost::shared_ptr<IModule> unit){}
	virtual ~IBehaviour(){}
	virtual bool Init(boost::shared_ptr<IModule> me){ return false;}
	virtual void RecieveMessage(CMessage &message){}
	inline bool IsActive(){
		return active;
	}
	inline bool SetActive(bool active){
		this->active = active;
	}
protected:
	//virtual void FinishBehaviour();
	boost::shared_ptr<IModule> unit;
	bool active;
};
