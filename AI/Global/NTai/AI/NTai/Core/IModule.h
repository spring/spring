/*
AF 2007
*/

class IModule {
public:
	IModule(){
		G = 0;
		valid = false;
	}
	IModule(Global* GL);
	virtual ~IModule();
	virtual void RecieveMessage(CMessage &message)=0;
	virtual bool Init(boost::shared_ptr<IModule> me)=0;
	void End(){}
	bool IsValid(){
		return valid;
	}
	bool SetValid(bool isvalid){
		valid = isvalid;
		return valid;
	}

	void AddListener(boost::shared_ptr<IModule> module);
	void RemoveListener(boost::shared_ptr<IModule> module);
	void FireEventListener(CMessage &message);
	void DestroyModule();

	void operator()(){}
	Global* G;
protected:
	bool valid;
	boost::shared_ptr<IModule> me;
	set<boost::shared_ptr<IModule> > listeners;
};
