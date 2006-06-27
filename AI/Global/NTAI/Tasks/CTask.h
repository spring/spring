#ifndef NTAI_TASK_H
#define NTAI_TASK_H

class CTask{
public:
	virtual void Init(Global* GL);
	virtual void Update();
	virtual void UnitIdle();
	virtual void Event(CEvent e);
	virtual btype GetPrimaryType();
	virtual void Execute(set<int> units);
	virtual ~CTask();
};

#endif
