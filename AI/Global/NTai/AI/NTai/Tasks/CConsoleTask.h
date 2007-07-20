class CConsoleTask : public IModule {
public:
	CConsoleTask(Global* GL);
	CConsoleTask(Global* GL, string message);
	void RecieveMessage(CMessage &message);
	bool Init(boost::shared_ptr<IModule> me);
protected:
	string mymessage;
};
