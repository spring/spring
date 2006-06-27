
class CTaskFactory{
public:
	CTaskFactory(Global* GL);
	virtual ~CTaskFactory();
	void Event(CEvent e);
	CTask* GetTask(string TaskType);
	CTask* GetTask(btype TaskType);
private:
	Global* G;
};
